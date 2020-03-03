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
*                                                IPERF
*
* Filename : iperf.h
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
*                                               MODULE
*
* Note(s) : (1) This header file is protected from multiple pre-processor inclusion through use of the
*               IPerf present pre-processor macro definition.
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  IPERF_MODULE_PRESENT                                   /* See Note #1.                                         */
#define  IPERF_MODULE_PRESENT


/*
*********************************************************************************************************
*                                        IPerf VERSION NUMBER
*
* Note(s) : (1) (a) The IPerf module software version is denoted as follows :
*
*                       Vx.yy.zz
*
*                           where
*                                   V               denotes 'Version' label
*                                   x               denotes     major software version revision number
*                                   yy              denotes     minor software version revision number
*                                   zz              denotes sub-minor software version revision number
*
*               (b) The IPerf software version label #define is formatted as follows :
*
*                       ver = x.yyzz * 100 * 100
*
*                           where
*                                   ver             denotes software version number scaled as an integer value
*                                   x.yyzz          denotes software version number, where the unscaled integer
*                                                       portion denotes the major version number & the unscaled
*                                                       fractional portion denotes the (concatenated) minor
*                                                       version numbers
*********************************************************************************************************
*/

#define  IPERF_VERSION                                 20400u   /* See Note #1.                                         */


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               EXTERNS
*********************************************************************************************************
*********************************************************************************************************
*/

#ifdef   IPERF_MODULE
#define  IPERF_EXT
#else
#define  IPERF_EXT  extern
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*
* Note(s) : (1) The IPerf module files are located in the following directories :
*
*               (a) \<Your Product Application>\iperf_cfg.h
*
*               (b) \<Network Protocol Suite>\Source\net_*.*
*
*               (c) (1) \<IPerf>\Source\iperf.h
*                                      \iperf.c
*                                      \iperf-c.c
*                                      \iperf-s.c
*
*                   (2) \<IPerf>\OS\<os>\iperf_os.*
*
*                       where
*                               <Your Product Application>      directory path for Your Product's Application
*                               <Network Protocol Suite>        directory path for network protocol suite
*                               <IPerf>                         directory path for IPerf module
*                               <os>                            directory name for specific operating system (OS)
*
*           (2) CPU-configuration software files are located in the following directories :
*
*               (a) \<CPU-Compiler Directory>\cpu_*.*
*               (b) \<CPU-Compiler Directory>\<cpu>\<compiler>\cpu*.*
*
*                       where
*                               <CPU-Compiler Directory>        directory path for common CPU-compiler software
*                               <cpu>                           directory name for specific processor (CPU)
*                               <compiler>                      directory name for specific compiler
*
*           (3) NO compiler-supplied standard library functions SHOULD be used.
*
*               (a) Standard library functions are implemented in the custom library module(s) :
*
*                       \<Custom Library Directory>\lib_*.*
*
*                           where
*                                   <Custom Library Directory>      directory path for custom library software
*
*           (4) Compiler MUST be configured to include as additional include path directories :
*
*               (a) '\<Your Product Application>\' directory                            See Note #1a
*
*               (b) '\<Network Protocol Suite>\'   directory                            See Note #1b
*
*               (c) '\<IPerf>\' directories                                             See Note #1c
*
*               (d) (1) '\<CPU-Compiler Directory>\'                  directory         See Note #2a
*                   (2) '\<CPU-Compiler Directory>\<cpu>\<compiler>\' directory         See Note #2b
*
*               (e) '\<Custom Library Directory>\' directory                            See Note #3a
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <cpu.h>                                               /* CPU Configuration              (see Note #2b)        */
#include  <cpu_core.h>                                          /* CPU Core Library               (see Note #2a)        */

#include  <lib_def.h>                                           /* Standard        Defines        (see Note #3a)        */
#include  <lib_str.h>                                           /* Standard String Library        (see Note #3a)        */

#include  <iperf_cfg.h>                                         /* Iperf Configuration File       (see Note #1a)        */

#include  <Source/net.h>                                        /* Network Protocol Suite         (see Note #1b)        */
#include  <Source/net_tcp.h>
#include  <Source/net_app.h>
#include  <Source/net_ascii.h>

/*
*********************************************************************************************************
*********************************************************************************************************
*                                        MODULE CONFIGURATION
*********************************************************************************************************
*********************************************************************************************************
*/

#if     (IPERF_CFG_SERVER_EN == DEF_ENABLED)
#define  IPERF_SERVER_MODULE_PRESENT
#endif

#if     (IPERF_CFG_CLIENT_EN == DEF_ENABLED)
#define  IPERF_CLIENT_MODULE_PRESENT
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        IPERF BUFFER DEFINES
*********************************************************************************************************
*/


#define  IPERF_TCP_BUF_LEN_MAX             IPERF_CFG_BUF_LEN
#define  IPERF_UDP_BUF_LEN_MAX                          1472u   /* Dev buf MUST be equal or greater than ...            */
#define  IPERF_UDP_BUF_LEN_MAX_IPv6                     1450u   /* ... NET_BUF_DATA_IX_TX + IPERF_UDP_BUF_LEN_MAX.      */

#define  IPERF_BUF_LEN_MAX             IPERF_TCP_BUF_LEN_MAX


/*
*********************************************************************************************************
*                                        IPERF SERVER DEFINES
*********************************************************************************************************
*/

#define  IPERF_SERVER_TCP_CONN_Q_SIZE                      1u

#define  IPERF_SERVER_UDP_TX_FINACK_COUNT                 10u
#define  IPERF_SERVER_UDP_TX_FINACK_ERR_MAX               10u

#define  IPERF_SERVER_UDP_HEADER_VERSION1         0x80000000u


/*
*********************************************************************************************************
*                                    IPERF STRING COMMAND DEFINES
*********************************************************************************************************
*/

#define  IPERF_CMD_ARG_NBR_MAX                            12u   /* Max nbr of arg(s) a cmd may pass on the string ...   */
                                                                /* ... holding the complete command.                    */


