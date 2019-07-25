/***************************************************************************//**
 * @file
 * @brief Common - Clock - Calendar Commands
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc.  Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.  This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

/********************************************************************************************************
 ********************************************************************************************************
 *                                           INCLUDE FILES
 ********************************************************************************************************
 *******************************************************************************************************/

#include  <rtos_description.h>

#ifdef  RTOS_MODULE_COMMON_SHELL_AVAIL
#include  <common/include/clk.h>

#include  <common/include/shell.h>

#include  <common/include/rtos_path.h>
#include  <common/include/rtos_err.h>
#include  <common/include/toolchains.h>

#include  <common/source/clk/clk_cmd_priv.h>
#include  <common/source/kal/kal_priv.h>
#include  <common/source/rtos/rtos_utils_priv.h>

/********************************************************************************************************
 ********************************************************************************************************
 *                                               LOCAL DEFINES
 ********************************************************************************************************
 *******************************************************************************************************/

#define  CLK_CMD_ARG_BEGIN                  ASCII_CHAR_HYPHEN_MINUS
#define  CLK_CMD_ARG_TIME_TYPE              ASCII_CHAR_LATIN_LOWER_T

#define  CLK_CMD_NBR_MIN_PER_HR             60u
#define  CLK_CMD_NBR_SEC_PER_MIN            60u

#define  RTOS_MODULE_CUR                    RTOS_CFG_MODULE_COMMON

/********************************************************************************************************
 ********************************************************************************************************
 *                                           LOCAL CONSTANTS
 ********************************************************************************************************
 *******************************************************************************************************/

#define  CLK_CMD_HELP                                        ("help")
#define  CLK_CMD_HELP_SHORT                                  ("h")

#define  CLK_CMD_FORMAT_DATETIME                             ("datetime")
#define  CLK_CMD_FORMAT_NTP                                  ("ntp")
#define  CLK_CMD_FORMAT_UNIX                                 ("unix")

#define  CLK_CMD_FORMAT_DATETIME_SHORT                       ("d")
#define  CLK_CMD_FORMAT_NTP_SHORT                            ("n")
#define  CLK_CMD_FORMAT_UNIX_SHORT                           ("u")

#define  CLK_CMD_OUTPUT_CMD_LIST                             ("Command List: ")
#define  CLK_CMD_OUTPUT_ERR                                  ("Error: ")
#define  CLK_CMD_OUTPUT_SUCCESS                              ("Completed successfully")
#define  CLK_CMD_OUTPUT_TABULATION                           ("\t")

#define  CLK_CMD_OUTPUT_ERR_CMD_ARG_INVALID                  ("Invalid Arguments")
#define  CLK_CMD_OUTPUT_ERR_CMD_NOT_IMPLEMENTED              ("This command is not yet implemented")
#define  CLK_CMD_OUTPUT_ERR_CONV_DISABLED                    ("CLK_CFG_STR_CONV_EN is not enabled. Formatting not available.")
#define  CLK_CMD_OUTPUT_ERR_CMD_INTERNAL_ERR                 ("Clk Internal Error. Date & Time cannot be converted.")

#define  CLK_CMD_HELP_SET                                    ("usage: clk_set [VALUE] {FORMAT}\r\n")

#define  CLK_CMD_HELP_GET                                    ("usage: clk_get {FORMAT}\r\n")

#define  CLK_CMD_HELP_VALUE                                  ("where VALUE is:\r\n"                                                 \
                                                              "  YYYY-MM-DD {HH:MM:SS {UTC+/-HH:MM}}      for DATETIME format.\r\n" \
                                                              "  a 32-bit integer greater than 946684799  for UNIX     format.\r\n" \
                                                              "  a 32-bit integer greater than 3155673599 for NTP      format.\r\n")

#define  CLK_CMD_HELP_FORMAT                                 ("where FORMAT is:\r\n"                        \
                                                              "\r\n"                                        \
                                                              "  -d, --datetime   DATETIME format.\r\n"     \
                                                              "  -u, --unix       UNIX     format.\r\n"     \
                                                              "  -n, --ntp        NTP      format.\r\n\r\n" \
                                                              "  if FORMAT is not provided, VALUE is assumed to be in DATETIME format.\r\n")

/********************************************************************************************************
 ********************************************************************************************************
 *                                               DATA TYPES
 ********************************************************************************************************
 *******************************************************************************************************/

/*
 ********************************************************************************************************
 *                                       CLOCK COMMAND TIME DATA TYPE
 *******************************************************************************************************/

typedef enum clk_cmd_time_type {
  CLK_CMD_TIME_TYPE_NONE = 0x00,
  CLK_CMD_TIME_TYPE_DATETIME = 0x30,
  CLK_CMD_TIME_TYPE_NTP = 0x31,
  CLK_CMD_TIME_TYPE_UNIX = 0x32
} CLK_CMD_TIME_TYPE;

/*
 ********************************************************************************************************
 *                                   CLOCK COMMAND ARGUMENT DATA TYPE
 *******************************************************************************************************/

typedef struct clk_cmd_arg {
  CLK_CMD_TIME_TYPE TimeType;
  CPU_CHAR          *DatePtr;
  CPU_CHAR          *TimePtr;
  CPU_CHAR          *OffsetPtr;
} CLK_CMD_ARG;

/*
 ********************************************************************************************************
 *                                   CLOCK COMMAND PARSE STATUS DATA TYPE
 *******************************************************************************************************/

typedef enum clk_cmd_parse_status {
  CLK_CMD_PARSE_STATUS_SUCCESS,
  CLK_CMD_PARSE_STATUS_EMPTY,
  CLK_CMD_PARSE_STATUS_INVALID_ARG,
  CLK_CMD_PARSE_STATUS_HELP
} CLK_CMD_PARSE_STATUS;

