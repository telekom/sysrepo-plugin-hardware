// (C) 2020 Deutsche Telekom AG.
//
// Deutsche Telekom AG and all other contributors /
// copyright owners license this file to you under the Apache
// License, Version 2.0 (the "License"); you may not use this
// file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the License for the
// specific language governing permissions and limitations
// under the License.

#ifndef GLOBALS_H
#define GLOBALS_H

#include <sysrepo-cpp/Session.hpp>

void logMessage(sr_log_level_t log, std::string msg) {
    msg = "IETF-Hardware: " + msg;
    switch (log) {
    case SR_LL_ERR:
        SRP_LOG_ERRMSG(msg.c_str());
        break;
    case SR_LL_WRN:
        SRP_LOG_WRNMSG(msg.c_str());
        break;
    case SR_LL_INF:
        SRP_LOG_INFMSG(msg.c_str());
        break;
    case SR_LL_DBG:
    default:
        SRP_LOG_DBGMSG(msg.c_str());
    }
}

#endif  // GLOBALS_H
