#pragma once
#ifndef WLED_FCN_DECLARE_H
#define WLED_FCN_DECLARE_H

/*
 * All globally accessible functions are declared here
 */

//alexa.cpp
#ifndef WLED_DISABLE_ALEXA
void onAlexaChange(EspalexaDevice* dev);
void alexaInit();
void handleAlexa();
void onAlexaChange(EspalexaDevice* dev);
#endif

//button.cpp
void shortPressAction(uint8_t b=0);
void longPressAction(uint8_t b=0);
void doublePressAction(uint8_t b=0);
bool isButtonPressed(uint8_t b=0);
void handleButton();
void handleIO();
void touchButtonISR();

//cfg.cpp
bool deserializeConfig(JsonObject doc, bool fromFS = false);
bool deserializeConfigFromFS();
bool deserializeConfigSec();
void serializeConfig();
void serializeConfigSec();

//dmx.cpp
void initDMX();
void handleDMX();

//e131.cpp
void handleE131Packet(e131_packet_t* p, IPAddress clientIP, byte protocol);

//file.cpp
bool handleFileRead(AsyncWebServerRequest*, String path);
bool writeObjectToFileUsingId(const char* file, uint16_t id, const JsonDocument* content);
bool writeObjectToFile(const char* file, const char* key, const JsonDocument* content);
bool readObjectFromFileUsingId(const char* file, uint16_t id, JsonDocument* dest, const JsonDocument* filter = nullptr);
bool readObjectFromFile(const char* file, const char* key, JsonDocument* dest, const JsonDocument* filter = nullptr);
void updateFSInfo();
void closeFile();
inline bool writeObjectToFileUsingId(const String &file, uint16_t id, const JsonDocument* content) { return writeObjectToFileUsingId(file.c_str(), id, content); };
inline bool writeObjectToFile(const String &file, const char* key, const JsonDocument* content) { return writeObjectToFile(file.c_str(), key, content); };
inline bool readObjectFromFileUsingId(const String &file, uint16_t id, JsonDocument* dest, const JsonDocument* filter = nullptr) { return readObjectFromFileUsingId(file.c_str(), id, dest); };
inline bool readObjectFromFile(const String &file, const char* key, JsonDocument* dest, const JsonDocument* filter = nullptr) { return readObjectFromFile(file.c_str(), key, dest); };
bool initFS();

//hue.cpp
void handleHue();
void reconnectHue();
void onHueError(void* arg, AsyncClient* client, int8_t error);
void onHueConnect(void* arg, AsyncClient* client);
void sendHuePoll();
void onHueData(void* arg, AsyncClient* client, void *data, size_t len);

//improv.cpp
enum ImprovRPCType {
  Command_Wifi = 0x01,
  Request_State = 0x02,
  Request_Info = 0x03,
  Request_Scan = 0x04
};

void handleImprovPacket();
void sendImprovRPCResult(ImprovRPCType type, uint8_t n_strings = 0, const char **strings = nullptr);
void sendImprovStateResponse(uint8_t state, bool error = false);
void sendImprovInfoResponse();
void startImprovWifiScan();
void handleImprovWifiScan();
void sendImprovIPRPCResult(ImprovRPCType type);

//ir.cpp
void initIR();
void deInitIR();
void handleIR();

//json.cpp
bool deserializeState(JsonObject root, byte callMode = CALL_MODE_DIRECT_CHANGE, byte presetId = 0);
void serializeState(JsonObject root, bool forPreset = false, bool includeBri = true, bool segmentBounds = true, bool selectedSegmentsOnly = false);
void serializeInfo(JsonObject root);
void serializeModeNames(JsonArray root);
void serializeModeData(JsonArray root);
void serveJson(AsyncWebServerRequest* request);
#ifdef WLED_ENABLE_JSONLIVE
bool serveLiveLeds(AsyncWebServerRequest* request, uint32_t wsClient = 0);
#endif
template<typename DestType>
bool getJsonValue(const JsonVariant& element, DestType& destination) {
  if (element.isNull()) return false;
  destination = element.as<DestType>();
  return true;
}
template<typename DestType, typename DefaultType>
bool getJsonValue(const JsonVariant& element, DestType& destination, const DefaultType defaultValue) {
  destination = defaultValue;
  return getJsonValue(element, destination);
}