/********************************************************************************************************
 ********************************************************************************************************
 *                                       LOCAL FUNCTION PROTOTYPES
 ********************************************************************************************************
 *******************************************************************************************************/

static CPU_INT16S ClkCmd_Help(CPU_INT16U      argc,
                              CPU_CHAR        *p_argv[],
                              SHELL_OUT_FNCT  out_fnct,
                              SHELL_CMD_PARAM *p_cmd_param);

static CPU_INT16S ClkCmd_Set(CPU_INT16U      argc,
                             CPU_CHAR        *p_argv[],
                             SHELL_OUT_FNCT  out_fnct,
                             SHELL_CMD_PARAM *p_cmd_param);

static CPU_INT16S ClkCmd_Get(CPU_INT16U      argc,
                             CPU_CHAR        *p_argv[],
                             SHELL_OUT_FNCT  out_fnct,
                             SHELL_CMD_PARAM *p_cmd_param);

static CPU_INT16S ClkCmd_GetDayOfYr(CPU_INT16U      argc,
                                    CPU_CHAR        *p_argv[],
                                    SHELL_OUT_FNCT  out_fnct,
                                    SHELL_CMD_PARAM *p_cmd_param);

static CLK_CMD_PARSE_STATUS ClkCmd_CmdArgParse(CPU_INT16U  argc,
                                               CPU_CHAR    *p_argv[],
                                               CLK_CMD_ARG *p_cmd_args);

static CPU_INT16S ClkCmd_OutputCmdTbl(SHELL_CMD       *p_cmd_tbl,
                                      SHELL_OUT_FNCT  out_fnct,
                                      SHELL_CMD_PARAM *p_cmd_param);

static CPU_INT16S ClkCmd_OutputError(CPU_CHAR        *p_error,
                                     SHELL_OUT_FNCT  out_fnct,
                                     SHELL_CMD_PARAM *p_cmd_param);

static CPU_INT16S ClkCmd_OutputMsg(const CPU_CHAR  *p_msg,
                                   CPU_BOOLEAN     new_line_start,
                                   CPU_BOOLEAN     new_line_end,
                                   CPU_BOOLEAN     tab_start,
                                   SHELL_OUT_FNCT  out_fnct,
                                   SHELL_CMD_PARAM *p_cmd_param);

/********************************************************************************************************
 ********************************************************************************************************
 *                                               LOCAL TABLES
 ********************************************************************************************************
 *******************************************************************************************************/

static SHELL_CMD ClkCmdTbl[] =
{
  { "clk_help", ClkCmd_Help },
  { "clk_set", ClkCmd_Set },
  { "clk_get", ClkCmd_Get },
  { "clk_get_day_of_yr", ClkCmd_GetDayOfYr },
  { 0, 0 }
};

/********************************************************************************************************
 ********************************************************************************************************
 *                                                   MACRO
 ********************************************************************************************************
 *******************************************************************************************************/

#define  CLK_CMD_OUT_MSG_CHK(out_val, cur_out_cnt, exit_fail_label)     do { \
    switch (out_val) {                                                       \
      case SHELL_OUT_RTN_CODE_CONN_CLOSED:                                   \
      case SHELL_OUT_ERR:                                                    \
        out_val = SHELL_EXEC_ERR;                                            \
        goto exit_fail_label;                                                \
                                                                             \
                                                                             \
      default:                                                               \
        cur_out_cnt += out_val;                                              \
        break;                                                               \
    }                                                                        \
} while (0)

/********************************************************************************************************
 ********************************************************************************************************
 *                                           GLOBAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                               ClkCmd_Init()
 *
 * @brief    Adds the Clk commands to Shell.
 *
 * @param    p_err   Pointer to the variable that will receive one of the following error code(s) from
 *                   this function:
 *                       - RTOS_ERR_NONE
 *                       - RTOS_ERR_OWNERSHIP
 *                       - RTOS_ERR_ALREADY_EXISTS
 *                       - RTOS_ERR_BLK_ALLOC_CALLBACK
 *                       - RTOS_ERR_SEG_OVF
 *                       - RTOS_ERR_OS_SCHED_LOCKED
 *                       - RTOS_ERR_NOT_AVAIL
 *                       - RTOS_ERR_POOL_EMPTY
 *                       - RTOS_ERR_WOULD_OVF
 *                       - RTOS_ERR_OS_OBJ_DEL
 *                       - RTOS_ERR_WOULD_BLOCK
 *                       - RTOS_ERR_INVALID_ARG
 *                       - RTOS_ERR_NO_MORE_RSRC
 *                       - RTOS_ERR_IS_OWNER
 *                       - RTOS_ERR_ABORT
 *                       - RTOS_ERR_TIMEOUT
 *******************************************************************************************************/
void ClkCmd_Init(RTOS_ERR *p_err)
{
  RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err,; );

  Shell_CmdTblAdd((CPU_CHAR *)"clk",
                  ClkCmdTbl,
                  p_err);
}

/********************************************************************************************************
 ********************************************************************************************************
 *                                           LOCAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                               ClkCmd_Help()
 *
 * @brief    Output the available commands.
 *
 * @param    argc            Count of the arguments supplied.
 *
 * @param    p_argv          Array of pointers to the strings which are those arguments.
 *
 * @param    out_fnct        Callback to a respond to the requester.
 *
 * @param    p_cmd_param     Pointer to additional information to pass to the command.
 *
 * @return   The number of positive data octets transmitted, if NO error(s).
 *               - SHELL_OUT_RTN_CODE_CONN_CLOSED, if implemented connection closed.
 *               - SHELL_OUT_ERR, otherwise.
 *******************************************************************************************************/
