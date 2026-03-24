#pragma once

#include <Arduino.h>

#include <type_traits>

class PicoJson {
public:
    PicoJson(Print &out) : PicoJson(out, nullptr, State::start) {}

    PicoJson(const PicoJson &other) = delete;

    PicoJson(PicoJson &&other)
        : out(other.out),
          parent(other.parent),
          child(other.child),
          state(other.state) {
        if (parent) {
            if (parent->child == &other) {
                parent->child = this;
            }
        }

        if (child) {
            child->parent = this;
        }

        other.state = State::closed;
    }

    PicoJson &operator=(const PicoJson &other) = delete;

    bool operator=(bool value) {
        if (state == State::start) {
            out.print(value ? "true" : "false");
        }
        state = State::closed;
        return value;
    }

    template <typename T,
              typename std::enable_if<std::is_integral<T>::value &&
                                          !std::is_same<T, bool>::value,
                                      int>::type = 0>
    T operator=(T value) {
        if (state == State::start) {
            out.print(value);
        }
        state = State::closed;
        return value;
    }

    double operator=(double value) {
        if (state == State::start) {
            out.print(value);
        }
        state = State::closed;
        return value;
    }

    std::nullptr_t operator=(std::nullptr_t) {
        if (state == State::start) {
            out.print("null");
        }
        state = State::closed;
        return nullptr;
    }

    const char *operator=(const char *value) {
        if (state != State::start) {
            return value;
        }

        if (value) {
            writeString(value);
        } else {
            out.write("null");
        }
        state = State::closed;
        return value;
    }

    const String &operator=(const String &value) {
        *this = value.c_str();
        return value;
    }

    const __FlashStringHelper *operator=(const __FlashStringHelper *value) {
        if (state != State::start) {
            return value;
        }

        if (value) {
            writeStringPGM(reinterpret_cast<const char *>(value));
        } else {
            out.write("null");
        }
        state = State::closed;
        return value;
    }

    class Value {
    public:
        Value(std::nullptr_t = nullptr) : type(Type::null_t), i(0) {}
        Value(bool v) : type(Type::boolean), b(v) {}
        Value(int v) : type(Type::integer), i(v) {}
        Value(long v) : type(Type::integer), i(v) {}
        Value(float v) : type(Type::real), d(v) {}
        Value(double v) : type(Type::real), d(v) {}
        Value(const char *v) : type(v ? Type::cstr : Type::null_t), s(v) {}
        Value(const __FlashStringHelper *v)
            : type(v ? Type::fstr : Type::null_t), f(v) {}

        void apply(PicoJson &j) const {
            switch (type) {
                case Type::null_t:
                    j = nullptr;
                    break;
                case Type::boolean:
                    j = b;
                    break;
                case Type::integer:
                    j = i;
                    break;
                case Type::real:
                    j = d;
                    break;
                case Type::cstr:
                    j = s;
                    break;
                case Type::fstr:
                    j = f;
                    break;
            }
        }

    private:
        enum class Type { null_t, boolean, integer, real, cstr, fstr } type;
        union {
            bool b;
            long i;
            double d;
            const char *s;
            const __FlashStringHelper *f;
        };
    };

    void operator=(std::initializer_list<Value> values) {
        for (const Value &v : values) {
            PicoJson item = append();
            v.apply(item);
        }
    }

    PicoJson append() {
        char c = ',';

        if (state == State::start) {
            state = State::list;
            c = '[';
        }

        if (state == State::list) {
            closeChild();

            out.write(c);
            PicoJson ret(out, this);
            child = &ret;
            return ret;
        }

        return PicoJson(out, nullptr, State::closed);
    }

    template <typename T,
              typename std::enable_if<std::is_integral<T>::value &&
                                          !std::is_same<T, bool>::value,
                                      int>::type = 0>
    PicoJson operator[](T idx) {
        (void)idx;
        return append();
    }

    PicoJson add(const char *key) {
        char c = ',';

        if (state == State::start) {
            state = State::object;
            c = '{';
        }

        if (state == State::object) {
            closeChild();
            out.write(c);
            writeString(key);
            out.write(':');
            PicoJson ret(out, this);
            child = &ret;
            return ret;
        }

        return PicoJson(out, nullptr, State::closed);
    }

    PicoJson add(const __FlashStringHelper *key) {
        char c = ',';

        if (state == State::start) {
            state = State::object;
            c = '{';
        }

        if (state == State::object) {
            closeChild();
            out.write(c);
            writeStringPGM(reinterpret_cast<const char *>(key));
            out.write(':');
            PicoJson ret(out, this);
            child = &ret;
            return ret;
        }

        return PicoJson(out, nullptr, State::closed);
    }

    PicoJson operator[](const char *key) { return add(key); }
    PicoJson operator[](const __FlashStringHelper *key) { return add(key); }

    virtual ~PicoJson() {
        close();

        if (!parent) return;

        if (parent->child == this) {
            parent->child = nullptr;
            return;
        }
    }

    void close() {
        closeChild();
        switch (state) {
            case State::object:
                out.write('}');
                break;
            case State::list:
                out.write(']');
                break;
            case State::start:
                out.write("null");
            default:
                break;
        }

        state = State::closed;
    }

protected:
    Print &out;

    PicoJson *parent;
    PicoJson *child;

    enum class State {
        start,
        object,
        list,
        closed,
    } state;

    PicoJson(Print &out, PicoJson *parent, State state = State::start)
        : out(out), parent(parent), child(nullptr), state(state) {}

    void writeChar(char c) {
        switch (c) {
            case '"':
                out.write("\\\"");
                break;
            case '\\':
                out.write("\\\\");
                break;
            case '\b':
                out.write("\\b");
                break;
            case '\f':
                out.write("\\f");
                break;
            case '\n':
                out.write("\\n");
                break;
            case '\r':
                out.write("\\r");
                break;
            case '\t':
                out.write("\\t");
                break;
            default:
                if (c < 0x20) {
                    out.printf("\\u%04x", c);
                } else {
                    out.write(c);
                }
        }
    }

    void writeString(const char *value) {
        out.write('"');
        char c;
        while (value && (c = *value++)) {
            writeChar(c);
        }
        out.write('"');
    }

    void writeStringPGM(const char *value) {
        out.write('"');
        char c;
        while (value && (c = pgm_read_byte(value++))) {
            writeChar(c);
        }
        out.write('"');
    }

    void closeChild() {
        if (child) {
            child->close();
            child->parent = nullptr;
            child = nullptr;
        }
    }
};
