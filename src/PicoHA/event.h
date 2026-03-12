#pragma once

#include "entity.h"

namespace PicoHA {

class Event : public Entity {
public:
    Event(AbstractDevice & device, const String & identifier,
          const String & name)
        : Entity(device, identifier, name) {}

    virtual JsonDocument get_autodiscovery_json() const override {
        JsonDocument json = Entity::get_autodiscovery_json();
        {
            json["event_types"][0] = identifier;
        }
        return json;
    }

    virtual void trigger() {
        get_mqtt().publish(get_state_topic(), identifier);
    }

protected:
    virtual String get_platform() const override { return "event"; }
    virtual String get_state_topic() const override {
        return get_topic_prefix();
    }
};

class QueuedEvent : public Event {
public:
    QueuedEvent(AbstractDevice & device, const String & identifier,
                const String & name)
        : Event(device, identifier, name), pending(false) {}

    virtual void tick() { fire(); }

    virtual void fire() override {
        if (get_mqtt().connected() && pending) {
            Event::trigger();
            pending = false;
        }
    }

    virtual void trigger() override {
        pending = true;
        fire();
    }

protected:
    bool pending;
};
};  // namespace PicoHA