/*
*********************************************************************************************************
*                             IPERF DEFAULT CONFIGURATION OPTION DEFINES
*********************************************************************************************************
*/

#define  IPERF_DFLT_PROTOCOL               IPERF_PROTOCOL_TCP   /* Dflt proto       is TCP.                             */
#define  IPERF_DFLT_TCP_BUF_LEN             IPERF_CFG_BUF_LEN   /* Dflt TCP buf     is maximum buf len.                 */
#define  IPERF_DFLT_UDP_BUF_LEN         IPERF_UDP_BUF_LEN_MAX   /* Dflt UDP buf     is 1472 bytes.                      */
#define  IPERF_DFLT_IP_REMOTE                        "0.0.0.0"  /* Dflt remote   IP is NOT set.                         */
#define  IPERF_DFLT_PORT                                5001u   /* Dflt port        is 5001.                            */
#define  IPERF_DFLT_BYTES_NBR                              0u   /* Dftl send by     is duration.                        */
#define  IPERF_DFLT_DURATION_MS                        10000u   /* Dflt duration    is 10 sec.                          */
#define  IPERF_DFLT_PERSISTENT                   DEF_DISABLED   /* Dflt persistent  is NOT en'd.                        */
#define  IPERF_DFLT_IF                                     1u   /* Dflt IF          is 1.                               */
#define  IPERF_DFLT_FMT             IPERF_ASCII_FMT_KBITS_SEC   /* Dflt fmt         is kbps.                            */
#define  IPERF_DFLT_RX_WIN      NET_TCP_DFLT_RX_WIN_SIZE_OCTET   /* Dflt rx win size is max.                             */
#define  IPERF_DFLT_TX_WIN      NET_TCP_DFLT_TX_WIN_SIZE_OCTET   /* Dflt tx win size is max.                             */
#define  IPERF_DFLT_INTERVAL_MS                         1000u   /* Dflt interval    is 1000 ms.                         */


/*
*********************************************************************************************************
*                                        IPERF TIMEOUT DEFINES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                  IPERF RETRY & TIME DELAY DEFINES
*********************************************************************************************************
*/

#define  IPERF_OPEN_MAX_RETRY                              2u   /* Max nbr of  retries on open.                         */
#define  IPERF_OPEN_MAX_DLY_MS                             5u   /* Dly between retries on open.                         */

#define  IPERF_BIND_MAX_RETRY                              2u   /* Max nbr of  retries on bind.                         */
#define  IPERF_BIND_MAX_DLY_MS                             5u   /* Dly between retries on bind.                         */

#define  IPERF_SERVER_TCP_RX_MAX_RETRY                     3u   /* Max nbr of  retries on rx.                           */
#define  IPERF_SERVER_UDP_RX_MAX_RETRY                     3u   /* Max nbr of  retries on rx.                           */

#define  IPERF_SERVER_UDP_TX_MAX_RETRY                     3u   /* Max nbr of  retries on tx.                           */
#define  IPERF_SERVER_UDP_TX_MAX_DLY_MS                   50u   /* Dly between retries on tx.                           */

#define  IPERF_CLIENT_UDP_RX_MAX_RETRY                     3u   /* Max nbr of  retries on rx.                           */

#define  IPERF_CLIENT_TCP_TX_MAX_RETRY                     3u   /* Max nbr of  retries on tx.                           */
#define  IPERF_CLIENT_TCP_TX_MAX_DLY_MS                    1u   /* Dly between retries on tx.                           */
#define  IPERF_CLIENT_UDP_TX_MAX_RETRY                     3u   /* Max nbr of  retries on tx.                           */
#define  IPERF_CLIENT_UDP_TX_MAX_DLY_MS                    1u   /* Dly between retries on tx.                           */


#define  IPERF_RX_UDP_FINACK_MAX_RETRY                    10u   /* Max nbr of retries  on rx'd in UDP FIN ACK.          */
#define  IPERF_RX_UDP_FINACK_MAX_DLY_MS                   50u   /* Dly between retries on rx'd in UDP FIN ACK.          */
#define  IPERF_RX_UDP_FINACK_MAX_TIMEOUT_MS               50u   /* Timeout for sock data  rx'd in UDP FIN ACK.          */


#define  IPERF_UDP_CPU_USAGE_CALC_DLY_MS                 200u   /* Dly between end of UDP and CPU usage calc.           */

/*
*********************************************************************************************************
*                                     IPERF ASCII OPTION DEFINES
*********************************************************************************************************
*/

#define  IPERF_ASCII_BEGIN_OPT                            '-'   /* Opt beginning.                                       */
#define  IPERF_ASCII_IP_SEP                               '.'   /* IP addr separator.                                   */
#define  IPERF_ASCII_OPT_HELP                             'h'   /* Help menu            opt.                            */
#define  IPERF_ASCII_OPT_VER                              'v'   /* Ver                  opt.                            */
#define  IPERF_ASCII_OPT_SERVER                           's'   /* Server               opt.                            */
#define  IPERF_ASCII_OPT_CLIENT                           'c'   /* Client               opt.                            */
#define  IPERF_ASCII_OPT_FMT                              'f'   /* Fmt                  opt.                            */
#define  IPERF_ASCII_OPT_LENGTH                           'l'   /* Buf len              opt.                            */
#define  IPERF_ASCII_OPT_TIME                             't'   /* Tx duration          opt.                            */
#define  IPERF_ASCII_OPT_NUMBER                           'n'   /* TX nbr bytes         opt.                            */
#define  IPERF_ASCII_OPT_PORT                             'p'   /* Port                 opt.                            */
#define  IPERF_ASCII_OPT_UDP                              'u'   /* UDP protocol         opt.                            */
#define  IPERF_ASCII_OPT_WINDOW                           'w'   /* Win size             opt.                            */
#define  IPERF_ASCII_OPT_PERSISTENT                       'D'   /* Server Persistent    opt.                            */
#define  IPERF_ASCII_OPT_IPV6                             'V'   /* IPV6                 opt.                            */
#define  IPERF_ASCII_OPT_INTERVAL                         'i'   /* Rate update interval opt.                            */

                                                                /* --------------- IPERF TRANSFER RATE ---------------- */
