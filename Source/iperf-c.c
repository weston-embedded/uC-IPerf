/*
*********************************************************************************************************
*                                              uC/IPerf
*                                 TCP-IP Transfer Measurement Utility
*
*                    Copyright 2009-2020 Silicon Laboratories Inc. www.silabs.com
*
*                                 SPDX-License-Identifier: APACHE-2.0
*
*               This software is subject to an open source license and is distributed by
*                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
*                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                            IPERF CLIENT
*
* Filename : iperf-c.c
* Version  : V2.04.00
*********************************************************************************************************
* Note(s)  : (1) Assumes uC/TCP-IP V2.05 (or more recent version) is included in the project build.
*
*            (2) (a) (1) uC/IPerf is designed to work  with                NLANR IPerf 2.0.2 or higher.
*
*                    (2) uC/IPerf should be compatible with kPerf or jPerf using IPerf 2.0.2 or higher.
*
*                (b) Supports NLANR Iperf with the following restrictions/constraints :
*
*                    (1) TCP:
*                        (A) Multi-threaded                                NOT supported on both   mode
*                        (B) Measure bandwith                                  Supported on both   mode
*                        (C) Report MSS/MTU size & observed read sizes     NOT supported on both   mode
*                        (D) Support for TCP window size via socket buffer     Supported on server mode
*
*                    (2) UDP:
*                        (A) Multi-threaded                                NOT supported on both   mode
*                        (B) Create UDP streams of specified bandwidth     NOT supported on client mode
*                        (C) Measure packet loss                               Supported on server mode
*                        (D) Measure delay jitter                          NOT supported on both   mode
*                        (E) Multicast capable                             NOT supported on both   mode
*
*                (c) More information about NLANR IPerf can be obtained online at
*                    http://www.onl.wustl.edu/restricted/iperf.html.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#define    MICRIUM_SOURCE
#define    IPERF_CLIENT_MODULE
#include  "iperf.h"
#include  <Source/net_util.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*
* Note(s) : (1) See 'iperf.h  MODULE CONFIGURATION'.
*********************************************************************************************************
*********************************************************************************************************
*/

#ifdef  IPERF_CLIENT_MODULE_PRESENT


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void         IPerf_ClientSocketInit(IPERF_TEST   *p_test,
                                            IPERF_ERR    *p_err);


static  void         IPerf_ClientTCP       (IPERF_TEST   *p_test,
                                            IPERF_ERR    *p_err);


static  void         IPerf_ClientUDP       (IPERF_TEST   *p_test,
                                            IPERF_ERR    *p_err);


static  void         IPerf_ClientTxUDP_FIN (IPERF_TEST   *p_test,
                                            CPU_CHAR     *p_data_buf);


static  CPU_BOOLEAN  IPerf_ClientTx        (IPERF_TEST   *p_test,
                                            CPU_CHAR     *p_data_buf,
                                            CPU_INT16U    retry_max,
                                            CPU_INT32U    time_dly_ms,
                                            IPERF_ERR    *p_err);


static  void         IPerf_ClientPattern   (CPU_CHAR     *cp,
                                            CPU_INT16U    cnt);


