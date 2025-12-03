/*
 * Class implementation for addressing various light types
 */

#include <Arduino.h>
#include <IPAddress.h>
#include "src/dependencies/network/Network.h" // for isConnected() (& WiFi)
#ifdef ARDUINO_ARCH_ESP32
#include <ESPmDNS.h>
#include "driver/ledc.h"
#include "soc/ledc_struct.h"
  #if !(defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3))
    #define LEDC_MUTEX_LOCK()    do {} while (xSemaphoreTake(_ledc_sys_lock, portMAX_DELAY) != pdPASS)
    #define LEDC_MUTEX_UNLOCK()  xSemaphoreGive(_ledc_sys_lock)
    extern xSemaphoreHandle _ledc_sys_lock;
  #else
    #define LEDC_MUTEX_LOCK()
    #define LEDC_MUTEX_UNLOCK()
  #endif
#endif
#include "bus_manager.h"
#include "bus_wrapper.h"
#include <bits/unique_ptr.h>
// must be included *after* bus_wrapper.h (conflicting R(), G(), B() macros)
#include "colors.h"
#include "network.h"

extern char hostName[];
extern bool cctICused;
extern bool useParallelI2S;

//udp.cpp
uint8_t realtimeBroadcast(uint8_t type, IPAddress client, uint16_t length, const byte *buffer, uint8_t bri=255, bool isRGBW=false);

//util.cpp (contraproductive!!)
// PSRAM allocation wrappers
#if defined(ARDUINO_ARCH_ESP32) && !defined(ARDUINO_ARCH_ESP32C3)
extern "C" {
  void *p_malloc(size_t);           // prefer PSRAM over DRAM
  void *p_calloc(size_t, size_t);   // prefer PSRAM over DRAM
  void *p_realloc(void *, size_t);  // prefer PSRAM over DRAM
  inline void p_free(void *ptr) { heap_caps_free(ptr); }
  void *d_malloc(size_t);           // prefer DRAM over PSRAM
  void *d_calloc(size_t, size_t);   // prefer DRAM over PSRAM
  void *d_realloc(void *, size_t);  // prefer DRAM over PSRAM
  inline void d_free(void *ptr) { heap_caps_free(ptr); }
}
#else
#define p_malloc d_malloc
#define p_calloc d_calloc
#define p_realloc d_realloc
#define p_free d_free
#define d_malloc malloc
#define d_calloc calloc
//#define d_realloc realloc
#define d_free free
extern "C" {
  void *d_realloc(void *, size_t); // implement free + malloc to be consistent with ESP32
}
#endif
#define BFRALLOC_NOBYTEACCESS    (1 << 0) // ESP32 has 32bit accessible DRAM (usually ~50kB free) that must not be byte-accessed
#define BFRALLOC_PREFER_DRAM     (1 << 1) // prefer DRAM over PSRAM
#define BFRALLOC_ENFORCE_DRAM    (1 << 2) // use DRAM only, no PSRAM
#define BFRALLOC_PREFER_PSRAM    (1 << 3) // prefer PSRAM over DRAM
#define BFRALLOC_ENFORCE_PSRAM   (1 << 4) // use PSRAM if available, otherwise fall back to DRAM
#define BFRALLOC_CLEAR           (1 << 5) // clear allocated buffer after allocation
void *allocate_buffer(size_t size, uint32_t type); // buffer allocator with MIN_HEAP_SIZE enforcement

//color mangling macros
#define RGBW32(r,g,b,w) (uint32_t((byte(w) << 24) | (byte(r) << 16) | (byte(g) << 8) | (byte(b))))
#define R(c) (byte((c) >> 16))
#define G(c) (byte((c) >> 8))
#define B(c) (byte(c))
#define W(c) (byte((c) >> 24))


static ColorOrderMap _colorOrderMap = {};

bool ColorOrderMap::add(uint16_t start, uint16_t len, uint8_t colorOrder) {
  if (count() >= WLED_MAX_COLOR_ORDER_MAPPINGS || len == 0 || (colorOrder & 0x0F) > COL_ORDER_MAX) return false; // upper nibble contains W swap information
  _mappings.push_back({start,len,colorOrder});
  DEBUGBUS_PRINTF_P(PSTR("Bus: Add COM (%d,%d,%d)\n"), (int)start, (int)len, (int)colorOrder);
  return true;
}

uint8_t ColorOrderMap::getPixelColorOrder(uint16_t pix, uint8_t defaultColorOrder) const {
  // upper nibble contains W swap information
  // when ColorOrderMap's upper nibble contains value >0 then swap information is used from it, otherwise global swap is used
  for (const auto& map : _mappings) {
    if (pix >= map.start && pix < (map.start + map.len)) return map.colorOrder | ((map.colorOrder >> 4) ? 0 : (defaultColorOrder & 0xF0));
  }
  return defaultColorOrder;
}


void Bus::calculateCCT(uint32_t c, uint8_t &ww, uint8_t &cw) {
  unsigned cct = 0; //0 - full warm white, 255 - full cold white
  unsigned w = W(c);

  if (_cct > -1) {                                    // using RGB?
    if (_cct >= 1900)    cct = (_cct - 1900) >> 5;    // convert K in relative format
    else if (_cct < 256) cct = _cct;                  // already relative
  } else {
    cct = (approximateKelvinFromRGB(c) - 1900) >> 5;  // convert K (from RGB value) to relative format
  }
  
  //0 - linear (CCT 127 = 50% warm, 50% cold), 127 - additive CCT blending (CCT 127 = 100% warm, 100% cold)
  if (cct       < _cctBlend) ww = 255;
  else                       ww = ((255-cct) * 255) / (255 - _cctBlend);
  if ((255-cct) < _cctBlend) cw = 255;
  else                       cw = (cct * 255) / (255 - _cctBlend);

  ww = (w * ww) / 255; //brightness scaling
  cw = (w * cw) / 255;
}

uint32_t Bus::autoWhiteCalc(uint32_t c) const {
  unsigned aWM = _autoWhiteMode;
  if (_gAWM < AW_GLOBAL_DISABLED) aWM = _gAWM;
  if (aWM == RGBW_MODE_MANUAL_ONLY) return c;
  unsigned w = W(c);
  //ignore auto-white calculation if w>0 and mode DUAL (DUAL behaves as BRIGHTER if w==0)
  if (w > 0 && aWM == RGBW_MODE_DUAL) return c;
  unsigned r = R(c);
  unsigned g = G(c);
  unsigned b = B(c);
  if (aWM == RGBW_MODE_MAX) return RGBW32(r, g, b, r > g ? (r > b ? r : b) : (g > b ? g : b)); // brightest RGB channel
  w = r < g ? (r < b ? r : b) : (g < b ? g : b);
  if (aWM == RGBW_MODE_AUTO_ACCURATE) { r -= w; g -= w; b -= w; } //subtract w in ACCURATE mode
  return RGBW32(r, g, b, w);
}


BusDigital::BusDigital(const BusConfig &bc, uint8_t nr)
: Bus(bc.type, bc.start, bc.autoWhite, bc.count, bc.reversed, (bc.refreshReq || bc.type == TYPE_TM1814))
, _busPowerSum(0)
, _skip(bc.skipAmount) //sacrificial pixels
, _colorOrder(bc.colorOrder)
, _milliAmpsPerLed(bc.milliAmpsPerLed)
, _milliAmpsMax(bc.milliAmpsMax)
, _milliAmpsLimit(0)
{
  DEBUGBUS_PRINTLN(F("Bus: Creating digital bus."));
  if (!isDigital(bc.type) || !bc.count) { DEBUGBUS_PRINTLN(F("Not digial or empty bus!")); return; }
  if (!PinManager::allocatePin(bc.pins[0], true, PinOwner::BusDigital)) { DEBUGBUS_PRINTLN(F("Pin 0 allocated!")); return; }
  _frequencykHz = 0U;
  _pins[0] = bc.pins[0];
  if (is2Pin(bc.type)) {
    if (!PinManager::allocatePin(bc.pins[1], true, PinOwner::BusDigital)) {
      cleanup();
      DEBUGBUS_PRINTLN(F("Pin 1 allocated!"));
      return;
    }
    _pins[1] = bc.pins[1];
    _frequencykHz = bc.frequency ? bc.frequency : 2000U; // 2MHz clock if undefined
  }
  _iType = PolyBus::getI(bc.type, _pins, nr);
  if (_iType == I_NONE) { DEBUGBUS_PRINTLN(F("Incorrect iType!")); return; }
  _hasRgb = hasRGB(bc.type);
  _hasWhite = hasWhite(bc.type);
  _hasCCT = hasCCT(bc.type);
  uint16_t lenToCreate = bc.count;
  if (bc.type == TYPE_WS2812_1CH_X3) lenToCreate = NUM_ICS_WS2812_1CH_3X(bc.count); // only needs a third of "RGB" LEDs for NeoPixelBus
  _busPtr = PolyBus::create(_iType, _pins, lenToCreate + _skip, nr);
  _valid = (_busPtr != nullptr) && bc.count > 0;
  // fix for wled#4759
  if (_valid) for (unsigned i = 0; i < _skip; i++) {
    PolyBus::setPixelColor(_busPtr, _iType, i, 0, COL_ORDER_GRB); // set sacrificial pixels to black (CO does not matter here)
  }
  DEBUGBUS_PRINTF_P(PSTR("Bus: %successfully inited #%u (len:%u, type:%u (RGB:%d, W:%d, CCT:%d), pins:%u,%u [itype:%u] mA=%d/%d)\n"),
    _valid?"S":"Uns",
    (int)nr,
    (int)bc.count,
    (int)bc.type,
    (int)_hasRgb, (int)_hasWhite, (int)_hasCCT,
    (unsigned)_pins[0], is2Pin(bc.type)?(unsigned)_pins[1]:255U,
    (unsigned)_iType,
    (int)_milliAmpsPerLed, (int)_milliAmpsMax
  );
}