#define  IPERF_ASCII_FMT_BITS_SEC                         'b'   /*            bits/sec                                  */
#define  IPERF_ASCII_FMT_KBITS_SEC                        'k'   /*           Kbits/sec                                  */
#define  IPERF_ASCII_FMT_MBITS_SEC                        'm'   /*           Mbits/sec                                  */
#define  IPERF_ASCII_FMT_GBITS_SEC                        'g'   /*           Gbits/sec                                  */
#define  IPERF_ASCII_FMT_BYTES_SEC                        'B'   /*           bytes/sec                                  */
#define  IPERF_ASCII_FMT_KBYTES_SEC                       'K'   /*          Kbytes/sec                                  */
#define  IPERF_ASCII_FMT_MBYTES_SEC                       'M'   /*          Mbytes/sec                                  */
#define  IPERF_ASCII_FMT_GBYTES_SEC                       'G'   /*          Gbytes/sec                                  */
#define  IPERF_ASCII_FMT_ADAPTIVE_BITS_SEC                'a'   /* adaptive   bits/sec                                  */
#define  IPERF_ASCII_FMT_ADAPTIVE_BYTES_SEC               'A'   /* adaptive  bytes/sec                                  */

#define  IPERF_ASCII_SPACE                                ' '   /* ASCII val for space                                  */
#define  IPERF_ASCII_QUOTE                                '\"'  /* ASCII val for quote                                  */
#define  IPERF_ASCII_CMD_NAME_DELIMITER                   '_'   /* ASCII val for underscore                             */
#define  IPERF_ASCII_ARG_END                              '\0'  /* ASCII val for arg separator.                         */


/*
*********************************************************************************************************
*                                            ALIGN OPTION
*********************************************************************************************************
*/

#if 0                                                            /* #### NET-445                                         */
#if (NET_VERSION >= 21000u)
#define  IPERF_CFG_ALIGN_BUF_EN                 DEF_ENABLED
#else
#define  IPERF_CFG_ALIGN_BUF_EN                 DEF_DISABLED
#endif

#else
#define  IPERF_CFG_ALIGN_BUF_EN                 DEF_DISABLED
#endif


/*
*********************************************************************************************************
*                                               TRACING
*********************************************************************************************************
*/

                                                                /* Trace level, default to TRACE_LEVEL_OFF              */
#ifndef  TRACE_LEVEL_OFF
#define  TRACE_LEVEL_OFF                                   0
#endif

#ifndef  TRACE_LEVEL_INFO
#define  TRACE_LEVEL_INFO                                  1
#endif

#ifndef  TRACE_LEVEL_DBG
#define  TRACE_LEVEL_DBG                                   2
#endif

#ifndef  IPERF_TRACE_LEVEL
#define  IPERF_TRACE_LEVEL                   TRACE_LEVEL_OFF
#endif

#if    ( (defined(IPERF_TRACE))       && \
         (defined(IPERF_TRACE_LEVEL)) && \
         (IPERF_TRACE_LEVEL >= TRACE_LEVEL_INFO) )

    #if    (IPERF_TRACE_LEVEL >= TRACE_LEVEL_DBG)
        #define  IPERF_TRACE_DBG(msg)     IPERF_TRACE  msg
    #else
        #define  IPERF_TRACE_DBG(msg)
    #endif

    #define  IPERF_TRACE_INFO(msg)     IPERF_TRACE  msg

#else
    #define  IPERF_TRACE_DBG(msg)
    #define  IPERF_TRACE_INFO(msg)
#endif


/*
*********************************************************************************************************
*                                    IPERF OUTPUT MESSAGE DEFINES
*********************************************************************************************************
*/

#define  IPERF_MSG_MENU                   "Server specific:\n\r"                                                   \
                                          " -s              Run in server mode\n\r"                                \
                                          " -D              Run the server as persistent\n\r"                      \
                                          " -w              Rx TCP window size (IGNORED with UDP option)\n\r"      \
                                          "\n\r"                                                                   \
                                          "Client specific:\n\r"                                                   \
                                          " -c              Run in client mode\n\r"                                \
                                          " -t              Time in seconds to transmit for (default 10 secs)\n\r" \
                                          " -n              Number of bytes to transmit (instead of -t)\n\r"       \
                                          "<host>           IP address of <host> to connect to\n\r"                \
                                          "\n\r"                                                                   \
                                          "Options:\n\r"                                                           \
                                          " -f    [kmKM]    Format to report: kbits, mbits, Kbytes, MBytes\n\r"    \
                                          " -l              Length of buffer to read or write (default 8 KB)\n\r"  \
                                          " -p              Server port to listen on/connect to\n\r"               \
                                          " -u              Use UDP rather than TCP\n\r"                           \
                                          " -i              MILISECONDS between periodic bandwidth report\n\r"     \
                                          "\n\r"                                                                   \
                                          "Miscellaneous:\n\r"                                                     \
                                          " -h              Print this message\n\r"                                \
                                          " -v              Print version information\n\r"

#define  IPERF_MSG_VER_STR_MAX_LEN                         8u

#define  IPERF_MSG_ERR_BUF_LEN            "Buffer length specified exceed maximum buffer length\n\r"

#define  IPERF_MSG_ERR_UDP_LEN            "UDP Protocol don't support fragmentation\n\r" \
                                          "UDP max buffer length is 1460 bytes"

#define  IPERF_MSG_ERR_OPT                "Invalid option\n\r"                     \
                                          "Usage: IPerf [-s|c host] [options]\n\r" \
                                          "Try IPerf -h for more information\n\r"

#define  IPERF_MSG_ERR_OPT_NOT_SUPPORTED  "Option not supported/implemented\n\r"

