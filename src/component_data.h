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
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace hardware {

struct ComponentData {

    void setXpathForAllMembers(sysrepo::S_Session& session,
                               libyang::S_Data_Node& parent,
                               std::string const& mainXpath) const {
        std::string componentPath(mainXpath + "/component[name='" + name + "']");
        logMessage(SR_LL_DBG, "Setting values for component: " + name);
        // +--rw name              string
        // +--ro description?      string
        // +--ro hardware-rev?     string
        // +--ro serial-num?       string
        // +--ro mfg-name?         string
        // +--ro model-name?       string
        // +--rw alias?            string
        setXpath(session, parent, componentPath + "/class", classType);

        if (description) {
            setXpath(session, parent, componentPath + "/description", description.value());
        }
        if (hardwareRev) {
            setXpath(session, parent, componentPath + "/hardware-rev", hardwareRev.value());
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
        // +--ro software-rev?     string
        // +--ro firmware-rev?     string
        // +--ro uuid?             yang:uuid
        if (softwareRev) {
            setXpath(session, parent, componentPath + "/software-rev", softwareRev.value());
        }
        if (firmwareRev) {
            setXpath(session, parent, componentPath + "/firmware-rev", firmwareRev.value());
        }
        if (uuid) {
            setXpath(session, parent, componentPath + "/uuid", uuid.value());
        }
        // +--rw parent?           -> ../../component/name
        // +--rw parent-rel-pos?   int32
        if (parentName) {
            setXpath(session, parent, componentPath + "/parent", parentName.value());
        }
        if (parent_rel_pos) {
            setXpath(session, parent, componentPath + "/parent-rel-pos",
                     std::to_string(parent_rel_pos.value()));
        }
        // childlist to value
        // +--ro contains-child*   -> ../../component/name
        for (auto const& elem : children) {
            setXpath(session, parent, componentPath + "/contains-child", elem);
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

        std::cout << "children: ";
        for (auto const& c : children) {
            std::cout << c << ", ";
        }
        std::cout << std::endl;
    }
    std::string name;
    std::string classType;
    std::optional<int32_t> physicalID;
    std::optional<std::string> description;
    std::optional<std::string> parentName;
    std::optional<int32_t> parent_rel_pos;
    std::vector<std::string> children;
    std::optional<std::string> hardwareRev;
    std::optional<std::string> firmwareRev;
    std::optional<std::string> softwareRev;
    std::optional<std::string> serial;
    std::optional<std::string> mfgName;
    std::optional<std::string> modelName;
    std::optional<std::string> alias;
    std::optional<std::string> assetID;
    std::optional<std::string> uuid;
};

}  // namespace hardware

#endif  // COMPONENT_DATA_H
