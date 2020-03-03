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
* Filename : iperf.c
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
#define    IPERF_MODULE
#include  "iperf.h"


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

                                                                /* --------------- IPERF PARSING FNCTS ---------------- */
static  CPU_INT16U   IPerf_ArgScan   (CPU_CHAR         *p_in,
                                      CPU_CHAR         *p_arg_tbl[],
                                      CPU_INT16U        arg_tbl_size,
                                      IPERF_ERR        *p_err);

static  void         IPerf_ArgParse  (CPU_INT16U        argc,
                                      CPU_CHAR         *p_argv[],
                                      IPERF_OPT        *p_opt,
                                      IPERF_OUT_FNCT    p_out_fnct,
                                      IPERF_OUT_PARAM  *p_out_param,
                                      IPERF_ERR        *p_err);

static  void         IPerf_ArgFmtGet (CPU_CHAR        *p_str_int_arg,
                                      IPERF_FMT       *p_fmt,
                                      IPERF_ERR       *p_err);


                                                                /* ---------------- IPERF PRINT FNCTS ----------------- */
static  void         IPerf_PrintErr  (IPERF_OUT_FNCT    p_out_fnct,
                                      IPERF_OUT_PARAM  *p_out_param,
                                      IPERF_ERR         err);

static  void         IPerf_PrintVer  (IPERF_OUT_FNCT    p_out_fnct,
                                      IPERF_OUT_PARAM  *p_out_param);

static  void         IPerf_PrintMenu (IPERF_OUT_FNCT    p_out_fnct,
                                      IPERF_OUT_PARAM  *p_out_param);


                                                                /* ----------------- IPERF TEST FNCTS ----------------- */
static  IPERF_TEST  *IPerf_TestSrch  (IPERF_TEST_ID    test_id);

static  IPERF_TEST  *IPerf_TestAdd   (IPERF_TEST_ID    test_id,
                                      IPERF_ERR       *p_err);

static  void         IPerf_TestRemove(IPERF_TEST      *p_test);

static  void         IPerf_TestInsert(IPERF_TEST      *p_test);

static  void         IPerf_TestUnlink(IPERF_TEST      *p_test);

static  IPERF_TEST  *IPerf_TestGet   (IPERF_ERR       *p_err);

static  void         IPerf_TestFree  (IPERF_TEST      *p_test);

static  void         IPerf_TestClr   (IPERF_TEST      *p_test);


/*
*********************************************************************************************************
*                                            IPerf_Init()
*
* Description : (1) Initialize & start IPerf application :
*
*                   (a) Initialize IPerf test pool
*                   (b) Initialize Iperf test table
*                   (c) Initialize IPerf Test List pointer
*                   (d) Initialize IPerf Next Test ID
*                   (e) Initialize IPerf CPU timestamp's timer frequency
*                   (e) IPerf/operating system initialization
*
*
* Argument(s) : p_err       Pointer to variable that will receive the return error code from this function :
*
*                               IPERF_ERR_NONE                  IPerf initialization successful.
*                               IPERF_ERR_CPU_TS_FREQ           IPerf initialization timestamp
*                                                                   NOT successfully initialized.
*                               IPERF_OS_ERR_INIT_TASK          IPerf initialization task
*                                                                   NOT successfully initialized.
*                               IPERF_OS_ERR_INIT_Q             IPerf initialization Q
*                                                                   NOT successfully initialized.
*
* Return(s)   : none.
*
* Caller(s)   : Your Product's Application.
*
*               This function is an IPerf initialization function & MAY be called by
*               application/initialization function(s).
*
* Note(s)     : (2) IPerf test pool MUST be initialized PRIOR to initializing the pool with pointers
*                   to IPerf test.
*********************************************************************************************************
*/

void  IPerf_Init (IPERF_ERR  *p_err)
{
    CPU_INT16U   i;
    IPERF_TEST  *p_test;
    CPU_ERR      err;


    IPERF_TRACE_DBG(("\n\n\n\r------------- IPERF TRACE DEBUG -------------"));
    IPERF_TRACE_DBG(("\n\ruC/IPerf initialization start ...\n\r"));
    IPERF_TRACE_DBG(("Initialize uC/IPerf OS ... "));


                                                                /* -------------- INIT IPERF TESTS POOL --------------- */
    IPerf_TestPoolPtr = (IPERF_TEST *)0;                        /* Init-clr test pool (see Note #2).                    */


                                                                /* --------------- INIT IPERF TEST TBL ---------------- */
    p_test = &IPerf_TestTbl[0];
    for (i = 0u; i < IPERF_CFG_MAX_NBR_TEST; i++) {
        p_test->Status = IPERF_TEST_STATUS_FREE;                /* Init each IPerf test as free/NOT used.               */

#if (NET_DBG_CFG_MEM_CLR_EN == DEF_ENABLED)
        IPerf_TestClr(p_test);
#endif
                                                                /* Free each IPerf test to pool (see Note #2).          */
        p_test->NextPtr   = IPerf_TestPoolPtr;
        IPerf_TestPoolPtr = p_test;

        p_test++;
    }


    IPerf_TestListHeadPtr = (IPERF_TEST *)0;                    /* ------------- INIT IPERF TEST LIST PTR ------------- */


    IPerf_NextTestID      =  IPERF_TEST_ID_INIT;                /* ------------- INIT IPERF NEXT TEST ID -------------- */


                                                                /* ------------ INIT IPERF CPU TS TMR FREQ ------------ */
    IPerf_CPU_TmrFreq  =  CPU_TS_TmrFreqGet(&err);

    if (err  != CPU_ERR_NONE) {
       *p_err = IPERF_ERR_CPU_TS_FREQ;
        return;
    }

                                                                /* ------------------ IPERF/OS INIT ------------------- */
    IPerf_OS_Init(p_err);                                       /* Create IPerf obj(s).                                 */

    if (*p_err == IPERF_OS_ERR_NONE) {
       *p_err = IPERF_ERR_NONE;
        IPERF_TRACE_DBG(("done.\n\n\n\r"));
    } else {
        IPERF_TRACE_DBG(("fail.\n\r"));
    }
}


/*
*********************************************************************************************************
*                                       IPerf_TestTaskHandler()
*
* Description : (1) Handle IPerf tests in the test queue:
*
*                  (a) Wait for queued tests
*                  (b) Search IPerf Test List for test with test id
*                  (c) Start server or client test
*
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_OS_Task().
*
*               This function is an IPerf to operating system (OS) function & SHOULD be called only
*               by appropriate IPerf-operating system port function(s).
*
* Note(s)     : (2) IPerf_Tbl[test_id] validated in IPerf_Parse().
*********************************************************************************************************
*/

void  IPerf_TestTaskHandler (void)
{
    IPERF_TEST_ID   test_id;
    IPERF_TEST     *p_test;
    IPERF_OPT      *p_opt;
    IPERF_ERR       err;


    while (DEF_ON) {
                                                                /* --------------- WAIT FOR IPERF TEST ---------------- */
        do {
            test_id = IPerf_OS_TestQ_Wait(&err);
        } while (err != IPERF_OS_ERR_NONE);


                                                                /* --------------- SRCH IPERF TEST LIST --------------- */
        p_test = IPerf_TestSrch(test_id);
        if (p_test == (IPERF_TEST *)0) {                        /* If test NOT found, ...                               */
            p_test->Err = IPERF_ERR_TEST_NOT_FOUND;             /* ... rtn err.                                         */
            IPERF_TRACE_DBG(("IPerf Error: IPerf test ID not found.\n"));
            continue;
        }

        p_opt       = &p_test->Opt;
        p_test->Err = IPERF_ERR_NONE;

                                                                /* -------------- START SERVER OR CLIENT -------------- */
        switch (p_opt->Mode) {
#ifdef  IPERF_SERVER_MODULE_PRESENT
            case IPERF_MODE_SERVER:
                 p_test->Status = IPERF_TEST_STATUS_RUNNING;
                 IPerf_ServerStart(p_test, &err);
                 if (err == IPERF_ERR_NONE) {
                    p_test->Status = IPERF_TEST_STATUS_DONE;
                 } else {
                    p_test->Status = IPERF_TEST_STATUS_ERR;
                 }
                 break;
#endif


#ifdef  IPERF_CLIENT_MODULE_PRESENT
            case IPERF_MODE_CLIENT:
                 p_test->Status = IPERF_TEST_STATUS_RUNNING;
                 p_test->Err    = IPERF_ERR_NONE;
                 IPerf_ClientStart(p_test, &err);
                 if (err == IPERF_ERR_NONE) {
                    p_test->Status = IPERF_TEST_STATUS_DONE;
                 } else {
                    p_test->Status = IPERF_TEST_STATUS_ERR;
                    p_test->Err    = err;
                 }
                 break;
#endif

            default:
                 p_test->Status = IPERF_TEST_STATUS_FREE;
                 break;
        }
    }
}


