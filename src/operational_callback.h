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

#include <sensor_data.h>
#include <utils/globals.h>
#include <utils/magic_enum.hpp>
#include <utils/rapidjson/document.h>
#include <utils/rapidjson/istreamwrapper.h>

#include <chrono>
#include <fstream>
#include <regex>
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

        if (!std::regex_search(request_xpath, std::regex("ietf-hardware:(.*)"))) {
            logMessage(SR_LL_ERR, std::string("Invalid requested xpath: ") + request_xpath);
            return SR_ERR_CALLBACK_FAILED;
        }

        std::string const set_xpath("/ietf-hardware:hardware");

        int rc = system("/usr/bin/lshw -json > /tmp/hardware_components.json");
        if (rc == -1) {
            logMessage(SR_LL_ERR, "lshw command failed");
            return SR_ERR_CALLBACK_FAILED;
        }
        logMessage(SR_LL_DBG, "lshw command returned:" + std::to_string(rc));

        // +--ro last-change?   yang:date-and-time
        std::time_t lastChange(
            std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
        char timeString[100];
        if (std::strftime(timeString, sizeof(timeString), "%FT%TZ", std::localtime(&lastChange))) {
            setValue(session, parent, set_xpath + "/last-change", timeString);
        }

        std::ifstream ifs("/tmp/hardware_components.json", std::ifstream::in);
        if (ifs.fail()) {
            logMessage(SR_LL_ERR, "Can't open: /tmp/hardware_components.json");
            return SR_ERR_CALLBACK_FAILED;
        }
        IStreamWrapper isw(ifs);

        Document doc;
        doc.ParseStream(isw);
        if (!doc.IsArray()) {
            logMessage(SR_LL_ERR, "lshw json root-node is not an array");
            return SR_ERR_CALLBACK_FAILED;
        }

        auto newArray = doc.GetArray();

        parseAndSetComponents(session, parent, set_xpath, newArray, std::string());

        try {
            HardwareSensors hwsens;
            std::vector<Sensor> sensors = hwsens.parseSensorData();
            setSensorData(session, parent, set_xpath, sensors);
        } catch (std::exception const& e) {
            logMessage(SR_LL_WRN, "hardware-sensors nodes failure: " + std::string(e.what()));
        }

        if (!parent) {
            return SR_ERR_CALLBACK_FAILED;
        }
        return SR_ERR_OK;
    }

    static std::string toIANAclass(std::string const& inputClass) {
        std::string returnedClass;
        static std::unordered_map<std::string, std::string> _{
            {"storage", "iana-hardware:storage-drive"},
            {"power", "iana-hardware:battery"},
            {"processor", "iana-hardware:cpu"},
            {"network", "iana-hardware:port"}};

        if (_.find(inputClass) != _.end()) {
            returnedClass = _.at(inputClass);
        }
        return returnedClass;
    }

    void setSensorData(S_Session& session,
                       S_Data_Node& parent,
                       std::string const& request_xpath,
                       std::vector<Sensor> const& sensors) {
        for (auto& sensor : sensors) {
            std::string sensorPath = request_xpath + "/component[name='" + sensor.name + "']";
            setValue(session, parent, sensorPath + "/class", "iana-hardware:sensor");
            setValue(session, parent, sensorPath + "/sensor-data/value",
                     std::to_string(sensor.value));
            setValue(session, parent, sensorPath + "/sensor-data/value-type",
                     Sensor::getValueTypeForModel(sensor.valueType));
            setValue(session, parent, sensorPath + "/sensor-data/value-scale",
                     std::string(magic_enum::enum_name(sensor.valueScale)));
            setValue(session, parent, sensorPath + "/sensor-data/value-precision",
                     std::to_string(sensor.valuePrecision));
            setValue(session, parent, sensorPath + "/sensor-data/oper-status", "ok");
            char timeString[100];
            if (std::strftime(timeString, sizeof(timeString), "%FT%TZ",
                              std::localtime(&sensor.valueTimestamp))) {
                setValue(session, parent, sensorPath + std::string("/sensor-data/value-timestamp"),
                         timeString);
            }
        }
    }

    std::vector<std::string> parseAndSetComponents(S_Session& session,
                                                   S_Data_Node& parent,
                                                   std::string const& request_xpath,
                                                   Value const& parsee,
                                                   std::string const& parentName) {
        std::vector<std::string> siblings;
        for (auto& m : parsee.GetArray()) {
            Value::ConstMemberIterator itr = m.FindMember("id");
            std::string name, componentPath;
            if (itr != m.MemberEnd()) {
                name = itr->value.GetString();
                componentPath = request_xpath + "/component[name='" + name + "']";
            } else {
                // invalid entry, go to the next one
                continue;
            }

            // firmware node, skip this one and set the parent's firmware-rev
            // +--ro firmware-rev?     string
            if (name == "firmware" && !parentName.empty() &&
                (itr = m.FindMember("version")) != m.MemberEnd()) {
                componentPath = request_xpath + "/component[name='" + parentName + "']";
                setValue(session, parent, componentPath + "/firmware-rev", itr->value.GetString());
                continue;
            }
            siblings.push_back(name);

            // +--rw name              string
            // +--ro description?      string
            // +--ro hardware-rev?     string
            // +--ro serial-num?       string
            // +--ro mfg-name?         string
            // +--ro model-name?       string
            // +--rw alias?            string
            for (auto const& mapValue : getLSHWtoIETFmap()) {
                std::string const stringValue = mapValue.first;
                if ((itr = m.FindMember(stringValue.c_str())) != m.MemberEnd()) {
                    setValue(session, parent, componentPath + mapValue.second,
                             itr->value.GetString());
                }
            }

            // +--rw class             identityref
            if ((itr = m.FindMember("class")) != m.MemberEnd()) {
                std::string ianaClass = toIANAclass(itr->value.GetString());
                if (!ianaClass.empty()) {
                    setValue(session, parent, componentPath + "/class", ianaClass);
                }
            }
            // +--ro software-rev?     string
            // +--ro uuid?             yang:uuid
            if ((itr = m.FindMember("configuration")) != m.MemberEnd()) {
                Value::ConstMemberIterator config_elem = itr->value.FindMember("uuid");
                if (config_elem != itr->value.MemberEnd()) {
                    setValue(session, parent, componentPath + "/uuid",
                             config_elem->value.GetString());
                }
                if ((config_elem = itr->value.FindMember("driverversion")) !=
                    itr->value.MemberEnd()) {
                    setValue(session, parent, componentPath + "/software-rev",
                             config_elem->value.GetString());
                }
                if ((config_elem = itr->value.FindMember("firmware")) != itr->value.MemberEnd()) {
                    setValue(session, parent, componentPath + "/firmware-rev",
                             config_elem->value.GetString());
                }
            }
            // +--rw parent?           -> ../../component/name
            if (!parentName.empty()) {
                setValue(session, parent, componentPath + "/parent", parentName.c_str());
            }
            if ((itr = m.FindMember("children")) != m.MemberEnd()) {
                std::vector<std::string> childList = parseAndSetComponents(
                    session, parent, request_xpath, itr->value.GetArray(), name);
                // childlist to value
                // +--ro contains-child*   -> ../../component/name
                for (auto const& elem : childList) {
                    setValue(session, parent, componentPath + "/contains-child", elem);
                }
            }
        }
        return siblings;
    }

    void printCurrentConfig(S_Session& session, char const* module_name) {
        try {
            std::string xpath(std::string("/") + module_name + std::string(":*//*"));
            auto values = session->get_items(xpath.c_str());
            if (values == nullptr)
                return;

            for (unsigned int i = 0; i < values->val_cnt(); i++)
                logMessage(SR_LL_DBG, values->val(i)->to_string());

        } catch (const std::exception& e) {
            logMessage(SR_LL_WRN, e.what());
        }
    }

    bool setValue(S_Session& session,
                  S_Data_Node& parent,
                  std::string const& node_xpath,
                  std::string const& value) {
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
            logMessage(SR_LL_WRN, "At path " + node_xpath + ", value " + value + " " + e.what());
            return false;
        }
        return true;
    }

    static std::unordered_map<std::string, std::string> const& getLSHWtoIETFmap() {
        static std::unordered_map<std::string, std::string> const _{
            {"description", "/description"}, {"vendor", "/mfg-name"},
            {"serial", "/serial-num"},       {"product", "/model-name"},
            {"version", "/hardware-rev"},    {"handle", "/alias"}};
        return _;
    }
};

}  // namespace hardware

#endif  // OPERATIONAL_CALLBACK_H