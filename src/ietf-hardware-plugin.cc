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

#include <sysrepo-cpp/Session.hpp>

#include <stdio.h>

extern "C" {
    void sr_plugin_cleanup_cb(sr_session_ctx_t* session, void* private_data);
    int sr_plugin_init_cb(sr_session_ctx_t* session, void** private_data);
}

using sysrepo::S_Session;
using sysrepo::S_Subscribe;
using sysrepo::Session;
using sysrepo::Subscribe;
using sysrepo::Callback;

struct HardwareModel {
    S_Subscribe subscription;
    S_Session sess;
};

void logMessage(sr_log_level_t log, std::string msg) {
    switch (log) {
        case SR_LL_ERR:
            SRP_LOG_ERRMSG(msg.c_str());
        case SR_LL_WRN:
            SRP_LOG_WRNMSG(msg.c_str());
        case SR_LL_INF:
            SRP_LOG_INFMSG(msg.c_str());
        case SR_LL_DBG:
        default:
            SRP_LOG_DBGMSG(msg.c_str());
    }
}

struct ConfigurationCallback : public Callback {
    int module_change([[maybe_unused]] S_Session session,
                        char const* module_name,
                        [[maybe_unused]] char const* xpath,
                        sr_event_t event,
                        uint32_t /* request_id */,
                        void* /* private_ctx */) override {
        printCurrentConfig(session, module_name);
        return SR_ERR_OK;
    }

    void printCurrentConfig(S_Session session, char const* module_name) {
        try {
            std::string xpath(std::string("/") + module_name + std::string(":*//*"));
            auto values = session->get_items(xpath.c_str());
            if (values == nullptr)
                return;

            for(unsigned int i = 0; i < values->val_cnt(); i++)
                logMessage(SR_LL_INF, values->val(i)->to_string());

        } catch (const std::exception& e) {
            logMessage(SR_LL_ERR, e.what());
        }
    }
};

HardwareModel theModel;

int sr_plugin_init_cb(sr_session_ctx_t* session, void** /*private_data*/)
{
    int rc = 0;
    theModel.sess = std::make_shared<Session>(session);
    theModel.subscription = std::make_shared<Subscribe>(theModel.sess);
    return rc;
}

void sr_plugin_cleanup_cb(sr_session_ctx_t* /*session*/, void* /*private_data*/)
{
    // nothing to cleanup except freeing the subscriptions
    theModel.subscription->unsubscribe();
    SRP_LOG_DBGMSG("IETF-Hardware plugin cleanup finished.");
}