/*
*********************************************************************************************************
*                                          IPerf_TestStart()
*
* Description : (1) Validate & schedule a new IPerf test :
*
*                  (a) Validate arguments values pointer
*                  (b) Add new test into IPerf Test List
*                  (c) Scan     & parse command line
*                  (d) Validate & parse arguments
*                  (e) Queuing test
*                  (f) Validate next test ID
*
*
* Argument(s) : argv            Pointer to the string arguments values.
*
*               p_out_fnct      Pointer to string output function.
*
*               p_out_param     Pointer to        output function parameters.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   IPERF_ERR_NONE                         Test successfully queued.
*                                   IPERF_ERR_ARG_INVALID_PTR              String arguments values null pointer.
*
*                                                                          --- RETURNED BY IPerf_TestAdd() : ----
*                                   IPERF_ERR_TEST_NONE_AVAIL              NO available host group to allocate.
*
*                                                                          --- RETURNED BY IPerf_ArgScan() : ----
*                                   IPERF_ERR_ARG_TBL_FULL                 Argument table full.
*
*                                                                          --- RETURNED BY IPerf_ArgParse() :----.
*                                   IPERF_ERR_ARG_NO_ARG                   Arguments         NOT specified.
*                                   IPERF_ERR_ARG_NO_VAL                   Values            NOT specified.
*                                   IPERF_ERR_ARG_INVALID_OPT              Arguments         NOT valided.
*                                   IPERF_ERR_ARG_INVALID_VAL              Values            NOT valided.
*                                   IPERF_ERR_ARG_INVALID_MODE             Mode              NOT enabled.
*                                   IPERF_ERR_ARG_INVALID_REMOTE_ADDR      Remote addresse   NOT valided.
*                                   IPERF_ERR_ARG_EXCEED_MAX_LEN           Buffer length     NOT valided.
*                                   IPERF_ERR_ARG_EXCEED_MAX_UDP_LEN       UDP buffer length NOT valided.
*                                   IPERF_ERR_ARG_PARAM_NOT_SUPPORTED_OPT  Option            NOT supported.
*                                   IPERF_ERR_ARG_PARAM_NOT_SUPPORTED_VAL  Option value      NOT supported.
*
*                                                                          - RETURNED BY IPerf_OS_TestQ_Post() :-
*                                   IPERF_OS_ERR_MSG_Q                     Message NOT successfully posted.
*
* Return(s)   : Test ID,            if no error.
*
*               IPERF_TEST_ID_NONE, otherwise.
*
* Caller(s)   : Your Product's Application.
*
*               This function is an IPerf application interface (API) function & MAY be called by
*               application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

IPERF_TEST_ID  IPerf_TestStart (CPU_CHAR         *argv,
                                IPERF_OUT_FNCT    p_out_fnct,
                                IPERF_OUT_PARAM  *p_out_param,
                                IPERF_ERR        *p_err)
{
    IPERF_OPT   *p_opt;
    IPERF_TEST  *p_test;
    CPU_INT16U   argc;
    CPU_CHAR    *arg_tbl[IPERF_CMD_ARG_NBR_MAX + 1u];


                                                                /* ---------------- VALIDATE ARGV PTR ----------------- */
    if (argv == (CPU_CHAR *)0) {
       *p_err = IPERF_ERR_ARG_INVALID_PTR;
        return (IPERF_TEST_ID_NONE);
    }


                                                                /* -------- ADD NEW TEST INTO IPERF TEST LIST --------- */
    p_test = IPerf_TestAdd(IPerf_NextTestID, p_err);
    if (p_test == (IPERF_TEST *)0) {
        return (IPERF_TEST_ID_NONE);                            /* Rtn err from IPerf_TestAdd().                        */
    }

                                                                /* -------------- SCAN & PARSE CMD LINE --------------- */
    argc = IPerf_ArgScan((CPU_CHAR  *)argv,
                         (CPU_CHAR **)&arg_tbl[0],
                         (CPU_INT16U ) IPERF_CMD_ARG_NBR_MAX,
                         (IPERF_ERR *) p_err);


                                                                /* -------------- VALIDATE & PARSE ARGS --------------- */
    p_opt    = &p_test->Opt;
    IPerf_ArgParse((CPU_INT16U    ) argc,
                   (CPU_CHAR    **)&arg_tbl[0],
                   (IPERF_OPT    *) p_opt,
                   (IPERF_OUT_FNCT) p_out_fnct,
                   (void         *) p_out_param,
                   (IPERF_ERR    *) p_err);
    if (*p_err != IPERF_ERR_NONE) {
        IPerf_PrintErr(p_out_fnct, p_out_param, *p_err);
        IPerf_TestRemove(p_test);
        return (IPERF_TEST_ID_NONE);
    }


                                                                /* ---------------------- Q TEST ---------------------- */
    p_test->Status = IPERF_TEST_STATUS_QUEUED;
    IPerf_OS_TestQ_Post(p_test->TestID, p_err);
    switch (*p_err) {
        case IPERF_OS_ERR_NONE:                                 /* Test is now q`d.                                     */
            *p_err = IPERF_ERR_NONE;
             break;


        case IPERF_OS_ERR_Q:
        default:
             p_test->Status = IPERF_TEST_STATUS_ERR;
             IPERF_TRACE_DBG(("IPerf_Q error: %u.\n", (unsigned int)p_err));
             IPerf_TestRemove(p_test);
             return (IPERF_TEST_ID_NONE);
    }


                                                                /* ----------- VALIDATE IPERF NEXT TEST ID ------------ */
    IPerf_NextTestID++;
    if (IPerf_NextTestID == DEF_INT_16U_MAX_VAL) {
        IPerf_NextTestID  = IPERF_TEST_ID_INIT;
    }


    return (p_test->TestID);
}


/*
*********************************************************************************************************
*                                        IPerf_TestShellStart()
*
* Description : (1) Validate & schedule a new IPerf test :
*
*                  (a) Validate arguments values pointer
*                  (b) Add new test into IPerf Test List
*                  (c) Scan     & parse command line
*                  (d) Validate & parse arguments
*                  (e) Queuing test
*                  (f) Validate next test ID
*
*
* Argument(s) : argc            Number of arguments.
*
*               p_argv          Pointer to the string arguments values.
*
*               p_out_fnct      Pointer to string output function.
*
*               p_out_param     Pointer to        output function parameters.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   IPERF_ERR_NONE                         Test successfully queued.
*                                   IPERF_ERR_ARG_INVALID_PTR              String arguments values null pointer.
*
*                                                                          --- RETURNED BY IPerf_TestAdd() : ----
*                                   IPERF_ERR_TEST_NONE_AVAIL              NO available host group to allocate.
*
*                                                                          --- RETURNED BY IPerf_ArgScan() : ----
*                                   IPERF_ERR_ARG_TBL_FULL                 Argument table full.
*
*                                                                          --- RETURNED BY IPerf_ArgParse() :----.
*                                   IPERF_ERR_ARG_NO_ARG                   Arguments         NOT specified.
*                                   IPERF_ERR_ARG_NO_VAL                   Values            NOT specified.
*                                   IPERF_ERR_ARG_INVALID_OPT              Arguments         NOT valided.
*                                   IPERF_ERR_ARG_INVALID_VAL              Values            NOT valided.
*                                   IPERF_ERR_ARG_INVALID_MODE             Mode              NOT enabled.
*                                   IPERF_ERR_ARG_INVALID_REMOTE_ADDR      Remote addresse   NOT valided.
*                                   IPERF_ERR_ARG_EXCEED_MAX_LEN           Buffer length     NOT valided.
*                                   IPERF_ERR_ARG_EXCEED_MAX_UDP_LEN       UDP buffer length NOT valided.
*                                   IPERF_ERR_ARG_PARAM_NOT_SUPPORTED_OPT  Option            NOT supported.
*                                   IPERF_ERR_ARG_PARAM_NOT_SUPPORTED_VAL  Option value      NOT supported.
*
*                                                                          - RETURNED BY IPerf_OS_TestQ_Post() :-
*                                   IPERF_OS_ERR_MSG_Q                     Message NOT successfully posted.
*
* Return(s)   : Test ID,            if no error.
*
*               IPERF_TEST_ID_NONE, otherwise.
*
* Caller(s)   : Your Product's Application.
*
*               This function is an IPerf application interface (API) function & MAY be called by
*               application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

IPERF_TEST_ID  IPerf_TestShellStart (CPU_INT16U        argc,
                                     CPU_CHAR         *p_argv[],
                                     IPERF_OUT_FNCT    p_out_fnct,
                                     IPERF_OUT_PARAM  *p_out_param,
                                     IPERF_ERR        *p_err)
{
    IPERF_OPT   *p_opt;
    IPERF_TEST  *p_test;


                                                                /* -------- ADD NEW TEST INTO IPERF TEST LIST --------- */
    p_test = IPerf_TestAdd(IPerf_NextTestID, p_err);
    if (p_test == (IPERF_TEST *)0) {
        return (IPERF_TEST_ID_NONE);                            /* Rtn err from IPerf_TestAdd().                        */
    }


                                                                /* -------------- VALIDATE & PARSE ARGS --------------- */
    p_opt = &p_test->Opt;
    IPerf_ArgParse((CPU_INT16U       ) argc,
                   (CPU_CHAR       **)&p_argv[0],
                   (IPERF_OPT       *) p_opt,
                   (IPERF_OUT_FNCT   ) p_out_fnct,
                   (IPERF_OUT_PARAM *) p_out_param,
                   (IPERF_ERR       *) p_err);
    if (*p_err != IPERF_ERR_NONE) {
        IPerf_PrintErr(p_out_fnct, p_out_param, *p_err);
        IPerf_TestRemove(p_test);
        return (IPERF_TEST_ID_NONE);
    }


                                                                /* ---------------------- Q TEST ---------------------- */
    p_test->Status = IPERF_TEST_STATUS_QUEUED;
    IPerf_OS_TestQ_Post(p_test->TestID, p_err);
    switch (*p_err) {
        case IPERF_OS_ERR_NONE:                                 /* Test is now q`d.                                     */
            *p_err = IPERF_ERR_NONE;
             break;


        case IPERF_OS_ERR_Q:
        default:
             p_test->Status = IPERF_TEST_STATUS_ERR;
             IPERF_TRACE_DBG(("IPerf_Q error: %u.\n", (unsigned int)p_err));
             IPerf_TestRemove(p_test);
             return (IPERF_TEST_ID_NONE);
    }


                                                                /* ----------- VALIDATE IPERF NEXT TEST ID ------------ */
    IPerf_NextTestID++;
    if (IPerf_NextTestID == DEF_INT_16U_MAX_VAL) {
        IPerf_NextTestID  = IPERF_TEST_ID_INIT;
    }


    return (p_test->TestID);
}


