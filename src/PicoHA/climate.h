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

    static String to_string(const Mode mode) {
        switch (mode) {
            case Mode::off:
                return F("off");
            case Mode::heat:
                return F("heat");
            case Mode::automatic:
                return F("auto");
            case Mode::cool:
                return F("cool");
            case Mode::dry:
                return F("dry");
            case Mode::fan_only:
                return F("fan_only");
            default:
                return F("unknown");
        }
    }

    static String to_string(const Action action) {
        switch (action) {
            case Action::off:
                return F("off");
            case Action::heating:
                return F("heating");
            case Action::cooling:
                return F("cooling");
            case Action::drying:
                return F("drying");
            case Action::idle:
                return F("idle");
            case Action::fan:
                return F("fan");
            default:
                return F("unknown");
        }
    }

    Climate(AbstractDevice & device, const String & identifier,
            const String & name)
        : Entity(device, identifier, name),
          min_temp(std::numeric_limits<double>::quiet_NaN()),
          max_temp(std::numeric_limits<double>::quiet_NaN()),
          temp_step(std::numeric_limits<double>::quiet_NaN()),
          temperature_unit(TemperatureUnit::celsius),
          modes({Mode::off, Mode::heat, Mode::automatic, Mode::cool, Mode::dry,
                 Mode::fan_only}),
          power_watcher([this] { return power_getter ? power_getter() : true; },
                        [this](bool new_value) {
                            get_mqtt().publish(get_topic_prefix() + F("/power"),
                                               new_value ? F("ON") : F("OFF"));
                        }),
          mode_watcher(
              [this] { return mode_getter ? mode_getter() : Mode::off; },
              [this](Mode new_mode) {
                  get_mqtt().publish(get_topic_prefix() + F("/mode"),
                                     to_string(new_mode));
              }),
          action_watcher(
              [this] { return action_getter ? action_getter() : Action::off; },
              [this](Action new_action) {
                  get_mqtt().publish(get_topic_prefix() + F("/action"),
                                     to_string(new_action));
              }),
          target_temperature_watcher(
              [this] {
                  return target_temperature_getter
                             ? target_temperature_getter()
                             : std::numeric_limits<double>::quiet_NaN();
              },
              [this](double new_temperature) {
                  get_mqtt().publish(
                      get_topic_prefix() + F("/target_temperature"),
                      String(new_temperature, 2));
              }),
          current_temperature_watcher(
              [this] {
                  return current_temperature_getter
                             ? current_temperature_getter()
                             : std::numeric_limits<double>::quiet_NaN();
              },
              [this](double new_temperature) {
                  get_mqtt().publish(
                      get_topic_prefix() + F("/current_temperature"),
                      String(new_temperature, 2));
              }) {}

    virtual JsonDocument get_autodiscovery_json() const override {
        JsonDocument json = Entity::get_autodiscovery_json();
        {
            unsigned int idx = 0;
            for (const Mode mode : modes) {
                json[F("modes")][idx++] = to_string(mode);
            }
        }
        if (!std::isnan(min_temp)) {
            json[F("min_temp")] = min_temp;
        }
        if (!std::isnan(max_temp)) {
            json[F("max_temp")] = max_temp;
        }
        if (!std::isnan(temp_step)) {
            json[F("temp_step")] = temp_step;
        }
        json[F("temperature_unit")] =
            temperature_unit == TemperatureUnit::fahrenheit ? F("F") : F("C");

        if (action_getter) {
            json[F("action_topic")] = get_topic_prefix() + F("/action");
        }
        if (power_getter) {
            json[F("power_topic")] = get_topic_prefix() + F("/power");
        }
        if (mode_getter) {
            json[F("mode_state_topic")] = get_topic_prefix() + F("/mode");
        }
        if (current_temperature_getter) {
            json[F("current_temperature_topic")] =
                get_topic_prefix() + F("/current_temperature");
        }
        if (target_temperature_getter) {
            json[F("target_temperature_topic")] =
                get_topic_prefix() + F("/target_temperature");
        }

        if (power_setter) {
            json[F("power_command_topic")] =
                get_topic_prefix() + F("/power/set");
        }
        if (mode_setter) {
            json[F("mode_command_topic")] = get_topic_prefix() + F("/mode/set");
        }
        if (target_temperature_setter) {
            json[F("temperature_command_topic")] =
                get_topic_prefix() + F("/target_temperature/set");
        }

        return json;
    }

    void begin() override {
        if (power_setter) {
            get_mqtt().subscribe(get_topic_prefix() + F("/power/set"),
                                 [this](const String & payload) {
                                     if (!power_setter) return;

                                     if (payload == F("ON")) {
                                         power_setter(true);
                                     } else if (payload == F("OFF")) {
                                         power_setter(false);
                                     }
                                 });
        }

        if (mode_setter) {
            get_mqtt().subscribe(get_topic_prefix() + F("/mode/set"),
                                 [this](const String & payload) {
                                     if (!mode_setter) return;

                                     for (const Mode mode : modes) {
                                         if (payload == to_string(mode)) {
                                             mode_setter(mode);
                                             break;
                                         }
                                     }
                                 });
        }

        if (target_temperature_setter) {
            get_mqtt().subscribe(
                get_topic_prefix() + F("/target_temperature/set"),
                [this](const String & payload) {
                    if (!target_temperature_setter) return;
                    target_temperature_setter(payload.toDouble());
                });
        }
    }

    void tick() override {
        power_watcher.tick();
        mode_watcher.tick();
        action_watcher.tick();
        target_temperature_watcher.tick();
        current_temperature_watcher.tick();
    }

    void fire() override {
        if (power_getter) power_watcher.fire();
        if (mode_getter) mode_watcher.fire();
        if (action_getter) action_watcher.fire();
        if (target_temperature_getter) target_temperature_watcher.fire();
        if (current_temperature_getter) current_temperature_watcher.fire();
    }

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