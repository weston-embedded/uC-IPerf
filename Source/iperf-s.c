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
*                                            IPERF SERVER
*
* Filename : iperf-s.c
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
#define    IPERF_SERVER_MODULE
#include  "iperf.h"
#include  <Source/net_cfg_net.h>

#ifdef  NET_IPv4_MODULE_EN
#include  <IP/IPv4/net_ipv4.h>
#endif
#ifdef  NET_IPv6_MODULE_EN
#include  <IP/IPv6/net_ipv6.h>
#endif

#include  <Source/net_sock.h>
#include  <Source/net_tcp.h>
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

#ifdef  IPERF_SERVER_MODULE_PRESENT


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void         IPerf_ServerSockInit    (IPERF_TEST   *p_test,
                                              IPERF_ERR    *p_err);

static  void         IPerf_ServerTCP         (IPERF_TEST   *p_test,
                                              IPERF_ERR    *p_err);

static  void         IPerf_ServerRxWinSizeSet(NET_SOCK_ID   sock_id,
                                              CPU_INT16U    win_size,
                                              IPERF_ERR    *p_err);

static  void         IPerf_ServerUDP         (IPERF_TEST   *p_test,
                                              IPERF_ERR    *p_err);

static  void         IPerf_ServerUDP_FINACK  (IPERF_TEST   *p_test,
                                              CPU_CHAR     *p_data_buf);

static  CPU_BOOLEAN  IPerf_ServerRxPkt       (IPERF_TEST   *p_test,
                                              NET_SOCK_ID   sock_id,
                                              CPU_CHAR     *p_data_buf,
                                              CPU_INT16U    retry_max,
                                              IPERF_ERR    *p_err);


