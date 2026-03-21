#include <PicoMQTT.h>

#include "PicoHA.h"

PicoMQTT::Client mqtt;

PicoHA::Device device{mqtt, "PicoHA", "mlesniew", "picoha", ""};

bool binarino;
String text = "PicoHA";
String color = "Red";
int power = 10;

PicoHA::BinarySensor * binary_sensor;
PicoHA::NumericSensor<size_t> * text_lenght_sensor;

PicoHA::Event * pingpong_event;
PicoHA::Event * reboot_event;

PicoHA::Switch * onoff_switch;
PicoHA::Text * text_input;
PicoHA::Button * reset_button;
PicoHA::Select * input_select;
PicoHA::Number<int> * power_input;

PicoHA::Climate * climate;

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
    WiFi.begin();

    mqtt.host = "192.168.1.100";
    mqtt.begin();

    PicoHA::add_diagnostic_entities(device);

    binary_sensor =
        &device.addEntity<PicoHA::BinarySensor>("binarino", "Binarino");
    binary_sensor->bind(&binarino);
    binary_sensor->device_class = "power";

    text_lenght_sensor = &device.addEntity<PicoHA::NumericSensor<size_t>>(
        "text_length", "Text Length");
    text_lenght_sensor->getter = [] { return text.length(); };
    text_lenght_sensor->icon = "dog";

    pingpong_event = &device.addEntity<PicoHA::Event>("pingpong", "Ping Pong");
    reboot_event = &device.addEntity<PicoHA::Event>("reboot", "Reboot");
    reboot_event->trigger();

    onoff_switch = &device.addEntity<PicoHA::Switch>("onoff", "Toggle");
    onoff_switch->bind(&binarino);

    text_input = &device.addEntity<PicoHA::Text>("text", "Text");
    text_input->bind(&text);

    reset_button = &device.addEntity<PicoHA::Button>("reset", "Reset");
    reset_button->on_press = [] {
        text = "PicoHA";
        color = "Red";
        binarino = false;
        power = 10;
    };

    input_select = &device.addEntity<PicoHA::Select>("color", "Color");
    input_select->options = {"Red", "Yellow", "Green"};
    input_select->bind(&color);

    power_input = &device.addEntity<PicoHA::Number<int>>("power", "Power");
    power_input->bind(&power);

    auto & climate_device = device.addChildDevice("climate", "Climate", "mlesniew", "picoha-climate", "");
    climate = &climate_device.addEntity<PicoHA::Climate>("climate", "");
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

    device.begin();

    Serial.println("Setup complete");
}

void loop() {
    mqtt.loop();
    device.tick();

    {
        static unsigned long last_event = millis();

        if (millis() - last_event > 5000) {
            Serial.println("Tick!");

            pingpong_event->trigger();
            last_event = millis();

            binarino = !binarino;

            climate_state.current_temperature +=
                (climate_state.action == PicoHA::Climate::Action::heating)
                    ? 0.1
                    : ((climate_state.action ==
                        PicoHA::Climate::Action::cooling)
                           ? -0.1
                           : 0);
        }
    }

    {
        // climate logic
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