/*
*********************************************************************************************************
*                                         IPerf_TestRelease()
*
* Description : (1) Remove test of ring array holding:
*
*                   (a) Search IPerf Test List for test with test id
*                   (b) If test not running, remove test of IPerf Test List
*
*
* Argument(s) : test_id     Test ID of the test to release.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               IPERF_ERR_NONE                Test removed
*                               IPERF_ERR_TEST_INVALID_ID     Invalid test ID.
*                               IPERF_ERR_TEST_RUNNING        Test running and can't be released.
*
* Return(s)   : none.
*
* Caller(s)   : Your Product's Application.
*
*               This function is a IPerf application interface (API) function & MAY be called by
*               application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  IPerf_TestRelease (IPERF_TEST_ID   test_id,
                         IPERF_ERR      *p_err)
{
    IPERF_TEST  *p_test;


                                                                /* --------------- SRCH IPERF TEST LIST --------------- */
    p_test = IPerf_TestSrch(test_id);
    if (p_test == (IPERF_TEST *)0) {                            /* If test NOT found, ...                               */
       *p_err = IPERF_ERR_TEST_INVALID_ID;                      /* ... rtn err.                                         */
        IPERF_TRACE_DBG(("IPerf Get Status Error: IPerf test ID not found.\n"));
        return;
    }


                                                                /* ---------- REMOVE TEST OF IPERF TEST LIST ---------- */
    switch (p_test->Status) {
        case IPERF_TEST_STATUS_RUNNING:                         /* If test currently running, ...                       */
            *p_err = IPERF_ERR_TEST_RUNNING;                    /* ... rtn err.                                         */
             return;


        case IPERF_TEST_STATUS_FREE:
             break;


        case IPERF_TEST_STATUS_QUEUED:
        case IPERF_TEST_STATUS_DONE:
        case IPERF_TEST_STATUS_ERR:
        default:
             IPerf_TestRemove(p_test);
             break;
    }

   *p_err = IPERF_ERR_NONE;
}


/*
*********************************************************************************************************
*                                        IPerf_GetTestStatus()
*
* Description : (1) Get test status:
*
*                   (a) Search IPerf Test List for test with test id
*                   (b) If found, return test status
*
*
* Argument(s) : test_id     Test ID of the test to get status.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               IPERF_ERR_NONE                    Valid status.
*                               IPERF_ERR_TEST_INVALID_ID       Invalid test ID.
*
* Return(s)   : Status of specified test.
*
* Caller(s)   : Your Product's Application.
*
*               This function is a IPerf application interface (API) function & MAY be called by
*               application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

IPERF_TEST_STATUS  IPerf_TestGetStatus (IPERF_TEST_ID   test_id,
                                        IPERF_ERR      *p_err)
{
    IPERF_TEST         *p_test;
    IPERF_TEST_STATUS   status;


                                                                /* --------------- SRCH IPERF TEST LIST --------------- */
    p_test = IPerf_TestSrch(test_id);
    if (p_test == (IPERF_TEST *)0) {                            /* If test NOT found, ...                               */
       *p_err = IPERF_ERR_TEST_INVALID_ID;                      /* ... rtn err.                                         */
        IPERF_TRACE_DBG(("IPerf Get Status Error: IPerf test ID not found.\n"));
        return (IPERF_TEST_STATUS_ERR);
    }

                                                                /* ----------------- RTN TEST STATUS ------------------ */
    status = p_test->Status;

   *p_err  = IPERF_ERR_NONE;

    return (status);
}


/*
*********************************************************************************************************
*                                       IPerf_GetTestResults()
*
* Description : (1) Get test result:
*
*                   (a) Search IPerf Test List for test with test id
*                   (b) If found, return copy test result
*
*
* Argument(s) : test_id         Test ID of the test to get result.
*
*               p_test_result   Pointer to structure that will receive result of specified test.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   IPERF_ERR_NONE                    Valid test result.
*                                   IPERF_ERR_TEST_INVALID_ID       Invalid test ID.
*                                   IPERF_ERR_TEST_INVALID_RESULT   Invalid test result.
*
* Return(s)   : none.
*
* Caller(s)   : Your Product's Application.
*
*               This function is a IPerf application interface (API) function & MAY be called by
*               application function(s).
*
* Note(s)     : (2) Test results can be obtains before, after, or even during a test run.
*********************************************************************************************************
*/

void  IPerf_TestGetResults (IPERF_TEST_ID   test_id,
                            IPERF_TEST     *p_test_result,
                            IPERF_ERR      *p_err)
{
    IPERF_TEST         *p_test;
    IPERF_TEST_STATUS   status;


                                                                /* --------------- SRCH IPERF TEST LIST --------------- */
    p_test = IPerf_TestSrch(test_id);
    if (p_test == (IPERF_TEST *)0) {                            /* If test NOT found, ...                               */
       *p_err = IPERF_ERR_TEST_INVALID_ID;                      /* ... rtn err.                                         */
        IPERF_TRACE_DBG(("IPerf Get Status Error: IPerf test ID not found.\n"));
        return;
    }


                                                                /* ---- COPY INTERNAL IPERF TEST TO APP IPERF TEST ---- */
    status = p_test->Status;
    switch (status) {
        case IPERF_TEST_STATUS_QUEUED:
        case IPERF_TEST_STATUS_RUNNING:
        case IPERF_TEST_STATUS_DONE:
        case IPERF_TEST_STATUS_ERR:
            *p_err         =  IPERF_ERR_NONE;
            *p_test_result = *p_test;
             break;


        case IPERF_TEST_STATUS_FREE:
        default:
            *p_err = IPERF_ERR_TEST_INVALID_RESULT;
             break;
    }
}


