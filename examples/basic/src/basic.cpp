#include <PicoMQTT.h>

#include "PicoHA.h"

PicoMQTT::Client mqtt;

PicoHA::Device device{mqtt, "PicoHA", "mlesniew", "picoha", ""};

bool ping_pong_state = false;
bool binarino;
String text = "PicoHA";
String color = "Red";
int power = 10;

PicoHA::BinarySensor * binary_sensor;
PicoHA::NumericSensor<size_t> * text_lenght_sensor;

PicoHA::Event<PicoString> * pingpong_event;
PicoHA::SimpleEvent * reboot_event;

PicoHA::Switch * onoff_switch;
PicoHA::Text * text_input;
PicoHA::Button * reset_button;
PicoHA::Select * input_select;
PicoHA::Number<int> * power_input;
PicoHA::Cover * cover;
PicoHA::Climate * climate;

int cover_position = 100;
PicoHA::Cover::State cover_state = PicoHA::Cover::State::open;

struct {
    double current_temperature;
    double target_temperature;
    PicoHA::Climate::Mode mode;
    PicoHA::Climate::Action action;
} climate_state = {22.5, 24.0, PicoHA::Climate::Mode::heat,
                   PicoHA::Climate::Action::heating};

void setup() {
    Serial.begin(115200);

    WiFi.hostname("picoha");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    mqtt.host = "192.168.1.100";
    mqtt.begin();

    PicoHA::add_diagnostic_entities(device);

    binary_sensor = &device.addBinarySensor("binarino", "Binarino");
    binary_sensor->bind(&binarino);
    binary_sensor->device_class = "power";

    text_lenght_sensor =
        &device.addNumericSensor<size_t>("text_length", "Text Length");
    text_lenght_sensor->getter = [] { return text.length(); };
    text_lenght_sensor->icon = "dog";

    pingpong_event =
        &device.addEvent<PicoString>("pingpong", "Ping Pong", {"ping", "pong"});
    reboot_event = &device.addEvent("reboot", "Reboot");
    reboot_event->trigger();

    onoff_switch = &device.addSwitch("onoff", "Toggle");
    onoff_switch->bind(&binarino);

    text_input = &device.addText("text", "Text");
    text_input->bind(&text);

    reset_button = &device.addButton("reset", "Reset");
    reset_button->on_press = [] {
        text = "PicoHA";
        color = "Red";
        binarino = false;
        power = 10;
    };

    input_select = &device.addSelect("color", "Color");
    input_select->options = {"Red", "Yellow", "Green"};
    input_select->bind(&color);

    power_input = &device.addNumber<int>("power", "Power");
    power_input->bind(&power);

    auto & climate_device = device.addChildDevice(
        "climate", "Climate", "mlesniew", "picoha-climate", "");
    climate = &climate_device.addClimate("climate", "");
    climate->min_temp = 15;
    climate->max_temp = 30;
    climate->temp_step = 0.5;
    climate->modes = PicoHA::Climate::Mode::off | PicoHA::Climate::Mode::heat |
                     PicoHA::Climate::Mode::cool;
    climate->bind_mode(&climate_state.mode);
    climate->bind_action(&climate_state.action);
    climate->bind_current_temperature(&climate_state.current_temperature);
    climate->bind_target_temperature(&climate_state.target_temperature);
    climate->power_getter = [] {
        return climate_state.mode != PicoHA::Climate::Mode::off;
    };
    climate->power_setter = [](bool new_value) {
        if (!new_value) {
            climate_state.mode = PicoHA::Climate::Mode::off;
        } else if (climate_state.mode == PicoHA::Climate::Mode::off) {
            climate_state.mode = PicoHA::Climate::Mode::heat;
        };
    };

    cover = &device.addCover("Cover", "cover");
    cover->position_getter = [] { return cover_position; };
    cover->state_getter = [] { return cover_state; };
    cover->command_setter = [](const PicoHA::Cover::Command command) {
        switch (command) {
            case PicoHA::Cover::Command::open:
                cover_state = PicoHA::Cover::State::opening;
                break;
            case PicoHA::Cover::Command::close:
                cover_state = PicoHA::Cover::State::closing;
                break;
            case PicoHA::Cover::Command::stop:
                cover_state = PicoHA::Cover::State::stopped;
                break;
        }
    };

    device.begin();

    Serial.println("Setup complete");
}

void loop() {
    mqtt.loop();
    device.loop();

    {
        static unsigned long last_event = millis();

        if (millis() - last_event > 5000) {
            Serial.println("Tick!");

            pingpong_event->trigger(ping_pong_state ? "ping" : "pong");
            ping_pong_state = !ping_pong_state;
            last_event = millis();

            binarino = !binarino;

            climate_state.current_temperature +=
                (climate_state.action == PicoHA::Climate::Action::heating)
                    ? 0.1
                    : ((climate_state.action ==
                        PicoHA::Climate::Action::cooling)
                           ? -0.1
                           : 0);

            switch (cover_state) {
                case PicoHA::Cover::State::opening:
                    cover_position += 20;
                    if (cover_position >= 100) {
                        cover_position = 100;
                        cover_state = PicoHA::Cover::State::open;
                    }
                    break;
                case PicoHA::Cover::State::closing:
                    cover_position -= 20;
                    if (cover_position <= 0) {
                        cover_position = 0;
                        cover_state = PicoHA::Cover::State::closed;
                    }
                    break;
                case PicoHA::Cover::State::stopped:
                case PicoHA::Cover::State::open:
                case PicoHA::Cover::State::closed:
                    break;
            }
        }
    }

    {
        // climate logicclimate_state
        if (climate_state.mode == PicoHA::Climate::Mode::off) {
            climate_state.action = PicoHA::Climate::Action::off;
        } else {
            if (climate_state.mode == PicoHA::Climate::Mode::heat) {
                climate_state.action =
                    (climate_state.current_temperature <
                     climate_state.target_temperature)
                        ? climate_state.action =
                              PicoHA::Climate::Action::heating
                        : climate_state.action = PicoHA::Climate::Action::idle;
            } else if (climate_state.mode == PicoHA::Climate::Mode::cool) {
                climate_state.action =
                    (climate_state.current_temperature >
                     climate_state.target_temperature)
                        ? climate_state.action =
                              PicoHA::Climate::Action::cooling
                        : climate_state.action = PicoHA::Climate::Action::idle;
            } else {
                climate_state.action = PicoHA::Climate::Action::idle;
            }
        }
    }
}
