#pragma once

#include "entity.h"

namespace PicoHA {

class Event : public Entity {
public:
    Event(const PicoString & identifier, const PicoString & name)
        : Entity(identifier, name), pending(false) {}

    virtual PicoJson print_autodiscovery_json(const AbstractDevice & device,
                                              Print & out) const override;

    virtual void trigger() { pending = true; }

protected:
    virtual void tick(AbstractDevice & device) override { fire(device); }
    virtual void fire(AbstractDevice & device) override;

    virtual String get_platform() const override { return F("event"); }
    virtual String get_state_topic(
        const AbstractDevice & device) const override {
        return get_topic_prefix(device);
    }

    void write_event(Print & out) const;

    bool pending;
};

inline Event & AbstractDevice::addEvent(const PicoString & id,
                                        const PicoString & name) {
    return addEntity<Event>(id, name);
}
}  // namespace PicoHA