/*
*********************************************************************************************************
*                                        IPerf_TestClrStats()
*
* Description : Clear test statistics.
*
* Argument(s) : p_test      Pointer to an IPerf test statistics.
*               ------      Argument validated in IPerf_Init()
*                                    checked   in IPerf_TestClr(),
*                                                   by IPerf_TestAdd();
*                                              in IPerf_ServerRxPkt()
*                                                   by IPerf_TestTaskHandler().
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_TestClr(),
*               IPerf_ServerRxPkt().
*
*               This function is an INTERNAL IPerf function & SHOULD NOT be called by application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  IPerf_TestClrStats (IPERF_STATS  *p_stats)
{
    p_stats->TS_Start_ms       =  0u;
    p_stats->TS_End_ms         =  0u;

    p_stats->NbrCalls          =  0u;

    p_stats->Errs              =  0u;
    p_stats->Bytes             =  0u;
    p_stats->TransitoryErrCnts =  0u;

    p_stats->UDP_RxLastPkt     = -1;
    p_stats->UDP_LostPkt       =  0u;
    p_stats->UDP_OutOfOrder    =  0u;
    p_stats->UDP_DupPkt        =  0u;
    p_stats->UDP_AsyncErr      =  DEF_NO;
    p_stats->UDP_EndErr        =  DEF_NO;

#if (IPERF_CFG_BANDWIDTH_CALC_EN == DEF_ENABLED)
    p_stats->Bandwidth         =  0u;
#endif
#if (IPERF_CFG_CPU_USAGE_MAX_CALC_EN == DEF_ENABLED)
    p_stats->CPU_UsageMax      =  0u;
    p_stats->CPU_UsageAvg      =  0u;
    p_stats->CPU_CalcNbr       =  0u;
#endif

}


/*
*********************************************************************************************************
*                                          IPerf_Get_TS_ms()
*
* Description : Get current CPU timestamp in millisecond.
*
* Argument(s) : none.
*
* Return(s)   : Current CPU timestamp in millisecond.
*
* Caller(s)   : IPerf_Server_TCP(),
*               IPerf_Server_UDP(),
*               IPerf_Client_TCP(),
*               IPerf_Client_UDP(),
*               Your Product's Application.
*
*               This function is a IPerf application interface (API) function & MAY be called by
*               application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

IPERF_TS_MS  IPerf_Get_TS_ms (void)
{
    CPU_INT64U   ts;
    IPERF_TS_MS  ts_ms;


    ts    = (CPU_INT64U )CPU_TS_Get32();
    ts_ms = (IPERF_TS_MS)((ts * DEF_TIME_NBR_mS_PER_SEC) / IPerf_CPU_TmrFreq);

    return (ts_ms);
}


/*
*********************************************************************************************************
*                                        IPerf_Get_TS_Max_ms()
*
* Description : Get maximum of millisecond supported by CPU timestamp functionality.
*
* Argument(s) : none.
*
* Return(s)   : Maximum CPU timestamp in millisecond.
*
* Caller(s)   : IPerf_UpdateBandwidth(),
*               Your Product's Application.
*
*               This function is a IPerf application interface (API) function & MAY be called by
*               application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

IPERF_TS_MS  IPerf_Get_TS_Max_ms (void)
{
    CPU_TS_TMR_FREQ  tmr_freq;
    CPU_INT32U       ts_max;


    tmr_freq = IPerf_CPU_TmrFreq   / DEF_TIME_NBR_mS_PER_SEC;
    ts_max   = IPERF_TS_MS_MAX_VAL / tmr_freq;

    return ((IPERF_TS_MS)ts_max);
}


/*
*********************************************************************************************************
*                                         IPerf_GetDataFmtd()
*
* Description : Convert from bytes to specified unit.
*
*
* Argument(s) : fmt             Format character.
*
*               bytes_qty       Bytes quantity to convert.
*
* Return(s)   : Bytes quantity formatted.
*
* Caller(s)   : IPerf_UpdateBandwidth() &
*               Your Product's Application.
*
*               This function is a IPerf application interface (API) function & MAY be called by
*               application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT32U  IPerf_GetDataFmtd (IPERF_FMT   fmt,
                               CPU_INT32U  bytes_qty)
{
    CPU_INT32U  data_fmt;


                                                                /* ------------ DECODE FMT & CONVERT CALC ------------- */
    switch (fmt) {
            case IPERF_ASCII_FMT_BITS_SEC:
                 data_fmt = bytes_qty * DEF_OCTET_NBR_BITS;
                 break;


            case IPERF_ASCII_FMT_KBITS_SEC:
                 data_fmt = (bytes_qty * DEF_OCTET_NBR_BITS) / 1000u;
                 break;


            case IPERF_ASCII_FMT_MBITS_SEC:
                 data_fmt = (bytes_qty * DEF_OCTET_NBR_BITS) / (1000u * 1000u);
                 break;


            case IPERF_ASCII_FMT_GBITS_SEC:
                 data_fmt = (bytes_qty * DEF_OCTET_NBR_BITS) / (1000u * 1000u * 1000u);
                 break;


            case IPERF_ASCII_FMT_BYTES_SEC:
                 data_fmt = bytes_qty;
                 break;


            case IPERF_ASCII_FMT_KBYTES_SEC:
                 data_fmt = bytes_qty / 1024u;
                 break;


            case IPERF_ASCII_FMT_MBYTES_SEC:
                 data_fmt = bytes_qty / (1024u * 1024u);
                 break;


            case IPERF_ASCII_FMT_GBYTES_SEC:
                 data_fmt = bytes_qty / (1024u * 1024u * 1024u);
                 break;


            default:
                 data_fmt = 0u;
                 break;
    }

    return (data_fmt);
}


/*
*********************************************************************************************************
*                                       IPerf_UpdateBandwidth()
*
* Description : Calculation of the bandwidth and update of test statistics.
*
* Argument(s) : p_test      Pointer to a test.
*               ------      Argument validated in IPerf_Init();
*                                    checked   in IPerf_ServerTCP();
*                                                 IPerf_ServerUDP();
*                                                 IPerf_ClientTCP();
*                                                 IPerf_ClientUDP()
*
*                                                   by IPerf_TestTaskHandler().
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_ServerTCP(),
*               IPerf_ServerUDP(),
*               IPerf_ClientTCP(),
*               IPerf_ClientUDP().
*
*               This function is an INTERNAL IPerf server/client function & MUST NOT be called by
*               application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

#if (IPERF_CFG_BANDWIDTH_CALC_EN == DEF_ENABLED)
void  IPerf_UpdateBandwidth (IPERF_TEST   *p_test,
                             IPERF_TS_MS  *p_ts_ms_prev,
                             CPU_INT32U   *p_data_bytes_prev)
{
    IPERF_STATS  *p_stats;
    IPERF_OPT    *p_opt;
    CPU_INT32U    data_bytes_cur;
    CPU_INT32U    data_bytes_delta;
    CPU_INT32U    data_fmtd_delta;
    IPERF_TS_MS   ts_max;
    IPERF_TS_MS   ts_ms_cur;
    IPERF_TS_MS   ts_ms_delta;


    p_stats = &p_test->Stats;
    p_opt   = &p_test->Opt;

                                                                /* elapsed time calc.                                   */
    if (p_stats->TS_End_ms != 0u) {
       *p_ts_ms_prev     =  p_stats->TS_Start_ms;
        ts_ms_cur        =  p_stats->TS_End_ms;
        data_bytes_delta =  p_stats->Bytes;
    } else {
        data_bytes_cur   =  p_stats->Bytes;
        data_bytes_delta =  data_bytes_cur - *p_data_bytes_prev;
        ts_ms_cur        =  IPerf_Get_TS_ms();
    }

    if (ts_ms_cur >= *p_ts_ms_prev) {
        ts_ms_delta = (ts_ms_cur - *p_ts_ms_prev);
    } else {
        ts_max      =  IPerf_Get_TS_Max_ms();
        ts_ms_delta = (ts_ms_cur + (ts_max - *p_ts_ms_prev));
    }

    if (ts_ms_delta >= p_opt->Interval_ms) {

        if (ts_ms_delta == 0) {                                 /* Prevent division by 0.                               */
            return;
        }
                                                                /* Get amount of data formated correclty.               */
        data_fmtd_delta    =  IPerf_GetDataFmtd(p_opt->Fmt, data_bytes_delta);

                                                                /* Bandwidth calc.                                      */
        p_stats->Bandwidth = (data_fmtd_delta * DEF_TIME_NBR_mS_PER_SEC) / ts_ms_delta;

       *p_ts_ms_prev       = ts_ms_cur;                         /* Update ptrs values for the next call.                */
       *p_data_bytes_prev  = data_bytes_cur;

        IPERF_TRACE_DBG(("Time Interval (ms)       = %u\n\r", (unsigned int)ts_ms_delta));
        IPERF_TRACE_DBG(("Delta bytes tx'd or rx'd = %u\n\r", (unsigned int)data_bytes_delta));
        IPERF_TRACE_DBG(("Bandwidth                = %u [format = %c]\n\r\n\r\n\r", (unsigned int)p_stats->Bandwidth, p_opt->Fmt));
    }
}
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           IPerf_ArgScan()
*
* Description : Scan & parse the command line.
*
* Argument(s) : p_in            Pointer to a NULL terminated string holding a complete command & its argument(s).
*               ----            Argument checked   in IPerf_TestStart().
*
*               p_arg_tbl       Pointer to an array of pointer that will receive pointers to token.
*               ---------       Argument validated in IPerf_TestStart().
*
*               arg_tbl_size    Size of arg_tbl array.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   IPERF_ERR_NONE              No error.
*                                   IPERF_ERR_ARG_TBL_FULL      Argument table full & token still to be parsed.
*
* Return(s)   : Number of token(s) (command name & argument(s)).
*
* Caller(s)   : IPerf_TestStart().
*
* Note(s)     : (1) The first token is always the command name itself.
*
*               (2) This function modify the 'in' arguments by replacing token's delimiter characters by
*                   termination character ('\0').
*********************************************************************************************************
*/