/*
*********************************************************************************************************
*                                         IPerf_ServerStart()
*
* Description : (1) IPerf as a server main loop :
*
*                   (a) Initialize socket
*                   (b) Run IPerf as TCP/UDP server
*                   (c) Close socket used
*
*
* Argument(s) : p_test      Pointer to a test.
*               ------      Argument validated in IPerf_Init(),
*                                    checked   in IPerf_TestTaskHandler().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               IPERF_ERR_NONE                  socket     successfully.
*                               IPERF_ERR_SERVER_SOCK_OPEN      socket NOT successfully opened.
*                               IPERF_ERR_SERVER_SOCK_BIND      socket NOT successfully bond.
*                               IPERF_ERR_SERVER_SOCK_LISTEN    socket NOT successfully listened.
*                               IPERF_ERR_SERVER_SOCK_ACCEPT           NOT successfully accepted.
*                               IPERF_ERR_SERVER_WIN_SIZE              NOT successfully set windows size.
*                               IPERF_ERR_SERVER_SOCK_CLOSE            NOT successfully closed.
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
void  IPerf_ServerStart (IPERF_TEST  *p_test,
                         IPERF_ERR   *p_err)
{
    IPERF_OPT    *p_opt;
    IPERF_CONN   *p_conn;
    CPU_BOOLEAN   run;
    NET_ERR       err;


    IPERF_TRACE_INFO(("\n\r------------- IPerf SERVER START -------------\n\r"));
    IPERF_TRACE_DBG(("Init server socket\n\r"));


                                                                /* ------------- INIT SOCK / OPEN & BIND -------------- */
    IPerf_ServerSockInit(p_test, p_err);
    if (*p_err != IPERF_ERR_NONE) {
        return;
    }

    IPERF_TRACE_INFO(("IPerf Server Task : \n\r"));
    p_conn = &p_test->Conn;
    p_opt  = &p_test->Opt;

    run = DEF_YES;


                                                                /* --------------- RUN IPERF TCP/UDP RX --------------- */
    while (run == DEF_YES) {
        p_test->Status = IPERF_TEST_STATUS_RUNNING;
        if (p_opt->Protocol == IPERF_PROTOCOL_TCP) {
            IPerf_ServerTCP(p_test, p_err);
        } else {
            IPerf_ServerUDP(p_test, p_err);
        }

        if (*p_err != IPERF_ERR_NONE) {
            p_test->Err = *p_err;
            run         =  DEF_NO;
        }

        if (p_opt->Persistent == DEF_DISABLED) {
             run = DEF_NO;
        }
    }


                                                                /* -------------------- CLOSE SOCK -------------------- */
    IPERF_TRACE_DBG(("Closing socket.\n\r"));
    if (p_conn->SockID != NET_SOCK_ID_NONE) {
       (void)NetApp_SockClose((NET_SOCK_ID) p_conn->SockID,
                              (CPU_INT32U ) 0u,
                              (NET_ERR   *)&err);
        if (err != NET_APP_ERR_NONE) {
            IPERF_TRACE_DBG(("Sock close error : %u\n\r", (unsigned int)err));
        }
    }

    IPERF_TRACE_INFO(("\n\r------------- IPerf SERVER ENDED -------------\n\r"));
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
*                                       IPerf_ServerSockInit()
*
* Description : (1) Initialize one socket for sever use :
*
*                   (a) Open socket for incoming connection
*                   (b) Bind socket on any address & current test's options port
*                   (c) If TCP server, do a socket listen.
*
*
* Argument(s) : p_test      Pointer to a test.
*               ------      Argument validated in IPerf_Init(),
*                                    checked   in IPerf_ServerStart(),
*                                                   by IPerf_TestTaskHandler().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               IPERF_ERR_NONE                  socket     successfully initialized.
*                               IPERF_ERR_SERVER_SOCK_OPEN      socket NOT successfully opened.
*                               IPERF_ERR_SERVER_SOCK_BIND      socket NOT successfully bond.
*                               IPERF_ERR_SERVER_SOCK_LISTEN    socket NOT successfully listened.
*
*
* Return(s)   : none.
*
*
* Caller(s)   : IPerf_ServerStart().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  IPerf_ServerSockInit (IPERF_TEST  *p_test,
                                    IPERF_ERR   *p_err)
{
    IPERF_OPT             *p_opt;
    IPERF_CONN            *p_conn;
    NET_SOCK_ID            sock_id;
    NET_SOCK_ADDR          sock_addr_server;
    NET_SOCK_ADDR_FAMILY   addr_family;
    CPU_INT16U             server_port;
    CPU_BOOLEAN            cfg_succeed;
    NET_ERR                err;
#ifdef  NET_IPv4_MODULE_EN
    NET_IPv4_ADDR          addr_any_ipv4;
#endif

    p_opt  = &p_test->Opt;
    p_conn = &p_test->Conn;

    server_port = p_opt->Port;
    addr_family = ((p_opt->IPv4 == DEF_YES) ? (NET_SOCK_ADDR_FAMILY_IP_V4) : (NET_SOCK_ADDR_FAMILY_IP_V6));

                                                                /* -------------------- OPEN SOCK --------------------- */
    IPERF_TRACE_DBG(("Server SockOpen "));
    if (p_opt->Protocol == IPERF_PROTOCOL_UDP) {
        IPERF_TRACE_DBG(("UDP ... "));
        sock_id = NetApp_SockOpen((NET_SOCK_PROTOCOL_FAMILY) addr_family,
                                  (NET_SOCK_TYPE           ) NET_SOCK_TYPE_DATAGRAM,
                                  (NET_SOCK_PROTOCOL       ) NET_SOCK_PROTOCOL_UDP,
                                  (CPU_INT16U              ) IPERF_OPEN_MAX_RETRY,
                                  (CPU_INT32U              ) IPERF_OPEN_MAX_DLY_MS,
                                  (NET_ERR                *)&err);
    } else {
        IPERF_TRACE_DBG(("TCP ... "));
        sock_id = NetApp_SockOpen((NET_SOCK_PROTOCOL_FAMILY) addr_family,
                                  (NET_SOCK_TYPE           ) NET_SOCK_TYPE_STREAM,
                                  (NET_SOCK_PROTOCOL       ) NET_SOCK_PROTOCOL_TCP,
                                  (CPU_INT16U              ) IPERF_OPEN_MAX_RETRY,
                                  (CPU_INT32U              ) IPERF_OPEN_MAX_DLY_MS,
                                  (NET_ERR                *)&err);
    }
    switch (err) {
        case NET_APP_ERR_NONE:
             IPERF_TRACE_DBG(("done.\n\r"));
             break;


        case NET_APP_ERR_FAULT:
        case NET_APP_ERR_NONE_AVAIL:
        case NET_APP_ERR_INVALID_ARG:
        default:
             IPERF_TRACE_DBG(("Fail, error : %u.\n\r", (unsigned int)err));
            *p_err = IPERF_ERR_SERVER_SOCK_OPEN;
             return;
    }

    cfg_succeed = NetSock_CfgBlock(sock_id, NET_SOCK_BLOCK_SEL_BLOCK, &err);
    if (cfg_succeed != DEF_OK) {
       *p_err = IPERF_ERR_SERVER_SOCK_OPT;
        return;
    }

                                                                /* -------------------- BIND SOCK --------------------- */
    Mem_Clr((void     *)&sock_addr_server,
            (CPU_SIZE_T) NET_SOCK_ADDR_SIZE);

    switch (addr_family) {
#ifdef  NET_IPv4_MODULE_EN
        case NET_SOCK_PROTOCOL_FAMILY_IP_V4:
             addr_any_ipv4 = NET_IPv4_ADDR_NONE;
             NetApp_SetSockAddr(              &sock_addr_server,
                                               NET_SOCK_ADDR_FAMILY_IP_V4,
                                               server_port,
                                (CPU_INT08U *)&addr_any_ipv4,
                                               NET_IPv4_ADDR_SIZE,
                                              &err);
             break;
#endif

#ifdef  NET_IPv6_MODULE_EN
        case NET_SOCK_PROTOCOL_FAMILY_IP_V6:

             NetApp_SetSockAddr(              &sock_addr_server,
                                               NET_SOCK_ADDR_FAMILY_IP_V6,
                                               server_port,
                                (CPU_INT08U *)&NET_IPv6_ADDR_ANY,
                                               NET_IPv6_ADDR_SIZE,
                                              &err);
             break;
#endif
        default:
             IPERF_TRACE_DBG(("Fail, error: %u.\n\r", (unsigned int)err));
            *p_err = IPERF_ERR_SERVER_INVALID_IP_FAMILY;
             return;
    }


    IPERF_TRACE_DBG(("Server Sock Bind ... "));
   (void)NetApp_SockBind((NET_SOCK_ID      ) sock_id,
                         (NET_SOCK_ADDR   *)&sock_addr_server,
                         (NET_SOCK_ADDR_LEN) NET_SOCK_ADDR_SIZE,
                         (CPU_INT16U       ) IPERF_BIND_MAX_RETRY,
                         (CPU_INT32U       ) IPERF_BIND_MAX_DLY_MS,
                         (NET_ERR         *)&err);
     switch (err) {
        case NET_APP_ERR_NONE:
             p_conn->SockID = sock_id;
             Mem_Copy(&p_conn->ServerAddrPort, &sock_addr_server, NET_SOCK_ADDR_SIZE);
             IPERF_TRACE_DBG(("done.\n\r"));
             break;


        case NET_APP_ERR_FAULT:
        case NET_APP_ERR_NONE_AVAIL:
        case NET_APP_ERR_INVALID_ARG:
        default:
             IPERF_TRACE_DBG(("Fail error : %u.\n\r", (unsigned int)err));
            (void)NetApp_SockClose((NET_SOCK_ID) sock_id,
                                   (CPU_INT32U ) 0u,
                                   (NET_ERR   *)&err);
             if (err != NET_APP_ERR_NONE) {
                 IPERF_TRACE_DBG(("Close socket error : %u.\n\r", (unsigned int)err));
             }
            *p_err = IPERF_ERR_SERVER_SOCK_BIND;
             return;
    }



    if (p_opt->Protocol == IPERF_PROTOCOL_TCP) {                /* Only TCP req a sock listen & a TCP Rx window size.   */
        IPERF_TRACE_DBG(("Server listen ... "));                /* ------------------- SOCK LISTEN -------------------- */
       (void)NetApp_SockListen((NET_SOCK_ID    ) sock_id,
                               (NET_SOCK_Q_SIZE) IPERF_SERVER_TCP_CONN_Q_SIZE,
                               (NET_ERR       *)&err);
        switch (err) {
            case NET_APP_ERR_NONE:
                 IPERF_TRACE_DBG(("done.\n\r"));
                 break;


            case NET_APP_ERR_FAULT:
            case NET_APP_ERR_INVALID_OP:
            case NET_APP_ERR_INVALID_ARG:
            case NET_APP_ERR_FAULT_TRANSITORY:
            default:
                 IPERF_TRACE_DBG(("Fail error : %u.\n\r", (unsigned int)err));
                (void)NetApp_SockClose((NET_SOCK_ID) sock_id,
                                       (CPU_INT32U ) 0u,
                                       (NET_ERR   *)&err);
                 if (err != NET_APP_ERR_NONE) {
                     IPERF_TRACE_DBG(("Close socket error : %u.\n\r", (unsigned int)err));
                 }
                *p_err = IPERF_ERR_SERVER_SOCK_LISTEN;
                 return;
        }


                                                                /* ----------------- SET WINDOW SIZE ------------------ */
        IPERF_TRACE_DBG(("Set accepted socket window size... "));
        IPerf_ServerRxWinSizeSet(sock_id, p_opt->WinSize, p_err);
        if (*p_err != IPERF_ERR_NONE) {
            IPERF_TRACE_DBG(("Error\n\r"));
            return;
        }
        IPERF_TRACE_DBG(("Done\n\r"));
    }



   *p_err = IPERF_ERR_NONE;
}


