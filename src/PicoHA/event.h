#pragma once

#include "entity.h"

namespace PicoHA {

class Event : public Entity {
public:
    Event(AbstractDevice & device, const String & identifier,
          const String & name)
        : Entity(device, identifier, name) {}

    virtual JsonDocument get_autodiscovery_json() const override;
    virtual void trigger();

protected:
    virtual String get_platform() const override { return F("event"); }
    virtual String get_state_topic() const override {
        return get_topic_prefix();
    }
};

class QueuedEvent : public Event {
public:
    QueuedEvent(AbstractDevice & device, const String & identifier,
                const String & name)
        : Event(device, identifier, name), pending(false) {}

    virtual void tick() override;
    virtual void fire() override;
    virtual void trigger() override;

protected:
    bool pending;
};
};  // namespace PicoHA