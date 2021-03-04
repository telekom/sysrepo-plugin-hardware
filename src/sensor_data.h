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

#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <utils/globals.h>

#include <sensors/sensors.h>

#include <chrono>
#include <map>
#include <math.h>
#include <stdint.h>
#include <vector>

struct SensorsInitFail : public std::exception {
    const char* what() const throw() override {
        return "sensor_init() failure";
    }
};

namespace hardware {

struct Sensor {

    Sensor(std::string const& sensorName)
        : name(sensorName), value(0), valueType(ValueType::unknown), valueScale(ValueScale::units),
          valuePrecision(0),
          valueTimestamp(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())){};

    enum class ValueType {
        other = 1,
        unknown = 2,
        volts_ac = 3,
        volts_dc = 4,
        amperes = 5,
        watts = 6,
        hertz = 7,
        celsius = 8,
        percent_rh = 9,
        rpm = 10,
        cmm = 11,
        truth_value = 12
    };

    enum class ValueScale {
        yocto = 1,
        zepto = 2,
        atto = 3,
        femto = 4,
        pico = 5,
        nano = 6,
        micro = 7,
        milli = 8,
        units = 9,
        kilo = 10,
        mega = 11,
        giga = 12,
        tera = 13,
        peta = 14,
        exa = 15,
        zetta = 16,
        yotta = 17
    };

    static std::string getValueTypeForModel(ValueType inputType) {
        std::string returnedType;
        static std::unordered_map<ValueType, std::string> _{
            {ValueType::other, "other"},
            {ValueType::unknown, "unknown"},
            {ValueType::volts_ac, "volts-AC"},
            {ValueType::volts_dc, "volts-DC"},
            {ValueType::amperes, "amperes"},
            {ValueType::watts, "watts"},
            {ValueType::hertz, "hertz"},
            {ValueType::celsius, "celsius"},
            {ValueType::percent_rh, "percent-RH"},
            {ValueType::rpm, "rpm"},
            {ValueType::cmm, "cmm"},
            {ValueType::truth_value, "truth-value"}};
        if (_.find(inputType) != _.end()) {
            returnedType = _.at(inputType);
        }
        return returnedType;
    }

    bool setValueFromSubfeature(sensors_chip_name const* cn,
                                sensors_feature const* feature,
                                sensors_subfeature_type type,
                                int32_t precision) {
        double val;
        sensors_subfeature const* subf = sensors_get_subfeature(cn, feature, type);
        if (!subf) {
            return false;
        }
        if (subf->flags & SENSORS_MODE_R) {
            int rc = sensors_get_value(cn, subf->number, &val);
            if (rc < 0) {
                logMessage(SR_LL_WRN,
                           std::string("setValueFromSubfeature(): ") + std::to_string(rc));
                return false;
            } else {
                logMessage(SR_LL_DBG, std::string("Got sensor: ") + cn->prefix + "/" +
                                          feature->name + " value " + std::to_string(val));
                value = val * std::pow(10, precision);
                return true;
            }
        } else {
            logMessage(SR_LL_WRN, std::string("Couldn't read sensor: ") + cn->prefix + "/" +
                                      feature->name + "/" + subf->name);
            return false;
        }
    }

    std::string name;
    int32_t value;
    ValueType valueType;
    ValueScale valueScale;
    int32_t valuePrecision;
    std::time_t valueTimestamp;
};

struct HardwareSensors {

    HardwareSensors() {
        if (sensors_init(nullptr) != 0) {
            throw SensorsInitFail();
        }
    }

    ~HardwareSensors() {
        sensors_cleanup();
    }

    std::vector<Sensor> parseSensorData() {
        std::vector<Sensor> sensors;
        sensors_chip_name const* cn = nullptr;
        int c = 0;
        while ((cn = sensors_get_detected_chips(0, &c))) {
            sensors_feature const* feature = nullptr;
            int f = 0;
            while ((feature = sensors_get_features(cn, &f))) {
                Sensor tempSensor(std::string(cn->prefix) + "/" + feature->name);
                bool result(false);
                switch (feature->type) {
                case SENSORS_FEATURE_IN:
                    tempSensor.valueType = Sensor::ValueType::volts_dc;
                    tempSensor.valuePrecision = 3;
                    result = tempSensor.setValueFromSubfeature(
                        cn, feature, SENSORS_SUBFEATURE_IN_INPUT, tempSensor.valuePrecision);
                    break;
                case SENSORS_FEATURE_CURR:
                    tempSensor.valueType = Sensor::ValueType::amperes;
                    tempSensor.valuePrecision = 3;
                    result = tempSensor.setValueFromSubfeature(
                        cn, feature, SENSORS_SUBFEATURE_CURR_INPUT, tempSensor.valuePrecision);
                    break;
                case SENSORS_FEATURE_TEMP:
                    tempSensor.valueType = Sensor::ValueType::celsius;
                    result = tempSensor.setValueFromSubfeature(
                        cn, feature, SENSORS_SUBFEATURE_TEMP_INPUT, tempSensor.valuePrecision);
                    break;
                case SENSORS_FEATURE_FAN:
                    tempSensor.valueType = Sensor::ValueType::rpm;
                    result = tempSensor.setValueFromSubfeature(
                        cn, feature, SENSORS_SUBFEATURE_FAN_INPUT, tempSensor.valuePrecision);
                    break;
                case SENSORS_FEATURE_POWER:
                    tempSensor.valueType = Sensor::ValueType::watts;
                    result = tempSensor.setValueFromSubfeature(
                        cn, feature, SENSORS_SUBFEATURE_POWER_INPUT, tempSensor.valuePrecision);
                    break;
                case SENSORS_FEATURE_HUMIDITY:
                    tempSensor.valueType = Sensor::ValueType::percent_rh;
                    result = tempSensor.setValueFromSubfeature(
                        cn, feature, SENSORS_SUBFEATURE_HUMIDITY_INPUT, tempSensor.valuePrecision);
                    break;
                default:
                    tempSensor.valueType = Sensor::ValueType::other;
                    break;
                }
                if (result) {
                    sensors.emplace_back(tempSensor);
                }
            }
        }
        return sensors;
    }
};

}  // namespace hardware

#endif  // SENSOR_DATA_H