/*
*********************************************************************************************************
*                                         IPerf_ClientStart()
*
* Description : (1) Process IPerf as a client :
*
*                   (a) Initialize socket
*                   (b) Run TCP or UDP transmitter
*                   (c) Close used socket
*
*
* Argument(s) : p_test      Pointer to a test.
*               ------      Argument validated in IPerf_Init(),
*                                    checked   in IPerf_TestTaskHandler().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               IPERF_ERR_NONE                      successfully completed.
*                               IPERF_ERR_CLIENT_SOCK_CLOSE     NOT successfully closed
*
*                                                               - RETURNED BY IPerf_TestGet() : -
*                               IPERF_ERR_CLIENT_SOCK_OPEN      NOT successfully opened.
*                               IPERF_ERR_CLIENT_SOCK_BIND      NOT successfully bond.
*                               IPERF_ERR_CLIENT_SOCK_CONN      NOT successfully connected.
*
*                                                               - RETURNED BY IPerf_TestGet() : -
*                               IPERF_ERR_CLIENT_SOCK_TX        Error on transmission.
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_TestTaskHandler().
*
*               This function is an INTERNAL IPerf function & MUST NOT be called by application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/
void  IPerf_ClientStart (IPERF_TEST  *p_test,
                         IPERF_ERR   *p_err)
{
    IPERF_OPT   *p_opt;
    IPERF_CONN  *p_conn;
    NET_ERR      err;


    p_conn = &p_test->Conn;
    p_opt  = &p_test->Opt;

    IPERF_TRACE_DBG(("\n\r-------------IPerf DBG CLIENT START-------------\n\r"));
    IPERF_TRACE_DBG(("Init Client socket.\n\r"));



    IPerf_ClientSocketInit(p_test, p_err);                      /* -------------------- INIT SOCK --------------------- */

    if (*p_err != IPERF_ERR_NONE) {
        return;
    }


                                                                /* --------------- TCP/UDP TRANSMITTER ---------------- */
    if (p_opt->Protocol == IPERF_PROTOCOL_TCP) {
        IPerf_ClientTCP(p_test, p_err);
    } else if (p_opt->Protocol == IPERF_PROTOCOL_UDP) {
        IPerf_ClientUDP(p_test, p_err);
    }


                                                                /* -------------------- CLOSE SOCK -------------------- */
    if (p_conn->SockID != NET_SOCK_ID_NONE) {
       (void)NetApp_SockClose((NET_SOCK_ID) p_conn->SockID,
                              (CPU_INT32U ) 0u,
                              (NET_ERR   *)&err);
        switch (err) {
            case NET_APP_ERR_NONE:
                 break;


            case NET_APP_ERR_FAULT_TRANSITORY:
            case NET_APP_ERR_FAULT:
            case NET_APP_ERR_INVALID_ARG:
            default:
                 IPERF_TRACE_DBG(("Closing socket error: %u.\n\r", (unsigned int)err));
                 if (*p_err == IPERF_ERR_NONE) {                /* Don't erase previous err.                            */
                     *p_err =  IPERF_ERR_CLIENT_SOCK_CLOSE;
                 }
                 break;
        }
    }

    IPERF_TRACE_DBG(("\n\r-------------IPerf DBG CLIENT END-------------\n\r"));
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                      IPerf_ClientSocketInit()
*
* Description : (1) Initialize one socket for client use :
*
*                   (a) Open a socket
*                   (b) If bind client is enabled , bind the socket on local address & the same port as server
*                   (c) Connect to remote addr & port
*
*
* Argument(s) : p_test      Pointer to a test.
*               ------      Argument validated in IPerf_Init(),
*                                    checked   in IPerf_ClientStart(),
*                                                   by IPerf_TestTaskHandler().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               IPERF_ERR_NONE                  socket     successfully initialized
*                               IPERF_ERR_CLIENT_SOCK_OPEN      socket NOT successfully opened.
*                               IPERF_ERR_CLIENT_SOCK_BIND      socket NOT successfully bond.
*                               IPERF_ERR_CLIENT_SOCK_CONN      socket NOT successfully connected.
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_Server_Start().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  IPerf_ClientSocketInit (IPERF_TEST  *p_test,
                                      IPERF_ERR   *p_err)
{
    IPERF_OPT             *p_opt;
    IPERF_CONN            *p_conn;
    NET_SOCK_ID            sock_id;
    NET_SOCK_ADDR_FAMILY   addr_family;
    NET_ERR                err;
    CPU_INT16U             server_port;
    CPU_BOOLEAN            cfg_succeed;
#if (IPERF_CFG_CLIENT_BIND_EN == DEF_ENABLED)
    NET_IF_NBR             if_nbr;
    NET_IP_ADDRS_QTY       addr_tbl_size;
#ifdef  NET_IPv4_MODULE_EN
    NET_IPv4_ADDR          addr_ipv4_tbl[NET_IPv4_CFG_IF_MAX_NBR_ADDR];
#endif
#ifdef  NET_IPv6_MODULE_EN
    NET_IPv6_ADDR          addr_ipv6_tbl[NET_IPv6_CFG_IF_MAX_NBR_ADDR];
#endif
#endif
#ifdef  NET_IPv4_MODULE_EN
    NET_IPv4_ADDR          remote_addr_ipv4;
#endif
#ifdef  NET_IPv6_MODULE_EN
    NET_IPv6_ADDR          remote_addr_ipv6;
#endif


    p_opt  = &p_test->Opt;
    p_conn = &p_test->Conn;



    server_port = p_opt->Port;
    addr_family = ((p_opt->IPv4 == DEF_YES) ? (NET_SOCK_ADDR_FAMILY_IP_V4) : (NET_SOCK_ADDR_FAMILY_IP_V6));

                                                                /* --------------------- OPEN SOCK -------------------- */
    IPERF_TRACE_DBG(("Socket Open "));
    if (p_opt->Protocol == IPERF_PROTOCOL_UDP) {
        IPERF_TRACE_DBG(("UDP : "));
        sock_id = NetApp_SockOpen((NET_SOCK_PROTOCOL_FAMILY) addr_family,
                                  (NET_SOCK_TYPE           ) NET_SOCK_TYPE_DATAGRAM,
                                  (NET_SOCK_PROTOCOL       ) NET_SOCK_PROTOCOL_UDP,
                                  (CPU_INT16U              ) IPERF_OPEN_MAX_RETRY,
                                  (CPU_INT32U              ) IPERF_OPEN_MAX_DLY_MS,
                                  (NET_ERR                *)&err);
    } else {
        IPERF_TRACE_DBG(("TCP : "));
        sock_id = NetApp_SockOpen((NET_SOCK_PROTOCOL_FAMILY) addr_family,
                                  (NET_SOCK_TYPE           ) NET_SOCK_TYPE_STREAM,
                                  (NET_SOCK_PROTOCOL       ) NET_SOCK_PROTOCOL_TCP,
                                  (CPU_INT16U              ) IPERF_OPEN_MAX_RETRY,
                                  (CPU_INT32U              ) IPERF_OPEN_MAX_DLY_MS,
                                  (NET_ERR                *)&err);
    }
    switch (err) {
        case NET_APP_ERR_NONE:
             p_conn->SockID = sock_id;
             IPERF_TRACE_DBG(("Done, Socket ID = %d.\n\r", sock_id));
             break;


        case NET_APP_ERR_FAULT:
        case NET_APP_ERR_NONE_AVAIL:
        case NET_APP_ERR_INVALID_ARG:
        default:
             IPERF_TRACE_DBG(("Fail, error: %u.\n\r", (unsigned int)err));
            *p_err = IPERF_ERR_CLIENT_SOCK_OPEN;
             return;
    }

    switch (addr_family) {
#ifdef  NET_IPv4_MODULE_EN
        case NET_SOCK_PROTOCOL_FAMILY_IP_V4:
             remote_addr_ipv4 = NetASCII_Str_to_IPv4((CPU_CHAR *) p_opt->IP_AddrRemote,
                                                     (NET_ERR  *)&err);
             if (err != NET_ASCII_ERR_NONE) {
                IPERF_TRACE_DBG(("Fail, error: %u.\n\r", (unsigned int)err));
               *p_err = IPERF_ERR_CLIENT_INVALID_IP_FAMILY;
                return;
             }

             NetApp_SetSockAddr(              &p_conn->ServerAddrPort,
                                               NET_SOCK_ADDR_FAMILY_IP_V4,
                                               server_port,
                                (CPU_INT08U *)&remote_addr_ipv4,
                                               sizeof(remote_addr_ipv4),
                                              &err);
             break;
#endif

#ifdef  NET_IPv6_MODULE_EN
        case NET_SOCK_PROTOCOL_FAMILY_IP_V6:
             remote_addr_ipv6 = NetASCII_Str_to_IPv6((CPU_CHAR *) p_opt->IP_AddrRemote,
                                                     (NET_ERR  *)&err);
             if (err != NET_ASCII_ERR_NONE) {
                 IPERF_TRACE_DBG(("Fail, error: %u.\n\r", (unsigned int)err));
                *p_err = IPERF_ERR_CLIENT_INVALID_IP_FAMILY;
                 return;
             }


             NetApp_SetSockAddr(              &p_conn->ServerAddrPort,
                                               NET_SOCK_ADDR_FAMILY_IP_V6,
                                               server_port,
                                (CPU_INT08U *)&remote_addr_ipv6,
                                               sizeof(remote_addr_ipv6),
                                              &err);
             break;
#endif
        default:
             IPERF_TRACE_DBG(("Fail, error: %u.\n\r", (unsigned int)err));
            *p_err = IPERF_ERR_CLIENT_INVALID_IP_FAMILY;
             return;
    }

    cfg_succeed = NetSock_CfgBlock(sock_id, NET_SOCK_BLOCK_SEL_BLOCK, &err);
    if (cfg_succeed != DEF_OK) {
       *p_err = IPERF_ERR_CLIENT_SOCK_OPT;
        return;
    }

#if (IPERF_CFG_CLIENT_BIND_EN == DEF_ENABLED)

   if_nbr      = p_conn->IF_Nbr;
                                                                /* -------------------- BIND SOCK --------------------- */
   Mem_Clr((void     *)&p_conn->ClientAddrPort,
           (CPU_SIZE_T) NET_SOCK_ADDR_SIZE);

    switch (addr_family) {
#ifdef  NET_IPv4_MODULE_EN
        case NET_SOCK_PROTOCOL_FAMILY_IP_V4:
             addr_tbl_size = sizeof(NET_IPv4_ADDR);

            (void)NetIPv4_GetAddrHost((NET_IF_NBR        ) if_nbr,
                                      (NET_IPv4_ADDR    *)&addr_ipv4_tbl[0],
                                      (NET_IP_ADDRS_QTY *)&addr_tbl_size,
                                      (NET_ERR          *)&err);

             NetApp_SetSockAddr(              &p_conn->ClientAddrPort,
                                               NET_SOCK_ADDR_FAMILY_IP_V4,
                                               server_port,
                                (CPU_INT08U *)&addr_ipv4_tbl[0],
                                               NET_IPv4_ADDR_SIZE,
                                              &err);
             break;
#endif

#ifdef  NET_IPv6_MODULE_EN
        case NET_SOCK_PROTOCOL_FAMILY_IP_V6:
             addr_tbl_size = sizeof(addr_ipv6_tbl) / sizeof(NET_IPv6_ADDR);
            (void)NetIPv6_GetAddrHost((NET_IF_NBR        ) if_nbr,
                                      (NET_IPv6_ADDR    *)&addr_ipv6_tbl[0],
                                      (NET_IP_ADDRS_QTY *)&addr_tbl_size,
                                      (NET_ERR          *)&err);

             NetApp_SetSockAddr(              &p_conn->ClientAddrPort,
                                               NET_SOCK_ADDR_FAMILY_IP_V6,
                                               server_port,
                                (CPU_INT08U *)&addr_ipv6_tbl[0],
                                               NET_IPv6_ADDR_SIZE,
                                              &err);
           break;
#endif
        default:
             IPERF_TRACE_DBG(("Fail, error: %u.\n\r", (unsigned int)err));
            *p_err = IPERF_ERR_CLIENT_INVALID_IP_FAMILY;
             return;
    }

    IPERF_TRACE_DBG(("Socket Bind ... "));
   (void)NetApp_SockBind((NET_SOCK_ID      ) p_conn->SockID,
                         (NET_SOCK_ADDR   *)&p_conn->ClientAddrPort,
                         (NET_SOCK_ADDR_LEN) NET_SOCK_ADDR_SIZE,
                         (CPU_INT16U       ) IPERF_BIND_MAX_RETRY,
                         (CPU_INT32U       ) IPERF_BIND_MAX_DLY_MS,
                         (NET_ERR         *)&err);

    switch (err) {
        case NET_APP_ERR_NONE:
             IPERF_TRACE_DBG(("done\r\n\r"));
             break;


        case NET_APP_ERR_NONE_AVAIL:
        case NET_APP_ERR_INVALID_ARG:
        case NET_APP_ERR_INVALID_OP:
        case NET_APP_ERR_FAULT:
        default:
             IPERF_TRACE_DBG(("Fail error: %u.\n\r", (unsigned int)err));
            *p_err = IPERF_ERR_CLIENT_SOCK_BIND;
                                                                /* Close sock.                                          */
            (void)NetApp_SockClose((NET_SOCK_ID) p_conn->SockID,
                                   (CPU_INT32U ) 0u,
                                   (NET_ERR   *)&err);
             return;
    }
#endif

                                                                /* ---------------- REMOTE CONNECTION ----------------- */
                                                                /* Remote IP addr for sock conn.                        */
    IPERF_TRACE_DBG(("Socket Conn ... "));
   (void)NetApp_SockConn((NET_SOCK_ID      ) p_conn->SockID,
                         (NET_SOCK_ADDR   *)&p_conn->ServerAddrPort,
                         (NET_SOCK_ADDR_LEN) NET_SOCK_ADDR_SIZE,
                         (CPU_INT16U       ) IPERF_CFG_CLIENT_CONN_MAX_RETRY,
                         (CPU_INT32U       ) IPERF_CFG_CLIENT_CONN_MAX_TIMEOUT_MS,
                         (CPU_INT32U       ) IPERF_CFG_CLIENT_CONN_MAX_DLY_MS,
                         (NET_ERR         *)&err);
    switch (err) {
        case NET_APP_ERR_NONE:
             IPERF_TRACE_DBG(("Done\n\r"));
             IPERF_TRACE_DBG(("Connected to : %s, port: %u\r\n\r", p_opt->IP_AddrRemote,
                                                     (unsigned int)p_opt->Port));
             break;


        case NET_APP_ERR_FAULT:
        case NET_APP_ERR_INVALID_OP:
        case NET_APP_ERR_NONE_AVAIL:
        case NET_APP_ERR_INVALID_ARG:
        case NET_APP_ERR_FAULT_TRANSITORY:
        default:
             IPERF_TRACE_DBG(("Fail error: %u.\n\r", (unsigned int)err));
            *p_err = IPERF_ERR_CLIENT_SOCK_CONN;
                                                                /* Close sock.                                          */
            (void)NetApp_SockClose((NET_SOCK_ID) p_conn->SockID,
                                   (CPU_INT32U ) 0u,
                                   (NET_ERR   *)&err);
             return;
    }

   *p_err = IPERF_ERR_NONE;

    return;
}