/*
*********************************************************************************************************
*                                          IPerf_ServerTCP()
*
* Description : (1) IPerf TCP server :
*
*                   (a) Wait for client connection (socket accept incoming connection)
*                   (b) Set rx window size
*                   (c) Receive packet until socket close received from client
*                   (d) Close accepted socket
*
*
* Argument(s) : p_test      Pointer to a test.
*               ------      Argument validated in IPerf_Init(),
*                                    checked   in IPerf_ServerStart(),
*                                                   by IPerf_TestTaskHandler().
*
*               p_err       Pointer to variable that will receive the return error code from this function
*
*                               IPERF_ERR_NONE                  Test successfully completed.
*                               IPERF_ERR_SERVER_SOCK_ACCEPT    NOT  successfully accepted client connection.
*                               IPERF_ERR_SERVER_SOCK_CLOSE     NOT  successfully closed socket accepted.
*
*                                                               - RETURNED BY IPerf_ServerRxWinSizeSet() : --
*                               IPERF_ERR_SERVER_WIN_SIZE       Windows size can't be set.
*
*                                                               ----- RETURNED BY IPerf_ServerRxPkt() : -----
*                               IPERF_ERR_SERVER_SOCK_RX        Fatal error with rx socket.
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_ServerStart().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  IPerf_ServerTCP (IPERF_TEST  *p_test,
                               IPERF_ERR   *p_err)
{
    IPERF_CONN         *p_conn;
    CPU_CHAR           *p_data_buf;
    CPU_BOOLEAN         rx_done;
    NET_SOCK_ID         sock_id;
    NET_SOCK_ADDR_LEN   addr_len_client;
#if (IPERF_CFG_CPU_USAGE_MAX_CALC_EN == DEF_ENABLED)
    CPU_INT16U          cpu_usage;
#endif
#if (IPERF_CFG_BANDWIDTH_CALC_EN == DEF_ENABLED)
    CPU_BOOLEAN         rx_started;
    IPERF_TS_MS         ts_ms_prev;
    CPU_INT32U          rx_bytes_prev;
#endif
#if ((IPERF_CFG_BANDWIDTH_CALC_EN     == DEF_ENABLED) || \
     (IPERF_CFG_CPU_USAGE_MAX_CALC_EN == DEF_ENABLED))
    IPERF_STATS        *p_stats;
#endif
    NET_ERR             err;


    p_conn      = &p_test->Conn;
    p_conn->Run =  DEF_NO;

                                                                /* ------------------- SOCK ACCEPT -------------------- */
    IPERF_TRACE_DBG(("TCP Server Accept ... "));

    p_test->Status  = IPERF_TEST_STATUS_RUNNING;
    addr_len_client = sizeof(p_conn->ClientAddrPort);
    sock_id         = NetApp_SockAccept((NET_SOCK_ID        ) p_conn->SockID,
                                        (NET_SOCK_ADDR     *)&p_conn->ClientAddrPort,
                                        (NET_SOCK_ADDR_LEN *)&addr_len_client,
                                        (CPU_INT16U         ) IPERF_CFG_SERVER_ACCEPT_MAX_RETRY,
                                        (CPU_INT32U         ) IPERF_CFG_SERVER_ACCEPT_MAX_TIMEOUT_MS,
                                        (CPU_INT32U         ) IPERF_CFG_SERVER_ACCEPT_MAX_DLY_MS,
                                        (NET_ERR           *)&err);
    switch (err) {
        case NET_APP_ERR_NONE:
             p_conn->SockID_TCP_Server = sock_id;
            *p_err                     = IPERF_ERR_NONE;
             IPERF_TRACE_DBG(("Done.\n\r"));
             break;


        case NET_APP_ERR_NONE_AVAIL:
        case NET_APP_ERR_FAULT_TRANSITORY:
        case NET_APP_ERR_FAULT:
        case NET_APP_ERR_INVALID_OP:
        case NET_APP_ERR_INVALID_ARG:
        default:
            *p_err = IPERF_ERR_SERVER_SOCK_ACCEPT;
             IPERF_TRACE_INFO(("Error : %u.\n\r", (unsigned int)err));
             return;
    }


                                                                /* --------------------- RX PKTS ---------------------- */
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

    NetSock_CfgTimeoutRxQ_Set(p_conn->SockID, IPERF_CFG_SERVER_TCP_RX_MAX_TIMEOUT_MS, &err);

