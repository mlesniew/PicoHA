#pragma once

#include <list>

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PicoMQTT.h>
#include <PicoSlugify.h>

namespace PicoHA {

class Entity;

class Device {
    public:
        Device(
            const String & name, const String & manufacturer,
            const String & model, const String & suggested_area);

        virtual ~Device() {}

        Device(const Device &) = delete;
        Device & operator=(const Device &) = delete;

        Device(Device &&) = delete;
        Device & operator=(Device &&) = delete;

        JsonDocument get_autodiscovery_json() const;

        virtual String get_unique_id() const = 0;
        virtual String get_topic_prefix() const = 0;

        virtual String get_availability_topic() const = 0;
        virtual const Device * get_parent_device() const = 0;

        virtual String get_default_entity_id_prefix() const;

        virtual void begin();
        virtual void tick();
        virtual void fire();
        void autodiscovery();

        const String identifier;
        const String name;
        const String manufacturer;
        const String model;
        const String suggested_area;

        unsigned long update_interval;

        friend class Entity;
        friend class ChildDevice;

        virtual PicoMQTT::Client & get_mqtt() = 0;

    protected:
        std::set<Device *> devices;
        std::set<Entity *> entities;

    private:
        unsigned long last_update;
};

class RootDevice : public Device {
    public:
        RootDevice(
            PicoMQTT::Client & mqtt,
            const String & name, const String & manufacturer,
            const String & model, const String & suggested_area) : Device(name, manufacturer, model, suggested_area), mqtt(mqtt) {
        }

        virtual String get_unique_id() const {
            return PicoSlugify::slugify(model) + "-" + String((uint32_t)(ESP.getEfuseMac() >> 24), HEX);
        }

        virtual String get_topic_prefix() const override {
            return model + "/" + get_unique_id();
        }

        virtual const Device * get_parent_device() const override {
            return nullptr;
        }

        virtual String get_availability_topic() const override {
            return get_topic_prefix() + "/availability";
        }

        virtual void begin() override;

        virtual PicoMQTT::Client & get_mqtt() override { return mqtt; }

    protected:
        PicoMQTT::Client & mqtt;
};

class ChildDevice : public Device {
    public:
        ChildDevice(
            Device & parent,
            const String & identifier, const String & name, const String & manufacturer,
            const String & model, const String & suggested_area)
            : Device(name, manufacturer, model, suggested_area),
              parent(parent), identifier(PicoSlugify::slugify(identifier)) {
            parent.devices.insert(this);
        }

        virtual ~ChildDevice() {
            parent.devices.erase(this);
        }

        virtual String get_unique_id() const {
            return parent.get_unique_id() + "-" + identifier;
        }

        virtual String get_topic_prefix() const {
            return parent.get_topic_prefix() + identifier + "/";
        }

        virtual const Device * get_parent_device() const override {
            return &parent;
        }

        virtual String get_availability_topic() const {
            return parent.get_availability_topic();
        }

        virtual PicoMQTT::Client & get_mqtt() override { return parent.get_mqtt(); }

        Device & parent;
        const String identifier;
};

}