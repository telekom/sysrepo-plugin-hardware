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

#ifndef COMPONENT_DATA_H
#define COMPONENT_DATA_H

#include <stdint.h>
#include <utils/globals.h>

#include <iostream>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace hardware {

using libyang::Data_Node;
using libyang::Data_Node_Leaf_List;
using libyang::S_Context;
using libyang::S_Data_Node;
using libyang::S_Schema_Node;
using libyang::Schema_Node_Leaf;
using libyang::Schema_Node_Leaflist;
using libyang::Schema_Node_List;

struct ComponentData;
struct SensorThreshold;

using ComponentMap = std::unordered_map<std::string, std::shared_ptr<ComponentData>>;
using ComponentList = std::list<std::shared_ptr<ComponentData>>;
using SensorThresholdList = std::list<std::shared_ptr<SensorThreshold>>;

struct SensorThreshold {
    enum class ThresholdType { min, max };

    SensorThreshold(std::string const& newName)
        : name(newName), value(0), type(ThresholdType::min), triggered(false){};

    std::string name;
    int32_t value;
    ThresholdType type;
    bool triggered;
};

struct ComponentData {

    ComponentData(std::string const& compName,
                  std::string const& compClass = "iana-hardware:unknown")
        : name(compName), classType(compClass){};

    virtual ~ComponentData() = default;

    virtual void setXpathForAllMembers(sysrepo::S_Session& session,
                                       S_Data_Node& parent,
                                       std::string const& mainXpath) const {
        std::string componentPath(mainXpath + "/component[name='" + name + "']");
        logMessage(SR_LL_DBG, "Setting values for component: " + name);
        // +--rw name              string
        // +--rw class             identityref
        // +--ro physical-index?   int32 {entity-mib}?
        // +--ro description?      string
        // +--rw parent?           -> ../../component/name
        // +--rw parent-rel-pos?   int32
        // +--ro contains-child*   -> ../../component/name
        // +--ro hardware-rev?     string
        // +--ro software-rev?     string
        // +--ro firmware-rev?     string
        // +--ro serial-num?       string
        // +--ro mfg-name?         string
        // +--ro model-name?       string
        // +--rw alias?            string
        // +--rw asset-id?         string
        // +--rw uri*              inet:uri
        // +--ro uuid?             yang:uuid
        setXpath(session, parent, componentPath + "/class", classType);

        if (description) {
            setXpath(session, parent, componentPath + "/description", description.value());
        }
        if (physicalID && parent && parent->node_module()->feature_state("entity-mib") == 1) {
            setXpath(session, parent, componentPath + "/physical-index",
                     std::to_string(physicalID.value()));
        }
        if (parentName) {
            setXpath(session, parent, componentPath + "/parent", parentName.value());
        }
        if (parent_rel_pos) {
            setXpath(session, parent, componentPath + "/parent-rel-pos",
                     std::to_string(parent_rel_pos.value()));
        }
        // childlist to value
        for (auto const& elem : children) {
            setXpath(session, parent, componentPath + "/contains-child", elem);
        }
        if (hardwareRev) {
            setXpath(session, parent, componentPath + "/hardware-rev", hardwareRev.value());
        }
        if (firmwareRev) {
            setXpath(session, parent, componentPath + "/firmware-rev", firmwareRev.value());
        }
        if (softwareRev) {
            setXpath(session, parent, componentPath + "/software-rev", softwareRev.value());
        }
        if (serial) {
            setXpath(session, parent, componentPath + "/serial-num", serial.value());
        }
        if (mfgName) {
            setXpath(session, parent, componentPath + "/mfg-name", mfgName.value());
        }
        if (modelName) {
            setXpath(session, parent, componentPath + "/model-name", modelName.value());
        }
        if (alias) {
            setXpath(session, parent, componentPath + "/alias", alias.value());
        }
        if (assetID) {
            setXpath(session, parent, componentPath + "/asset-id", assetID.value());
        }
        // uri to value
        for (auto const& elem : uri) {
            setXpath(session, parent, componentPath + "/uri", elem);
        }
        if (uuid) {
            setXpath(session, parent, componentPath + "/uuid", uuid.value());
        }
    }