#define  IPERF_MSG_ERR_NOT_EN             "Option NOT Enabled\n\r"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                     IPERF ERROR CODES DATA TYPE
*********************************************************************************************************
*/

typedef  enum iperf_err {

    IPERF_ERR_NONE                        =           0u,  /* No err.                                              */

    IPERF_ERR_CPU_TS_FREQ                 =           1u,  /* CPU timestamp's timer frequency invalid &/or         */
                                                           /*     NOT yet configured.                              */

    IPERF_ERR_TEST_NONE_AVAIL             =          10u,  /* Iperf Test List full.                                */
    IPERF_ERR_TEST_NOT_FOUND              =          11u,  /* Test not found in IPerf Test List.                   */
    IPERF_ERR_TEST_INVALID_ID             =          12u,  /* Invalid test ID.                                     */
    IPERF_ERR_TEST_INVALID_RESULT         =          13u,  /* Invalid test result.                                 */
    IPERF_ERR_TEST_RUNNING                =          14u,  /* Test running and can't be remove.                    */


    IPERF_ERR_ARG_TBL_FULL                =          20u,  /* Arg tbl full.                                        */
    IPERF_ERR_ARG_NO_ARG                  =          21u,  /* No arg.                                              */
    IPERF_ERR_ARG_NO_VAL                  =          22u,  /* No val for arg opt.                                  */
    IPERF_ERR_ARG_NO_TEST                 =          23u,  /* No test to be processed (only version and or help).  */
    IPERF_ERR_ARG_INVALID_MODE            =          24u,  /* Server or Client mode not en.                        */
    IPERF_ERR_ARG_INVALID_OPT             =          25u,  /* Invalid arg opt.                                     */
    IPERF_ERR_ARG_INVALID_PTR             =          26u,  /* Null ptr for argv.                                   */
    IPERF_ERR_ARG_INVALID_VAL             =          27u,  /* Invalid arg opt val.                                 */
    IPERF_ERR_ARG_INVALID_REMOTE_ADDR     =          28u,  /* Err setting remote addr.                             */
    IPERF_ERR_ARG_EXCEED_MAX_LEN          =          29u,  /* Len val exceed buf size (TCP or UDP).                */
    IPERF_ERR_ARG_EXCEED_MAX_UDP_LEN      =          30u,  /* UDP buf len exceed max udp len.                      */
    IPERF_ERR_ARG_PARAM_NOT_SUPPORTED_OPT =          31u,  /* Arg opt     not supported.                           */
    IPERF_ERR_ARG_PARAM_NOT_SUPPORTED_VAL =          32u,  /* Arg opt val not supported.                           */

    IPERF_ERR_SERVER_SOCK_OPEN            =          50u,  /* Server err on sock open.                             */
    IPERF_ERR_SERVER_SOCK_CLOSE           =          51u,  /* Server err on sock close.                            */
    IPERF_ERR_SERVER_SOCK_BIND            =          52u,  /* Server err on sock bind.                             */
    IPERF_ERR_SERVER_SOCK_LISTEN          =          53u,  /* Server err on sock listen.                           */
    IPERF_ERR_SERVER_SOCK_ACCEPT          =          54u,  /* Server err on sock accept.                           */
    IPERF_ERR_SERVER_SOCK_RX              =          55u,  /* Server err on sock rx.                               */
    IPERF_ERR_SERVER_WIN_SIZE             =          56u,  /* Server err on win size setup.                        */
    IPERF_ERR_SERVER_INVALID_IP_FAMILY    =          57u,  /* Server err on reading IP family.                     */
    IPERF_ERR_SERVER_SOCK_OPT             =          58u,  /* Server err on configuring socket options.             */

    IPERF_ERR_CLIENT_SOCK_OPEN            =          70u,  /* Client err on sock open.                             */
    IPERF_ERR_CLIENT_SOCK_CLOSE           =          71u,  /* Client err on sock close.                            */
    IPERF_ERR_CLIENT_SOCK_BIND            =          72u,  /* Client err on sock bind.                             */
    IPERF_ERR_CLIENT_SOCK_CONN            =          73u,  /* Client err on sock conn.                             */
    IPERF_ERR_CLIENT_SOCK_TX              =          74u,  /* Client err on sock tx.                               */
    IPERF_ERR_CLIENT_SOCK_TX_INV_ARG      =          75u,  /* Client err on sock tx arg.                           */

    IPERF_ERR_CLIENT_INVALID_IP           =          90u,   /* Client err on reading IP conversion.                 */
    IPERF_ERR_CLIENT_INVALID_IP_FAMILY    =          91u,   /* Client err on reading IP family.                     */
    IPERF_ERR_CLIENT_SOCK_OPT             =          92u,   /* Client err on setting socket options.                */

    IPERF_OS_ERR_NONE                     =        1000u,

    IPERF_OS_ERR_INIT_Q                   =        1001u,
    IPERF_OS_ERR_INIT_TASK                =        1002u,
    IPERF_OS_ERR_Q                        =        1005u,


    IPERF_ERR_EXT_SHELL_INIT              =        2001u
} IPERF_ERR;


/*
*********************************************************************************************************
*                                         IPERF MODE DATA TYPE
*********************************************************************************************************
*/

typedef  enum  iperf_mode {
    IPERF_MODE_SERVER = 0u,                                     /* Server IPerf mode.                                   */
    IPERF_MODE_CLIENT = 1u                                      /* Client IPerf mode.                                   */
} IPERF_MODE;


/*
*********************************************************************************************************
*                                      IPERF PROTOCOL DATA TYPE
*********************************************************************************************************
*/

typedef  enum  iperf_protocol {
    IPERF_PROTOCOL_TCP = 0u,                                    /* IPerf opt protocol TCP.                              */
    IPERF_PROTOCOL_UDP = 1u                                     /* IPerf opt protocol UDP.                              */
} IPERF_PROTOCOL;


/*
*********************************************************************************************************
*                                       IPERF TEST ID DATA TYPE
*********************************************************************************************************
*/