/*
*********************************************************************************************************
*                                          IPerf_ClientTCP()
*
* Description : (1) IPerf TCP client (tranmitter):
*
*                   (a) Initialize buffer
*                   (b) Transmit data until the end of test is reached
*
*
* Argument(s) : p_test      Pointer to a test.
*               ------      Argument validated in IPerf_Init(),
*                                    checked   in IPerf_ClientStart(),
*                                                   by IPerf_TestTaskHandler().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               IPERF_ERR_NONE                  successfully transmitted.
*                               IPERF_ERR_CLIENT_SOCK_TX    NOT successfully completed.
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_ClientStart().
*
* Note(s)     : none.
*********************************************************************************************************
*/
static  void  IPerf_ClientTCP (IPERF_TEST  *p_test,
                               IPERF_ERR   *p_err)
{
    IPERF_OPT    *p_opt;
    IPERF_CONN   *p_conn;
    IPERF_STATS  *p_stats;
    CPU_CHAR     *p_data_buf;
    CPU_BOOLEAN   tx_done;
#if (IPERF_CFG_CPU_USAGE_MAX_CALC_EN == DEF_ENABLED)
    CPU_INT16U    cpu_usage;
#endif
#if (IPERF_CFG_BANDWIDTH_CALC_EN == DEF_ENABLED)
    IPERF_TS_MS   ts_ms_prev;
    CPU_INT32U    tx_bytes_prev;
#endif
    NET_ERR       err;



    p_opt   = &p_test->Opt;
    p_conn  = &p_test->Conn;
    p_stats = &p_test->Stats;
    tx_done =  DEF_NO;


                                                                /* ----------------- INIT BUF PATTERN ----------------- */
#if (IPERF_CFG_ALIGN_BUF_EN == DEF_ENABLED)
    p_data_buf = (CPU_CHAR *)NetIF_GetTxDataAlignPtr((NET_IF_NBR) p_conn->IF_Nbr,
                                                     (void     *)&IPerf_Buf[0],
                                                     (NET_ERR  *)&err);
    if (err != NET_IF_ERR_NONE) {
        p_data_buf = &IPerf_Buf[0];
    }
#else
    p_data_buf = &IPerf_Buf[0];
#endif

    NetSock_CfgTimeoutTxQ_Set(p_conn->SockID, IPERF_CFG_CLIENT_TCP_TX_MAX_TIMEOUT_MS, &err);

    IPerf_ClientPattern(p_data_buf, p_opt->BufLen);

                                                                /* --------------------- TX DATA ---------------------- */
    IPERF_TRACE_DBG(("------------------- TCP START SENDING -------------------\n\r"));
    p_stats->TS_Start_ms = IPerf_Get_TS_ms();
#if (IPERF_CFG_BANDWIDTH_CALC_EN == DEF_ENABLED)
    ts_ms_prev           = p_stats->TS_Start_ms;
    tx_bytes_prev        = 0u;
#endif
    p_conn->Run          = DEF_YES;

    while (tx_done == DEF_NO) {                                 /* Loop until the end of sending process.               */
        tx_done = IPerf_ClientTx((IPERF_TEST *)p_test,
                                 (CPU_CHAR   *)p_data_buf,
                                 (CPU_INT16U  )IPERF_CLIENT_TCP_TX_MAX_RETRY,
                                 (CPU_INT32U  )IPERF_CLIENT_TCP_TX_MAX_DLY_MS,
                                 (IPERF_ERR  *)p_err);

#if (IPERF_CFG_CPU_USAGE_MAX_CALC_EN == DEF_ENABLED)
        cpu_usage = IPerf_OS_CPU_Usage();
        p_stats->CPU_UsageAvg += cpu_usage;
        p_stats->CPU_CalcNbr++;
        if (cpu_usage > p_stats->CPU_UsageMax) {
            p_stats->CPU_UsageMax = cpu_usage;
        }
#endif

#if (IPERF_CFG_BANDWIDTH_CALC_EN == DEF_ENABLED)
        IPerf_UpdateBandwidth(p_test, &ts_ms_prev, &tx_bytes_prev);
#endif
    }

    p_conn->Run = DEF_NO;
    if (*p_err != IPERF_ERR_CLIENT_SOCK_TX) {
        IPERF_TRACE_DBG(("*************** CLIENT TCP RESULT ***************\n\r"));
        IPERF_TRACE_DBG(("Tx Call count  = %u \n\r", (unsigned int)p_stats->NbrCalls));
        IPERF_TRACE_DBG(("Tx Err  count  = %u \n\r", (unsigned int)p_stats->Errs));
        IPERF_TRACE_DBG(("------------------- END SENDING -------------------\n\r"));
       *p_err = IPERF_ERR_NONE;
    }
}