//led.cpp
void setValuesFromSegment(uint8_t s);
#define setValuesFromMainSeg()          setValuesFromSegment(strip.getMainSegmentId())
#define setValuesFromFirstSelectedSeg() setValuesFromSegment(strip.getFirstSelectedSegId())
void toggleOnOff();
void applyBri();
void applyFinalBri();
void applyValuesToSelectedSegs();
void colorUpdated(byte callMode);
void stateUpdated(byte callMode);
void updateInterfaces(uint8_t callMode);
void handleTransitions();
void handleNightlight();

#ifdef WLED_ENABLE_LOXONE
//lx_parser.cpp
bool parseLx(int lxValue, byte* rgbw);
void parseLxJson(int lxValue, byte segId, bool secondary);
#endif

//mqtt.cpp
bool initMqtt();
void publishMqtt();

//ntp.cpp
String getTZNamesJSONString();
void handleTime();
void handleNetworkTime();
void sendNTPPacket();
bool checkNTPResponse();
void updateLocalTime();
void getTimeString(char* out);
bool checkCountdown();
void setCountdown();
byte weekdayMondayFirst();
void checkTimers();
void calculateSunriseAndSunset();
void setTimeFromAPI(uint32_t timein);

const uint8_t TH_SUNRISE = 255;
const uint8_t TH_SUNSET = 254;

struct Timer {
  uint8_t preset;
  uint8_t hour;
  int8_t minute;
  uint8_t weekdays;
  uint8_t monthStart;
  uint8_t monthEnd;
  uint8_t dayStart;
  uint8_t dayEnd;
  inline bool isEnabled() const { return (weekdays & 0x01) && (preset != 0); }
  inline bool isSunrise() const { return hour == TH_SUNRISE; }
  inline bool isSunset() const { return hour == TH_SUNSET; }
  inline bool isRegular() const { return hour < TH_SUNSET; }
  Timer() : preset(0), hour(0), minute(0), weekdays(255), monthStart(1), monthEnd(12), dayStart(1), dayEnd(31) {}
  Timer(uint8_t p, uint8_t h, int8_t m, uint8_t wd, uint8_t ms = 1, uint8_t me = 12, uint8_t ds = 1, uint8_t de = 31)
    : preset(p), hour(h), minute(m), weekdays(wd), monthStart(ms), monthEnd(me), dayStart(ds), dayEnd(de) {}
};

void addTimer(uint8_t preset, uint8_t hour, int8_t minute, uint8_t weekdays,
              uint8_t monthStart = 1, uint8_t monthEnd = 12, uint8_t dayStart = 1, uint8_t dayEnd = 31);
void removeTimer(size_t index);
void clearTimers();
size_t getTimerCount();
void compactTimers();

//overlay.cpp
void handleOverlayDraw();
void _overlayAnalogCountdown();
void _overlayAnalogClock();

//playlist.cpp
void shufflePlaylist();
void unloadPlaylist();
int16_t loadPlaylist(JsonObject playlistObject, byte presetId = 0);
void handlePlaylist();
void serializePlaylist(JsonObject obj);

//presets.cpp
const char *getPresetsFileName(bool persistent = true);
bool presetNeedsSaving();
void initPresetsFile();
void handlePresets();
bool applyPreset(byte index, byte callMode = CALL_MODE_DIRECT_CHANGE);
bool applyPresetFromPlaylist(byte index);
void applyPresetWithFallback(uint8_t presetID, uint8_t callMode, uint8_t effectID = 0, uint8_t paletteID = 0);
inline bool applyTemporaryPreset() {return applyPreset(255);};
void savePreset(byte index, const char* pname = nullptr, JsonObject saveobj = JsonObject());
inline void saveTemporaryPreset() {savePreset(255);};
void deletePreset(byte index);
bool getPresetName(byte index, String& name);

