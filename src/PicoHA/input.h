#pragma once
#include <functional>

#include "entity.h"

namespace PicoHA {

class Button : public EntityWithCommand {
public:
    using EntityWithCommand::EntityWithCommand;

    std::function<void()> on_press;

protected:
    virtual String get_platform() const override { return F("button"); }

    virtual void on_command(const String & command) override {
        if (on_press && command == F("PRESS")) {
            on_press();
        }
    }
};

template <typename T>
class InputEntity : public EntityWithCommand, public EntityWithState<T> {
public:
    InputEntity(AbstractDevice & device, const String & identifier,
                const String & name)
        : Entity(device, identifier, name),
          EntityWithCommand(device, identifier, name),
          EntityWithState<T>(device, identifier, name) {}

    virtual void bind(T * value) override {
        EntityWithState<T>::bind(value);
        setter = [value](const T & new_value) { *value = new_value; };
    }

    std::function<void(T)> setter;
};

template <typename T>
class Number : public InputEntity<T> {
public:
    Number(AbstractDevice & device, const String & identifier,
           const String & name)
        : Entity(device, identifier, name),
          InputEntity<T>(device, identifier, name),
          min(1),
          max(100),
          step(1) {}

    virtual JsonDocument get_autodiscovery_json() const override {
        JsonDocument json = Entity::get_autodiscovery_json();
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
    Text(AbstractDevice & device, const String & identifier,
         const String & name)
        : Entity(device, identifier, name),
          InputEntity(device, identifier, name),
          min(0),
          max(0),
          is_password(false) {}

    virtual JsonDocument get_autodiscovery_json() const override {
        JsonDocument json = Entity::get_autodiscovery_json();
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
    Switch(AbstractDevice & device, const String & identifier,
           const String & name)
        : Entity(device, identifier, name),
          InputEntity(device, identifier, name) {}

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
    Select(AbstractDevice & device, const String & identifier,
           const String & name)
        : Entity(device, identifier, name),
          InputEntity(device, identifier, name) {}

    virtual JsonDocument get_autodiscovery_json() const override {
        JsonDocument json = Entity::get_autodiscovery_json();
        {
            unsigned int idx = 0;
            for (const String & event_type : options) {
                json[F("options")][idx++] = event_type;
            }
        }
        return json;
    }

    std::set<String> options;

protected:
    virtual String get_platform() const override { return F("select"); }

    virtual void on_command(const String & command) override {
        if (setter && options.find(command) != options.end()) {
            setter(command);
        }
    };
};

};  // namespace PicoHA