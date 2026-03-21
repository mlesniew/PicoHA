#pragma once

#include "entity.h"

namespace PicoHA {

class Event : public Entity {
public:
    Event(AbstractDevice & device, const PicoString & identifier,
          const PicoString & name)
        : Entity(device, identifier, name), pending(false) {}

    virtual JsonDocument get_autodiscovery_json(
        const AbstractDevice & device) const override;

    virtual void tick(AbstractDevice & device) override { fire(device); }
    virtual void fire(AbstractDevice & device) override;

    virtual void trigger() { pending = true; }

protected:
    virtual String get_platform() const override { return F("event"); }
    virtual String get_state_topic(
        const AbstractDevice & device) const override {
        return get_topic_prefix(device);
    }

    bool pending;
};

};  // namespace PicoHA