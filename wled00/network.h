#pragma once

#ifndef WLED_NETWORK_H
#define WLED_NETWORK_H

#ifdef ARDUINO_ARCH_ESP32
// Declare internal Arduino library function to work around the fact that WiFi.setHostname() does not propagate the
// hostname to the network interface if it's been previously used.  The underlying ESP-IDF layer supports this just
// fine; it's an oversight in the Arduino layer that we can work around by calling the internal function directly.
esp_err_t set_esp_interface_hostname(esp_interface_t interface, const char * hostname);
#endif

#ifndef WLED_DISABLE_ESPNOW
typedef struct {
  char    magic[4];     // enough to store "WLED"
  uint8_t packet:4;     // packet sequence
  uint8_t noOfPackets:4;// total number of packets
  uint8_t data[245];    // payload
} __attribute__((packed, aligned(1))) EspNowPartialPacket;

typedef struct {
  char     magic[4];    // enough to store "WLED"
  uint8_t  version:4;   // message packet version (changes when packet size changes); not intended to change beyond 15 (0 means unspecified/irrelevant)
  uint8_t  channel:4;   // master's WiFi channel used
  uint32_t time;        // may be used for time synchronisation (NOTE: time_t varies in size on ESP32 and ESP8266)
  uint8_t  reserved[7]; // 7 bytes reserved for future use
} __attribute__((packed, aligned(1))) EspNowBeacon;

void initESPNow(bool resetAP = false);
void stopESPNow();
void sendESPNowHeartBeat();
#endif

typedef struct WiFiConfig {
  char clientSSID[33];
  char clientPass[65];
  uint8_t bssid[6];
  IPAddress staticIP;
  IPAddress staticGW;
  IPAddress staticSN;
  WiFiConfig(const char *ssid="", const char *pass="", IPAddress ip = (uint32_t)0, IPAddress gw = (uint32_t)0, IPAddress subnet = (uint32_t)0x00FFFFFFU) // little endian
  : staticIP(ip)
  , staticGW(gw)
  , staticSN(subnet)
  {
      strlcpy(clientSSID, ssid, 33);
      strlcpy(clientPass, pass, 65);
      memset(bssid, 0, sizeof(bssid));
  }
} wifi_config;

#ifdef WLED_USE_ETHERNET
// For ESP32, the remaining five pins are at least somewhat configurable.
// eth_address  is in range [0..31], indicates which PHY (MAC?) address should be allocated to the interface
// eth_power    is an output GPIO pin used to enable/disable the ethernet port (and/or external oscillator)
// eth_mdc      is an output GPIO pin used to provide the clock for the management data
// eth_mdio     is an input/output GPIO pin used to transfer management data
// eth_type     is the physical ethernet module's type (ETH_PHY_LAN8720, ETH_PHY_TLK110)
// eth_clk_mode defines the GPIO pin and GPIO mode for the clock signal
//              However, there are really only four configurable options on ESP32:
//              ETH_CLOCK_GPIO0_IN    == External oscillator, clock input  via GPIO0
//              ETH_CLOCK_GPIO0_OUT   == ESP32 provides 50MHz clock output via GPIO0
//              ETH_CLOCK_GPIO16_OUT  == ESP32 provides 50MHz clock output via GPIO16
//              ETH_CLOCK_GPIO17_OUT  == ESP32 provides 50MHz clock output via GPIO17
typedef struct EthernetSettings {
  uint8_t        eth_address;
  int            eth_power;
  int            eth_mdc;
  int            eth_mdio;
  eth_phy_type_t eth_type;
  eth_clock_mode_t eth_clk_mode;
} ethernet_settings;

extern const ethernet_settings ethernetBoards[];

#include "pin_manager.h"
#define WLED_ETH_RSVD_PINS_COUNT 6
extern const managed_pin_type esp32_nonconfigurable_ethernet_pins[WLED_ETH_RSVD_PINS_COUNT];
#endif

bool initEthernet(); // result is informational
int  getSignalQuality(int rssi);
IPAddress resolveHostname(const String& hostname, bool useMDNS = true);
void fillMAC2Str(char *str, const uint8_t *mac);
void fillStr2MAC(uint8_t *mac, const char *str);
int  findWiFi(bool doScan = false);
bool isWiFiConfigured();
void WiFiEvent(WiFiEvent_t event);

#endif // WLED_NETWORK_H