#if ((IPERF_CFG_BANDWIDTH_CALC_EN     == DEF_ENABLED) || \
     (IPERF_CFG_CPU_USAGE_MAX_CALC_EN == DEF_ENABLED))
    p_stats    = &p_test->Stats;
#endif

#if (IPERF_CFG_BANDWIDTH_CALC_EN == DEF_ENABLED)
    rx_started =  DEF_NO;
#endif

    rx_done    =  DEF_NO;

    IPERF_TRACE_INFO(("TCP Socket Received start... \n\r"));
    while (rx_done == DEF_NO) {                                 /* Loop until sock is closed by the client.             */
        rx_done = IPerf_ServerRxPkt((IPERF_TEST   *)p_test,
                                    (NET_SOCK_ID   )sock_id,
                                    (CPU_CHAR     *)p_data_buf,
                                    (CPU_INT16U    )IPERF_SERVER_TCP_RX_MAX_RETRY,
                                    (IPERF_ERR    *)p_err);

#if (IPERF_CFG_CPU_USAGE_MAX_CALC_EN == DEF_ENABLED)
        cpu_usage = IPerf_OS_CPU_Usage();
        p_stats->CPU_UsageAvg += cpu_usage;
        p_stats->CPU_CalcNbr++;
        if (cpu_usage > p_stats->CPU_UsageMax) {
            p_stats->CPU_UsageMax = cpu_usage;
        }
#endif

#if (IPERF_CFG_BANDWIDTH_CALC_EN == DEF_ENABLED)
        if (rx_started == DEF_NO) {
            ts_ms_prev    = p_stats->TS_Start_ms;
            rx_bytes_prev = 0u;
            rx_started    = DEF_YES;
        }
        IPerf_UpdateBandwidth(p_test, &ts_ms_prev, &rx_bytes_prev);
#endif
    }

    p_conn->Run = DEF_NO;
    IPERF_TRACE_INFO(("TCP Socket Received Done\n\rClose socket accepted..."));


                                                                /* -------------------- CLOSE SOCK -------------------- */
   (void)NetApp_SockClose((NET_SOCK_ID) p_conn->SockID_TCP_Server,
                          (CPU_INT32U ) 0u,
                          (NET_ERR   *)&err);
    switch (err) {
        case NET_APP_ERR_NONE:
             IPERF_TRACE_DBG(("Done\n\r"));
             break;


        case NET_APP_ERR_FAULT_TRANSITORY:
        case NET_APP_ERR_FAULT:
        case NET_APP_ERR_INVALID_ARG:
        default:
             if (*p_err == IPERF_ERR_NONE) {                    /* Don't overwrite err from IPerf_ServerRxPkt().        */
                 *p_err  = IPERF_ERR_SERVER_SOCK_CLOSE;
             }
             IPERF_TRACE_DBG(("Error : %u.\n\r", (unsigned int)err));
             break;
    }
}


