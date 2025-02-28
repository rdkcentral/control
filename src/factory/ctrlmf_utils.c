#include <stdlib.h>
#include <stdio.h>
#include <ctrlmf_utils.h>

#define CTRLMF_INVALID_STR_LEN (24)

static char ctrlmf_invalid_str[CTRLMF_INVALID_STR_LEN];

static const char *ctrlmf_invalid_return(int value);

const char *ctrlmf_invalid_return(int value) {
   snprintf(ctrlmf_invalid_str, sizeof(ctrlmf_invalid_str), "INVALID(%d)", value);
   ctrlmf_invalid_str[sizeof(ctrlmf_invalid_str) - 1] = '\0';
   return(ctrlmf_invalid_str);
}

const char *ctrlm_iarm_call_result_str(ctrlm_iarm_call_result_t value) {
   switch(value) {
      case CTRLM_IARM_CALL_RESULT_SUCCESS:                 { return("SUCCESS"); }
      case CTRLM_IARM_CALL_RESULT_ERROR:                   { return("ERROR"); }
      case CTRLM_IARM_CALL_RESULT_ERROR_READ_ONLY:         { return("READ_ONLY"); }
      case CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER: { return("INVALID_PARAMETER"); }
      case CTRLM_IARM_CALL_RESULT_ERROR_API_REVISION:      { return("API_REVISION"); }
      case CTRLM_IARM_CALL_RESULT_ERROR_NOT_SUPPORTED:     { return("NOT_SUPPORTED"); }
      case CTRLM_IARM_CALL_RESULT_INVALID:                 { return("INVALID"); }
      default: {
         return(ctrlmf_invalid_return(value));
      }
   }
}

const char *iarm_result_str(IARM_Result_t value) {
   switch(value) {
      case IARM_RESULT_SUCCESS:       { return("SUCCESS"); }
      case IARM_RESULT_INVALID_PARAM: { return("INVALID_PARAM"); }
      case IARM_RESULT_INVALID_STATE: { return("INVALID_STATE"); }
      case IARM_RESULT_IPCCORE_FAIL:  { return("IPCCORE_FAIL"); }
      case IARM_RESULT_OOM:           { return("OOM"); }
      default: {
         return(ctrlmf_invalid_return(value));
      }
   }
}