static  CPU_INT16U  IPerf_ArgScan (CPU_CHAR    *p_in,
                                   CPU_CHAR    *p_arg_tbl[],
                                   CPU_INT16U   arg_tbl_size,
                                   IPERF_ERR   *p_err)
{
    CPU_CHAR     *p_in_rd;
    CPU_INT16U    tok_ix;
    CPU_BOOLEAN   end_tok_found;
    CPU_BOOLEAN   quote_opened;


    p_in_rd       = p_in;
    tok_ix        = 0u;
    end_tok_found = DEF_YES;
    quote_opened  = DEF_NO;
                                                                /* ------------------ SCAN CMD LINE  ------------------ */
    while (*p_in_rd) {
        switch (*p_in_rd) {
            case IPERF_ASCII_QUOTE:                             /* Quote char found.                                    */
                 if (quote_opened == DEF_YES) {
                      quote_opened  =  DEF_NO;
                     *p_in_rd       = (CPU_CHAR)0;
                      end_tok_found =  DEF_YES;
                 } else {
                      quote_opened  =  DEF_YES;
                 }
                 break;


            case IPERF_ASCII_SPACE:                             /* Space char found.                                    */
                 if ((end_tok_found == DEF_NO) &&               /* If first space between tok & quote NOT opened ...    */
                     (quote_opened  == DEF_NO)) {
                     *p_in_rd        = IPERF_ASCII_ARG_END;     /* ... put termination char.                            */
                      end_tok_found  = DEF_YES;
                 }
                 break;


            default:                                            /* Other char found.                                    */
                 if (end_tok_found == DEF_YES) {
                     if (tok_ix < arg_tbl_size) {
                         p_arg_tbl[tok_ix] = p_in_rd;           /* Set p_arg_tbl ptr to tok location.                   */
                         tok_ix++;
                         end_tok_found     = DEF_NO;
                     } else {
                        *p_err = IPERF_ERR_ARG_TBL_FULL;
                         return (0u);
                     }
                 }
                 break;
        }
        p_in_rd++;
    }

   *p_err = IPERF_ERR_NONE;

    return (tok_ix);
}


/*
*********************************************************************************************************
*                                          IPerf_ArgParse()
*
* Description : (1) Valid argument & set IPerf command structure :
*
*                   (a) Validate arguments counts
*                   (b) Read from command line option & overwrite default test options
*                   (c) Validate test mode set
*                   (d) Validate buffer size option to not exceed maximum buffer size
*
*
* Argument(s) : argc            Count of the arguments supplied &    argv   .
*
*               p_argv          Pointer to an array of pointers to the strings which are arguments.
*               ------          Argument validated in IPerf_TestStart().
*
*               p_opt           Pointer to the current IPerf test cmd.
*               -----           Argument validated in IPerf_Init(),
*                                        checked   in IPerf_TestStart().
*
*               p_out_fnct      Pointer to string output function.
*
*               p_out_param     Pointer to        output function parameters.
*
*               p_err           Pointer to variable that will receive the return error code from
*                               this function :
*
*                                   IPERF_ERR_NONE                          Test configuration successfully parsed.
*                                   IPERF_ERR_ARG_NO_ARG                    Arguments         NOT specified.
*                                   IPERF_ERR_ARG_NO_VAL                    Values            NOT specified.
*                                   IPERF_ERR_ARG_INVALID_OPT               Arguments         NOT valided.
*                                   IPERF_ERR_ARG_INVALID_VAL               Values            NOT valided.
*                                   IPERF_ERR_ARG_INVALID_MODE              Mode              NOT enabled.
*                                   IPERF_ERR_ARG_INVALID_REMOTE_ADDR       Remote addresse   NOT valided.
*                                   IPERF_ERR_ARG_EXCEED_MAX_LEN            Buffer length     NOT valided.
*                                   IPERF_ERR_ARG_EXCEED_MAX_UDP_LEN        UDP buffer length NOT valided.
*                                   IPERF_ERR_ARG_PARAM_NOT_SUPPORTED_OPT   Option            NOT supported.
*                                   IPERF_ERR_ARG_PARAM_NOT_SUPPORTED_VAL   Option value      NOT supported.
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_TestStart().
*
* Note(s)     : none.
*********************************************************************************************************
*/
static void  IPerf_ArgParse (CPU_INT16U        argc,
                             CPU_CHAR         *p_argv[],
                             IPERF_OPT        *p_opt,
                             IPERF_OUT_FNCT    p_out_fnct,
                             IPERF_OUT_PARAM  *p_out_param,
                             IPERF_ERR        *p_err)
{
    CPU_INT16U    arg_ctr;                                      /* Switch between each param/val.                       */
    CPU_INT16U    arg_int;
    CPU_BOOLEAN   find_opt;                                     /* Switch between param & val srch.                     */
    CPU_BOOLEAN   next_arg;                                     /* Switch to the next arg.                              */
    CPU_BOOLEAN   server_client_found;
    CPU_BOOLEAN   help_version_found;
    CPU_BOOLEAN   len_found;                                    /* Prevent erasing len with UDP protocol.               */
    CPU_BOOLEAN   parse_done;                                   /* Exit parsing while.                                  */
    CPU_CHAR      opt_found;                                    /* Save the opt val wish will be found.                 */
    CPU_CHAR     *p_opt_line;                                   /* Ptr  to cur pos in the cmd line.                     */
    CPU_BOOLEAN   win_found;                                    /* Prevent erasing win size.                            */
#ifdef  IPERF_CLIENT_MODULE_PRESENT
    CPU_BOOLEAN   valid_char;
#endif


                                                                /* ----------------- VALIDATE ARG CNT ----------------- */
    if (argc <= 1u) {
       *p_err = IPERF_ERR_ARG_NO_ARG;
        IPERF_TRACE_DBG(("IPerf no arg error : %u\n" , (unsigned int)*p_err));
        return;
    }


   *p_err               = IPERF_ERR_NONE;
    find_opt            = DEF_YES;                              /* Start by trying to find an opt.                      */
    next_arg            = DEF_YES;                              /* First arg is the cmd name.                           */
    len_found           = DEF_NO;
    server_client_found = DEF_NO;
    help_version_found  = DEF_NO;
    parse_done          = DEF_NO;
    arg_ctr             = 0;
    p_opt_line          = p_argv[arg_ctr];
    win_found           = DEF_NO;

                                                                /* ------------------ RD & SET OPTS ------------------- */
    while (parse_done == DEF_NO) {

        if (next_arg == DEF_YES) {
            arg_ctr++;
            if (arg_ctr < argc) {
                p_opt_line = p_argv[arg_ctr];                   /* Set ptr to the next possible opt.                    */
            } else {
                parse_done = DEF_YES;
            }
        }

        if (( find_opt == DEF_YES) &&
            (*p_opt_line == IPERF_ASCII_BEGIN_OPT)) {
            p_opt_line++;                                       /* Param found.                                         */
            if (p_opt_line[1] == IPERF_ASCII_ARG_END) {         /* If param have only one char, it's valid.             */
                find_opt  =  DEF_NO;                            /* Try to find a val.                                   */
                next_arg  =  DEF_YES;                           /* Switch to the next argv.                             */
                opt_found = *p_opt_line;                        /* Save opt found.                                      */

            } else {
               *p_err = IPERF_ERR_ARG_INVALID_OPT;              /* Param with more than 2 chars is invalid.             */
            }

        } else if (find_opt == DEF_NO) {                        /* If an opt is found.                                  */
            next_arg = DEF_YES;
            switch (opt_found) {
                case IPERF_ASCII_OPT_HELP:                      /* Help opt found.                                      */
                     IPerf_PrintMenu(p_out_fnct, p_out_param);
                     help_version_found = DEF_YES;
                     next_arg           = DEF_NO;
                     break;


                case IPERF_ASCII_OPT_VER:                       /* Ver opt found.                                       */
                     IPerf_PrintVer(p_out_fnct, p_out_param);
                     help_version_found = DEF_YES;
                     next_arg           = DEF_NO;
                     break;


                case IPERF_ASCII_OPT_SERVER:                    /* Server opt found.                                    */
#ifdef  IPERF_SERVER_MODULE_PRESENT
                     if (server_client_found == DEF_NO) {       /* IF mode has not been set.                            */
                         server_client_found = DEF_YES;
                         p_opt->Mode = IPERF_MODE_SERVER;
                         if (win_found == DEF_NO) {
                            p_opt->WinSize = IPERF_DFLT_RX_WIN;
                         }

                     } else {                                   /* Mode can't be set more than one time.                */
                         *p_err = IPERF_ERR_ARG_INVALID_OPT;
                     }
                     next_arg = DEF_NO;
#else
                    *p_err    = IPERF_ERR_ARG_INVALID_MODE;     /* Rtn err if mode opt not en.                          */
#endif
                     break;


                case IPERF_ASCII_OPT_CLIENT:                    /* Client opt found.                                    */
#ifdef  IPERF_CLIENT_MODULE_PRESENT
                     if (server_client_found == DEF_NO) {
                         server_client_found = DEF_YES;

                         valid_char = ASCII_IsDigHex(*p_opt_line);
                         if (valid_char == DEF_FALSE) {
                            *p_err = IPERF_ERR_ARG_INVALID_REMOTE_ADDR;
                             return;
                         }

                         Str_Copy(p_opt->IP_AddrRemote, p_opt_line);
                         p_opt->Mode = IPERF_MODE_CLIENT;

                         if (win_found == DEF_NO) {
                            p_opt->WinSize = IPERF_DFLT_TX_WIN;
                         }

                     } else {                                   /* Mode can't be set more than one time.                */
                        *p_err = IPERF_ERR_ARG_INVALID_OPT;
                     }
#else
                    *p_err = IPERF_ERR_ARG_INVALID_MODE;        /* Rtn err if mode opt not en'd.                        */
#endif
                     break;


                case IPERF_ASCII_OPT_FMT:                       /* Fmt opt found.                                       */
                     IPerf_ArgFmtGet(p_opt_line, &p_opt->Fmt, p_err);
                     break;


                case IPERF_ASCII_OPT_TIME:                      /* Time dur opt found.                                  */
                     arg_int            =  Str_ParseNbr_Int32U((CPU_CHAR  *)p_opt_line,
                                                               (CPU_CHAR **)0,
                                                               (CPU_INT08U )DEF_NBR_BASE_DEC);
                     p_opt->Duration_ms = arg_int * DEF_TIME_NBR_mS_PER_SEC;
                     p_opt->BytesNbr    = 0u;
                     break;


                case IPERF_ASCII_OPT_NUMBER:                    /* Buf nbr opt found.                                   */
                     p_opt->BytesNbr    = Str_ParseNbr_Int32U((CPU_CHAR  *)p_opt_line,
                                                              (CPU_CHAR **)0,
                                                              (CPU_INT08U )DEF_NBR_BASE_DEC);
                     p_opt->Duration_ms = 0u;
                     break;


                case IPERF_ASCII_OPT_LENGTH:                    /* Len opt found.                                       */
                     p_opt->BufLen = Str_ParseNbr_Int32U((CPU_CHAR  *)p_opt_line,
                                                         (CPU_CHAR **)0,
                                                         (CPU_INT08U )DEF_NBR_BASE_DEC);
                     len_found     = DEF_YES;
                     break;


                case IPERF_ASCII_OPT_PORT:                      /* Port opt found.                                      */
                     p_opt->Port = Str_ParseNbr_Int32U((CPU_CHAR  *)p_opt_line,
                                                       (CPU_CHAR **)0,
                                                       (CPU_INT08U )DEF_NBR_BASE_DEC);
                     break;


                case IPERF_ASCII_OPT_UDP:                       /* UDP mode opt found.                                  */
                     p_opt->Protocol = IPERF_PROTOCOL_UDP;
                     if (len_found == DEF_NO) {
                        p_opt->BufLen = IPERF_DFLT_UDP_BUF_LEN; /* Use dflt UDP len.                                    */
                     }                                          /* UDP frag not yet supported.                          */
                     if (p_opt->BufLen > IPERF_UDP_BUF_LEN_MAX) {
                        *p_err = IPERF_ERR_ARG_EXCEED_MAX_UDP_LEN;
                     }
                     next_arg = DEF_NO;
                     break;

                case IPERF_ASCII_OPT_WINDOW:                    /* Win opt found.                                       */
                     p_opt->WinSize = Str_ParseNbr_Int32U((CPU_CHAR  *)p_opt_line,
                                                          (CPU_CHAR **)0,
                                                          (CPU_INT08U )DEF_NBR_BASE_DEC);
                     win_found      = DEF_YES;
                     break;


                case IPERF_ASCII_OPT_PERSISTENT:                /* Persistent server opt found.                         */
                     p_opt->Persistent = DEF_ENABLED;
                     next_arg          = DEF_NO;
                     break;


                case IPERF_ASCII_OPT_INTERVAL:                  /* Interval opt found.                                  */
                     p_opt->Interval_ms = Str_ParseNbr_Int32U((CPU_CHAR  *)p_opt_line,
                                                              (CPU_CHAR **)0,
                                                              (CPU_INT08U )DEF_NBR_BASE_DEC);
                     break;


                case IPERF_ASCII_OPT_IPV6:                      /* IPV6 opt found.                                      */
                     p_opt->IPv4 = DEF_NO;
                     break;


                default:                                        /* Unknown opt found.                                   */
                    *p_err = IPERF_ERR_ARG_INVALID_OPT;         /* Rtn invalid opt err.                                 */
                     break;
            }
            find_opt = DEF_YES;

        } else {
            p_opt_line++;
        }

        if ((p_opt_line == ASCII_CHAR_NULL) ||
            (arg_ctr >= argc)              ) {
             parse_done = DEF_YES;
        }

        if (*p_err != IPERF_ERR_NONE) {
            IPERF_TRACE_DBG(("IPerf option error : %u\n" , (unsigned int)*p_err));

            return;
        }
    }


                                                                /* ---------------- VALIDATE TEST MODE ---------------- */
    if (server_client_found == DEF_NO) {
        if (help_version_found == DEF_NO) {
           *p_err = IPERF_ERR_ARG_INVALID_OPT;
        } else {
           *p_err = IPERF_ERR_ARG_NO_TEST;
        }
        return;
    }
                                                                /* ---------------- VALIDATE BUF SIZE ----------------- */
    if (p_opt->IPv4 == DEF_YES) {
        if (((p_opt->Protocol == IPERF_PROTOCOL_TCP) && (p_opt->BufLen > IPERF_TCP_BUF_LEN_MAX)) ||
            ((p_opt->Protocol == IPERF_PROTOCOL_UDP) && (p_opt->BufLen > IPERF_UDP_BUF_LEN_MAX))) {
           *p_err = IPERF_ERR_ARG_EXCEED_MAX_LEN;
            return;
        }
    } else {
        if (((p_opt->Protocol == IPERF_PROTOCOL_TCP) && (p_opt->BufLen > IPERF_TCP_BUF_LEN_MAX))       ||
            ((p_opt->Protocol == IPERF_PROTOCOL_UDP) && (p_opt->BufLen > IPERF_UDP_BUF_LEN_MAX_IPv6))) {
           *p_err = IPERF_ERR_ARG_EXCEED_MAX_LEN;
            return;
        }
    }
}


