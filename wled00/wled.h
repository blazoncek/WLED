#ifndef WLED_H
#define WLED_H
/*
   Main sketch, global variable declarations
   @title WLED project sketch
   @version 0.15.3-b2
   @author 2016-2024 Christian Schwinne (@Aircookie), 2021-2026 Blaz Kristan (@blazoncek)
 */

// version code in format yymmddb (b = daily build)
#ifndef AUTOBUILD
#define VERSION 2608180
#else
#define VERSION BUILD
#endif

//uncomment this if you have a "my_config.h" file you'd like to use
//#define WLED_USE_MY_CONFIG

// ESP8266-01 (blue) got too little storage space to work with WLED. 0.10.2 is the last release supporting this unit.

// ESP8266-01 (black) has 1MB flash and can thus fit the whole program, although OTA update is not possible. Use 1M(128K SPIFFS).
// 2-step OTA may still be possible: https://github.com/wled/WLED/issues/2040#issuecomment-981111096
// Uncomment some of the following lines to disable features:
// Alternatively, with platformio pass your chosen flags to your custom build target in platformio_override.ini

// You are required to disable ArduinoOTA over-the-air updates:
//#define WLED_DISABLE_OTA         // saves 14kb
#ifdef WLED_ENABLE_AOTA
  #if defined(WLED_DISABLE_OTA)
    #warning WLED_DISABLE_OTA was defined but it will be ignored due to WLED_ENABLE_AOTA.
  #endif
  #undef WLED_DISABLE_OTA
#endif

// You can choose some of these features to disable:
//#define WLED_DISABLE_ALEXA       // saves 11kb
//#define WLED_DISABLE_HUESYNC     // saves 4kb
//#define WLED_DISABLE_INFRARED    // saves 12kb, there is no pin left for this on ESP8266-01
#ifndef WLED_DISABLE_MQTT
  #define WLED_ENABLE_MQTT         // saves 12kb
#endif
#ifndef WLED_DISABLE_ADALIGHT      // can be used to disable reading commands from serial RX pin (see issue #3128).
  #define WLED_ENABLE_ADALIGHT     // disable saves 5Kb (uses GPIO3 (RX) for serial). Related serial protocols: Adalight/TPM2, Improv, Serial JSON, Continuous Serial Streaming
#else
  #undef WLED_ENABLE_ADALIGHT      // disable has priority over enable
#endif
//#define WLED_ENABLE_DMX          // uses 3.5kb
#ifndef WLED_DISABLE_LOXONE
  #define WLED_ENABLE_LOXONE       // uses 1.2kb
#endif
#ifndef WLED_DISABLE_WEBSOCKETS
  #define WLED_ENABLE_WEBSOCKETS
#else
  #define WLED_ENABLE_JSONLIVE     // peek LED output via /json/live (WS binary peek is always enabled)
#endif

//#define WLED_DISABLE_ESPNOW      // Removes dependence on esp now

#define WLED_ENABLE_FS_EDITOR      // enable /edit page for editing FS content. Will also be disabled with OTA lock

// to toggle usb serial debug (un)comment the following line
//#define WLED_DEBUG

// filesystem specific debugging
//#define WLED_DEBUG_FS

#ifndef WLED_WATCHDOG_TIMEOUT
  // 3 seconds should be enough to detect a lockup
  // define WLED_WATCHDOG_TIMEOUT=0 to disable watchdog, default
  #define WLED_WATCHDOG_TIMEOUT 0
#endif

//optionally disable brownout detector on ESP32.
//This is generally a terrible idea, but improves boot success on boards with a 3.3v regulator + cap setup that can't provide 400mA peaks
//#define WLED_DISABLE_BROWNOUT_DET

#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>

// Library inclusions.
#include <Arduino.h>
#ifdef ESP8266
  #ifdef WLED_ENABLE_HUB75MATRIX
    #undef WLED_ENABLE_HUB75MATRIX
  #endif
  #include <ESP8266WiFi.h>
  #include <ESP8266mDNS.h>
  #include <ESPAsyncTCP.h>
  #include <LittleFS.h>
  #define WLED_FS LittleFS
  extern "C"
  {
  #include <user_interface.h>
  }
  #define WIFI_MODE_STA WIFI_STA
  #define WIFI_MODE_AP WIFI_AP
  #define WIFI_MODE_APSTA WIFI_AP_STA
  #ifndef WLED_DISABLE_ESPNOW
    #include <espnow.h>
    #include <QuickEspNow.h>
  #endif
#else // ESP32
  #if defined(CONFIG_IDF_TARGET_ESP32C3)
    #ifdef WLED_ENABLE_HUB75MATRIX
      #undef WLED_ENABLE_HUB75MATRIX
    #endif
  #endif
  #include <HardwareSerial.h>  // ensure we have the correct "Serial" on new MCUs (depends on ARDUINO_USB_MODE and ARDUINO_USB_CDC_ON_BOOT)
  #include <WiFi.h>
  #include <ETH.h>
  #include "esp_wifi.h"
  #include <ESPmDNS.h>
  #include <AsyncTCP.h>
  #if LOROL_LITTLEFS
    #ifndef CONFIG_LITTLEFS_FOR_IDF_3_2
      #define CONFIG_LITTLEFS_FOR_IDF_3_2
    #endif
    #include <LITTLEFS.h>
    #define WLED_FS LITTLEFS
  #else
    #include <LittleFS.h>
    #define WLED_FS LittleFS
  #endif
  #include "esp_task_wdt.h"

  #ifndef WLED_DISABLE_ESPNOW
    #include <esp_now.h>
    #include <QuickEspNow.h>
  #endif
#endif
#include <Wire.h>
#include <SPI.h>
#ifndef ESP8266
  #include <SD.h>
#endif

#include "src/dependencies/network/Network.h"

#ifdef WLED_USE_MY_CONFIG
  #include "my_config.h"
#endif

#include <ESPAsyncWebServer.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <SPIFFSEditor.h>
#include "src/dependencies/time/TimeLib.h"
#include "src/dependencies/timezone/Timezone.h"
#include "src/dependencies/toki/Toki.h"

