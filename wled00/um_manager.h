#pragma once

#ifndef WLED_USERMODMANAGER_H
#define WLED_USERMODMANAGER_H

// requires wled.h to be included first

#ifdef WLED_DEBUG_USERMODS
  #define DEBUGUM_PRINT(x) DEBUGOUT.print(x)
  #define DEBUGUM_PRINTLN(x) DEBUGOUT.println(x)
  #define DEBUGUM_PRINTF(x...) DEBUGOUT.printf(x)
  #define DEBUGUM_PRINTF_P(x...) DEBUGOUT.printf_P(x)
#else
  #define DEBUGUM_PRINT(x)
  #define DEBUGUM_PRINTLN(x)
  #define DEBUGUM_PRINTF(x...)
  #define DEBUGUM_PRINTF_P(x...)
#endif

/* Definitions for usermod base class, usermod manager namespace */

typedef enum UM_Data_Types {
  UMT_BYTE = 0,
  UMT_UINT16,
  UMT_INT16,
  UMT_UINT32,
  UMT_INT32,
  UMT_FLOAT,
  UMT_DOUBLE,
  UMT_BYTE_ARR,
  UMT_UINT16_ARR,
  UMT_INT16_ARR,
  UMT_UINT32_ARR,
  UMT_INT32_ARR,
  UMT_FLOAT_ARR,
  UMT_DOUBLE_ARR
} um_types_t;

typedef struct UM_Exchange_Data {
  // should just use: size_t arr_size, void **arr_ptr, byte *ptr_type
  size_t       u_size;                 // size of u_data array
  um_types_t  *u_type;                 // array of data types
  void       **u_data;                 // array of pointers to data
  UM_Exchange_Data() {
    u_size = 0;
    u_type = nullptr;
    u_data = nullptr;
  }
  ~UM_Exchange_Data() {
    if (u_type) delete[] u_type;
    if (u_data) delete[] u_data;
  }
} um_data_t;

const unsigned int um_data_size = sizeof(um_data_t);  // 12 bytes

class Usermod {
  protected:
    um_data_t *um_data; // um_data should be allocated using new in (derived) Usermod's setup() or constructor
  public:
    Usermod() { um_data = nullptr; }
    virtual ~Usermod() { if (um_data) delete um_data; }
    virtual void setup() = 0; // pure virtual, has to be overriden
    virtual void loop() = 0;  // pure virtual, has to be overriden
    virtual void handleOverlayDraw() {}                                      // called after all effects have been processed, just before strip.show()
    virtual bool handleButton(uint8_t b) { return false; }                   // button overrides are possible here
    virtual bool getUMData(um_data_t **data) { if (data) *data = nullptr; return false; }; // usermod data exchange [see examples for audio effects]
    virtual void connected() {}                                              // called when WiFi is (re)connected
    virtual void appendConfigData(Print& settingsScript);                    // helper function called from usermod settings page to add metadata for entry fields
    virtual void addToJsonState(JsonObject& obj) {}                          // add JSON objects for WLED state
    virtual void addToJsonInfo(JsonObject& obj) {}                           // add JSON objects for UI Info page
    virtual void readFromJsonState(JsonObject& obj) {}                       // process JSON messages received from web server
    virtual void addToConfig(JsonObject& obj) {}                             // add JSON entries that go to cfg.json
    virtual bool readFromConfig(JsonObject& obj) { return true; }            // Note as of 2021-06 readFromConfig() now needs to return a bool, see usermod_v2_example.h
    virtual void onMqttConnect(bool sessionPresent) {}                       // fired when MQTT connection is established (so usermod can subscribe)
    virtual bool onMqttMessage(char* topic, char* payload) { return false; } // fired upon MQTT message received (wled topic)
    virtual bool publishMqtt() { return false; }                             // fired upon MQTT publish
    virtual bool onEspNowMessage(uint8_t* sender, uint8_t* payload, uint8_t len) { return false; } // fired upon ESP-NOW message received
    virtual void onUpdateBegin(bool) {}                                      // fired prior to and after unsuccessful firmware update
    virtual void onStateChange(uint8_t mode) {}                              // fired upon WLED state change
    virtual uint16_t getId() {return USERMOD_ID_UNSPECIFIED;}

  // API shims
  private:
    static Print* oappend_shim;
    // old form of appendConfigData; called by default appendConfigData(Print&) with oappend_shim set up
    // private so it is not accidentally invoked except via Usermod::appendConfigData(Print&)
    virtual void appendConfigData() {};
  protected:
    // Shim for oappend(), which used to exist in utils.cpp
    template<typename T> static inline void oappend(const T& t) { oappend_shim->print(t); };
#ifdef ESP8266
    // Handle print(PSTR()) without crashing by detecting PROGMEM strings
    static void oappend(const char* c) { if ((intptr_t) c >= 0x40000000) oappend_shim->print(FPSTR(c)); else oappend_shim->print(c); };
#endif
};

namespace UsermodManager {
  void loop();
  void handleOverlayDraw();
  bool handleButton(uint8_t b);
  bool getUMData(um_data_t **um_data, uint8_t mod_id = USERMOD_ID_RESERVED); // USERMOD_ID_RESERVED will poll all usermods
  void setup();
  void connected();
  void appendConfigData(Print&);
  void addToJsonState(JsonObject& obj);
  void addToJsonInfo(JsonObject& obj);
  void readFromJsonState(JsonObject& obj);
  void addToConfig(JsonObject& obj);
  bool readFromConfig(JsonObject& obj);
#ifndef WLED_DISABLE_MQTT
  void onMqttConnect(bool sessionPresent);
  bool onMqttMessage(char* topic, char* payload);
  bool publishMqtt();
#endif
#ifndef WLED_DISABLE_ESPNOW
  bool onEspNowMessage(uint8_t* sender, uint8_t* payload, uint8_t len);
#endif
  void onUpdateBegin(bool);
  void onStateChange(uint8_t);
  bool add(Usermod* um);
  Usermod* lookup(uint16_t mod_id);
  byte getModCount();
};

#endif // WLED_USERMODMANAGER_H