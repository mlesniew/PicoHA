#pragma once

#include <PicoString.h>

#include <vector>

#include "entity.h"

namespace PicoHA {

class Event : public Entity {
public:
    Event(const PicoString & identifier, const PicoString & name,
          const std::initializer_list<PicoString> & event_types)
        : Entity(identifier, name),
          event_types(event_types),
          pending(nullptr) {}

    Event(const PicoString & identifier, const PicoString & name)
        : Event(identifier, name, {identifier}) {}

    virtual PicoJson print_autodiscovery_json(const AbstractDevice & device,
                                              Print & out) const override;

    void trigger(const PicoString & event_type) {
        if (std::find(event_types.begin(), event_types.end(), event_type) !=
            event_types.end()) {
            pending = event_type;
        }
    }

    void trigger() { trigger(event_types[0]); }

protected:
    virtual void tick(AbstractDevice & device) override { fire(device); }
    virtual void fire(AbstractDevice & device) override;

    virtual String get_platform() const override { return F("event"); }
    virtual String get_state_topic(
        const AbstractDevice & device) const override {
        return get_topic_prefix(device);
    }

    void write_event(Print & out) const;

    std::vector<PicoString> event_types;
    PicoString pending;
};

inline Event & AbstractDevice::addEvent(const PicoString & id,
                                        const PicoString & name) {
    return addEntity<Event>(id, name);
}
}  // namespace PicoHA