#include "event.h"

namespace PicoHA {

PicoJson Event::print_autodiscovery_json(const AbstractDevice & device,
                                         Print & out) const {
    PicoJson json = Entity::print_autodiscovery_json(device, out);
    json[F("event_types")].append() = identifier;
    return json;
}

void Event::fire(AbstractDevice & device) {
    auto & mqtt = device.get_mqtt();
    if (mqtt.connected() && pending) {
        ByteCounter counter;
        write_event(counter);
        auto publish = mqtt.begin_publish(get_state_topic(device),
                                          counter.getCount(), 0, true);
        write_event(publish);
        publish.send();

        pending = false;
    }
}

void Event::write_event(Print & out) const {
    PicoJson json(out);
    json[F("event_type")] = identifier;
}

}  // namespace PicoHA