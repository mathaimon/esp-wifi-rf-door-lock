#ifndef CONFIG_H
#define CONFIG_H
#include <WiFi.h>

// Declare I/O Pins
int RfInterruptPin = 13;
int relayPin = 12;

// Delays and timings
int relayOnDelay = 1000;  // Duration for the relay to remain powered
unsigned long mqttReconnectDelay =
    10000;  // Delay to retry failed mqtt connection
unsigned long wifiReconnectInterval =
    5000;  // Delay to retry failed wifi connection

// Authorized RF Keys
unsigned long authorizedRFKeys[] = {1111111, 1111111};  // add your own keys
int numAuthorizedRFKeys =
    sizeof(authorizedRFKeys) / sizeof(authorizedRFKeys[0]);

// Device Name for MDNS
const char* devName = "main-gate";

// OTA Credentials
const char* ota_username = "<username>";
const char* ota_password = "<password>";

// WiFi Credentials
const char* wifi_ssid = "<Your SSID>";
const char* wifi_password = "<Your Password>";

// Wifi Config
// IPAddress device_local_IP(192, 168, 68, 250); // Static IP
IPAddress device_local_IP(0, 0, 0, 0);  // Dynamic IP
IPAddress device_gateway(192, 168, 68, 1);
IPAddress device_subnet(255, 255, 0, 0);
IPAddress device_primaryDNS(1, 1, 1, 1);
IPAddress device_secondaryDNS(1, 0, 0, 1);

// MQTT Config
const char* MQTT_Server = "192.168.68.254";
const int MQTT_Port = 1883;  // default mqtt port

const char* MQTT_Subscription_Topic = "/maingate/cmd";
const char* MQTT_Publish_Topic = "/maingate/status";

// Initialize Functions    *******************************

// Return true if the entered key value is authorized
bool isAuthorizedRF(unsigned long keyIdDecimal);

// Switch on relay when the switchRelayFlag flag is set to true
void controlRealy();

// Check if WiFi is alive and try reconnect
void checkAndReconnectWifi();

// Reconnect the Mqtt Connection
bool MQTT_Reconnect();
void MQTT_Subscription_Callback(char* topic, byte* payload,
                                unsigned int length);

// OTA Callback Functions
void onOTAStart();
void onOTAProgress(size_t current, size_t final);
void onOTAEnd(bool success);

#endif