    void setValueFromLSHWmap(std::string node, std::string value) {
        if (node == "description") {
            description = value;
        } else if (node == "serial") {
            serial = value;
        } else if (node == "version") {
            hardwareRev = value;
        } else if (node == "vendor") {
            mfgName = value;
        } else if (node == "product") {
            modelName = value;
        } else if (node == "handle") {
            alias = value;
        }
    }

    void printExistingData() const {
        std::cout << "Name: " << name << std::endl;
        std::cout << "Class: " << classType << std::endl;
        if (physicalID) {
            std::cout << "Physid: " << physicalID.value() << std::endl;
        }
        if (description) {
            std::cout << "description: " << description.value() << std::endl;
        }
        if (parentName) {
            std::cout << "parent: " << parentName.value() << std::endl;
        }
        if (parent_rel_pos) {
            std::cout << "parent_rel_pos: " << parent_rel_pos.value() << std::endl;
        }
        if (hardwareRev) {
            std::cout << "hardwareRev: " << hardwareRev.value() << std::endl;
        }
        if (firmwareRev) {
            std::cout << "firmwareRev: " << firmwareRev.value() << std::endl;
        }
        if (softwareRev) {
            std::cout << "softwareRev: " << softwareRev.value() << std::endl;
        }
        if (serial) {
            std::cout << "serial: " << serial.value() << std::endl;
        }
        if (mfgName) {
            std::cout << "mfgName: " << mfgName.value() << std::endl;
        }
        if (modelName) {
            std::cout << "modelName: " << modelName.value() << std::endl;
        }
        if (alias) {
            std::cout << "alias: " << alias.value() << std::endl;
        }
        if (assetID) {
            std::cout << "assetID: " << assetID.value() << std::endl;
        }
        if (uuid) {
            std::cout << "uuid: " << uuid.value() << std::endl;
        }
        if (!sensorThresholds.empty()) {
            std::cout << "sensor thresholds: ";
        }
        for (auto const& c : sensorThresholds) {
            std::cout << c << ", ";
        }
        if (!uri.empty()) {
            std::cout << std::endl << "uri: ";
        }
        for (auto const& c : uri) {
            std::cout << c << ", ";
        }
        if (!children.empty()) {
            std::cout << std::endl << "children: ";
        }
        for (auto const& c : children) {
            std::cout << c << ", ";
        }
        std::cout << std::endl;
    }

    bool checkForConfigMatch(std::shared_ptr<ComponentData> component) {
        // We can't compare optionals w/o checking if values are present because of the following
        // supposition: lhs is considered equal to rhs if, and only if, both lhs and rhs do not
        // contain a value. Given this we can't have nodes with no 'parents' be considered equal.
        if (component->parentName && component->parent_rel_pos && parentName && parent_rel_pos) {
            return (component->classType == classType) && (component->parentName == parentName) &&
                   (component->parent_rel_pos == parent_rel_pos);
        }
        return false;
    }

    void replaceWritableValues(std::shared_ptr<ComponentData> component) {
        name = component->name;
        alias = component->alias;
        assetID = component->assetID;
        uri = component->uri;
    }

    void parseAndSetPhysicalID(std::string physid) {
        size_t foundPos = physid.find('.');
        if (foundPos != std::string::npos) {
            physid = physid.substr(foundPos + 1, physid.length());
        }
        try {
            physicalID = std::stoi(physid, nullptr, 16);
            if (physicalID < 1) {
                physicalID = std::nullopt;
            }
        } catch (std::exception const& e) {
            logMessage(SR_LL_WRN, std::string("Couldn't convert physical-id: ") + e.what());
        }
    }