/*
*********************************************************************************************************
*                                          IPerf_ArgFmtGet()
*
* Description : (1) Get format argument from command line string :
*
*                   (a) Validate string pointer
*                   (b) Validate format & set test option
*
*
* Argument(s) : p_str_int_arg   Pointer to first string charater of the integer argument value in the arguments array.
*               -------------   Argument checked in IPerf_ArgParse().
*
*               p_fmt           Pointer to variable that will receive the format result.
*               -----           Argument validated in IPerf_Init(),
*                                        checked   in IPerf_ArgParse
                                                        by IPerf_TestStart().
*
*               p_err           Pointer to variable that will receive the return error code from this function
*
*                                   IPERF_ERR_NONE                             format value is valided & set.
*                                   IPERF_ERR_ARG_PARAM_VALUE_NOT_SUPPORTED    format value is NOT supported.
*                                   IPERF_ERR_ARG_INVALID_VAL                  format value is NOT valided.
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_ArgParse().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  IPerf_ArgFmtGet (CPU_CHAR   *p_str_arg,
                               IPERF_FMT  *p_fmt,
                               IPERF_ERR  *p_err)
{
    if (p_str_arg[1] != IPERF_ASCII_ARG_END) {
        *p_err = IPERF_ERR_ARG_INVALID_VAL;                     /* Can NOT have more than one char.                     */
         return;
    }


                                                                /* ---------------- VALIDATE & SET FMT ---------------- */
   *p_fmt = *p_str_arg;
    switch (*p_fmt) {
        case IPERF_ASCII_FMT_BITS_SEC:
        case IPERF_ASCII_FMT_KBITS_SEC:
        case IPERF_ASCII_FMT_MBITS_SEC:
        case IPERF_ASCII_FMT_GBITS_SEC:
        case IPERF_ASCII_FMT_BYTES_SEC:
        case IPERF_ASCII_FMT_KBYTES_SEC:
        case IPERF_ASCII_FMT_MBYTES_SEC:
        case IPERF_ASCII_FMT_GBYTES_SEC:
            *p_err = IPERF_ERR_NONE;
             break;


        case IPERF_ASCII_FMT_ADAPTIVE_BITS_SEC:
        case IPERF_ASCII_FMT_ADAPTIVE_BYTES_SEC:
            *p_err = IPERF_ERR_ARG_PARAM_NOT_SUPPORTED_VAL;
             break;


        default:
            *p_err = IPERF_ERR_ARG_INVALID_VAL;
             break;
    }
}


/*
*********************************************************************************************************
*                                         IPerf_PrintOutput()
*
* Description : Output error code string via IPerf output display function.
*                   (a) Validate output function pointer
*                   (b) Print error string
*
*
* Argument(s) : p_out_fnct      Pointer to string output function.
*
*               p_out_param     Pointer to        output function parameters.
*
*               err             Desired error code to output corresponding error code string.
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_TestStart().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  IPerf_PrintErr (IPERF_OUT_FNCT    p_out_fnct,
                              IPERF_OUT_PARAM  *p_out_param,
                              IPERF_ERR         err)
{
                                                                /* ---------------- VALIDATE FNCT PTR ----------------- */
    if (p_out_fnct == (IPERF_OUT_FNCT)0) {
        return;
    }

                                                                /* ---------------- PRINT ERR CODE STR ---------------- */
    switch (err) {
        case IPERF_ERR_ARG_EXCEED_MAX_UDP_LEN:
             p_out_fnct(IPERF_MSG_ERR_UDP_LEN,           p_out_param);
             break;


        case IPERF_ERR_ARG_EXCEED_MAX_LEN:
             p_out_fnct(IPERF_MSG_ERR_BUF_LEN,           p_out_param);
             break;


        case IPERF_ERR_ARG_PARAM_NOT_SUPPORTED_OPT:
             p_out_fnct(IPERF_MSG_ERR_OPT_NOT_SUPPORTED, p_out_param);
             break;


        case IPERF_ERR_ARG_INVALID_MODE:
             p_out_fnct(IPERF_MSG_ERR_NOT_EN,            p_out_param);
             break;

        case IPERF_ERR_ARG_NO_TEST:
             break;


        case IPERF_ERR_ARG_NO_ARG:
        case IPERF_ERR_ARG_INVALID_OPT:
        case IPERF_ERR_ARG_INVALID_VAL:
        case IPERF_ERR_ARG_NO_VAL:
        case IPERF_ERR_ARG_INVALID_REMOTE_ADDR:
        default:
             p_out_fnct(IPERF_MSG_ERR_OPT,               p_out_param);
             break;
    }
}


