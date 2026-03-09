#include <PicoMQTT.h>
#include <PicoUtils.h>

#include "PicoHA.h"

PicoUtils::PinOutput wifi_led(2, false);
PicoUtils::WiFiControlSmartConfig wifi_control(wifi_led);

PicoMQTT::Client mqtt;

PicoHA::Device device{mqtt, "PicoHA", "mlesniew", "picoha", ""};

bool binarino;
String text = "PicoHA";
String color = "Red";
int power = 10;

PicoHA::BinarySensor binary_sensor{device, "binarino", "Binarino"};
PicoHA::NumericSensor<size_t> text_lenght_sensor{device, "text_length",
                                                 "Text Length"};

PicoHA::Event pingpong_event{device, "pingpong", "Ping Pong"};
PicoHA::QueuedEvent reboot_event{device, "reboot", "Reboot"};

PicoHA::Switch onoff_switch{device, "onoff", "Toggle"};
PicoHA::Text text_input{device, "text", "Text"};
PicoHA::Button reset_button{device, "reset", "Reset"};
PicoHA::Select input_select{device, "color", "Color"};
PicoHA::Number<int> power_input{device, "power", "Power"};

PicoHA::ChildDevice climate_device{device,     "climate",        "Climate",
                                   "mlesniew", "picoha-climate", ""};

PicoHA::Climate climate{climate_device, "climate", ""};

struct {
    double current_temperature;
    double target_temperature;
    PicoHA::Climate::Mode mode;
    PicoHA::Climate::Action action;
} climate_state = {22.5, 24.0, PicoHA::Climate::Mode::heat,
                   PicoHA::Climate::Action::heating};

void setup() {
    Serial.begin(115200);
    wifi_led.init();
    WiFi.hostname("picoha");
    wifi_control.init();

    mqtt.host = "192.168.1.100";
    mqtt.begin();

    binary_sensor.bind(&binarino);
    binary_sensor.device_class = "power";

    text_lenght_sensor.getter = [] { return text.length(); };
    text_lenght_sensor.icon = "dog";

    pingpong_event.event_types = {"ping", "pong"};

    onoff_switch.bind(&binarino);
    text_input.bind(&text);

    reboot_event.trigger();

    reset_button.on_press = [] {
        text = "PicoHA";
        color = "Red";
        binarino = false;
        power = 10;
    };

    input_select.options = {"Red", "Yellow", "Green"};
    input_select.bind(&color);

    power_input.bind(&power);

    climate.min_temp = 15;
    climate.max_temp = 30;
    climate.temp_step = 0.5;
    climate.modes = {PicoHA::Climate::Mode::off, PicoHA::Climate::Mode::heat,
                     PicoHA::Climate::Mode::cool};
    climate.bind_mode(&climate_state.mode);
    climate.bind_action(&climate_state.action);
    climate.bind_current_temperature(&climate_state.current_temperature);
    climate.bind_target_temperature(&climate_state.target_temperature);
    climate.power_getter = [] {
        return climate_state.mode != PicoHA::Climate::Mode::off;
    };
    climate.power_setter = [](bool new_value) {
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
    wifi_control.tick();
    mqtt.loop();
    device.tick();

    {
        static unsigned long last_event = millis();
        static bool pong = false;

        if (millis() - last_event > 5000) {
            Serial.println("Tick!");

            pingpong_event.trigger(pong ? "pong" : "ping");
            last_event = millis();
            pong = !pong;

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