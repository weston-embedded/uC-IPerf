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
*                                    IPERF OPERATING SYSTEM LAYER
*
*                                          Micrium uC/OS-III
*
* Filename : iperf_os.c
* Version  : V2.04.00
*********************************************************************************************************
* Note(s)  : (1) Assumes uC/OS-III V3.01.0 (or more recent version) is included in the project build.
*
*            (2) REQUIREs the following uC/OS-III feature(s) to be ENABLED :
*
*                    ------- FEATURE ------        -- MINIMUM CONFIGURATION FOR IPERF/OS PORT --

*                (a) Messages                      OS_CFG_MSG_POOL_SIZE >= IPERF_CFG_Q_SIZE (see 'OS OBJECT DEFINES')
*
*                (b) Message Queue
*                    (1) OS_CFG_TASK_Q_EN          Enabled
*
*                (c) Task Statistic                If IPERF_CFG_CPU_USAGE_MAX_CALC_EN is enabled
*                    (1) OS_CFG_TASK_PROFILE_EN    Enabled
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
#include  "../../Source/iperf.h"
#include  <os_cfg_app.h>
#include  <Source/os.h>
                                             /* See this 'iperf_os.c  Note #1'.                      */

/*
*********************************************************************************************************
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                /* See 'iperf_os.c  Note #1'.                           */
#if (OS_VERSION < 3010u)
    #error  "OS_VERSION [SHOULD be >= V3.01.0]"
#endif



                                                                /* See 'iperf_os.c  Note #2b'.                          */
#if (OS_CFG_TASK_Q_EN < 1u)
    #error  "OS_CFG_TASK_Q_EN illegally #define'd in 'os_cfg.h' [MUST be  > 0, (see 'iperf_os.c  Note #2b')]"
#endif


                                                                /* See 'iperf_os.c  Note #2c'.                          */
#if (IPERF_CFG_CPU_USAGE_MAX_CALC_EN == DEF_ENABLED)

  #if (OS_CFG_TASK_PROFILE_EN < 1u)
      #error  "OS_CFG_TASK_PROFILE_EN illegally #define'd in 'os_cfg.h'. MUST be >= 1u."
  #endif

  #if (OS_CFG_DBG_EN < 1)
      #error  "OS_CFG_DBG_EN illegally #define'd in 'os_cfg.h'. MUST be >= 1u."
  #endif

  #if (OS_CFG_STAT_TASK_EN < 1)
      #error "OS_CFG_STAT_TASK_EN illegally #define'd in 'os_cfg.h'. MUST be >= 1u."
  #endif
#endif



#ifndef      IPERF_OS_CFG_TASK_PRIO
    #error  "IPERF_OS_CFG_TASK_PRIO not #define'd in 'iperf_cfg.h' [MUST be >= 0u]"
#elif       (IPERF_OS_CFG_TASK_PRIO < 0u)
    #error  "IPERF_OS_CFG_TASK_PRIO illegally #define'd in 'iperf_cfg.h' [MUST be >= 0u]"
#endif



#ifndef      IPERF_CFG_Q_SIZE
    #error  "IPERF_CFG_Q_SIZE not #define'd in 'iperf_cfg.h' [MUST be > 0u]"

#elif       (IPERF_CFG_Q_SIZE < 1u)
    #error  "IPERF_CFG_Q_SIZE illegally #define'd in 'iperf_cfg.h' [MUST be > 0u]"

#elif      (IPERF_CFG_Q_SIZE > IPERF_CFG_MAX_NBR_TEST)
    #error  "IPERF_CFG_Q_SIZE illegally #define'd in 'iperf_cfg.h' [MUST be <= IPERF_CFG_MAX_NBR_TEST]"
#endif




#ifndef      IPERF_OS_CFG_TASK_STK_SIZE
    #error  "IPERF_OS_CFG_TASK_STK_SIZE not #define'd in 'iperf_cfg.h' [MUST be > 0u]"
#elif       (IPERF_OS_CFG_TASK_STK_SIZE < 1u)
    #error  "IPERF_OS_CFG_TASK_STK_SIZE illegally #define'd in 'iperf_cfg.h' [MUST be > 0u]"
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                     OS TASK/OBJECT NAME DEFINES
*********************************************************************************************************
*/

                                                                /* -------------------- TASK NAMES -------------------- */
#define  IPERF_OS_TASK_NAME                 "IPerf Task"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/


                                                                /* --------------------- TASK TCB --------------------- */
static  OS_TCB     IPerf_TaskTCB;


                                                                /* -------------------- TASK STACK -------------------- */
static  CPU_STK    IPERF_OS_TaskStk[IPERF_OS_CFG_TASK_STK_SIZE];


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                /* ---------- IPERF TASK MANAGEMENT FUNCTION ---------- */
static  void  IPerf_OS_Task (void  *p_data);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           IPERF FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           IPerf_OS_Init()
*
* Description : (1) Perform IPerf/OS initialisation:
*
*                   (a) Create IPerf Cmd Q.
*                   (b) Create IPerf task.
*
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function
*
*                           IPERF_OS_ERR_NONE               IPerf/OS initialization successful.
*                           IPERF_OS_ERR_INIT_TASK          IPerf    initialization task
*                                                               NOT successfully initialized.
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  IPerf_OS_Init (IPERF_ERR  *p_err)
{
    OS_ERR  os_err;
                                                                /* Create IPerf task.                                   */
    OSTaskCreate((OS_TCB     *)&IPerf_TaskTCB,
                 (CPU_CHAR   *) IPERF_OS_TASK_NAME,
                 (OS_TASK_PTR ) IPerf_OS_Task,
                 (void       *) 0u,
                 (OS_PRIO     ) IPERF_OS_CFG_TASK_PRIO,
                 (CPU_STK    *)&IPERF_OS_TaskStk[0],
                 (CPU_STK_SIZE)(IPERF_OS_CFG_TASK_STK_SIZE / 10u),
                 (CPU_STK_SIZE) IPERF_OS_CFG_TASK_STK_SIZE,
                 (OS_MSG_QTY  ) IPERF_CFG_Q_SIZE + 1u,
                 (OS_TICK     ) 0u,
                 (void       *) 0u,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&os_err);

    if (os_err !=  OS_ERR_NONE) {
       *p_err = IPERF_OS_ERR_INIT_TASK;
        return;
    }

   *p_err = IPERF_OS_ERR_NONE;
}


