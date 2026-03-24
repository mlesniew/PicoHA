#pragma once

#include <Arduino.h>
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

inline PicoString smart_slugify(const PicoString & s,
                                const char separator = '_') {
    if (PicoSlugify::isSlug(s, separator)) {
        return s;
    } else {
        return PicoSlugify::slugify(s, separator);
    }
}

class ByteCounter : public Print {
public:
    ByteCounter() : count(0) {}

    virtual size_t write(uint8_t) override {
        ++count;
        return 1;
    }

    size_t write(const uint8_t * buffer, size_t size) override {
        count += size;
        return size;
    }

    size_t getCount() const { return count; }

private:
    size_t count = 0;
};

}  // namespace PicoHA
