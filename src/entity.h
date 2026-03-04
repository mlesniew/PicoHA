#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PicoMQTT.h>

#include "device.h"

namespace PicoHA {

class Device;

class Entity {
    public:
        Entity(Device & device, const String & identifier, const String & name);

        virtual void begin() {}
        virtual void tick() {}
        virtual void fire() {}

        virtual JsonDocument get_autodiscovery_json() const;
        virtual String get_autodiscovery_topic() const;

        virtual String get_unique_id() const;

        void autodiscovery();

        const String identifier;
        String name;
        String icon;
        bool is_diagnostic;
        bool enabled_by_default;

    protected:
        Device & device;
        PicoMQTT::Client & get_mqtt() { return device.get_mqtt(); }
        virtual String get_platform() const = 0;
};

}