//remote.cpp
void handleRemote();
// This is kind of an esoteric strucure because it's pulled from the "Wizmote"
// product spec. That remote is used as the baseline for behavior and availability
// since it's broadly commercially available and works out of the box as a drop-in
typedef struct WizMoteMessageStructure {
  uint8_t program;  // 0x91 for ON button, 0x81 for all others
  uint8_t seq[4];   // Incremetal sequence number 32 bit unsigned integer LSB first
  uint8_t dt1;      // Button Data Type (0x32)
  uint8_t button;   // Identifies which button is being pressed
  uint8_t dt2;      // Battery Level Data Type (0x01)
  uint8_t batLevel; // Battery Level 0-100

  uint8_t byte10;   // Unknown, maybe checksum
  uint8_t byte11;   // Unknown, maybe checksum
  uint8_t byte12;   // Unknown, maybe checksum
  uint8_t byte13;   // Unknown, maybe checksum
} message_structure_t;

//set.cpp
bool isAsterisksOnly(const char* str, byte maxLen);
void handleSettingsSet(AsyncWebServerRequest *request, byte subPage);
bool handleSet(AsyncWebServerRequest *request, const String& req, bool apply=true);

//udp.cpp
void notify(byte callMode, bool followUp=false);
uint8_t realtimeBroadcast(uint8_t type, IPAddress client, uint16_t length, const uint8_t *buffer, uint8_t bri=255, bool isRGBW=false);
void realtimeLock(uint32_t timeoutMs, byte md = REALTIME_MODE_GENERIC);
void exitRealtime();
void handleNotifications();
void setRealtimePixel(uint16_t i, byte r, byte g, byte b, byte w);
void refreshNodeList();
void sendSysInfoUDP();
void espNowSentCB(uint8_t* address, uint8_t status);
void espNowReceiveCB(uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast);

//network.cpp
bool initEthernet(); // result is informational
int  getSignalQuality(int rssi);
IPAddress resolveHostname(const String& hostname, bool useMDNS = true);
void fillMAC2Str(char *str, const uint8_t *mac);
void fillStr2MAC(uint8_t *mac, const char *str);
#ifndef WLED_DISABLE_ESPNOW
void initESPNow(bool resetAP = false);
void stopESPNow();
void sendESPNowHeartBeat();
#endif
int  findWiFi(bool doScan = false);
bool isWiFiConfigured();
void WiFiEvent(WiFiEvent_t event);

//usermods_list.cpp
void registerUsermods();

