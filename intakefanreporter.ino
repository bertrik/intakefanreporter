#include <Arduino.h>

#include <ArduinoOTA.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

#define PIN_STATE   A0

static WiFiManager wifiManager;
static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);
static char esp_id[16];

static bool mqtt_send(const char *topic, const char *value, bool retained)
{
    bool result = false;
    if (!mqttClient.connected()) {
        Serial.print("Connecting MQTT...");
        result = mqttClient.connect(esp_id, topic, 0, retained, "unknown");
        Serial.println(result ? "OK" : "FAIL");
    }
    if (mqttClient.connected()) {
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
    ArduinoOTA.setPassword("intakefan");
    ArduinoOTA.begin();

    wifiManager.setConfigPortalTimeout(120);
    wifiManager.autoConnect("ESP-INTAKEFAN");

    mqttClient.setServer("mosquitto.space.revspace.nl", 1883);
}

void loop(void)
{
    static unsigned long last_tick = 0;
    static int last_state = -1;

    // check state once a second
    unsigned long tick = millis() / 1000;
    if (tick != last_tick) {
        last_tick = tick;

        int state = (analogRead(PIN_STATE) > 512);
        if (state != last_state) {
            // send a message if the state changed
            mqtt_send("revspace/intakefan/state", state ? "on" : "off", true);
            last_state = state;
        }
    }

    ArduinoOTA.handle();
    mqttClient.loop();
}
