#include "event.h"

namespace PicoHA {

JsonDocument Event::get_autodiscovery_json(
    const AbstractDevice & device) const {
    JsonDocument json = Entity::get_autodiscovery_json(device);
    json[F("event_types")][0] = identifier;
    return json;
}

void Event::fire(AbstractDevice & device) {
    auto & mqtt = device.get_mqtt();
    if (mqtt.connected() && pending) {
        JsonDocument json;
        json[F("event_type")] = identifier;
        auto publish = mqtt.begin_publish(get_state_topic(device),
                                          measureJson(json), 0, true);
        serializeJson(json, publish);
        publish.send();

        pending = false;
    }
}

}  // namespace PicoHA