#pragma once

#include <Arduino.h>

class PicoJson {
public:
    PicoJson(Print & out) : PicoJson(out, nullptr, State::start) {}

    PicoJson(const PicoJson & other) = delete;

    PicoJson(PicoJson && other)
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

    PicoJson & operator=(const PicoJson & other) = delete;

    unsigned int operator=(long value) {
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

    const char * operator=(const char * value) {
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

    const String & operator=(const String & value) {
        *this = value.c_str();
        return value;
    }

    const __FlashStringHelper * operator=(const __FlashStringHelper * value) {
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

    PicoJson append() {
        char c = ',';

        if (state == State::start) {
            state = State::list;
            c = '[';
        }

        if (state == State::list) {
            if (child) {
                child->close();
                child->parent = nullptr;
            }

            out.write(c);
            PicoJson ret(out, this);
            child = &ret;
            return ret;
        }

        return PicoJson(out, nullptr, State::closed);
    }

    PicoJson operator[](size_t idx) { return append(); }

    PicoJson add(const char * key) {
        char c = ',';

        if (state == State::start) {
            state = State::object;
            c = '{';
        }

        if (state == State::object) {
            if (child) {
                child->close();
                child->parent = nullptr;
            }

            out.write(c);
            writeString(key);
            out.write(':');
            PicoJson ret(out, this);
            child = &ret;
            return ret;
        }

        return PicoJson(out, nullptr, State::closed);
    }

    PicoJson add(const __FlashStringHelper * key) {
        char c = ',';

        if (state == State::start) {
            state = State::object;
            c = '{';
        }

        if (state == State::object) {
            out.write(c);
            writeStringPGM(reinterpret_cast<const char *>(key));
            out.write(':');
            PicoJson ret(out, this);
            child = &ret;
            return ret;
        }

        return PicoJson(out, nullptr, State::closed);
    }

    PicoJson operator[](const char * key) { return add(key); }
    PicoJson operator[](const __FlashStringHelper * key) { return add(key); }

    void setChild(PicoJson * ptr) {
        if (child) {
            child->close();
            child->parent = nullptr;
        }
        child = ptr;
    }

    virtual ~PicoJson() {
        if (child) {
            child->close();
            child->parent = nullptr;
            child = nullptr;
        }

        close();

        if (!parent) return;

        if (parent->child == this) {
            parent->child = nullptr;
            return;
        }
    }

    void close() {
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
    Print & out;

    PicoJson * parent;
    PicoJson * child;

    enum class State {
        start,
        object,
        list,
        closed,
    } state;

    PicoJson(Print & out, PicoJson * parent, State state = State::start)
        : out(out), parent(parent), child(nullptr), state(state) {}

    void writeString(const char * value) {
        out.write('"');
        while (value) {
            char c = *value++;
            if (!c) {
                break;
            }
            if (c == '"') {
                out.write('\\');
            }
            out.write(c);
        }
        out.write('"');
    }

    void writeStringPGM(const char * value) {
        out.write('"');
        while (value) {
            char c = pgm_read_byte(value++);
            if (!c) {
                break;
            }
            if (c == '"') {
                out.write('\\');
            }
            out.write(c);
        }
        out.write('"');
    }
};
