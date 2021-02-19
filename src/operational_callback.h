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

#ifndef OPERATIONAL_CALLBACK_H
#define OPERATIONAL_CALLBACK_H

#include <globals.h>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

#include <fstream>
#include <unordered_map>

namespace hardware {

struct OperationalCallback : public sysrepo::Callback {
    using S_Data_Node = libyang::S_Data_Node;
    using S_Context = libyang::S_Context;
    using Data_Node = libyang::Data_Node;
    using S_Session = sysrepo::S_Session;
    using Value = rapidjson::Value;
    using Document = rapidjson::Document;
    using IStreamWrapper = rapidjson::IStreamWrapper;

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
        parent = nullptr;

        int rc = system("/usr/bin/lshw -json > /tmp/hardware_components.json");
        if (rc == -1) {
            logMessage(SR_LL_ERR, std::string("lshw command failed"));
            return SR_ERR_CALLBACK_FAILED;
        }

        logMessage(SR_LL_DBG, std::string("lshw command returned:") + std::to_string(rc));

        std::ifstream ifs("/tmp/hardware_components.json", std::ifstream::in);
        IStreamWrapper isw(ifs);

        Document doc;
        doc.ParseStream(isw);
        if (!doc.IsArray()) {
            logMessage(SR_LL_ERR, "lshw json root-node is not an array");
            return SR_ERR_CALLBACK_FAILED;
        }

        auto newArray = doc.GetArray();

        std::vector<std::string> temp;
        parseArray(session, parent, request_xpath, newArray, std::string(), temp);

        if (!parent) {
            return SR_ERR_CALLBACK_FAILED;
        }
        return SR_ERR_OK;
    }

    void parseArray(S_Session& session,
                    S_Data_Node& parent,
                    char const* request_xpath,
                    Value const& parsee,
                    std::string const& parentName,
                    std::vector<std::string>& childList) {
        for (auto& m : parsee.GetArray()) {
            Value::ConstMemberIterator itr = m.FindMember("id");
            std::string name;
            if (itr != m.MemberEnd()) {
                name = itr->value.GetString();
            }
            for (auto const& mapValue : getMap()) {
                std::string const stringValue = mapValue.first;
                if ((itr = m.FindMember(stringValue.c_str())) != m.MemberEnd()) {
                    setValue(session, parent,
                             request_xpath + std::string("/component[name='") + name + "']" +
                                 mapValue.second,
                             itr->value.GetString());
                }
            }
            if (!parentName.empty()) {
                setValue(session, parent,
                         request_xpath + std::string("/component[name='") + name + "']/parent",
                         parentName.c_str());
            }
            if ((itr = m.FindMember("children")) != m.MemberEnd()) {
                std::vector<std::string> temp;
                parseArray(session, parent, request_xpath, itr->value.GetArray(), name, temp);
                // childlist to value
                for (auto const& elem : temp) {
                    setValue(session, parent,
                             request_xpath + std::string("/component[name='") + name +
                                 "']/contains-child",
                             elem);
                }
            }
        }
    }

    void printCurrentConfig(S_Session session, char const* module_name) {
        try {
            std::string xpath(std::string("/") + module_name + std::string(":*//*"));
            auto values = session->get_items(xpath.c_str());
            if (values == nullptr)
                return;

            for (unsigned int i = 0; i < values->val_cnt(); i++)
                logMessage(SR_LL_INF, values->val(i)->to_string());

        } catch (const std::exception& e) {
            logMessage(SR_LL_ERR, e.what());
        }
    }

    bool
    setValue(S_Session& session, S_Data_Node& parent, std::string node_xpath, std::string value) {
        try {
            S_Context ctx = session->get_context();
            if (parent) {
                parent->new_path(ctx, node_xpath.c_str(), value.c_str(), LYD_ANYDATA_CONSTSTRING,
                                 0);
            } else {
                parent = make_shared<Data_Node>(ctx, node_xpath.c_str(), value.c_str(),
                                                LYD_ANYDATA_CONSTSTRING, 0);
            }
        } catch (std::exception const& e) {
            logMessage(SR_LL_ERR, "At path " + node_xpath + ", value " + value + " " + e.what());
            return false;
        }
        return true;
    }

    static std::unordered_map<std::string, std::string> const& getMap() {
        static std::unordered_map<std::string, std::string> const _{
            {"description", "/description"}, {"vendor", "/mfg-name"},
            {"serial", "/serial-num"},       {"product", "/model-name"},
            {"version", "/hardware-rev"},    {"handle", "/alias"}};
        return _;
    }
};

}  // namespace hardware

#endif  // OPERATIONAL_CALLBACK_H