//util.cpp
#ifdef ESP8266
#define HW_RND_REGISTER RANDOM_REG32
#else // ESP32 family
#include "soc/wdev_reg.h"
#define HW_RND_REGISTER REG_READ(WDEV_RND_REG)
#endif
#define inoise8 perlin8   // fastled legacy alias
#define inoise16 perlin16 // fastled legacy alias
#define hex2int(a) (((a)>='0' && (a)<='9') ? (a)-'0' : ((a)>='A' && (a)<='F') ? (a)-'A'+10 : ((a)>='a' && (a)<='f') ? (a)-'a'+10 : 0)
[[gnu::pure]] int getNumVal(const String& req, uint16_t pos);
void parseNumber(const char* str, byte& val, byte minv=0, byte maxv=255);
bool getVal(JsonVariant elem, byte& val, byte minv=0, byte maxv=255); // getVal supports inc/decrementing and random ("X~Y(r|~[w][-][Z])" form)
[[gnu::pure]] bool getBoolVal(const JsonVariant &elem, bool dflt);
bool updateVal(const char* req, const char* key, byte& val, byte minv=0, byte maxv=255);
size_t printSetFormCheckbox(Print& settingsScript, const char* key, int val);
size_t printSetFormValue(Print& settingsScript, const char* key, int val);
size_t printSetFormValue(Print& settingsScript, const char* key, const char* val);
size_t printSetFormIndex(Print& settingsScript, const char* key, int index);
size_t printSetIdHTML(Print& settingsScript, const char* key, const char* val);
//void prepareHostname(char* hostname, size_t maxLen = 32);
[[gnu::pure]] bool isAsterisksOnly(const char* str, byte maxLen);
bool requestJSONBufferLock(uint8_t module=255);
void releaseJSONBufferLock();
uint8_t extractModeName(uint8_t mode, const char *src, char *dest, uint8_t maxLen);
uint8_t extractModeSlider(uint8_t mode, uint8_t slider, char *dest, uint8_t maxLen, uint8_t *var = nullptr);
int16_t extractModeDefaults(uint8_t mode, const char *segVar);
void checkSettingsPIN(const char *pin);
uint16_t crc16(const unsigned char* data_p, size_t length);
uint16_t beatsin88_t(accum88 beats_per_minute_88, uint16_t lowest = 0, uint16_t highest = 65535, uint32_t timebase = 0, uint16_t phase_offset = 0);
uint16_t beatsin16_t(accum88 beats_per_minute, uint16_t lowest = 0, uint16_t highest = 65535, uint32_t timebase = 0, uint16_t phase_offset = 0);
uint8_t beatsin8_t(accum88 beats_per_minute, uint8_t lowest = 0, uint8_t highest = 255, uint32_t timebase = 0, uint8_t phase_offset = 0);
void enumerateLedmaps();
[[gnu::hot]] uint8_t get_random_wheel_index(uint8_t pos);
[[gnu::hot, gnu::pure]] float mapf(float x, float in_min, float in_max, float out_min, float out_max);
[[gnu::hot, gnu::pure]] uint32_t hashInt(uint32_t s);
int32_t perlin1D_raw(uint32_t x, bool is16bit = false);
int32_t perlin2D_raw(uint32_t x, uint32_t y, bool is16bit = false);
int32_t perlin3D_raw(uint32_t x, uint32_t y, uint32_t z, bool is16bit = false);
uint16_t perlin16(uint32_t x);
uint16_t perlin16(uint32_t x, uint32_t y);
uint16_t perlin16(uint32_t x, uint32_t y, uint32_t z);
uint8_t perlin8(uint16_t x);
uint8_t perlin8(uint16_t x, uint16_t y);
uint8_t perlin8(uint16_t x, uint16_t y, uint16_t z);

// fast (true) random numbers using hardware RNG, all functions return values in the range lowerlimit to upperlimit-1
// note: for true random numbers with high entropy, do not call faster than every 200ns (5MHz)
// tests show it is still highly random reading it quickly in a loop (better than fastled PRNG)
// for 8bit and 16bit random functions: no limit check is done for best speed
// 32bit inputs are used for speed and code size, limits don't work if inverted or out of range
// inlining does save code size except for random(a,b) and 32bit random with limits
#define random hw_random // replace arduino random()
#define hw_random16 (uint16_t)hw_random
#define hw_random8 (uint8_t)hw_random
inline uint32_t hw_random() { return HW_RND_REGISTER; };
uint32_t hw_random(uint32_t upperlimit); // not inlined for code size
uint32_t hw_random(uint32_t lowerlimit, uint32_t upperlimit);
//template <typename T> T hw_random(T upperlimit) { return static_cast<T>(hw_random((uint32_t)upperlimit)); }
//template <typename T> T hw_random(T lowerlimit, T upperlimit) { return static_cast<T>(hw_random((uint32_t)lowerlimit, (uint32_t)upperlimit)); }

