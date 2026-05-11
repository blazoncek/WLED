#pragma once
#ifndef BusManager_h
#define BusManager_h

/*
 * Class for addressing various light types
 */

#include "const.h"
#include "pin_manager.h"
#include <vector>
#include <memory>

#if __cplusplus >= 201402L
using std::make_unique;
#else
// Really simple C++11 shim for non-array case; implementation from cppreference.com
template<class T, class... Args>
std::unique_ptr<T>
make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#endif

// enable additional debug output
#if defined(WLED_DEBUG_HOST)
  #include "net_debug.h"
  #define DEBUGOUT NetDebug
#else
  #define DEBUGOUT Serial
#endif

#ifdef WLED_DEBUG_BUS
  #ifndef ESP8266
  #include <rom/rtc.h>
  #endif
  #define DEBUGBUS_PRINT(x) DEBUGOUT.print(x)
  #define DEBUGBUS_PRINTLN(x) DEBUGOUT.println(x)
  #define DEBUGBUS_PRINTF(x...) DEBUGOUT.printf(x)
  #define DEBUGBUS_PRINTF_P(x...) DEBUGOUT.printf_P(x)
#else
  #define DEBUGBUS_PRINT(x)
  #define DEBUGBUS_PRINTLN(x)
  #define DEBUGBUS_PRINTF(x...)
  #define DEBUGBUS_PRINTF_P(x...)
#endif

//colors.cpp
uint16_t approximateKelvinFromRGB(uint32_t rgb);

#define GET_BIT(var,bit)    (((var)>>(bit))&0x01)
#define SET_BIT(var,bit)    ((var)|=(uint16_t)(0x0001<<(bit)))
#define UNSET_BIT(var,bit)  ((var)&=(~(uint16_t)(0x0001<<(bit))))

#define NUM_ICS_WS2812_1CH_3X(len) (((len)+2)/3)   // 1 WS2811 IC controls 3 zones (each zone has 1 LED, W)
#define IC_INDEX_WS2812_1CH_3X(i)  ((i)/3)

#define NUM_ICS_WS2812_2CH_3X(len) (((len)+1)*2/3) // 2 WS2811 ICs control 3 zones (each zone has 2 LEDs, CW and WW)
#define IC_INDEX_WS2812_2CH_3X(i)  ((i)*2/3)
#define WS2812_2CH_3X_SPANS_2_ICS(i) ((i)&0x01)    // every other LED zone is on two different ICs

//Light capability byte (unused) 0bRCCCTTTT
//bits 0/1/2/3: specifies a type of LED driver. A single "driver" may have different chip models but must have the same protocol/behavior
//bits 4/5/6: specifies the class of LED driver - 0b000 (dec. 0-15)  unconfigured/reserved
//                                              - 0b001 (dec. 16-31) digital (data pin only)
//                                              - 0b010 (dec. 32-47) analog (PWM)
//                                              - 0b011 (dec. 48-63) digital (data + clock / SPI)
//                                              - 0b100 (dec. 64-79) unused/reserved
//                                              - 0b101 (dec. 80-95) virtual network busses
//                                              - 0b110 (dec. 96-111) unused/reserved
//                                              - 0b111 (dec. 112-127) unused/reserved
//bit 7 is reserved and set to 0

