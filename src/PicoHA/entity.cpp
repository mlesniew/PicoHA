#include "entity.h"

#include <PicoSlugify.h>

namespace PicoHA {

Entity::Entity(AbstractDevice & device, const SmartString & identifier,
               const SmartString & name)
    : device(device),
      identifier(PicoSlugify::slugify(identifier)),
      name(name),
      is_diagnostic(false),
      enabled_by_default(true) {
    device.entities.insert(this);
}

Entity::~Entity() { device.entities.erase(this); }

JsonDocument Entity::get_autodiscovery_json() const {
    JsonDocument json;
    json[F("unique_id")] = get_unique_id();
    json[F("platform")] = get_platform();
    json[F("name")] = !name.isEmpty() ? String(name).c_str() : nullptr;
    if (!icon.isEmpty()) {
        json[F("icon")] = F("mdi:") + icon;
    }
    if (!device_class.isEmpty()) {
        json[F("device_class")] = device_class;
    }
    if (is_diagnostic) {
        json[F("entity_category")] = F("diagnostic");
    }
    if (!enabled_by_default) {
        json[F("enabled_by_default")] = false;
    }
    json[F("device")] = device.get_autodiscovery_json();
    json[F("availability_topic")] = device.get_availability_topic();
    json[F("default_entity_id")] =
        get_platform() + F(".") + device.get_default_entity_id_prefix() +
        (name.isEmpty() ? F("") : F("_") + identifier);

    if (!get_state_topic().isEmpty()) {
        json[F("state_topic")] = get_state_topic();
    }

    if (!get_command_topic().isEmpty()) {
        json[F("command_topic")] = get_command_topic();
    }

    return json;
}

String Entity::get_autodiscovery_topic() const {
    return F("homeassistant/") + get_platform() + F("/") + get_unique_id() +
           F("/config");
}

String Entity::get_unique_id() const {
    return device.get_unique_id() + F("-") + identifier;
}

void Entity::autodiscovery() {
    JsonDocument json = get_autodiscovery_json();
    auto publish = get_mqtt().begin_publish(get_autodiscovery_topic(),
                                            measureJson(json), 0, true);
    serializeJson(json, publish);
    publish.send();
}

void EntityWithCommand::begin() {
    get_mqtt().subscribe(get_command_topic(), [this](const String & payload) {
        on_command(payload);
    });
}

};  // namespace PicoHA