//DISCLAIMER
//The following function attemps to calculate the current LED power usage,
//and will limit the brightness to stay below a set amperage threshold.
//It is NOT a measurement and NOT guaranteed to stay within the ablMilliampsMax margin.
//Stay safe with high amperage and have a reasonable safety margin!
//I am NOT to be held liable for burned down garages or houses!

// To disable brightness limiter we either set output max current to 0 or single LED current to 0
void BusDigital::estimateCurrentAndLimitBri() {
  if (_milliAmpsLimit == 0) return;

  byte actualMilliampsPerLed = getLEDCurrent() == 255 ? 12 : getLEDCurrent(); // from testing an actual WS2815 strip

  // WARNING: _busPowerSum is accumulated/summed in setPixelColor() calls

  if (hasWhite()) {     // RGBW led total output with white LEDs enabled is still 50mA, so each channel uses less
    _busPowerSum *= 3;
    _busPowerSum >>= 2; // same as /= 4
  }

  // _busPowerSum has all the values of channels summed (max would be getLength()*765 as white is excluded) so convert to milliAmps
  BusDigital::_milliAmpsTotal = (_busPowerSum * actualMilliampsPerLed) / 765 + getLength(); // each LED uses about 1mA in standby, exclude that from power budget

  if (BusDigital::_milliAmpsTotal > _milliAmpsLimit) {
    // scale brightness down to stay in current limit
    uint8_t scaleB = max(1, _milliAmpsLimit * 255 / BusDigital::_milliAmpsTotal);
    //DEBUGBUS_PRINTF_P(PSTR("Bus: ABL reducing brightness from: %d to: %d\n"), (int)_bri, (int)((_bri * scaleB) >> 8));
    uint8_t cctWW = 0, cctCW = 0;
    unsigned hwLen = _len;
    if (_type == TYPE_WS2812_1CH_X3) hwLen = NUM_ICS_WS2812_1CH_3X(_len); // only needs a third of "RGB" LEDs for NeoPixelBus
    for (unsigned i = 0; i < hwLen; i++) {
      // TODO: fix CCT handling
      uint32_t c = color_fade(PolyBus::getPixelColor(_busPtr, _iType, i, 0), scaleB, true);
      if (hasCCT()) Bus::calculateCCT(c, cctWW, cctCW); // this will unfortunately corrupt CCT data
      PolyBus::setPixelColor(_busPtr, _iType, i, c, 0, (cctCW<<8) | cctWW); // repaint all pixels with new brightness
    }
    BusDigital::_milliAmpsTotal = _milliAmpsLimit;
  }
}

void BusDigital::show() {
  BusDigital::_milliAmpsTotal = 0;
  if (!_valid) return;
  // per-port ABL (will not work well with CCT LEDs)
  estimateCurrentAndLimitBri();  // will also fill _milliAmpsTotal
  PolyBus::show(_busPtr, _iType, _skip); // faster if buffer consistency is not important (no skipped LEDs)
  clearPixelsCurrent(); // reset for next show
}

bool BusDigital::canShow() const {
  if (!_valid) return true;
  return PolyBus::canShow(_busPtr, _iType);
}

//If LEDs are skipped, it is possible to use the first as a status LED.
//TODO only show if no new show due in the next 50ms
void BusDigital::setStatusPixel(uint32_t c) {
  if (_valid && _skip) {
    PolyBus::setPixelColor(_busPtr, _iType, 0, c, _colorOrderMap.getPixelColorOrder(_start, _colorOrder));
    if (canShow()) PolyBus::show(_busPtr, _iType);
  }
}

void BusDigital::setPixelColor(unsigned pix, uint32_t c) {
  if (!_valid) return;
  if (hasWhite()) c = autoWhiteCalc(c);
  if (Bus::_cct >= 1900) c = colorBalanceFromKelvin(Bus::_cct, c); //color correction from CCT
  c = color_fade(c, _bri, true);

  // pre-calcualte power usage for per-output ABL (a single bus should never have over 2000 LEDs so uint32_t is enough for _busPowerSum)
  // WARNING: assumes pixel is not modified agin until show() is called
  if (_milliAmpsLimit > 0) {
    uint8_t r = R(c), g = G(c), b = B(c), w = W(c);
    int sum = getLEDCurrent() == 255 ? (max(max(r,g),b)) * 3 : (r + g + b + w);
    addPixelCurrent(sum);
  }

  if (_reversed) pix = _len - pix -1;
  pix += _skip;
  const uint8_t co = _colorOrderMap.getPixelColorOrder(pix+_start, _colorOrder);
  if (_type == TYPE_WS2812_1CH_X3) { // map to correct IC, each controls 3 LEDs
    unsigned pOld = pix;
    pix = IC_INDEX_WS2812_1CH_3X(pix);
    const uint32_t cOld = PolyBus::getPixelColor(_busPtr, _iType, pix, co); // no need for restoreColorLossy, we just need to modify single channel
    switch (pOld % 3) { // change only the single channel (TODO: this can cause loss because of get/set)
      case 0: c = RGBW32(R(cOld), W(c)   , B(cOld), 0); break;
      case 1: c = RGBW32(W(c)   , G(cOld), B(cOld), 0); break;
      case 2: c = RGBW32(R(cOld), G(cOld), W(c)   , 0); break;
    }
  }
  uint16_t wwcw = 0;
  if (hasCCT()) {
    uint8_t cctWW = 0, cctCW = 0;
    Bus::calculateCCT(c, cctWW, cctCW);
    wwcw = (cctCW<<8) | cctWW;
    if (_type == TYPE_WS2812_WWA) c = RGBW32(cctWW, cctCW, 0, W(c));
  }
  PolyBus::setPixelColor(_busPtr, _iType, pix, c, co, wwcw);
}

size_t BusDigital::getPins(uint8_t* pinArray) const {
  unsigned numPins = is2Pin(_type) + 1;
  if (pinArray) for (unsigned i = 0; i < numPins; i++) pinArray[i] = _pins[i];
  return numPins;
}

size_t BusDigital::getBusSize() const {
  return sizeof(BusDigital) + (isOk() ? PolyBus::getDataSize(_busPtr, _iType) : 0); // does not include common I2S DMA buffer
}

void BusDigital::setColorOrder(uint8_t colorOrder) {
  // upper nibble contains W swap information
  if ((colorOrder & 0x0F) > 5) return;
  _colorOrder = colorOrder;
}

// credit @willmmiles & @netmindz https://github.com/wled/WLED/pull/4056
std::vector<LEDType> BusDigital::getLEDTypes() {
  return {
    {TYPE_WS2812_RGB,    "D",  PSTR("WS281x")},
    {TYPE_SK6812_RGBW,   "D",  PSTR("SK6812/WS2814 RGBW")},
    {TYPE_TM1814,        "D",  PSTR("TM1814")},
    {TYPE_WS2811_400KHZ, "D",  PSTR("400kHz")},
    {TYPE_TM1829,        "D",  PSTR("TM1829")},
    {TYPE_UCS8903,       "D",  PSTR("UCS8903")},
    {TYPE_APA106,        "D",  PSTR("APA106/PL9823")},
    {TYPE_TM1914,        "D",  PSTR("TM1914")},
    {TYPE_FW1906,        "D",  PSTR("FW1906 GRBCW")},
    {TYPE_UCS8904,       "D",  PSTR("UCS8904 RGBW")},
    {TYPE_WS2805,        "D",  PSTR("WS2805 RGBCW")},
    {TYPE_SM16825,       "D",  PSTR("SM16825 RGBCW")},
    {TYPE_WS2812_1CH_X3, "D",  PSTR("WS2811 White")},
    //{TYPE_WS2812_2CH_X3, "D",  PSTR("WS281x CCT")}, // not implemented
    {TYPE_WS2812_WWA,    "D",  PSTR("WS281x WWA")}, // amber ignored
    {TYPE_WS2801,        "2P", PSTR("WS2801")},
    {TYPE_APA102,        "2P", PSTR("APA102")},
    {TYPE_LPD8806,       "2P", PSTR("LPD8806")},
    {TYPE_LPD6803,       "2P", PSTR("LPD6803")},
    {TYPE_P9813,         "2P", PSTR("PP9813")},
  };
}