/*
*********************************************************************************************************
*                                          IPerf_ClientUDP()
*
* Description : (1) IPerf UDP client :
*
*                   (a) Initialize test performance statistics & buffer
*                   (b) Transmit data until the end of test is reached
*                   (c) Transmit UDP FIN datagram to finish UDP test
*
*
* Argument(s) : p_test      Pointer to a test.
*               ------      Argument validated in IPerf_Init(),
*                                    checked   in IPerf_ClientStart(),
*                                                   by IPerf_TestTaskHandler().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               IPERF_ERR_NONE                  successfully transmitted.
*                               IPERF_ERR_CLIENT_SOCK_TX    NOT successfully transmitted.
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_TaskHandler().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  IPerf_ClientUDP (IPERF_TEST  *p_test,
                               IPERF_ERR   *p_err)
{
    IPERF_OPT           *p_opt;
    IPERF_CONN          *p_conn;
    IPERF_STATS         *p_stats;
    IPERF_UDP_DATAGRAM  *p_buf;                                 /* UDP datagram buf ptr.                                */
    CPU_CHAR            *p_data_buf;
    CPU_INT32S           pkt_id;
    IPERF_TS_MS          ts_cur_ms;
    CPU_INT32U           tv_sec;
    CPU_INT32U           tv_usec;
    CPU_BOOLEAN          tx_done;
