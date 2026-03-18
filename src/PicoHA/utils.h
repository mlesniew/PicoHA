#pragma once

#include <Arduino.h>

namespace PicoHA {

template <typename T>
String to_string_default(const T v) {
    return String(v);
}

template <>
inline String to_string_default<bool>(const bool v) {
    return v ? F("ON") : F("OFF");
}

}  // namespace PicoHA