void BusDigital::begin() {
  if (!_valid) return;
  PolyBus::begin(_busPtr, _iType, _pins, _frequencykHz);
}

void BusDigital::cleanup() {
  DEBUGBUS_PRINTLN(F("Digital Cleanup."));
  PolyBus::cleanup(_busPtr, _iType);
  _iType = I_NONE;
  _valid = false;
  _busPtr = nullptr;
  //PinManager::deallocateMultiplePins(_pins, 2, PinOwner::BusDigital);
  PinManager::deallocatePin(_pins[1], PinOwner::BusDigital);
  PinManager::deallocatePin(_pins[0], PinOwner::BusDigital);
}


#ifdef ESP8266
  // 1 MHz clock
  #define CLOCK_FREQUENCY 1000000UL
#else
  // Use XTAL clock if possible to avoid timer frequency error when setting APB clock < 80 Mhz
  // https://github.com/espressif/arduino-esp32/blob/2.0.2/cores/esp32/esp32-hal-ledc.c
  #ifdef SOC_LEDC_SUPPORT_XTAL_CLOCK
    #define CLOCK_FREQUENCY 40000000UL
  #else
    #define CLOCK_FREQUENCY 80000000UL
  #endif
#endif

#ifdef ESP8266
  #define MAX_BIT_WIDTH 10
#else
  #ifdef SOC_LEDC_TIMER_BIT_WIDE_NUM
    // C6/H2/P4: 20 bit, S2/S3/C2/C3: 14 bit
    #define MAX_BIT_WIDTH SOC_LEDC_TIMER_BIT_WIDE_NUM 
  #else
    // ESP32: 20 bit (but in reality we would never go beyond 16 bit as the frequency would be to low)
    #define MAX_BIT_WIDTH 14
  #endif
#endif

BusPwm::BusPwm(const BusConfig &bc)
: Bus(bc.type, bc.start, bc.autoWhite, 1, bc.reversed, bc.refreshReq) // hijack Off refresh flag to indicate usage of dithering
{
  if (!isPWM(bc.type)) return;
  unsigned numPins = numPWMPins(bc.type);
  [[maybe_unused]] const bool dithering = _needsRefresh;
  _frequency = bc.frequency ? bc.frequency : WLED_PWM_FREQ;
  // duty cycle resolution (_depth) can be extracted from this formula: CLOCK_FREQUENCY > _frequency * 2^_depth
  for (_depth = MAX_BIT_WIDTH; _depth > 8; _depth--) if (((CLOCK_FREQUENCY/_frequency) >> _depth) > 0) break;

  managed_pin_type pins[numPins];
  for (unsigned i = 0; i < numPins; i++) pins[i] = {(int8_t)bc.pins[i], true};
  if (PinManager::allocateMultiplePins(pins, numPins, PinOwner::BusPwm)) {
    #ifdef ESP8266
    analogWriteRange((1<<_depth)-1);
    analogWriteFreq(_frequency);
    #else
    // for 2 pin PWM CCT strip pinManager will make sure both LEDC channels are in the same speed group and sharing the same timer
    _ledcStart = PinManager::allocateLedc(numPins);
    if (_ledcStart == 255) { //no more free LEDC channels
      PinManager::deallocateMultiplePins(pins, numPins, PinOwner::BusPwm);
      DEBUGBUS_PRINTLN(F("No more free LEDC channels!"));
      return;
    }
    // if _needsRefresh is true (UI hack) we are using dithering (credit @dedehai & @zalatnaicsongor)
    if (dithering) _depth = 12; // fixed 8 bit depth PWM with 4 bit dithering (ESP8266 has no hardware to support dithering)
    #endif

    for (unsigned i = 0; i < numPins; i++) {
      _pins[i] = bc.pins[i]; // store only after allocateMultiplePins() succeeded
      #ifdef ESP8266
      pinMode(_pins[i], OUTPUT);
      #else
      unsigned channel = _ledcStart + i;
      ledcSetup(channel, _frequency, _depth - (dithering*4)); // with dithering _frequency doesn't really matter as resolution is 8 bit
      ledcAttachPin(_pins[i], channel);
      // LEDC timer reset credit @dedehai
      uint8_t group = (channel / 8), timer = ((channel / 2) % 4); // same fromula as in ledcSetup()
      ledc_timer_rst((ledc_mode_t)group, (ledc_timer_t)timer); // reset timer so all timers are almost in sync (for phase shift)
      #endif
    }
    _hasRgb = hasRGB(bc.type);
    _hasWhite = hasWhite(bc.type);
    _hasCCT = hasCCT(bc.type);
    _valid = true;
  }
  DEBUGBUS_PRINTF_P(PSTR("%successfully inited PWM strip with type %u, frequency %u, bit depth %u and pins %u,%u,%u,%u,%u\n"), _valid?"S":"Uns", bc.type, _frequency, _depth, _pins[0], _pins[1], _pins[2], _pins[3], _pins[4]);
}

void BusPwm::setPixelColor(unsigned pix, uint32_t c) {
  if (pix != 0 || !_valid) return; //only react to first pixel
  if (_type != TYPE_ANALOG_3CH) c = autoWhiteCalc(c);
  if (Bus::_cct >= 1900 && (_type == TYPE_ANALOG_3CH || _type == TYPE_ANALOG_4CH)) {
    c = colorBalanceFromKelvin(Bus::_cct, c); //color correction from CCT
  }
  uint8_t r = R(c);
  uint8_t g = G(c);
  uint8_t b = B(c);
  uint8_t w = W(c);

  switch (_type) {
    case TYPE_ANALOG_1CH: //one channel (white), relies on auto white calculation
      _data[0] = w;
      break;
    case TYPE_ANALOG_2CH: //warm white + cold white
      if (cctICused) {
        _data[0] = w;
        _data[1] = Bus::_cct < 0 || Bus::_cct > 255 ? 127 : Bus::_cct;
      } else {
        Bus::calculateCCT(c, _data[0], _data[1]);
      }
      break;
    case TYPE_ANALOG_5CH: //RGB + warm white + cold white
      if (cctICused)
        _data[4] = Bus::_cct < 0 || Bus::_cct > 255 ? 127 : Bus::_cct;
      else
        Bus::calculateCCT(c, w, _data[4]);
    case TYPE_ANALOG_4CH: //RGBW
      _data[3] = w;
    case TYPE_ANALOG_3CH: //standard dumb RGB
      _data[0] = r; _data[1] = g; _data[2] = b;
      break;
  }
}