#if (IPERF_CFG_CPU_USAGE_MAX_CALC_EN == DEF_ENABLED)
    CPU_INT16U           cpu_usage;
#endif
#if (IPERF_CFG_BANDWIDTH_CALC_EN == DEF_ENABLED)
    IPERF_TS_MS          ts_ms_prev;
    CPU_INT32U           tx_bytes_prev;
#endif
#if (IPERF_CFG_ALIGN_BUF_EN == DEF_ENABLED)
    NET_ERR              err;
#endif


    p_opt   = &p_test->Opt;
    p_conn  = &p_test->Conn;
    p_stats = &p_test->Stats;
    tx_done =  DEF_NO;
    pkt_id  =  0u;


                                                                /* ----------------- INIT BUF PATTERN ----------------- */
#if (IPERF_CFG_ALIGN_BUF_EN == DEF_ENABLED)
    p_data_buf = (CPU_CHAR *)NetIF_GetTxDataAlignPtr((NET_IF_NBR) p_conn->IF_Nbr,
                                                     (void     *)&IPerf_Buf[0],
                                                     (NET_ERR  *)&err);
    if (err != NET_IF_ERR_NONE) {
        p_data_buf = &IPerf_Buf[0];
    }
#else
    p_data_buf  = &IPerf_Buf[0];
#endif

    IPerf_ClientPattern(p_data_buf, p_opt->BufLen);
    p_buf       = (IPERF_UDP_DATAGRAM *)p_data_buf;
    p_conn->Run =  DEF_YES;


                                                                /* --------------------- TX DATA ---------------------- */
    IPERF_TRACE_DBG(("------------------- UDP START SENDING -------------------\n\r"));
    p_stats->TS_Start_ms = IPerf_Get_TS_ms();
#if (IPERF_CFG_BANDWIDTH_CALC_EN == DEF_ENABLED)
    ts_ms_prev           = p_stats->TS_Start_ms;
    tx_bytes_prev        = 0u;