#ifndef WLED_DISABLE_ALEXA
  #define ESPALEXA_ASYNC
  #define ESPALEXA_NO_SUBPAGE
  #define ESPALEXA_MAXDEVICES 10
  // #define ESPALEXA_DEBUG
  #include "src/dependencies/espalexa/Espalexa.h"
  #include "src/dependencies/espalexa/EspalexaDevice.h"
#endif

#ifdef WLED_ENABLE_DMX
 #if defined(ESP8266) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S2)
  #include "src/dependencies/dmx/ESPDMX.h"
 #else //ESP32
  #include "src/dependencies/dmx/SparkFunDMX.h"
 #endif
#endif

#include "src/dependencies/e131/ESPAsyncE131.h"
#ifndef WLED_DISABLE_MQTT
#include <AsyncMqttClient.h>
#endif

#define ARDUINOJSON_DECODE_UNICODE 0
#include "src/dependencies/json/AsyncJson-v6.h"
#include "src/dependencies/json/ArduinoJson-v6.h"

#define FASTLED_INTERNAL //remove annoying pragma messages
#define USE_GET_MILLISECOND_TIMER
#include "FastLED.h"
// fastled sin16 replacement
#ifdef sin16
  #undef sin16
#endif
#define sin16(x) sin16_t(x)
#define cos16(x) cos16_t(x)
#include "const.h"
#include "colors.h"
#include "fcn_declare.h"
#include "network.h"
#include "NodeStruct.h"
#include "pin_manager.h"
#include "bus_manager.h"
#include "um_manager.h"
#include "FX.h"

// ESP32-WROVER features SPI RAM (aka PSRAM) which can be allocated using ps_malloc()
// we can create custom PSRAMDynamicJsonDocument to use such feature (replacing DynamicJsonDocument)
// The following is a construct to enable code to compile without it.
// There is a code that will still not use PSRAM though:
//    AsyncJsonResponse is a derived class that implements DynamicJsonDocument (AsyncJson-v6.h)
#if defined(ARDUINO_ARCH_ESP32) && defined(BOARD_HAS_PSRAM)
struct PSRAM_Allocator {
  static inline void* allocate(size_t size)                  { return d_malloc(size); }
  static inline void* reallocate(void* ptr, size_t new_size) { return d_realloc(ptr, new_size); }
  static inline void  deallocate(void* pointer)              { d_free(pointer); }
};
using PSRAMDynamicJsonDocument = BasicJsonDocument<PSRAM_Allocator>;
#else
#define PSRAMDynamicJsonDocument DynamicJsonDocument
#endif

#ifndef SPIFFS_EDITOR_AIRCOOOKIE
  #error You are not using the Aircoookie fork of the ESPAsyncWebserver library.\
  Using upstream puts your WiFi password at risk of being served by the filesystem.\
  Comment out this error message to build regardless.
#endif

#ifndef WLED_DISABLE_INFRARED
  #define _IR_ENABLE_DEFAULT_ false
  #include <IRremoteESP8266.h>
  #include <IRrecv.h>
  #include <IRutils.h>
#endif

// GLOBAL VARIABLES
// both declared and defined in header (solution from http://www.keil.com/support/docs/1868.htm)
//
//e.g. byte test = 2 becomes WLED_GLOBAL byte test _INIT(2);
//     int arr[]{0,1,2} becomes WLED_GLOBAL int arr[] _INIT_N(({0,1,2}));

#ifndef WLED_DEFINE_GLOBAL_VARS
  #define WLED_GLOBAL extern
  #define _INIT(x)
  #define _INIT_N(x)
  #define _INIT_PROGMEM(x)
#else
  #define WLED_GLOBAL
  #define _INIT(x) = x
  //needed to ignore commas in array definitions
  #define UNPACK( ... ) __VA_ARGS__
  #define _INIT_N(x) UNPACK x
  #define _INIT_PROGMEM(x) PROGMEM = x
#endif

#define STRINGIFY(X) #X
#define TOSTRING(X) STRINGIFY(X)
#ifdef countof
  #undef countof
#endif
#define countof(x) (sizeof(x)/sizeof(x[0]))

// Global Variable definitions
WLED_GLOBAL unsigned build _INIT(VERSION);
WLED_GLOBAL char versionString[] _INIT_PROGMEM(TOSTRING(WLED_VERSION));
WLED_GLOBAL char releaseString[] _INIT_PROGMEM(WLED_RELEASE_NAME); // somehow this will not work if using "const char releaseString[]
#define WLED_CODENAME "Kōsen"

// AP and OTA default passwords (for maximum security change them!)
WLED_GLOBAL char apPass[65]  _INIT(WLED_AP_PASS);
WLED_GLOBAL char otaPass[33] _INIT(DEFAULT_OTA_PASS);

WLED_GLOBAL int8_t rlyPin       _INIT(RLYPIN);
WLED_GLOBAL bool   rlyMde       _INIT(RLYMDE);      // Relay mode (1 = active high, 0 = active low, flipped in cfg.json)
WLED_GLOBAL bool   rlyOpenDrain _INIT(RLYODRAIN);   // Use open drain (floating pin) when relay should be off
WLED_GLOBAL uint8_t       rlyDelay     _INIT(0);    // delay after switching relay (in 1/100 seconds)
WLED_GLOBAL unsigned long rlyStartTime _INIT(0UL);  // needed for power-on delay (only set during delay itself)

#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S2) || (defined(RX) && defined(TX))
  // use RX/TX as set by the framework - these boards do _not_ have RX=3 and TX=1
  constexpr uint8_t hardwareRX = RX;
  constexpr uint8_t hardwareTX = TX;
#else
  // use defaults for RX/TX
  constexpr uint8_t hardwareRX = 3;
  constexpr uint8_t hardwareTX = 1;
#endif

WLED_GLOBAL char ntpServerName[33] _INIT("0.wled.pool.ntp.org");   // NTP server to use