static CPU_INT16S ClkCmd_Help(CPU_INT16U      argc,
                              CPU_CHAR        *p_argv[],
                              SHELL_OUT_FNCT  out_fnct,
                              SHELL_CMD_PARAM *p_cmd_param)
{
  CPU_INT16S ret_val;

  PP_UNUSED_PARAM(argc);
  PP_UNUSED_PARAM(p_argv);

  ret_val = ClkCmd_OutputCmdTbl(ClkCmdTbl,
                                out_fnct,
                                p_cmd_param);

  return (ret_val);
}

/****************************************************************************************************//**
 *                                               ClkCmd_Set()
 *
 * @brief    Set the current date and time.
 *
 * @param    argc            Count of the arguments supplied.
 *
 * @param    p_argv          Array of pointers to the strings which are those arguments.
 *
 * @param    out_fnct        Callback to a respond to the requester.
 *
 * @param    p_cmd_param     Pointer to additional information to pass to the command.
 *
 * @return   The number of positive data octets transmitted, if NO error(s).
 *               - SHELL_OUT_RTN_CODE_CONN_CLOSED, if implemented connection closed.
 *               - SHELL_OUT_ERR, otherwise.
 *******************************************************************************************************/
static CPU_INT16S ClkCmd_Set(CPU_INT16U      argc,
                             CPU_CHAR        *p_argv[],
                             SHELL_OUT_FNCT  out_fnct,
                             SHELL_CMD_PARAM *p_cmd_param)
{
#if  (CLK_CFG_STR_CONV_EN == DEF_ENABLED)
  CPU_CHAR date_time_str[CLK_STR_FMT_MAX_LEN];
#endif
#if  (CLK_CFG_NTP_EN == DEF_ENABLED)
  CPU_INT32U ts_ntp_toset;
#endif
#if  (CLK_CFG_UNIX_EN == DEF_ENABLED)
  CLK_TS_SEC ts_unix_toset;
#endif
  CPU_INT16S           ret_val = 0u;
  CPU_INT16S           byte_out_cnt = 0u;
  CLK_YR               yr = 0u;
  CLK_MONTH            month = 0u;
  CLK_DAY              day = 0u;
  CLK_HR               hr = 0u;
  CLK_MIN              min = 0u;
  CLK_SEC              sec = 0u;
  CLK_TZ_SEC           tz_sec = 0u;
  CPU_SIZE_T           off_len = 0u;
  CPU_INT08S           off_hr = 0u;
  CPU_INT08S           off_min = 0u;
  CPU_CHAR             *p_next;
  CPU_CHAR             *p_next_tmp;
  CLK_CMD_ARG          cmd_arg;
  CLK_DATE_TIME        date_time;
  CPU_INT32U           off_ix;
  CLK_TS_SEC           ts_sec;
  CPU_BOOLEAN          success;
  CLK_CMD_PARSE_STATUS status;

  status = ClkCmd_CmdArgParse(argc,
                              p_argv,
                              &cmd_arg);
  switch (status) {
    case CLK_CMD_PARSE_STATUS_SUCCESS:
      break;

    case CLK_CMD_PARSE_STATUS_INVALID_ARG:
      ret_val = ClkCmd_OutputError((CPU_CHAR *)CLK_CMD_OUTPUT_ERR_CMD_ARG_INVALID,
                                   out_fnct,
                                   p_cmd_param);

      CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);
      goto exit_ok;

    case CLK_CMD_PARSE_STATUS_EMPTY:
    case CLK_CMD_PARSE_STATUS_HELP:
    default:
      ret_val = ClkCmd_OutputMsg(CLK_CMD_HELP_SET,
                                 DEF_YES,
                                 DEF_YES,
                                 DEF_NO,
                                 out_fnct,
                                 p_cmd_param);

      CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);

      ret_val = ClkCmd_OutputMsg(CLK_CMD_HELP_VALUE,
                                 DEF_YES,
                                 DEF_YES,
                                 DEF_NO,
                                 out_fnct,
                                 p_cmd_param);

      CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);

      ret_val = ClkCmd_OutputMsg(CLK_CMD_HELP_FORMAT,
                                 DEF_YES,
                                 DEF_YES,
                                 DEF_NO,
                                 out_fnct,
                                 p_cmd_param);

      CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);
      goto exit_ok;
  }

  switch (cmd_arg.TimeType) {
    case CLK_CMD_TIME_TYPE_NONE:
    case CLK_CMD_TIME_TYPE_DATETIME:
      //                                                           Parsing date.
      if (cmd_arg.DatePtr != DEF_NULL) {
        yr = (CLK_YR)   Str_ParseNbr_Int32U((const  CPU_CHAR *)cmd_arg.DatePtr,
                                            &p_next,
                                            10u);

        month = (CLK_MONTH)Str_ParseNbr_Int32U((const  CPU_CHAR *)&p_next[1],
                                               &p_next_tmp,
                                               10u);

        day = (CLK_DAY)  Str_ParseNbr_Int32U((const  CPU_CHAR *)&p_next_tmp[1],
                                             DEF_NULL,
                                             10u);
      } else {
        goto arg_invalid;
      }

      //                                                           Parsing time. (optional)
      if (cmd_arg.TimePtr != DEF_NULL) {
        hr = (CLK_HR) Str_ParseNbr_Int32U((const  CPU_CHAR *)cmd_arg.TimePtr,
                                          &p_next,
                                          10u);

        min = (CLK_MIN)Str_ParseNbr_Int32U((const  CPU_CHAR *)&p_next[1],
                                           &p_next_tmp,
                                           10u);

        sec = (CLK_SEC)Str_ParseNbr_Int32U((const  CPU_CHAR *)&p_next_tmp[1],
                                           DEF_NULL,
                                           10u);

        //                                                         Parsing timezone. (optional).
        if (cmd_arg.OffsetPtr != DEF_NULL) {
          off_len = Str_Len(cmd_arg.OffsetPtr);

          for (off_ix = 0u; off_ix < off_len; off_ix++) {
            if ((cmd_arg.OffsetPtr[off_ix] == ASCII_CHAR_HYPHEN_MINUS)
                || (cmd_arg.OffsetPtr[off_ix] == ASCII_CHAR_PLUS_SIGN)) {
              off_hr = (CPU_INT08S)Str_ParseNbr_Int32S((const  CPU_CHAR *)&cmd_arg.OffsetPtr[off_ix],
                                                       &p_next,
                                                       10u);

              off_min = (CPU_INT08S)Str_ParseNbr_Int32S((const  CPU_CHAR *)&p_next[1],
                                                        DEF_NULL,
                                                        10u);
              break;
            }
          }

          tz_sec = (CLK_TZ_SEC)((off_hr * (CPU_INT08S)CLK_CMD_NBR_MIN_PER_HR + off_min)
                                * (CPU_INT08S)CLK_CMD_NBR_SEC_PER_MIN);
        }
      }

      success = Clk_DateTimeMake(&date_time,                    // Make a datetime struct.
                                 yr,
                                 month,
                                 day,
                                 hr,
                                 min,
                                 sec,
                                 tz_sec);

      if (success == DEF_FAIL) {
        goto arg_invalid;
      }
      //                                                           Converting datetime to timestamp.
      success = Clk_DateTimeToTS(&ts_sec,
                                 &date_time);

      if (success == DEF_FAIL) {
        goto arg_invalid;
      }
      //                                                           Setting clock to timestamp.
      success = Clk_SetTS(ts_sec);

      if (success == DEF_FAIL) {
        goto arg_invalid;
      }
      break;

#if  (CLK_CFG_NTP_EN == DEF_ENABLED)
    case CLK_CMD_TIME_TYPE_NTP:
      ts_ntp_toset = Str_ParseNbr_Int32U(cmd_arg.DatePtr,
                                         DEF_NULL,
                                         10u);
      success = Clk_SetTS_NTP(ts_ntp_toset);

      if (success == DEF_FAIL) {
        goto arg_invalid;
      }
      break;
#endif

#if  (CLK_CFG_UNIX_EN == DEF_ENABLED)
    case CLK_CMD_TIME_TYPE_UNIX:
      ts_unix_toset = (CLK_TS_SEC)Str_ParseNbr_Int32U(cmd_arg.DatePtr,
                                                      DEF_NULL,
                                                      10u);
      success = Clk_SetTS_Unix(ts_unix_toset);

      if (success == DEF_FAIL) {
        goto arg_invalid;
      }
      break;
#endif

    default:
      goto arg_invalid;
  }

  success = Clk_GetDateTime(&date_time);
  if (success == DEF_FAIL) {
    goto arg_invalid;
  }