/*
*********************************************************************************************************
*                                           IPerf_OS_Task()
*
* Description : OS-dependent shell task to proceed IPerf test handler.
*
* Argument(s) : p_data      Pointer to task initialization data (required by uC/OS-III).
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_OS_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  IPerf_OS_Task (void  *p_data)
{
   (void)&p_data;                                               /* Prevent compiler warning.                            */

    while (DEF_ON) {
        IPerf_TestTaskHandler();
    }
}


/*
*********************************************************************************************************
*                                        IPerf_OS_TestQ_Wait()
*
* Description : Wait on IPerf test ID of test to start.
*
* Argument(s) : p_err       Pointer to variable that will receive the return error code from this function :
*
*                               IPERF_OS_ERR_Q_NONE     Message     received.
*                               IPERF_OS_ERR_Q          Message NOT received.
*
* Return(s)   : Test ID to start.
*
* Caller(s)   : IPerf_TestTaskHandler().
*
*               This function is an INTERNAL IPerf function & MUST NOT be called by application function(s).
*
* Note(s)     : (1) IPerf message from parser MUST be acquired --i.e. MUST wait for message; do NOT timeout.
*********************************************************************************************************
*/

IPERF_TEST_ID  IPerf_OS_TestQ_Wait (IPERF_ERR  *p_err)
{
    IPERF_TEST_ID   iperf_test_id;
    OS_ERR          os_err;
    OS_MSG_SIZE     os_msg_size;
    void           *p_msg;
    CPU_ADDR        msg_val;

                                                                /* Wait on IPerf task queue ...                         */
    p_msg = OSTaskQPend((OS_TICK      ) 0u,                     /* ... without timeout (see Note #1b).                  */
                        (OS_OPT       ) OS_OPT_PEND_BLOCKING,
                        (OS_MSG_SIZE *)&os_msg_size,
                        (CPU_TS      *) 0u,
                        (OS_ERR      *)&os_err);

    switch (os_err) {
        case OS_ERR_NONE:
             msg_val       = (CPU_ADDR)p_msg;
             iperf_test_id = (IPERF_TEST_ID)msg_val;            /* Decode interface of signaled receive test ID.        */
            *p_err         =  IPERF_OS_ERR_NONE;
             break;


        case OS_ERR_Q_EMPTY:
        case OS_ERR_TIMEOUT:
        case OS_ERR_PEND_ISR:
        case OS_ERR_PEND_ABORT:
        case OS_ERR_PEND_WOULD_BLOCK:
        case OS_ERR_SCHED_LOCKED:
        default:
             iperf_test_id = IPERF_TEST_ID_NONE;
            *p_err         = IPERF_OS_ERR_Q;
             break;
    }

    return (iperf_test_id);
}


/*
*********************************************************************************************************
*                                        IPerf_OS_TestQ_Post()
*
* Description : Post IPerf test ID of test to start.
*
* Argument(s) : test_id    Test ID of test to start.
*
*               p_err      Pointer to variable that will receive the return error code from this function
*
*                               IPERF_OS_ERR_NONE       Message     successfully posted.
*                               IPERF_OS_ERR_Q          Message NOT successfully posted.
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_Parse().
*
*               This function is an INTERNAL IPerf function & MUST NOT be called by application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  IPerf_OS_TestQ_Post (IPERF_TEST_ID   iperf_test_id,
                           IPERF_ERR      *p_err)
{
    OS_ERR    os_err;
    CPU_ADDR  iperf_test_id_msg;


    iperf_test_id_msg = (CPU_ADDR)iperf_test_id;

    OSTaskQPost((OS_TCB    *)&IPerf_TaskTCB,                    /* Post msg to msg queue.                               */
                (void      *) iperf_test_id_msg,
                (OS_MSG_SIZE) 0u,
                (OS_OPT     ) OS_OPT_POST_FIFO,
                (OS_ERR    *)&os_err);

    switch (os_err) {
        case OS_ERR_NONE:
            *p_err = IPERF_OS_ERR_NONE;
             break;


        case OS_ERR_Q_MAX:
        case OS_ERR_MSG_POOL_EMPTY:
        case OS_ERR_STATE_INVALID:
        default:
            *p_err = IPERF_OS_ERR_Q;
             break;
    }
}


/*
*********************************************************************************************************
*                                        IPerf_OS_CPU_Usage()
*
* Description : Get current CPU usage
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_ServerTCP(),
*               IPerf_ServerUDP(),
*               IPerf_ClientTCP(),
*               IPerf_ClientUDP().
*
*               This function is an INTERNAL IPerf function & MUST NOT be called by application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if (IPERF_CFG_CPU_USAGE_MAX_CALC_EN == DEF_ENABLED)
CPU_INT16U  IPerf_OS_CPU_Usage (void)
{
    CPU_INT16U  usage;


    usage = 10000u - OSIdleTaskTCB.CPUUsage;

    return (usage);
}
#endif

