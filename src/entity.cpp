#include <PicoSlugify.h>

#include "entity.h"

namespace PicoHA {


Entity::Entity(Device & device, const String & identifier, const String & name)
    : device(device), identifier(PicoSlugify::slugify(identifier)), name(name), is_diagnostic(false),
      enabled_by_default(true) {
    device.entities.push_back(this);
}

JsonDocument Entity::get_autodiscovery_json() const {
    JsonDocument json;
    json["unique_id"] = get_unique_id();
    json["platform"] = get_platform();
    json["name"] = name.length() ? name.c_str() : nullptr;
    if (icon.length()) {
        json["icon"] = "mdi:" + icon;
    }
    if (is_diagnostic) {
        json["entity_category"] = "diagnostic";
    }
    if (!enabled_by_default) {
        json[enabled_by_default] = false;
    }
    json["device"] = device.get_autodiscovery_json();
    json["availability_topic"] = device.get_availability_topic();
    json["default_entity_id"] = get_platform() + "." + device.get_default_entity_id_prefix() + identifier;
    return json;
}

String Entity::get_autodiscovery_topic() const {
    return "homeassistant/" + get_platform() + "/" + get_unique_id() + "/config";
}

String Entity::get_unique_id() const {
    return device.get_unique_id() + "-" + identifier;
}

void Entity::autodiscovery() {
    JsonDocument json = get_autodiscovery_json();
    auto publish = get_mqtt().begin_publish(get_autodiscovery_topic(), measureJson(json), 0, true);
    serializeJson(json, publish);
    publish.send();
}

};