#pragma once

#include <PicoString.h>

#include <vector>

#include "entity.h"

namespace PicoHA {

template <typename T = PicoString,
          String (*to_string)(T) = to_string_default<T>>
class Event : public Entity {
public:
    Event(const PicoString & identifier, const PicoString & name,
          const std::initializer_list<T> & event_types)
        : Entity(identifier, name),
          event_types(event_types),
          is_pending(false) {}

    virtual PicoJson print_autodiscovery_json(const AbstractDevice & device,
                                              Print & out) const override {
        PicoJson json = Entity::print_autodiscovery_json(device, out);
        auto event_types_json = json[F("event_types")];
        for (const auto & event_type : event_types) {
            event_types_json.append() = to_string(event_type);
        }
        return json;
    }

    void trigger(const T event_type) {
        is_pending = false;
        if (std::find(event_types.begin(), event_types.end(), event_type) !=
            event_types.end()) {
            pending_event = event_type;
            is_pending = true;
        }
    }

    virtual void trigger() { trigger(event_types[0]); }

protected:
    virtual void tick(AbstractDevice & device) override { fire(device); }

    virtual void fire(AbstractDevice & device) override {
        auto & mqtt = device.get_mqtt();
        if (mqtt.connected() && is_pending) {
            ByteCounter counter;
            write_event(counter);
            auto publish = mqtt.begin_publish(get_state_topic(device),
                                              counter.getCount(), 0, true);
            write_event(publish);
            publish.send();

            is_pending = false;
        }
    }

    void write_event(Print & out) const {
        PicoJson json(out);
        json[F("event_type")] = to_string(pending_event);
    }

    virtual String get_platform() const override { return F("event"); }
    virtual String get_state_topic(
        const AbstractDevice & device) const override {
        return get_topic_prefix(device);
    }

    const std::vector<T> event_types;
    bool is_pending;
    T pending_event;
};

class SimpleEvent : public Event<PicoString, to_string_default<PicoString>> {
public:
    SimpleEvent(const PicoString & identifier, const PicoString & name)
        : Event(identifier, name, {identifier}) {}

    virtual void trigger() override { Event::trigger(identifier); }
};

template <typename T = PicoString,
          String (*to_string)(T) = to_string_default<T>>
inline Event<T, to_string> & AbstractDevice::addEvent(
    const PicoString & id, const PicoString & name,
    const std::initializer_list<T> & event_types) {
    return addEntity<Event<T, to_string>>(id, name, event_types);
}

inline SimpleEvent & AbstractDevice::addEvent(const PicoString & id,
                                              const PicoString & name) {
    return addEntity<SimpleEvent>(id, name);
}

}  // namespace PicoHA