void BusPwm::show() {
  if (!_valid) return;
  // if _needsRefresh is true (UI hack) we are using dithering (credit @dedehai & @zalatnaicsongor)
  // https://github.com/wled/WLED/pull/4115 and https://github.com/zalatnaicsongor/WLED/pull/1)
  const bool     dithering = _needsRefresh; // avoid working with bitfield
  const size_t   numPins = getPins();
  const unsigned maxBri = (1<<_depth);      // possible values: 16384 (14), 8192 (13), 4096 (12), 2048 (11), 1024 (10), 512 (9) and 256 (8) 
  [[maybe_unused]] const unsigned bitShift = dithering * 4;  // if dithering, _depth is 12 bit but LEDC channel is set to 8 bit (using 4 fractional bits)

  // use CIE brightness formula (linear + cubic) to approximate human eye perceived brightness
  // see: https://en.wikipedia.org/wiki/Lightness
  unsigned pwmBri = _bri;
  if (pwmBri < 21) {                                   // linear response for values [0-20]
    pwmBri = (pwmBri * maxBri + 2300 / 2) / 2300 ;     // adding '0.5' before division for correct rounding, 2300 gives a good match to CIE curve
  } else {                                             // cubic response for values [21-255]
    float temp = float(pwmBri + 41) / float(255 + 41); // 41 is to match offset & slope to linear part
    temp = temp * temp * temp * (float)maxBri;
    pwmBri = (unsigned)temp;                           // pwmBri is in range [0-maxBri] C
  }

  [[maybe_unused]] unsigned hPoint = 0;  // phase shift (0 - maxBri)
  // we will be phase shifting every channel by previous pulse length (plus dead time if required)
  // phase shifting is only mandatory when using H-bridge to drive reverse-polarity PWM CCT (2 wire) LED type 
  // CCT additive blending must be 0 (WW & CW will not overlap) otherwise signals *will* overlap
  // for all other cases it will just try to "spread" the load on PSU
  // Phase shifting requires that LEDC timers are synchronised (see setup()). For PWM CCT (and H-bridge) it is
  // also mandatory that both channels use the same timer (pinManager takes care of that).
  for (unsigned i = 0; i < numPins; i++) {
    unsigned duty = (_data[i] * pwmBri) / 255;    
    #ifdef ESP8266
    if (_reversed) duty = maxBri - duty;
    analogWrite(_pins[i], duty);
    #else
    int deadTime = 0;
    if (_type == TYPE_ANALOG_2CH && Bus::_cctBlend == 0) {
      // add dead time between signals (when using dithering, two full 8bit pulses are required)
      deadTime = (1+dithering) << bitShift;
      // we only need to take care of shortening the signal at (almost) full brightness otherwise pulses may overlap
      if (_bri >= 254 && duty >= maxBri / 2 && duty < maxBri) duty -= deadTime << 1; // shorten duty of larger signal except if full on
      if (_reversed) deadTime = -deadTime; // need to invert dead time to make phaseshift go the opposite way so low signals dont overlap
    }
    if (_reversed) duty = maxBri - duty;
    unsigned channel = _ledcStart + i;
    unsigned gr = channel/8;  // high/low speed group
    unsigned ch = channel%8;  // group channel
    // directly write to LEDC struct as there is no HAL exposed function for dithering
    // duty has 20 bit resolution with 4 fractional bits (24 bits in total)
    LEDC.channel_group[gr].channel[ch].duty.duty = duty << ((!dithering)*4);  // lowest 4 bits are used for dithering, shift by 4 bits if not using dithering
    LEDC.channel_group[gr].channel[ch].hpoint.hpoint = hPoint >> bitShift;    // hPoint is at _depth resolution (needs shifting if dithering)
    ledc_update_duty((ledc_mode_t)gr, (ledc_channel_t)ch);
    hPoint += duty + deadTime;        // offset to cascade the signals
    if (hPoint >= maxBri) hPoint = 0; // offset it out of bounds, reset
    #endif
  }
}

size_t BusPwm::getPins(uint8_t* pinArray) const {
  if (!_valid) return 0;
  unsigned numPins = numPWMPins(_type);
  if (pinArray) for (unsigned i = 0; i < numPins; i++) pinArray[i] = _pins[i];
  return numPins;
}

// credit @willmmiles & @netmindz https://github.com/wled/WLED/pull/4056
std::vector<LEDType> BusPwm::getLEDTypes() {
  return {
    {TYPE_ANALOG_1CH, "A",      PSTR("PWM White")},
    {TYPE_ANALOG_2CH, "AA",     PSTR("PWM CCT")},
    {TYPE_ANALOG_3CH, "AAA",    PSTR("PWM RGB")},
    {TYPE_ANALOG_4CH, "AAAA",   PSTR("PWM RGBW")},
    {TYPE_ANALOG_5CH, "AAAAA",  PSTR("PWM RGB+CCT")},
    //{TYPE_ANALOG_6CH, "AAAAAA", PSTR("PWM RGB+DCCT")}, // unimplementable ATM
  };
}

void BusPwm::deallocatePins() {
  size_t numPins = getPins();
  for (unsigned i = 0; i < numPins; i++) {
    PinManager::deallocatePin(_pins[i], PinOwner::BusPwm);
    if (!PinManager::isPinOk(_pins[i])) continue;
    #ifdef ESP8266
    digitalWrite(_pins[i], LOW); //turn off PWM interrupt
    #else
    if (_ledcStart < WLED_MAX_ANALOG_CHANNELS) ledcDetachPin(_pins[i]);
    #endif
  }
  #ifdef ARDUINO_ARCH_ESP32
  PinManager::deallocateLedc(_ledcStart, numPins);
  #endif
}


BusOnOff::BusOnOff(const BusConfig &bc)
: Bus(bc.type, bc.start, bc.autoWhite, 1, bc.reversed)
, _data(0)
{
  if (!Bus::isOnOff(bc.type)) return;

  uint8_t currentPin = bc.pins[0];
  if (!PinManager::allocatePin(currentPin, true, PinOwner::BusOnOff)) {
    return;
  }
  _pin = currentPin; //store only after allocatePin() succeeds
  pinMode(_pin, OUTPUT);
  _hasRgb = false;
  _hasWhite = false;
  _hasCCT = false;
  _valid = true;
  DEBUGBUS_PRINTF_P(PSTR("%successfully inited On/Off strip with pin %u\n"), _valid?"S":"Uns", _pin);
}

void BusOnOff::setPixelColor(unsigned pix, uint32_t c) {
  if (pix != 0 || !_valid) return; //only react to first pixel
  c = autoWhiteCalc(c);
  uint8_t r = R(c);
  uint8_t g = G(c);
  uint8_t b = B(c);
  uint8_t w = W(c);
  _data = bool(r|g|b|w) && bool(_bri) ? 0xFF : 0;
}

void BusOnOff::show() {
  if (!_valid) return;
  digitalWrite(_pin, _reversed ? !(bool)_data : (bool)_data);
}

size_t BusOnOff::getPins(uint8_t* pinArray) const {
  if (!_valid) return 0;
  if (pinArray) pinArray[0] = _pin;
  return 1;
}

// credit @willmmiles & @netmindz https://github.com/wled/WLED/pull/4056
std::vector<LEDType> BusOnOff::getLEDTypes() {
  return {
    {TYPE_ONOFF, "", PSTR("On/Off")},
  };
}

BusNetwork::BusNetwork(const BusConfig &bc)
: Bus(bc.type, bc.start, bc.autoWhite, bc.count)
, _broadcastLock(false)
{
  switch (bc.type) {
    case TYPE_NET_ARTNET_RGB:
      _UDPtype = 2;
      break;
    case TYPE_NET_ARTNET_RGBW:
      _UDPtype = 2;
      break;
    case TYPE_NET_E131_RGB:
      _UDPtype = 1;
      break;
    default: // TYPE_NET_DDP_RGB / TYPE_NET_DDP_RGBW
      _UDPtype = 0;
      break;
  }
  _hasRgb = hasRGB(bc.type);
  _hasWhite = hasWhite(bc.type);
  _hasCCT = false;
  _UDPchannels = _hasWhite + 3;
  _client = IPAddress(bc.pins[0],bc.pins[1],bc.pins[2],bc.pins[3]);
  #ifdef ARDUINO_ARCH_ESP32
  _hostname = bc.text;
  resolveHostname(); // resolve hostname to IP address if needed
  #endif
  _data = (uint8_t*)d_calloc(_len, _UDPchannels);
  _valid = (_data != nullptr);
  DEBUGBUS_PRINTF_P(PSTR("%successfully inited virtual strip with type %u and IP %u.%u.%u.%u\n"), _valid?"S":"Uns", bc.type, bc.pins[0], bc.pins[1], bc.pins[2], bc.pins[3]);
}

void BusNetwork::setPixelColor(unsigned pix, uint32_t c) {
  if (!_valid || pix >= _len) return;
  if (_hasWhite) c = autoWhiteCalc(c);
  if (Bus::_cct >= 1900) c = colorBalanceFromKelvin(Bus::_cct, c); //color correction from CCT
  unsigned offset = pix * _UDPchannels;
  _data[offset]   = R(c);
  _data[offset+1] = G(c);
  _data[offset+2] = B(c);
  if (_hasWhite) _data[offset+3] = W(c);
}

void BusNetwork::show() {
  if (!_valid || !canShow()) return;
  _broadcastLock = true;
  realtimeBroadcast(_UDPtype, _client, _len, _data, _bri, hasWhite());
  _broadcastLock = false;
}

size_t BusNetwork::getPins(uint8_t* pinArray) const {
  if (pinArray) for (unsigned i = 0; i < 4; i++) pinArray[i] = _client[i];
  return 4;
}

#ifdef ARDUINO_ARCH_ESP32
void BusNetwork::resolveHostname() {
  static unsigned long nextResolve = 0;
  if (millis() > nextResolve && _hostname.length() > 0) {
    nextResolve = millis() + 600000; // resolve only every 10 minutes
    IPAddress clnt = ::resolveHostname(_hostname, true);
    if (clnt != IPAddress()) _client = clnt;
  }
}
#endif