// WiFi CONFIG (all these can be changed via web UI, no need to set them here)
WLED_GLOBAL std::vector<WiFiConfig> multiWiFi _INIT_N(({{CLIENT_SSID,CLIENT_PASS}})); // initialise vector with default WiFi
WLED_GLOBAL IPAddress dnsAddress _INIT_N(((  8,   8,  8,  8)));   // Google's DNS
WLED_GLOBAL char hostName[33]    _INIT(MDNS_NAME);                // mDNS address (*.local, replaced by wled-XXXXXX if default is used)
WLED_GLOBAL bool mDNSenabled     _INIT(true);                     // use mDNS (default is true, can be changed in web UI)
WLED_GLOBAL char apSSID[33]      _INIT("");                       // AP off by default (unless setup)
WLED_GLOBAL int8_t selectedWiFi  _INIT(0);
WLED_GLOBAL byte apChannel       _INIT(1);                        // 2.4GHz WiFi AP channel (1-13)
WLED_GLOBAL byte apHide          _INIT(0);                        // hidden AP SSID
  #ifndef WLED_AP_TIMEOUT
WLED_GLOBAL byte apBehavior      _INIT(AP_BEHAVIOR_BOOT_NO_CONN); // access point opens when no connection after boot by default
  #else
WLED_GLOBAL byte apBehavior      _INIT(AP_BEHAVIOR_TEMPORARY); // access point opens when no connection after boot by default
  #endif
  #ifdef ARDUINO_ARCH_ESP32
WLED_GLOBAL bool noWifiSleep _INIT(true);                         // disabling modem sleep modes will increase heat output and power usage, but may help with connection issues
  #else
WLED_GLOBAL bool noWifiSleep _INIT(false);
  #endif
WLED_GLOBAL bool force802_3g _INIT(false);
#ifdef ARDUINO_ARCH_ESP32
  #if defined(LOLIN_WIFI_FIX) && (defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3))
WLED_GLOBAL uint8_t txPower _INIT(WIFI_POWER_8_5dBm);
  #else
WLED_GLOBAL uint8_t txPower _INIT(WIFI_POWER_19_5dBm);
  #endif
#endif

#ifdef WLED_USE_ETHERNET
  #ifdef WLED_ETH_DEFAULT                                          // default ethernet board type if specified
    WLED_GLOBAL int ethernetType _INIT(WLED_ETH_DEFAULT);          // ethernet board type
  #else
    WLED_GLOBAL int ethernetType _INIT(WLED_ETH_NONE);             // use none for ethernet board type if default not defined
  #endif
#endif

// LED CONFIG
WLED_GLOBAL bool turnOnAtBoot _INIT(true);                // turn on LEDs at power-up
WLED_GLOBAL byte bootPreset   _INIT(0);                   // save preset to load after power-up

//if true, a segment per bus will be created on boot and LED settings save
//if false, only one segment spanning the total LEDs is created,
//but not on LED settings save if there is more than one segment currently
#if defined(ARDUINO_ARCH_ESP32) && !defined(CONFIG_IDF_TARGET_ESP32C3)
WLED_GLOBAL bool useParallelI2S     _INIT(true);  // parallel I2S for ESP32
#endif
#ifdef WLED_USE_IC_CCT
WLED_GLOBAL bool cctICused          _INIT(true);  // CCT IC used (Athom 15W bulbs)
#else
WLED_GLOBAL bool cctICused          _INIT(false); // CCT IC used (Athom 15W bulbs)
#endif
WLED_GLOBAL bool gammaCorrectCol    _INIT(true);  // use gamma correction on colors
WLED_GLOBAL bool gammaCorrectBri    _INIT(false); // use gamma correction on brightness
WLED_GLOBAL float gammaCorrectVal   _INIT(2.2f);  // gamma correction value
WLED_GLOBAL uint32_t (*gamma32Func)(uint32_t) _INIT(nullGamma32);

WLED_GLOBAL byte colPri[] _INIT_N(({ 255, 160, 0, 0 }));  // current RGB(W) primary color. colPri[] should be updated if you want to change the color.
WLED_GLOBAL byte colSec[] _INIT_N(({ 0, 0, 0, 0 }));      // current RGB(W) secondary color

WLED_GLOBAL byte briMultiplier _INIT(100);          // % of brightness to set (to limit power, if you set it to 50 and set bri to 255, actual brightness will be 127)

// User Interface CONFIG
#ifndef SERVERNAME
WLED_GLOBAL char serverDescription[33] _INIT("WLED");  // Name of module - use default
#else
WLED_GLOBAL char serverDescription[33] _INIT(SERVERNAME);  // use predefined name
#endif
WLED_GLOBAL bool simplifiedUI          _INIT(false);   // enable simplified UI
WLED_GLOBAL byte cacheInvalidate       _INIT(0);       // used to invalidate browser cache

// Sync CONFIG
WLED_GLOBAL NodesMap Nodes;
WLED_GLOBAL bool nodeListEnabled _INIT(true);
WLED_GLOBAL bool nodeBroadcastEnabled _INIT(true);

#ifndef WLED_DISABLE_INFRARED
WLED_GLOBAL int8_t irPin        _INIT(IRPIN);
WLED_GLOBAL byte irEnabled      _INIT(IRTYPE); // Infrared receiver
#endif
WLED_GLOBAL bool irApplyToAllSelected _INIT(true); //apply IR or ESP-NOW to all selected segments

#ifndef WLED_DISABLE_ALEXA
WLED_GLOBAL bool alexaEnabled _INIT(false);                       // enable device discovery by Amazon Echo
WLED_GLOBAL char alexaInvocationName[33] _INIT("Light");          // speech control name of device. Choose something voice-to-text can understand
WLED_GLOBAL byte alexaNumPresets _INIT(0);                        // number of presets to expose to Alexa, starting from preset 1, up to 9
#endif

WLED_GLOBAL uint16_t realtimeTimeoutMs _INIT(2500);               // ms timeout of realtime mode before returning to normal mode
WLED_GLOBAL int arlsOffset _INIT(0);                              // realtime LED offset
WLED_GLOBAL bool arlsDisableGammaCorrection _INIT(true);          // activate if gamma correction is handled by the source
WLED_GLOBAL bool arlsForceMaxBri _INIT(false);                    // enable to force max brightness if source has very dark colors that would be black

