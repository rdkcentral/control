
/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2014 RDK Management
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
#ifndef _CTRLM_LOG_H_
#define _CTRLM_LOG_H_

#include <stdio.h>

#ifdef ANSI_CODES_DISABLED
#define XLOG_OPTS_DEFAULT (XLOG_OPTS_DATE | XLOG_OPTS_TIME | XLOG_OPTS_LF | XLOG_OPTS_MOD_NAME | XLOG_OPTS_LEVEL)
#else
#define XLOG_OPTS_DEFAULT (XLOG_OPTS_DATE | XLOG_OPTS_TIME | XLOG_OPTS_LF | XLOG_OPTS_MOD_NAME | XLOG_OPTS_LEVEL | XLOG_OPTS_COLOR)
#endif

#ifndef XLOG_MODULE_ID
#define XLOG_MODULE_ID XLOG_MODULE_ID_CTRLM
#endif

#include "rdkx_logger.h"

#endif
