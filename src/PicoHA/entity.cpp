#include "entity.h"

namespace PicoHA {

Entity::Entity(const PicoString & identifier, const PicoString & name)
    : identifier(smart_slugify(identifier)),
      name(name),
      is_diagnostic(false),
      enabled_by_default(true),
      next(nullptr) {}

PicoJson Entity::print_autodiscovery_json(const AbstractDevice & device,
                                          Print & out) const {
    PicoJson json(out);

    json[F("unique_id")] = get_unique_id(device);
    json[F("platform")] = get_platform();
    if (name.isEmpty()) {
        json[F("name")] = nullptr;
    } else {
        json[F("name")] = name;
    }
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
    device.print_autodiscovery_json(json[F("device")]);
    json[F("availability_topic")] = device.get_availability_topic();
    json[F("default_entity_id")] =
        get_platform() + F(".") + device.get_default_entity_id_prefix() +
        (name.isEmpty() ? F("") : F("_") + identifier);

    const String state_topic = get_state_topic(device);
    if (!state_topic.isEmpty()) {
        json[F("state_topic")] = state_topic;
    }

    const String command_topic = get_command_topic(device);
    if (!command_topic.isEmpty()) {
        json[F("command_topic")] = command_topic;
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
    ByteCounter counter;
    print_autodiscovery_json(device, counter);

    auto publish = device.get_mqtt().begin_publish(
        get_autodiscovery_topic(device), counter.getCount(), 0, true);
    print_autodiscovery_json(device, publish);
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