#endif

    while (tx_done == DEF_NO) {
        ts_cur_ms           =  IPerf_Get_TS_ms();
        tv_sec              =  ts_cur_ms /  DEF_TIME_NBR_mS_PER_SEC;
        tv_usec             = (ts_cur_ms - (tv_sec * DEF_TIME_NBR_mS_PER_SEC)) * (DEF_TIME_NBR_uS_PER_SEC / DEF_TIME_NBR_mS_PER_SEC);
        p_buf->ID           =  NET_UTIL_HOST_TO_NET_32(pkt_id);  /* Set UDP datagram to send.                            */
        p_buf->TimeVar_sec  =  NET_UTIL_HOST_TO_NET_32(tv_sec);
        p_buf->TimeVar_usec =  NET_UTIL_HOST_TO_NET_32(tv_usec);

        tx_done             =  IPerf_ClientTx((IPERF_TEST *)p_test,
                                              (CPU_CHAR   *)p_data_buf,
                                              (CPU_INT16U  )IPERF_CLIENT_UDP_TX_MAX_RETRY,
                                              (CPU_INT32U  )IPERF_CLIENT_UDP_TX_MAX_DLY_MS,
                                              (IPERF_ERR  *)p_err);
        if (*p_err == IPERF_ERR_NONE) {
             pkt_id++;
        }

#if (IPERF_CFG_CPU_USAGE_MAX_CALC_EN == DEF_ENABLED)
        cpu_usage = IPerf_OS_CPU_Usage();
        p_stats->CPU_UsageAvg += cpu_usage;
        p_stats->CPU_CalcNbr++;
        if (cpu_usage > p_stats->CPU_UsageMax) {
            p_stats->CPU_UsageMax = cpu_usage;
        }
#endif

#if (IPERF_CFG_BANDWIDTH_CALC_EN == DEF_ENABLED)
        IPerf_UpdateBandwidth(p_test, &ts_ms_prev, &tx_bytes_prev);
#endif
    }

                                                                /* -------------------- TX UDP FIN -------------------- */
    ts_cur_ms           =  IPerf_Get_TS_ms();
    tv_sec              =  ts_cur_ms /  DEF_TIME_NBR_mS_PER_SEC;
    tv_usec             = (ts_cur_ms - (tv_sec * DEF_TIME_NBR_mS_PER_SEC)) * (DEF_TIME_NBR_uS_PER_SEC / DEF_TIME_NBR_mS_PER_SEC);
    p_buf->ID           =  NET_UTIL_HOST_TO_NET_32(-pkt_id);    /* Prepare UDP datagram FIN to send.                    */
    p_buf->TimeVar_sec  =  NET_UTIL_HOST_TO_NET_32( tv_sec);
    p_buf->TimeVar_usec =  NET_UTIL_HOST_TO_NET_32( tv_usec);
    p_conn->Run         =  DEF_NO;
    if (*p_err != IPERF_ERR_CLIENT_SOCK_TX) {
         IPerf_ClientTxUDP_FIN(p_test, p_data_buf);

         IPERF_TRACE_DBG(("*************** CLIENT UDP RESULT ***************\n\r"));
         IPERF_TRACE_DBG(("Tx Call count  = %u \n\r", (unsigned int)p_stats->NbrCalls));
         IPERF_TRACE_DBG(("Tx Err  count  = %u \n\r", (unsigned int)p_stats->Errs));
         IPERF_TRACE_DBG(("*************************************************\n\r"));
         IPERF_TRACE_DBG(("------------------- END SENDING -------------------\n\r"));
        *p_err = IPERF_ERR_NONE;
    }

#if 0
#if (IPERF_CFG_CPU_USAGE_MAX_CALC_EN == DEF_ENABLED)
    KAL_Dly(IPERF_UDP_CPU_USAGE_CALC_DLY_MS);               /* dly is needed to update task stats.                  */
    p_stats->CPU_UsageMax = IPerf_OS_CPU_Usage();
    p_stats->CPU_UsageAvg = IPerf_OS_CPU_Usage();
    p_stats->CPU_CalcNbr++;
#endif
#endif
}


/*
*********************************************************************************************************
*                                      IPerf_ClientTxUDP_FIN()
*
* Description : (1) Transmit UDP datagram FIN (end of UDP test) & receive UDP FINACK from server :
*
*                   (a) Transmit UDP client datagram header FIN
*                   (b) Receive  UDP FINACK from server
*
*
* Argument(s) : p_test      Pointer to a test.
*               ------      Argument validated in IPerf_Init(),
*                                    checked   in IPerf_ClientUDP(),
*                                                   by IPerf_TestTaskHandler().
*
*               p_data_buf  Pointer to data to transmit.
*               ----------  Argument validated in IPerf_ClientUDP().
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_ClientUDP().
*
* Note(s)     : (1) $$$$ Server send UDP statistic into the FINACK paket (bytes reveived, stop time,
*                   lost count, out of order count, last paket id received and jitter). Presently,
*                   this data is not saved into the test statistics.
*********************************************************************************************************
*/