// credit @willmmiles & @netmindz https://github.com/wled/WLED/pull/4056
std::vector<LEDType> BusNetwork::getLEDTypes() {
  return {
    {TYPE_NET_DDP_RGB,     "N",     PSTR("DDP RGB (network)")},      // should be "NNNN" to determine 4 "pin" fields
    {TYPE_NET_ARTNET_RGB,  "N",     PSTR("Art-Net RGB (network)")},
    {TYPE_NET_DDP_RGBW,    "N",     PSTR("DDP RGBW (network)")},
    {TYPE_NET_ARTNET_RGBW, "N",     PSTR("Art-Net RGBW (network)")},
    // hypothetical extensions, not strictly network types, but virtual buses
    //{TYPE_VIRTUAL_I2C_W,   "V",     PSTR("I2C White (virtual)")}, // allows setting I2C address in _pin[0]
    //{TYPE_VIRTUAL_I2C_CCT, "V",     PSTR("I2C CCT (virtual)")}, // allows setting I2C address in _pin[0]
    //{TYPE_VIRTUAL_I2C_RGB, "VVV",   PSTR("I2C RGB (virtual)")}, // allows setting I2C address in _pin[0] and 2 additional values in _pin[1] & _pin[2]
    //{TYPE_USERMOD,         "UUUUU", PSTR("Usermod (virtual)"), {{1,2,3,4,5}}}, // 5 data fields (see https://github.com/wled/WLED/pull/4123) and 5 unchageable pins
  };
}

void BusNetwork::cleanup() {
  DEBUGBUS_PRINTLN(F("Virtual Cleanup."));
  d_free(_data);
  _data = nullptr;
  _type = I_NONE;
  _valid = false;
}


#ifdef WLED_ENABLE_HUB75MATRIX
  #include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
  #include <ESP32-VirtualMatrixPanel-I2S-DMA.h>

  #if !(defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3))
    #error "Unsupported ESP platform for HUB75 display. Only ESP32, ESP32-S2 and ESP32-S3 are supported."
  #else
    #warning "HUB75 driver enabled (experimental)"
  #endif

/*
// functions to get/set bits in an array - based on functions created by @Brandon502 for GOL
// used for tracking dirty LEDs in HUB75 matrix
static bool getBitFromArray(const uint8_t* byteArray, size_t position) { // get bit value
  size_t byteIndex = position >> 3; // position / 8
  size_t bitIndex  = position & 7;  // position % 8
  uint8_t byteValue = byteArray[byteIndex];
  return (byteValue >> bitIndex) & 1;
}

static void setBitInArray(uint8_t* byteArray, size_t position, bool value) {  // set bit - with error handling for nullptr
    size_t byteIndex = position >> 3; // position / 8
    size_t bitIndex  = position & 7;  // position % 8
    if (value) byteArray[byteIndex] |=  (uint8_t)(1U << bitIndex);
    else       byteArray[byteIndex] &= ~(uint8_t)(1U << bitIndex);
}

static inline size_t getBitArrayBytes(size_t num_bits) { // number of bytes needed for an array with num_bits bits
  return (num_bits + 7) >> 3; // (num_bits + 7) / 8
}

static inline void setBitArray(uint8_t* byteArray, size_t numBits, bool value) {  // set all bits to same value
  size_t len = getBitArrayBytes(numBits);
  memset(byteArray, value * 0xFF, len);
}
*/

static constexpr size_t HUB75_PIN_COUNT = sizeof(HUB75_I2S_CFG::gpio) / sizeof(int8_t);

// known controller board pinouts
static const uint8_t * const getHub75Pins(uint8_t type, uint8_t *dest = nullptr) {
  const uint8_t *b = nullptr;
  switch (type) {
    default:
    case TYPE_HUB75MATRIX_FORUM: {
      static uint8_t a[HUB75_PIN_COUNT] PROGMEM = { 2, 15,  4, 16, 27, 17,  5, 18, 19, 21, 12, 26, 25, 22 };
      b = a;
    }
    case TYPE_HUB75MATRIX_PORTAL: {
      static uint8_t a[HUB75_PIN_COUNT] PROGMEM = { 42, 41, 40, 38, 39, 37, 45, 36, 48, 35, 21, 47, 14,  2 };
      b = a;
    }
    case TYPE_HUB75MATRIX_MOONHUB: {
      static uint8_t a[HUB75_PIN_COUNT] PROGMEM = {  1,  5,  6,  7, 13,  9, 16, 48, 47, 21, 38,  8,  4, 18 };
      b = a;
    }
    case TYPE_HUB75MATRIX_TRINITY: {
      static uint8_t a[HUB75_PIN_COUNT] PROGMEM = { 25, 26, 27, 14, 12, 13, 23, 19,  5, 17, 18,  4, 15, 16 };
      b = a;
    }
    case TYPE_HUB75MATRIX_S3: {
      static uint8_t a[HUB75_PIN_COUNT] PROGMEM = {  1,  2, 42, 41, 40, 39, 45, 48, 47, 21, 38,  8,  3, 18 };
      b = a;
    }
  }
  if (dest != nullptr) memcpy_P(dest, b, HUB75_PIN_COUNT);
  return b;
}

