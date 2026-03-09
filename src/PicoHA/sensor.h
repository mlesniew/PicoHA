#pragma once

#include <functional>

#include "entity.h"

namespace PicoHA {

template <typename T>
class Sensor : public EntityWithState<T> {
public:
    using EntityWithState<T>::EntityWithState;

protected:
    virtual String get_platform() const override { return "sensor"; }
};

}  // namespace PicoHA