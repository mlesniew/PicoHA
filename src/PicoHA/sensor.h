#pragma once

#include <functional>

#include "entity.h"

namespace PicoHA {

template <typename T, String (*to_string)(const T) = to_string_default<T>>
class Sensor : public EntityWithState<T, to_string> {
public:
    Sensor(AbstractDevice & device, const SmartString & identifier,
           const SmartString & name)
        : Entity(device, identifier, name),
          EntityWithState<T, to_string>(device, identifier, name) {}

protected:
    virtual String get_platform() const override { return F("sensor"); }
};

template <typename T>
class NumericSensor : public Sensor<T> {
public:
    NumericSensor(AbstractDevice & device, const SmartString & identifier,
                  const SmartString & name = "")
        : Entity(device, identifier, name),
          Sensor<T>(device, identifier, name),
          suggested_display_precision(-1) {}

    JsonDocument get_autodiscovery_json() const override {
        JsonDocument json = Sensor<T>::get_autodiscovery_json();

        json[F("unit_of_measurement")] =
            !unit_of_measurement.isEmpty() ? String(unit_of_measurement).c_str()
                                           : nullptr;
        if (suggested_display_precision >= 0) {
            json[F("suggested_display_precision")] =
                suggested_display_precision;
        }
        json[F("state_class")] =
            !state_class.isEmpty() ? state_class : F("measurement");
        return json;
    }

    SmartString unit_of_measurement;
    int suggested_display_precision;
    SmartString state_class;
};

template <typename T, String (*to_string)(const T)>
class EnumSensor : public Sensor<T, to_string> {
public:
    EnumSensor(AbstractDevice & device, const SmartString & identifier,
               const SmartString & name = "")
        : Entity(device, identifier, name),
          Sensor<T, to_string>(device, identifier, name) {}
};

}  // namespace PicoHA