BusHub75Matrix::BusHub75Matrix(const BusConfig &bc)
: Bus(bc.type, bc.start, bc.autoWhite, bc.count, false, bc.refreshReq)
, _matrixWidth(0)
//, _ledsDirty(nullptr)
, display(nullptr)
, virtualDisp(nullptr)
{
  #ifdef WLED_DEBUG_BUS
  size_t lastHeap = getFreeHeapSize();
  #endif
  //_valid = false; // not needed as Bus constructor already set it to false
  _hasRgb = true;
  _hasWhite = false;
  _hasCCT = false;

  // clamp panel width and height to multiples of 32
  uint8_t dim[2];
  dim[0] = bc.pins[0] & 0xE0;
  dim[1] = bc.pins[1] & 0xE0;
  for (int j=0; j<2; j++) {
    if (dim[j] <  32) dim[j] =  32;
    if (dim[j] > 128) dim[j] = 128;
    // this may not be needed if sizes allowed include [96]
    if (dim[j] & 0x40) dim[j] &= 0x40;
  }

  HUB75_I2S_CFG mxconfig; // default config
  mxconfig.double_buff = false;   // (default) do no double buffering to save RAM
  //mxconfig.double_buff = true;  // need to call flipDMABuffer() in each show()
  mxconfig.driver = (HUB75_I2S_CFG::shift_driver)bc.pins[3];
  // mxconfig.latch_blanking = 3;
  mxconfig.i2sspeed = (HUB75_I2S_CFG::clk_speed)(bc.frequency * 1000); // correctly set in set.cpp (8000, 16000, 20000)
  // mxconfig.min_refresh_rate = 90;
  // mxconfig.min_refresh_rate = 120;
  mxconfig.clkphase = bc.reversed;

  uint8_t chainLength = constrain(bc.pins[2], 1, 16); // number of chained panels
  // pre-calcualte rows and columns based on chain length
  uint8_t _rows, _cols;
  // possible combinations: (simple, horizontal) 1x1, 2x1, 3x1, 4x1, (complex & vertical) 2x2=5, 3x2, 4x2, 3x3, 4x3, 1x2=13, 1x3=14, 1x4=15, 4x4
  if      (chainLength <   5) { _rows = 1; _cols = chainLength; }       // 1 to 4 panels in a single row
  else if (chainLength <   9) { _rows = 2; _cols = chainLength / 2; }   // 7 does not exist and 5 is rounded down for 2(x2)
  else if (chainLength <  13) { _rows = 3; _cols = chainLength / 3; }   // 10 & 11 do not exist
  else if (chainLength <  16) { _rows = chainLength - 11; _cols = 1; }  // hack for 1x2, 1x3, 1x4 vertical panels
  else if (chainLength >= 16) { _rows = 4; _cols = 4; }
  mxconfig.chain_length = _rows * _cols;  // allows chaining multiple panels

  mxconfig.mx_width = dim[0];   // panel width in pixels
  mxconfig.mx_height = dim[1];  // panel height in pixels
  if (isOffRefreshRequired()) { // we reuse off refresh flag for quarter-scan panels
    mxconfig.mx_width = dim[0] << 1;  // panel width in pixels for quarter-scan is double
    mxconfig.mx_height = dim[1] >> 1; // panel height in pixels for quarter-scan is half
  }

  // check for too many pixels & reduce panel count if necessary
  // ESP32: MAX_LEDS (8192) will consume 32k for strip LED buffer + 32k for (1) segment buffer + 8k for dirty bits = 72k RAM!!!
  // S3: MAX_LEDS (16384) will consume 64k for strip LED buffer + 64k for (1) segment buffer + 16k for dirty bits = 144k RAM!!!
  // S2: MAX_LEDS (2048) will consume 8k for strip LED buffer + 8k for (1) segment buffer + 1k for dirty bits = 17k RAM!!!
  // all will also need driver's internal buffers (12-bit, 8-bit, 4-bit or 3-bit depth)
  while (mxconfig.mx_height * mxconfig.mx_width * mxconfig.chain_length > MAX_LEDS) {
    mxconfig.chain_length--;
    if (mxconfig.chain_length == 10 || mxconfig.chain_length == 11) mxconfig.chain_length = 9; // skip non-existing 10 & 11
    if (mxconfig.chain_length == 7) mxconfig.chain_length--;
  }
  if (mxconfig.chain_length == 0) {
    DEBUGBUS_PRINTLN("No panels to drive (too large panel?)");
    return;
  }

  if (mxconfig.getPixelColorDepthBits() != 8) mxconfig.setPixelColorDepthBits(8); // this is the default
#if defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32S2)// classic esp32, or esp32-s2: reduce bitdepth for large panels
  if (mxconfig.mx_height >= 64) {
    if      (mxconfig.chain_length * mxconfig.mx_width > 192) mxconfig.setPixelColorDepthBits(3);
    else if (mxconfig.chain_length * mxconfig.mx_width > 64)  mxconfig.setPixelColorDepthBits(4);
  }
#endif

  switch (_type) {
    case TYPE_HUB75MATRIX_FORUM:
    case TYPE_HUB75MATRIX_PORTAL:
    case TYPE_HUB75MATRIX_MOONHUB:
    case TYPE_HUB75MATRIX_TRINITY:
    case TYPE_HUB75MATRIX_S3:
      getHub75Pins(_type, (uint8_t*)&mxconfig.gpio);
      break;
    default:
      DEBUGBUS_PRINTLN(F("Unknown HUB75 matrix type. Aborting!"));
      return;
  }
  //PinManagerPinType pins[HUB75_PIN_COUNT];
  //for (size_t i = 0; i < HUB75_PIN_COUNT; i++) pins[i] = {((int8_t*)&mxconfig.gpio)[i], true};
  if (!PinManager::allocateMultiplePins((int8_t*)&mxconfig.gpio, HUB75_PIN_COUNT, PinOwner::HUB75, true)) {
    DEBUGBUS_PRINTLN("Failed to allocate pins for HUB75");
    return;
  }

  switch (bc.colorOrder) {
    case COL_ORDER_BGR:
      std::swap(mxconfig.gpio.r1, mxconfig.gpio.b1);
      std::swap(mxconfig.gpio.r2, mxconfig.gpio.b2);
      break;
    case COL_ORDER_GRB:
      std::swap(mxconfig.gpio.r1, mxconfig.gpio.g1);
      std::swap(mxconfig.gpio.r2, mxconfig.gpio.g2);
      break;
    case COL_ORDER_GBR:
      std::swap(mxconfig.gpio.r1, mxconfig.gpio.b1);
      std::swap(mxconfig.gpio.r2, mxconfig.gpio.b2);
      std::swap(mxconfig.gpio.r1, mxconfig.gpio.g1);
      std::swap(mxconfig.gpio.r2, mxconfig.gpio.g2);
      break;
    case COL_ORDER_RBG:
      std::swap(mxconfig.gpio.g1, mxconfig.gpio.b1);
      std::swap(mxconfig.gpio.g2, mxconfig.gpio.b2);
      break;
    case COL_ORDER_BRG:
      std::swap(mxconfig.gpio.r1, mxconfig.gpio.g1);
      std::swap(mxconfig.gpio.r2, mxconfig.gpio.g2);
      std::swap(mxconfig.gpio.r1, mxconfig.gpio.b1);
      std::swap(mxconfig.gpio.r2, mxconfig.gpio.b2);
      break;
    case COL_ORDER_RGB:
    default:
      break;
  }

  DEBUGBUS_PRINTF_P(PSTR("MatrixPanel_I2S_DMA config - %ux%u length: %u\n"), mxconfig.mx_width, mxconfig.mx_height, mxconfig.chain_length);
  DEBUGBUS_PRINTF_P(PSTR("R1_PIN=%u, G1_PIN=%u, B1_PIN=%u, R2_PIN=%u, G2_PIN=%u, B2_PIN=%u, A_PIN=%u, B_PIN=%u, C_PIN=%u, D_PIN=%u, E_PIN=%u, LAT_PIN=%u, OE_PIN=%u, CLK_PIN=%u\n"),
                mxconfig.gpio.r1, mxconfig.gpio.g1, mxconfig.gpio.b1, mxconfig.gpio.r2, mxconfig.gpio.g2, mxconfig.gpio.b2,
                mxconfig.gpio.a, mxconfig.gpio.b, mxconfig.gpio.c, mxconfig.gpio.d, mxconfig.gpio.e, mxconfig.gpio.lat, mxconfig.gpio.oe, mxconfig.gpio.clk);

  // OK, now we can create our matrix object
  display = new(std::nothrow) MatrixPanel_I2S_DMA(mxconfig);
  if (display == nullptr) {
    DEBUGBUS_PRINTLN(F("*** MatrixPanel_I2S_DMA !KABOOM! driver object allocation failed ***"));
    DEBUGBUS_PRINTF_P(PSTR("heap usage: %u\n"), lastHeap - getFreeHeapSize());
    cleanup(); // free allocated pins
    return;
  }

  // for quad-scan panels or 2 or more rows we create a virtual panel that maps to the physical one
  if (_rows > 1 || isOffRefreshRequired()) {  // quarter-scan panels need virtual panel (hijack off-refresh)
    PANEL_CHAIN_TYPE chainType = CHAIN_NONE;  // default for quarter-scan panels that do not use chaining
    if (_rows > 1 || _cols > 1) chainType = CHAIN_BOTTOM_LEFT_UP; // CHAIN_TOP_RIGHT_DOWN might be more natural fit
    virtualDisp = new(std::nothrow) VirtualMatrixPanel((*display), _rows, _cols, dim[0], dim[1], chainType);
    if (virtualDisp) {
      virtualDisp->setRotation(0);
      // adjust scan rate based on height
      switch (bc.pins[1]) {
        case 16:
          virtualDisp->setPhysicalPanelScanRate(FOUR_SCAN_16PX_HIGH);
          break;
        default:
          DEBUGBUS_PRINTLN(F("Unsupported height"));
          // fallthrough and use 32px
        case 32:
          virtualDisp->setPhysicalPanelScanRate(FOUR_SCAN_32PX_HIGH);
          break;
        case 64:
          virtualDisp->setPhysicalPanelScanRate(FOUR_SCAN_64PX_HIGH);
          break;
      }
    }
  }

  DEBUGBUS_PRINTLN(F("MatrixPanel_I2S_DMA created"));
  DEBUGBUS_PRINTF_P(PSTR("heap usage: %u\n"), lastHeap - getFreeHeapSize());
  DEBUGBUS_PRINTF_P(PSTR("Hub75 Length: %u\n"), _len);

  // let's adjust default brightness (using hardware)
  display->setBrightness(0);    // range is 0-255, 0 - 0%, 255 - 100%
  delay(24); // experimental

  // Allocate memory and start DMA display
  if (!display->begin()) {
    DEBUGBUS_PRINTLN(F("*** MatrixPanel_I2S_DMA !KABOOM! I2S memory buffer allocation failed ***"));
    DEBUGBUS_PRINTF_P(PSTR("heap usage: %u\n"), lastHeap - getFreeHeapSize());
    cleanup();  // free allocated pins and display object
    return;
  } else {
    DEBUGBUS_PRINTLN(F("MatrixPanel_I2S_DMA begin ok"));
    DEBUGBUS_PRINTF_P(PSTR("heap usage: %u\n"), lastHeap - getFreeHeapSize());
    delay(18);  // experiment - give the driver a moment (~ one full frame @ 60hz) to settle

    //_ledsDirty = (byte*) allocate_buffer(getBitArrayBytes(_len), BFRALLOC_ENFORCE_DRAM | BFRALLOC_CLEAR); // create LEDs dirty bits
    //if (_ledsDirty == nullptr) {
    //  cleanup();
    //  DEBUGBUS_PRINTLN(F("MatrixPanel_I2S_DMA not started - not enough memory for dirty bits!"));
    //  DEBUGBUS_PRINTF_P(PSTR("heap usage: %u\n"), lastHeap - getFreeHeapSize());
    //  return;  //  fail if we cannot get memory for the buffer
    //}
    //DEBUGBUS_PRINTLN(F("BusHub75Matrix LEDs dirty bit optimization enabled."));
    //DEBUGBUS_PRINTF_P(PSTR("BusHub75Matrix LED buffers use %u bytes.\n"), getBitArrayBytes(_len));

    display->clearScreen();   // initially clear the screen buffer
    DEBUGBUS_PRINTLN(F("MatrixPanel_I2S_DMA clear ok"));

    _matrixWidth = virtualDisp ? virtualDisp->width() : display->width();  // cache width - it will never change
    DEBUGBUS_PRINTF_P(PSTR("MatrixPanel_I2S_DMA %sstarted, width=%u, %u pixels.\n"), _valid? "":"not ", _matrixWidth, _len);
    _valid = true;
  }

}

