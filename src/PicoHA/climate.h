#pragma once

#include <cmath>

#include "entity.h"

namespace PicoHA {

class Climate : public Entity {
public:
    enum class Action { off, heating, cooling, drying, idle, fan };
    enum class Mode : unsigned char {
        off = 1 << 0,
        automatic = 1 << 1,
        cool = 1 << 2,
        heat = 1 << 3,
        dry = 1 << 4,
        fan_only = 1 << 5
    };
    enum class TemperatureUnit : char { celsius = 'C', fahrenheit = 'F' };

    Climate(const PicoString & identifier, const PicoString & name);

    virtual PicoJson print_autodiscovery_json(const AbstractDevice & device,
                                              Print & out) const override;

    double min_temp, max_temp, temp_step;
    TemperatureUnit temperature_unit;

    Mode modes;

    void bind_action(const Action * action) {
        action_getter =
            PicoCallback<Action>([](const Action * a) { return *a; }, action);
    }

    void bind_power(bool * power) {
        power_getter =
            PicoCallback<bool>([](const bool * p) { return *p; }, power);
        power_setter = PicoCallback<void, bool>(
            [](bool * p, bool new_value) { *p = new_value; }, power);
    }

    void bind_mode(Mode * mode) {
        mode_getter =
            PicoCallback<Mode>([](const Mode * m) { return *m; }, mode);
        mode_setter = PicoCallback<void, Mode>(
            [](Mode * m, Mode new_mode) { *m = new_mode; }, mode);
    }

    void bind_current_temperature(double * current_temperature) {
        current_temperature_getter = PicoCallback<double>(
            [](const double * t) { return *t; }, current_temperature);
    }

    void bind_target_temperature(double * target_temperature) {
        target_temperature_getter = PicoCallback<double>(
            [](const double * t) { return *t; }, target_temperature);
        target_temperature_setter = PicoCallback<void, double>(
            [](double * t, double new_value) { *t = new_value; },
            target_temperature);
    }

    PicoCallback<Action> action_getter;

    PicoCallback<bool> power_getter;
    PicoCallback<void, bool> power_setter;

    PicoCallback<Mode> mode_getter;
    PicoCallback<void, Mode> mode_setter;

    PicoCallback<double> current_temperature_getter;
    PicoCallback<double> target_temperature_getter;
    PicoCallback<void, double> target_temperature_setter;

protected:
    void begin(AbstractDevice & device) override;
    void tick(AbstractDevice & device) override;
    void fire(AbstractDevice & device) override;
    void end(AbstractDevice & device) override;

    virtual String get_platform() const override { return F("climate"); }

private:
    void publish_power(AbstractDevice & device, bool new_power);
    void publish_mode(AbstractDevice & device, Mode new_mode);
    void publish_action(AbstractDevice & device, Action new_action);
    void publish_target_temperature(AbstractDevice & device,
                                    double new_target_temperature);
    void publish_current_temperature(AbstractDevice & device,
                                     double new_current_temperature);

    bool power;
    Mode mode;
    Action action;
    double target_temperature;
    double current_temperature;
};

inline Climate::Mode operator|(Climate::Mode a, Climate::Mode b) {
    return static_cast<Climate::Mode>(static_cast<unsigned char>(a) |
                                      static_cast<unsigned char>(b));
}

inline bool operator&(Climate::Mode a, Climate::Mode b) {
    return static_cast<unsigned char>(a) & static_cast<unsigned char>(b);
}

inline Climate & AbstractDevice::addClimate(const PicoString & id,
                                            const PicoString & name) {
    return addEntity<Climate>(id, name);
}

}  // namespace PicoHA