#define TYPE_NONE                 0            //light is not configured
#define TYPE_RESERVED             1            //unused. Might indicate a "virtual" light
//Digital types (data pin only) (16-39)
#define TYPE_DIGITAL_MIN         16            // first usable digital type
#define TYPE_WS2812_1CH          18            //white-only chips (1 channel per IC) (unused)
#define TYPE_WS2812_1CH_X3       19            //white-only chips (3 channels per IC)
#define TYPE_WS2812_2CH_X3       20            //CCT chips (1st IC controls WW + CW of 1st zone and CW of 2nd zone, 2nd IC controls WW of 2nd zone and WW + CW of 3rd zone)
#define TYPE_WS2812_WWA          21            //amber + warm + cold white
#define TYPE_WS2812_RGB          22
#define TYPE_GS8608              23            //same driver as WS2812, but will require signal 2x per second (else displays test pattern)
#define TYPE_WS2811_400KHZ       24            //half-speed WS2812 protocol, used by very old WS2811 units
#define TYPE_TM1829              25
#define TYPE_UCS8903             26
#define TYPE_APA106              27
#define TYPE_FW1906              28            //RGB + CW + WW + unused channel (6 channels per IC)
#define TYPE_UCS8904             29            //first RGBW digital type (hardcoded in busmanager.cpp, memUsage())
#define TYPE_SK6812_RGBW         30
#define TYPE_TM1814              31
#define TYPE_WS2805              32            //RGB + WW + CW
#define TYPE_TM1914              33            //RGB
#define TYPE_SM16825             34            //RGB + WW + CW
#define TYPE_WS281X_DUAL         35            //dual WS28XX chip setup (RGB + W00)
//#define TYPE_WS281X_WWCW         36            //dual WS28XX chip setup (RGB + WW+CW+0);  same as FW1906
#define TYPE_DIGITAL_MAX         39            // last usable digital type
//"Analog" types (40-47)
#define TYPE_ONOFF               40            //binary output (relays etc.; NOT PWM)
#define TYPE_ANALOG_MIN          41            // first usable analog type
#define TYPE_ANALOG_1CH          41            //single channel PWM. Uses value of brightest RGBW channel
#define TYPE_ANALOG_2CH          42            //analog WW + CW
#define TYPE_ANALOG_3CH          43            //analog RGB
#define TYPE_ANALOG_4CH          44            //analog RGBW
#define TYPE_ANALOG_5CH          45            //analog RGB + WW + CW
#define TYPE_ANALOG_6CH          46            //analog RGB + A + WW + CW
#define TYPE_ANALOG_MAX          47            // last usable analog type
//Digital types (data + clock / SPI) (48-63)
#define TYPE_2PIN_MIN            48
#define TYPE_WS2801              50
#define TYPE_APA102              51
#define TYPE_LPD8806             52
#define TYPE_P9813               53
#define TYPE_LPD6803             54
#define TYPE_HD108               55
#define TYPE_2PIN_MAX            63
//Digital types (Hub75 matrix) (64-71)
#if defined(WLED_ENABLE_HUB75MATRIX) && (defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3))
#define TYPE_HUB75MATRIX_MIN     64
#if defined(CONFIG_IDF_TARGET_ESP32S3)
#define TYPE_HUB75MATRIX_PORTAL  64           //Adafruit Matrix Portal S3 board (https://www.adafruit.com/product/5778)
#define TYPE_HUB75MATRIX_MOONHUB 65           //MoonHub75 board
#define TYPE_HUB75MATRIX_S3      66           //plain S3 Hub75 matrix board
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
#define TYPE_HUB75MATRIX_S2DRIVE 64           //S2 drive (https://www.ledclub.net/2025/03/15/esp32-s2-drive-p4-80x40-led-matrix/)
#elif defined(CONFIG_IDF_TARGET_ESP32)
#define TYPE_HUB75MATRIX_TRINITY 64           //Trinity/ElectroDragon ESP32 board (https://esp32trinity.com/, https://www.electrodragon.com/product/rgb-matrix-panel-drive-interface-board-for-esp32-dma/)
#define TYPE_HUB75MATRIX_FORUM   65           //ESP32 Forum/SmartMatrix board (https://github.com/rorosaurus/esp32-hub75-driver)
#endif
#define TYPE_HUB75MATRIX_CUSTOM  71           //custom pins defined in hub75pin.json file (manually uploaded, contains JSON array of pin numbers; no validation performed)
#define TYPE_HUB75MATRIX_MAX     71
#endif
//Network types (master broadcast) (80-95)
#define TYPE_VIRTUAL_MIN         80
#define TYPE_NET_DDP_RGB         80           //network DDP RGB bus (master broadcast bus)
#define TYPE_NET_E131_RGB        81           //network E131 RGB bus (master broadcast bus, unused)
#define TYPE_NET_ARTNET_RGB      82           //network ArtNet RGB bus (master broadcast bus, unused)
#define TYPE_NET_DDP_RGBW        88           //network DDP RGBW bus (master broadcast bus)
#define TYPE_NET_ARTNET_RGBW     89           //network ArtNet RGB bus (master broadcast bus, unused)
#define TYPE_VIRTUAL_MAX         95
//Special usermod type
#define TYPE_USERMOD            127           //Usermod defined bus type

