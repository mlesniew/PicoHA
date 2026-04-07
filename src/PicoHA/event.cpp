#include "event.h"

namespace PicoHA {

PicoJson Event::print_autodiscovery_json(const AbstractDevice & device,
                                         Print & out) const {
    PicoJson json = Entity::print_autodiscovery_json(device, out);
    auto event_types_json = json[F("event_types")];
    for (const auto & event_type : event_types) {
        event_types_json.append() = event_type;
    }
    return json;
}

void Event::fire(AbstractDevice & device) {
    auto & mqtt = device.get_mqtt();
    if (mqtt.connected() && !pending.isEmpty()) {
        ByteCounter counter;
        write_event(counter);
        auto publish = mqtt.begin_publish(get_state_topic(device),
                                          counter.getCount(), 0, true);
        write_event(publish);
        publish.send();

        pending = (const char *)nullptr;
    }
}

void Event::trigger(const PicoString & event_type) {
    pending = (const char *)nullptr;
    if (std::find(event_types.begin(), event_types.end(), event_type) !=
        event_types.end()) {
        pending = event_type;
    }
}

void Event::write_event(Print & out) const {
    PicoJson json(out);
    json[F("event_type")] = pending;
}

}  // namespace PicoHA