/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>
#include <secure_wrapper.h>
#include "ctrlm.h"
#include "ctrlm_log.h"
#include "ctrlm_ipc_voice.h"
#include "ctrlm_shared_memory.h"
#include "ctrlm_config_default.h"

#define CTRLM_SHARED_MEMORY_NAME "/ctrlm_shared_memory"

#define CTRLM_VOICE_INIT_BLOB_MAX_LENGTH (16 * 1024)

typedef struct {
   char key[CTRLM_VOICE_QUERY_STRING_MAX_LENGTH];
   char value[CTRLM_VOICE_QUERY_STRING_MAX_LENGTH];
} query_string_pair_t;

typedef struct {
   uint32_t crash_count;
   uint32_t invalid_ctrlm_db;
   uint32_t invalid_hal_nvm;
   bool     sat_enabled;
   bool     mtls_enabled;
   bool     secure_url_required;
   bool     url_ptt_valid;
   char     url_ptt[CTRLM_VOICE_SERVER_URL_MAX_LENGTH];
   bool     url_ff_valid;
   char     url_ff[CTRLM_VOICE_SERVER_URL_MAX_LENGTH];
   #ifdef CTRLM_LOCAL_MIC_TAP
   bool     url_mic_tap_valid;
   char     url_mic_tap[CTRLM_VOICE_SERVER_URL_MAX_LENGTH];
   #endif

   uint32_t            query_string_ptt_count;
   query_string_pair_t query_string_ptt_pairs[CTRLM_VOICE_QUERY_STRING_MAX_PAIRS];

   bool init_blob_valid;
   char init_blob[CTRLM_VOICE_INIT_BLOB_MAX_LENGTH];
} ctrlm_shared_memory_t;

typedef struct {
   int                    fd;
   ctrlm_shared_memory_t *mmap;
} ctrlm_sm_global_t;

static ctrlm_sm_global_t g_ctrlm_sm = { -1, NULL };

bool ctrlm_sm_init() {
   bool created = false;

   if(g_ctrlm_sm.fd < 0) {
      errno = 0;
      g_ctrlm_sm.fd = shm_open(CTRLM_SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);

      if(g_ctrlm_sm.fd < 0) {
         int errsv = errno;
         XLOGD_ERROR("Unable to open shared memory <%s>", strerror(errsv));
         return(false);
      }
   }

   struct stat buf;
   errno = 0;
   if(fstat(g_ctrlm_sm.fd, &buf)) {
      int errsv = errno;
      XLOGD_ERROR("Unable to stat shared memory <%s>", strerror(errsv));
      ctrlm_sm_term(true);
      return(false);
   }

   if(0 == buf.st_size) {
      XLOGD_INFO("Created shared memory region");
      errno = 0;
      if(ftruncate(g_ctrlm_sm.fd, sizeof(ctrlm_shared_memory_t)) < 0) {
         int errsv = errno;
         XLOGD_ERROR("Unable to set shared memory size %d <%s>", g_ctrlm_sm.fd, strerror(errsv));
         ctrlm_sm_term(true);
         return(false);
      }
      created = true;
   }

   if(NULL == g_ctrlm_sm.mmap) {
      errno = 0;
      g_ctrlm_sm.mmap = (ctrlm_shared_memory_t *)mmap(NULL, sizeof(ctrlm_shared_memory_t), (PROT_READ | PROT_WRITE), MAP_SHARED, g_ctrlm_sm.fd, 0);

      if(NULL == g_ctrlm_sm.mmap) {
         int errsv = errno;
         XLOGD_ERROR("unable to map shared memory %d <%s>", g_ctrlm_sm.fd, strerror(errsv));
         ctrlm_sm_term(true);
         return(false);
      }

      if(created) {
         errno_t safec_rc = memset_s(g_ctrlm_sm.mmap, sizeof(ctrlm_shared_memory_t), 0, sizeof(ctrlm_shared_memory_t));
         ERR_CHK(safec_rc);

         // Set initial values
         g_ctrlm_sm.mmap->sat_enabled         = JSON_BOOL_VALUE_VOICE_ENABLE_SAT;
         g_ctrlm_sm.mmap->mtls_enabled        = JSON_BOOL_VALUE_VOICE_ENABLE_MTLS;
         g_ctrlm_sm.mmap->secure_url_required = JSON_BOOL_VALUE_VOICE_REQUIRE_SECURE_URL;
      }
   }

   XLOGD_INFO("Shared memory open successful");

   return(true);
}