/*
*********************************************************************************************************
*                                        IPerf_PrintVersion()
*
* Description : Output IPerf version via IPerf output display function.
*                   (a) Validate output function pointer
*                   (b) Print version
*
*
* Argument(s) : p_out_fnct      Pointer to string output function.
*
*               p_out_param     Pointer to        output function parameters.
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_ArgValidSet().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  IPerf_PrintVer (IPERF_OUT_FNCT    p_out_fnct,
                              IPERF_OUT_PARAM  *p_out_param)
{
    CPU_INT32U  ver;
    CPU_CHAR    buf[IPERF_MSG_VER_STR_MAX_LEN + 1];

                                                                /* ---------------- VALIDATE FNCT PTR ----------------- */
    if (p_out_fnct == (IPERF_OUT_FNCT)0) {
        return;
    }

                                                                /* ------------------ PRINT VERSION ------------------- */
    ver = IPERF_VERSION / 10000u;                               /* Major.                                               */
   (void)Str_FmtNbr_Int32U(ver,   2, DEF_NBR_BASE_DEC, ' ', DEF_NO, DEF_NO, &buf[0]);
    buf[2] = '.';

    ver = IPERF_VERSION % 10000u;                               /* Minor.                                               */
    ver = ver           / 100u;
   (void)Str_FmtNbr_Int32U(ver,   2, DEF_NBR_BASE_DEC, '0', DEF_NO, DEF_NO, &buf[3]);
    buf[5] = '.';

    ver = IPERF_VERSION % 100u;                                 /* Revision.                                            */
   (void)Str_FmtNbr_Int32U(ver,   2, DEF_NBR_BASE_DEC, '0', DEF_NO, DEF_YES, &buf[6]);

    p_out_fnct("iperf version", p_out_param);
    p_out_fnct( buf,            p_out_param);
    p_out_fnct("\r\n",          p_out_param);
}


/*
*********************************************************************************************************
*                                        IPerf_PrintMenu()
*
* Description : Output IPerf menu via IPerf output display function.
*                   (a) Validate output function pointer
*                   (b) Print menu
*
*
* Argument(s) : p_out_fnct      Pointer to string output function.
*
*               p_out_param     Pointer to        output function parameters.
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_ArgValidSet().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  IPerf_PrintMenu (IPERF_OUT_FNCT    p_out_fnct,
                               IPERF_OUT_PARAM  *p_out_param)
{
                                                                /* ---------------- VALIDATE FNCT PTR ----------------- */
    if (p_out_fnct == (IPERF_OUT_FNCT)0) {
        return;
    }

                                                                /* -------------------- PRINT MENU -------------------- */
    p_out_fnct(IPERF_MSG_MENU, p_out_param);
}


/*
*********************************************************************************************************
*                                          IPerf_TestSrch()
*
* Description : Search IPerf test List for test with specific test ID.
*
*               (1) IPerf tests are linked to form an IPerf Test List.
*
*                   (a) In the diagram below, ... :
*
*                       (1) The horizontal row represents the list of IPerf tests.
*
*                       (2) 'IPerf_TestListHead' points to the head of the IPerf Test List.
*
*                       (3) IPerf tests' 'PrevPtr' & 'NextPtr' doubly-link each test to form the
*                           IPerf Test List.
*
*                   (b) (1) For any IPerf Test List lookup, all IPerf tests are searched in order
*                           to find the test with the appropriate test ID.
*
*                       (2) To expedite faster IPerf Test List lookup :
*
*                           (A) (1) (a) Iperf tests are added at;            ...
*                                   (b) Iperf tests are searched starting at ...
*                               (2) ... the head of the IPerf Test List.
*
*                           (B) As IPerf tests are added into the list, older IPerf tests migrate to the tail
*                               of the IPerf Test List.  Once an IPerf test is left, it is removed from the
*                               IPerf Test List.
*
*
*                                        |                                               |
*                                        |<------------ List of IPerf Tests ------------>|
*                                        |                (see Note #1a1)                |
*
*                                    New IPerf test                            Oldest Iperf test
*                                   inserted at head                           in IPerf Test List
*                                   (see Note #1b2A2)                           (see Note #1b2B)
*
*                                           |                 NextPtr                 |
*                                           |             (see Note #1a3)             |
*                                           v                    |                    v
*                                                                |
*                      Head of IPerf     -------       -------   v   -------       -------
*                        Test List  ---->|     |------>|     |------>|     |------>|     |
*                                        |     |       |     |       |     |       |     |        Tail of IPerf
*                     (see Note #1a2)    |     |<------|     |<------|     |<------|     |<----     Test List
*                                        -------       -------   ^   -------       -------
*                                                                |
*                                                                |
*                                                             PrevPtr
*                                                         (see Note #1a3)
*
*
* Argument(s) : test_id     Interface number to search for host group.
*
* Return(s)   : Pointer to IPerf test with specific test ID, if found.
*
*               Pointer to NULL,                             otherwise.
*
* Caller(s)   : IPerf_TestTaskHandler(),
*               IPerf_TestGetStatus(),
*               IPerf_TestGetResults().
*
* Note(s)     : none.
*********************************************************************************************************
*/
static  IPERF_TEST  *IPerf_TestSrch (IPERF_TEST_ID  test_id)
{
    IPERF_TEST   *p_test;
    IPERF_TEST   *p_test_next;
    CPU_BOOLEAN   found;


    p_test = IPerf_TestListHeadPtr;                             /* Start @ IPerf Test List head.                        */
    found  = DEF_NO;

    while ((p_test != (IPERF_TEST *)0) &&                       /* Srch Iperf Test List ...                             */
           (found  ==  DEF_NO        )) {                       /* ... until test id found.                             */

        p_test_next = (IPERF_TEST *) p_test->NextPtr;
                                                                /* Cmp TestID.                                          */
        found = (p_test->TestID == test_id) ? DEF_YES : DEF_NO;

        if (found != DEF_YES) {                                 /* If NOT found, adv to next IPerf Test.                */
            p_test = p_test_next;
        }
    }

    return (p_test);
}


/*
*********************************************************************************************************
*                                           IPerf_TestAdd()
*
* Description : (1) Add a test to the IPerf Test List :
*
*                   (a) Get a     test from test pool
*                   (b) Configure test
*                   (c) Insert    test into IPerf Test List
*
*
* Argument(s) : test_id     Test ID to add test.
*               -------     validated in IPerf_TestStart().
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   IPERF_ERR_NONE               Test succesfully added.
*
*                                                                -- RETURNED BY IPerf_TestGet() : ---
*                                   IPERF_ERR_TEST_NONE_AVAIL    NO available host group to allocate.
*
* Return(s)   : Pointer to test, if no error.
*
*               Pointer to NULL, otherwise.
*
* Caller(s)   : IPerf_TestStart().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  IPERF_TEST  *IPerf_TestAdd (IPERF_TEST_ID   test_id,
                                    IPERF_ERR      *p_err)
{
    IPERF_TEST  *p_test;


    p_test = IPerf_TestGet(p_err);                              /* ------------------ GET IPERF TEST ------------------ */
    if (p_test == (IPERF_TEST *)0) {
        return ((IPERF_TEST *)0);                               /* Rtn err from IPerf_TestGet().                        */
    }


                                                                /* ------------------ CFG IPERF TEST ------------------ */
    p_test->TestID = test_id;


                                                                /* --------- INSERT TEST INTO IPERF TEST LIST --------- */
    IPerf_TestInsert(p_test);


   *p_err =  IPERF_ERR_NONE;

    return (p_test);
}


/*
*********************************************************************************************************
*                                       IPerf_TestRemove()
*
* Description : Remove a test from the IPerf Test List :
*
*                   (a) Remove test from IPerf Test List
*                   (b) Free   test back to    test pool
*
*
* Argument(s) : p_test      Pointer to a test.
*               ------      Argument checked in IPerf_TestStart()
*                                               IPerf_TestGetStatus(),
*                                               IPerf_TestGetResults()
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_TestStart()
*               IPerf_TestGetStatus(),
*               IPerf_TestGetResults().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  IPerf_TestRemove (IPERF_TEST  *p_test)
{
    IPerf_TestUnlink(p_test);                                   /* --------- REMOVE TEST FROM IPERF TEST LIST --------- */


    IPerf_TestFree(p_test);                                     /* -------------------- FREE TEST --------------------- */
}