//Color orders
#define COL_ORDER_GRB             0           //GRB(w),defaut
#define COL_ORDER_RGB             1           //common for WS2811
#define COL_ORDER_BRG             2
#define COL_ORDER_RBG             3
#define COL_ORDER_BGR             4
#define COL_ORDER_GBR             5
#define COL_ORDER_MAX             5

extern byte briMultiplier;

struct BusConfig; // forward declaration

// Defines an LED Strip and its color ordering.
typedef struct {
  uint16_t start;
  uint16_t len;
  uint8_t colorOrder;
} ColorOrderMapEntry;

struct ColorOrderMap {
    bool add(uint16_t start, uint16_t len, uint8_t colorOrder);

    inline uint8_t count() const { return _mappings.size(); }
    inline void reserve(size_t num) { _mappings.reserve(num); }

    void reset() {
      _mappings.clear();
      _mappings.shrink_to_fit();
    }

    const ColorOrderMapEntry* get(uint8_t n) const {
      if (n >= count()) return nullptr;
      return &(_mappings[n]);
    }

    [[gnu::hot]] uint8_t getPixelColorOrder(uint16_t pix, uint8_t defaultColorOrder) const;

  private:
    std::vector<ColorOrderMapEntry> _mappings;
};


typedef struct {
  uint8_t id;
  const char *type;
  const char *name;
  std::vector<uint8_t> requiredPins;
} LEDType;


//parent class of BusDigital, BusPwm, and BusNetwork
class Bus {
  public:
    Bus(uint8_t type, uint16_t start, uint8_t aw, uint16_t len = 1, bool reversed = false, bool refresh = false)
    : _type(type)
    , _bri(255)
    , _start(start)
    , _len(std::max(len,(uint16_t)1))
    , _reversed(reversed)
    , _valid(false)
    , _needsRefresh(refresh)
    , _scale(100)
    {
      _autoWhiteMode = Bus::hasWhite(type) ? aw : RGBW_MODE_MANUAL_ONLY;
    };

    virtual ~Bus() {} //throw the bus under the bus

    virtual void     begin()                                    {};
    virtual void     show()                                     = 0;
    virtual bool     canShow() const                            { return true; }
    virtual void     setStatusPixel(uint32_t c)                 {}
    virtual void     setPixelColor(unsigned pix, uint32_t c)    = 0;
    virtual void     setBrightness(uint8_t b)                   { _bri = scaleBri(b, briMultiplier); }; // use global modifier
    virtual void     setColorOrder(uint8_t co)                  {}
    virtual uint8_t  getBrightness() const                      { return _bri; }
    virtual size_t   getPins(uint8_t* pinArray = nullptr) const { return 0; }
    virtual uint16_t getLength() const                          { return isOk() ? _len : 0; }
    virtual uint8_t  getColorOrder() const                      { return COL_ORDER_RGB; }
    virtual unsigned skippedLeds() const                        { return 0; }
    virtual uint16_t getFrequency() const                       { return 0U; }
    virtual uint16_t getLEDCurrent() const                      { return 0; }
    virtual uint16_t getUsedCurrent() const                     { return 0; }
    virtual uint16_t getMaxCurrent() const                      { return 0; }
    virtual size_t   getBusSize() const                         { return sizeof(Bus); }
    virtual const String getCustomText() const                  { return String(); }

