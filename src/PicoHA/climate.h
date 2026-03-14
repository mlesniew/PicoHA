#pragma once

#include <PicoUtils.h>

#include <cmath>

#include "entity.h"

namespace PicoHA {

class Climate : public Entity {
public:
    enum class Action { off, heating, cooling, drying, idle, fan };
    enum class Mode { automatic, off, cool, heat, dry, fan_only };
    enum class TemperatureUnit : char { celsius = 'C', fahrenheit = 'F' };

    Climate(AbstractDevice & device, const String & identifier,
            const String & name);

    virtual JsonDocument get_autodiscovery_json() const override;

    void begin() override;
    void tick() override;
    void fire() override;

    double min_temp, max_temp, temp_step;
    TemperatureUnit temperature_unit;

    std::set<Mode> modes;

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
    virtual String get_platform() const override { return F("climate"); }

    PicoUtils::Watch<bool> power_watcher;
    PicoUtils::Watch<Mode> mode_watcher;
    PicoUtils::Watch<Action> action_watcher;
    PicoUtils::Watch<double> target_temperature_watcher;
    PicoUtils::Watch<double> current_temperature_watcher;
};

}  // namespace PicoHA