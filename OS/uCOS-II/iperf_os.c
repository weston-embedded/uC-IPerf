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
*                                          Micrium uC/OS-II
*
* Filename : iperf_os.c
* Version  : V2.04.00
*********************************************************************************************************
* Note(s)  : (1) Assumes uC/OS-II V2.86 (or more recent version) is included in the project build.
*
*            (2) REQUIREs the following uC/OS-II features to be ENABLED :
*
*                    ------- FEATURE --------    --------- MINIMUM CONFIGURATION FOR IPERF/OS PORT ----------
*
*                (a) Message Queue
*                    (1) OS_Q_EN                     Enabled
*
*                (b) Task Statistic                  If IPERF_CFG_CPU_USAGE_MAX_CALC_EN is enabled
*                    (1) OS_TASK_STAT_EN             Enabled
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <Source/iperf.h>

#include  <ucos_ii.h>                                           /* See this 'iperf_os.h  Note #1'.                      */


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
#define  IPERF_OS_TASK_NAME                       "IPerf Task"

#define  IPERF_OS_TASK_NAME_SIZE_MAX                      11    /* Max of IPerf task name size.                         */


                                                                /* -------------------- OBJ NAMES --------------------- */
#define  IPERF_OS_Q_NAME                          "IPerf Cmd Q"

#define  IPERF_OS_Q_NAME_SIZE_MAX                         12    /* Max of Iperf Q name sizes.                           */


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                /* --------------------- TASK STK --------------------- */
static  OS_STK     IPERF_OS_TaskStk[IPERF_OS_CFG_TASK_STK_SIZE];

                                                                /* ---------------------- CMD Q ----------------------- */
static  OS_EVENT  *IPERF_OS_Q_Ptr;
static  void      *IPERF_OS_Q[IPERF_CFG_Q_SIZE];


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                /* ------------- IPERF MAIN TASK FUNCTION ------------- */
static  void  IPerf_OS_Task (void  *p_data);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                /* See 'iperf_os.c  Note #1'.                           */
#if     (OS_VERSION < 286)
#error  "OS_VERSION [SHOULD be >= V2.86]"
#endif



                                                                /* See 'iperf_os.c  Note #2a'.                          */
#if     (OS_Q_EN < 1u)
#error  "OS_Q_EN illegally #define'd in 'os_cfg.h' [MUST be  > 0, (see 'iperf_os.c  Note #2a1')]"
#endif



                                                                /* See 'iperf_os.c  Note #2b'.                          */
#if    ((IPERF_CFG_CPU_USAGE_MAX_CALC_EN == DEF_ENABLED) && \
        (OS_TASK_STAT_EN                  < 1))
#error  "OS_TASK_STAT_EN  illegally #define'd in 'os_cfg.h' [MUST be  > 0, (see 'iperf_os.c  Note #2b1')]"
#endif




#ifndef  IPERF_OS_CFG_TASK_PRIO
#error  "IPERF_OS_CFG_TASK_PRIO  not #define'd in 'app_cfg.h' [MUST be  >= 0u]"
#elif   (IPERF_OS_CFG_TASK_PRIO < 0u)
#error  "IPERF_OS_CFG_TASK_PRIO illegally #define'd in 'app_cfg.h' [MUST be  >= 0u]"
#endif




#ifndef  IPERF_CFG_Q_SIZE
#error  "IPERF_CFG_Q_SIZE not #define'd in 'app_cfg.h' [MUST be  > 0u]"
#elif   (IPERF_CFG_Q_SIZE < 1u)
#error  "IPERF_CFG_Q_SIZE illegally #define'd in 'app_cfg.h' [MUST be  > 0u]"
#endif




#ifndef  IPERF_OS_CFG_TASK_STK_SIZE
#error  "IPERF_OS_CFG_TASK_STK_SIZE not #define'd in 'app_cfg.h' [MUST be  > 0u]"
#elif   (IPERF_OS_CFG_TASK_STK_SIZE < 1u)
#error  "IPERF_OS_CFG_TASK_STK_SIZE illegally #define'd in 'app_cfg.h' [MUST be  > 0u]"
#endif


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
*                   (b) Set    IPerf Cms Q name.
*                   (c) Create IPerf task.
*                   (d) Set    IPerf task name.
*
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function
*
*                           IPERF_OS_ERR_NONE               IPerf/OS initialization successful.
*
*                           IPERF_OS_ERR_INIT_Q             IPerf    initialization Q
*                                                               NOT successfully initialized.
*
*                           IPERF_OS_ERR_INIT_TASK          IPerf    initialization task
*                                                               NOT successfully initialized.
*
* Return(s)   :  none.
*
* Caller(s)   :  IPerf_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  IPerf_OS_Init (IPERF_ERR  *p_err)
{
    INT8U         os_err;

                                                                /* ---------- INITIALIZE/CREATE IPERF QUEUE ----------- */
    IPERF_OS_Q_Ptr = OSQCreate(&IPERF_OS_Q[0],
                                IPERF_CFG_Q_SIZE + 1);

    if (IPERF_OS_Q_Ptr == (OS_EVENT *)0) {
       *p_err = IPERF_OS_ERR_INIT_Q;
        return;
    }

#if (((OS_VERSION >= 288) && (OS_EVENT_NAME_EN   >  0)) || \
     ((OS_VERSION <  288) && (OS_EVENT_NAME_SIZE >= IPERF_OS_Q_NAME_SIZE_MAX)))
    OSEventNameSet((OS_EVENT *) IPERF_OS_Q_Ptr,
                   (INT8U    *) IPERF_OS_Q_NAME,
                   (INT8U    *)&os_err);
    if (os_err !=  OS_ERR_NONE) {
       *p_err = IPERF_OS_ERR_INIT_Q;
        return;
    }
