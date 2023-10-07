#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <RCSwitch.h>
#include <WiFi.h>
#include <config.h>

RCSwitch mySwitch = RCSwitch();
bool switchRelayFlag = false;
unsigned long recievedRfKey = 0;

WiFiClient ESPClient;
PubSubClient client(ESPClient);

void setup() {
  Serial.begin(115200);
  mySwitch.enableReceive(RfInterruptPin);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  // Initialize WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  if (!WiFi.config(device_local_IP, device_gateway, device_subnet,
                   device_primaryDNS, device_secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed");
    while (1) {
      delay(1000);
    }
  } else {
    Serial.print("ESP IP Address : ");
    Serial.println(WiFi.localIP());
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

  if (!client.connected()) {
    MQTT_Reconnect();
  }

  client.loop();

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

void MQTT_Reconnect() {
  while (!client.connected()) {
    Serial.print("[MQTT] Attempting MQTT connection...");
    String clientId = "ESP32Client-";  // Random Client Id
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");

      client.subscribe(MQTT_Subscription_Topic);
    } else {
      Serial.print("[MQTT] failed, rc=");
      Serial.print(client.state());
      delay(1000);
    }
  }
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
