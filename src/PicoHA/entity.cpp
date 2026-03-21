#include "entity.h"

namespace PicoHA {

Entity::Entity(const PicoString & identifier, const PicoString & name)
    : identifier(smart_slugify(identifier)),
      name(name),
      is_diagnostic(false),
    enabled_by_default(true),
    next(nullptr) {}

JsonDocument Entity::get_autodiscovery_json(
    const AbstractDevice & device) const {
    JsonDocument json;
    json[F("unique_id")] = get_unique_id(device);
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

    if (!get_state_topic(device).isEmpty()) {
        json[F("state_topic")] = get_state_topic(device);
    }

    if (!get_command_topic(device).isEmpty()) {
        json[F("command_topic")] = get_command_topic(device);
    }

    return json;
}

String Entity::get_autodiscovery_topic(const AbstractDevice & device) const {
    return String(F("homeassistant/")) + get_platform() + F("/") +
           get_unique_id(device) + F("/config");
}

String Entity::get_unique_id(const AbstractDevice & device) const {
    return device.get_unique_id() + F("-") + identifier;
}

void Entity::autodiscovery(AbstractDevice & device) {
    JsonDocument json = get_autodiscovery_json(device);
    auto publish = device.get_mqtt().begin_publish(
        get_autodiscovery_topic(device), measureJson(json), 0, true);
    serializeJson(json, publish);
    publish.send();
}

EntityWithCommand::~EntityWithCommand() {}

void EntityWithCommand::begin(AbstractDevice & device) {
    device.get_mqtt().subscribe(
        get_command_topic(device),
        [this](const String & payload) { on_command(payload); });
}

void EntityWithCommand::end(AbstractDevice & device) {
    device.get_mqtt().unsubscribe(get_command_topic(device));
}

};  // namespace PicoHA