#endif


                                                                /* Create IPerf task.                                   */
#if (OS_TASK_CREATE_EXT_EN == 1)

#if (OS_STK_GROWTH == 1)
    os_err = OSTaskCreateExt((void (*)(void *)) IPerf_OS_Task,
                             (void          * ) 0,
                             (OS_STK        * )&IPERF_OS_TaskStk[IPERF_OS_CFG_TASK_STK_SIZE - 1],
                             (INT8U           ) IPERF_OS_CFG_TASK_PRIO,
                             (INT16U          ) IPERF_OS_CFG_TASK_PRIO,
                             (OS_STK        * )&IPERF_OS_TaskStk[0],
                             (INT32U          ) IPERF_OS_CFG_TASK_STK_SIZE,
                             (void          * ) 0,
                             (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
#else
    os_err = OSTaskCreateExt((void (*)(void *)) IPerf_OS_Task,
                             (void          * ) 0,
                             (OS_STK        * )&IPERF_OS_TaskStk[0],
                             (INT8U           ) IPERF_OS_CFG_TASK_PRIO,
                             (INT16U          ) IPERF_OS_CFG_TASK_PRIO,
                             (OS_STK        * )&IPERF_OS_TaskStk[IPERF_OS_CFG_TASK_STK_SIZE - 1],
                             (INT32U          ) IPERF_OS_CFG_TASK_STK_SIZE,
                             (void          * ) 0,
                             (INT16U          )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
#endif

#else

#if (OS_STK_GROWTH == 1)
    os_err = OSTaskCreate((void (*)(void *)) IPerf_OS_Task,
                          (void          * ) 0,
                          (OS_STK        * )&IPERF_OS_TaskStk[IPERF_OS_CFG_TASK_STK_SIZE - 1],
                          (INT8U           ) IPERF_OS_CFG_TASK_PRIO);
#else
    os_err = OSTaskCreate((void (*)(void *)) IPerf_OS_Task,
                          (void          * ) 0,
                          (OS_STK        * )&IPERF_OS_TaskStk[0],
                          (INT8U           ) IPERF_OS_CFG_TASK_PRIO);
#endif

#endif

    if (os_err !=  OS_ERR_NONE) {
       *p_err = IPERF_OS_ERR_INIT_TASK;
        return;
    }


#if (((OS_VERSION >= 288) && (OS_TASK_NAME_EN   >  0)) || \
     ((OS_VERSION <  288) && (OS_TASK_NAME_SIZE >= IPERF_OS_TASK_NAME_SIZE_MAX)))
    OSTaskNameSet((INT8U  ) IPERF_OS_CFG_TASK_PRIO,
                  (INT8U *) IPERF_OS_TASK_NAME,
                  (INT8U *)&os_err);
    if (os_err !=  OS_ERR_NONE) {
       *p_err = IPERF_OS_ERR_INIT_TASK;
        return;
    }
#endif

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
*                               IPERF_OS_ERR_Q_NONE             Message     received.
*                               IPERF_OS_ERR_MSG_Q              Message NOT received.
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
    void           *p_q;
    CPU_ADDR        msg_val;
    IPERF_TEST_ID   test_id;
    INT8U           os_err;


    p_q = OSQPend((OS_EVENT *) IPERF_OS_Q_Ptr,                  /* Wait on IPerf receive test Q.                        */
                  (INT16U    ) 0,                               /* Preferably without timeout (see Note #1).            */
                  (INT8U    *)&os_err);
    switch (os_err) {
        case OS_ERR_NONE:
             msg_val = (CPU_ADDR     )p_q;
             test_id = (IPERF_TEST_ID)msg_val;                  /* Decode interface of signaled receive test ID.        */
            *p_err   =  IPERF_OS_ERR_NONE;
             break;

        case OS_ERR_TIMEOUT:
        case OS_ERR_PEVENT_NULL:
        case OS_ERR_EVENT_TYPE:
        case OS_ERR_PEND_ISR:
        case OS_ERR_PEND_LOCKED:
        case OS_ERR_PEND_ABORT:
        default:
             test_id = IPERF_TEST_ID_NONE;
            *p_err   = IPERF_OS_ERR_Q;
             break;
    }

    return (test_id);
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
*                               IPERF_OS_ERR_NONE               Message     successfully posted.
*                               IPERF_OS_ERR_MSG_Q              Message NOT successfully posted.
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
    INT8U     os_err;
    CPU_ADDR  iperf_test_id_msg;


    iperf_test_id_msg = (CPU_ADDR)iperf_test_id;

    os_err            =  OSQPost((OS_EVENT *)IPERF_OS_Q_Ptr,          /* Signal IPerf receive test task with test_id.         */
                                 (void     *)iperf_test_id_msg);
    switch (os_err) {
        case OS_ERR_NONE:
            *p_err = IPERF_OS_ERR_NONE;
             break;


        case OS_ERR_Q_FULL:
        case OS_ERR_PEVENT_NULL:
        case OS_ERR_EVENT_TYPE:
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
    return (OSCPUUsage);
}
#endif

