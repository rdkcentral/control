/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 RDK Management
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

/*
 * CI mock for the installed rdkx logger surface.
 *
 * ctrlm and xr_voice_sdk.h expect logger types, macros, and some std:: names
 * from rdkx_logger.h, but that installed header is not available as a simple
 * source header in this reduced native build. This file provides only the
 * compile-time surface ctrlm needs and keeps logging as a no-op in CI.
 */
#ifndef CTRLM_CI_RDKX_LOGGER_H_
#define CTRLM_CI_RDKX_LOGGER_H_

#ifdef __cplusplus
#include <string>
#include <map>
#include <tuple>
/* The real rdkx_logger.h pulls in std:: namespace indirectly; replicate only the needed names here */
// ctrlm expects these to be in the global namespace, so we need to pull them in here
using std::get;
using std::map;
using std::string;
using std::tuple;
#endif

/* Log option flags */
#define XLOG_OPTS_DATE      (1 << 0)
#define XLOG_OPTS_TIME      (1 << 1)
#define XLOG_OPTS_LF        (1 << 2)
#define XLOG_OPTS_MOD_NAME  (1 << 3)
#define XLOG_OPTS_LEVEL     (1 << 4)
#define XLOG_OPTS_COLOR     (1 << 5)

/* Module IDs */
#define XLOG_MODULE_ID_CTRLM   0
#define XLOG_MODULE_ID_VSDK    1
#define XLOG_MODULE_ID_XRAUDIO 2

#ifndef XLOG_MODULE_ID
#define XLOG_MODULE_ID XLOG_MODULE_ID_CTRLM
#endif

/* Stub types and functions used by xlog_fprintf */
#ifndef XLOG_OPTS_DEFAULT
#define XLOG_OPTS_DEFAULT     0
#endif

#ifndef XLOG_COLOR_NONE
#define XLOG_COLOR_NONE       0
#endif

#ifndef XLOG_COLOR_BLU
#define XLOG_COLOR_BLU        0
#endif

#ifndef XLOG_COLOR_GRN
#define XLOG_COLOR_GRN        0
#endif

#ifndef XLOG_LINE_NONE
#define XLOG_LINE_NONE        0
#endif

typedef int xlog_level_t;

#ifndef XLOG_LEVEL_DEBUG
#define XLOG_LEVEL_DEBUG      0
#endif

#ifndef XLOG_LEVEL_INFO
#define XLOG_LEVEL_INFO       1
#endif

#ifndef XLOG_LEVEL_WARN
#define XLOG_LEVEL_WARN       2
#endif

#ifndef XLOG_LEVEL_ERROR
#define XLOG_LEVEL_ERROR      3
#endif

#ifndef XLOG_LEVEL_FATAL
#define XLOG_LEVEL_FATAL      4
#endif

#ifndef XLOG_LEVEL_TELEMETRY
#define XLOG_LEVEL_TELEMETRY  5
#endif

#ifndef XLOG_BUF_SIZE_DEFAULT
#define XLOG_BUF_SIZE_DEFAULT 0
#endif

typedef struct {
   int options;
   int color;
   const char *function;
   int line;
   int level;
   int id;
   int size_max;
} xlog_args_t;

#ifndef xlog_fprintf
#define xlog_fprintf(args, output, fmt, ...) do { (void)(args); (void)(output); (void)sizeof(fmt); } while(0)
#endif

#ifndef xlog_printf
#define xlog_printf(args, fmt, ...) do { (void)(args); (void)sizeof(fmt); } while(0)
#endif

#ifndef xlog_init
#define xlog_init(id, path, options, console, syslog) (0)
#endif

#ifndef xlog_level_set_all
#define xlog_level_set_all(level) do { (void)(level); } while(0)
#endif

#ifndef xlog_level_get
#define xlog_level_get(id) (XLOG_LEVEL_INFO)
#endif

#ifndef xlog_term
#define xlog_term() do { } while(0)
#endif

/* Logging macros - silently discard in stub builds (avoids -Werror=format-zero-length) */
#ifndef XLOGD
#define XLOGD(level, opts, color, size, fmt, ...) do { (void)(level); (void)(opts); (void)(color); (void)(size); (void)sizeof(fmt); } while(0)
#endif
#ifndef XLOGD_INFO
#define XLOGD_INFO(fmt, ...)              do { (void)sizeof(fmt); } while(0)
#endif
#ifndef XLOGD_DEBUG
#define XLOGD_DEBUG(fmt, ...)             do { (void)sizeof(fmt); } while(0)
#endif
#ifndef XLOGD_ERROR
#define XLOGD_ERROR(fmt, ...)             do { (void)sizeof(fmt); } while(0)
#endif
#ifndef XLOGD_WARN
#define XLOGD_WARN(fmt, ...)              do { (void)sizeof(fmt); } while(0)
#endif
#ifndef XLOGD_FATAL
#define XLOGD_FATAL(fmt, ...)             do { (void)sizeof(fmt); } while(0)
#endif
#ifndef XLOGD_TELEMETRY
#define XLOGD_TELEMETRY(fmt, ...)         do { (void)sizeof(fmt); } while(0)
#endif
#ifndef XLOGD_INFO_OPTS
#define XLOGD_INFO_OPTS(opts, fmt, ...)   do { (void)(opts); (void)sizeof(fmt); } while(0)
#endif
#ifndef XLOGD_NO_LF
#define XLOGD_NO_LF(level, fmt, ...)      do { (void)(level); (void)sizeof(fmt); } while(0)
#endif
/* XLOGD_OUTPUT is used as an output-handle VALUE (second arg to xlog_fprintf), not a logging macro */
#ifndef XLOGD_OUTPUT
#define XLOGD_OUTPUT NULL
#endif

#ifndef XLOGD_AUTOMATION_INFO
#define XLOGD_AUTOMATION_INFO(fmt, ...)   do { (void)sizeof(fmt); } while(0)
#endif
#ifndef XLOGD_AUTOMATION_DEBUG
#define XLOGD_AUTOMATION_DEBUG(fmt, ...)  do { (void)sizeof(fmt); } while(0)
#endif
#ifndef XLOGD_AUTOMATION_ERROR
#define XLOGD_AUTOMATION_ERROR(fmt, ...)  do { (void)sizeof(fmt); } while(0)
#endif
#ifndef XLOGD_AUTOMATION_WARN
#define XLOGD_AUTOMATION_WARN(fmt, ...)   do { (void)sizeof(fmt); } while(0)
#endif
#ifndef XLOGD_AUTOMATION_TELEMETRY
#define XLOGD_AUTOMATION_TELEMETRY(fmt, ...) do { (void)sizeof(fmt); } while(0)
#endif

#ifndef XLOG_INFO
#define XLOG_INFO(fmt, ...)    do { (void)sizeof(fmt); } while(0)
#endif
#ifndef XLOG_DEBUG
#define XLOG_DEBUG(fmt, ...)   do { (void)sizeof(fmt); } while(0)
#endif
#ifndef XLOG_ERROR
#define XLOG_ERROR(fmt, ...)   do { (void)sizeof(fmt); } while(0)
#endif
#ifndef XLOG_WARN
#define XLOG_WARN(fmt, ...)    do { (void)sizeof(fmt); } while(0)
#endif
#ifndef XLOG_RAW
#define XLOG_RAW(fmt, ...)     do { (void)sizeof(fmt); } while(0)
#endif
#ifndef XLOG_FLUSH
#define XLOG_FLUSH()           do { } while(0)
#endif

#endif /* _RDKX_LOGGER_H_ */
