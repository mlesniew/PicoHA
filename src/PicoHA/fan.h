#pragma once
#include <vector>

#include "input.h"

namespace PicoHA {

class Fan : public Switch {
public:
    using Switch::Switch;

protected:
    virtual String get_platform() const override { return F("fan"); }
};

template <typename T, String (*to_string)(T)>
class FanWithPresetModes : public Fan {
public:
    FanWithPresetModes(const PicoString & identifier, const PicoString & name,
                       const std::initializer_list<T> & preset_modes)
        : Fan(identifier, name), preset_modes(preset_modes) {}

    virtual void bind_preset(T * value) {
        preset_mode_getter = PicoCallback<T>([](T * v) { return *v; }, value);
        preset_mode_setter = PicoCallback<void, T>(
            [](T * v, T new_value) { *v = new_value; }, value);
    }

    const std::vector<T> preset_modes;
    PicoCallback<T> preset_mode_getter;
    PicoCallback<void, T> preset_mode_setter;

protected:
    T preset;

    virtual PicoJson print_autodiscovery_json(const AbstractDevice & device,
                                              Print & out) const override {
        PicoJson json = Fan::print_autodiscovery_json(device, out);

        {
            auto preset_modes_json = json[F("preset_modes")];
            for (T mode : preset_modes) {
                preset_modes_json.append() = to_string(mode);
            }
        }

        if (preset_mode_setter) {
            json[F("preset_mode_command_topic")] =
                get_preset_command_topic(device);
        }

        if (preset_mode_getter) {
            json[F("preset_mode_state_topic")] = get_preset_state_topic(device);
        }

        return json;
    }

    virtual void begin(AbstractDevice & device) override {
        Fan::begin(device);
        if (preset_mode_getter) {
            preset = preset_mode_getter();
        }
        if (preset_mode_setter) {
            get_mqtt(device).subscribe(
                get_preset_command_topic(device),
                [this](const char * payload) { on_preset_command(payload); });
        }
    }

    virtual void loop(AbstractDevice & device) override {
        Fan::loop(device);

        if (!preset_mode_getter) {
            return;
        }

        T new_preset = preset_mode_getter();
        if (new_preset != preset) {
            publish_preset(device, new_preset);
        }
    }

    virtual void fire(AbstractDevice & device) override {
        Fan::fire(device);

        if (preset_mode_getter) {
            publish_preset(device, preset_mode_getter());
        }
    }

    virtual void publish_preset(AbstractDevice & device, T new_preset) {
        preset = new_preset;
        get_mqtt(device).publish(get_preset_state_topic(device),
                                 to_string(preset));
    }

    String get_preset_state_topic(const AbstractDevice & device) const {
        return get_topic_prefix(device) + F("/preset");
    }

    String get_preset_command_topic(const AbstractDevice & device) const {
        return get_preset_state_topic(device) + F("/set");
    }

    void on_preset_command(const char * payload) {
        for (T p : preset_modes) {
            if (to_string(p) == payload) {
                preset_mode_setter(p);
                return;
            }
        }
    }
};

inline Fan & AbstractDevice::addFan(const PicoString & id,
                                    const PicoString & name) {
    return addEntity<Fan>(id, name);
}

template <typename T, String (*to_string)(T)>
FanWithPresetModes<T, to_string> & AbstractDevice::addFanWithPresetModes(
    const PicoString & id, const PicoString & name,
    const std::initializer_list<T> & preset_modes) {
    return addEntity<FanWithPresetModes<T, to_string>>(id, name, preset_modes);
}

}  // namespace PicoHA