// memory allocation wrappers
extern "C" {
  #if defined(BOARD_HAS_PSRAM)
  void *p_malloc(size_t);           // prefer PSRAM over DRAM
  void *p_calloc(size_t, size_t);   // prefer PSRAM over DRAM
  void *p_realloc(void *, size_t);  // prefer PSRAM over DRAM
  inline void p_free(void *ptr) { heap_caps_free(ptr); }
  #else
  #define p_malloc d_malloc
  #define p_calloc d_calloc
  #define p_realloc d_realloc
  #define p_free d_free
  #endif
  void *d_malloc(size_t);           // prefer DRAM over PSRAM
  void *d_calloc(size_t, size_t);   // prefer DRAM over PSRAM
  void *d_realloc(void *, size_t);  // prefer DRAM over PSRAM
  #ifdef ESP8266
  inline void d_free(void *ptr) { free(ptr); }
  #else
  inline void d_free(void *ptr) { heap_caps_free(ptr); }
  #endif
}
#ifndef ESP8266
inline size_t getFreeHeapSize() { return heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT); } // returns free heap (ESP.getFreeHeap() can include other memory types)
inline size_t getContiguousFreeHeap() { return heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT); } // returns largest contiguous free block
#else
inline size_t getFreeHeapSize() { return ESP.getFreeHeap(); } // returns free heap
inline size_t getContiguousFreeHeap() { return ESP.getMaxFreeBlockSize(); } // returns largest contiguous free block
#endif
#define BFRALLOC_NOBYTEACCESS    (1 << 0) // ESP32 has 32bit accessible DRAM (usually ~50kB free) that must not be byte-accessed
#define BFRALLOC_PREFER_DRAM     (1 << 1) // prefer DRAM over PSRAM
#define BFRALLOC_ENFORCE_DRAM    (1 << 2) // use DRAM only, no PSRAM
#define BFRALLOC_PREFER_PSRAM    (1 << 3) // prefer PSRAM over DRAM
#define BFRALLOC_ENFORCE_PSRAM   (1 << 4) // use PSRAM if available, otherwise fall back to DRAM
#define BFRALLOC_CLEAR           (1 << 5) // clear allocated buffer after allocation
void *allocate_buffer(size_t size, uint32_t type);

// RAII guard class for the JSON Buffer lock
// Modeled after std::lock_guard
class JSONBufferGuard {
  bool holding_lock;
  public:
    inline JSONBufferGuard(uint8_t module=255) : holding_lock(requestJSONBufferLock(module)) {};
    inline ~JSONBufferGuard() { if (holding_lock) releaseJSONBufferLock(); };
    inline JSONBufferGuard(const JSONBufferGuard&) = delete; // Noncopyable
    inline JSONBufferGuard& operator=(const JSONBufferGuard&) = delete;
    inline JSONBufferGuard(JSONBufferGuard&& r) : holding_lock(r.holding_lock) { r.holding_lock = false; };  // but movable
    inline JSONBufferGuard& operator=(JSONBufferGuard&& r) { holding_lock |= r.holding_lock; r.holding_lock = false; return *this; };
    inline bool owns_lock() const { return holding_lock; }
    explicit inline operator bool() const { return owns_lock(); };
    inline void release() { if (holding_lock) releaseJSONBufferLock(); holding_lock = false; }
};

//wled_math.cpp
//float cos_t(float phi); // use float math
//float sin_t(float phi);
//float tan_t(float x);
int16_t sin16_t(uint16_t theta);
int16_t cos16_t(uint16_t theta);
uint8_t sin8_t(uint8_t theta);
uint8_t cos8_t(uint8_t theta);
float sin_approx(float theta); // uses integer math (converted to float), accuracy +/-0.0015 (compared to sinf())
float cos_approx(float theta);
float tan_approx(float x);
float atan2_t(float y, float x);
float acos_t(float x);
float asin_t(float x);
template <typename T> T atan_t(T x);
float floor_t(float x);
float fmod_t(float num, float denom);
uint32_t sqrt32_bw(uint32_t x);
#define sqrt16 sqrt32_bw
#define sin_t sin_approx
#define cos_t cos_approx
#define tan_t tan_approx

/*
#include <math.h>  // standard math functions. use a lot of flash
#define sin_t sinf
#define cos_t cosf
#define tan_t tanf
#define asin_t asinf
#define acos_t acosf
#define atan_t atanf
#define fmod_t fmodf
#define floor_t floorf
*/

//wled_serial.cpp
void handleSerial();
void updateBaudRate(uint32_t rate);

//wled_server.cpp
void createEditHandler(bool enable);
void initServer();
void serveMessage(AsyncWebServerRequest* request, uint16_t code, const String& headl, const String& subl="", byte optionT=255);
void serveJsonError(AsyncWebServerRequest* request, uint16_t code, uint16_t error);
void serveSettings(AsyncWebServerRequest* request, bool post = false);
void serveSettingsJS(AsyncWebServerRequest* request);

//ws.cpp
void handleWs();
void wsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
void sendDataWs(AsyncWebSocketClient * client = nullptr);

//xml.cpp
void XML_response(Print& dest);
void getSettingsJS(byte subPage, Print& dest);

#endif
