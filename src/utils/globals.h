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

#define COMPONENTS_LOCATION "/tmp/hardware_components.json"
#define DEFAULT_POLL_INTERVAL 60  // seconds

struct SensorsInitFail : public std::exception {
    const char* what() const throw() override {
        return "sensor_init() failure";
    }
};

static void logMessage(sr_log_level_t log, std::string msg) {
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

static bool setXpath(sysrepo::S_Session& session,
                     libyang::S_Data_Node& parent,
                     std::string const& node_xpath,
                     std::string const& value) {
    try {
        libyang::S_Context ctx = session->get_context();
        if (parent) {
            parent->new_path(ctx, node_xpath.c_str(), value.c_str(), LYD_ANYDATA_CONSTSTRING, 0);
        } else {
            parent = std::make_shared<libyang::Data_Node>(ctx, node_xpath.c_str(), value.c_str(),
                                                          LYD_ANYDATA_CONSTSTRING, 0);
        }
    } catch (std::runtime_error const& e) {
        logMessage(SR_LL_WRN,
                   "At path " + node_xpath + ", value " + value + " " + ", error: " + e.what());
        return false;
    }
    return true;
}

#endif  // GLOBALS_H
