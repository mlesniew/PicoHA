#include "cover.h"

namespace PicoHA {

static String to_string(const Cover::State state) {
    switch (state) {
        case Cover::State::open:
            return F("open");
        case Cover::State::opening:
            return F("opening");
        case Cover::State::closed:
            return F("closed");
        case Cover::State::closing:
            return F("closing");
        case Cover::State::stopped:
            return F("stopped");
    }
    return F("unknown");
}

Cover::Cover(const PicoString & identifier, const PicoString & name)
    : Entity(identifier, name), current_position(0), state(State::stopped) {}

PicoJson Cover::print_autodiscovery_json(const AbstractDevice & device,
                                         Print & out) const {
    auto json = Entity::print_autodiscovery_json(device, out);

    const String prefix = get_topic_prefix(device);

    if (position_getter) {
        json[F("position_topic")] = prefix + F("/position");
    }

    if (position_setter) {
        json[F("set_position_topic")] = prefix + F("/position/set");
    }

    return json;
}

void Cover::begin(AbstractDevice & device) {
    auto & mqtt = device.get_mqtt();

    if (command_setter) {
        mqtt.subscribe(
            get_command_topic(device),
            [this](const char * payload) {
                if (!command_setter) return;
                if (strcmp_P(payload, PSTR("OPEN")) == 0) {
                    command_setter(Command::open);
                } else if (strcmp_P(payload, PSTR("CLOSE")) == 0) {
                    command_setter(Command::close);
                } else if (strcmp_P(payload, PSTR("STOP")) == 0) {
                    command_setter(Command::stop);
                }
            },
            6);
    }

    if (position_setter) {
        mqtt.subscribe(
            get_topic_prefix(device) + F("/position/set"),
            [this](const char * payload) {
                if (!position_setter) return;
                int value = atoi(payload);
                if (value >= 0 && value <= 100) {
                    position_setter(value);
                }
            },
            4);
    }
}

void Cover::loop(AbstractDevice & device) {
    if (position_getter) {
        double new_position = position_getter();

        if (new_position != current_position) {
            publish_position(device, new_position);
        }
    }

    if (state_getter) {
        State new_state = state_getter();

        if (new_state != state) {
            publish_state(device, new_state);
        }
    }
}

void Cover::fire(AbstractDevice & device) {
    if (position_getter) {
        publish_position(device, position_getter());
    }

    if (state_getter) {
        publish_state(device, state_getter());
    }
}

void Cover::end(AbstractDevice & device) {
    auto & mqtt = device.get_mqtt();

    if (command_setter) {
        mqtt.unsubscribe(get_command_topic(device));
    }

    if (position_setter) {
        mqtt.unsubscribe(get_topic_prefix(device) + F("/position/set"));
    }
}

void Cover::publish_position(AbstractDevice & device, int new_position) {
    current_position = new_position;
    device.get_mqtt().publish(get_topic_prefix(device) + F("/position"),
                              String(current_position));
}

void Cover::publish_state(AbstractDevice & device, State new_state) {
    state = new_state;
    device.get_mqtt().publish(get_state_topic(device), to_string(state));
}

}  // namespace PicoHA