typedef  CPU_INT16U  IPERF_TEST_ID;

#define  IPERF_TEST_ID_NONE                                0u   /* Null IPerf test ID.                                  */
#define  IPERF_TEST_ID_INIT                                1u   /* First test ID, set at init.                          */


/*
*********************************************************************************************************
*                                     IPERF TEST STATUS DATA TYPE
*********************************************************************************************************
*/

typedef  enum  iperf_status {
    IPERF_TEST_STATUS_FREE    = 0u,                             /* Test unused.                                         */
    IPERF_TEST_STATUS_QUEUED  = 1u,                             /* Test Q'd                                             */
    IPERF_TEST_STATUS_RUNNING = 2u,                             /* Test currently run.                                  */
    IPERF_TEST_STATUS_DONE    = 3u,                             /* Test done      with no err.                          */
    IPERF_TEST_STATUS_ERR     = 4u,                             /* Test done      with    err.                          */
} IPERF_TEST_STATUS;


/*
*********************************************************************************************************
*                                      IPERF TIMESTAMP DATA TYPE
*********************************************************************************************************
*/

typedef  CPU_TS32  IPERF_TS_MS;

#define  IPERF_TS_MS_MAX_VAL  DEF_INT_32U_MAX_VAL


/*
*********************************************************************************************************
*                                  IPERF DATA FORMAT UNIT DATA TYPE
*********************************************************************************************************
*/

typedef  CPU_CHAR  IPERF_FMT;


/*
*********************************************************************************************************
*                                  IPERF COMMAND PARAMETER DATA TYPE
*
* Note(s) : (1) This structure is used to pass additional parameters to the output function.
*
*           (2) This variable is not used by IPerf, this variable is present to be compliant with shell.
*********************************************************************************************************
*/

typedef  struct  IPERF_OUT_PARAM {
    void         *p_cur_working_dir;                            /* Cur working dir ptr.                                 */
    void         *p_out_opt;                                    /* Output opt      ptr.                                 */
    CPU_BOOLEAN  *p_session_active;                             /* Session status flag (see Note #2).                   */
} IPERF_OUT_PARAM;


/*
*********************************************************************************************************
*                              IPERF COMMAND FUNCTION POINTER DATA TYPE
*********************************************************************************************************
*/

typedef  void  (*IPERF_OUT_FNCT)(CPU_CHAR         *,
                                 IPERF_OUT_PARAM  *);


/*
*********************************************************************************************************
*                                     IPERF TEST OPTION DATA TYPE
*********************************************************************************************************
*/

typedef  struct  iperf_opt {
    IPERF_MODE      Mode;                                               /* Server or client mode.                            */
    IPERF_PROTOCOL  Protocol;                                           /* UDP    or TCP protocol.                           */
    CPU_INT16U      Port;                                               /* Server or client port.                            */
    CPU_BOOLEAN     IPv4;                                               /* IPv4 or IPv6  protocol.                           */
    CPU_CHAR        IP_AddrRemote[NET_ASCII_LEN_MAX_ADDR_IP + 1u];      /* IP Addr Remote to tx.                             */
    CPU_INT16U      BytesNbr;                                           /* Nbr of bytes   to tx.                             */
    CPU_INT16U      BufLen;                                             /* Buf len        to tx or rx.                       */
    CPU_INT16U      Duration_ms;                                        /* Time in sec    to tx.                             */
    CPU_INT16U      WinSize;                                            /* Win size       to tx or rx.                       */
    CPU_BOOLEAN     Persistent;                                         /* Server in persistent mode.                        */
    IPERF_FMT       Fmt;                                                /* Result rate fmt.                                  */
    CPU_INT16U      Interval_ms;                                        /* Interval (ms) between bandwidth update.           */
} IPERF_OPT;


/*
*********************************************************************************************************
*                                     IPERF STATISTICS DATA TYPE
*********************************************************************************************************
*/

typedef  struct  iperf_stats {
    CPU_INT32U   NbrCalls;                                      /* Nbr of I/O sys calls.                                */

    CPU_INT32U   Bytes;                                         /* Nbr of bytes rx'd or tx'd on net.                    */
    CPU_INT32U   Errs;                                          /* Nbr of       rx   or tx  errs.                       */
    CPU_INT32U   TransitoryErrCnts;                             /* Nbr of transitory err.                               */

    CPU_INT32S   UDP_RxLastPkt;                                 /* Prev         pkt ID rx'd                             */
    CPU_INT32U   UDP_LostPkt;                                   /* Nbr  of  UDP pkt lost                                */
    CPU_INT32U   UDP_OutOfOrder;                                /* Nbr  of      pkt    rx'd out of order.               */
    CPU_INT32U   UDP_DupPkt;                                    /* Nbr  of      pkt ID rx'd more than once.             */
    CPU_BOOLEAN  UDP_AsyncErr;                                  /* First    UDP pkt    rx'd.                            */
    CPU_BOOLEAN  UDP_EndErr;                                    /* Err with UDP FIN or FINACK.                          */

    IPERF_TS_MS  TS_Start_ms;                                   /* Start timestamp (ms).                                */
    IPERF_TS_MS  TS_End_ms;                                     /* End   timestamp (ms).                                */

#if (IPERF_CFG_BANDWIDTH_CALC_EN == DEF_ENABLED)
    CPU_INT32U   Bandwidth;                                     /* Rx or Tx cur bandwidth.                              */
#endif

#if (IPERF_CFG_CPU_USAGE_MAX_CALC_EN == DEF_ENABLED)
    CPU_INT32U   CPU_UsageMax;                                  /* Max CPU usage reached.                               */
    CPU_INT32U   CPU_UsageAvg;
    CPU_INT32U   CPU_CalcNbr;
#endif
} IPERF_STATS;


/*
*********************************************************************************************************
*                              IPERF LOCAL & REMOTE CONNECTION DATA TYPE
*********************************************************************************************************
*/