void ctrlm_sm_term(bool unlink) {
   if(NULL != g_ctrlm_sm.mmap) {
      errno = 0;
      if(munmap(g_ctrlm_sm.mmap, sizeof(ctrlm_shared_memory_t)) < 0) {
         int errsv = errno;
         XLOGD_WARN("unable to unmap shared memory %d <%s>", g_ctrlm_sm.fd, strerror(errsv));
      }
      g_ctrlm_sm.mmap = NULL;
   }

   if(g_ctrlm_sm.fd > 0) {
      errno = 0;
      if(close(g_ctrlm_sm.fd) < 0) {
         int errsv = errno;
         XLOGD_WARN("unable to close shared memory %d <%s>", g_ctrlm_sm.fd, strerror(errsv));
      }
      g_ctrlm_sm.fd = -1;
   }

   if(unlink) {
      errno = 0;
      if(shm_unlink(CTRLM_SHARED_MEMORY_NAME) < 0) {
         int errsv = errno;
         XLOGD_ERROR("unable to unlink shared memory <%s>", strerror(errsv));
      }
   }
}

void ctrlm_sm_recovery_crash_count_read(uint32_t *value) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   if(value == NULL) {
      XLOGD_ERROR("invalid params");
      return;
   }
   *value = g_ctrlm_sm.mmap->crash_count;
}

void ctrlm_sm_recovery_crash_count_write(uint32_t value) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   g_ctrlm_sm.mmap->crash_count = value;
}

void ctrlm_sm_recovery_invalid_hal_nvm_read(uint32_t *value) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   if(value == NULL) {
      XLOGD_ERROR("invalid params");
      return;
   }
   *value = g_ctrlm_sm.mmap->invalid_hal_nvm;
}

void ctrlm_sm_recovery_invalid_hal_nvm_write(uint32_t value) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   g_ctrlm_sm.mmap->invalid_hal_nvm = value;
}

void ctrlm_sm_recovery_invalid_ctrlm_db_read(uint32_t *value) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   if(value == NULL) {
      XLOGD_ERROR("invalid params");
      return;
   }
   *value = g_ctrlm_sm.mmap->invalid_ctrlm_db;
}

void ctrlm_sm_recovery_invalid_ctrlm_db_write(uint32_t value) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   g_ctrlm_sm.mmap->invalid_ctrlm_db = value;
}

void ctrlm_sm_voice_sat_enable_read(bool &value) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   value = g_ctrlm_sm.mmap->sat_enabled;
}

void ctrlm_sm_voice_sat_enable_write(bool value) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   g_ctrlm_sm.mmap->sat_enabled = value;
}

void ctrlm_sm_voice_mtls_enable_read(bool &value) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   value = g_ctrlm_sm.mmap->mtls_enabled;
}

void ctrlm_sm_voice_mtls_enable_write(bool value) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   g_ctrlm_sm.mmap->mtls_enabled = value;
}

void ctrlm_sm_voice_secure_url_required_read(bool &value) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   value = g_ctrlm_sm.mmap->secure_url_required;
}

void ctrlm_sm_voice_secure_url_required_write(bool value) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   g_ctrlm_sm.mmap->secure_url_required = value;
}

void ctrlm_sm_voice_url_ptt_read(std::string &url) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   if(g_ctrlm_sm.mmap->url_ptt_valid) {
      url = g_ctrlm_sm.mmap->url_ptt;
   }
}

void ctrlm_sm_voice_url_ptt_write(std::string url) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   errno_t safec_rc = strncpy_s(g_ctrlm_sm.mmap->url_ptt, sizeof(g_ctrlm_sm.mmap->url_ptt), url.c_str(), sizeof(g_ctrlm_sm.mmap->url_ptt) - 1);
   ERR_CHK(safec_rc);
   g_ctrlm_sm.mmap->url_ptt[sizeof(g_ctrlm_sm.mmap->url_ptt) - 1] = '\0';
   g_ctrlm_sm.mmap->url_ptt_valid = true;
}

void ctrlm_sm_voice_url_ff_read(std::string &url) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   if(g_ctrlm_sm.mmap->url_ff_valid) {
      url = g_ctrlm_sm.mmap->url_ff;
   }
}

void ctrlm_sm_voice_url_ff_write(std::string url) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   errno_t safec_rc = strncpy_s(g_ctrlm_sm.mmap->url_ff, sizeof(g_ctrlm_sm.mmap->url_ff), url.c_str(), sizeof(g_ctrlm_sm.mmap->url_ff) - 1);
   ERR_CHK(safec_rc);
   g_ctrlm_sm.mmap->url_ff[sizeof(g_ctrlm_sm.mmap->url_ff) - 1] = '\0';
   g_ctrlm_sm.mmap->url_ff_valid = true;
}