#ifdef WLED_ENABLE_DMX
 #if defined(ESP8266) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S2)
  WLED_GLOBAL DMXESPSerial dmx;
 #else //ESP32
  WLED_GLOBAL SparkFunDMX dmx;
 #endif
  WLED_GLOBAL uint16_t e131ProxyUniverse _INIT(0);                  // output this E1.31 (sACN) / ArtNet universe via MAX485 (0 = disabled)
  // dmx CONFIG
  WLED_GLOBAL byte DMXChannels _INIT(7);        // number of channels per fixture
  WLED_GLOBAL byte DMXFixtureMap[15] _INIT_N(({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }));
  // assigns the different channels to different functions. See wled21_dmx.ino for more information.
  WLED_GLOBAL uint16_t DMXGap _INIT(10);          // gap between the fixtures. makes addressing easier because you don't have to memorize odd numbers when climbing up onto a rig.
  WLED_GLOBAL uint16_t DMXStart _INIT(10);        // start address of the first fixture
  WLED_GLOBAL uint16_t DMXStartLED _INIT(0);      // LED from which DMX fixtures start
#endif
WLED_GLOBAL uint16_t e131Universe      _INIT(1);                      // settings for E1.31 (sACN) protocol (only DMX_MODE_MULTIPLE_* can span over consecutive universes)
WLED_GLOBAL uint16_t e131Port          _INIT(5568);                   // DMX in port. E1.31 default is 5568, Art-Net is 6454
WLED_GLOBAL byte e131Priority          _INIT(0);                      // E1.31 port priority (if != 0 priority handling is active)
WLED_GLOBAL E131Priority highPriority  _INIT(3);                      // E1.31 highest priority tracking, init = timeout in seconds
WLED_GLOBAL byte DMXMode               _INIT(DMX_MODE_MULTIPLE_RGB);  // DMX mode (s.a.)
WLED_GLOBAL uint16_t DMXAddress        _INIT(1);                      // DMX start address of fixture, a.k.a. first Channel [for E1.31 (sACN) protocol]
WLED_GLOBAL uint16_t DMXSegmentSpacing _INIT(0);                      // Number of void/unused channels between each segments DMX channels
WLED_GLOBAL byte e131LastSequenceNumber[E131_MAX_UNIVERSE_COUNT] _INIT_N(({0})); // to detect packet loss; 1 mandatory for DDP
WLED_GLOBAL bool e131Multicast         _INIT(false);                  // multicast or unicast
WLED_GLOBAL bool e131SkipOutOfSequence _INIT(false);                  // freeze instead of flickering
WLED_GLOBAL uint16_t pollReplyCount    _INIT(0);                      // count number of replies for ArtPoll node report

// mqtt
WLED_GLOBAL unsigned long lastMqttReconnectAttempt _INIT(0);  // used for other periodic tasks too
#ifndef WLED_DISABLE_MQTT
  #ifndef MQTT_MAX_TOPIC_LEN
    #define MQTT_MAX_TOPIC_LEN 32
  #endif
  #ifndef MQTT_MAX_SERVER_LEN
    #define MQTT_MAX_SERVER_LEN 32
  #endif
  WLED_GLOBAL AsyncMqttClient *mqtt _INIT(NULL);
  WLED_GLOBAL bool mqttEnabled _INIT(false);
  WLED_GLOBAL char mqttStatusTopic[MQTT_MAX_TOPIC_LEN + 8] _INIT("");         // this must be global because of async handlers
  WLED_GLOBAL char mqttDeviceTopic[MQTT_MAX_TOPIC_LEN + 1] _INIT("");         // main MQTT topic (individual per device, default is wled/mac)
  WLED_GLOBAL char mqttGroupTopic[MQTT_MAX_TOPIC_LEN + 1]  _INIT("wled/all"); // second MQTT topic (for example to group devices)
  WLED_GLOBAL char mqttServer[MQTT_MAX_SERVER_LEN + 1]     _INIT("");         // both domains and IPs should work (no SSL)
  WLED_GLOBAL char mqttUser[41] _INIT("");                   // optional: username for MQTT auth
  WLED_GLOBAL char mqttPass[65] _INIT("");                   // optional: password for MQTT auth
  WLED_GLOBAL char mqttClientID[41] _INIT("");               // override the client ID
  WLED_GLOBAL uint16_t mqttPort _INIT(1883);
  WLED_GLOBAL bool retainMqttMsg _INIT(false);               // retain brightness and color
  #define WLED_MQTT_CONNECTED (mqtt != nullptr && mqtt->connected())
#else
  #define WLED_MQTT_CONNECTED false
#endif

WLED_GLOBAL uint16_t serialBaud _INIT(1152); // serial baud rate, multiply by 100
WLED_GLOBAL bool     serialCanRX _INIT(false);
WLED_GLOBAL bool     serialCanTX _INIT(false);

#ifndef WLED_DISABLE_ESPNOW
WLED_GLOBAL bool enableESPNow        _INIT(false);                  // global on/off for ESP-NOW
WLED_GLOBAL byte statusESPNow        _INIT(ESP_NOW_STATE_UNINIT);   // state of ESP-NOW stack (0 uninitialised, 1 initialised, 2 error)
WLED_GLOBAL bool useESPNowSync       _INIT(false);                  // use ESP-NOW wireless technology for sync
WLED_GLOBAL std::vector<std::array<uint8_t, 6>> masterRemotes;      // MAC of ESP-NOW sync masters or remotes (Wiz Mote)
WLED_GLOBAL byte senderESPNow[6]     _INIT_N(({0,0,0,0,0,0}));      // last seen ESP-NOW sender
WLED_GLOBAL byte channelESPNow       _INIT(1);                      // last channel used when searching for master
WLED_GLOBAL unsigned long scanESPNow _INIT(0UL);                    // timestamp (in ms) of next ESP-NOW channel scan
WLED_GLOBAL unsigned long heartbeatESPNow _INIT(0UL);               // last heartbeat/beacon millis()
WLED_GLOBAL int16_t wizMoteButton    _INIT(-1);
#endif

// Time CONFIG
#ifndef WLED_NTP_ENABLED
  #define WLED_NTP_ENABLED false
#endif
#ifndef WLED_TIMEZONE
  #define WLED_TIMEZONE 0
#endif
#ifndef WLED_UTC_OFFSET
  #define WLED_UTC_OFFSET 0