/*
*********************************************************************************************************
*                                     IPerf_ServerRxWinSizeSet()
*
* Description : (1) Configure receive window size :
*
*                   (a) Get socket connection id
*                   (b) Configure connection receive window size
*
*
* Argument(s) : sock_id     Windows size socket ID to change.
*
*               win_size    New windows size.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               IPERF_ERR_NONE               windows size     successfully set.
*                               IPERF_ERR_SERVER_WIN_SIZE    windows size NOT successfully set.
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_ServerTCP().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  IPerf_ServerRxWinSizeSet(NET_SOCK_ID   sock_id,
                                       CPU_INT16U    win_size,
                                       IPERF_ERR    *p_err)
{
    NET_CONN_ID  conn_id;
    CPU_BOOLEAN  set;
    NET_ERR      err;

                                                                /* ------------------- GET CONN ID -------------------- */
    conn_id = NetSock_GetConnTransportID(sock_id, &err);

    switch (err) {
        case NET_SOCK_ERR_NONE:
             IPERF_TRACE_DBG(("Setting window size of connection ID : %d\n\r", conn_id));
             break;


        case NET_SOCK_ERR_INVALID_TYPE:
        case NET_INIT_ERR_NOT_COMPLETED:
        case NET_SOCK_ERR_INVALID_SOCK:
        case NET_SOCK_ERR_NOT_USED:
        case NET_CONN_ERR_INVALID_CONN:
        case NET_CONN_ERR_NOT_USED:
        case NET_ERR_FAULT_LOCK_ACQUIRE:
        default:
             IPERF_TRACE_DBG(("Setting window size fail with error : %u\n\r", (unsigned int)err));
            *p_err = IPERF_ERR_SERVER_WIN_SIZE;
             return;
    }
                                                                /* ------------------- SET WIN SIZE ------------------- */
#if (NET_VERSION >= 21001u)
    set = NetTCP_ConnCfgRxWinSize(conn_id, win_size, &err);
#else
    set = NetTCP_ConnCfgRxWinSize(conn_id, win_size);
#endif
    if (set == DEF_OK) {
       *p_err = IPERF_ERR_NONE;
    } else {
       *p_err = IPERF_ERR_SERVER_WIN_SIZE;
    }
}


/*
*********************************************************************************************************
*                                          IPerf_ServerUDP()
*
* Description : (1) IPerf UDP server :
*
*                   (a) Receive UDP packet
*                   (b) Decode packet & update UDP stat
*
*
* Argument(s) : p_test      Pointer to a test.
*               ------      Argument validated in IPerf_Init(),
*                                    checked   in IPerf_ServerStart(),
*                                                   by IPerf_TestTaskHandler().
*
*               p_err       Pointer to variable that will receive the return error code from this function
*
*                               IPERF_ERR_NONE                  Test successfully completed.
*
*                                                               ----- RETURNED BY IPerf_ServerRxPkt() : -----
*                               IPERF_ERR_SERVER_SOCK_RX        Fatal error with rx socket.
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_ServerStart().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  IPerf_ServerUDP (IPERF_TEST  *p_test,
                               IPERF_ERR   *p_err)
{
    IPERF_STATS         *p_stats;
    IPERF_CONN          *p_conn;
    CPU_CHAR            *p_data_buf;
    IPERF_UDP_DATAGRAM  *p_buf;                                 /* Ptr to cast the buf on UDP Datagram.                 */
    CPU_INT32U           pkt_ctr;
    CPU_INT32S           pkt_id;
    CPU_BOOLEAN          rx_done;
#if (IPERF_CFG_CPU_USAGE_MAX_CALC_EN == DEF_ENABLED)
    CPU_INT16U           cpu_usage;
#endif
#if (IPERF_CFG_BANDWIDTH_CALC_EN == DEF_ENABLED)
    IPERF_TS_MS          ts_ms_prev;
    CPU_INT32U           rx_bytes_prev;