void BusHub75Matrix::setPixelColor(unsigned pix, uint32_t c) {
  if (!_valid) return;
  if (_cct >= 1900) c = colorBalanceFromKelvin(_cct, c);  //color correction from CCT

  // dirty bit optimization might not be necessary as each pixel is only update once per frame (see WS2812FX::show())
  //if (c && !getBitFromArray(_ledsDirty, pix)) return;     // ignore black if pixel is already black
  //setBitInArray(_ledsDirty, pix, (bool)c);                // dirty = true means "color is not BLACK"

  uint8_t r = R(c);
  uint8_t g = G(c);
  uint8_t b = B(c);
  int16_t x = pix % _matrixWidth;
  int16_t y = pix / _matrixWidth;
  if (virtualDisp) virtualDisp->drawPixelRGB888(x, y, r, g, b);
  else display->drawPixelRGB888(x, y, r, g, b);
}

void BusHub75Matrix::setBrightness(uint8_t b) {
  _bri = b;
  if (!_valid) return;
  display->setBrightness(_bri);
}

void BusHub75Matrix::show(void) {
  if (!_valid) return;
  if (display->getCfg().double_buff) { // double buffering enabled
    display->flipDMABuffer();
    display->clearScreen();
  }
}

void BusHub75Matrix::cleanup() {
  _valid = false;
  if (display) {
    display->stopDMAoutput();  // terminate DMA driver (display goes black)
    DEBUGBUS_PRINTLN("HUB75 output ended.");
    delay(30); // give some time to settle
    if (virtualDisp) delete virtualDisp;
    virtualDisp = nullptr;
    delete display;
    display = nullptr;
  }
  deallocatePins();
  //free(_ledsDirty); // no need to check for nullptr
  //_ledsDirty = nullptr;
}

void BusHub75Matrix::deallocatePins() {
  uint8_t pins[HUB75_PIN_COUNT];
  getHub75Pins(_type, pins);
  PinManager::deallocateMultiplePins(pins, HUB75_PIN_COUNT, PinOwner::HUB75);
}

uint16_t BusHub75Matrix::getFrequency() const { 
  return (uint16_t)(display ? (unsigned)display->getCfg().i2sspeed / 1000 : 0);
}

size_t BusHub75Matrix::getPins(uint8_t* pinArray) const {
  if (pinArray) {
    const HUB75_I2S_CFG &mxconfig = display->getCfg();
    uint8_t chainLength = mxconfig.chain_length;
    // adjust for hack used in UI
    if (virtualDisp != nullptr) {
      // using complex display arrangement (vertical or multiple rows/columns)
      if (mxconfig.chain_length <= 4) chainLength += 11;  // 1x2, 1x3, 1x4 arrangements
      if (mxconfig.chain_length == 4 && virtualDisp->width() == virtualDisp->height()) chainLength = 5; // 2x2 arrangement
    }
    pinArray[0] = mxconfig.mx_width;  // 16-128
    pinArray[1] = mxconfig.mx_height; // 16-64
    pinArray[2] = chainLength;        // 1-16 (invalid values: 7, 10, 11)
    pinArray[3] = (uint8_t)mxconfig.driver;
    pinArray[4] = 255;                // reserved
    getHub75Pins(_type, &pinArray[5]);// ignoreable extension
  }
  return 5 + HUB75_PIN_COUNT;
}

std::vector<LEDType> BusHub75Matrix::getLEDTypes() {
  std::vector<LEDType> types = {
    {TYPE_HUB75MATRIX_PORTAL,  "H", PSTR("HUB75 (Adafruit Matrix Portal)")},
    {TYPE_HUB75MATRIX_MOONHUB, "H", PSTR("HUB75 (Moonhub T7 S3)")},
    {TYPE_HUB75MATRIX_S3,      "H", PSTR("HUB75 (S3 with PSRAM)")},
    {TYPE_HUB75MATRIX_TRINITY, "H", PSTR("HUB75 (Trinity/ElectroDragon)")},
    {TYPE_HUB75MATRIX_FORUM,   "H", PSTR("HUB75 (ESP32 Forum Pinout)")}
  };
  for (auto &t : types) {
    t.requiredPins.resize(HUB75_PIN_COUNT);
    getHub75Pins(t.id, &t.requiredPins[0]);  // vector behaves like an array
    // if the above does not work, try this:
    //const uint8_t * const pins = getHub75Pins(t.id);
    //for (size_t i = 0; i < HUB75_PIN_COUNT; i++) t.requiredPins[i] = pgm_read_byte_near(&pins[i]);
  }
  return types;
}
#endif // WLED_ENABLE_HUB75MATRIX


//utility to get the approx. memory usage of a given BusConfig
size_t BusConfig::memUsage(unsigned nr) const {
  if (Bus::isVirtual(type)) {
    return sizeof(BusNetwork) + (count * Bus::getNumberOfChannels(type));
  } else if (Bus::isDigital(type)) {
    // if any of digital buses uses I2S, there is additional common I2S DMA buffer not accounted for here
    return sizeof(BusDigital) + PolyBus::memUsage(count + skipAmount, PolyBus::getI(type, pins, nr));
  } else if (Bus::isOnOff(type)) {
    return sizeof(BusOnOff);
  } else {
    return sizeof(BusPwm);
  }
}


size_t BusManager::memUsage() {
  // when ESP32, S2 & S3 use parallel I2S only the largest bus determines the total memory requirements for back buffers
  // front buffers are always allocated per bus
  unsigned size = 0;
  unsigned maxI2S = 0;
  #if !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(ESP8266)
  unsigned digitalCount = 0;
  #endif
  for (const auto &bus : busses) {
    size += bus->getBusSize();
    #if !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(ESP8266)
    if (bus->isDigital() && !bus->is2Pin()) {
      digitalCount++;
      if ((PolyBus::isParallelI2S1Output() && digitalCount <= 8) || (!PolyBus::isParallelI2S1Output() && digitalCount == 1)) {
        #ifdef NPB_CONF_4STEP_CADENCE
        constexpr unsigned stepFactor = 4; // 4 step cadence (4 bits per pixel bit)
        #else
        constexpr unsigned stepFactor = 3; // 3 step cadence (3 bits per pixel bit)
        #endif
        unsigned i2sCommonSize = stepFactor * bus->getLength() * bus->getNumberOfChannels() * (bus->is16bit()+1);
        if (i2sCommonSize > maxI2S) maxI2S = i2sCommonSize;
      }
    }
    #endif
  }
  return size + maxI2S;
}

int BusManager::add(const BusConfig &bc) {
  DEBUGBUS_PRINTF_P(PSTR("Bus: Adding bus (p:%d v:%d)\n"), getNumBusses(), getNumVirtualBusses());
  #if defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32S2)
  const unsigned maxDigital = WLED_MAX_RMT_CHANNELS + (PolyBus::isParallelI2S1Output() ? WLED_MAX_DIGITAL_CHANNELS - WLED_MAX_RMT_CHANNELS : 1);
  #elif defined(CONFIG_IDF_TARGET_ESP32S3)
  const unsigned maxDigital = WLED_MAX_RMT_CHANNELS + (PolyBus::isParallelI2S1Output() ? WLED_MAX_DIGITAL_CHANNELS - WLED_MAX_RMT_CHANNELS : 0);
  #else
  const unsigned maxDigital = WLED_MAX_DIGITAL_CHANNELS; // ESP8266 and ESP32-C3
  #endif
  unsigned digital = 0;
  unsigned analog  = 0;
  unsigned twoPin  = 0;
  unsigned hub75   = 0;
  for (const auto &bus : busses) {
    if (bus->isPWM()) analog += bus->getPins(); // number of analog channels used
    if (bus->isDigital() && !bus->is2Pin()) digital++;
    if (bus->is2Pin()) twoPin++;
    if (bus->isHub75()) hub75++;
  }
  if (Bus::isVirtual(bc.type)) {
    busses.push_back(make_unique<BusNetwork>(bc));
  #ifdef WLED_ENABLE_HUB75MATRIX
  } else if (Bus::isHub75(bc.type)) {
    if (hub75 > 0) return -1; // only one HUB75 matrix bus allowed
    busses.push_back(make_unique<BusHub75Matrix>(bc));
  #endif
  } else if (Bus::isDigital(bc.type)) {
    if (digital >= maxDigital && !Bus::is2Pin(bc.type)) return -1; // too many digital channels used
    busses.push_back(make_unique<BusDigital>(bc, Bus::is2Pin(bc.type) ? twoPin : digital));
  } else if (Bus::isOnOff(bc.type)) {
    busses.push_back(make_unique<BusOnOff>(bc));
  } else {
    if (analog >= WLED_MAX_ANALOG_CHANNELS) return -1;
    busses.push_back(make_unique<BusPwm>(bc));
  }
  return busses.back()->isOk() ? busses.size() : -1;
}