#endif
WLED_GLOBAL bool ntpEnabled      _INIT(WLED_NTP_ENABLED); // get internet time. Only required if you use clock overlays or time-activated macros
WLED_GLOBAL bool useAMPM         _INIT(false);            // 12h/24h clock format
WLED_GLOBAL byte currentTimezone _INIT(WLED_TIMEZONE);    // Timezone ID. Refer to timezones array in wled10_ntp.ino
WLED_GLOBAL int utcOffsetSecs    _INIT(WLED_UTC_OFFSET);  // Seconds to offset from UTC before timzone calculation

WLED_GLOBAL byte overlayCurrent _INIT(0);    // 0: no overlay 1: analog clock 2: was single-digit clock 3: was cronixie
WLED_GLOBAL uint16_t overlayMin _INIT(0), overlayMax _INIT(DEFAULT_LED_COUNT - 1);   // boundaries of overlay mode

WLED_GLOBAL byte analogClock12pixel _INIT(0);               // The pixel in your strip where "midnight" would be
WLED_GLOBAL bool analogClockSecondsTrail _INIT(false);      // Display seconds as trail of LEDs instead of a single pixel
WLED_GLOBAL bool analogClock5MinuteMarks _INIT(false);      // Light pixels at every 5-minute position
WLED_GLOBAL bool analogClockSolidBlack _INIT(false);        // Show clock overlay only if all LEDs are solid black (effect is 0 and color is black)

WLED_GLOBAL bool countdownMode _INIT(false);                         // Clock will count down towards date
WLED_GLOBAL byte countdownYear _INIT(20), countdownMonth _INIT(1);   // Countdown target date, year is last two digits
WLED_GLOBAL byte countdownDay  _INIT(1) , countdownHour  _INIT(0);
WLED_GLOBAL byte countdownMin  _INIT(0) , countdownSec   _INIT(0);

WLED_GLOBAL byte macroNl   _INIT(0);        // after nightlight delay over
WLED_GLOBAL byte macroCountdown _INIT(0);
WLED_GLOBAL byte macroAlexaOn _INIT(0), macroAlexaOff _INIT(0);

// Security CONFIG
WLED_GLOBAL bool otaLock        _INIT(false);     // prevents OTA firmware updates without password. ALWAYS enable if system exposed to any public networks
WLED_GLOBAL bool wifiLock       _INIT(false);     // prevents access to WiFi settings when OTA lock is enabled
#ifdef WLED_ENABLE_AOTA
WLED_GLOBAL bool aOtaEnabled    _INIT(true);      // ArduinoOTA allows easy updates directly from the IDE. Careful, it does not auto-disable when OTA lock is on
#else
WLED_GLOBAL bool aOtaEnabled    _INIT(false);     // ArduinoOTA allows easy updates directly from the IDE. Careful, it does not auto-disable when OTA lock is on
#endif
WLED_GLOBAL bool otaSameSubnet  _INIT(true);      // prevent OTA updates from other subnets (e.g. internet) if no PIN is set
WLED_GLOBAL char settingsPIN[5] _INIT(WLED_PIN);  // PIN for settings pages
WLED_GLOBAL bool correctPIN     _INIT(!strlen(settingsPIN));
WLED_GLOBAL unsigned long lastEditTime _INIT(0);

WLED_GLOBAL uint16_t userVar0 _INIT(0), userVar1 _INIT(0); //available for use in usermod

// internal global variable declarations
// wifi
WLED_GLOBAL bool staActive _INIT(true);
WLED_GLOBAL bool apActive _INIT(false);
WLED_GLOBAL byte apClients _INIT(0);
WLED_GLOBAL bool forceReconnect _INIT(false);
WLED_GLOBAL unsigned long lastReconnectAttempt _INIT(0);
WLED_GLOBAL bool interfacesInited _INIT(false);
WLED_GLOBAL bool wasConnected _INIT(false);

// color
WLED_GLOBAL byte lastRandomIndex _INIT(0);        // used to save last random color so the new one is not the same
WLED_GLOBAL std::vector<CRGBPalette16> customPalettes;  // custom palettes
WLED_GLOBAL uint8_t paletteBlend _INIT(0);        // determines bending and wrapping of palette: 0: blend, wrap if moving (SEGMENT.speed>0); 1: blend, always wrap; 2: blend, never wrap; 3: don't blend or wrap

// transitions
WLED_GLOBAL uint8_t       transitionStyle          _INIT(0);      // effect/on-off transitionig style
WLED_GLOBAL bool          transitionActive         _INIT(false);  // flag to indicate on/off transition is active
WLED_GLOBAL uint16_t      transitionDelay          _INIT(750);    // global transition duration
WLED_GLOBAL uint16_t      transitionDelayDefault   _INIT(750);    // default transition time (stored in cfg.json)
WLED_GLOBAL unsigned long transitionStartTime      _INIT(0UL);
WLED_GLOBAL bool          jsonTransitionOnce       _INIT(false);  // flag to override transitionDelay (playlist, JSON API: "live" & "seg":{"i"} & "tt")
WLED_GLOBAL uint8_t       randomPaletteChangeTime  _INIT(5);      // amount of time [s] between random palette changes (min: 1s, max: 255s)
WLED_GLOBAL bool          useHarmonicRandomPalette _INIT(true);   // use *harmonic* random palette generation (nicer looking) or truly random

// nightlight
WLED_GLOBAL byte nightlightTargetBri   _INIT(0);                  // brightness after nightlight is over
WLED_GLOBAL byte nightlightMode        _INIT(NL_MODE_FADE);       // See const.h for available modes. Was nightlightFade
WLED_GLOBAL bool nightlightActive      _INIT(false);
WLED_GLOBAL bool nightlightActiveOld   _INIT(false);
WLED_GLOBAL byte nightlightDelayMins   _INIT(60);                 // nighlight duration in minutes
WLED_GLOBAL byte nightlightDelayMinsDefault _INIT(nightlightDelayMins);
WLED_GLOBAL unsigned long nightlightStartTime;
WLED_GLOBAL byte briNlT                _INIT(0);                  // brightness at the start of nightlight
WLED_GLOBAL byte colNlT[]              _INIT_N(({ 0, 0, 0, 0 })); // colors at the start of nightlight