#if (CLK_CFG_STR_CONV_EN == DEF_ENABLED)
  success = Clk_DateTimeToStr(&date_time,
                              CLK_STR_FMT_YYYY_MM_DD_HH_MM_SS_UTC,
                              date_time_str,
                              CLK_STR_FMT_MAX_LEN);

  if (success == DEF_FAIL) {
    goto arg_invalid;
  }

  ret_val = ClkCmd_OutputMsg(date_time_str,
                             DEF_YES,
                             DEF_YES,
                             DEF_NO,
                             out_fnct,
                             p_cmd_param);

  CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);
  goto exit_ok;
#else
  goto conv_not_enabled;
#endif

#if  (CLK_CFG_STR_CONV_EN == DEF_DISABLED)
conv_not_enabled:
  ret_val = ClkCmd_OutputError((CPU_CHAR *)CLK_CMD_OUTPUT_ERR_CONV_DISABLED,
                               out_fnct,
                               p_cmd_param);

  CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);
  goto exit_ok;
#endif

arg_invalid:
  ret_val = ClkCmd_OutputError((CPU_CHAR *)CLK_CMD_OUTPUT_ERR_CMD_ARG_INVALID,
                               out_fnct,
                               p_cmd_param);

  CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);
  goto exit_ok;

exit_ok:
  ret_val = byte_out_cnt;

exit_fail:
  return (ret_val);
}

/****************************************************************************************************//**
 *                                               ClkCmd_Get()
 *
 * @brief    Manage a clock get command according to the specified format.
 *
 * @param    argc            Count of the arguments supplied.
 *
 * @param    p_argv          Array of pointers to the strings which are those arguments.
 *
 * @param    out_fnct        Callback to a respond to the requester.
 *
 * @param    p_cmd_param     Pointer to additional information to pass to the command.
 *
 * @return   The number of positive data octets transmitted, if NO error(s).
 *               - SHELL_OUT_RTN_CODE_CONN_CLOSED, if implemented connection closed.
 *               - SHELL_OUT_ERR, otherwise.
 *******************************************************************************************************/
static CPU_INT16S ClkCmd_Get(CPU_INT16U      argc,
                             CPU_CHAR        *p_argv[],
                             SHELL_OUT_FNCT  out_fnct,
                             SHELL_CMD_PARAM *p_cmd_param)
{
#if  (CLK_CFG_STR_CONV_EN == DEF_ENABLED)
  CPU_CHAR date_time_str[CLK_STR_FMT_MAX_LEN];
#endif
#if  (CLK_CFG_UNIX_EN == DEF_ENABLED)
  CLK_TS_SEC ts_unix_sec;
#endif
#if  (CLK_CFG_NTP_EN == DEF_ENABLED)
  CLK_TS_SEC ts_ntp_sec;
#endif
#if ((CLK_CFG_STR_CONV_EN == DEF_ENABLED) \
  || (CLK_CFG_UNIX_EN == DEF_ENABLED)     \
  || (CLK_CFG_NTP_EN == DEF_ENABLED))
  CPU_CHAR str[DEF_INT_32U_NBR_DIG_MAX + 1];
