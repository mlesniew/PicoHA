#include "climate.h"

namespace PicoHA {

static String to_string(const Climate::Mode mode) {
    switch (mode) {
        case Climate::Mode::off:
            return F("off");
        case Climate::Mode::heat:
            return F("heat");
        case Climate::Mode::automatic:
            return F("auto");
        case Climate::Mode::cool:
            return F("cool");
        case Climate::Mode::dry:
            return F("dry");
        case Climate::Mode::fan_only:
            return F("fan_only");
        default:
            return F("unknown");
    }
}

static String to_string(const Climate::Action action) {
    switch (action) {
        case Climate::Action::off:
            return F("off");
        case Climate::Action::heating:
            return F("heating");
        case Climate::Action::cooling:
            return F("cooling");
        case Climate::Action::drying:
            return F("drying");
        case Climate::Action::idle:
            return F("idle");
        case Climate::Action::fan:
            return F("fan");
        default:
            return F("unknown");
    }
}

Climate::Climate(AbstractDevice & device, const PicoString & identifier,
                 const PicoString & name)
    : Entity(device, identifier, name),
      min_temp(std::numeric_limits<double>::quiet_NaN()),
      max_temp(std::numeric_limits<double>::quiet_NaN()),
      temp_step(std::numeric_limits<double>::quiet_NaN()),
      temperature_unit(TemperatureUnit::celsius),
      modes({Mode::off | Mode::heat | Mode::automatic | Mode::cool | Mode::dry |
             Mode::fan_only}),
      power(false),
      mode(Mode::off),
      action(Action::off),
      target_temperature(std::numeric_limits<double>::quiet_NaN()),
      current_temperature(std::numeric_limits<double>::quiet_NaN()) {}

JsonDocument Climate::get_autodiscovery_json() const {
    JsonDocument json = Entity::get_autodiscovery_json();
    {
        unsigned int idx = 0;
        for (unsigned char mode_bit = 1; mode_bit != 0; mode_bit <<= 1) {
            if ((static_cast<unsigned char>(modes) & mode_bit) != 0) {
                json[F("modes")][idx++] =
                    to_string(static_cast<Mode>(mode_bit));
            }
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
        json[F("power_command_topic")] = get_topic_prefix() + F("/power/set");
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

void Climate::begin() {
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
        get_mqtt().subscribe(
            get_topic_prefix() + F("/mode/set"),
            [this](const String & payload) {
                if (!mode_setter) return;

                for (unsigned char mode_bit = 1; mode_bit != 0;
                     mode_bit <<= 1) {
                    if ((static_cast<unsigned char>(modes) & mode_bit) != 0) {
                        Climate::Mode mode =
                            static_cast<Climate::Mode>(mode_bit);
                        if (payload == to_string(mode)) {
                            mode_setter(mode);
                            break;
                        }
                    }
                }
            });
    }

    if (target_temperature_setter) {
        get_mqtt().subscribe(get_topic_prefix() + F("/target_temperature/set"),
                             [this](const String & payload) {
                                 if (!target_temperature_setter) return;
                                 target_temperature_setter(payload.toDouble());
                             });
    }
}

void Climate::tick() {
    if (power_getter) {
        bool new_power = power_getter();
        if (new_power != power) {
            publish_power(new_power);
        }
    }

    if (mode_getter) {
        Mode new_mode = mode_getter();
        if (new_mode != mode) {
            publish_mode(new_mode);
        }
    }

    if (action_getter) {
        Action new_action = action_getter();
        if (new_action != action) {
            publish_action(new_action);
        }
    }

    if (target_temperature_getter) {
        double new_target_temperature = target_temperature_getter();
        if ((std::isnan(new_target_temperature) !=
             std::isnan(target_temperature)) ||
            ((!std::isnan(new_target_temperature) &&
              std::abs(new_target_temperature - target_temperature) >= 0.01))) {
            publish_target_temperature(new_target_temperature);
        }
    }

    if (current_temperature_getter) {
        double new_current_temperature = current_temperature_getter();
        if ((std::isnan(new_current_temperature) !=
             std::isnan(current_temperature)) ||
            ((!std::isnan(new_current_temperature) &&
              std::abs(new_current_temperature - current_temperature) >=
                  0.01))) {
            publish_current_temperature(new_current_temperature);
        }
    }
}

void Climate::fire() {
    if (power_getter) {
        publish_power(power_getter());
    }
    if (mode_getter) {
        publish_mode(mode_getter());
    }
    if (action_getter) {
        publish_action(action_getter());
    }
    if (target_temperature_getter) {
        publish_target_temperature(target_temperature_getter());
    }
    if (current_temperature_getter) {
        publish_current_temperature(current_temperature_getter());
    }
}

void Climate::publish_power(bool new_power) {
    power = new_power;
    get_mqtt().publish(get_topic_prefix() + F("/power"),
                       to_string_default(power));
}

void Climate::publish_mode(Mode new_mode) {
    mode = new_mode;
    get_mqtt().publish(get_topic_prefix() + F("/mode"), to_string(mode));
}

void Climate::publish_action(Action new_action) {
    action = new_action;
    get_mqtt().publish(get_topic_prefix() + F("/action"), to_string(action));
}

void Climate::publish_target_temperature(double new_target_temperature) {
    target_temperature = new_target_temperature;
    get_mqtt().publish(get_topic_prefix() + F("/target_temperature"),
                       String(target_temperature, 2));
}

void Climate::publish_current_temperature(double new_current_temperature) {
    current_temperature = new_current_temperature;
    get_mqtt().publish(get_topic_prefix() + F("/current_temperature"),
                       String(current_temperature, 2));
}

}  // namespace PicoHA