    inline  bool     hasRGB() const                             { return _hasRgb; }
    inline  bool     hasWhite() const                           { return _hasWhite; }
    inline  bool     hasCCT() const                             { return _hasCCT; }
    inline  bool     isDigital() const                          { return isDigital(_type); }
    inline  bool     is2Pin() const                             { return is2Pin(_type); }
    inline  bool     isOnOff() const                            { return isOnOff(_type); }
    inline  bool     isPWM() const                              { return isPWM(_type); }
    inline  bool     isVirtual() const                          { return isVirtual(_type); }
    inline  bool     isHub75() const                            { return isHub75(_type); }
    inline  bool     isUsermod() const                          { return isUsermod(_type); }
    inline  bool     is16bit() const                            { return is16bit(_type); }
    inline  bool     mustRefresh() const                        { return mustRefresh(_type); }
    inline  void     setReversed(bool reversed)                 { _reversed = reversed; }
    inline  void     setStart(uint16_t start)                   { _start = start; }
    inline  void     setAutoWhiteMode(uint8_t m)                { if (m < 5) _autoWhiteMode = m; }
    inline  uint8_t  getAutoWhiteMode() const                   { return _autoWhiteMode; }
    inline  size_t   getNumberOfChannels() const                { return hasWhite() + 3*hasRGB() + hasCCT(); }
    inline  uint16_t getStart() const                           { return _start; }
    inline  uint8_t  getType() const                            { return _type; }
    inline  bool     isOk() const                               { return _valid; }
    inline  bool     isReversed() const                         { return _reversed; }
    inline  bool     isOffRefreshRequired() const               { return _needsRefresh; }
    inline  bool     containsPixel(uint16_t pix) const          { return pix >= _start && pix < _start + _len; }
    inline  uint8_t  getBrightnessFactor() const                { return _scale; }

    static inline std::vector<LEDType> getLEDTypes()            { return {{TYPE_NONE, "", PSTR("None")}}; } // not used. just for reference for derived classes
    static constexpr size_t   getNumberOfPins(uint8_t type)     { return isUsermod(type) || isHub75(type) ? 5 : isVirtual(type) ? 4 : isPWM(type) ? numPWMPins(type) : is2Pin(type) + 1; } // credit @PaoloTK
    static constexpr size_t   getNumberOfChannels(uint8_t type) { return hasWhite(type) + 3*hasRGB(type) + hasCCT(type); }
    static constexpr bool hasRGB(uint8_t type) {
      return !((type >= TYPE_WS2812_1CH && type <= TYPE_WS2812_WWA) || type == TYPE_ANALOG_1CH || type == TYPE_ANALOG_2CH || type == TYPE_ONOFF);
    }
    static constexpr bool hasWhite(uint8_t type) {
      return  (type >= TYPE_WS2812_1CH && type <= TYPE_WS2812_WWA) || type == TYPE_WS281X_DUAL || //type == TYPE_WS281X_WWCW ||
              type == TYPE_SK6812_RGBW || type == TYPE_TM1814 || type == TYPE_UCS8904 ||
              type == TYPE_FW1906 || type == TYPE_WS2805 || type == TYPE_SM16825 ||        // digital types with white channel
              (type > TYPE_ONOFF && type <= TYPE_ANALOG_5CH && type != TYPE_ANALOG_3CH) || // analog types with white channel
              type == TYPE_NET_DDP_RGBW || type == TYPE_NET_ARTNET_RGBW;                   // network types with white channel
    }
    static constexpr bool hasCCT(uint8_t type) {
      return  type == TYPE_WS2812_2CH_X3 || type == TYPE_WS2812_WWA || //type == TYPE_WS281X_WWCW ||
              type == TYPE_ANALOG_2CH    || type == TYPE_ANALOG_5CH ||
              type == TYPE_FW1906        || type == TYPE_WS2805     ||
              type == TYPE_SM16825;
    }
    static constexpr bool  isTypeValid(uint8_t type)  { return (type > 15 && type < 128); }
    static constexpr bool  isDigital(uint8_t type)    { return (type >= TYPE_DIGITAL_MIN && type <= TYPE_DIGITAL_MAX) || is2Pin(type); }
    static constexpr bool  is2Pin(uint8_t type)       { return (type >= TYPE_2PIN_MIN && type <= TYPE_2PIN_MAX); }
    static constexpr bool  isOnOff(uint8_t type)      { return (type == TYPE_ONOFF); }
    static constexpr bool  isPWM(uint8_t type)        { return (type >= TYPE_ANALOG_MIN && type <= TYPE_ANALOG_MAX); }
    static constexpr bool  isVirtual(uint8_t type)    { return (type >= TYPE_VIRTUAL_MIN && type <= TYPE_VIRTUAL_MAX); }
#if defined(WLED_ENABLE_HUB75MATRIX) && (defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3))
    static constexpr bool  isHub75(uint8_t type)      { return (type >= TYPE_HUB75MATRIX_MIN && type <= TYPE_HUB75MATRIX_MAX); }
#else
    static constexpr bool  isHub75(uint8_t type)      { return false; }
#endif
    static constexpr bool  isUsermod(uint8_t type)    { return type == TYPE_USERMOD; }
    static constexpr bool  is16bit(uint8_t type)      { return type == TYPE_UCS8903 || type == TYPE_UCS8904 || type == TYPE_SM16825; }
    static constexpr bool  mustRefresh(uint8_t type)  { return type == TYPE_TM1814; }
    static constexpr int   numPWMPins(uint8_t type)   { return (type - 40); }

