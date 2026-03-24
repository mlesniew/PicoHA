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
        action_getter = [action] { return *action; };
    }

    void bind_power(bool * power) {
        power_getter = [power] { return *power; };
        power_setter = [power](bool new_value) { *power = new_value; };
    }

    void bind_mode(Mode * mode) {
        mode_getter = [mode] { return *mode; };
        mode_setter = [mode](Mode new_mode) { *mode = new_mode; };
    }

    void bind_current_temperature(double * current_temperature) {
        current_temperature_getter = [current_temperature] {
            return *current_temperature;
        };
    }

    void bind_target_temperature(double * target_temperature) {
        target_temperature_getter = [target_temperature] {
            return *target_temperature;
        };
        target_temperature_setter = [target_temperature](double new_value) {
            *target_temperature = new_value;
        };
    }

    std::function<Action()> action_getter;

    std::function<bool()> power_getter;
    std::function<void(bool)> power_setter;

    std::function<Mode()> mode_getter;
    std::function<void(Mode)> mode_setter;

    std::function<double()> current_temperature_getter;

    std::function<double()> target_temperature_getter;
    std::function<void(double)> target_temperature_setter;

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