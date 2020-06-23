/**************************************************************************//**
 * Copyright 2018, Silicon Laboratories Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#ifndef SL_WFX_CLI_H
#define SL_WFX_CLI_H

/** X.x.x: Major version of the CLI */
#define SL_WFX_CLI_VERSION_MAJOR      1
/** x.X.x: Minor version of the CLI */
#define SL_WFX_CLI_VERSION_MINOR      0
/** x.x.X: Revision of the CLI */
#define SL_WFX_CLI_VERSION_REVISION   0

/* Some helper defines to get a version string */
#define SL_WFX_CLI_VERSTR2(x) #x
#define SL_WFX_CLI_VERSTR(x) SL_WFX_CLI_VERSTR2(x)

/** Provides the version of the driver */
#define SL_WFX_CLI_VERSION   ((SL_WFX_CLI_VERSION_MAJOR) << 16 \
                             | (SL_WFX_CLI_VERSION_MINOR) << 8 \
                             | (SL_WFX_CLI_VERSION_REVISION))
/** Provides the version of the driver as string */
#define SL_WFX_CLI_VERSION_STRING     SL_WFX_CLI_VERSTR(SL_WFX_CLI_VERSION_MAJOR) "." \
                                      SL_WFX_CLI_VERSTR(SL_WFX_CLI_VERSION_MINOR) "." \
                                      SL_WFX_CLI_VERSTR(SL_WFX_CLI_VERSION_REVISION)

#endif // SL_WFX_CLI_H