// brightness
WLED_GLOBAL unsigned long lastOnTime _INIT(0);
WLED_GLOBAL bool offMode             _INIT(false);         // correctly initialised in WLED::beginStrip()
WLED_GLOBAL byte briS                _INIT(128);           // default brightness
WLED_GLOBAL byte bri                 _INIT(128);           // global brightness (set)
WLED_GLOBAL byte briOld              _INIT(0);             // global brightness while in transition loop (previous iteration)
WLED_GLOBAL byte briT                _INIT(0);             // global brightness during transition (if > 0 WLED is still on)
WLED_GLOBAL byte briLast             _INIT(128);           // brightness before turned off. Used for toggle function
WLED_GLOBAL byte whiteLast           _INIT(128);           // white channel before turned off. Used for toggle function in ir.cpp

// button
struct Button {
  unsigned long pressedTime;        // time button was pressed
  unsigned long waitTime;           // time to wait for next button press
  int8_t        pin;                // pin number
  struct {
    uint8_t     type          : 6;  // button type (push, long, double, etc.)
    bool        pressedBefore : 1;  // button was pressed before
    bool        longPressed   : 1;  // button was long pressed
  };
  uint8_t       macroButton;        // macro/preset to call on button press
  uint8_t       macroLongPress;     // macro/preset to call on long press
  uint8_t       macroDoublePress;   // macro/preset to call on double press

  Button(int8_t p, uint8_t t, uint8_t mB = 0, uint8_t mLP = 0, uint8_t mDP = 0)
  : pressedTime(0)
  , waitTime(0)
  , pin(p)
  , type(t)
  , pressedBefore(false)
  , longPressed(false)
  , macroButton(mB)
  , macroLongPress(mLP)
  , macroDoublePress(mDP) {}
};
WLED_GLOBAL std::vector<Button> buttons; // vector of button structs
WLED_GLOBAL bool buttonPublishMqtt                            _INIT(false);
WLED_GLOBAL bool disablePullUp                                _INIT(false);
WLED_GLOBAL byte touchThreshold                               _INIT(TOUCH_THRESHOLD);

// notifications
WLED_GLOBAL bool sendNotifications    _INIT(false);           // master notification switch
WLED_GLOBAL bool sendNotificationsRT  _INIT(false);           // master notification switch (runtime)
WLED_GLOBAL unsigned long notificationSentTime _INIT(0);
WLED_GLOBAL byte notificationSentCallMode _INIT(CALL_MODE_INIT);
WLED_GLOBAL uint8_t notificationCount _INIT(0);
WLED_GLOBAL uint8_t syncGroups    _INIT(0x01);                // sync send groups this instance syncs to (bit mapped)
WLED_GLOBAL uint8_t receiveGroups _INIT(0x01);                // sync receive groups this instance belongs to (bit mapped)
WLED_GLOBAL bool receiveApplyToAllSelected     _INIT(false);      // when sender has less segments than receivers and receiveSegmentOptions is unselected, propagate to all selected segments instead of main segment only
WLED_GLOBAL bool receiveNotificationBrightness _INIT(true);       // apply brightness from incoming notifications
WLED_GLOBAL bool receiveNotificationColor      _INIT(true);       // apply color
WLED_GLOBAL bool receiveNotificationEffects    _INIT(true);       // apply effects setup
WLED_GLOBAL bool receiveNotificationPalette    _INIT(true);       // apply palette
WLED_GLOBAL bool receiveSegmentOptions         _INIT(true);       // apply segment options
WLED_GLOBAL bool receiveSegmentBounds          _INIT(false);      // apply segment bounds (start, stop, offset)
WLED_GLOBAL bool receiveDirect                 _INIT(true);       // receive UDP/Hyperion realtime
WLED_GLOBAL bool notifyDirect _INIT(true);                        // send notification if change via UI or HTTP API
WLED_GLOBAL bool notifyButton _INIT(true);                        // send if updated by button or infrared remote
WLED_GLOBAL bool notifyAlexa  _INIT(false);                       // send notification if updated via Alexa
WLED_GLOBAL bool notifyHue    _INIT(false);                       // send notification if Hue light changes
WLED_GLOBAL bool notifyMQTT   _INIT(false);                       // send notification on MQTT change

// effects
WLED_GLOBAL byte effectCurrent _INIT(0);
WLED_GLOBAL byte effectSpeed _INIT(128);
WLED_GLOBAL byte effectIntensity _INIT(128);
WLED_GLOBAL byte effectPalette _INIT(0);
WLED_GLOBAL bool stateChanged _INIT(false);

// network
WLED_GLOBAL uint16_t udpPort    _INIT(21324); // WLED notifier default port
WLED_GLOBAL uint16_t udpPort2   _INIT(65506); // WLED notifier supplemental port
WLED_GLOBAL uint16_t udpRgbPort _INIT(19446); // Hyperion port
WLED_GLOBAL uint8_t  udpNumRetries _INIT(0);  // Number of times a UDP sync message is retransmitted. Increase to increase reliability
WLED_GLOBAL bool     udpConnected _INIT(false);
WLED_GLOBAL bool     udp2Connected _INIT(false);
WLED_GLOBAL bool     udpRgbConnected _INIT(false);
WLED_GLOBAL char escapedMac[13] _INIT("");
WLED_GLOBAL DNSServer dnsServer;

// ui style
WLED_GLOBAL bool showWelcomePage _INIT(false);

// hue
#ifndef WLED_DISABLE_HUESYNC
WLED_GLOBAL bool huePollingEnabled _INIT(false);           // poll hue bridge for light state
WLED_GLOBAL uint16_t huePollIntervalMs _INIT(2500);        // low values (< 1sec) may cause lag but offer quicker response
WLED_GLOBAL char hueApiKey[47] _INIT("api");               // key token will be obtained from bridge
WLED_GLOBAL byte huePollLightId _INIT(1);                  // ID of hue lamp to sync to. Find the ID in the hue app ("about" section)
WLED_GLOBAL IPAddress hueIP _INIT_N(((0, 0, 0, 0))); // IP address of the bridge
WLED_GLOBAL bool hueApplyOnOff _INIT(true);
WLED_GLOBAL bool hueApplyBri _INIT(true);
WLED_GLOBAL bool hueApplyColor _INIT(true);
WLED_GLOBAL byte hueError _INIT(HUE_ERROR_INACTIVE);
// WLED_GLOBAL uint16_t hueFailCount _INIT(0);
WLED_GLOBAL float hueXLast _INIT(0), hueYLast _INIT(0);
WLED_GLOBAL uint16_t hueHueLast _INIT(0), hueCtLast _INIT(0);
WLED_GLOBAL byte hueSatLast _INIT(0), hueBriLast _INIT(0);
WLED_GLOBAL unsigned long hueLastRequestSent _INIT(0);
WLED_GLOBAL bool hueAuthRequired _INIT(false);
WLED_GLOBAL bool hueReceived _INIT(false);
WLED_GLOBAL bool hueStoreAllowed _INIT(false), hueNewKey _INIT(false);
#endif