#endif
    NET_ERR              err;


    p_conn      = &p_test->Conn;
    p_stats     = &p_test->Stats;
    pkt_ctr     =  0u;
    pkt_id      =  0u;
    p_conn->Run =  DEF_NO;
    rx_done     =  DEF_NO;

                                                                /* ------------------- RX UDP PKTS -------------------- */
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

    NetSock_CfgTimeoutRxQ_Set(p_conn->SockID, IPERF_CFG_SERVER_UDP_RX_MAX_TIMEOUT_MS, &err);

    p_buf      = (IPERF_UDP_DATAGRAM *)p_data_buf;

    IPERF_TRACE_INFO(("UDP Socket Received start\n\r"));        /* Loop until end pkt rx'd or sock close from client.   */
    while (rx_done == DEF_NO) {
        rx_done = IPerf_ServerRxPkt((IPERF_TEST   *)p_test,
                                    (NET_SOCK_ID   )p_conn->SockID,
                                    (CPU_CHAR     *)p_data_buf,
                                    (CPU_INT16U    )IPERF_SERVER_UDP_RX_MAX_RETRY,
                                    (IPERF_ERR    *)p_err);


                                                                /*--------- DECODE RXD PKT & UPDATE UDP STATS --------- */
        if (*p_err == IPERF_ERR_NONE) {
            pkt_ctr++;
            NET_UTIL_VAL_COPY_GET_NET_32(&pkt_id, &p_buf->ID);  /* Copy ID from datagram to pkt ID.                     */
            if (pkt_ctr == 1u && pkt_id >= 0) {                 /* First pkt rx'd.                                      */
                IPERF_TRACE_INFO(("UDP First paket received :  start udp process.\n\r"));
                if (pkt_id > 0u) {
                    IPERF_TRACE_DBG(("Paket ID not synchronised (first packet not received).\n\r"));
                    p_stats->UDP_AsyncErr = DEF_YES;            /*First UDP pkt from client not rx'd.                   */
                    p_stats->UDP_LostPkt++;
                }

                p_stats->TS_Start_ms = IPerf_Get_TS_ms();
#if (IPERF_CFG_BANDWIDTH_CALC_EN == DEF_ENABLED)
                ts_ms_prev          = p_stats->TS_Start_ms;
                rx_bytes_prev       = 0u;
#endif
                p_stats->UDP_RxLastPkt = pkt_id;
            }else if ((pkt_ctr == 1u) &&
                      (pkt_id   < 0 )) {                        /* Rx'd old udp fin ack ...                             */
                pkt_ctr     = 0u;                               /* ... discard pkt.                                     */
                p_conn->Run = DEF_NO;


            } else if (pkt_id < 0) {                            /* Rx'd end pkt.                                        */
                p_stats->TS_End_ms = IPerf_Get_TS_ms();
                p_conn->Run        = DEF_NO;
                rx_done            = DEF_YES;
                IPerf_ServerUDP_FINACK(p_test, p_data_buf);     /* Send FINACK.                                         */
                IPERF_TRACE_INFO(("UDP Socket Received done\n\r"));

                                                                /* Rx'd pkt id not expected ...                         */
            } else if ((pkt_id  != (p_stats->UDP_RxLastPkt + 1)) &&
                       (pkt_ctr  >  1u)                          ) {
                if (pkt_id < p_stats->UDP_RxLastPkt + 1) {      /* Pkt out of order; ....                               */
                    p_stats->UDP_OutOfOrder++;
                } else {                                        /* ... pkts lost.                                       */
                    p_stats->UDP_LostPkt += (pkt_id - (p_stats->UDP_RxLastPkt + 1u));
                }


            } else if ((pkt_id  == p_stats->UDP_AsyncErr) &&    /* Rx'd dup pkt.                                        */
                       (pkt_ctr  > 1u)                   ) {
                p_stats->UDP_DupPkt++;
            }

            if ((pkt_ctr > 1u                    ) &&           /* Sto pkt id for next pkt decode.                      */
                (pkt_id  > p_stats->UDP_RxLastPkt)) {
                 p_stats->UDP_RxLastPkt = pkt_id;
            }
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
        if (pkt_ctr > 1u) {
            IPerf_UpdateBandwidth(p_test, &ts_ms_prev, &rx_bytes_prev);
        }
#endif
    }
}


