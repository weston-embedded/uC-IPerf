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
*                                      IPERF CONFIGURATION FILE
*
*                                              TEMPLATE
*
* Filename : iperf_cfg.h
* Version  : V2.04.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           TASKS PRIORITIES
* Notes: (1) Task priorities configuration values should be used by the IPerf OS port. The following task priorities
*            should be defined:
*
*                   IPERF_OS_CFG_TASK_PRIO
*
*            Task priorities can be defined either in this configuration file 'iperf_cfg.h' or in a global
*            OS tasks priorities configuration header file which must be included in 'iperf_cfg.h'.
*********************************************************************************************************
*/

#define  IPERF_OS_CFG_TASK_PRIO                           12u   /* IPerf task priority.                                 */


/*
*********************************************************************************************************
*                                             STACK SIZES
*                            Size of the task stack (# of OS_STK entries)
*********************************************************************************************************
*/

#define  IPERF_OS_CFG_TASK_STK_SIZE                     1024u   /* IPerf task stack size.                               */


/*
*********************************************************************************************************
*                                                IPERF
*
* Note(s) : (1) Configures the maximum number of tests that can be started and/or running at a
*               given time.
*
*           (2) Configure the size of the ring array holding the tests results. The maximum number of test
*               SHOULD be equal or greater than the Iperf queue size.
*********************************************************************************************************
*/

#define  IPERF_CFG_Q_SIZE                                  2u   /* Configure IPerf queue size (see Note #1).            */

#define  IPERF_CFG_MAX_NBR_TEST                            5u   /* Configure the maximum number of tests (See Note #2). */

                                                                /* Configure IPerf bandwidth calculation :              */
#define  IPERF_CFG_BANDWIDTH_CALC_EN              DEF_DISABLED
                                                                /* DEF_ENABLED     Bandwidth calculation ENABLED        */
                                                                /* DEF_DISABLED    Bandwidth calculation DISABLED       */

                                                                /* Configure IPerf CPU usage calculation :              */
#define  IPERF_CFG_CPU_USAGE_MAX_CALC_EN          DEF_ENABLED
                                                                /* DEF_ENABLED     CPU usage calculation ENABLED        */
                                                                /* DEF_DISABLED    CPU usage calculation DISABLED       */

#define  IPERF_CFG_BUF_LEN                              8192u   /* Configure maximum buffer size used to send/receive.  */



                                                                /* Configure IPerf server :                             */
#define  IPERF_CFG_SERVER_EN                      DEF_ENABLED
                                                                /* DEF_ENABLED     UDP & TCP server ENABLED             */
                                                                /* DEF_DISABLED    UDP & TCP server DISABLED            */

#define  IPERF_CFG_SERVER_ACCEPT_MAX_RETRY                10u   /* Configure server maximum of    retries   on accept.  */

#define  IPERF_CFG_SERVER_ACCEPT_MAX_DLY_MS              500u   /* Configure server delay between retries   on accept.  */

#define  IPERF_CFG_SERVER_ACCEPT_MAX_TIMEOUT_MS         5000u   /* Configure server maximum inactivity time on accept.  */

#define  IPERF_CFG_SERVER_TCP_RX_MAX_TIMEOUT_MS         5000u   /* Configure server maximum inactivity time on TCP Rx.  */

#define  IPERF_CFG_SERVER_UDP_RX_MAX_TIMEOUT_MS         5000u   /* Configure server maximum inactivity time on UDP Rx.  */



                                                                /* Configure IPerf client :                             */
#define  IPERF_CFG_CLIENT_EN                      DEF_ENABLED
                                                                /* DEF_ENABLED     UDP & TCP client ENABLED             */
                                                                /* DEF_DISABLED    UDP & TCP client DISABLED            */

                                                                /* Configure client to bind on same port as server      */
#define  IPERF_CFG_CLIENT_BIND_EN                 DEF_DISABLED
                                                                /* DEF_ENABLED     bind client ENABLED                  */
                                                                /* DEF_DISABLED    bind client DISABLED                 */

#define  IPERF_CFG_CLIENT_CONN_MAX_RETRY                  10u   /* Configure client maximum of    retries   on connect. */

#define  IPERF_CFG_CLIENT_CONN_MAX_DLY_MS                500u   /* Configure client delay between retries   on connect. */

#define  IPERF_CFG_CLIENT_CONN_MAX_TIMEOUT_MS           5000u   /* Configure client maximum inactivity time on connect. */

#define  IPERF_CFG_CLIENT_TCP_TX_MAX_TIMEOUT_MS         5000u   /* Configure client maximum inactivity time on TCP Tx.  */


/*
*********************************************************************************************************
*                                     TRACE / DEBUG CONFIGURATION
*********************************************************************************************************
*/

#ifndef  TRACE_LEVEL_OFF
#define  TRACE_LEVEL_OFF                                   0
#endif

#ifndef  TRACE_LEVEL_INFO
#define  TRACE_LEVEL_INFO                                  1
#endif

#ifndef  TRACE_LEVEL_DBG
#define  TRACE_LEVEL_DBG                                   2
#endif

#define  IPERF_TRACE_LEVEL                   TRACE_LEVEL_OFF
#define  IPERF_TRACE                                  printf
