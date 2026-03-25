#pragma once

#include "entity.h"

namespace PicoHA {

template <typename T, String (*to_string)(const T) = to_string_default<T>>
class Sensor : public EntityWithState<T, to_string> {
public:
    Sensor(const PicoString & identifier, const PicoString & name)
        : EntityWithState<T, to_string>(identifier, name) {}

protected:
    virtual String get_platform() const override { return F("sensor"); }
};

template <typename T>
class NumericSensor : public Sensor<T> {
public:
    NumericSensor(const PicoString & identifier, const PicoString & name = "")
        : Sensor<T>(identifier, name), suggested_display_precision(-1) {}

    PicoJson print_autodiscovery_json(const AbstractDevice & device,
                                      Print & out) const override {
        PicoJson json = Sensor<T>::print_autodiscovery_json(device, out);

        if (unit_of_measurement.isEmpty()) {
            json[F("unit_of_measurement")] = nullptr;
        } else {
            json[F("unit_of_measurement")] = unit_of_measurement;
        }
        if (suggested_display_precision >= 0) {
            json[F("suggested_display_precision")] =
                suggested_display_precision;
        }
        json[F("state_class")] =
            !state_class.isEmpty() ? state_class : F("measurement");
        return json;
    }

    PicoString unit_of_measurement;
    int suggested_display_precision;
    PicoString state_class;
};

template <typename T, String (*to_string)(const T)>
class EnumSensor : public Sensor<T, to_string> {
public:
    EnumSensor(const PicoString & identifier, const PicoString & name = "")
        : Sensor<T, to_string>(identifier, name) {}
};

class BinarySensor : public Sensor<bool, to_string_default<bool>> {
public:
    BinarySensor(const PicoString & identifier, const PicoString & name)
        : Sensor<bool, to_string_default<bool>>(identifier, name) {}

protected:
    virtual String get_platform() const override { return F("binary_sensor"); }
};

inline BinarySensor & AbstractDevice::addBinarySensor(const PicoString & id,
                                                      const PicoString & name) {
    return addEntity<BinarySensor>(id, name);
}

template <typename T>
inline Sensor<T, to_string_default<T>> & AbstractDevice::addSensor(
    const PicoString & id, const PicoString & name) {
    return addEntity<Sensor<T, to_string_default<T>>>(id, name);
}

template <typename T>
inline NumericSensor<T> & AbstractDevice::addNumericSensor(
    const PicoString & id, const PicoString & name) {
    return addEntity<NumericSensor<T>>(id, name);
}

}  // namespace PicoHA