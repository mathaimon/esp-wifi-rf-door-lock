#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <RCSwitch.h>
#include <WiFi.h>
#include <config.h>

RCSwitch mySwitch = RCSwitch();
bool switchRelayFlag = false;
unsigned long recievedRfKey = 0;
unsigned long lastWifiReconnect = 0;
unsigned long lastMqttReconnect = 0;

WiFiClient ESPClient;
PubSubClient client(ESPClient);

void setup() {
  Serial.begin(115200);
  mySwitch.enableReceive(RfInterruptPin);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  Serial.println("\n[System] Starting Wireless Door Lock");

  // Initialize WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  if (!WiFi.config(device_local_IP, device_gateway, device_subnet,
                   device_primaryDNS, device_secondaryDNS)) {
    Serial.println("[WiFi] STA Failed to configure");
  }

  // Initialize MQTT
  client.setServer(MQTT_Server, MQTT_Port);
  client.setCallback(MQTT_Subscription_Callback);
}

void loop() {
  if (mySwitch.available()) {
    recievedRfKey = mySwitch.getReceivedValue();
    Serial.print("[RF] Recieved RF Key : ");
    Serial.println(recievedRfKey);

    if (isAuthorizedRF(recievedRfKey)) {
      switchRelayFlag = true;
    }
    mySwitch.resetAvailable();
  }
  // Check if wifi is live and reconnect
  checkAndReconnectWifi();

  if (!client.connected() && WiFi.status() == WL_CONNECTED) {
    if (millis() - lastMqttReconnect > 5000) {
      lastMqttReconnect = millis();
      if (MQTT_Reconnect()) {
        lastMqttReconnect = 0;
      }
    }
  } else {
    client.loop();
  }

  // Control the relay with appropriate flag
  controlRealy();
}

bool isAuthorizedRF(unsigned long keyIdDecimal) {
  for (int i = 0; i < numAuthorizedRFKeys; i++) {
    if (keyIdDecimal == authorizedRFKeys[i]) {
      Serial.println("[Auth] Valid Key");
      return true;
    }
  }
  Serial.println("[Auth] Invalid Key");
  return false;
}

void controlRealy() {
  if (switchRelayFlag) {
    Serial.println("[Relay] Switching On Relay");
    client.publish(MQTT_Publish_Topic, "open");
    digitalWrite(relayPin, HIGH);
    delay(relayOnDelay);
    Serial.println("[Relay] Switching Off Relay");
    client.publish(MQTT_Publish_Topic, "close");
    switchRelayFlag = false;
  } else {
    digitalWrite(relayPin, LOW);
  }
}

void checkAndReconnectWifi() {
  if ((WiFi.status() != WL_CONNECTED) &&
      (millis() - lastWifiReconnect > wifiReconnectInterval)) {
    Serial.println("[WiFi] Attempting Reconnect");
    WiFi.disconnect();
    WiFi.reconnect();
    lastWifiReconnect = millis();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("[WiFi] Connected to WiFi. Dev IP : ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("[WiFi] Failed to connect WiFi");
    }
  }
}

bool MQTT_Reconnect() {
  Serial.print("[MQTT] Attempting MQTT connection...");
  String clientId = "ESP32Client-";  // Random Client Id
  clientId += String(random(0xffff), HEX);

  if (client.connect(clientId.c_str())) {
    Serial.println("connected");
    // Set Subscriptions
    client.subscribe(MQTT_Subscription_Topic);
  } else {
    Serial.print("[MQTT] failed, rc=");
    Serial.println(client.state());
  }
  return client.connected();
}

void MQTT_Subscription_Callback(char* topic, byte* payload,
                                unsigned int length) {
  Serial.print("[MQTT] Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  DynamicJsonDocument docSub(1024);
  deserializeJson(docSub, payload);
  const char* command = docSub["cmd"];
  if (strcmp(command, "open") == 0) {
    switchRelayFlag = true;
  }
}
