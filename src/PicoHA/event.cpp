#include "event.h"

namespace PicoHA {

JsonDocument Event::get_autodiscovery_json(
    const AbstractDevice & device) const {
    JsonDocument json = Entity::get_autodiscovery_json(device);
    json[F("event_types")][0] = identifier;
    return json;
}

void Event::fire(AbstractDevice & device) {
    if (device.get_mqtt().connected() && pending) {
        trigger(device);
        pending = false;
    }
}

}  // namespace PicoHA