    static inline int16_t  getCCT()                   { return _cct; }
    static inline void     setGlobalAWMode(uint8_t m) { if (m < 5) _gAWM = m; else _gAWM = AW_GLOBAL_DISABLED; }
    static inline uint8_t  getGlobalAWMode()          { return _gAWM; }
    static inline void     setCCT(int16_t cct)        { _cct = cct; }
    static inline int8_t   getCCTBlend()              { return (_cctBlend * 100 + (_cctBlend >= 0 ? 64 : -64)) / 127; } // returns -100 to +100, +/-100% = +/-127. +/-64 for rounding 
    static inline void     setCCTBlend(int8_t b)      { // input is -100 to +100
      _cctBlend = (std::max(-100, std::min(100, (int)b)) * 127 + (b >= 0 ? 50 : -50)) / 100; // +/-50 for rounding, b=+/-100% -> +/-127
      //compile-time limiter for hardware that can't power both white channels at max
      #ifdef WLED_MAX_CCT_BLEND
        if (_cctBlend > WLED_MAX_CCT_BLEND) _cctBlend = WLED_MAX_CCT_BLEND;
      #endif
    }
    static void calculateCCT(uint32_t c, uint8_t &ww, uint8_t &cw);

  protected:
    uint8_t  _type;
    uint8_t  _bri;
    uint16_t _start;
    uint16_t _len;
    //struct { //using bitfield struct adds about 250 bytes to binary size
      bool _reversed;//     : 1;
      bool _valid;//        : 1;
      bool _needsRefresh;// : 1;
      bool _hasRgb;//       : 1;
      bool _hasWhite;//     : 1;
      bool _hasCCT;//       : 1;
    //} __attribute__ ((packed));
    uint8_t  _scale;  // used to scale output in % (i.e. <100 reduce brightness, >100 increase brightness)
    uint8_t  _autoWhiteMode;
    // global Auto White Calculation override
    static uint8_t _gAWM;
    // _cct has the following meanings (see calculateCCT() & BusManager::setSegmentCCT()):
    //    -1 means to extract approximate CCT value in K from RGB (in calcualteCCT())
    //    [0,255] is the exact CCT value where 0 means warm and 255 cold
    //    [1900,10060] only for color correction expressed in K (colorBalanceFromKelvin())
    static int16_t _cct;
    // _cctBlend determines WW/CW blending, see calculateCCT()
    //  < 0 - linear blending in center, single white at both ends, single white zone extends with decreased value (-127 min)
    //    0 - linear (CCT 127 => 50% warm, 50% cold)
    //   63 - semi additive/nonlinear (CCT 127 => 66% warm, 66% cold)
    //  127 - additive CCT blending (CCT 127 => 100% warm, 100% cold)
    static int8_t _cctBlend;

