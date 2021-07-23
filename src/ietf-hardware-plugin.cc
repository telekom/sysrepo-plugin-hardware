// telekom / sysrepo-plugin-hardware
//
// This program is made available under the terms of the
// BSD 3-Clause license which is available at
// https://opensource.org/licenses/BSD-3-Clause
//
// SPDX-FileCopyrightText: 2021 Deutsche Telekom AG
//
// SPDX-License-Identifier: BSD-3-Clause

#include "callback.h"
#include <stdio.h>

extern "C" {
void sr_plugin_cleanup_cb(sr_session_ctx_t* session, void* private_data);
int sr_plugin_init_cb(sr_session_ctx_t* session, void** private_data);
}

using sysrepo::S_Session;
using sysrepo::S_Subscribe;
using sysrepo::Session;
using sysrepo::Subscribe;

struct HardwareModel {
    S_Subscribe subscription;
    S_Session sess;
    static std::string const moduleName;
};

std::string const HardwareModel::moduleName = "ietf-hardware";

static HardwareModel theModel;

int sr_plugin_init_cb(sr_session_ctx_t* session, void** /*private_data*/) {
    theModel.sess = std::make_shared<Session>(session);
    theModel.subscription = std::make_shared<Subscribe>(theModel.sess);
    std::string oper_xpath("/" + HardwareModel::moduleName + ":" + "hardware");
    try {
        theModel.subscription->module_change_subscribe(
            HardwareModel::moduleName.c_str(), &hardware::Callback::configurationCallback, nullptr,
            0, SR_SUBSCR_ENABLED | SR_SUBSCR_DONE_ONLY);
        theModel.subscription->oper_get_items_subscribe(HardwareModel::moduleName.c_str(),
                                                        &hardware::Callback::operationalCallback,
                                                        oper_xpath.c_str());
    } catch (std::exception const& e) {
        logMessage(SR_LL_ERR, std::string("sr_plugin_init_cb: ") + e.what());
        theModel.subscription.reset();
        return SR_ERR_OPERATION_FAILED;
    }

    return SR_ERR_OK;
}

void sr_plugin_cleanup_cb(sr_session_ctx_t* /*session*/, void* /*private_data*/) {
    theModel.subscription.reset();
    theModel.sess.reset();
    logMessage(SR_LL_DBG, "plugin cleanup finished.");
}
