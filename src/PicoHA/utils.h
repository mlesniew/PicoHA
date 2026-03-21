#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PicoSlugify.h>
#include <PicoString.h>

namespace PicoHA {

template <typename T>
String to_string_default(const T v) {
    return String(v);
}

template <>
inline String to_string_default<bool>(const bool v) {
    return v ? F("ON") : F("OFF");
}

inline bool convertToJson(const PicoString & s, JsonVariant variant) {
    return variant.set(String(s));
}

inline PicoString smart_slugify(const PicoString & s,
                                const char separator = '_') {
    if (PicoSlugify::isSlug(s, separator)) {
        return s;
    } else {
        return PicoSlugify::slugify(s, separator);
    }
}

}  // namespace PicoHA