static  void  IPerf_ClientTxUDP_FIN (IPERF_TEST  *p_test,
                                     CPU_CHAR    *p_data_buf)
{
    IPERF_CONN         *p_conn;
    IPERF_STATS        *p_stats;
    CPU_INT08U          data_len;
    CPU_INT08U          tx_ctr;
    CPU_INT16U          tx_err_ctr;
    CPU_INT32S          data_received;
    CPU_BOOLEAN         done;
    NET_SOCK_ADDR_LEN   addr_len_server;
    NET_ERR             err;


    p_conn     = &p_test->Conn;
    p_stats    = &p_test->Stats;
    data_len   =  128u;
    tx_ctr     =  0u;
    tx_err_ctr =  0u;
    done       =  DEF_NO;

    IPERF_TRACE_DBG(("Sending FIN ACK:"));

    while (done == DEF_NO) {                                    /* Loop until rx'd UDP FINACK or tx'd 10 time.          */


                                                                /* ------------------ TX CLIENT HDR ------------------- */
        addr_len_server = sizeof(p_conn->ServerAddrPort);
       (void)NetApp_SockTx((NET_SOCK_ID      ) p_conn->SockID,
                           (void            *) p_data_buf,
                           (CPU_INT16U       ) data_len,
                           (CPU_INT16S       ) NET_SOCK_FLAG_NONE,
                           (NET_SOCK_ADDR   *)&p_conn->ServerAddrPort,
                           (NET_SOCK_ADDR_LEN) addr_len_server,
                           (CPU_INT16U       ) IPERF_CLIENT_UDP_TX_MAX_RETRY,
                           (CPU_INT32U       ) 0,
                           (CPU_INT32U       ) IPERF_CLIENT_UDP_TX_MAX_DLY_MS,
                           (NET_ERR         *)&err);
        switch (err) {
            case NET_APP_ERR_NONE:                              /* Tx successful.                                        */
                 tx_ctr++;                                      /* Inc tx cnt.                                           */
                 IPERF_TRACE_DBG(("."));
                 break;


            case NET_APP_ERR_CONN_CLOSED:                       /* Err, exit sending process.                            */
            case NET_APP_ERR_FAULT:
            case NET_APP_ERR_INVALID_ARG:
            case NET_APP_ERR_INVALID_OP:
                 tx_ctr = IPERF_SERVER_UDP_TX_FINACK_COUNT;
                 break;


            case NET_ERR_TX:
            default:
                 IPERF_TRACE_DBG(("Tx error: %u\n\r", (unsigned int)err));
                 tx_err_ctr++;
                 break;
        }


                                                                /* -------------- RX SERVER UDP FIN ACK --------------- */
        if (err == NET_APP_ERR_NONE) {
            data_received = NetApp_SockRx((NET_SOCK_ID        ) p_conn->SockID,
                                          (void              *) p_data_buf,
                                          (CPU_INT16U         ) IPERF_UDP_BUF_LEN_MAX,
                                          (CPU_INT16U         ) 0u,
                                          (CPU_INT16S         ) NET_SOCK_FLAG_NONE,
                                          (NET_SOCK_ADDR     *)&p_conn->ServerAddrPort,
                                          (NET_SOCK_ADDR_LEN *)&addr_len_server,
                                          (CPU_INT16U         ) IPERF_RX_UDP_FINACK_MAX_RETRY,
                                          (CPU_INT32U         ) IPERF_RX_UDP_FINACK_MAX_TIMEOUT_MS,
                                          (CPU_INT32U         ) IPERF_RX_UDP_FINACK_MAX_DLY_MS,
                                          (NET_ERR           *)&err);
            if (data_received > 0u) {
                p_stats->UDP_EndErr = DEF_NO;
                IPERF_TRACE_DBG(("\n\rReceived UDP FINACK from server.\n\r"));
                return;                                         /* Rx'd server UDP FINACK, UDP test done.               */
                                                                /* $$$$ Sto rx'd stats of server (see Note #1).         */
            }
        }

        if (tx_ctr == IPERF_SERVER_UDP_TX_FINACK_COUNT) {
            p_stats->UDP_EndErr = DEF_YES;
            IPERF_TRACE_DBG(("\n\rSent %u UDP FIN and did not receive UDP FINACK from server.\n\r", (unsigned int)IPERF_SERVER_UDP_TX_FINACK_COUNT));
            done = DEF_YES;
        } else if (tx_err_ctr > IPERF_SERVER_UDP_TX_FINACK_ERR_MAX) {
            p_stats->UDP_EndErr = DEF_YES;
            IPERF_TRACE_DBG(("\n\rTx errors exceed maximum, %u FIN ACK sent\n\r", (unsigned int)tx_ctr));
            done = DEF_YES;
        }
    }
}


