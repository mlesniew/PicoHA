#pragma once
#include <algorithm>
#include <vector>

#include "entity.h"

namespace PicoHA {

class Button : public EntityWithCommand {
public:
    using EntityWithCommand::EntityWithCommand;

    PicoCallback<void> on_press;

protected:
    virtual String get_platform() const override { return F("button"); }

    virtual void on_command(const String & command) override {
        if (on_press && command == F("PRESS")) {
            on_press();
        }
    }
};

template <typename T>
class InputEntity : public EntityWithState<T> {
public:
    InputEntity(const PicoString & identifier, const PicoString & name)
        : EntityWithState<T>(identifier, name) {}

protected:
    virtual void begin(AbstractDevice & device) override {
        EntityWithState<T>::begin(device);
        device.get_mqtt().subscribe(
            get_command_topic(device),
            [this](const String & payload) { on_command(payload); });
    }

    virtual void end(AbstractDevice & device) override {
        device.get_mqtt().unsubscribe(get_command_topic(device));
    }

    virtual String get_command_topic(
        const AbstractDevice & device) const override {
        return this->get_topic_prefix(device) + F("/set");
    }

    virtual void on_command(const String & command) = 0;

public:
    virtual void bind(T * value) override {
        EntityWithState<T>::bind(value);
        setter = PicoCallback<void, T>(
            [](T * v, T new_value) { *v = new_value; }, value);
    }

    PicoCallback<void, T> setter;
};

template <typename T>
class Number : public InputEntity<T> {
public:
    Number(const PicoString & identifier, const PicoString & name)
        : InputEntity<T>(identifier, name), min(1), max(100), step(1) {}

    virtual PicoJson print_autodiscovery_json(const AbstractDevice & device,
                                              Print & out) const override {
        PicoJson json = Entity::print_autodiscovery_json(device, out);
        json[F("min")] = min;
        json[F("max")] = max;
        json[F("step")] = step;
        return json;
    }

    T min;
    T max;
    T step;

protected:
    virtual String get_platform() const override { return F("number"); }

    virtual void on_command(const String & command) override {
        if (this->setter) {
            this->setter((T)command.toDouble());
        }
    };
};

class Text : public InputEntity<String> {
public:
    Text(const PicoString & identifier, const PicoString & name)
        : InputEntity(identifier, name), min(0), max(0), is_password(false) {}

    virtual PicoJson print_autodiscovery_json(const AbstractDevice & device,
                                              Print & out) const override {
        PicoJson json = Entity::print_autodiscovery_json(device, out);
        if (min) json[F("min")] = min;
        if (max) json[F("max")] = max;
        if (!pattern.isEmpty()) {
            json[F("pattern")] = pattern;
        }
        if (is_password) {
            json[F("mode")] = F("password");
        }
        return json;
    }

    size_t min;
    size_t max;
    String pattern;
    bool is_password;

protected:
    virtual String get_platform() const override { return F("text"); }

    virtual void on_command(const String & command) override {
        if (setter) {
            setter(command);
        }
    };
};

class Switch : public InputEntity<bool> {
public:
    Switch(const PicoString & identifier, const PicoString & name)
        : InputEntity(identifier, name) {}

protected:
    virtual String get_platform() const override { return F("switch"); }

    virtual void on_command(const String & command) override {
        if (!setter) {
            return;
        }

        if (command == F("ON")) {
            setter(true);
        } else if (command == F("OFF")) {
            setter(false);
        }
    };
};

class Select : public InputEntity<String> {
public:
    Select(const PicoString & identifier, const PicoString & name)
        : InputEntity(identifier, name) {}

    virtual PicoJson print_autodiscovery_json(const AbstractDevice & device,
                                              Print & out) const override {
        PicoJson json = Entity::print_autodiscovery_json(device, out);
        {
            auto options_list = json[F("options")];
            for (const String & event_type : options) {
                options_list.append() = event_type;
            }
        }
        return json;
    }

    std::vector<PicoString> options;

protected:
    virtual String get_platform() const override { return F("select"); }

    virtual void on_command(const String & command) override {
        if (setter && std::find(options.begin(), options.end(), command) !=
                          options.end()) {
            setter(command);
        }
    };
};

inline Button & AbstractDevice::addButton(const PicoString & id,
                                          const PicoString & name) {
    return addEntity<Button>(id, name);
}
inline Switch & AbstractDevice::addSwitch(const PicoString & id,
                                          const PicoString & name) {
    return addEntity<Switch>(id, name);
}
inline Text & AbstractDevice::addText(const PicoString & id,
                                      const PicoString & name) {
    return addEntity<Text>(id, name);
}
inline Select & AbstractDevice::addSelect(const PicoString & id,
                                          const PicoString & name) {
    return addEntity<Select>(id, name);
}
template <typename T>
inline Number<T> & AbstractDevice::addNumber(const PicoString & id,
                                             const PicoString & name) {
    return addEntity<Number<T>>(id, name);
}

}  // namespace PicoHA