#endif
  CPU_INT16S           ret_val = 0u;
  CPU_INT16S           byte_out_cnt = 0u;
  CLK_CMD_ARG          cmd_arg;
  CLK_DATE_TIME        date_time;
  CPU_BOOLEAN          success;
  CLK_CMD_PARSE_STATUS parse_status;

  parse_status = ClkCmd_CmdArgParse(argc,
                                    p_argv,
                                    &cmd_arg);

  switch (parse_status) {
    case CLK_CMD_PARSE_STATUS_EMPTY:
    case CLK_CMD_PARSE_STATUS_SUCCESS:
      break;

    case CLK_CMD_PARSE_STATUS_INVALID_ARG:
      ret_val = ClkCmd_OutputError((CPU_CHAR *)CLK_CMD_OUTPUT_ERR_CMD_ARG_INVALID,
                                   out_fnct,
                                   p_cmd_param);

      CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);
      goto exit_ok;

    case CLK_CMD_PARSE_STATUS_HELP:
    default:
      ret_val = ClkCmd_OutputMsg(CLK_CMD_HELP_GET,
                                 DEF_YES,
                                 DEF_YES,
                                 DEF_NO,
                                 out_fnct,
                                 p_cmd_param);

      CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);

      ret_val = ClkCmd_OutputMsg(CLK_CMD_HELP_FORMAT,
                                 DEF_YES,
                                 DEF_YES,
                                 DEF_NO,
                                 out_fnct,
                                 p_cmd_param);

      CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);
      goto exit_ok;
  }

  switch (cmd_arg.TimeType) {
    case CLK_CMD_TIME_TYPE_NONE:
    case CLK_CMD_TIME_TYPE_DATETIME:
      success = Clk_GetDateTime(&date_time);
      if (success == DEF_FAIL) {
        goto date_time_invalid;
      }

#if (CLK_CFG_STR_CONV_EN == DEF_ENABLED)
      success = Clk_DateTimeToStr(&date_time,
                                  CLK_STR_FMT_YYYY_MM_DD_HH_MM_SS_UTC,
                                  date_time_str,
                                  CLK_STR_FMT_MAX_LEN);
      if (success == DEF_FAIL) {
        goto date_time_invalid;
      }

      ret_val = ClkCmd_OutputMsg(date_time_str,
                                 DEF_YES,
                                 DEF_YES,
                                 DEF_NO,
                                 out_fnct,
                                 p_cmd_param);
      CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);

      (void)Str_FmtNbr_Int32U(date_time.DayOfWk,
                              DEF_INT_32U_NBR_DIG_MAX,
                              10u,
                              '\0',
                              DEF_NO,
                              DEF_YES,
                              str);

      ret_val = ClkCmd_OutputMsg(str,
                                 DEF_YES,
                                 DEF_YES,
                                 DEF_NO,
                                 out_fnct,
                                 p_cmd_param);
      CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);

      (void)Str_FmtNbr_Int32U(date_time.DayOfYr,
                              DEF_INT_32U_NBR_DIG_MAX,
                              10u,
                              '\0',
                              DEF_NO,
                              DEF_YES,
                              str);

      ret_val = ClkCmd_OutputMsg(str,
                                 DEF_YES,
                                 DEF_YES,
                                 DEF_NO,
                                 out_fnct,
                                 p_cmd_param);
      CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);
      break;
#else
      goto conv_not_enabled;
#endif

#if  (CLK_CFG_NTP_EN == DEF_ENABLED)
    case CLK_CMD_TIME_TYPE_NTP:
      success = Clk_GetTS_NTP(&ts_ntp_sec);
      if (success == DEF_FAIL) {
        goto date_time_invalid;
      }

      (void)Str_FmtNbr_Int32U(ts_ntp_sec,
                              DEF_INT_32U_NBR_DIG_MAX,
                              10u,
                              '\0',
                              DEF_NO,
                              DEF_YES,
                              str);

      ret_val = ClkCmd_OutputMsg(str,
                                 DEF_YES,
                                 DEF_YES,
                                 DEF_NO,
                                 out_fnct,
                                 p_cmd_param);

      CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);
      break;
#endif

#if  (CLK_CFG_UNIX_EN == DEF_ENABLED)
    case CLK_CMD_TIME_TYPE_UNIX:
      success = Clk_GetTS_Unix(&ts_unix_sec);
      if (success == DEF_FAIL) {
        goto date_time_invalid;
      }

      (void)Str_FmtNbr_Int32U(ts_unix_sec,
                              DEF_INT_32U_NBR_DIG_MAX,
                              10u,
                              '\0',
                              DEF_NO,
                              DEF_YES,
                              str);

      ret_val = ClkCmd_OutputMsg(str,
                                 DEF_YES,
                                 DEF_YES,
                                 DEF_NO,
                                 out_fnct,
                                 p_cmd_param);

      CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);
      break;
#endif

    default:
      ret_val = ClkCmd_OutputError((CPU_CHAR *)CLK_CMD_OUTPUT_ERR_CMD_ARG_INVALID,
                                   out_fnct,
                                   p_cmd_param);

      CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);
  }
  goto exit_ok;

