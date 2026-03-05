#include <PicoMQTT.h>
#include <PicoUtils.h>

#include "PicoHA.h"

PicoUtils::PinOutput wifi_led(2, false);
PicoUtils::WiFiControlSmartConfig wifi_control(wifi_led);

PicoMQTT::Client mqtt;

PicoHA::RootDevice device{mqtt, "PicoHA", "mlesniew", "picoha", ""};

bool binarino;

PicoHA::BinarySensor binary_sensor{device, "binarino", "Binarino"};
PicoHA::NumericSensor<unsigned long> uptime_sensor{device, "uptime", "Uptime"};
PicoHA::Event pingpong_event{device, "pingpong", "Ping Pong"};
PicoHA::QueuedEvent reboot_event{device, "reboot", "Reboot"};

void setup() {
    Serial.begin(115200);
    wifi_led.init();
    WiFi.hostname("picoha");
    wifi_control.init();

    mqtt.host = "192.168.1.100";
    mqtt.begin();

    binary_sensor.bind(&binarino);
    binary_sensor.device_class = "power";

    uptime_sensor.bind([] { return millis() / 1000; });
    uptime_sensor.is_diagnostic = true;
    uptime_sensor.state_class = "total_increasing";
    uptime_sensor.icon = "dog";

    pingpong_event.event_types = {"ping", "pong"};

    reboot_event.trigger();

    device.begin();
}

void loop() {
    wifi_control.tick();
    mqtt.loop();
    device.tick();
    binarino = (millis() / 60000) % 2;

    {
        static unsigned long last_event = millis();
        static bool pong = false;

        if (millis() - last_event > 5000) {
            pingpong_event.trigger(pong ? "pong" : "ping");
            last_event = millis();
            pong = !pong;
        }
    }
}