#include <Arduino.h>

#include <ArduinoOTA.h>
#include "OTA_PASSWORD.h"
#include <WiFiManager.h>
#include <PubSubClient.h>

#define PIN_STATE   A0

#define MQTT_TOPIC  "revspace/intakefan/state"

static WiFiManager wifiManager;
static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);
static char esp_id[16];

static bool mqtt_send(const char *topic, const char *value, bool retained)
{
    bool result = mqttClient.connected();
    if (result) {
        Serial.print("Publishing ");
        Serial.print(value);
        Serial.print(" to ");
        Serial.print(topic);
        Serial.print("...");
        result = mqttClient.publish(topic, value, retained);
        Serial.println(result ? "OK" : "FAIL");
    }
    return result;
}

void setup(void)
{
    Serial.begin(115200);
    Serial.println("\nESP-INTAKEFAN");

    // get ESP id
    sprintf(esp_id, "%08X", ESP.getChipId());
    Serial.print("ESP ID: ");
    Serial.println(esp_id);

    ArduinoOTA.setHostname("esp-intakefan");
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.begin();

    wifiManager.setConfigPortalTimeout(120);
    wifiManager.autoConnect("ESP-INTAKEFAN");

    mqttClient.setServer("mosquitto.space.revspace.nl", 1883);
}

void loop(void)
{
    static unsigned long last_tick = 0;
    static int last_state = -1;
    static int fail_count = 0;

    // check state once a second
    unsigned long tick = millis() / 1000;
    if (tick != last_tick) {
        last_tick = tick;

        // try to stay connected
        if (mqttClient.connected()) {
            fail_count = 0;
        } else {
            Serial.print("Connecting MQTT...");
            if (!mqttClient.connect(esp_id, MQTT_TOPIC, 0, true, "unknown")) {
                fail_count++;
            }
            if (fail_count > 60) {
                Serial.println("MQTT connection failed, restarting ...");
                ESP.restart();
            }
        }

        // send a message if the state changed
        int state = (analogRead(PIN_STATE) > 512);
        if (state != last_state) {
            if (mqtt_send(MQTT_TOPIC, state ? "on" : "off", true)) {
                last_state = state;
            }
        }
    }

    ArduinoOTA.handle();
    mqttClient.loop();
}