#if  (CLK_CFG_STR_CONV_EN == DEF_DISABLED)
conv_not_enabled:
  ret_val = ClkCmd_OutputError((CPU_CHAR *)CLK_CMD_OUTPUT_ERR_CONV_DISABLED,
                               out_fnct,
                               p_cmd_param);

  CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);
  goto exit_ok;
#endif

date_time_invalid:
  ret_val = ClkCmd_OutputError((CPU_CHAR *)CLK_CMD_OUTPUT_ERR_CMD_INTERNAL_ERR,
                               out_fnct,
                               p_cmd_param);

  CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);
  goto exit_ok;

exit_ok:
  ret_val = byte_out_cnt;

exit_fail:
  return (ret_val);
}

/****************************************************************************************************//**
 *                                           ClkCmd_GetDayOfYr()
 *
 * @brief    Manage a clock get command according to the specified format.
 *
 * @param    argc            Count of the arguments supplied.
 *
 * @param    p_argv          Array of pointers to the strings which are those arguments.
 *
 * @param    out_fnct        Callback to a respond to the requester.
 *
 * @param    p_cmd_param     Pointer to additional information to pass to the command.
 *
 * @return   The number of positive data octets transmitted, if NO error(s).
 *               - SHELL_OUT_RTN_CODE_CONN_CLOSED, if implemented connection closed.
 *               - SHELL_OUT_ERR, otherwise.
 *******************************************************************************************************/
static CPU_INT16S ClkCmd_GetDayOfYr(CPU_INT16U      argc,
                                    CPU_CHAR        *p_argv[],
                                    SHELL_OUT_FNCT  out_fnct,
                                    SHELL_CMD_PARAM *p_cmd_param)
{
  CPU_CHAR             str_day_of_yr[4] = { 0 };
  CPU_INT16S           ret_val = 0u;
  CPU_INT16S           byte_out_cnt = 0u;
  CLK_YR               yr = 0u;
  CLK_MONTH            month = 0u;
  CLK_DAY              day = 0u;
  CLK_DAY              day_of_yr = 0u;
  CPU_CHAR             *p_next;
  CPU_CHAR             *p_next_tmp;
  CLK_CMD_ARG          cmd_arg;
  CLK_CMD_PARSE_STATUS status;
#if ((CLK_CFG_UNIX_EN == DEF_ENABLED) \
  || (CLK_CFG_NTP_EN == DEF_ENABLED))
  CLK_TS_SEC    ts;
  CLK_DATE_TIME date;
  CPU_BOOLEAN   success;
#endif

  status = ClkCmd_CmdArgParse(argc,
                              p_argv,
                              &cmd_arg);
  switch (status) {
    case CLK_CMD_PARSE_STATUS_SUCCESS:
      break;

    case CLK_CMD_PARSE_STATUS_INVALID_ARG:
      ret_val = ClkCmd_OutputError((CPU_CHAR *)CLK_CMD_OUTPUT_ERR_CMD_ARG_INVALID,
                                   out_fnct,
                                   p_cmd_param);

      CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);
      goto exit_ok;

    case CLK_CMD_PARSE_STATUS_EMPTY:
    case CLK_CMD_PARSE_STATUS_HELP:
    default:
      ret_val = ClkCmd_OutputMsg(CLK_CMD_HELP_SET,
                                 DEF_YES,
                                 DEF_YES,
                                 DEF_NO,
                                 out_fnct,
                                 p_cmd_param);

      CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);

      ret_val = ClkCmd_OutputMsg(CLK_CMD_HELP_VALUE,
                                 DEF_YES,
                                 DEF_YES,
                                 DEF_NO,
                                 out_fnct,
                                 p_cmd_param);

      CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);

      ret_val = ClkCmd_OutputMsg(CLK_CMD_HELP_FORMAT,
                                 DEF_YES,
                                 DEF_YES,
                                 DEF_NO,
                                 out_fnct,
                                 p_cmd_param);

      CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);
      goto exit_ok;
  }

  switch (cmd_arg.TimeType) {
    case CLK_CMD_TIME_TYPE_NONE:
    case CLK_CMD_TIME_TYPE_DATETIME:
      //                                                           Parsing date.
      if (cmd_arg.DatePtr != DEF_NULL) {
        yr = (CLK_YR)   Str_ParseNbr_Int32U((const  CPU_CHAR *)cmd_arg.DatePtr,
                                            &p_next,
                                            10u);

        month = (CLK_MONTH)Str_ParseNbr_Int32U((const  CPU_CHAR *)&p_next[1],
                                               &p_next_tmp,
                                               10u);

        day = (CLK_DAY)  Str_ParseNbr_Int32U((const  CPU_CHAR *)&p_next_tmp[1],
                                             DEF_NULL,
                                             10u);
      } else {
        goto arg_invalid;
      }

      day_of_yr = Clk_GetDayOfYr(yr, month, day);

      break;

#if  (CLK_CFG_NTP_EN == DEF_ENABLED)
    case CLK_CMD_TIME_TYPE_NTP:
      ts = Str_ParseNbr_Int32U(cmd_arg.DatePtr, DEF_NULL, 10u);

      success = Clk_TS_NTP_ToDateTime(ts, 0, &date);
      if (success == DEF_FAIL) {
        goto arg_invalid;
      }
      day_of_yr = date.DayOfYr;
      break;
#endif

#if  (CLK_CFG_UNIX_EN == DEF_ENABLED)
    case CLK_CMD_TIME_TYPE_UNIX:
      ts = Str_ParseNbr_Int32U(cmd_arg.DatePtr, DEF_NULL, 10u);
      success = Clk_TS_UnixToDateTime(ts, 0, &date);
      if (success == DEF_FAIL) {
        goto arg_invalid;
      }
      day_of_yr = date.DayOfYr;
      break;
#endif

    default:
      goto arg_invalid;
  }

  Str_FmtNbr_Int32U(day_of_yr,
                    4,
                    10u,
                    '\0',
                    DEF_NO,
                    DEF_YES,
                    str_day_of_yr);

  ret_val = ClkCmd_OutputMsg(str_day_of_yr,
                             DEF_YES,
                             DEF_YES,
                             DEF_NO,
                             out_fnct,
                             p_cmd_param);
  CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);
  goto exit_ok;

