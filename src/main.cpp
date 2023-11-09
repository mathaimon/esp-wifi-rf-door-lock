#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <ElegantOTA.h>
#include <PubSubClient.h>
#include <RCSwitch.h>
#include <WiFi.h>
#include <config.h>

RCSwitch mySwitch = RCSwitch();
bool switchRelayFlag = false;
unsigned long recievedRfKey = 0;
unsigned long lastWifiReconnect = 0;
unsigned long lastMqttReconnect = 0;
String lastUnlockBy = "none";
unsigned long ota_progress_millis = 0;
bool show_ip = true;

WiFiClient ESPClient;
PubSubClient client(ESPClient);
AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  mySwitch.enableReceive(RfInterruptPin);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  Serial.println("\n[System] Starting Wireless Door Lock");

  // Initialize WiFi
  WiFi.setHostname(devName);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  if (!WiFi.config(device_local_IP, device_gateway, device_subnet,
                   device_primaryDNS, device_secondaryDNS)) {
    Serial.println("[WiFi] STA Failed to configure");
  }

  // Setup MDNS
  if (!MDNS.begin(devName)) {
    Serial.println("[MDNS]Error setting up MDNS responder!");
    while (1) {
      delay(500);
    }
  }

  // Initialize MQTT
  client.setServer(MQTT_Server, MQTT_Port);
  client.setCallback(MQTT_Subscription_Callback);

  // Setup Web Server
  // Send the device name as notifier
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    String response = "You have reached Dev : " + String(devName);
    request->send(200, "text/plain", response);
  });

  server.on("/restart", HTTP_GET,
            [](AsyncWebServerRequest* request) { ESP.restart(); });

  ElegantOTA.begin(&server);  // Start ElegantOTA
  ElegantOTA.setAuth(ota_username, ota_password);
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  server.begin();
  Serial.println("[WebServer] Http server started");
}

void loop() {
  // Authenticate and unlock using RF
  if (mySwitch.available()) {
    recievedRfKey = mySwitch.getReceivedValue();
    Serial.print("[RF] Recieved RF Key : ");
    Serial.println(recievedRfKey);

    if (isAuthorizedRF(recievedRfKey)) {
      lastUnlockBy = "rf";
      switchRelayFlag = true;
    }
    delay(1000);
    mySwitch.resetAvailable();
  }
  // Check if wifi is live and reconnect
  checkAndReconnectWifi();

  if (!client.connected() && WiFi.status() == WL_CONNECTED) {
    if (millis() - lastMqttReconnect > mqttReconnectDelay) {
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
    char buffer[256];
    DynamicJsonDocument docPub(1024);
    docPub["status"] = "unlocked";

    // Add the last unlocked by status
    if (lastUnlockBy == "rf") {
      // Add the rf key id to the message
      String _lastUnlockByString = "RF [";
      _lastUnlockByString += recievedRfKey;
      _lastUnlockByString += "]";
      docPub["actionBy"] = _lastUnlockByString;
    } else if (lastUnlockBy == "mqtt") {
      docPub["actionBy"] = "MQTT";
    }

    Serial.println("[Relay] Switching On Relay");
    digitalWrite(relayPin, HIGH);
    delay(relayOnDelay);
    Serial.println("[Relay] Switching Off Relay");
    // Followed the instructions from this example
    // https://arduinojson.org/v6/how-to/use-arduinojson-with-pubsubclient/
    size_t n = serializeJson(docPub, buffer);
    client.publish(MQTT_Publish_Topic, buffer, false);
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
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[WiFi] Failed to connect WiFi");
      show_ip = true;
    }
  }
  // Show the local IP of device
  // show_ip flag to show IP only Once
  if (WiFi.status() == WL_CONNECTED && show_ip) {
    Serial.print("[WiFi] Connected to WiFi. Dev IP : ");
    Serial.println(WiFi.localIP());
    show_ip = false;
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
    lastUnlockBy = "mqtt";
    switchRelayFlag = true;
  }
}

// OTA Callback Functions
void onOTAStart() {
  // Log when OTA has started
  Serial.println("[OTA] Update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("[OTA] Progress Current: %u bytes, Final: %u bytes\n",
                  current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("[OTA] update finished successfully!");
    ESP.restart();
  } else {
    Serial.println("[OTA] There was an error during OTA update!");
  }
  // <Add your own code here>
}