    static void populateConfigData(sysrepo::S_Session& session, char const* module_name) {
        std::string const data_xpath(std::string("/") + module_name + ":hardware");
        S_Data_Node toplevel(session->get_data(data_xpath.c_str()));
        std::shared_ptr<ComponentData> component;
        std::shared_ptr<SensorThreshold> sensThreshold;
        bool isSensorNotification(false);

        hwConfigData.clear();
        for (S_Data_Node& root : toplevel->tree_for()) {
            for (S_Data_Node const& node : root->tree_dfs()) {
                S_Schema_Node schema = node->schema();
                switch (schema->nodetype()) {
                case LYS_LIST: {
                    if (std::string(schema->name()) == "sensor-threshold") {
                        isSensorNotification = true;
                    } else {
                        isSensorNotification = false;
                    }
                    break;
                }
                case LYS_LEAF: {
                    Data_Node_Leaf_List leaf(node);
                    Schema_Node_Leaf sleaf(schema);
                    if (isSensorNotification) {
                        if (sleaf.is_key() && component) {
                            sensThreshold = std::make_shared<SensorThreshold>(leaf.value_str());
                            component->sensorThresholds.push_back(sensThreshold);
                        } else if (component && sensThreshold) {
                            if (std::string(sleaf.name()) == "max") {
                                sensThreshold->type = SensorThreshold::ThresholdType::max;
                                sensThreshold->value = leaf.value()->int32();
                            } else if (std::string(sleaf.name()) == "min") {
                                sensThreshold->type = SensorThreshold::ThresholdType::min;
                                sensThreshold->value = leaf.value()->int32();
                            }
                        } else if (std::string(sleaf.name()) == "poll-interval") {
                            ComponentData::pollInterval = leaf.value()->uint32();
                        }
                    } else {
                        if (sleaf.is_key()) {
                            component = std::make_shared<ComponentData>(leaf.value_str());
                            hwConfigData.push_back(component);
                        } else if (component) {
                            if (std::string(sleaf.name()) == "class") {
                                component->classType = leaf.value_str();
                            } else if (std::string(sleaf.name()) == "parent") {
                                component->parentName = leaf.value_str();
                            } else if (std::string(sleaf.name()) == "parent-rel-pos") {
                                component->parent_rel_pos = leaf.value()->int32();
                            } else if (std::string(sleaf.name()) == "alias") {
                                component->alias = leaf.value_str();
                            } else if (std::string(sleaf.name()) == "asset-id") {
                                component->assetID = leaf.value_str();
                            }
                        }
                    }
                    break;
                }
                case LYS_LEAFLIST: {
                    Data_Node_Leaf_List leaflist(node);
                    Schema_Node_Leaflist sleaf(schema);
                    if (!isSensorNotification && component && std::string(sleaf.name()) == "uri") {
                        component->uri.emplace_back(leaflist.value_str());
                    }
                    break;
                }
                default:
                    break;
                }
            }
        }
    }

    std::string name;
    std::string classType;

    std::optional<int32_t> physicalID;
    std::optional<std::string> description;
    std::optional<std::string> parentName;
    std::optional<int32_t> parent_rel_pos;
    std::list<std::string> children;
    std::optional<std::string> hardwareRev;
    std::optional<std::string> firmwareRev;
    std::optional<std::string> softwareRev;
    std::optional<std::string> serial;
    std::optional<std::string> mfgName;
    std::optional<std::string> modelName;
    std::optional<std::string> alias;
    std::optional<std::string> assetID;
    std::optional<std::string> uuid;
    std::list<std::string> uri;
    SensorThresholdList sensorThresholds;

    static uint32_t pollInterval;
    static ComponentList hwConfigData;
};

ComponentList ComponentData::hwConfigData;
uint32_t ComponentData::pollInterval = DEFAULT_POLL_INTERVAL;

}  // namespace hardware

#endif  // COMPONENT_DATA_H
