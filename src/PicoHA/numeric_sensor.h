#pragma once

#include "sensor.h"

namespace PicoHA {

template <typename T>
class NumericSensor : public Sensor<T> {
public:
    NumericSensor(AbstractDevice & device, const String & identifier,
                  const String & name = "")
        : Entity(device, identifier, name),
          Sensor<T>(device, identifier, name),
          suggested_display_precision(-1) {}

    JsonDocument get_autodiscovery_json() const override {
        JsonDocument json = Sensor<T>::get_autodiscovery_json();
        json["unit_of_measurement"] = unit_of_measurement.length()
                                          ? unit_of_measurement.c_str()
                                          : nullptr;
        if (suggested_display_precision >= 0) {
            json["suggested_display_precision"] = suggested_display_precision;
        }
        json["state_class"] =
            state_class.length() ? state_class : "measurement";
        return json;
    }

    String unit_of_measurement;
    int suggested_display_precision;
    String state_class;
};

}  // namespace PicoHA