#include "event.h"

namespace PicoHA {

JsonDocument Event::get_autodiscovery_json() const {
    JsonDocument json = Entity::get_autodiscovery_json();
    json[F("event_types")][0] = identifier;
    return json;
}

void Event::trigger() { get_mqtt().publish(get_state_topic(), identifier); }

void QueuedEvent::tick() { fire(); }

void QueuedEvent::fire() {
    if (get_mqtt().connected() && pending) {
        Event::trigger();
        pending = false;
    }
}

void QueuedEvent::trigger() {
    pending = true;
    fire();
}

}  // namespace PicoHA