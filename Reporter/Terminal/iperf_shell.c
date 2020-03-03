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
*                                           IPERF REPORTER
*
* Filename : iperf_shell.c
* Version  : V2.04.00
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
#include  "iperf_shell.h"
#include  "iperf_rep.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  IPERF_CMD_NAME         "iperf"
#define  IPERF_CMD_TBL_NAME     IPERF_CMD_NAME
#define  IPERF_CMD_NAME_START   IPERF_CMD_NAME


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

typedef  struct  {
    SHELL_OUT_FNCT    OutFnct;
    SHELL_CMD_PARAM  *OutOpt_Ptr;
} IPERF_SHELL_OUT_PARAM;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*********************************************************************************************************
*/


static  SHELL_CMD IPerfCmdTbl[] =
{
    {IPERF_CMD_NAME_START,                IPerfShell_Start},
    {0, 0 }
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

void  IPerfShell_OutputFnct (CPU_CHAR         *p_buf,
                             IPERF_OUT_PARAM  *p_param);


/*
*********************************************************************************************************
*                                            IPerfShell_Init()
*
* Description : Add IPerf functions to uC-Shell.
*
* Argument(s) : p_err    is a pointer to an error code which will be returned to your application:
*
*                             IPERF_ERR_NONE            No error.
*
*                             IPERF_ERR_EXT_SHELL_INIT  Command table not added to uC-Shell
*
* Return(s)   : none.
*
* Caller(s)   : AppTaskStart().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  IPerfShell_Init (IPERF_ERR  *p_err)
{
    SHELL_ERR  shell_err;


    Shell_CmdTblAdd(IPERF_CMD_TBL_NAME, IPerfCmdTbl, &shell_err);

    if (shell_err == SHELL_ERR_NONE) {
        *p_err = IPERF_ERR_NONE;
    } else {
        *p_err = IPERF_ERR_EXT_SHELL_INIT;
         return;
    }
}




/*
*********************************************************************************************************
*                                          IPerfShell_Start()
*
* Description : IPERF_TEST_TODO Add function description.
*
* Argument(s) : argc            IPERF_TEST_TODO Add description for 'argc'
*
*               p_argv          IPERF_TEST_TODO Add description for 'p_argv'
*
*               out_fnct        IPERF_TEST_TODO Add description for 'out_fnct'
*
*               p_cmd_param     IPERF_TEST_TODO Add description for 'p_cmd_param'
*
* Return(s)   : IPERF_TEST_TODO Add return value description.
*
* Caller(s)   : IPerfCmdTbl.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT16S  IPerfShell_Start  (CPU_INT16U        argc,
                               CPU_CHAR         *p_argv[],
                               SHELL_OUT_FNCT    out_fnct,
                               SHELL_CMD_PARAM  *p_cmd_param)
{
    IPERF_SHELL_OUT_PARAM  outparam;
    IPERF_OUT_PARAM        param;
    IPERF_TEST_ID          test_id;
    IPERF_ERR              err_iperf;


    outparam.OutFnct    =  out_fnct;
    outparam.OutOpt_Ptr =  p_cmd_param;
    param.p_out_opt     = &outparam;

    test_id = IPerf_TestShellStart(argc, p_argv, &IPerfShell_OutputFnct, &param, &err_iperf);
    if (err_iperf == IPERF_ERR_NONE) {
        IPerf_Reporter(test_id,
                      &IPerfShell_OutputFnct,
                      &param);
    }

    return (1);
}


/*
*********************************************************************************************************
*                                       IPerfShell_OutputFnct()
*
* Description : Output a string.
*
* Argument(s) : p_buf       Pointer to buffer to output.
*
* Return(s)   : none.
*
* Caller(s)   : IPerfShell_Start().
*
* Note(s)     : (1) The string pointed to by p_buf has to be NUL ('\0') terminated.
*********************************************************************************************************
*/

void  IPerfShell_OutputFnct (CPU_CHAR         *p_buf,
                             IPERF_OUT_PARAM  *p_param)
{
    IPERF_SHELL_OUT_PARAM  *p_param_shell;
    SHELL_CMD_PARAM        *p_cmd_param;
    CPU_INT16S              output;
    CPU_INT32U              str_len;

                                                                /* ----------------- VALIDATE POINTER ----------------- */
    if (p_buf == (CPU_CHAR *)0) {
        return;
    }
                                                                /* ------------------ DISPLAY STRING ------------------ */
    p_param_shell = p_param->p_out_opt;
    p_cmd_param   = p_param_shell->OutOpt_Ptr;


    str_len = Str_Len(p_buf);
    output = p_param_shell->OutFnct(p_buf,
                                    str_len,
                                    p_cmd_param->pout_opt);
    (void)&output;
}