typedef  struct  iperf_conn {
    NET_SOCK_ID       SockID;                                   /* Sock          used by server and client.             */
    NET_SOCK_ID       SockID_TCP_Server;                        /* Accepted sock used by TCP server to rx.              */
    NET_SOCK_ADDR     ServerAddrPort;                           /* Server sock addr IP.                                 */
    NET_SOCK_ADDR     ClientAddrPort;                           /* Client sock addr IP.                                 */
    NET_IF_NBR        IF_Nbr;                                   /* Local IF to tx or rx.                                */
    CPU_BOOLEAN       Run;                                      /* Server (rx'd) or client (tx'd) started.              */
} IPERF_CONN;


/*
*********************************************************************************************************
*                                        IPERF TEST DATA TYPE
*********************************************************************************************************
*/

typedef  struct  iperf_test  IPERF_TEST;

struct  iperf_test {
    IPERF_TEST_ID       TestID;                                 /* IPerf test         ID.                               */
    IPERF_TEST_STATUS   Status;                                 /* IPerf cur          status.                           */
    IPERF_ERR           Err;                                    /* IPerf err          storage.                          */
    IPERF_OPT           Opt;                                    /* IPerf test         opt data.                         */
    IPERF_STATS         Stats;                                  /* IPerf stats/result     data.                         */
    IPERF_CONN          Conn;                                   /* IPerf conn             data.                         */
    IPERF_TEST         *PrevPtr;                                /* Ptr to PREV IPerf test.                              */
    IPERF_TEST         *NextPtr;                                /* Ptr to NEXT IPerf test.                              */
};


/*
*********************************************************************************************************
*                                    IPERF UDP DATAGRAM DATA TYPE
*********************************************************************************************************
*/

typedef  struct  iperf_udp_datagram {
    CPU_INT32S  ID;                                             /* Pkt  ID.                                             */
    CPU_INT32U  TimeVar_sec;                                    /* Time var for  sec.                                   */
    CPU_INT32U  TimeVar_usec;                                   /* Time var for usec.                                   */
} IPERF_UDP_DATAGRAM;


/*
*********************************************************************************************************
*                                  IPERF UDP SERVER HEADER DATA TYPE
*********************************************************************************************************
*/

typedef  struct  iperf_server_udp_hdr {
    CPU_INT32S  Flags;                                          /* Server flag.                                         */
    CPU_INT32U  TotLen_Hi;                                      /* Tot  bytes rx'd hi  part.                            */
    CPU_INT32U  TotLen_Lo;                                      /* Tot  bytes rx'd low part.                            */
    CPU_INT32S  Stop_sec;                                       /* Stop time  in  sec.                                  */
    CPU_INT32S  Stop_usec;                                      /* Stop time  in usec.                                  */
    CPU_INT32U  LostPkt_ctr;                                    /* Lost pkt   cnt.                                      */
    CPU_INT32U  OutOfOrder_ctr;                                 /* Rx   pkt   out of order cnt.                         */
    CPU_INT32S  RxLastPkt;                                      /* Last pkt   ID  rx'd.                                 */
    CPU_INT32U  Jitter_Hi;                                      /* Jitter hi.                                           */
    CPU_INT32U  Jitter_Lo;                                      /* Jitter low.                                          */
} IPERF_SERVER_UDP_HDR;


/*
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*/
                                                                /* Buf used to tx or rx.                                */
IPERF_EXT  CPU_CHAR          IPerf_Buf[IPERF_CFG_BUF_LEN + (CPU_CFG_DATA_SIZE - 1)];

                                                                /* IPerf tests tbl (opt, conn, & stats data).           */
IPERF_EXT  IPERF_TEST        IPerf_TestTbl[IPERF_CFG_MAX_NBR_TEST];

IPERF_EXT  IPERF_TEST       *IPerf_TestPoolPtr;                 /* Ptr to pool of free test.                            */

IPERF_EXT  IPERF_TEST       *IPerf_TestListHeadPtr;             /* Ptr to head of IPerf test Grp List.                  */

IPERF_EXT  IPERF_TEST_ID     IPerf_NextTestID;

IPERF_EXT  CPU_TS_TMR_FREQ   IPerf_CPU_TmrFreq;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*                                         DEFINED IN iperf.c
*********************************************************************************************************
*/

void               IPerf_Init           (IPERF_ERR        *p_err);

void               IPerf_TestTaskHandler(void);


IPERF_TEST_ID      IPerf_TestStart      (CPU_CHAR         *argv,
                                         IPERF_OUT_FNCT    p_out_fnct,
                                         IPERF_OUT_PARAM  *p_out_opt,
                                         IPERF_ERR        *p_err);

IPERF_TEST_ID      IPerf_TestShellStart (CPU_INT16U        argc,
                                         CPU_CHAR         *p_argv[],
                                         IPERF_OUT_FNCT    p_out_fnct,
                                         IPERF_OUT_PARAM  *p_out_param,
                                         IPERF_ERR        *p_err);

void               IPerf_TestRelease    (IPERF_TEST_ID     test_id,
                                         IPERF_ERR        *p_err);

IPERF_TEST_STATUS  IPerf_TestGetStatus  (IPERF_TEST_ID     test_id,
                                         IPERF_ERR        *p_err);

void               IPerf_TestGetResults (IPERF_TEST_ID     test_id,
                                         IPERF_TEST       *p_test_result,
                                         IPERF_ERR        *p_err);

void               IPerf_TestClrStats   (IPERF_STATS      *p_stats);


IPERF_TS_MS        IPerf_Get_TS_ms      (void);

IPERF_TS_MS        IPerf_Get_TS_Max_ms  (void);


CPU_INT32U         IPerf_GetDataFmtd    (IPERF_FMT         fmt,
                                         CPU_INT32U        bytes_qty);


#if (IPERF_CFG_BANDWIDTH_CALC_EN == DEF_ENABLED)
void               IPerf_UpdateBandwidth(IPERF_TEST       *p_test,
                                         IPERF_TS_MS      *p_ts_ms_prev,
                                         CPU_INT32U       *p_data_bytes_prev);
