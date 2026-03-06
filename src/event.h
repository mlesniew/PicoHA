#pragma once

#include <entity.h>

#include <set>

namespace PicoHA {

class Event : public Entity {
public:
    Event(Device & device, const String & identifier, const String & name)
        : Entity(device, identifier, name), event_types({this->identifier}) {}

    virtual JsonDocument get_autodiscovery_json() const override {
        JsonDocument json = Entity::get_autodiscovery_json();
        {
            unsigned int idx = 0;
            for (const String & event_type : event_types) {
                json["event_types"][idx++] = event_type;
            }
        }
        return json;
    }

    virtual void trigger(const String & event_type) {
        get_mqtt().publish(get_state_topic(), event_type);
    }

    void trigger() { trigger(this->identifier); }

    std::set<String> event_types;

protected:
    virtual String get_platform() const override { return "event"; }
    virtual String get_state_topic() const override {
        return get_topic_prefix();
    }
};

class QueuedEvent : public Event {
public:
    using Event::Event;

    virtual void tick() { fire(); }

    virtual void fire() override {
        while (get_mqtt().connected() && !pending_event_types.empty()) {
            auto it = event_types.begin();
            if (get_mqtt().publish(get_state_topic(), *it)) {
                pending_event_types.erase(it);
            }
        }
    }

    virtual void trigger(const String & event_type = "") override {
        pending_event_types.insert(event_type.isEmpty() ? this->identifier
                                                        : event_type);
        fire();
    }

protected:
    std::set<String> pending_event_types;
};
};  // namespace PicoHA