#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

extern "C" {
extern const char _rodata_start;
extern const char _rodata_end;
}

namespace PicoHA {

template <typename T>
String to_string_default(const T v) {
    return String(v);
}

template <>
inline String to_string_default<bool>(const bool v) {
    return v ? F("ON") : F("OFF");
}

class SmartString {
public:
    SmartString(const char * s = nullptr) : ptr(nullptr) { assign(s); }
    SmartString(const __FlashStringHelper * s)
        : SmartString(reinterpret_cast<const char *>(s)) {}
    SmartString(const String & s) : SmartString(s.c_str()) {}

    SmartString(const SmartString & other) : ptr(nullptr) { assign(other.ptr); }
    SmartString(SmartString && other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }

    SmartString & operator=(const SmartString & other) {
        if (this == &other) {
            return *this;
        }

        assign(other.ptr);
        return *this;
    }

    SmartString & operator=(SmartString && other) noexcept {
        if (this == &other) {
            return *this;
        }

        clear();
        ptr = other.ptr;
        other.ptr = nullptr;
        return *this;
    }

    SmartString & operator=(const char * s) {
        assign(s);
        return *this;
    }

    SmartString & operator=(const __FlashStringHelper * s) {
        assign(reinterpret_cast<const char *>(s));
        return *this;
    }

    SmartString & operator=(const String & s) {
        assign(s.c_str());
        return *this;
    }

    operator String() const {
        if (!ptr) return String();

        if (is_literal(ptr)) {
            return String(reinterpret_cast<__FlashStringHelper *>(ptr));
        } else {
            return String(ptr);
        }
    }

    ~SmartString() { clear(); }

    bool isEmpty() const {
        return !ptr ||
               (is_literal(ptr) ? (strlen_P(ptr) == 0) : (strlen(ptr) == 0));
    }

private:
    void clear() {
        if (ptr && !is_literal(ptr)) {
            free(ptr);
        }
        ptr = nullptr;
    }

    void assign(const char * s) {
        clear();

        if (s) {
            if (is_literal(s)) {
                ptr = const_cast<char *>(s);
            } else {
                ptr = strdup(s);
            }
        }
    }

#if defined(ESP8266)
    static bool is_literal(const char * ptr) {
        return ptr >=
               (const char *)0x40200000;  // && ptr < (const char *)0x40300000;
    }
#elif defined(ESP32)
    static bool is_literal(const char * ptr) {
        return ptr >= &_rodata_start && ptr < &_rodata_end;
    }
#endif

    char * ptr;
};

#ifdef ESP8266
inline String operator+(const String & a, const SmartString & b) {
    return a + String(b);
}

inline String operator+(const SmartString & a, const String & b) {
    return String(a) + b;
}

inline String operator+(const SmartString & a, const SmartString & b) {
    return String(a) + String(b);
}

inline String operator+(const SmartString & a, const __FlashStringHelper * b) {
    return String(a) + String(b);
}
#endif

inline String operator+(const __FlashStringHelper * a, const SmartString & b) {
    return String(a) + String(b);
}

inline bool convertToJson(const SmartString & s, JsonVariant variant) {
    return variant.set(String(s));
}

}  // namespace PicoHA