/*
*********************************************************************************************************
*                                          IPerf_ClientTx()
*
* Description : Transmit data to socket until the end of test or entire buffer is sent.
*
*               (a) Handle the end of test.
*               (b) If test not done, transmit data
*
*
* Argument(s) : p_test      Pointer to a test.
*               ------      Argument validated in IPerf_Init(),
*                                    checked   in IPerf_ClientTCP(),
*                                    checked   in IPerf_ClientUDP(),
*                                                   by IPerf_TestTaskHandler().
*
*               p_data_buf  Pointer to data to transmit.
*               ----------  Argument validated in IPerf_ClientTCP(),
*                                                 IPerf_ClientUDP().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               IPERF_ERR_NONE              Transmit     completed successfully,
*                               IPERF_ERR_CLIENT_SOCK_TX    Transmit NOT completed successfully.
*
* Return(s)   : DEF_YES       test     completed,
*               DEF_NO        test NOT completed.
*
*
* Caller(s)   : IPerf_ClientTCP();
*               IPerf_ClientUDP().
*********************************************************************************************************
*/
static  CPU_BOOLEAN  IPerf_ClientTx (IPERF_TEST   *p_test,
                                     CPU_CHAR     *p_data_buf,
                                     CPU_INT16U    retry_max,
                                     CPU_INT32U    time_dly_ms,
                                     IPERF_ERR    *p_err)
{
    IPERF_OPT          *p_opt;
    IPERF_CONN         *p_conn;
    IPERF_STATS        *p_stats;
    CPU_INT16U          tx_buf_len;
    CPU_INT16U          tx_len;
    CPU_INT16U          tx_len_tot;
    CPU_INT16U          buf_len;
    CPU_BOOLEAN         test_done;
    IPERF_TS_MS         ts_cur;
    IPERF_TS_MS         ts_max;
    IPERF_TS_MS         ts_ms_delta;
    NET_SOCK_ADDR_LEN   addr_len_server;
    NET_ERR             err;


    p_opt           = &p_test->Opt;
    p_conn          = &p_test->Conn;
    p_stats         = &p_test->Stats;
    tx_len_tot      =  0u;
    test_done       =  DEF_NO;
    buf_len         =  p_opt->BufLen;
    addr_len_server =  sizeof(p_conn->ServerAddrPort);
   *p_err           =  IPERF_ERR_NONE;

    while ((tx_len_tot < buf_len) &&                            /* Loop until tx tot len < buf len ...                  */
           (test_done == DEF_NO) ) {                            /* ... & test NOT done.                                 */



                                                                /* --------------- HANDLING END OF TEST --------------- */
        if (p_opt->Duration_ms > 0u) {
            ts_cur = IPerf_Get_TS_ms();                         /* Tx until time duration is not reached.               */
            ts_max = IPerf_Get_TS_Max_ms();
            if (ts_cur  >= p_stats->TS_Start_ms) {
                ts_ms_delta = ts_cur - p_stats->TS_Start_ms;
            } else {
                ts_ms_delta = ts_cur + (ts_max - p_stats->TS_Start_ms);
            }


            if ((ts_ms_delta >= (ts_max - 100u))             ||
                (ts_ms_delta >= p_opt->Duration_ms)) {
                 p_stats->TS_End_ms = ts_cur;
                 test_done          = DEF_YES;
            }

        } else if (p_stats->Bytes >= p_opt->BytesNbr) {
            p_stats->TS_End_ms = IPerf_Get_TS_ms();
            test_done          = DEF_YES;
        }

                                                                /* --------------------- TX DATA ---------------------- */
        if (test_done == DEF_NO) {
            tx_buf_len =  buf_len - tx_len_tot;
            p_stats->NbrCalls++;
            tx_len = NetApp_SockTx((NET_SOCK_ID      ) p_conn->SockID,
                                   (void            *) p_data_buf,
                                   (CPU_INT16U       ) tx_buf_len,
                                   (CPU_INT16S       ) NET_SOCK_FLAG_NONE,
                                   (NET_SOCK_ADDR   *)&p_conn->ServerAddrPort,
                                   (NET_SOCK_ADDR_LEN) addr_len_server,
                                   (CPU_INT16U       ) retry_max,
                                   (CPU_INT32U       ) 0,
                                   (CPU_INT32U       ) time_dly_ms,
                                   (NET_ERR         *)&err);
            if (tx_len > 0) {                                   /* If          tx len > 0, ...                          */
                tx_len_tot     += tx_len;                       /* ... inc tot tx len & bytes tx'd.                     */
                p_stats->Bytes += tx_len;
            }

            switch (err) {
                case NET_APP_ERR_NONE:
                     break;


                case NET_ERR_TX:                                /* If transitory tx err, ...                            */
                     p_stats->TransitoryErrCnts++;              /* ... inc tot TransitoryErrCnts.                       */
                     break;


                case NET_APP_ERR_FAULT:
                case NET_APP_ERR_CONN_CLOSED:
                case NET_APP_ERR_INVALID_OP:
                case NET_APP_ERR_INVALID_ARG:
                default:
                     p_stats->Errs++;
                     if ((p_opt->Protocol == IPERF_PROTOCOL_UDP   ) &&
                         (p_opt->BufLen    > IPERF_UDP_BUF_LEN_MAX)) {
                         *p_err = IPERF_ERR_CLIENT_SOCK_TX_INV_ARG;
                     } else {
                         *p_err = IPERF_ERR_CLIENT_SOCK_TX;
                     }
                     test_done = DEF_YES;
                     IPERF_TRACE_DBG(("Tx fatal Err : %u\n\r", (unsigned int)err));
                     break;
            }
        }
    }

    return (test_done);
}


/*
*********************************************************************************************************
*                                       IPerf_ClientPattern()
*
* Description : Fill a buffer with standard IPerf buffer pattern.
*
* Argument(s) : p_buf       Pointer to a buffer to fill.
*               ------      Argument validated in IPerf_ClientTCP(),
*                                    validated in IPerf_ClientUDP().
*
*               buf_len     Buffer lenght.
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_ClientTCP(),
*               IPerf_ClientUDP().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  IPerf_ClientPattern (CPU_CHAR    *p_buf,
                                   CPU_INT16U   buf_len)
{
    CPU_CHAR  *p_buf_wr;


    p_buf_wr = p_buf;
    while (buf_len-- > 0u) {
       *p_buf_wr = (CPU_CHAR)((buf_len % 10u) + '0');
        p_buf_wr++;
    }
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*
* Note(s) : (1) See 'MODULE'.
*********************************************************************************************************
*********************************************************************************************************
*/

#endif                                                          /* End of IPerf client module include (see Note #1).    */