arg_invalid:
  ret_val = ClkCmd_OutputError((CPU_CHAR *)CLK_CMD_OUTPUT_ERR_CMD_ARG_INVALID,
                               out_fnct,
                               p_cmd_param);

  CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);
  goto exit_ok;

exit_ok:
  ret_val = byte_out_cnt;

exit_fail:
  return (ret_val);
}

/****************************************************************************************************//**
 *                                           ClkCmd_CmdArgParse()
 *
 * @brief    Parse and validate the argument for a clock test command.
 *
 * @param    argc        Count of the arguments supplied.
 *
 * @param    p_argv      Array of pointers to the strings which are those arguments.
 *
 * @param    p_cmd_args  Pointer to structure that will be filled during parse operation.
 *
 * @return   Clock command parse status:
 *               - CLK_CMD_PARSE_STATUS_SUCCESS
 *               - CLK_CMD_PARSE_STATUS_EMPTY
 *               - CLK_CMD_PARSE_STATUS_INVALID_ARG
 *               - CLK_CMD_PARSE_STATUS_HELP
 *
 * @note
 *******************************************************************************************************/
static CLK_CMD_PARSE_STATUS ClkCmd_CmdArgParse(CPU_INT16U  argc,
                                               CPU_CHAR    *p_argv[],
                                               CLK_CMD_ARG *p_cmd_args)
{
  CPU_INT16U i;
  CPU_INT16U arg_caught = 0u;

  p_cmd_args->TimeType = CLK_CMD_TIME_TYPE_NONE;
  p_cmd_args->DatePtr = DEF_NULL;
  p_cmd_args->TimePtr = DEF_NULL;
  p_cmd_args->OffsetPtr = DEF_NULL;

  if (argc == 1) {
    return (CLK_CMD_PARSE_STATUS_EMPTY);
  }

  for (i = 1u; i < argc; i++) {
    if (*p_argv[i] == CLK_CMD_ARG_BEGIN) {
      if (*(p_argv[i] + 1) == CLK_CMD_ARG_BEGIN) {              // --option type argument.
        if (Str_Cmp(p_argv[i] + 2, CLK_CMD_FORMAT_DATETIME) == 0) {
          p_cmd_args->TimeType = CLK_CMD_TIME_TYPE_DATETIME;
        } else if (Str_Cmp(p_argv[i] + 2, CLK_CMD_FORMAT_NTP) == 0) {
          p_cmd_args->TimeType = CLK_CMD_TIME_TYPE_NTP;
        } else if (Str_Cmp(p_argv[i] + 2, CLK_CMD_FORMAT_UNIX) == 0) {
          p_cmd_args->TimeType = CLK_CMD_TIME_TYPE_UNIX;
        } else if (Str_Cmp(p_argv[i] + 2, CLK_CMD_HELP) == 0) {
          return (CLK_CMD_PARSE_STATUS_HELP);
        } else {
          return (CLK_CMD_PARSE_STATUS_INVALID_ARG);
        }
      } else {                                                  // -o type argument.
        if (Str_Cmp(p_argv[i] + 1, CLK_CMD_FORMAT_DATETIME_SHORT) == 0) {
          p_cmd_args->TimeType = CLK_CMD_TIME_TYPE_DATETIME;
        } else if (Str_Cmp(p_argv[i] + 1, CLK_CMD_FORMAT_NTP_SHORT) == 0) {
          p_cmd_args->TimeType = CLK_CMD_TIME_TYPE_NTP;
        } else if (Str_Cmp(p_argv[i] + 1, CLK_CMD_FORMAT_UNIX_SHORT) == 0) {
          p_cmd_args->TimeType = CLK_CMD_TIME_TYPE_UNIX;
        } else if (Str_Cmp(p_argv[i] + 1, CLK_CMD_HELP_SHORT) == 0) {
          return (CLK_CMD_PARSE_STATUS_HELP);
        } else {
          return (CLK_CMD_PARSE_STATUS_INVALID_ARG);
        }
      }
    } else {
      switch (arg_caught) {
        case 0:
          p_cmd_args->DatePtr = p_argv[i];
          break;

        case 1:
          p_cmd_args->TimePtr = p_argv[i];
          break;

        case 2:
          p_cmd_args->OffsetPtr = p_argv[i];
          break;

        default:
          break;
      }

      arg_caught++;
    }
  }

  return (CLK_CMD_PARSE_STATUS_SUCCESS);
}

/****************************************************************************************************//**
 *                                           ClkCmd_OutputCmdTbl()
 *
 * @brief    Format and output the clock test command table
 *
 * @param    p_cmd_tbl       is the pointer on the pointer table
 *
 * @param    out_fnct        is a callback to a respond to the requester.
 *
 * @param    p_cmd_param     is a pointer to additional information to pass to the command.
 *
 * @return   The number of positive data octets transmitted, if NO error(s).
 *               - SHELL_OUT_RTN_CODE_CONN_CLOSED, if implemented connection closed.
 *               - SHELL_OUT_ERR, otherwise.
 *******************************************************************************************************/