    uint32_t autoWhiteCalc(uint32_t c, uint8_t &ww, uint8_t &cw) const;
    static inline unsigned __attribute__((optimize("O2"))) scaleBri(unsigned bri, unsigned multiplier) {
      if (multiplier == 100) return bri;
      unsigned b = (bri * multiplier) / 100;
      return b > 255 ? 255 : b;
    }
};


class BusDigital : public Bus {
  public:
    BusDigital(const BusConfig &bc, uint8_t nr);
    ~BusDigital() { cleanup(); }

    void show() override;
    bool canShow() const override;
    void setStatusPixel(uint32_t c) override;
    [[gnu::hot]] void setPixelColor(unsigned pix, uint32_t c) override;
    void setColorOrder(uint8_t colorOrder) override;
    uint8_t  getColorOrder() const override  { return _colorOrder; }
    size_t   getPins(uint8_t* pinArray = nullptr) const override;
    unsigned skippedLeds() const override    { return _skip; }
    uint16_t getFrequency() const override   { return _frequencykHz; }
    uint16_t getLEDCurrent() const override  { return _milliAmpsPerLed; }
    uint16_t getUsedCurrent() const override { return _milliAmpsTotal; }
    uint16_t getMaxCurrent() const override  { return _milliAmpsMax; }
    size_t   getBusSize() const override;

    inline void setCurrentLimit(uint16_t milliAmps) { _milliAmpsLimit = milliAmps; DEBUGBUS_PRINTF_P(PSTR("Bus: Set current limit to %d mA\n"), (int)milliAmps); }
    inline void addPixelCurrent(int sum)            { _busPowerSum += sum; }
    inline void clearPixelsCurrent()                { _busPowerSum = 0; }

    void begin() override;
    void cleanup();

    static std::vector<LEDType> getLEDTypes();

  private:
    void    *_busPtr;
    uint32_t _busPowerSum;
    uint16_t _frequencykHz;
    uint16_t _milliAmpsMax;
    uint16_t _milliAmpsLimit;
    uint8_t  _pins[2];
    uint8_t  _skip;
    uint8_t  _colorOrder;
    uint8_t  _iType;
    uint8_t  _milliAmpsPerLed;
    bool     _consistent; // RMT bus needs consistent buffers otherwise skipped LEDs or gaps may show random colors

    static uint16_t _milliAmpsTotal; // is overwitten/recalculated on each show()

    inline uint32_t restoreColorLossy(uint32_t c, uint8_t restoreBri) const {
      if (restoreBri < 255) {
        uint8_t* chan = (uint8_t*) &c;
        for (uint_fast8_t i=0; i<4; i++) {
          uint_fast16_t val = chan[i];
          chan[i] = ((val << 8) + restoreBri) / (restoreBri + 1); // adding restoreBri slightly improves recovery / stops degradation on re-scale
        }
      }
      return c;
    }

    void estimateCurrentAndLimitBri();
};


class BusPwm : public Bus {
  public:
    BusPwm(const BusConfig &bc);
    ~BusPwm() { cleanup(); }

    void setPixelColor(unsigned pix, uint32_t c) override;
    size_t   getPins(uint8_t* pinArray = nullptr) const override;
    uint16_t getFrequency() const override { return _frequency; }
    size_t   getBusSize() const override   { return sizeof(BusPwm); }
    void show() override;
    inline void cleanup() { deallocatePins(); }

    static std::vector<LEDType> getLEDTypes();

  private:
    uint16_t _frequency;
    uint8_t _pins[5]; // must be less or equal to OUTPUT_MAX_PINS
    uint8_t _data[5]; // must be less or equal to OUTPUT_MAX_PINS
    #ifdef ARDUINO_ARCH_ESP32
    uint8_t _ledcStart;
    #endif
    uint8_t _depth;

    void deallocatePins();
};


class BusOnOff : public Bus {
  public:
    BusOnOff(const BusConfig &bc);
    ~BusOnOff() { cleanup(); }

