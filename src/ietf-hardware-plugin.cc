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
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

#include <fstream>
#include <stdio.h>

extern "C" {
    void sr_plugin_cleanup_cb(sr_session_ctx_t* session, void* private_data);
    int sr_plugin_init_cb(sr_session_ctx_t* session, void** private_data);
}

using libyang::S_Data_Node;

using sysrepo::S_Callback;
using sysrepo::S_Session;
using sysrepo::S_Subscribe;
using sysrepo::Session;
using sysrepo::Subscribe;
using sysrepo::Callback;

struct HardwareModel {
    S_Subscribe subscription;
    S_Session sess;
    static std::string moduleName;
};

std::string HardwareModel::moduleName = "ietf-hardware";

void logMessage(sr_log_level_t log, std::string msg) {
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

    int oper_get_items(sysrepo::S_Session session,
                       const char* module_name,
                       const char* /* path */,
                       const char* request_xpath,
                       uint32_t /* request_id */,
                       S_Data_Node& parent,
                       void* /* private_data */) override {
        using namespace rapidjson;
        int rc = system("/usr/bin/lshw -json > /tmp/hardware_components.json");
        if(rc == -1) {
            logMessage(SR_LL_ERR, std::string("lshw command failed"));
            return SR_ERR_CALLBACK_FAILED;
        }

        logMessage(SR_LL_DBG, std::string("lshw command returned:") + std::to_string(rc));

        std::ifstream ifs("/tmp/hardware_components.json", std::ifstream::in);
        IStreamWrapper isw(ifs);

        Document doc;
        doc.ParseStream(isw);
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

static HardwareModel theModel;

int sr_plugin_init_cb(sr_session_ctx_t* session, void** /*private_data*/)
{
    theModel.sess = std::make_shared<Session>(session);
    theModel.subscription = std::make_shared<Subscribe>(theModel.sess);
    S_Callback callback = std::make_shared<ConfigurationCallback>();

    std::string oper_xpath("/" + HardwareModel::moduleName + ":" + "hardware");
    try {
        theModel.subscription->module_change_subscribe(HardwareModel::moduleName.c_str(), callback,
                nullptr, nullptr, 0, SR_SUBSCR_ENABLED | SR_SUBSCR_DONE_ONLY);
        theModel.subscription->oper_get_items_subscribe(HardwareModel::moduleName.c_str(),
                oper_xpath.c_str(), callback);
    } catch (std::exception const& e) {
        logMessage(SR_LL_ERR, std::string("sr_plugin_init_cb: ") + e.what());
        theModel.subscription.reset();
        return SR_ERR_OPERATION_FAILED;
    }

    return SR_ERR_OK;
}

void sr_plugin_cleanup_cb(sr_session_ctx_t* /*session*/, void* /*private_data*/)
{
    // nothing to cleanup except freeing the subscriptions
    theModel.subscription->unsubscribe();
    SRP_LOG_DBGMSG("IETF-Hardware plugin cleanup finished.");
}