static CPU_INT16S ClkCmd_OutputCmdTbl(SHELL_CMD       *p_cmd_tbl,
                                      SHELL_OUT_FNCT  out_fnct,
                                      SHELL_CMD_PARAM *p_cmd_param)
{
  SHELL_CMD  *p_shell_cmd;
  CPU_INT16S ret_val;
  CPU_INT16S acc_ret_val;

  ret_val = ClkCmd_OutputMsg(CLK_CMD_OUTPUT_CMD_LIST,
                             DEF_YES,
                             DEF_YES,
                             DEF_NO,
                             out_fnct,
                             p_cmd_param);
  switch (ret_val) {
    case SHELL_OUT_RTN_CODE_CONN_CLOSED:
    case SHELL_OUT_ERR:
      return (SHELL_EXEC_ERR);

    default:
      break;
  }

  acc_ret_val = ret_val;
  p_shell_cmd = p_cmd_tbl;

  while (p_shell_cmd->Fnct != 0) {
    ret_val = ClkCmd_OutputMsg(p_shell_cmd->Name,
                               DEF_NO,
                               DEF_YES,
                               DEF_YES,
                               out_fnct,
                               p_cmd_param);
    switch (ret_val) {
      case SHELL_OUT_RTN_CODE_CONN_CLOSED:
      case SHELL_OUT_ERR:
        return (SHELL_EXEC_ERR);

      default:
        break;
    }
    p_shell_cmd++;
    acc_ret_val += ret_val;
  }

  return (acc_ret_val);
}

/****************************************************************************************************//**
 *                                           ClkCmd_OutputError()
 *
 * @brief    Outputs error message.
 *
 * @param    p_error         Pointer to a string describing the error.
 *
 * @param    out_fnct        Callback to a respond to the requester.
 *
 * @param    p_cmd_param     Pointer to additional information to pass to the command.
 *
 * @return   The number of positive data octets transmitted, if NO error(s).
 *               - SHELL_OUT_RTN_CODE_CONN_CLOSED, if implemented connection closed.
 *               - SHELL_OUT_ERR, otherwise.
 *******************************************************************************************************/
static CPU_INT16S ClkCmd_OutputError(CPU_CHAR        *p_error,
                                     SHELL_OUT_FNCT  out_fnct,
                                     SHELL_CMD_PARAM *p_cmd_param)
{
  CPU_INT16S ret_val;
  CPU_INT16S byte_out_cnt = 0;

  ret_val = ClkCmd_OutputMsg(CLK_CMD_OUTPUT_ERR,
                             DEF_YES,
                             DEF_NO,
                             DEF_NO,
                             out_fnct,
                             p_cmd_param);

  CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);

  ret_val = ClkCmd_OutputMsg(p_error,
                             DEF_NO,
                             DEF_YES,
                             DEF_NO,
                             out_fnct,
                             p_cmd_param);

  CLK_CMD_OUT_MSG_CHK(ret_val, byte_out_cnt, exit_fail);

  ret_val = byte_out_cnt;

exit_fail:
  return (ret_val);
}

/****************************************************************************************************//**
 *                                           ClkCmd_OutputMsg()
 *
 * @brief    Format and output a message.
 *
 * @param    p_msg           Pointer of char on the string to format and output.
 *
 * @param    new_line_start  If DEF_YES, will add a new line character at the start.
 *
 * @param    new_line_end    If DEF_YES, will add a new line character at the end.
 *
 * @param    tab_start       If DEF_YES, will add a tab character at the start.
 *
 * @param    out_fnct        Callback to a respond to the requester.
 *
 * @param    p_cmd_param     Pointer to additional information to pass to the command.
 *
 * @return   The number of positive data octets transmitted, if NO error(s).
 *               - SHELL_OUT_RTN_CODE_CONN_CLOSED, if implemented connection closed.
 *               - SHELL_OUT_ERR, otherwise.
 *******************************************************************************************************/
static CPU_INT16S ClkCmd_OutputMsg(const CPU_CHAR  *p_msg,
                                   CPU_BOOLEAN     new_line_start,
                                   CPU_BOOLEAN     new_line_end,
                                   CPU_BOOLEAN     tab_start,
                                   SHELL_OUT_FNCT  out_fnct,
                                   SHELL_CMD_PARAM *p_cmd_param)
{
  CPU_INT16U output_len;
  CPU_INT16S output;
  CPU_INT16S byte_out_cnt = 0;

  if (new_line_start == DEF_YES) {
    output = out_fnct((CPU_CHAR *)STR_NEW_LINE,
                      STR_NEW_LINE_LEN,
                      p_cmd_param->OutputOptPtr);

    CLK_CMD_OUT_MSG_CHK(output, byte_out_cnt, exit_fail);
  }

  if (tab_start == DEF_YES) {
    output = out_fnct((CPU_CHAR *)CLK_CMD_OUTPUT_TABULATION,
                      1,
                      p_cmd_param->OutputOptPtr);

    CLK_CMD_OUT_MSG_CHK(output, byte_out_cnt, exit_fail);
  }

  output_len = (CPU_INT16U)Str_Len(p_msg);
  output = out_fnct((CPU_CHAR *)p_msg,
                    output_len,
                    p_cmd_param->OutputOptPtr);

  CLK_CMD_OUT_MSG_CHK(output, byte_out_cnt, exit_fail);

  if (new_line_end == DEF_YES) {
    output = out_fnct((CPU_CHAR *)STR_NEW_LINE,
                      STR_NEW_LINE_LEN,
                      p_cmd_param->OutputOptPtr);

    CLK_CMD_OUT_MSG_CHK(output, byte_out_cnt, exit_fail);
  }

  output = byte_out_cnt;

exit_fail:
  return (output);
}

#endif // #ifdef  RTOS_MODULE_COMMON_SHELL_AVAIL