#endif


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*                                   DEFINED IN SERVER'S  iperf-s.c
*********************************************************************************************************
*/

#ifdef  IPERF_SERVER_MODULE_PRESENT
void  IPerf_ServerStart(IPERF_TEST  *p_test,
                        IPERF_ERR   *p_err);
#endif


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*                                   DEFINED IN CLIENT'S  iperf-c.c
*********************************************************************************************************
*/

#ifdef  IPERF_CLIENT_MODULE_PRESENT
void  IPerf_ClientStart(IPERF_TEST  *p_test,
                        IPERF_ERR   *p_err);
#endif


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*                                     DEFINED IN OS'S  iperf_os.c
*********************************************************************************************************
*/

void           IPerf_OS_Init      (IPERF_ERR      *p_err);

void           IPerf_OS_TestQ_Post(IPERF_TEST_ID   test_id,
                                   IPERF_ERR      *p_err);

IPERF_TEST_ID  IPerf_OS_TestQ_Wait(IPERF_ERR      *p_err);

#if (IPERF_CFG_CPU_USAGE_MAX_CALC_EN == DEF_ENABLED)
CPU_INT16U     IPerf_OS_CPU_Usage (void);
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                     IPERF CONFIGURATION ERRORS
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  IPERF_CFG_SERVER_EN
    #error  "IPERF_CFG_SERVER_EN not #define'd in 'iperf_cfg.h' [MUST be DEF_DISABLED || DEF_ENABLED]"
#elif  ((IPERF_CFG_SERVER_EN != DEF_ENABLED ) && \
        (IPERF_CFG_SERVER_EN != DEF_DISABLED))
    #error  "IPERF_CFG_SERVER_EN illegally #define'd in 'iperf_cfg.h' [MUST be DEF_DISABLED || DEF_ENABLED] "


#elif   (IPERF_CFG_SERVER_EN == DEF_ENABLED)

    #ifndef  IPERF_CFG_SERVER_ACCEPT_MAX_RETRY
        #error  "IPERF_CFG_SERVER_ACCEPT_MAX_RETRY not #define'd in 'iperf_cfg.h' [MUST be >= 0]"

    #elif   (IPERF_CFG_SERVER_ACCEPT_MAX_RETRY      < 0)
        #error  "IPERF_CFG_SERVER_ACCEPT_MAX_RETRY illegally #define'd in 'iperf_cfg.h' [MUST be >= 0]"
    #endif

    #ifndef  IPERF_CFG_SERVER_ACCEPT_MAX_DLY_MS
        #error  "IPERF_CFG_SERVER_ACCEPT_MAX_DLY_MS not #define'd in 'iperf_cfg.h' [MUST be >= 0]"

    #elif   (IPERF_CFG_SERVER_ACCEPT_MAX_DLY_MS     < 0)
        #error  "IPERF_CFG_SERVER_ACCEPT_MAX_DLY_MS illegally #define'd in 'iperf_cfg.h' [MUST be >= 0]"
    #endif

    #ifndef  IPERF_CFG_SERVER_ACCEPT_MAX_TIMEOUT_MS
        #error  "IPERF_CFG_SERVER_ACCEPT_MAX_TIMEOUT_MS not #define'd in 'iperf_cfg.h' [MUST be >= 0]"

    #elif   (IPERF_CFG_SERVER_ACCEPT_MAX_TIMEOUT_MS < 0)
        #error  "IPERF_CFG_SERVER_ACCEPT_MAX_TIMEOUT_MS illegally #define'd in 'iperf_cfg.h' [MUST be >= 0]"
    #endif

    #ifndef  IPERF_CFG_SERVER_TCP_RX_MAX_TIMEOUT_MS
        #error  "IPERF_CFG_SERVER_TCP_RX_MAX_TIMEOUT_MS not #define'd in 'iperf_cfg.h' [MUST be >= 0]"

    #elif   (IPERF_CFG_SERVER_TCP_RX_MAX_TIMEOUT_MS < 0)
        #error  "IPERF_CFG_SERVER_TCP_RX_MAX_TIMEOUT_MS illegally #define'd in 'iperf_cfg.h' [MUST be >= 0]"
    #endif

    #ifndef  IPERF_CFG_SERVER_UDP_RX_MAX_TIMEOUT_MS
        #error  "IPERF_CFG_SERVER_UDP_RX_MAX_TIMEOUT_MS not #define'd in 'iperf_cfg.h' [MUST be >= 0]"

    #elif   (IPERF_CFG_SERVER_UDP_RX_MAX_TIMEOUT_MS < 0)
        #error  "IPERF_CFG_SERVER_UDP_RX_MAX_TIMEOUT_MS illegally #define'd in 'iperf_cfg.h' [MUST be >= 0]"
    #endif


#endif



#ifndef  IPERF_CFG_CLIENT_EN
    #error  "IPERF_CFG_CLIENT_EN not #define'd in 'iperf_cfg.h' [MUST be DEF_DISABLED || DEF_ENABLED]"
#elif  ((IPERF_CFG_CLIENT_EN != DEF_ENABLED ) && \
        (IPERF_CFG_CLIENT_EN != DEF_DISABLED))
    #error  "IPERF_CFG_CLIENT_EN illegally #define'd in 'iperf_cfg.h' [MUST be DEF_DISABLED || DEF_ENABLED]"