    void setPixelColor(unsigned pix, uint32_t c) override;
    size_t   getPins(uint8_t* pinArray) const override;
    size_t   getBusSize() const override { return sizeof(BusOnOff); }
    void show() override;
    inline void cleanup() { PinManager::deallocatePin(_pin, PinOwner::BusOnOff); }

    static std::vector<LEDType> getLEDTypes();

  private:
    uint8_t _pin;
    uint8_t _data;
};


class BusNetwork : public Bus {
  public:
    BusNetwork(const BusConfig &bc);
    ~BusNetwork() { cleanup(); }

    bool canShow() const override  { return !_broadcastLock; } // this should be a return value from UDP routine if it is still sending data out
    [[gnu::hot]] void setPixelColor(unsigned pix, uint32_t c) override;
    size_t getPins(uint8_t* pinArray = nullptr) const override;
    size_t getBusSize() const override  { return sizeof(BusNetwork) + (isOk() ? _len * _UDPchannels : 0); }
    void   show() override;
    void   cleanup();
    #ifdef ARDUINO_ARCH_ESP32
    void   resolveHostname();
    const String getCustomText() const override { return _hostname; }
    #endif

    static std::vector<LEDType> getLEDTypes();

  private:
    uint8_t   *_data;
    IPAddress _client;
    #ifdef ARDUINO_ARCH_ESP32
    String    _hostname;
    #endif
    uint8_t   _UDPtype;
    uint8_t   _UDPchannels;
    bool      _broadcastLock;
};


// Hub75 driver will eat about 12kB of flash and about 3kB of RAM so it is conditionally included for the moment
#if defined(WLED_ENABLE_HUB75MATRIX) && (defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3))
// forward declarations
class MatrixPanel_I2S_DMA;
class VirtualMatrixPanel;
class HUB75_I2S_CFG;

class BusHub75Matrix : public Bus {
  public:
    BusHub75Matrix(const BusConfig &bc);
    [[gnu::hot]] void setPixelColor(unsigned pix, uint32_t c) override;
    void show() override;
    void setBrightness(uint8_t b) override;
    uint16_t getFrequency() const override;
    size_t getPins(uint8_t* pinArray = nullptr) const override;
    size_t getBusSize() const override;
    void deallocatePins();
    void cleanup();

    static inline uint8_t *getCustomPinsArray() { return _customPins; }

    ~BusHub75Matrix() {
      cleanup();
    }

    static std::vector<LEDType> getLEDTypes(void);

  private:
    unsigned _matrixWidth;
    //byte *_ledsDirty;
    MatrixPanel_I2S_DMA *display;
    VirtualMatrixPanel  *virtualDisp;
    static uint8_t _customPins[14]; // another option would be to allocate memory dynamically (if needed), but this is simpler for now
};
#endif


//temporary struct for passing bus configuration to bus
struct BusConfig {
  uint8_t type;
  uint16_t count;
  uint16_t start;
  uint8_t colorOrder;
  bool reversed;
  uint8_t skipAmount;
  bool refreshReq;
  uint8_t autoWhite;
  uint8_t pins[OUTPUT_MAX_PINS] = {255, 255, 255, 255, 255};
  uint16_t frequency;
  uint8_t milliAmpsPerLed;
  uint16_t milliAmpsMax;
  String text;
  uint8_t scale;