/*
*********************************************************************************************************
*                                      IPerf_ServerUDP_FINACK()
*
* Description : (1) Server UDP FINACK send loop :
*
*                   (a) Set  UDP FINACK server header
*                   (b) Send UDP FINACK for 10 times or until received UDP FIN
*                   (c) Try to receive UDP FIN from client
*
*
* Argument(s) : p_test      Pointer to a test.
*               ------      Argument validated in IPerf_Init(),
*                                    checked   in IPerf_ServerUDP(),
*                                                   by IPerf_TestTaskHandler().
*
*               p_data_buf  Pointer to data to transmit.
*               ----------  Argument validated in IPerf_ClientUDP().
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_ServerUDP().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  IPerf_ServerUDP_FINACK (IPERF_TEST *p_test,
                                      CPU_CHAR   *p_data_buf)
{
    IPERF_STATS           *p_stats;
    IPERF_CONN            *p_conn;
    IPERF_OPT             *p_opt;
    IPERF_UDP_DATAGRAM    *p_datagram;                          /* Ptr to set UDP pkt datagram hdr.                     */
    IPERF_SERVER_UDP_HDR  *p_hdr;                               /* Ptr to set UDP pkt server   hdr.                     */
    CPU_INT08U             buf_len;
    CPU_INT08U             tx_ctr;
    CPU_INT16U             tx_err_ctr;
    CPU_INT32S             bytes_received;
    CPU_BOOLEAN            done;
    NET_SOCK_ADDR_LEN      addr_len_client;
    NET_ERR                err;


    p_opt                 = &p_test->Opt;
    p_conn                = &p_test->Conn;
    p_stats               = &p_test->Stats;


                                                                /* ------------------ SET SERVER HDR ------------------ */
    p_datagram            = (IPERF_UDP_DATAGRAM   *) p_data_buf;
    p_hdr                 = (IPERF_SERVER_UDP_HDR *)p_datagram + 1u;
    p_hdr->Flags          =  NET_UTIL_HOST_TO_NET_32(IPERF_SERVER_UDP_HEADER_VERSION1);
    p_hdr->TotLen_Hi      =    0u;
    p_hdr->TotLen_Lo      =  NET_UTIL_HOST_TO_NET_32(p_stats->Bytes &  0xFFFFFFFF);
    p_hdr->Stop_sec       =  NET_UTIL_HOST_TO_NET_32(p_stats->TS_End_ms * 1000u);
    p_hdr->Stop_usec      =  NET_UTIL_HOST_TO_NET_32(p_stats->TS_End_ms / 1000u);
    p_hdr->LostPkt_ctr    =  NET_UTIL_HOST_TO_NET_32(p_stats->UDP_LostPkt);
    p_hdr->OutOfOrder_ctr =  NET_UTIL_HOST_TO_NET_32(p_stats->UDP_OutOfOrder);
    p_hdr->RxLastPkt      =  NET_UTIL_HOST_TO_NET_32(p_stats->UDP_RxLastPkt);
    p_hdr->Jitter_Hi      =    0u;
    p_hdr->Jitter_Lo      =    0u;
    addr_len_client       =  sizeof(p_conn->ClientAddrPort);
    buf_len               =  128u;
    tx_ctr                =    0u;
    tx_err_ctr            =    0u;
    done                  =  DEF_NO;



                                                                /* ----------------- SEND UDP FINACK ------------------ */
    IPERF_TRACE_DBG(("Sending FIN ACK :"));
    while (done == DEF_NO) {
       (void)NetApp_SockTx((NET_SOCK_ID      ) p_conn->SockID,
                           (void            *) p_data_buf,
                           (CPU_INT16U       ) buf_len,
                           (CPU_INT16S       ) NET_SOCK_FLAG_NONE,
                           (NET_SOCK_ADDR   *)&p_conn->ClientAddrPort,
                           (NET_SOCK_ADDR_LEN) addr_len_client,
                           (CPU_INT16U       ) IPERF_SERVER_UDP_TX_MAX_RETRY,
                           (CPU_INT32U       ) 0,
                           (CPU_INT32U       ) IPERF_SERVER_UDP_TX_MAX_DLY_MS,
                           (NET_ERR         *)&err);

        switch (err) {
            case NET_APP_ERR_NONE:                              /* Tx successful.                                       */
                 tx_ctr++;                                      /* Inc tx cnt.                                          */
                 if (tx_ctr == IPERF_SERVER_UDP_TX_FINACK_COUNT) {
                    done = DEF_YES;
                 }
                 IPERF_TRACE_DBG(("."));
                 break;


            case NET_APP_ERR_CONN_CLOSED:
            case NET_APP_ERR_FAULT:
            case NET_APP_ERR_INVALID_ARG:
            case NET_APP_ERR_INVALID_OP:
                 tx_ctr = IPERF_SERVER_UDP_TX_FINACK_COUNT;
                 break;


            case NET_ERR_TX:
            default:
                 IPERF_TRACE_INFO(("Tx error : %u\n\r", (unsigned int)err));
                 tx_err_ctr++;
                 if (tx_err_ctr > IPERF_SERVER_UDP_TX_FINACK_ERR_MAX) {
                     p_stats->UDP_EndErr = DEF_YES;
                     IPERF_TRACE_INFO(("Tx errors exceed maximum, %u FIN ACK was send\n\r", (unsigned int)tx_ctr));
                     done = DEF_YES;
                 }
                 break;
        }


                                                                /* ------------- TRY TO RX CLIENT UDP FIN ------------- */
        bytes_received = NetApp_SockRx((NET_SOCK_ID        ) p_conn->SockID,
                                       (void              *) p_data_buf,
                                       (CPU_INT16U         ) p_opt->BufLen,
                                       (CPU_INT16U         ) 0u,
                                       (CPU_INT16S         ) NET_SOCK_FLAG_NONE,
                                       (NET_SOCK_ADDR     *)&p_conn->ClientAddrPort,
                                       (NET_SOCK_ADDR_LEN *)&addr_len_client,
                                       (CPU_INT16U         ) 0u,
                                       (CPU_INT32U         ) IPERF_RX_UDP_FINACK_MAX_TIMEOUT_MS,
                                       (CPU_INT32U         ) 0u,
                                       (NET_ERR           *)&err);

        if ((err             == NET_SOCK_ERR_NONE) &&
            (bytes_received   > 0u               )) {
             bytes_received   = NetApp_SockRx((NET_SOCK_ID        ) p_conn->SockID,
                                              (void              *) p_data_buf,
                                              (CPU_INT16U         ) p_opt->BufLen,
                                              (CPU_INT16U         ) 0u,
                                              (CPU_INT16S         ) NET_SOCK_FLAG_NONE,
                                              (NET_SOCK_ADDR     *)&p_conn->ClientAddrPort,
                                              (NET_SOCK_ADDR_LEN *)&addr_len_client,
                                              (CPU_INT16U         ) 0u,
                                              (CPU_INT32U         ) IPERF_RX_UDP_FINACK_MAX_TIMEOUT_MS,
                                              (CPU_INT32U         ) 0u,
                                              (NET_ERR           *)&err);
            if (bytes_received <= 0u) {
                done = DEF_YES;                                 /* Conn closed or sock err.                             */
            }
         } else if ((err             == NET_ERR_RX) &&
                    (bytes_received  == 0u        )) {
            done = DEF_YES;
         }

    }
    IPERF_TRACE_DBG(("End UDP receive process\n\r\n\r"));
}



