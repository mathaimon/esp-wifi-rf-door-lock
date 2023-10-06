#ifndef CONFIG_H
#define CONFIG_H

// Declare I/O Pins
int RfInterruptPin = 13;
int relayPin = 12;

// Delays and timings
int relayOnDelay = 3000;  // Duration for the relay to remain powered

// Authorized RF Keys
unsigned long authorizedRFKeys[] = {8112609, 4755297};  // Add your own RF Keys
int numAuthorizedRFKeys =
    sizeof(authorizedRFKeys) / sizeof(authorizedRFKeys[0]);

// Function Declarations

// Return true if the entered key value is authorized
bool isAuthorizedRF(unsigned long keyIdDecimal);

// Switch on relay when the switchRelayFlag flag is set to true
void controlRealy();

#endif