  BusConfig(uint8_t busType, uint8_t* ppins, uint16_t pstart = 0, uint16_t len = DEFAULT_LED_COUNT, uint8_t pcolorOrder = COL_ORDER_GRB, bool rev = false, uint8_t skip = 0, byte aw=RGBW_MODE_MANUAL_ONLY, uint16_t clock_kHz=0U, uint8_t maPerLed=LED_MILLIAMPS_DEFAULT, uint16_t maMax=ABL_MILLIAMPS_DEFAULT, String sometext = "", uint8_t pscale = 100)
  : count(std::max(len,(uint16_t)1))
  , start(pstart)
  , colorOrder(pcolorOrder)
  , reversed(rev)
  , skipAmount(skip)
  , autoWhite(aw)
  , frequency(clock_kHz)
  , milliAmpsPerLed(maPerLed)
  , milliAmpsMax(maMax)
  , text(sometext)
  , scale(std::max(1,(int)pscale))
  {
    refreshReq = (bool) GET_BIT(busType,7);
    type = busType & 0x7F;  // bit 7 may be/is hacked to include refresh info (1=refresh in off state, 0=no refresh)
    if (Bus::isDigital(type)) count = std::min(count, (uint16_t)MAX_LEDS_PER_BUS);
    size_t nPins = Bus::getNumberOfPins(type);
    for (size_t i = 0; i < nPins; i++) pins[i] = ppins[i];
    DEBUGBUS_PRINTF_P(PSTR("Bus: Config (%d-%d, type:%d, CO:%d, rev:%d, skip:%d, AW:%d kHz:%d, mA:%d/%d)\n"),
      (int)start, (int)(start+len),
      (int)type,
      (int)colorOrder,
      (int)reversed,
      (int)skipAmount,
      (int)autoWhite,
      (int)frequency,
      (int)milliAmpsPerLed, (int)milliAmpsMax
    );
  }

  size_t memUsage(unsigned nr = 0) const;
};


//fine tune power estimation constants for your setup
//you can set it to 0 if the ESP is powered by USB and the LEDs by external
#ifndef MA_FOR_ESP
  #ifdef ESP8266
    #define MA_FOR_ESP         80 //how much mA does the ESP use (Wemos D1 about 80mA)
  #else
    #define MA_FOR_ESP        120 //how much mA does the ESP use (ESP32 about 120mA)
  #endif
#endif

namespace BusManager {

  extern std::vector<std::unique_ptr<Bus>> busses;
  extern uint16_t _gMilliAmpsUsed;

  #ifdef ESP32_DATA_IDLE_HIGH
  void    esp32RMTInvertIdle() ;
  #endif
  inline size_t   getNumVirtualBusses() {
    size_t j = 0;
    for (const auto &bus : busses) j += bus->isVirtual();
    return j;
  }

  #ifdef WLED_DEBUG // used only in general debug
  size_t          memUsage();
  #endif
  inline uint16_t currentMilliamps()     { return _gMilliAmpsUsed; }
  void initializeABL(unsigned gMilliAmpsMax);  // setup per output ABL parameters, call once after buses are initialized

  void useParallelOutput(); // workaround for inaccessible PolyBus
  bool hasParallelOutput(); // workaround for inaccessible PolyBus

  //do not call this method from system context (network callback)
  void removeAll();
  int  add(const BusConfig &bc);

  void on();
  void off();

  void  show();
  bool canAllShow();

  inline void    setStatusPixel(uint32_t c) { for (auto &bus : busses) bus->setStatusPixel(c);}
  inline void    setBrightness(uint8_t b)   { for (auto &bus : busses) bus->setBrightness(b); }
  inline void    setPixelColor(unsigned pix, uint32_t c) { for (auto &bus : busses) bus->setPixelColor(pix - bus->getStart(), c); } // rely on unsigned underflow
  // for setSegmentCCT(), cct can only be in [-1,255] range; allowWBCorrection will convert it to K
  // WARNING: setSegmentCCT() is a misleading name!!! much better would be setGlobalCCT() or just setCCT()
  void           setSegmentCCT(int16_t cct, bool allowWBCorrection = false);
  inline int16_t getSegmentCCT()         { return Bus::getCCT(); }
  inline Bus*    getBus(size_t busNr)    { return busNr < busses.size() ? busses[busNr].get() : nullptr; }
  inline size_t  getNumBusses()          { return busses.size(); }

  //semi-duplicate of strip.getLengthTotal() (though that just returns strip._length, calculated in finalizeInit())
  inline uint16_t getTotalLength(bool onlyPhysical = false) {
    unsigned len = 0;
    for (const auto &bus : busses) if (!(bus->isVirtual() && onlyPhysical)) len += bus->getLength();
    return len;
  }
  String         getLEDTypesJSONString();
  ColorOrderMap& getColorOrderMap();
};
#endif