// countdown
WLED_GLOBAL unsigned long countdownTime _INIT(1514764800L);
WLED_GLOBAL bool countdownOverTriggered _INIT(true);

WLED_GLOBAL byte lastTimerMinute  _INIT(0);
WLED_GLOBAL std::vector<Timer> timers;
WLED_GLOBAL bool doAdvancePlaylist _INIT(false);

//improv
WLED_GLOBAL byte improvActive _INIT(0); //0: no improv packet received, 1: improv active, 2: provisioning
WLED_GLOBAL byte improvError _INIT(0);

//playlists
WLED_GLOBAL int16_t currentPlaylist _INIT(-1);
//still used for "PL=~" HTTP API command
WLED_GLOBAL byte presetCycCurr _INIT(0);
WLED_GLOBAL byte presetCycMin _INIT(1);
WLED_GLOBAL byte presetCycMax _INIT(5);

// realtime
WLED_GLOBAL byte realtimeMode _INIT(REALTIME_MODE_INACTIVE);
WLED_GLOBAL byte realtimeOverride _INIT(REALTIME_OVERRIDE_NONE);        // used only if not using main segment only (no need to override in such case)
WLED_GLOBAL IPAddress realtimeIP _INIT_N(((0, 0, 0, 0)));
WLED_GLOBAL unsigned long realtimeTimeout _INIT(0);
WLED_GLOBAL uint8_t tpmPacketCount _INIT(0);
WLED_GLOBAL uint16_t tpmPayloadFrameSize _INIT(0);
WLED_GLOBAL bool useMainSegmentOnly _INIT(false);
WLED_GLOBAL bool realtimeRespectLedMaps _INIT(true);                    // Respect LED maps when receiving realtime data

WLED_GLOBAL unsigned long lastInterfaceUpdate _INIT(0);
WLED_GLOBAL byte interfaceUpdateCallMode _INIT(CALL_MODE_INIT);

// alexa udp
#ifndef WLED_DISABLE_ALEXA
  WLED_GLOBAL Espalexa espalexa;
  WLED_GLOBAL EspalexaDevice* espalexaDevice;
#endif

// network time
#ifndef WLED_LAT
  #define WLED_LAT 0.0f
#endif
#ifndef WLED_LON
  #define WLED_LON 0.0f
#endif
#define NTP_NEVER 999000000L
WLED_GLOBAL bool ntpConnected _INIT(false);
WLED_GLOBAL time_t localTime _INIT(0);
WLED_GLOBAL unsigned long ntpLastSyncTime _INIT(NTP_NEVER);
WLED_GLOBAL unsigned long ntpPacketSentTime _INIT(NTP_NEVER);
WLED_GLOBAL IPAddress ntpServerIP;
WLED_GLOBAL uint16_t ntpLocalPort _INIT(2390);
WLED_GLOBAL uint16_t rolloverMillis _INIT(0);
WLED_GLOBAL float longitude _INIT(WLED_LON);
WLED_GLOBAL float latitude _INIT(WLED_LAT);
WLED_GLOBAL time_t sunrise _INIT(0);
WLED_GLOBAL time_t sunset _INIT(0);
WLED_GLOBAL Toki toki _INIT(Toki());

// General filesystem
WLED_GLOBAL size_t fsBytesUsed _INIT(0);
WLED_GLOBAL size_t fsBytesTotal _INIT(0);
WLED_GLOBAL unsigned long presetsModifiedTime _INIT(0L);

// presets
WLED_GLOBAL byte currentPreset _INIT(0);

WLED_GLOBAL byte errorFlag _INIT(0);

WLED_GLOBAL volatile bool doSerializeConfig _INIT(false);        // flag to initiate saving of config
WLED_GLOBAL volatile bool doReboot          _INIT(false);        // flag to initiate reboot from async handlers

// status led
#if defined(STATUSLED)
WLED_GLOBAL unsigned long ledStatusLastMillis _INIT(0);
WLED_GLOBAL uint8_t ledStatusType _INIT(0); // current status type - corresponds to number of blinks per second
WLED_GLOBAL bool ledStatusState _INIT(false); // the current LED state
#endif

// server library objects
WLED_GLOBAL AsyncWebServer server _INIT({{80}});
#ifdef WLED_ENABLE_WEBSOCKETS
WLED_GLOBAL AsyncWebSocket ws _INIT({{"/ws"}});
#endif
#ifndef WLED_DISABLE_HUESYNC
WLED_GLOBAL AsyncClient     *hueClient _INIT(NULL);
#endif

// udp interface objects
WLED_GLOBAL WiFiUDP notifierUdp, rgbUdp, notifier2Udp;
WLED_GLOBAL WiFiUDP ntpUdp;
#ifndef WLED_USE_DDP_ONLY
WLED_GLOBAL ESPAsyncE131 e131 _INIT({{handleE131Packet}});
#endif
WLED_GLOBAL ESPAsyncE131 ddp  _INIT({{handleE131Packet}});
WLED_GLOBAL bool e131NewData _INIT(false);

// led fx library object
WLED_GLOBAL WS2812FX   strip         _INIT(WS2812FX());
WLED_GLOBAL std::vector<BusConfig> busConfigs;    //temporary, to remember values from network callback until after
WLED_GLOBAL byte       doInit        _INIT(0);    // bitfield: 1 - bus, 2 - 2D, 3-8 - reserved
WLED_GLOBAL int8_t     loadLedmap    _INIT(-1);
WLED_GLOBAL uint8_t    currentLedmap _INIT(0);
#ifndef ESP8266
WLED_GLOBAL char  *ledmapNames[WLED_MAX_LEDMAPS-1] _INIT_N(({nullptr}));
#endif
#if WLED_MAX_LEDMAPS>16
WLED_GLOBAL uint32_t ledMaps _INIT(0); // bitfield representation of available ledmaps
#else
WLED_GLOBAL uint16_t ledMaps _INIT(0); // bitfield representation of available ledmaps
#endif