// credit @willmmiles
static String LEDTypesToJson(const std::vector<LEDType>& types) {
  String json;
  for (const auto &type : types) {
    // capabilities follows similar pattern as JSON API
    int capabilities = Bus::hasRGB(type.id) | Bus::hasWhite(type.id)<<1 | Bus::hasCCT(type.id)<<2 | Bus::is16bit(type.id)<<4 | Bus::mustRefresh(type.id)<<5;
    char str[256];
    sprintf_P(str, PSTR("{i:%d,c:%d,t:\"%s\",n:\"%s\""), type.id, capabilities, type.type, type.name);
    if (type.requiredPins.size() > 0) {
      strcat_P(str, PSTR(",p:["));
      for (const auto &p : type.requiredPins) {
        char pinStr[8];
        sprintf_P(pinStr, PSTR("%d,"), p);
        strcat(str, pinStr);
      }
      str[strlen(str)-1] = ']'; // replace last comma with bracket
    }
    strcat_P(str, PSTR("},"));
    json += str;
  }
  return json;
}

// credit @willmmiles & @netmindz https://github.com/wled/WLED/pull/4056
String BusManager::getLEDTypesJSONString() {
  String json = "[";
  json += LEDTypesToJson(BusDigital::getLEDTypes());
  json += LEDTypesToJson(BusOnOff::getLEDTypes());
  json += LEDTypesToJson(BusPwm::getLEDTypes());
  #ifdef WLED_ENABLE_HUB75MATRIX
  json += LEDTypesToJson(BusHub75Matrix::getLEDTypes());
  #endif
  json += LEDTypesToJson(BusNetwork::getLEDTypes());
  //json += LEDTypesToJson(BusVirtual::getLEDTypes());
  json.setCharAt(json.length()-1, ']'); // replace last comma with bracket
  return json;
}

void BusManager::useParallelOutput() {
  DEBUGBUS_PRINTLN(F("Bus: Enabling parallel I2S."));
  PolyBus::setParallelI2S1Output();
}

bool BusManager::hasParallelOutput() {
  return PolyBus::isParallelI2S1Output();
}

//do not call this method from system context (network callback)
void BusManager::removeAll() {
  DEBUGBUS_PRINTLN(F("Removing all."));
  //prevents crashes due to deleting busses while in use.
  while (!canAllShow()) yield();
  busses.clear();
  PolyBus::setParallelI2S1Output(false);
}

#ifdef ESP32_DATA_IDLE_HIGH
// #2478
// If enabled, RMT idle level is set to HIGH when off
// to prevent leakage current when using an N-channel MOSFET to toggle LED power
void BusManager::esp32RMTInvertIdle() {
  bool idle_out;
  unsigned rmt = 0;
  unsigned u = 0;
  for (auto &bus : busses) {
    if (bus->getLength()==0 || !bus->isDigital() || bus->is2Pin()) continue;
    #if defined(CONFIG_IDF_TARGET_ESP32C3)    // 2 RMT, only has 1 I2S but NPB does not support it ATM
      if (u > 1) return;
      rmt = u;
    #elif defined(CONFIG_IDF_TARGET_ESP32S2)  // 4 RMT, only has 1 I2S bus, supported in NPB
      if (u > 3) return;
      rmt = u;
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)  // 4 RMT, has 2 I2S but NPB does not support them ATM
      if (u > 3) return;
      rmt = u;
    #else
      unsigned numI2S = !PolyBus::isParallelI2S1Output(); // if using parallel I2S, RMT is used 1st
      if (numI2S > u) continue;
      if (u > 7 + numI2S) return;
      rmt = u - numI2S;
    #endif
    //assumes that bus number to rmt channel mapping stays 1:1
    rmt_channel_t ch = static_cast<rmt_channel_t>(rmt);
    rmt_idle_level_t lvl;
    rmt_get_idle_level(ch, &idle_out, &lvl);
    if (lvl == RMT_IDLE_LEVEL_HIGH) lvl = RMT_IDLE_LEVEL_LOW;
    else if (lvl == RMT_IDLE_LEVEL_LOW) lvl = RMT_IDLE_LEVEL_HIGH;
    else continue;
    rmt_set_idle_level(ch, idle_out, lvl);
    u++
  }
}
#endif

void BusManager::on() {
  #ifdef ESP8266
  //Fix for turning off onboard LED breaking bus
  if (PinManager::getPinOwner(LED_BUILTIN) == PinOwner::BusDigital) {
    for (auto &bus : busses) {
      uint8_t pins[2] = {255,255};
      if (bus->isDigital() && bus->getPins(pins)) {
        if (pins[0] == LED_BUILTIN || pins[1] == LED_BUILTIN) {
          BusDigital &b = static_cast<BusDigital&>(*bus);
          b.begin();
          break;
        }
      }
    }
  }
  #else
  for (auto &bus : busses) if (bus->isVirtual()) {
    // virtual/network bus should check for IP change if hostname is specified
    // otherwise there are no endpoints to force DNS resolution
    BusNetwork &b = static_cast<BusNetwork&>(*bus);
    b.resolveHostname();
  }
  #endif
  #ifdef ESP32_DATA_IDLE_HIGH
  esp32RMTInvertIdle();
  #endif
}

void BusManager::off() {
  #ifdef ESP8266
  // turn off built-in LED if strip is turned off
  // this will break digital bus so will need to be re-initialised on On
  if (PinManager::getPinOwner(LED_BUILTIN) == PinOwner::BusDigital) {
    for (const auto &bus : busses) if (bus->isOffRefreshRequired()) return;
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
  }
  #endif
  #ifdef ESP32_DATA_IDLE_HIGH
  esp32RMTInvertIdle();
  #endif
}

void BusManager::show() {
  _gMilliAmpsUsed = 0;
  for (auto &bus : busses) {
    bus->show();
    _gMilliAmpsUsed += bus->getUsedCurrent();
  }
  //DEBUGBUS_PRINTF_P(PSTR("Bus: Total current used: %u mA\n"), (unsigned)_gMilliAmpsUsed);
}

void BusManager::setPixelColor(unsigned pix, uint32_t c) {
  for (auto &bus : busses) {
    if (bus->containsPixel(pix)) bus->setPixelColor(pix - bus->getStart(), c);
  }
}

void BusManager::setSegmentCCT(int16_t cct, bool allowWBCorrection) {
  if (cct > 255) cct = 255;
  if (cct >= 0) {
    //if white balance correction allowed, save as kelvin value instead of 0-255
    if (allowWBCorrection) cct = 1900 + (cct << 5);
  } else cct = -1; // will use kelvin approximation from RGB
  Bus::setCCT(cct);
}

bool BusManager::canAllShow() {
  for (const auto &bus : busses) if (!bus->canShow()) return false;
  return true;
}

// per bus ABL initialization (idea by @dedehai)
// handles global ABL and per bus ABL
// gMilliAmpsMax = global ABL limit (0 = no global ABL)
// must be called after all busses have been added and configured
void BusManager::initializeABL(unsigned gMilliAmpsMax) {
  unsigned numABLbuses = 0;
  unsigned totalDemand = 0;
  for (const auto &bus : busses) {
    if (bus->isOk() && bus->isDigital() && bus->getLEDCurrent() > 0) {
      unsigned actualMilliampsPerLed = bus->getLEDCurrent() == 255 ? 12 : bus->getLEDCurrent(); // from testing an actual WS2815 strip
      numABLbuses++; // count ABL enabled buses
      totalDemand += bus->getLength() * actualMilliampsPerLed; // total demand of all ABL enabled buses
    }
  }
  if (numABLbuses == 0) return; // no ABL compatible bus found

  unsigned ESPshare = MA_FOR_ESP / numABLbuses; // share of ESP current per ABL bus
  // set bus brightness limit
  for (auto &bus : busses) {
    if (bus->isOk() && bus->isDigital() && bus->getLEDCurrent() > 0) {
      BusDigital &busd = static_cast<BusDigital&>(*bus);
      unsigned actualMilliampsPerLed = busd.getLEDCurrent() == 255 ? 12 : busd.getLEDCurrent(); // from testing an actual WS2815 strip
      unsigned busLength = busd.getLength();
      uint64_t busDemand = busLength * actualMilliampsPerLed; // must be uint64_t to prevent overflow when multiplied by gMilliAmpsMax
      // global/strip ABL; divide max current per bus according to bus demand
      unsigned fairAmount = gMilliAmpsMax > 0 ? busDemand * gMilliAmpsMax / totalDemand : busd.getMaxCurrent();
      if (fairAmount > ESPshare) fairAmount -= ESPshare;      // subtract ESP share from bus ABL budget
      busd.setCurrentLimit(fairAmount);
    }
  }
}

ColorOrderMap& BusManager::getColorOrderMap() { return _colorOrderMap; }


bool PolyBus::_useParallelI2S = false;

// Bus static member definition
int16_t Bus::_cct = -1;
uint8_t Bus::_cctBlend = 0;
uint8_t Bus::_gAWM = 255;

uint16_t BusDigital::_milliAmpsTotal = 0;

std::vector<std::unique_ptr<Bus>> BusManager::busses;
uint16_t BusManager::_gMilliAmpsUsed = 0;
