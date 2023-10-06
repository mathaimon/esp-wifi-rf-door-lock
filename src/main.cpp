#include <Arduino.h>
#include <RCSwitch.h>
#include <config.h>

RCSwitch mySwitch = RCSwitch();
bool switchRelayFlag = false;
unsigned long recievedRfKey = 0;

void setup() {
  Serial.begin(115200);
  mySwitch.enableReceive(RfInterruptPin);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  Serial.println("\nAll Systems Ready");
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
    digitalWrite(relayPin, HIGH);
    delay(relayOnDelay);
    Serial.println("[Relay] Switching Off Relay");
    switchRelayFlag = false;
  } else {
    digitalWrite(relayPin, LOW);
  }
}