// global I2C SDA pin (used for usermods)
#ifndef I2CSDAPIN
WLED_GLOBAL int8_t i2c_sda  _INIT(-1);
#else
WLED_GLOBAL int8_t i2c_sda  _INIT(I2CSDAPIN);
#endif
// global I2C SCL pin (used for usermods)
#ifndef I2CSCLPIN
WLED_GLOBAL int8_t i2c_scl  _INIT(-1);
#else
WLED_GLOBAL int8_t i2c_scl  _INIT(I2CSCLPIN);
#endif

// global SPI DATA/MOSI/POCI pin (used for usermods)
#ifndef SPIMOSIPIN
WLED_GLOBAL int8_t spi_mosi  _INIT(-1);
#else
WLED_GLOBAL int8_t spi_mosi  _INIT(SPIMOSIPIN);
#endif
// global SPI DATA/MISO/PICO pin (used for usermods)
#ifndef SPIMISOPIN
WLED_GLOBAL int8_t spi_miso  _INIT(-1);
#else
WLED_GLOBAL int8_t spi_miso  _INIT(SPIMISOPIN);
#endif
// global SPI CLOCK/SCLK pin (used for usermods)
#ifndef SPISCLKPIN
WLED_GLOBAL int8_t spi_sclk  _INIT(-1);
#else
WLED_GLOBAL int8_t spi_sclk  _INIT(SPISCLKPIN);
#endif
// global SPI SOURCE SELECT/SS pin (used for SPI SD card)
#ifndef SPISSELPIN
WLED_GLOBAL int8_t spi_ssel  _INIT(-1);
#else
WLED_GLOBAL int8_t spi_ssel  _INIT(SPISSELPIN);
#endif

#ifndef ESP8266
  #if defined(SPIMOSIPIN) && defined(SPIMISOPIN) && defined(SPISCLKPIN) && defined(SPISSELPIN) && defined(WLED_USE_SD_SPI)
WLED_GLOBAL bool sdCard _INIT(true);
  #else
WLED_GLOBAL bool sdCard _INIT(false);
  #endif
#endif

// global ArduinoJson buffer
// we will allocate buffer in PSRAM if possible from setup() otherwise we will use heap
#if defined(CONFIG_IDF_TARGET_ESP32) && defined(BOARD_HAS_PSRAM)
  #warning "If compiling for ESP32 (rev.1), make sure to use '-mfix-esp32-psram-cache-issue' compiler flag to avoid PSRAM cache issues!"
#endif
#ifndef BOARD_HAS_PSRAM
WLED_GLOBAL StaticJsonDocument<JSON_BUFFER_SIZE> gDoc;
WLED_GLOBAL JsonDocument *pDoc _INIT(&gDoc);
#else
WLED_GLOBAL JsonDocument *pDoc _INIT(nullptr);
#endif
#if defined(ARDUINO_ARCH_ESP32)
WLED_GLOBAL SemaphoreHandle_t jsonBufferLockMutex _INIT(xSemaphoreCreateRecursiveMutex());
#endif
WLED_GLOBAL volatile uint8_t jsonBufferLock _INIT(0);

// enable additional debug output
#if defined(WLED_DEBUG_HOST)
  #include "net_debug.h"
  // On the host side, use netcat to receive the log statements: nc -l 7868 -u
  // use -D WLED_DEBUG_HOST='"192.168.xxx.xxx"' or FQDN within quotes
  #define DEBUGOUT NetDebug
  WLED_GLOBAL bool netDebugEnabled _INIT(true);
  WLED_GLOBAL char netDebugPrintHost[33] _INIT(WLED_DEBUG_HOST);
  #ifndef WLED_DEBUG_PORT
    #define WLED_DEBUG_PORT 7868
  #endif
  WLED_GLOBAL int netDebugPrintPort _INIT(WLED_DEBUG_PORT);
#else
  #define DEBUGOUT Serial
#endif

#ifdef WLED_DEBUG
  #ifndef ESP8266
  #include <rom/rtc.h>
  #endif
  #define DEBUG_PRINT(x) DEBUGOUT.print(x)
  #define DEBUG_PRINTLN(x) DEBUGOUT.println(x)
  #define DEBUG_PRINTF(x...) DEBUGOUT.printf(x)
  #define DEBUG_PRINTF_P(x...) DEBUGOUT.printf_P(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(x...)
  #define DEBUG_PRINTF_P(x...)
#endif

// debug macro variable definitions
#ifdef WLED_DEBUG
  WLED_GLOBAL unsigned long debugTime _INIT(0UL);
  WLED_GLOBAL int lastWifiState _INIT(WL_IDLE_STATUS);
  WLED_GLOBAL unsigned long wifiStateChangedTime _INIT(0UL);
  WLED_GLOBAL unsigned loops _INIT(0);
#endif

#define WLED_CONNECTED Network.isConnected()

#ifndef WLED_AP_SSID_UNIQUE
  #define WLED_SET_AP_SSID() do { \
    strncpy_P(apSSID, PSTR(WLED_AP_SSID), sizeof(apSSID)); \
  } while(0)
#else
  #define WLED_SET_AP_SSID() do { \
    snprintf_P( \
      apSSID, \
      sizeof(apSSID), \
      PSTR("%s_%s"), \
      WLED_AP_SSID, \
      escapedMac+6 \
    ); \
  } while(0)
#endif

//macro to convert F to const
#define SET_F(x)  (const char*)F(x)

class WLED {
public:
  WLED();
  static WLED& instance()
  {
    static WLED instance;
    return instance;
  }

  // boot starts here
  void setup();

  void loop();
  void reset();

  void beginStrip();
  void handleConnection();
  void stopAP(bool stopESPNow = true);
  void initAP(bool resetAP = false);
  void initConnection(bool force = false);
  void connected();
  #if defined(STATUSLED)
  void handleStatusLED();
  #endif
  #if WLED_WATCHDOG_TIMEOUT > 0
  void enableWatchdog();
  void disableWatchdog();
  #endif
};
#endif        // WLED_H