/*
*********************************************************************************************************
*                                       IPerf_TestInsert()
*
* Description : Insert a test into the IPerf Test List.
*
* Argument(s) : p_test      Pointer to a test.
*               ------      Argument checked in IPerf_TestAdd().
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_TestAdd().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  IPerf_TestInsert (IPERF_TEST  *p_test)
{
                                                                /* --------------- CFG IPERF TEST PTRS ---------------- */
    p_test->PrevPtr = (IPERF_TEST *)0;
    p_test->NextPtr = (IPERF_TEST *)IPerf_TestListHeadPtr;

                                                                /* ----------- INSERT IPERF TEST INTO LIST ------------ */
    if (IPerf_TestListHeadPtr != (IPERF_TEST *)0) {             /* If list NOT empty, insert before head.               */
        IPerf_TestListHeadPtr->PrevPtr = p_test;
    }

    IPerf_TestListHeadPtr = p_test;                             /* Insert test @ list head.                             */
}


/*
*********************************************************************************************************
*                                       IPerf_TestUnlink()
*
* Description : Unlink a test from the IPerf Test List.
*
* Argument(s) : p_test      Pointer to a test.
*               ------      Argument checked in IPerf_TestRemove()
*                                                 by IPerf_TestStart()
*                                                    IPerf_TestGetStatus(),
*                                                    IPerf_TestGetResults()
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_TestRemove().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  IPerf_TestUnlink (IPERF_TEST  *p_test)
{
    IPERF_TEST  *p_test_prev;
    IPERF_TEST  *p_test_next;

                                                                /* ----------- UNLINK IPERF TEST FROM LIST ------------ */
    p_test_prev = p_test->PrevPtr;
    p_test_next = p_test->NextPtr;
                                                                /* Point prev test to next test.                        */
    if (p_test_prev != (IPERF_TEST *)0) {
        p_test_prev->NextPtr  = p_test_next;
    } else {
        IPerf_TestListHeadPtr = p_test_next;
    }
                                                                /* Point next test to prev test.                        */
    if (p_test_next != (IPERF_TEST *)0) {
        p_test_next->PrevPtr = p_test_prev;
    }

#if (NET_DBG_CFG_MEM_CLR_EN == DEF_ENABLED)                     /* Clr test ptrs.                                       */
    p_test->PrevPtr = (IPERF_TEST *)0;
    p_test->NextPtr = (IPERF_TEST *)0;
#endif
}


/*
*********************************************************************************************************
*                                        IPerf_TestGet()
*
* Description : (1) Allocate & initialize a test :
*
*                   (a) Get a test
*                   (b) Validate   test
*                   (c) Initialize test
*                   (e) Return pointer to test
*                         OR
*                       Null pointer & error code, on failure
*
*               (2) The test pool is implemented as a stack :
*
*                   (a) 'IPerf_TestPoolPtr' points to the head   of the test pool.
*
*                   (b) Tests' 'NextPtr's link each test to form    the test pool stack.
*
*                   (c) Tests are inserted & removed at the head of the test pool stack.
*
*
*                                           Test are
*                                     inserted & removed
*                                        at the head
*                                       (see Note #2c)
*
*                                             |                 NextPtr
*                                             |             (see Note #2b)
*                                             v                    |
*                                                                  |
*                                          -------       -------   v   -------       -------
*                              Test   ---->|     |------>|     |------>|     |------>|     |
*                            Pointer       |     |       |     |       |     |       |     |
*                                          |     |       |     |       |     |       |     |
*                         (see Note #2a)   -------       -------       -------       -------
*
*                                          |                                               |
*                                          |<------------ Pool of Free Tests ------------->|
*                                          |                (see Note #2)                  |
*
*
* Argument(s) : p_err       Pointer to variable that will receive the return error code from this function :
*
*                               IPERF_ERR_NONE               Test successfully allocated & initialized.
*                               IPERF_ERR_TEST_NONE_AVAIL    NO available host group to allocate.
*
* Return(s)   : Pointer to test, if NO errors.
*
*               Pointer to NULL, otherwise.
*
* Caller(s)   : IPerf_TestAdd().
*
* Note(s)     : (3) (a) Test pool is accessed by 'IPerf_TestPoolPtr' during execution of
*
*                       (1) IPerf_Init()
*                       (2) IPerf_TestGet()
*                       (3) IPerf_TestFree()
*********************************************************************************************************
*/

static  IPERF_TEST  *IPerf_TestGet (IPERF_ERR  *p_err)
{
    IPERF_TEST  *p_test;


                                                                /* --------------------- GET TEST --------------------- */
    if (IPerf_TestPoolPtr != (IPERF_TEST *)0) {                 /* If IPerf test pool NOT empty, get test from pool.    */
        p_test             = (IPERF_TEST *)IPerf_TestPoolPtr;
        IPerf_TestPoolPtr  = (IPERF_TEST *)p_test->NextPtr;
    } else {                                                    /* Else none avail, rtn err.                            */
       *p_err =   IPERF_ERR_TEST_NONE_AVAIL;
        return ((IPERF_TEST *)0);
    }

                                                                /* -------------------- INIT TEST --------------------- */
    IPerf_TestClr(p_test);

   *p_err =  IPERF_ERR_NONE;

    return (p_test);
}


/*
*********************************************************************************************************
*                                          IPerf_TestFree()
*
* Description : (1) Free a test :
*
*                   (b) Clear  test controls
*                   (c) Free   test back to test pool
*
*
* Argument(s) : p_test      Pointer to a test.
*               ------      Argument checked in IPerf_TestRemove()
*                                                 by IPerf_TestStart()
*                                                    IPerf_TestGetStatus(),
*                                                    IPerf_TestGetResults().
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_TestRemove().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  IPerf_TestFree (IPERF_TEST  *p_test)
{
                                                                /* --------------------- CLR TEST --------------------- */
    p_test->Status = IPERF_TEST_STATUS_FREE;                    /* Set test as freed/NOT used.                          */

#if (NET_DBG_CFG_MEM_CLR_EN == DEF_ENABLED)
    IPerf_TestClr(p_test);
#endif

                                                                /* -------------------- FREE TEST --------------------- */
    p_test->NextPtr   = IPerf_TestPoolPtr;
    IPerf_TestPoolPtr = p_test;
}


/*
*********************************************************************************************************
*                                        IPerf_TestClr()
*
* Description : Clear IGMP host group controls.
*
* Argument(s) : p_test      Pointer to an IPerf test.
*               ------      Argument validated in IPerf_Init();
*                                    checked   in IPerf_TestGet()
*                                                   by IPerf_TestAdd();
*                                                 IPerf_TestFree()
*                                                   by IPerf_TestRemove().
*
* Return(s)   : none.
*
* Caller(s)   : IPerf_Init(),
*               IPerf_TestGet(),
*               IPerf_TestFree().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  IPerf_TestClr (IPERF_TEST  *p_test)
{
    IPERF_OPT         *p_opt;
    IPERF_STATS       *p_stats;
    IPERF_CONN        *p_conn;
    NET_SOCK_ADDR     *p_addr_sock;


    p_opt                     = &p_test->Opt;
    p_conn                    = &p_test->Conn;
    p_stats                   = &p_test->Stats;


    p_test->PrevPtr           = (IPERF_TEST *)0;
    p_test->NextPtr           = (IPERF_TEST *)0;

    p_test->TestID            =  IPERF_TEST_ID_NONE;
    p_test->Status            =  IPERF_TEST_STATUS_FREE;
    p_test->Err               =  IPERF_ERR_NONE;

    p_conn->SockID            =  NET_SOCK_ID_NONE;
    p_conn->SockID_TCP_Server =  NET_SOCK_ID_NONE;
    p_conn->IF_Nbr            =  IPERF_DFLT_IF;
    p_conn-> Run              =  DEF_NO;


    p_addr_sock               = &p_conn->ServerAddrPort;
    Mem_Clr(p_addr_sock, NET_SOCK_ADDR_SIZE);

    p_addr_sock               = &p_conn->ClientAddrPort;
    Mem_Clr(p_addr_sock, NET_SOCK_ADDR_SIZE);

    p_opt->Mode               =  IPERF_MODE_SERVER;
    p_opt->Protocol           =  IPERF_DFLT_PROTOCOL;
    p_opt->Port               =  IPERF_DFLT_PORT;
    p_opt->IPv4               =  DEF_YES;

    Str_Copy(p_opt->IP_AddrRemote, IPERF_DFLT_IP_REMOTE);
    p_opt->BytesNbr           =  IPERF_DFLT_BYTES_NBR;
    p_opt->BufLen             =  IPERF_DFLT_TCP_BUF_LEN;
    p_opt->Duration_ms        =  IPERF_DFLT_DURATION_MS;
    p_opt->WinSize            =  IPERF_DFLT_RX_WIN;
    p_opt->Persistent         =  IPERF_DFLT_PERSISTENT;
    p_opt->Fmt                =  IPERF_DFLT_FMT;
    p_opt->Interval_ms        =  IPERF_DFLT_INTERVAL_MS;

    IPerf_TestClrStats(p_stats);
}