#ifdef CTRLM_LOCAL_MIC_TAP
void ctrlm_sm_voice_url_mic_tap_read(std::string &url) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   if(g_ctrlm_sm.mmap->url_mic_tap_valid) {
      url = g_ctrlm_sm.mmap->url_mic_tap;
   }
}

void ctrlm_sm_voice_url_mic_tap_write(std::string url) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   errno_t safec_rc = strncpy_s(g_ctrlm_sm.mmap->url_mic_tap, sizeof(g_ctrlm_sm.mmap->url_mic_tap), url.c_str(), sizeof(g_ctrlm_sm.mmap->url_mic_tap) - 1);
   ERR_CHK(safec_rc);
   g_ctrlm_sm.mmap->url_mic_tap[sizeof(g_ctrlm_sm.mmap->url_mic_tap) - 1] = '\0';
   g_ctrlm_sm.mmap->url_mic_tap_valid = true;
}
#endif

void ctrlm_sm_voice_query_string_ptt_count_read(uint8_t &count) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   count = g_ctrlm_sm.mmap->query_string_ptt_count;
}

void ctrlm_sm_voice_query_string_ptt_count_write(uint8_t count) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   g_ctrlm_sm.mmap->query_string_ptt_count = count;
}

void ctrlm_sm_voice_query_string_ptt_read(unsigned int index, std::string &key, std::string &value) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   if(index >= g_ctrlm_sm.mmap->query_string_ptt_count) {
      XLOGD_ERROR("index out of range <%u> <%u>", index, g_ctrlm_sm.mmap->query_string_ptt_count);
      return;
   }
   query_string_pair_t *pair = &g_ctrlm_sm.mmap->query_string_ptt_pairs[index];
   key   = pair->key;
   value = pair->value;
}

void ctrlm_sm_voice_query_string_ptt_write(unsigned int index, const std::string &key, const std::string &value) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   if(index >= CTRLM_VOICE_QUERY_STRING_MAX_PAIRS) {
      XLOGD_ERROR("index out of range <%u> <%u>", index, CTRLM_VOICE_QUERY_STRING_MAX_PAIRS);
      return;
   }
   if(key.length() > CTRLM_VOICE_QUERY_STRING_MAX_LENGTH - 1) {
      XLOGD_ERROR("key would be truncated <%u> max size <%u>", key.length(), CTRLM_VOICE_QUERY_STRING_MAX_LENGTH - 1);
      return;
   }
   if(value.length() > CTRLM_VOICE_QUERY_STRING_MAX_LENGTH - 1) {
      XLOGD_ERROR("value would be truncated <%u> max size <%u>", value.length(), CTRLM_VOICE_QUERY_STRING_MAX_LENGTH - 1);
      return;
   }

   query_string_pair_t *pair = &g_ctrlm_sm.mmap->query_string_ptt_pairs[index];

   errno_t safec_rc = strncpy_s(pair->key, sizeof(pair->key), key.c_str(), key.length());
   ERR_CHK(safec_rc);

   safec_rc = strncpy_s(pair->value, sizeof(pair->value), value.c_str(), value.length());
   ERR_CHK(safec_rc);

   pair->key  [sizeof(pair->key)   - 1] = '\0';
   pair->value[sizeof(pair->value) - 1] = '\0';
}

bool ctrlm_sm_voice_init_blob_read(std::string &init, bool *valid) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return(false);
   }

   if(!g_ctrlm_sm.mmap->init_blob_valid) {
      *valid = false;
   } else {
      *valid = true;
      init = g_ctrlm_sm.mmap->init_blob;
   }
   return(true);
}

void ctrlm_sm_voice_init_blob_write(const std::string &init) {
   if(g_ctrlm_sm.mmap == NULL) {
      XLOGD_ERROR("mmap is invalid");
      return;
   }
   if(init.length() > sizeof(g_ctrlm_sm.mmap->init_blob) - 1) {
      XLOGD_ERROR("init blob would be truncated <%u> max size <%u>", init.length(), sizeof(g_ctrlm_sm.mmap->init_blob) - 1);
      g_ctrlm_sm.mmap->init_blob[0] = '\0';
      return;
   }

   errno_t safec_rc = strncpy_s(g_ctrlm_sm.mmap->init_blob, sizeof(g_ctrlm_sm.mmap->init_blob), init.c_str(), init.length());
   ERR_CHK(safec_rc);
   g_ctrlm_sm.mmap->init_blob[sizeof(g_ctrlm_sm.mmap->init_blob) - 1] = '\0';
   g_ctrlm_sm.mmap->init_blob_valid = true;
}
