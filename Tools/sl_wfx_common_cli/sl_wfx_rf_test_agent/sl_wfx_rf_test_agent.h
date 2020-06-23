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

#ifndef SL_WFX_RF_TEST_AGENT_H
#define SL_WFX_RF_TEST_AGENT_H

#include "sl_status.h"
#include "sl_wfx_api.h"
#include "sl_wfx_constants.h"

/** X.x.x: Major version of the test agent */
#define SL_WFX_RF_TEST_AGENT_VERSION_MAJOR      1
/** x.X.x: Minor version of the test agent */
#define SL_WFX_RF_TEST_AGENT_VERSION_MINOR      0
/** x.x.X: Revision of the test agent */
#define SL_WFX_RF_TEST_AGENT_VERSION_REVISION   0

/* Some helper defines to get a version string */
#define SL_WFX_RF_TEST_AGENT_VERSTR2(x) #x
#define SL_WFX_RF_TEST_AGENT_VERSTR(x)  SL_WFX_RF_TEST_AGENT_VERSTR2(x)

/** Provides the version of the test agent */
#define SL_WFX_RF_TEST_AGENT_VERSION   ((SL_WFX_RF_TEST_AGENT_VERSION_MAJOR) << 16 \
                                       | (SL_WFX_RF_TEST_AGENT_VERSION_MINOR) << 8 \
                                       | (SL_WFX_RF_TEST_AGENT_VERSION_REVISION))
/** Provides the version of the test agent as string */
#define SL_WFX_RF_TEST_AGENT_VERSION_STRING     SL_WFX_RF_TEST_AGENT_VERSTR(SL_WFX_RF_TEST_AGENT_VERSION_MAJOR) "." \
                                                SL_WFX_RF_TEST_AGENT_VERSTR(SL_WFX_RF_TEST_AGENT_VERSION_MINOR) "." \
                                                SL_WFX_RF_TEST_AGENT_VERSTR(SL_WFX_RF_TEST_AGENT_VERSION_REVISION)

sl_status_t sl_wfx_rf_test_agent_init(sl_wfx_rx_stats_t *wfx_rx_stats);
sl_status_t sl_wfx_rf_test_agent(sl_wfx_context_t* rf_test_context, int argc, char **argv);

#endif // SL_WFX_RF_TEST_AGENT_H