/*
*********************************************************************************************************
*                                         IPerf_ServerRxPkt()
*
* Description : Receive packet through a socket.
*
* Argument(s) : p_test      Pointer to a test.
*               ------      Argument validated in IPerf_Init();
*                                    checked   in IPerf_ServerTCP();
*                                                 IPerf_ServerUDP().
*                                                   by IPerf_TestTaskHandler().
*
*               sock_id     Socket descriptor/handle identifier of socket to receive data.
*
*               p_data_buf  Pointer to data to transmit.
*               ----------  Argument validated in IPerf_ServerTCP(),
*                                                 IPerf_ServerUDP.
*
*               retry_max   Maximum number of consecutive socket receive retries.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*
*                               IPERF_ERR_NONE                  NO    error with receive socket.
*                               IPERF_ERR_SERVER_SOCK_RX        fatal error with receive socket.
*
*
*
* Return(s)   : DEF_YES     Socket     ready to receive      paket,
*               DEF_NO      Socket NOT ready to receive more paket.
*
* Caller(s)   : IPerf_ServerTCP(),
*               IPerf_ServerUDP().
*
* Note(s)     : none.
*********************************************************************************************************
*/
static  CPU_BOOLEAN  IPerf_ServerRxPkt (IPERF_TEST   *p_test,
                                        NET_SOCK_ID   sock_id,
                                        CPU_CHAR     *p_data_buf,
                                        CPU_INT16U    retry_max,
                                        IPERF_ERR    *p_err)
{
    IPERF_OPT          *p_opt;
    IPERF_CONN         *p_conn;
    IPERF_STATS        *p_stats;
    NET_SOCK_ADDR_LEN   addr_len_client;
    CPU_INT16S          rx_len;
    CPU_INT16U          rx_buf_len;
    CPU_BOOLEAN         rx_done;
    CPU_BOOLEAN         rx_server_done;
    NET_ERR             err;


    p_opt           = &p_test->Opt;
    p_conn          = &p_test->Conn;
    p_stats         = &p_test->Stats;
    rx_buf_len      =  p_opt->BufLen;
    rx_done         =  DEF_NO;
    rx_server_done  =  DEF_NO;
   *p_err           =  IPERF_ERR_NONE;
    rx_buf_len      =  p_opt->BufLen;

    while ((rx_done        == DEF_NO) &&
           (rx_server_done == DEF_NO)) {
                                                                /* --------------- RX PKT THROUGH SOCK ---------------- */
        p_stats->NbrCalls++;
        addr_len_client = sizeof(p_conn->ClientAddrPort);
        rx_len          = NetApp_SockRx((NET_SOCK_ID        ) sock_id,
                                        (void              *) p_data_buf,
                                        (CPU_INT16U         ) rx_buf_len,
                                        (CPU_INT16U         ) 0u,
                                        (CPU_INT16S         ) NET_SOCK_FLAG_NONE,
                                        (NET_SOCK_ADDR     *)&p_conn->ClientAddrPort,
                                        (NET_SOCK_ADDR_LEN *)&addr_len_client,
                                        (CPU_INT16U         ) retry_max,
                                        (CPU_INT32U         ) 0,
                                        (CPU_INT32U         ) 0,
                                        (NET_ERR           *)&err);
        p_stats->Bytes += rx_len;
        switch (err) {
            case NET_APP_ERR_NONE:
            case NET_APP_ERR_DATA_BUF_OVF:
                 rx_done          = DEF_YES;
                 if (p_conn->Run == DEF_NO) {
                     IPerf_TestClrStats(p_stats);
                     p_stats->TS_Start_ms = IPerf_Get_TS_ms();
                     p_stats->TS_End_ms   = 0u;
                     p_conn->Run          = DEF_YES;
                 }
                 break;


            case NET_ERR_RX:                                    /* Transitory rx err(s), ...                            */
                 p_stats->TransitoryErrCnts++;
                 break;


            case NET_APP_ERR_CONN_CLOSED:                       /* Conn closed by peer.                                 */
                 p_stats->TS_End_ms = IPerf_Get_TS_ms();
                 rx_server_done     = DEF_YES;
                 break;


            case NET_APP_ERR_FAULT:
            case NET_APP_ERR_INVALID_ARG:
            case NET_APP_ERR_INVALID_OP:
            default:
                 p_stats->Errs++;
                 rx_server_done = DEF_YES;
                *p_err          = IPERF_ERR_SERVER_SOCK_RX;     /* Rtn fatal err(s).                                    */
                 break;
        }
    }

    return (rx_server_done);
}

/*
*********************************************************************************************************
*                                             MODULE END
*
* Note(s) : (1) See 'MODULE'.
*********************************************************************************************************
*/

#endif                                                          /* End of IPerf server module include (see Note #1).    */

