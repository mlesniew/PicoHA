#pragma once

#include <cmath>

#include "entity.h"

namespace PicoHA {

class Cover : public Entity {
public:
    enum class State { open, opening, closed, closing, stopped };
    enum class Command { open, close, stop };

    Cover(const PicoString & identifier, const PicoString & name);

    virtual PicoJson print_autodiscovery_json(const AbstractDevice & device,
                                              Print & out) const override;

    PicoCallback<int> position_getter;
    PicoCallback<void, int> position_setter;

    PicoCallback<State> state_getter;
    PicoCallback<void, Command> command_setter;

protected:
    void begin(AbstractDevice & device) override;
    void loop(AbstractDevice & device) override;
    void fire(AbstractDevice & device) override;
    void end(AbstractDevice & device) override;

    virtual String get_state_topic(
        const AbstractDevice & device) const override {
        if (!state_getter) {
            return String();
        }
        return get_topic_prefix(device) + F("/state");
    }

    virtual String get_command_topic(
        const AbstractDevice & device) const override {
        if (!command_setter) {
            return String();
        }
        return get_topic_prefix(device) + F("/command");
    }

    virtual String get_platform() const override { return F("cover"); }

private:
    void publish_position(AbstractDevice & device, int new_position);
    void publish_state(AbstractDevice & device, State new_state);

    int current_position;
    State state;
};

inline Cover & AbstractDevice::addCover(const PicoString & id,
                                        const PicoString & name) {
    return addEntity<Cover>(id, name);
}

}  // namespace PicoHA