#elif   (IPERF_CFG_CLIENT_EN == DEF_ENABLED)


    #ifndef  IPERF_CFG_CLIENT_BIND_EN
        #error  "IPERF_CFG_CLIENT_BIND_EN not #define'd in 'iperf_cfg.h' MUST be DEF_DISABLED || DEF_ENABLED]"
    #elif  ((IPERF_CFG_CLIENT_BIND_EN != DEF_ENABLED ) && \
            (IPERF_CFG_CLIENT_BIND_EN != DEF_DISABLED))
        #error  "IPERF_CFG_CLIENT_BIND_EN illegally #define'd in 'iperf_cfg.h' [MUST be DEF_DISABLED || DEF_ENABLED]"
    #endif


    #ifndef  IPERF_CFG_CLIENT_CONN_MAX_RETRY
        #error  "IPERF_CFG_CLIENT_CONN_MAX_RETRY not #define'd in 'iperf_cfg.h' [MUST be >= 0]"

    #elif   (IPERF_CFG_CLIENT_CONN_MAX_RETRY        < 0)
        #error  "IPERF_CFG_CLIENT_CONN_MAX_RETRY illegally #define'd in 'iperf_cfg.h' [MUST be >= 0]"
    #endif

    #ifndef  IPERF_CFG_CLIENT_CONN_MAX_DLY_MS
        #error  "IPERF_CFG_CLIENT_CONN_MAX_DLY_MS not #define'd in 'iperf_cfg.h' [MUST be >= 0]"

    #elif   (IPERF_CFG_CLIENT_CONN_MAX_DLY_MS       < 0)
        #error  "IPERF_CFG_CLIENT_CONN_MAX_DLY_MS illegally #define'd in 'iperf_cfg.h' [MUST be >= 0]"
    #endif

    #ifndef  IPERF_CFG_CLIENT_CONN_MAX_TIMEOUT_MS
        #error  "IPERF_CFG_CLIENT_CONN_MAX_TIMEOUT_MS not #define'd in 'iperf_cfg.h' [MUST be >= 0]"

    #elif   (IPERF_CFG_CLIENT_CONN_MAX_TIMEOUT_MS   < 0)
        #error  "IPERF_CFG_CLIENT_CONN_MAX_TIMEOUT_MS illegally #define'd in 'iperf_cfg.h' [MUST be >= 0]"
    #endif

    #ifndef  IPERF_CFG_CLIENT_TCP_TX_MAX_TIMEOUT_MS
        #error  "IPERF_CFG_CLIENT_TCP_TX_MAX_TIMEOUT_MS not #define'd in 'iperf_cfg.h' [MUST be >= 0]"

    #elif   (IPERF_CFG_CLIENT_TCP_TX_MAX_TIMEOUT_MS < 0)
        #error  "IPERF_CFG_CLIENT_TCP_TX_MAX_TIMEOUT_MS illegally #define'd in 'iperf_cfg.h' [MUST be >= 0]"
    #endif

#endif



#if    ((IPERF_CFG_SERVER_EN != DEF_ENABLED) && \
        (IPERF_CFG_CLIENT_EN != DEF_ENABLED))
    #error  "IPERF_CFG_SERVER_EN and/or IPERF_CFG_CLIENT_EN illegally #define'd in 'iperf_cfg.h'."
#endif



                                                                /* Correctly configured in 'iperf.h'; DO NOT MODIFY.    */
#ifndef  IPERF_CFG_ALIGN_BUF_EN
    #error  "IPERF_CFG_ALIGN_BUF_EN not #define'd in 'iperf.h' [MUST be DEF_DISABLED || DEF_ENABLED]"

#elif  ((IPERF_CFG_ALIGN_BUF_EN != DEF_ENABLED ) && \
        (IPERF_CFG_ALIGN_BUF_EN != DEF_DISABLED))
    #error  "IPERF_CFG_ALIGN_BUF_EN illegally #define'd in 'iperf.h' [MUST be DEF_DISABLED || DEF_ENABLED]"
#endif



#ifndef  IPERF_CFG_BANDWIDTH_CALC_EN
    #error  "IPERF_CFG_BANDWIDTH_CALC_EN not #define'd in 'iperf_cfg.h' [MUST be DEF_DISABLED || DEF_ENABLED]"
#elif  ((IPERF_CFG_BANDWIDTH_CALC_EN != DEF_ENABLED ) && \
        (IPERF_CFG_BANDWIDTH_CALC_EN != DEF_DISABLED))
    #error  "IPERF_CFG_BANDWIDTH_CALC_EN illegally #define'd in 'iperf_cfg.h' [MUST be DEF_DISABLED || DEF_ENABLED]"
#endif



#ifndef  IPERF_CFG_CPU_USAGE_MAX_CALC_EN
    #error  "IPERF_CFG_CPU_USAGE_MAX_CALC_EN not #define'd in 'iperf_cfg.h' [MUST be DEF_DISABLED || DEF_ENABLED]"
#elif  ((IPERF_CFG_CPU_USAGE_MAX_CALC_EN != DEF_ENABLED ) && \
        (IPERF_CFG_CPU_USAGE_MAX_CALC_EN != DEF_DISABLED))
    #error  "IPERF_CFG_CPU_USAGE_MAX_CALC_EN illegally #define'd in 'iperf_cfg.h' [MUST be DEF_DISABLED || DEF_ENABLED]"
#endif



#ifndef  IPERF_CFG_BUF_LEN
    #error  "IPERF_CFG_BUF_LEN not #define'd in 'iperf_cfg.h' [MUST be > 0]"

#elif   (IPERF_CFG_BUF_LEN < IPERF_UDP_BUF_LEN_MAX)
    #error  "IPERF_CFG_BUF_LEN illegally #define'd in 'iperf_cfg.h' [MUST be > IPERF_UDP_BUF_LEN_MAX]"
#endif


/*
*********************************************************************************************************
*                                    NETWORK CONFIGURATION ERRORS
*********************************************************************************************************
*/

                                                                /* See 'iperf.h  Note #1'.                              */
#if     (NET_VERSION < 205u)
    #error  "NET_VERSION                  [MUST be  >= V2.05]"
#endif



/*
*********************************************************************************************************
*                                      CPU CONFIGURATION ERRORS
*********************************************************************************************************
*/

#if     (CPU_CFG_TS_EN != DEF_ENABLED)
    #error  "CPU_CFG_TS_32_EN || CPU_CFG_TS_64_EN illegally #define'd in 'net_cfg.h' [MUST be DEF_ENABLED]"
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif                                                          /* End of IPerf module include.                         */

