#pragma once
#ifndef BusWrapper_h
#define BusWrapper_h

//#define NPB_CONF_4STEP_CADENCE
#include "NeoPixelBus.h"

//Hardware SPI Pins
#define P_8266_HS_MOSI 13
#define P_8266_HS_CLK  14
#define P_32_HS_MOSI   13
#define P_32_HS_CLK    14
#define P_32_VS_MOSI   23
#define P_32_VS_CLK    18

//The dirty list of possible bus types. Quite a lot...
#define I_NONE 0
//ESP8266 RGB
#define I_8266_U0_NEO_3 1
#define I_8266_U1_NEO_3 2
#define I_8266_DM_NEO_3 3
//RGBW
#define I_8266_U0_NEO_4 5
#define I_8266_U1_NEO_4 6
#define I_8266_DM_NEO_4 7
//400Kbps
#define I_8266_U0_400_3 9
#define I_8266_U1_400_3 10
#define I_8266_DM_400_3 11
//TM1814 (RGBW)
#define I_8266_U0_TM1_4 13
#define I_8266_U1_TM1_4 14
#define I_8266_DM_TM1_4 15
//TM1829 (RGB)
#define I_8266_U0_TM2_3 17
#define I_8266_U1_TM2_3 18
#define I_8266_DM_TM2_3 19
//UCS8903 (RGB)
#define I_8266_U0_UCS_3 21
#define I_8266_U1_UCS_3 22
#define I_8266_DM_UCS_3 23
//UCS8904 (RGBW)
#define I_8266_U0_UCS_4 25
#define I_8266_U1_UCS_4 26
#define I_8266_DM_UCS_4 27
//FW1906 GRBCW
#define I_8266_U0_FW6_5 29
#define I_8266_U1_FW6_5 30
#define I_8266_DM_FW6_5 31
//ESP8266 APA106
#define I_8266_U0_APA106_3 33
#define I_8266_U1_APA106_3 34
#define I_8266_DM_APA106_3 35
//WS2805 (RGBCW)
#define I_8266_U0_2805_5 37
#define I_8266_U1_2805_5 38
#define I_8266_DM_2805_5 39
//TM1914 (RGB)
#define I_8266_U0_TM1914_3 41
#define I_8266_U1_TM1914_3 42
#define I_8266_DM_TM1914_3 43
//SM16825 (RGBCW)
#define I_8266_U0_SM16825_5 45
#define I_8266_U1_SM16825_5 46
#define I_8266_DM_SM16825_5 47
//WS281x dual chip (RGBWxx)
#define I_8266_U0_NEODUAL_4 49
#define I_8266_U1_NEODUAL_4 50
#define I_8266_DM_NEODUAL_4 51

/*** ESP32 Neopixel methods ***/
//RGB
#define I_32_RN_NEO_3 1
#define I_32_I2_NEO_3 2
//RGBW
#define I_32_RN_NEO_4 5
#define I_32_I2_NEO_4 6
//400Kbps
#define I_32_RN_400_3 9
#define I_32_I2_400_3 10
//TM1814 (RGBW)
#define I_32_RN_TM1_4 13
#define I_32_I2_TM1_4 14
//TM1829 (RGB)
#define I_32_RN_TM2_3 17
#define I_32_I2_TM2_3 18
//UCS8903 (RGB)
#define I_32_RN_UCS_3 21
#define I_32_I2_UCS_3 22
//UCS8904 (RGBW)
#define I_32_RN_UCS_4 25
#define I_32_I2_UCS_4 26
//FW1906 GRBCW
#define I_32_RN_FW6_5 29
#define I_32_I2_FW6_5 30
//APA106
#define I_32_RN_APA106_3 33
#define I_32_I2_APA106_3 34
//WS2805 (RGBCW)
#define I_32_RN_2805_5 37
#define I_32_I2_2805_5 38
//TM1914 (RGB)
#define I_32_RN_TM1914_3 41
#define I_32_I2_TM1914_3 42
//SM16825 (RGBCW)
#define I_32_RN_SM16825_5 45
#define I_32_I2_SM16825_5 46
//WS281X dual chip (RGBWxx)
#define I_32_RN_NEODUAL_4 49
#define I_32_I2_NEODUAL_4 50

//APA102
#define I_HS_DOT_3 101 //hardware SPI
#define I_SS_DOT_3 102 //soft SPI

//LPD8806
#define I_HS_LPD_3 103
#define I_SS_LPD_3 104

//WS2801
#define I_HS_WS1_3 105
#define I_SS_WS1_3 106

//P9813
#define I_HS_P98_3 107
#define I_SS_P98_3 108

//LPD6803
#define I_HS_LPO_3 109
#define I_SS_LPO_3 110

//HD108
#define I_HS_HD1_3 111
#define I_SS_HD1_3 112

// clever constructs to build class names from macro parameters; credit @willmmiles
// x reresents ESP variant (Esp32 or Esp8266)
// y represents the bus type (I2s1, RmtN, Uart0, Uart1, Dma, (deprecated) BitBang)
// z represents the LED protocol/type (Ws2812x, Ws2805, Sk6812, Tm1814, Tm1914, Tm1829, 800Kbps, 400Kbps, Apa106, etc) defined in NeoBits.h
// RMT has a few additional speeds (Ws2811, Tx1812, Gs1903)
// there are several aliases for methods/protocols (Ws2814=WS2805, Ws2813=Ws2812x, Ws2812d=Ws2812x, Ws2811=Ws2812x, Ws2816=Ws2812x, Ws2812=800Kbps, Lc8812=Sk6812, etc)
#define NeoMethod(x,y,z) Neo ## x ## y ## z ## Method
//example: NeoMethod(Esp32, I2s1, Ws2812x) will be NeoEsp32I2s1Ws2812xMethod
//example: NeoMethod(Esp32, RmtN, Sk812) will be NeoEsp32RmtNSk6812Method
//example: NeoMethod(Esp8266, Uart0, Ws2813) will be NeoEsp8266Uart0Ws2813Method

#define NeoBus(g,x,y,z) NeoPixelBus<Neo ## g ## Feature, NeoMethod(x,y,z)>
//Possible entries for g,x,y,z are taken from NeoPixelBus' Features and Methods
//example g: Grb, Grbw, Rgbw, Rgb, Rgbww, Grbww, Grbcw, Grb48, Grbw48, ... (see NeoColorFeatures.h)
//example x: Esp32, Esp8266 (see NeoMethods.h)
//example y: I2s1, RmtN, Uart0, Uart1 (see NeoMethods.h; S2 only has I2s0 and is wrapped in I2s1)
//example z: Ws2812x, Ws2813, Ws2805, Sk6812, Apa106, ... (see NeoMethods.h and NeoBits.h)
//example: NeoBus(Grb, Esp32, RmtN, Ws2812x) will be NeoPixelBus<NeoGrbFeature, NeoEsp32RmtNWs2812xMethod>
//example: NeoBus(Grbw, Esp32, I2s1, Sk6812) will be NeoPixelBus<NeoGrbwFeature, NeoEsp32I2s1Sk6812Method>

#define TwoPinBus(g,x) NeoPixelBus<g ## Feature, x ## Method>
//example: TwoPinBus(NeoGrb, Ws2801SpiHz) will be NeoPixelBus<NeoGrbFeature, Ws2801SpiHzMethod>

// C3: I2S0 and I2S1 methods not supported in NPB (has one I2S bus)
// S2: I2S0 methods supported (single & parallel), I2S1 methods not supported (S2 has one I2S bus)
// S3: I2S0 methods not supported, I2S1 supports LCD parallel methods (has two I2S buses)
// https://github.com/Makuna/NeoPixelBus/blob/b32f719e95ef3c35c46da5c99538017ef925c026/src/internal/Esp32_i2s.h#L4
// https://github.com/Makuna/NeoPixelBus/blob/b32f719e95ef3c35c46da5c99538017ef925c026/src/internal/NeoEsp32RmtMethod.h#L857
#ifdef ARDUINO_ARCH_ESP32
  #if defined(CONFIG_IDF_TARGET_ESP32S3)
    // S3 will always use LCD parallel output for I2S1
    #define I2s1 LcdX8
    #define I2s1X8 LcdX8
  #elif defined(CONFIG_IDF_TARGET_ESP32S2)
    // S2 will use I2S0
    #define I2s1 I2s0
    #define I2s1X8 I2s0X8
  #endif
  #ifdef WLED_USE_ETHERNET
    // fix for #2542 (by @BlackBird77)
    //APA102
    #define DotStarSpiHz DotStarEsp32HspiHz
    //WS2801, LPD8806, LPD6803, P9813 (replace default (VSPI) methods with HSPI)
    #define Ws2801SpiHzMethod Ws2801MethodBase<TwoWireHspiImple<SpiSpeedHz>>
    #define Lpd8806SpiHzMethod Lpd8806MethodBase<TwoWireHspiImple<SpiSpeedHz>>
    #define Lpd6803SpiHzMethod Lpd6803MethodBase<TwoWireHspiImple<SpiSpeedHz>>
    #define P9813SpiHzMethod P9813MethodBase<TwoWireHspiImple<SpiSpeedHz>>
  #endif
  // RMT driver selection (remove once NPB employs similar fix; prevents flickering on Xtensa platforms) credit @willmmiles
  #if !defined(WLED_USE_SHARED_RMT) && !defined(__riscv)
    #include <NeoEsp32RmtHIMethod.h>
    #define RmtN RmtHIN
  #endif
#endif

// 48bit & 64bit to 24bit & 32bit RGB(W) conversion
#define toRGBW32(c) (RGBW32((c>>40)&0xFF, (c>>24)&0xFF, (c>>8)&0xFF, (c>>56)&0xFF))
#define RGBW32(r,g,b,w) (uint32_t((byte(w) << 24) | (byte(r) << 16) | (byte(g) << 8) | (byte(b))))

//handles pointer type conversion for all possible bus types
class PolyBus {
  private:
    static bool _useParallelI2S;

  public:
    static inline void setParallelI2S1Output(bool b = true) { _useParallelI2S = b; }
    static inline bool isParallelI2S1Output(void) { return _useParallelI2S; }

  // initialize SPI bus speed for DotStar methods
  template <class T>
  static void beginDotStar(void* busPtr, int8_t sck, int8_t miso, int8_t mosi, int8_t ss, uint16_t clock_kHz /* 0 == use default */) {
    T dotStar_strip = static_cast<T>(busPtr);
    #ifdef ESP8266
    dotStar_strip->Begin();
    #else
    if (sck == -1 && mosi == -1) dotStar_strip->Begin();
    else                         dotStar_strip->Begin(sck, miso, mosi, ss);
    #endif
    if (clock_kHz) dotStar_strip->SetMethodSettings(NeoSpiSettings((uint32_t)clock_kHz*1000));
  }

  // Begin & initialize the PixelSettings for TM1814 strips.
  template <class T>
  static void beginTM1814(void* busPtr) {
    T tm1814_strip = static_cast<T>(busPtr);
    tm1814_strip->Begin();
    // Max current for each LED (22.5 mA).
    tm1814_strip->SetPixelSettings(NeoTm1814Settings(/*R*/225, /*G*/225, /*B*/225, /*W*/225));
  }

  template <class T>
  static void beginTM1914(void* busPtr) {
    T tm1914_strip = static_cast<T>(busPtr);
    tm1914_strip->Begin();
    tm1914_strip->SetPixelSettings(NeoTm1914Settings());  //NeoTm1914_Mode_DinFdinAutoSwitch, NeoTm1914_Mode_DinOnly, NeoTm1914_Mode_FdinOnly
  }

  static void begin(void* busPtr, uint8_t busType, uint8_t* pins, uint16_t clock_kHz /* only used by DotStar */) {
    switch (busType) {
      case I_NONE: break;
    #ifdef ESP8266
      case I_8266_U0_NEO_3: (static_cast<NeoBus(Grb, Esp8266, Uart0, Ws2813)*>(busPtr))->Begin(); break;
      case I_8266_U1_NEO_3: (static_cast<NeoBus(Grb, Esp8266, Uart1, Ws2813)*>(busPtr))->Begin(); break;
      case I_8266_DM_NEO_3: (static_cast<NeoBus(Grb, Esp8266, Dma, 800Kbps)*>(busPtr))->Begin(); break;
      case I_8266_U0_NEO_4: (static_cast<NeoBus(Grbw, Esp8266, Uart0, Ws2813)*>(busPtr))->Begin(); break;
      case I_8266_U1_NEO_4: (static_cast<NeoBus(Grbw, Esp8266, Uart1, Ws2813)*>(busPtr))->Begin(); break;
      case I_8266_DM_NEO_4: (static_cast<NeoBus(Grbw, Esp8266, Dma, 800Kbps)*>(busPtr))->Begin(); break;
      case I_8266_U0_400_3: (static_cast<NeoBus(Grb, Esp8266, Uart0, 400Kbps)*>(busPtr))->Begin(); break;
      case I_8266_U1_400_3: (static_cast<NeoBus(Grb, Esp8266, Uart1, 400Kbps)*>(busPtr))->Begin(); break;
      case I_8266_DM_400_3: (static_cast<NeoBus(Grb, Esp8266, Dma, 400Kbps)*>(busPtr))->Begin(); break;
      case I_8266_U0_TM1_4: beginTM1814<NeoBus(WrgbTm1814, Esp8266, Uart0, Tm1814)*>(busPtr); break;
      case I_8266_U1_TM1_4: beginTM1814<NeoBus(WrgbTm1814, Esp8266, Uart1, Tm1814)*>(busPtr); break;
      case I_8266_DM_TM1_4: beginTM1814<NeoBus(WrgbTm1814, Esp8266, Dma, Tm1814)*>(busPtr); break;
      case I_8266_U0_TM2_3: (static_cast<NeoBus(Brg, Esp8266, Uart0, Tm1829)*>(busPtr))->Begin(); break;
      case I_8266_U1_TM2_3: (static_cast<NeoBus(Brg, Esp8266, Uart1, Tm1829)*>(busPtr))->Begin(); break;
      case I_8266_DM_TM2_3: (static_cast<NeoBus(Brg, Esp8266, Dma, Tm1829)*>(busPtr))->Begin(); break;
      case I_8266_U0_UCS_3: (static_cast<NeoBus(RgbUcs8903, Esp8266, Uart0, Ws2813)*>(busPtr))->Begin(); break;
      case I_8266_U1_UCS_3: (static_cast<NeoBus(RgbUcs8903, Esp8266, Uart1, Ws2813)*>(busPtr))->Begin(); break;
      case I_8266_DM_UCS_3: (static_cast<NeoBus(RgbUcs8903, Esp8266, Dma, 800Kbps)*>(busPtr))->Begin(); break;
      case I_8266_U0_UCS_4: (static_cast<NeoBus(RgbwUcs8904, Esp8266, Uart0, Ws2813)*>(busPtr))->Begin(); break;
      case I_8266_U1_UCS_4: (static_cast<NeoBus(RgbwUcs8904, Esp8266, Uart1, Ws2813)*>(busPtr))->Begin(); break;
      case I_8266_DM_UCS_4: (static_cast<NeoBus(RgbwUcs8904, Esp8266, Dma, 800Kbps)*>(busPtr))->Begin(); break;
      case I_8266_U0_APA106_3: (static_cast<NeoBus(Rbg, Esp8266, Uart0, Apa106)*>(busPtr))->Begin(); break;
      case I_8266_U1_APA106_3: (static_cast<NeoBus(Rbg, Esp8266, Uart1, Apa106)*>(busPtr))->Begin(); break;
      case I_8266_DM_APA106_3: (static_cast<NeoBus(Rbg, Esp8266, Dma, Apa106)*>(busPtr))->Begin(); break;
      case I_8266_U0_FW6_5: (static_cast<NeoBus(Grbcwx, Esp8266, Uart0, Ws2813)*>(busPtr))->Begin(); break;
      case I_8266_U1_FW6_5: (static_cast<NeoBus(Grbcwx, Esp8266, Uart1, Ws2813)*>(busPtr))->Begin(); break;
      case I_8266_DM_FW6_5: (static_cast<NeoBus(Grbcwx, Esp8266, Dma, 800Kbps)*>(busPtr))->Begin(); break;
      case I_8266_U0_2805_5: (static_cast<NeoBus(Grbww, Esp8266, Uart0, Ws2805)*>(busPtr))->Begin(); break;
      case I_8266_U1_2805_5: (static_cast<NeoBus(Grbww, Esp8266, Uart1, Ws2805)*>(busPtr))->Begin(); break;
      case I_8266_DM_2805_5: (static_cast<NeoBus(Grbww, Esp8266, Dma, Ws2805)*>(busPtr))->Begin(); break;
      case I_8266_U0_TM1914_3: beginTM1914<NeoBus(RgbTm1914, Esp8266, Uart0, Tm1914)*>(busPtr); break;
      case I_8266_U1_TM1914_3: beginTM1914<NeoBus(RgbTm1914, Esp8266, Uart1, Tm1914)*>(busPtr); break;
      case I_8266_DM_TM1914_3: beginTM1914<NeoBus(RgbTm1914, Esp8266, Dma, Tm1914)*>(busPtr); break;
      case I_8266_U0_SM16825_5: (static_cast<NeoBus(RgbwcSm16825e, Esp8266, Uart0, Ws2813)*>(busPtr))->Begin(); break;
      case I_8266_U1_SM16825_5: (static_cast<NeoBus(RgbwcSm16825e, Esp8266, Uart1, Ws2813)*>(busPtr))->Begin(); break;
      case I_8266_DM_SM16825_5: (static_cast<NeoBus(RgbwcSm16825e, Esp8266, Dma, 800Kbps)*>(busPtr))->Begin(); break;
      case I_8266_U0_NEODUAL_4: (static_cast<NeoBus(Rgbwxx, Esp8266, Uart0, Ws2813)*>(busPtr))->Begin(); break;
      case I_8266_U1_NEODUAL_4: (static_cast<NeoBus(Rgbwxx, Esp8266, Uart1, Ws2813)*>(busPtr))->Begin(); break;
      case I_8266_DM_NEODUAL_4: (static_cast<NeoBus(Rgbwxx, Esp8266, Dma, 800Kbps)*>(busPtr))->Begin(); break;
      case I_HS_DOT_3: beginDotStar<TwoPinBus(DotStarBgr, DotStarSpiHz)*>(busPtr, -1, -1, -1, -1, clock_kHz); break;
      case I_HS_LPD_3: beginDotStar<TwoPinBus(Lpd8806Grb, Lpd8806SpiHz)*>(busPtr, -1, -1, -1, -1, clock_kHz); break;
      case I_HS_LPO_3: beginDotStar<TwoPinBus(Lpd6803Grb, Lpd6803SpiHz)*>(busPtr, -1, -1, -1, -1, clock_kHz); break;
      case I_HS_WS1_3: beginDotStar<TwoPinBus(NeoRbg, Ws2801SpiHz)*>(busPtr, -1, -1, -1, -1, clock_kHz); break;
      case I_HS_P98_3: beginDotStar<TwoPinBus(P9813Bgr, P9813SpiHz)*>(busPtr, -1, -1, -1, -1, clock_kHz); break;
      case I_HS_HD1_3: beginDotStar<TwoPinBus(NeoBgr48, Hd108SpiHz)*>(busPtr, -1, -1, -1, -1, clock_kHz); break;
    #endif
    #ifdef ARDUINO_ARCH_ESP32
      // RMT buses
      case I_32_RN_NEO_3: (static_cast<NeoBus(Grb, Esp32, RmtN, Ws2812x)*>(busPtr))->Begin(); break;
      case I_32_RN_NEO_4: (static_cast<NeoBus(Grbw, Esp32, RmtN, Sk6812)*>(busPtr))->Begin(); break;
      case I_32_RN_400_3: (static_cast<NeoBus(Grb, Esp32, RmtN, 400Kbps)*>(busPtr))->Begin(); break;
      case I_32_RN_TM1_4: beginTM1814<NeoBus(WrgbTm1814, Esp32, RmtN, Tm1814)*>(busPtr); break;
      case I_32_RN_TM2_3: (static_cast<NeoBus(Brg, Esp32, RmtN, Tm1829)*>(busPtr))->Begin(); break;
      case I_32_RN_UCS_3: (static_cast<NeoBus(RgbUcs8903, Esp32, RmtN, Ws2812x)*>(busPtr))->Begin(); break;
      case I_32_RN_UCS_4: (static_cast<NeoBus(RgbwUcs8904, Esp32, RmtN, Ws2812x)*>(busPtr))->Begin(); break;
      case I_32_RN_FW6_5: (static_cast<NeoBus(Grbcwx, Esp32, RmtN, Ws2812x)*>(busPtr))->Begin(); break;
      case I_32_RN_APA106_3: (static_cast<NeoBus(Grb, Esp32, RmtN, Apa106)*>(busPtr))->Begin(); break;
      case I_32_RN_2805_5: (static_cast<NeoBus(Grbww, Esp32, RmtN, Ws2805)*>(busPtr))->Begin(); break;
      case I_32_RN_TM1914_3: beginTM1914<NeoBus(GrbTm1914, Esp32, RmtN, Tm1914)*>(busPtr); break;
      case I_32_RN_SM16825_5: (static_cast<NeoBus(RgbcwSm16825e, Esp32, RmtN, Ws2812x)*>(busPtr))->Begin(); break;
      case I_32_RN_NEODUAL_4: (static_cast<NeoBus(Rgbwxx, Esp32, RmtN, Ws2812x)*>(busPtr))->Begin(); break;
      // I2S1 bus or parellel buses
      #ifndef CONFIG_IDF_TARGET_ESP32C3
      case I_32_I2_NEO_3: if (_useParallelI2S) (static_cast<NeoBus(Grb, Esp32, I2s1X8, Ws2812x)*>(busPtr))->Begin(); else (static_cast<NeoBus(Grb, Esp32, I2s1, Ws2812x)*>(busPtr))->Begin(); break;
      case I_32_I2_NEO_4: if (_useParallelI2S) (static_cast<NeoBus(Grbw, Esp32, I2s1X8, Sk6812)*>(busPtr))->Begin(); else (static_cast<NeoBus(Grbw, Esp32, I2s1, Sk6812)*>(busPtr))->Begin(); break;
      case I_32_I2_400_3: if (_useParallelI2S) (static_cast<NeoBus(Grb, Esp32, I2s1X8, 400Kbps)*>(busPtr))->Begin(); else (static_cast<NeoBus(Grb, Esp32, I2s1, 400Kbps)*>(busPtr))->Begin(); break;
      case I_32_I2_TM1_4: if (_useParallelI2S) beginTM1814<NeoBus(WrgbTm1814, Esp32, I2s1X8, Tm1814)*>(busPtr); else beginTM1814<NeoBus(WrgbTm1814, Esp32, I2s1, Tm1814)*>(busPtr); break;
      case I_32_I2_TM2_3: if (_useParallelI2S) (static_cast<NeoBus(Brg, Esp32, I2s1X8, Tm1829)*>(busPtr))->Begin(); else (static_cast<NeoBus(Brg, Esp32, I2s1, Tm1829)*>(busPtr))->Begin(); break;
      case I_32_I2_UCS_3: if (_useParallelI2S) (static_cast<NeoBus(RgbUcs8903, Esp32, I2s1X8, 800Kbps)*>(busPtr))->Begin(); else (static_cast<NeoBus(RgbUcs8903, Esp32, I2s1, 800Kbps)*>(busPtr))->Begin(); break;
      case I_32_I2_UCS_4: if (_useParallelI2S) (static_cast<NeoBus(RgbwUcs8904, Esp32, I2s1X8, 800Kbps)*>(busPtr))->Begin(); else (static_cast<NeoBus(RgbwUcs8904, Esp32, I2s1, 800Kbps)*>(busPtr))->Begin(); break;
      case I_32_I2_FW6_5: if (_useParallelI2S) (static_cast<NeoBus(Grbcwx, Esp32, I2s1X8, 800Kbps)*>(busPtr))->Begin(); else (static_cast<NeoBus(Grbcwx, Esp32, I2s1, 800Kbps)*>(busPtr))->Begin(); break;
      case I_32_I2_APA106_3: if (_useParallelI2S) (static_cast<NeoBus(Grb, Esp32, I2s1X8, Apa106)*>(busPtr))->Begin(); else (static_cast<NeoBus(Grb, Esp32, I2s1, Apa106)*>(busPtr))->Begin(); break;
      case I_32_I2_2805_5: if (_useParallelI2S) (static_cast<NeoBus(Grbww, Esp32, I2s1X8, Ws2805)*>(busPtr))->Begin(); else (static_cast<NeoBus(Grbww, Esp32, I2s1, Ws2805)*>(busPtr))->Begin(); break;
      case I_32_I2_TM1914_3: if (_useParallelI2S) beginTM1914<NeoBus(GrbTm1914, Esp32, I2s1X8, Tm1914)*>(busPtr); else beginTM1914<NeoBus(GrbTm1914, Esp32, I2s1, Tm1914)*>(busPtr); break;
      case I_32_I2_SM16825_5: if (_useParallelI2S) (static_cast<NeoBus(RgbcwSm16825e, Esp32, I2s1X8, Ws2812x)*>(busPtr))->Begin(); else (static_cast<NeoBus(RgbcwSm16825e, Esp32, I2s1, Ws2812x)*>(busPtr))->Begin(); break;
      case I_32_I2_NEODUAL_4: if (_useParallelI2S) (static_cast<NeoBus(Rgbwxx, Esp32, I2s1X8, Ws2812x)*>(busPtr))->Begin(); else (static_cast<NeoBus(Rgbwxx, Esp32, I2s1, Ws2812x)*>(busPtr))->Begin(); break;
      #endif
      // ESP32 can (and should, to avoid inadvertantly driving the chip select signal) specify the pins used for SPI, but only in begin()
      case I_HS_DOT_3: beginDotStar<TwoPinBus(DotStarBgr, DotStarSpiHz)*>(busPtr, pins[1], -1, pins[0], -1, clock_kHz); break;
      case I_HS_LPD_3: beginDotStar<TwoPinBus(Lpd8806Grb, Lpd8806SpiHz)*>(busPtr, pins[1], -1, pins[0], -1, clock_kHz); break;
      case I_HS_LPO_3: beginDotStar<TwoPinBus(Lpd6803Grb, Lpd6803SpiHz)*>(busPtr, pins[1], -1, pins[0], -1, clock_kHz); break;
      case I_HS_WS1_3: beginDotStar<TwoPinBus(NeoRbg, Ws2801SpiHz)*>(busPtr, pins[1], -1, pins[0], -1, clock_kHz); break;
      case I_HS_P98_3: beginDotStar<TwoPinBus(P9813Bgr, P9813SpiHz)*>(busPtr, pins[1], -1, pins[0], -1, clock_kHz); break;
      case I_HS_HD1_3: beginDotStar<TwoPinBus(NeoBgr48, Hd108SpiHz)*>(busPtr, pins[1], -1, pins[0], -1, clock_kHz); break;
    #endif
      case I_SS_DOT_3: (static_cast<TwoPinBus(DotStarBgr, DotStar)*>(busPtr))->Begin(); break;
      case I_SS_LPD_3: (static_cast<TwoPinBus(Lpd8806Grb, Lpd8806)*>(busPtr))->Begin(); break;
      case I_SS_LPO_3: (static_cast<TwoPinBus(Lpd6803Grb, Lpd6803)*>(busPtr))->Begin(); break;
      case I_SS_WS1_3: (static_cast<TwoPinBus(NeoRbg, Ws2801)*>(busPtr))->Begin(); break;
      case I_SS_P98_3: (static_cast<TwoPinBus(P9813Bgr, P9813)*>(busPtr))->Begin(); break;
      case I_SS_HD1_3: (static_cast<TwoPinBus(NeoBgr48, Hd108)*>(busPtr))->Begin(); break;
    }
  }

  static void* create(uint8_t busType, uint8_t* pins, uint16_t len, uint8_t channel) {
  #if defined(ARDUINO_ARCH_ESP32) && !defined(CONFIG_IDF_TARGET_ESP32C3)
    // NOTE: "channel" is only used on ESP32 (and its variants) for RMT channel allocation
    // since 0.15.0-b3 I2S1 is favoured for classic ESP32 and moved to position 0 (channel 0) so we need to subtract 1 for correct RMT allocation
    #if defined(CONFIG_IDF_TARGET_ESP32)
    if (channel > 0) {
      channel--;
      if (_useParallelI2S && channel > 6) channel -= 7; // accommodate I2S1 which is used as 1st bus
    }
    #elif defined(CONFIG_IDF_TARGET_ESP32S2)
    if (_useParallelI2S && channel > 7) channel -= 8; // accommodate I2S1 which is used as 1st bus
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
    if (_useParallelI2S && channel > 7) channel -= 8; // accommodate I2S1 which is used as 1st bus
    #endif
  #endif
    void* busPtr = nullptr;
    switch (busType) {
      case I_NONE: break;
    #ifdef ESP8266
      case I_8266_U0_NEO_3: busPtr = new NeoBus(Grb, Esp8266, Uart0, Ws2813)(len, pins[0]); break;
      case I_8266_U1_NEO_3: busPtr = new NeoBus(Grb, Esp8266, Uart1, Ws2813)(len, pins[0]); break;
      case I_8266_DM_NEO_3: busPtr = new NeoBus(Grb, Esp8266, Dma, 800Kbps)(len, pins[0]); break;
      case I_8266_U0_NEO_4: busPtr = new NeoBus(Grbw, Esp8266, Uart0, Ws2813)(len, pins[0]); break;
      case I_8266_U1_NEO_4: busPtr = new NeoBus(Grbw, Esp8266, Uart1, Ws2813)(len, pins[0]); break;
      case I_8266_DM_NEO_4: busPtr = new NeoBus(Grbw, Esp8266, Dma, 800Kbps)(len, pins[0]); break;
      case I_8266_U0_400_3: busPtr = new NeoBus(Grb, Esp8266, Uart0, 400Kbps)(len, pins[0]); break;
      case I_8266_U1_400_3: busPtr = new NeoBus(Grb, Esp8266, Uart1, 400Kbps)(len, pins[0]); break;
      case I_8266_DM_400_3: busPtr = new NeoBus(Grb, Esp8266, Dma, 400Kbps)(len, pins[0]); break;
      case I_8266_U0_TM1_4: busPtr = new NeoBus(WrgbTm1814, Esp8266, Uart0, Tm1814)(len, pins[0]); break;
      case I_8266_U1_TM1_4: busPtr = new NeoBus(WrgbTm1814, Esp8266, Uart1, Tm1814)(len, pins[0]); break;
      case I_8266_DM_TM1_4: busPtr = new NeoBus(WrgbTm1814, Esp8266, Dma, Tm1814)(len, pins[0]); break;
      case I_8266_U0_TM2_3: busPtr = new NeoBus(Brg, Esp8266, Uart0, Tm1829)(len, pins[0]); break;
      case I_8266_U1_TM2_3: busPtr = new NeoBus(Brg, Esp8266, Uart1, Tm1829)(len, pins[0]); break;
      case I_8266_DM_TM2_3: busPtr = new NeoBus(Brg, Esp8266, Dma, Tm1829)(len, pins[0]); break;
      case I_8266_U0_UCS_3: busPtr = new NeoBus(RgbUcs8903, Esp8266, Uart0, Ws2813)(len, pins[0]); break;
      case I_8266_U1_UCS_3: busPtr = new NeoBus(RgbUcs8903, Esp8266, Uart1, Ws2813)(len, pins[0]); break;
      case I_8266_DM_UCS_3: busPtr = new NeoBus(RgbUcs8903, Esp8266, Dma, 800Kbps)(len, pins[0]); break;
      case I_8266_U0_UCS_4: busPtr = new NeoBus(RgbwUcs8904, Esp8266, Uart0, Ws2813)(len, pins[0]); break;
      case I_8266_U1_UCS_4: busPtr = new NeoBus(RgbwUcs8904, Esp8266, Uart1, Ws2813)(len, pins[0]); break;
      case I_8266_DM_UCS_4: busPtr = new NeoBus(RgbwUcs8904, Esp8266, Dma, 800Kbps)(len, pins[0]); break;
      case I_8266_U0_APA106_3: busPtr = new NeoBus(Rbg, Esp8266, Uart0, Apa106)(len, pins[0]); break;
      case I_8266_U1_APA106_3: busPtr = new NeoBus(Rbg, Esp8266, Uart1, Apa106)(len, pins[0]); break;
      case I_8266_DM_APA106_3: busPtr = new NeoBus(Rbg, Esp8266, Dma, Apa106)(len, pins[0]); break;
      case I_8266_U0_FW6_5: busPtr = new NeoBus(Grbcwx, Esp8266, Uart0, Ws2813)(len, pins[0]); break;
      case I_8266_U1_FW6_5: busPtr = new NeoBus(Grbcwx, Esp8266, Uart1, Ws2813)(len, pins[0]); break;
      case I_8266_DM_FW6_5: busPtr = new NeoBus(Grbcwx, Esp8266, Dma, 800Kbps)(len, pins[0]); break;
      case I_8266_U0_2805_5: busPtr = new NeoBus(Grbww, Esp8266, Uart0, Ws2805)(len, pins[0]); break;
      case I_8266_U1_2805_5: busPtr = new NeoBus(Grbww, Esp8266, Uart1, Ws2805)(len, pins[0]); break;
      case I_8266_DM_2805_5: busPtr = new NeoBus(Grbww, Esp8266, Dma, Ws2805)(len, pins[0]); break;
      case I_8266_U0_TM1914_3: busPtr = new NeoBus(RgbTm1914, Esp8266, Uart0, Tm1914)(len, pins[0]); break;
      case I_8266_U1_TM1914_3: busPtr = new NeoBus(RgbTm1914, Esp8266, Uart1, Tm1914)(len, pins[0]); break;
      case I_8266_DM_TM1914_3: busPtr = new NeoBus(RgbTm1914, Esp8266, Dma, Tm1914)(len, pins[0]); break;
      case I_8266_U0_SM16825_5: busPtr = new NeoBus(RgbwcSm16825e, Esp8266, Uart0, Ws2813)(len, pins[0]); break;
      case I_8266_U1_SM16825_5: busPtr = new NeoBus(RgbwcSm16825e, Esp8266, Uart1, Ws2813)(len, pins[0]); break;
      case I_8266_DM_SM16825_5: busPtr = new NeoBus(RgbwcSm16825e, Esp8266, Dma, 800Kbps)(len, pins[0]); break;
      case I_8266_U0_NEODUAL_4: busPtr = new NeoBus(Rgbwxx, Esp8266, Uart0, Ws2813)(len, pins[0]); break;
      case I_8266_U1_NEODUAL_4: busPtr = new NeoBus(Rgbwxx, Esp8266, Uart1, Ws2813)(len, pins[0]); break;
      case I_8266_DM_NEODUAL_4: busPtr = new NeoBus(Rgbwxx, Esp8266, Dma, 800Kbps)(len, pins[0]); break;
    #endif
    #ifdef ARDUINO_ARCH_ESP32
      // RMT buses
      case I_32_RN_NEO_3: busPtr = new NeoBus(Grb, Esp32, RmtN, Ws2812x)(len, pins[0], (NeoBusChannel)channel); break;
      case I_32_RN_NEO_4: busPtr = new NeoBus(Grbw, Esp32, RmtN, Sk6812)(len, pins[0], (NeoBusChannel)channel); break;
      case I_32_RN_400_3: busPtr = new NeoBus(Grb, Esp32, RmtN, 400Kbps)(len, pins[0], (NeoBusChannel)channel); break;
      case I_32_RN_TM1_4: busPtr = new NeoBus(WrgbTm1814, Esp32, RmtN, Tm1814)(len, pins[0], (NeoBusChannel)channel); break;
      case I_32_RN_TM2_3: busPtr = new NeoBus(Brg, Esp32, RmtN, Tm1829)(len, pins[0], (NeoBusChannel)channel); break;
      case I_32_RN_UCS_3: busPtr = new NeoBus(RgbUcs8903, Esp32, RmtN, Ws2812x)(len, pins[0], (NeoBusChannel)channel); break;
      case I_32_RN_UCS_4: busPtr = new NeoBus(RgbwUcs8904, Esp32, RmtN, Ws2812x)(len, pins[0], (NeoBusChannel)channel); break;
      case I_32_RN_APA106_3: busPtr = new NeoBus(Grb, Esp32, RmtN, Apa106)(len, pins[0], (NeoBusChannel)channel); break;
      case I_32_RN_FW6_5: busPtr = new NeoBus(Grbcwx, Esp32, RmtN, Ws2812x)(len, pins[0], (NeoBusChannel)channel); break;
      case I_32_RN_2805_5: busPtr = new NeoBus(Grbww, Esp32, RmtN, Ws2805)(len, pins[0], (NeoBusChannel)channel); break;
      case I_32_RN_TM1914_3: busPtr = new NeoBus(GrbTm1914, Esp32, RmtN, Tm1914)(len, pins[0], (NeoBusChannel)channel); break;
      case I_32_RN_SM16825_5: busPtr = new NeoBus(RgbcwSm16825e, Esp32, RmtN, Ws2812x)(len, pins[0], (NeoBusChannel)channel); break;
      case I_32_RN_NEODUAL_4: busPtr = new NeoBus(Rgbwxx, Esp32, RmtN, Ws2812x)(len, pins[0], (NeoBusChannel)channel); break;
      // I2S1 bus or paralell buses
      #ifndef CONFIG_IDF_TARGET_ESP32C3
      case I_32_I2_NEO_3: if (_useParallelI2S) busPtr = new NeoBus(Grb, Esp32, I2s1X8, Ws2812x)(len, pins[0]); else busPtr = new NeoBus(Grb, Esp32, I2s1, Ws2812x)(len, pins[0]); break;
      case I_32_I2_NEO_4: if (_useParallelI2S) busPtr = new NeoBus(Grbw, Esp32, I2s1X8, Sk6812)(len, pins[0]); else busPtr = new NeoBus(Grbw, Esp32, I2s1, Sk6812)(len, pins[0]); break;
      case I_32_I2_400_3: if (_useParallelI2S) busPtr = new NeoBus(Grb, Esp32, I2s1X8, 400Kbps)(len, pins[0]); else busPtr = new NeoBus(Grb, Esp32, I2s1, 400Kbps)(len, pins[0]); break;
      case I_32_I2_TM1_4: if (_useParallelI2S) busPtr = new NeoBus(WrgbTm1814, Esp32, I2s1X8, Tm1814)(len, pins[0]); else busPtr = new NeoBus(WrgbTm1814, Esp32, I2s1, Tm1814)(len, pins[0]); break;
      case I_32_I2_TM2_3: if (_useParallelI2S) busPtr = new NeoBus(Brg, Esp32, I2s1X8, Tm1829)(len, pins[0]); else busPtr = new NeoBus(Brg, Esp32, I2s1, Tm1829)(len, pins[0]); break;
      case I_32_I2_UCS_3: if (_useParallelI2S) busPtr = new NeoBus(RgbUcs8903, Esp32, I2s1X8, 800Kbps)(len, pins[0]); else busPtr = new NeoBus(RgbUcs8903, Esp32, I2s1, 800Kbps)(len, pins[0]); break;
      case I_32_I2_UCS_4: if (_useParallelI2S) busPtr = new NeoBus(RgbwUcs8904, Esp32, I2s1X8, 800Kbps)(len, pins[0]); else busPtr = new NeoBus(RgbwUcs8904, Esp32, I2s1, 800Kbps)(len, pins[0]); break;
      case I_32_I2_APA106_3: if (_useParallelI2S) busPtr = new NeoBus(Grb, Esp32, I2s1X8, Apa106)(len, pins[0]); else busPtr = new NeoBus(Grb, Esp32, I2s1, Apa106)(len, pins[0]); break;
      case I_32_I2_FW6_5: if (_useParallelI2S) busPtr = new NeoBus(Grbcwx, Esp32, I2s1X8, 800Kbps)(len, pins[0]); else busPtr = new NeoBus(Grbcwx, Esp32, I2s1, 800Kbps)(len, pins[0]); break;
      case I_32_I2_2805_5: if (_useParallelI2S) busPtr = new NeoBus(Grbww, Esp32, I2s1X8, Ws2805)(len, pins[0]); else busPtr = new NeoBus(Grbww, Esp32, I2s1, Ws2805)(len, pins[0]); break;
      case I_32_I2_TM1914_3: if (_useParallelI2S) busPtr = new NeoBus(GrbTm1914, Esp32, I2s1X8, Tm1914)(len, pins[0]); else busPtr = new NeoBus(GrbTm1914, Esp32, I2s1, Tm1914)(len, pins[0]); break;
      case I_32_I2_SM16825_5: if (_useParallelI2S) busPtr = new NeoBus(RgbcwSm16825e, Esp32, I2s1X8, Ws2812x)(len, pins[0]); else busPtr = new NeoBus(RgbcwSm16825e, Esp32, I2s1, Ws2812x)(len, pins[0]); break;
      case I_32_I2_NEODUAL_4: if (_useParallelI2S) busPtr = new NeoBus(Rgbwxx, Esp32, I2s1X8, Ws2812x)(len, pins[0]); else busPtr = new NeoBus(Rgbwxx, Esp32, I2s1, Ws2812x)(len, pins[0]); break;
      #endif
    #endif
      // for 2-wire: pins[1] is clk, pins[0] is dat.  begin expects (len, clk, dat)
      case I_HS_DOT_3: busPtr = new TwoPinBus(DotStarBgr, DotStarSpiHz)(len, pins[1], pins[0]); break;
      case I_SS_DOT_3: busPtr = new TwoPinBus(DotStarBgr, DotStar)(len, pins[1], pins[0]); break;
      case I_HS_LPD_3: busPtr = new TwoPinBus(Lpd8806Grb, Lpd8806SpiHz)(len, pins[1], pins[0]); break;
      case I_SS_LPD_3: busPtr = new TwoPinBus(Lpd8806Grb, Lpd8806)(len, pins[1], pins[0]); break;
      case I_HS_LPO_3: busPtr = new TwoPinBus(Lpd6803Grb, Lpd6803SpiHz)(len, pins[1], pins[0]); break;
      case I_SS_LPO_3: busPtr = new TwoPinBus(Lpd6803Grb, Lpd6803)(len, pins[1], pins[0]); break;
      case I_HS_WS1_3: busPtr = new TwoPinBus(NeoRbg, Ws2801SpiHz)(len, pins[1], pins[0]); break;
      case I_SS_WS1_3: busPtr = new TwoPinBus(NeoRbg, Ws2801)(len, pins[1], pins[0]); break;
      case I_HS_P98_3: busPtr = new TwoPinBus(P9813Bgr, P9813SpiHz)(len, pins[1], pins[0]); break;
      case I_SS_P98_3: busPtr = new TwoPinBus(P9813Bgr, P9813)(len, pins[1], pins[0]); break;
      case I_HS_HD1_3: busPtr = new TwoPinBus(NeoBgr48, Hd108SpiHz)(len, pins[1], pins[0]); break;
      case I_SS_HD1_3: busPtr = new TwoPinBus(NeoBgr48, Hd108)(len, pins[1], pins[0]); break;
    }

    return busPtr;
  }

  static void show(void* busPtr, uint8_t busType, bool consistent = true) {
    switch (busType) {
      case I_NONE: break;
    #ifdef ESP8266
      case I_8266_U0_NEO_3: (static_cast<NeoBus(Grb, Esp8266, Uart0, Ws2813)*>(busPtr))->Show(consistent); break;
      case I_8266_U1_NEO_3: (static_cast<NeoBus(Grb, Esp8266, Uart1, Ws2813)*>(busPtr))->Show(consistent); break;
      case I_8266_DM_NEO_3: (static_cast<NeoBus(Grb, Esp8266, Dma, 800Kbps)*>(busPtr))->Show(consistent); break;
      case I_8266_U0_NEO_4: (static_cast<NeoBus(Grbw, Esp8266, Uart0, Ws2813)*>(busPtr))->Show(consistent); break;
      case I_8266_U1_NEO_4: (static_cast<NeoBus(Grbw, Esp8266, Uart1, Ws2813)*>(busPtr))->Show(consistent); break;
      case I_8266_DM_NEO_4: (static_cast<NeoBus(Grbw, Esp8266, Dma, 800Kbps)*>(busPtr))->Show(consistent); break;
      case I_8266_U0_400_3: (static_cast<NeoBus(Grb, Esp8266, Uart0, 400Kbps)*>(busPtr))->Show(consistent); break;
      case I_8266_U1_400_3: (static_cast<NeoBus(Grb, Esp8266, Uart1, 400Kbps)*>(busPtr))->Show(consistent); break;
      case I_8266_DM_400_3: (static_cast<NeoBus(Grb, Esp8266, Dma, 400Kbps)*>(busPtr))->Show(consistent); break;
      case I_8266_U0_TM1_4: (static_cast<NeoBus(WrgbTm1814, Esp8266, Uart0, Tm1814)*>(busPtr))->Show(consistent); break;
      case I_8266_U1_TM1_4: (static_cast<NeoBus(WrgbTm1814, Esp8266, Uart1, Tm1814)*>(busPtr))->Show(consistent); break;
      case I_8266_DM_TM1_4: (static_cast<NeoBus(WrgbTm1814, Esp8266, Dma, Tm1814)*>(busPtr))->Show(consistent); break;
      case I_8266_U0_TM2_3: (static_cast<NeoBus(Brg, Esp8266, Uart0, Tm1829)*>(busPtr))->Show(consistent); break;
      case I_8266_U1_TM2_3: (static_cast<NeoBus(Brg, Esp8266, Uart1, Tm1829)*>(busPtr))->Show(consistent); break;
      case I_8266_DM_TM2_3: (static_cast<NeoBus(Brg, Esp8266, Dma, Tm1829)*>(busPtr))->Show(consistent); break;
      case I_8266_U0_UCS_3: (static_cast<NeoBus(RgbUcs8903, Esp8266, Uart0, Ws2813)*>(busPtr))->Show(consistent); break;
      case I_8266_U1_UCS_3: (static_cast<NeoBus(RgbUcs8903, Esp8266, Uart1, Ws2813)*>(busPtr))->Show(consistent); break;
      case I_8266_DM_UCS_3: (static_cast<NeoBus(RgbUcs8903, Esp8266, Dma, 800Kbps)*>(busPtr))->Show(consistent); break;
      case I_8266_U0_UCS_4: (static_cast<NeoBus(RgbwUcs8904, Esp8266, Uart0, Ws2813)*>(busPtr))->Show(consistent); break;
      case I_8266_U1_UCS_4: (static_cast<NeoBus(RgbwUcs8904, Esp8266, Uart1, Ws2813)*>(busPtr))->Show(consistent); break;
      case I_8266_DM_UCS_4: (static_cast<NeoBus(RgbwUcs8904, Esp8266, Dma, 800Kbps)*>(busPtr))->Show(consistent); break;
      case I_8266_U0_APA106_3: (static_cast<NeoBus(Rbg, Esp8266, Uart0, Apa106)*>(busPtr))->Show(consistent); break;
      case I_8266_U1_APA106_3: (static_cast<NeoBus(Rbg, Esp8266, Uart1, Apa106)*>(busPtr))->Show(consistent); break;
      case I_8266_DM_APA106_3: (static_cast<NeoBus(Rbg, Esp8266, Dma, Apa106)*>(busPtr))->Show(consistent); break;
      case I_8266_U0_FW6_5: (static_cast<NeoBus(Grbcwx, Esp8266, Uart0, Ws2813)*>(busPtr))->Show(consistent); break;
      case I_8266_U1_FW6_5: (static_cast<NeoBus(Grbcwx, Esp8266, Uart1, Ws2813)*>(busPtr))->Show(consistent); break;
      case I_8266_DM_FW6_5: (static_cast<NeoBus(Grbcwx, Esp8266, Dma, 800Kbps)*>(busPtr))->Show(consistent); break;
      case I_8266_U0_2805_5: (static_cast<NeoBus(Grbww, Esp8266, Uart0, Ws2805)*>(busPtr))->Show(consistent); break;
      case I_8266_U1_2805_5: (static_cast<NeoBus(Grbww, Esp8266, Uart1, Ws2805)*>(busPtr))->Show(consistent); break;
      case I_8266_DM_2805_5: (static_cast<NeoBus(Grbww, Esp8266, Dma, Ws2805)*>(busPtr))->Show(consistent); break;
      case I_8266_U0_TM1914_3: (static_cast<NeoBus(RgbTm1914, Esp8266, Uart0, Tm1914)*>(busPtr))->Show(consistent); break;
      case I_8266_U1_TM1914_3: (static_cast<NeoBus(RgbTm1914, Esp8266, Uart1, Tm1914)*>(busPtr))->Show(consistent); break;
      case I_8266_DM_TM1914_3: (static_cast<NeoBus(RgbTm1914, Esp8266, Dma, Tm1914)*>(busPtr))->Show(consistent); break;
      case I_8266_U0_SM16825_5: (static_cast<NeoBus(RgbwcSm16825e, Esp8266, Uart0, Ws2813)*>(busPtr))->Show(consistent); break;
      case I_8266_U1_SM16825_5: (static_cast<NeoBus(RgbwcSm16825e, Esp8266, Uart1, Ws2813)*>(busPtr))->Show(consistent); break;
      case I_8266_DM_SM16825_5: (static_cast<NeoBus(RgbwcSm16825e, Esp8266, Dma, 800Kbps)*>(busPtr))->Show(consistent); break;
      case I_8266_U0_NEODUAL_4: (static_cast<NeoBus(Rgbwxx, Esp8266, Uart0, Ws2813)*>(busPtr))->Show(consistent); break;
      case I_8266_U1_NEODUAL_4: (static_cast<NeoBus(Rgbwxx, Esp8266, Uart1, Ws2813)*>(busPtr))->Show(consistent); break;
      case I_8266_DM_NEODUAL_4: (static_cast<NeoBus(Rgbwxx, Esp8266, Dma, 800Kbps)*>(busPtr))->Show(consistent); break;
    #endif
    #ifdef ARDUINO_ARCH_ESP32
      // RMT buses
      case I_32_RN_NEO_3: (static_cast<NeoBus(Grb, Esp32, RmtN, Ws2812x)*>(busPtr))->Show(consistent); break;
      case I_32_RN_NEO_4: (static_cast<NeoBus(Grbw, Esp32, RmtN, Sk6812)*>(busPtr))->Show(consistent); break;
      case I_32_RN_400_3: (static_cast<NeoBus(Grb, Esp32, RmtN, 400Kbps)*>(busPtr))->Show(consistent); break;
      case I_32_RN_TM1_4: (static_cast<NeoBus(WrgbTm1814, Esp32, RmtN, Tm1814)*>(busPtr))->Show(consistent); break;
      case I_32_RN_TM2_3: (static_cast<NeoBus(Brg, Esp32, RmtN, Tm1829)*>(busPtr))->Show(consistent); break;
      case I_32_RN_UCS_3: (static_cast<NeoBus(RgbUcs8903, Esp32, RmtN, Ws2812x)*>(busPtr))->Show(consistent); break;
      case I_32_RN_UCS_4: (static_cast<NeoBus(RgbwUcs8904, Esp32, RmtN, Ws2812x)*>(busPtr))->Show(consistent); break;
      case I_32_RN_APA106_3: (static_cast<NeoBus(Grb, Esp32, RmtN, Apa106)*>(busPtr))->Show(consistent); break;
      case I_32_RN_FW6_5: (static_cast<NeoBus(Grbcwx, Esp32, RmtN, Ws2812x)*>(busPtr))->Show(consistent); break;
      case I_32_RN_2805_5: (static_cast<NeoBus(Grbww, Esp32, RmtN, Ws2805)*>(busPtr))->Show(consistent); break;
      case I_32_RN_TM1914_3: (static_cast<NeoBus(GrbTm1914, Esp32, RmtN, Tm1914)*>(busPtr))->Show(consistent); break;
      case I_32_RN_SM16825_5: (static_cast<NeoBus(RgbcwSm16825e, Esp32, RmtN, Ws2812x)*>(busPtr))->Show(consistent); break;
      case I_32_RN_NEODUAL_4: (static_cast<NeoBus(Rgbwxx, Esp32, RmtN, Ws2812x)*>(busPtr))->Show(consistent); break;
      // I2S1 bus or paralell buses
      #ifndef CONFIG_IDF_TARGET_ESP32C3
      case I_32_I2_NEO_3: if (_useParallelI2S) (static_cast<NeoBus(Grb, Esp32, I2s1X8, Ws2812x)*>(busPtr))->Show(consistent); else (static_cast<NeoBus(Grb, Esp32, I2s1, Ws2812x)*>(busPtr))->Show(consistent); break;
      case I_32_I2_NEO_4: if (_useParallelI2S) (static_cast<NeoBus(Grbw, Esp32, I2s1X8, Sk6812)*>(busPtr))->Show(consistent); else (static_cast<NeoBus(Grbw, Esp32, I2s1, Sk6812)*>(busPtr))->Show(consistent); break;
      case I_32_I2_400_3: if (_useParallelI2S) (static_cast<NeoBus(Grb, Esp32, I2s1X8, 400Kbps)*>(busPtr))->Show(consistent); else (static_cast<NeoBus(Grb, Esp32, I2s1, 400Kbps)*>(busPtr))->Show(consistent); break;
      case I_32_I2_TM1_4: if (_useParallelI2S) (static_cast<NeoBus(WrgbTm1814, Esp32, I2s1X8, Tm1814)*>(busPtr))->Show(consistent); else (static_cast<NeoBus(WrgbTm1814, Esp32, I2s1, Tm1814)*>(busPtr))->Show(consistent); break;
      case I_32_I2_TM2_3: if (_useParallelI2S) (static_cast<NeoBus(Brg, Esp32, I2s1X8, Tm1829)*>(busPtr))->Show(consistent); else (static_cast<NeoBus(Brg, Esp32, I2s1, Tm1829)*>(busPtr))->Show(consistent); break;
      case I_32_I2_UCS_3: if (_useParallelI2S) (static_cast<NeoBus(RgbUcs8903, Esp32, I2s1X8, 800Kbps)*>(busPtr))->Show(consistent); else (static_cast<NeoBus(RgbUcs8903, Esp32, I2s1, 800Kbps)*>(busPtr))->Show(consistent); break;
      case I_32_I2_UCS_4: if (_useParallelI2S) (static_cast<NeoBus(RgbwUcs8904, Esp32, I2s1X8, 800Kbps)*>(busPtr))->Show(consistent); else (static_cast<NeoBus(RgbwUcs8904, Esp32, I2s1, 800Kbps)*>(busPtr))->Show(consistent); break;
      case I_32_I2_APA106_3: if (_useParallelI2S) (static_cast<NeoBus(Grb, Esp32, I2s1X8, Apa106)*>(busPtr))->Show(consistent); else (static_cast<NeoBus(Grb, Esp32, I2s1, Apa106)*>(busPtr))->Show(consistent); break;
      case I_32_I2_FW6_5: if (_useParallelI2S) (static_cast<NeoBus(Grbcwx, Esp32, I2s1X8, 800Kbps)*>(busPtr))->Show(consistent); else (static_cast<NeoBus(Grbcwx, Esp32, I2s1, 800Kbps)*>(busPtr))->Show(consistent); break;
      case I_32_I2_2805_5: if (_useParallelI2S) (static_cast<NeoBus(Grbww, Esp32, I2s1X8, Ws2805)*>(busPtr))->Show(consistent); else (static_cast<NeoBus(Grbww, Esp32, I2s1, Ws2805)*>(busPtr))->Show(consistent); break;
      case I_32_I2_TM1914_3: if (_useParallelI2S) (static_cast<NeoBus(GrbTm1914, Esp32, I2s1X8, Tm1914)*>(busPtr))->Show(consistent); else (static_cast<NeoBus(GrbTm1914, Esp32, I2s1, Tm1914)*>(busPtr))->Show(consistent); break;
      case I_32_I2_SM16825_5: if (_useParallelI2S) (static_cast<NeoBus(RgbcwSm16825e, Esp32, I2s1X8, Ws2812x)*>(busPtr))->Show(consistent); else (static_cast<NeoBus(RgbcwSm16825e, Esp32, I2s1, Ws2812x)*>(busPtr))->Show(consistent); break;
      case I_32_I2_NEODUAL_4: if (_useParallelI2S) (static_cast<NeoBus(Rgbwxx, Esp32, I2s1X8, Ws2812x)*>(busPtr))->Show(consistent); else (static_cast<NeoBus(Rgbwxx, Esp32, I2s1, Ws2812x)*>(busPtr))->Show(consistent); break;
      #endif
    #endif
      case I_HS_DOT_3: (static_cast<TwoPinBus(DotStarBgr, DotStarSpiHz)*>(busPtr))->Show(consistent); break;
      case I_SS_DOT_3: (static_cast<TwoPinBus(DotStarBgr, DotStar)*>(busPtr))->Show(consistent); break;
      case I_HS_LPD_3: (static_cast<TwoPinBus(Lpd8806Grb, Lpd8806SpiHz)*>(busPtr))->Show(consistent); break;
      case I_SS_LPD_3: (static_cast<TwoPinBus(Lpd8806Grb, Lpd8806)*>(busPtr))->Show(consistent); break;
      case I_HS_LPO_3: (static_cast<TwoPinBus(Lpd6803Grb, Lpd6803SpiHz)*>(busPtr))->Show(consistent); break;
      case I_SS_LPO_3: (static_cast<TwoPinBus(Lpd6803Grb, Lpd6803)*>(busPtr))->Show(consistent); break;
      case I_HS_WS1_3: (static_cast<TwoPinBus(NeoRbg, Ws2801SpiHz)*>(busPtr))->Show(consistent); break;
      case I_SS_WS1_3: (static_cast<TwoPinBus(NeoRbg, Ws2801)*>(busPtr))->Show(consistent); break;
      case I_HS_P98_3: (static_cast<TwoPinBus(P9813Bgr, P9813SpiHz)*>(busPtr))->Show(consistent); break;
      case I_SS_P98_3: (static_cast<TwoPinBus(P9813Bgr, P9813)*>(busPtr))->Show(consistent); break;
      case I_HS_HD1_3: (static_cast<TwoPinBus(NeoBgr48, Hd108SpiHz)*>(busPtr))->Show(consistent); break;
      case I_SS_HD1_3: (static_cast<TwoPinBus(NeoBgr48, Hd108)*>(busPtr))->Show(consistent); break;
    }
  }

  static bool canShow(void* busPtr, uint8_t busType) {
    switch (busType) {
      case I_NONE: return true;
    #ifdef ESP8266
      case I_8266_U0_NEO_3: return (static_cast<NeoBus(Grb, Esp8266, Uart0, Ws2813)*>(busPtr))->CanShow(); break;
      case I_8266_U1_NEO_3: return (static_cast<NeoBus(Grb, Esp8266, Uart1, Ws2813)*>(busPtr))->CanShow(); break;
      case I_8266_DM_NEO_3: return (static_cast<NeoBus(Grb, Esp8266, Dma, 800Kbps)*>(busPtr))->CanShow(); break;
      case I_8266_U0_NEO_4: return (static_cast<NeoBus(Grbw, Esp8266, Uart0, Ws2813)*>(busPtr))->CanShow(); break;
      case I_8266_U1_NEO_4: return (static_cast<NeoBus(Grbw, Esp8266, Uart1, Ws2813)*>(busPtr))->CanShow(); break;
      case I_8266_DM_NEO_4: return (static_cast<NeoBus(Grbw, Esp8266, Dma, 800Kbps)*>(busPtr))->CanShow(); break;
      case I_8266_U0_400_3: return (static_cast<NeoBus(Grb, Esp8266, Uart0, 400Kbps)*>(busPtr))->CanShow(); break;
      case I_8266_U1_400_3: return (static_cast<NeoBus(Grb, Esp8266, Uart1, 400Kbps)*>(busPtr))->CanShow(); break;
      case I_8266_DM_400_3: return (static_cast<NeoBus(Grb, Esp8266, Dma, 400Kbps)*>(busPtr))->CanShow(); break;
      case I_8266_U0_TM1_4: return (static_cast<NeoBus(WrgbTm1814, Esp8266, Uart0, Tm1814)*>(busPtr))->CanShow(); break;
      case I_8266_U1_TM1_4: return (static_cast<NeoBus(WrgbTm1814, Esp8266, Uart1, Tm1814)*>(busPtr))->CanShow(); break;
      case I_8266_DM_TM1_4: return (static_cast<NeoBus(WrgbTm1814, Esp8266, Dma, Tm1814)*>(busPtr))->CanShow(); break;
      case I_8266_U0_TM2_3: return (static_cast<NeoBus(Brg, Esp8266, Uart0, Tm1829)*>(busPtr))->CanShow(); break;
      case I_8266_U1_TM2_3: return (static_cast<NeoBus(Brg, Esp8266, Uart1, Tm1829)*>(busPtr))->CanShow(); break;
      case I_8266_DM_TM2_3: return (static_cast<NeoBus(Brg, Esp8266, Dma, Tm1829)*>(busPtr))->CanShow(); break;
      case I_8266_U0_UCS_3: return (static_cast<NeoBus(RgbUcs8903, Esp8266, Uart0, Ws2813)*>(busPtr))->CanShow(); break;
      case I_8266_U1_UCS_3: return (static_cast<NeoBus(RgbUcs8903, Esp8266, Uart1, Ws2813)*>(busPtr))->CanShow(); break;
      case I_8266_DM_UCS_3: return (static_cast<NeoBus(RgbUcs8903, Esp8266, Dma, 800Kbps)*>(busPtr))->CanShow(); break;
      case I_8266_U0_UCS_4: return (static_cast<NeoBus(RgbwUcs8904, Esp8266, Uart0, Ws2813)*>(busPtr))->CanShow(); break;
      case I_8266_U1_UCS_4: return (static_cast<NeoBus(RgbwUcs8904, Esp8266, Uart1, Ws2813)*>(busPtr))->CanShow(); break;
      case I_8266_DM_UCS_4: return (static_cast<NeoBus(RgbwUcs8904, Esp8266, Dma, 800Kbps)*>(busPtr))->CanShow(); break;
      case I_8266_U0_APA106_3: return (static_cast<NeoBus(Rbg, Esp8266, Uart0, Apa106)*>(busPtr))->CanShow(); break;
      case I_8266_U1_APA106_3: return (static_cast<NeoBus(Rbg, Esp8266, Uart1, Apa106)*>(busPtr))->CanShow(); break;
      case I_8266_DM_APA106_3: return (static_cast<NeoBus(Rbg, Esp8266, Dma, Apa106)*>(busPtr))->CanShow(); break;
      case I_8266_U0_FW6_5: return (static_cast<NeoBus(Grbcwx, Esp8266, Uart0, Ws2813)*>(busPtr))->CanShow(); break;
      case I_8266_U1_FW6_5: return (static_cast<NeoBus(Grbcwx, Esp8266, Uart1, Ws2813)*>(busPtr))->CanShow(); break;
      case I_8266_DM_FW6_5: return (static_cast<NeoBus(Grbcwx, Esp8266, Dma, 800Kbps)*>(busPtr))->CanShow(); break;
      case I_8266_U0_2805_5: return (static_cast<NeoBus(Grbww, Esp8266, Uart0, Ws2805)*>(busPtr))->CanShow(); break;
      case I_8266_U1_2805_5: return (static_cast<NeoBus(Grbww, Esp8266, Uart1, Ws2805)*>(busPtr))->CanShow(); break;
      case I_8266_DM_2805_5: return (static_cast<NeoBus(Grbww, Esp8266, Dma, Ws2805)*>(busPtr))->CanShow(); break;
      case I_8266_U0_TM1914_3: return (static_cast<NeoBus(RgbTm1914, Esp8266, Uart0, Tm1914)*>(busPtr))->CanShow(); break;
      case I_8266_U1_TM1914_3: return (static_cast<NeoBus(RgbTm1914, Esp8266, Uart1, Tm1914)*>(busPtr))->CanShow(); break;
      case I_8266_DM_TM1914_3: return (static_cast<NeoBus(RgbTm1914, Esp8266, Dma, Tm1914)*>(busPtr))->CanShow(); break;
      case I_8266_U0_SM16825_5: return (static_cast<NeoBus(RgbwcSm16825e, Esp8266, Uart0, Ws2813)*>(busPtr))->CanShow(); break;
      case I_8266_U1_SM16825_5: return (static_cast<NeoBus(RgbwcSm16825e, Esp8266, Uart1, Ws2813)*>(busPtr))->CanShow(); break;
      case I_8266_DM_SM16825_5: return (static_cast<NeoBus(RgbwcSm16825e, Esp8266, Dma, 800Kbps)*>(busPtr))->CanShow(); break;
      case I_8266_U0_NEODUAL_4: return (static_cast<NeoBus(Rgbwxx, Esp8266, Uart0, Ws2813)*>(busPtr))->CanShow(); break;
      case I_8266_U1_NEODUAL_4: return (static_cast<NeoBus(Rgbwxx, Esp8266, Uart1, Ws2813)*>(busPtr))->CanShow(); break;
      case I_8266_DM_NEODUAL_4: return (static_cast<NeoBus(Rgbwxx, Esp8266, Dma, 800Kbps)*>(busPtr))->CanShow(); break;
    #endif
    #ifdef ARDUINO_ARCH_ESP32
      // RMT buses
      case I_32_RN_NEO_3: return (static_cast<NeoBus(Grb, Esp32, RmtN, Ws2812x)*>(busPtr))->CanShow(); break;
      case I_32_RN_NEO_4: return (static_cast<NeoBus(Grbw, Esp32, RmtN, Sk6812)*>(busPtr))->CanShow(); break;
      case I_32_RN_400_3: return (static_cast<NeoBus(Grb, Esp32, RmtN, 400Kbps)*>(busPtr))->CanShow(); break;
      case I_32_RN_TM1_4: return (static_cast<NeoBus(WrgbTm1814, Esp32, RmtN, Tm1814)*>(busPtr))->CanShow(); break;
      case I_32_RN_TM2_3: return (static_cast<NeoBus(Brg, Esp32, RmtN, Tm1829)*>(busPtr))->CanShow(); break;
      case I_32_RN_UCS_3: return (static_cast<NeoBus(RgbUcs8903, Esp32, RmtN, Ws2812x)*>(busPtr))->CanShow(); break;
      case I_32_RN_UCS_4: return (static_cast<NeoBus(RgbwUcs8904, Esp32, RmtN, Ws2812x)*>(busPtr))->CanShow(); break;
      case I_32_RN_APA106_3: return (static_cast<NeoBus(Grb, Esp32, RmtN, Apa106)*>(busPtr))->CanShow(); break;
      case I_32_RN_FW6_5: return (static_cast<NeoBus(Grbcwx, Esp32, RmtN, Ws2812x)*>(busPtr))->CanShow(); break;
      case I_32_RN_2805_5: return (static_cast<NeoBus(Grbww, Esp32, RmtN, Ws2805)*>(busPtr))->CanShow(); break;
      case I_32_RN_TM1914_3: return (static_cast<NeoBus(GrbTm1914, Esp32, RmtN, Tm1914)*>(busPtr))->CanShow(); break;
      case I_32_RN_SM16825_5: return (static_cast<NeoBus(RgbcwSm16825e, Esp32, RmtN, Ws2812x)*>(busPtr))->CanShow(); break;
      case I_32_RN_NEODUAL_4: return (static_cast<NeoBus(Rgbwxx, Esp32, RmtN, Ws2812x)*>(busPtr))->CanShow(); break;
      // I2S1 bus or paralell buses
      #ifndef CONFIG_IDF_TARGET_ESP32C3
      case I_32_I2_NEO_3: if (_useParallelI2S) return (static_cast<NeoBus(Grb, Esp32, I2s1X8, Ws2812x)*>(busPtr))->CanShow(); else return (static_cast<NeoBus(Grb, Esp32, I2s1, Ws2812x)*>(busPtr))->CanShow(); break;
      case I_32_I2_NEO_4: if (_useParallelI2S) return (static_cast<NeoBus(Grbw, Esp32, I2s1X8, Sk6812)*>(busPtr))->CanShow(); else return (static_cast<NeoBus(Grbw, Esp32, I2s1, Sk6812)*>(busPtr))->CanShow(); break;
      case I_32_I2_400_3: if (_useParallelI2S) return (static_cast<NeoBus(Grb, Esp32, I2s1X8, 400Kbps)*>(busPtr))->CanShow(); else return (static_cast<NeoBus(Grb, Esp32, I2s1, 400Kbps)*>(busPtr))->CanShow(); break;
      case I_32_I2_TM1_4: if (_useParallelI2S) return (static_cast<NeoBus(WrgbTm1814, Esp32, I2s1X8, Tm1814)*>(busPtr))->CanShow(); else return (static_cast<NeoBus(WrgbTm1814, Esp32, I2s1, Tm1814)*>(busPtr))->CanShow(); break;
      case I_32_I2_TM2_3: if (_useParallelI2S) return (static_cast<NeoBus(Brg, Esp32, I2s1X8, Tm1829)*>(busPtr))->CanShow(); else return (static_cast<NeoBus(Brg, Esp32, I2s1, Tm1829)*>(busPtr))->CanShow(); break;
      case I_32_I2_UCS_3: if (_useParallelI2S) return (static_cast<NeoBus(RgbUcs8903, Esp32, I2s1X8, 800Kbps)*>(busPtr))->CanShow(); else return (static_cast<NeoBus(RgbUcs8903, Esp32, I2s1, 800Kbps)*>(busPtr))->CanShow(); break;
      case I_32_I2_UCS_4: if (_useParallelI2S) return (static_cast<NeoBus(RgbwUcs8904, Esp32, I2s1X8, 800Kbps)*>(busPtr))->CanShow(); else return (static_cast<NeoBus(RgbwUcs8904, Esp32, I2s1, 800Kbps)*>(busPtr))->CanShow(); break;
      case I_32_I2_APA106_3: if (_useParallelI2S) return (static_cast<NeoBus(Grb, Esp32, I2s1X8, Apa106)*>(busPtr))->CanShow(); else return (static_cast<NeoBus(Grb, Esp32, I2s1, Apa106)*>(busPtr))->CanShow(); break;
      case I_32_I2_FW6_5: if (_useParallelI2S) return (static_cast<NeoBus(Grbcwx, Esp32, I2s1X8, 800Kbps)*>(busPtr))->CanShow(); else return (static_cast<NeoBus(Grbcwx, Esp32, I2s1, 800Kbps)*>(busPtr))->CanShow(); break;
      case I_32_I2_2805_5: if (_useParallelI2S) return (static_cast<NeoBus(Grbww, Esp32, I2s1X8, Ws2805)*>(busPtr))->CanShow(); else return (static_cast<NeoBus(Grbww, Esp32, I2s1, Ws2805)*>(busPtr))->CanShow(); break;
      case I_32_I2_TM1914_3: if (_useParallelI2S) return (static_cast<NeoBus(GrbTm1914, Esp32, I2s1X8, Tm1914)*>(busPtr))->CanShow(); else return (static_cast<NeoBus(GrbTm1914, Esp32, I2s1, Tm1914)*>(busPtr))->CanShow(); break;
      case I_32_I2_SM16825_5: if (_useParallelI2S) return (static_cast<NeoBus(RgbcwSm16825e, Esp32, I2s1X8, Ws2812x)*>(busPtr))->CanShow(); else return (static_cast<NeoBus(RgbcwSm16825e, Esp32, I2s1, Ws2812x)*>(busPtr))->CanShow(); break;
      case I_32_I2_NEODUAL_4: if (_useParallelI2S) return (static_cast<NeoBus(Rgbwxx, Esp32, I2s1X8, Ws2812x)*>(busPtr))->CanShow(); else return (static_cast<NeoBus(Rgbwxx, Esp32, I2s1, Ws2812x)*>(busPtr))->CanShow(); break;
      #endif
    #endif
      case I_HS_DOT_3: return (static_cast<TwoPinBus(DotStarBgr, DotStarSpiHz)*>(busPtr))->CanShow(); break;
      case I_SS_DOT_3: return (static_cast<TwoPinBus(DotStarBgr, DotStar)*>(busPtr))->CanShow(); break;
      case I_HS_LPD_3: return (static_cast<TwoPinBus(Lpd8806Grb, Lpd8806SpiHz)*>(busPtr))->CanShow(); break;
      case I_SS_LPD_3: return (static_cast<TwoPinBus(Lpd8806Grb, Lpd8806)*>(busPtr))->CanShow(); break;
      case I_HS_LPO_3: return (static_cast<TwoPinBus(Lpd6803Grb, Lpd6803SpiHz)*>(busPtr))->CanShow(); break;
      case I_SS_LPO_3: return (static_cast<TwoPinBus(Lpd6803Grb, Lpd6803)*>(busPtr))->CanShow(); break;
      case I_HS_WS1_3: return (static_cast<TwoPinBus(NeoRbg, Ws2801SpiHz)*>(busPtr))->CanShow(); break;
      case I_SS_WS1_3: return (static_cast<TwoPinBus(NeoRbg, Ws2801)*>(busPtr))->CanShow(); break;
      case I_HS_P98_3: return (static_cast<TwoPinBus(P9813Bgr, P9813SpiHz)*>(busPtr))->CanShow(); break;
      case I_SS_P98_3: return (static_cast<TwoPinBus(P9813Bgr, P9813)*>(busPtr))->CanShow(); break;
      case I_HS_HD1_3: return (static_cast<TwoPinBus(NeoBgr48, Hd108SpiHz)*>(busPtr))->CanShow(); break;
      case I_SS_HD1_3: return (static_cast<TwoPinBus(NeoBgr48, Hd108)*>(busPtr))->CanShow(); break;
    }
    return true;
  }

  [[gnu::hot]] static void setPixelColor(void* busPtr, uint8_t busType, uint16_t pix, uint32_t c, uint8_t co, uint16_t wwcw = 0) {
    uint8_t r = c >> 16;
    uint8_t g = c >> 8;
    uint8_t b = c >> 0;
    uint8_t w = c >> 24;
    RgbwColor col;
    uint8_t cctWW = wwcw & 0xFF, cctCW = (wwcw>>8) & 0xFF;

    // reorder channels to selected order
    switch (co & 0x0F) {
      default: col.G = g; col.R = r; col.B = b; break; //0 = GRB, default
      case  1: col.G = r; col.R = g; col.B = b; break; //1 = RGB, common for WS2811
      case  2: col.G = b; col.R = r; col.B = g; break; //2 = BRG
      case  3: col.G = r; col.R = b; col.B = g; break; //3 = RBG
      case  4: col.G = b; col.R = g; col.B = r; break; //4 = BGR
      case  5: col.G = g; col.R = b; col.B = r; break; //5 = GBR
    }
    // upper nibble contains W swap information
    switch (co >> 4) {
      default: col.W = w;                break; // no swapping
      case  1: col.W = col.B; col.B = w; break; // swap W & B
      case  2: col.W = col.G; col.G = w; break; // swap W & G
      case  3: col.W = col.R; col.R = w; break; // swap W & R
      case  4: std::swap(cctWW, cctCW);  break; // swap WW & CW
    }

    switch (busType) {
      case I_NONE: break;
    #ifdef ESP8266
      case I_8266_U0_NEO_3: (static_cast<NeoBus(Grb, Esp8266, Uart0, Ws2813)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_8266_U1_NEO_3: (static_cast<NeoBus(Grb, Esp8266, Uart1, Ws2813)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_8266_DM_NEO_3: (static_cast<NeoBus(Grb, Esp8266, Dma, 800Kbps)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_8266_U0_NEO_4: (static_cast<NeoBus(Grbw, Esp8266, Uart0, Ws2813)*>(busPtr))->SetPixelColor(pix, col); break;
      case I_8266_U1_NEO_4: (static_cast<NeoBus(Grbw, Esp8266, Uart1, Ws2813)*>(busPtr))->SetPixelColor(pix, col); break;
      case I_8266_DM_NEO_4: (static_cast<NeoBus(Grbw, Esp8266, Dma, 800Kbps)*>(busPtr))->SetPixelColor(pix, col); break;
      case I_8266_U0_400_3: (static_cast<NeoBus(Grb, Esp8266, Uart0, 400Kbps)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_8266_U1_400_3: (static_cast<NeoBus(Grb, Esp8266, Uart1, 400Kbps)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_8266_DM_400_3: (static_cast<NeoBus(Grb, Esp8266, Dma, 400Kbps)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_8266_U0_TM1_4: (static_cast<NeoBus(WrgbTm1814, Esp8266, Uart0, Tm1814)*>(busPtr))->SetPixelColor(pix, col); break;
      case I_8266_U1_TM1_4: (static_cast<NeoBus(WrgbTm1814, Esp8266, Uart1, Tm1814)*>(busPtr))->SetPixelColor(pix, col); break;
      case I_8266_DM_TM1_4: (static_cast<NeoBus(WrgbTm1814, Esp8266, Dma, Tm1814)*>(busPtr))->SetPixelColor(pix, col); break;
      case I_8266_U0_TM2_3: (static_cast<NeoBus(Brg, Esp8266, Uart0, Tm1829)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_8266_U1_TM2_3: (static_cast<NeoBus(Brg, Esp8266, Uart1, Tm1829)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_8266_DM_TM2_3: (static_cast<NeoBus(Brg, Esp8266, Dma, Tm1829)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_8266_U0_UCS_3: (static_cast<NeoBus(RgbUcs8903, Esp8266, Uart0, Ws2813)*>(busPtr))->SetPixelColor(pix, Rgb48Color(RgbColor(col))); break;
      case I_8266_U1_UCS_3: (static_cast<NeoBus(RgbUcs8903, Esp8266, Uart1, Ws2813)*>(busPtr))->SetPixelColor(pix, Rgb48Color(RgbColor(col))); break;
      case I_8266_DM_UCS_3: (static_cast<NeoBus(RgbUcs8903, Esp8266, Dma, 800Kbps)*>(busPtr))->SetPixelColor(pix, Rgb48Color(RgbColor(col))); break;
      case I_8266_U0_UCS_4: (static_cast<NeoBus(RgbwUcs8904, Esp8266, Uart0, Ws2813)*>(busPtr))->SetPixelColor(pix, Rgbw64Color(col)); break;
      case I_8266_U1_UCS_4: (static_cast<NeoBus(RgbwUcs8904, Esp8266, Uart1, Ws2813)*>(busPtr))->SetPixelColor(pix, Rgbw64Color(col)); break;
      case I_8266_DM_UCS_4: (static_cast<NeoBus(RgbwUcs8904, Esp8266, Dma, 800Kbps)*>(busPtr))->SetPixelColor(pix, Rgbw64Color(col)); break;
      case I_8266_U0_APA106_3: (static_cast<NeoBus(Rbg, Esp8266, Uart0, Apa106)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_8266_U1_APA106_3: (static_cast<NeoBus(Rbg, Esp8266, Uart1, Apa106)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_8266_DM_APA106_3: (static_cast<NeoBus(Rbg, Esp8266, Dma, Apa106)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_8266_U0_FW6_5: (static_cast<NeoBus(Grbcwx, Esp8266, Uart0, Ws2813)*>(busPtr))->SetPixelColor(pix, RgbwwColor(col.R, col.G, col.B, cctWW, cctCW)); break;
      case I_8266_U1_FW6_5: (static_cast<NeoBus(Grbcwx, Esp8266, Uart1, Ws2813)*>(busPtr))->SetPixelColor(pix, RgbwwColor(col.R, col.G, col.B, cctWW, cctCW)); break;
      case I_8266_DM_FW6_5: (static_cast<NeoBus(Grbcwx, Esp8266, Dma, 800Kbps)*>(busPtr))->SetPixelColor(pix, RgbwwColor(col.R, col.G, col.B, cctWW, cctCW)); break;
      case I_8266_U0_2805_5: (static_cast<NeoBus(Grbww, Esp8266, Uart0, Ws2805)*>(busPtr))->SetPixelColor(pix, RgbwwColor(col.R, col.G, col.B, cctWW, cctCW)); break;
      case I_8266_U1_2805_5: (static_cast<NeoBus(Grbww, Esp8266, Uart1, Ws2805)*>(busPtr))->SetPixelColor(pix, RgbwwColor(col.R, col.G, col.B, cctWW, cctCW)); break;
      case I_8266_DM_2805_5: (static_cast<NeoBus(Grbww, Esp8266, Dma, Ws2805)*>(busPtr))->SetPixelColor(pix, RgbwwColor(col.R, col.G, col.B, cctWW, cctCW)); break;
      case I_8266_U0_TM1914_3: (static_cast<NeoBus(RgbTm1914, Esp8266, Uart0, Tm1914)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_8266_U1_TM1914_3: (static_cast<NeoBus(RgbTm1914, Esp8266, Uart1, Tm1914)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_8266_DM_TM1914_3: (static_cast<NeoBus(RgbTm1914, Esp8266, Dma, Tm1914)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_8266_U0_SM16825_5: (static_cast<NeoBus(RgbwcSm16825e, Esp8266, Uart0, Ws2813)*>(busPtr))->SetPixelColor(pix, Rgbww80Color(col.R*257, col.G*257, col.B*257, cctWW*257, cctCW*257)); break;
      case I_8266_U1_SM16825_5: (static_cast<NeoBus(RgbwcSm16825e, Esp8266, Uart1, Ws2813)*>(busPtr))->SetPixelColor(pix, Rgbww80Color(col.R*257, col.G*257, col.B*257, cctWW*257, cctCW*257)); break;
      case I_8266_DM_SM16825_5: (static_cast<NeoBus(RgbwcSm16825e, Esp8266, Dma, 800Kbps)*>(busPtr))->SetPixelColor(pix, Rgbww80Color(col.R*257, col.G*257, col.B*257, cctWW*257, cctCW*257)); break;
      case I_8266_U0_NEODUAL_4: (static_cast<NeoBus(Rgbwxx, Esp8266, Uart0, Ws2813)*>(busPtr))->SetPixelColor(pix, col); break;
      case I_8266_U1_NEODUAL_4: (static_cast<NeoBus(Rgbwxx, Esp8266, Uart1, Ws2813)*>(busPtr))->SetPixelColor(pix, col); break;
      case I_8266_DM_NEODUAL_4: (static_cast<NeoBus(Rgbwxx, Esp8266, Dma, 800Kbps)*>(busPtr))->SetPixelColor(pix, col); break;
    #endif
    #ifdef ARDUINO_ARCH_ESP32
      // RMT buses
      case I_32_RN_NEO_3: (static_cast<NeoBus(Grb, Esp32, RmtN, Ws2812x)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_32_RN_NEO_4: (static_cast<NeoBus(Grbw, Esp32, RmtN, Sk6812)*>(busPtr))->SetPixelColor(pix, col); break;
      case I_32_RN_400_3: (static_cast<NeoBus(Grb, Esp32, RmtN, 400Kbps)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_32_RN_TM1_4: (static_cast<NeoBus(WrgbTm1814, Esp32, RmtN, Tm1814)*>(busPtr))->SetPixelColor(pix, col); break;
      case I_32_RN_TM2_3: (static_cast<NeoBus(Brg, Esp32, RmtN, Tm1829)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_32_RN_UCS_3: (static_cast<NeoBus(RgbUcs8903, Esp32, RmtN, Ws2812x)*>(busPtr))->SetPixelColor(pix, Rgb48Color(RgbColor(col))); break;
      case I_32_RN_UCS_4: (static_cast<NeoBus(RgbwUcs8904, Esp32, RmtN, Ws2812x)*>(busPtr))->SetPixelColor(pix, Rgbw64Color(col)); break;
      case I_32_RN_APA106_3: (static_cast<NeoBus(Grb, Esp32, RmtN, Apa106)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_32_RN_FW6_5: (static_cast<NeoBus(Grbcwx, Esp32, RmtN, Ws2812x)*>(busPtr))->SetPixelColor(pix, RgbwwColor(col.R, col.G, col.B, cctWW, cctCW)); break;
      case I_32_RN_2805_5: (static_cast<NeoBus(Grbww, Esp32, RmtN, Ws2805)*>(busPtr))->SetPixelColor(pix, RgbwwColor(col.R, col.G, col.B, cctWW, cctCW)); break;
      case I_32_RN_TM1914_3: (static_cast<NeoBus(GrbTm1914, Esp32, RmtN, Tm1914)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_32_RN_SM16825_5: (static_cast<NeoBus(RgbcwSm16825e, Esp32, RmtN, Ws2812x)*>(busPtr))->SetPixelColor(pix, Rgbww80Color(col.R*257, col.G*257, col.B*257, cctWW*257, cctCW*257)); break;
      case I_32_RN_NEODUAL_4: (static_cast<NeoBus(Rgbwxx, Esp32, RmtN, Ws2812x)*>(busPtr))->SetPixelColor(pix, col); break;
      // I2S1 bus or paralell buses
      #ifndef CONFIG_IDF_TARGET_ESP32C3
      case I_32_I2_NEO_3: if (_useParallelI2S) (static_cast<NeoBus(Grb, Esp32, I2s1X8, Ws2812x)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); else (static_cast<NeoBus(Grb, Esp32, I2s1, Ws2812x)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_32_I2_NEO_4: if (_useParallelI2S) (static_cast<NeoBus(Grbw, Esp32, I2s1X8, Sk6812)*>(busPtr))->SetPixelColor(pix, col); else (static_cast<NeoBus(Grbw, Esp32, I2s1, Sk6812)*>(busPtr))->SetPixelColor(pix, col); break;
      case I_32_I2_400_3: if (_useParallelI2S) (static_cast<NeoBus(Grb, Esp32, I2s1X8, 400Kbps)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); else (static_cast<NeoBus(Grb, Esp32, I2s1, 400Kbps)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_32_I2_TM1_4: if (_useParallelI2S) (static_cast<NeoBus(WrgbTm1814, Esp32, I2s1X8, Tm1814)*>(busPtr))->SetPixelColor(pix, col); else (static_cast<NeoBus(WrgbTm1814, Esp32, I2s1, Tm1814)*>(busPtr))->SetPixelColor(pix, col); break;
      case I_32_I2_TM2_3: if (_useParallelI2S) (static_cast<NeoBus(Brg, Esp32, I2s1X8, Tm1829)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); else (static_cast<NeoBus(Brg, Esp32, I2s1, Tm1829)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_32_I2_UCS_3: if (_useParallelI2S) (static_cast<NeoBus(RgbUcs8903, Esp32, I2s1X8, 800Kbps)*>(busPtr))->SetPixelColor(pix, Rgb48Color(RgbColor(col))); else (static_cast<NeoBus(RgbUcs8903, Esp32, I2s1, 800Kbps)*>(busPtr))->SetPixelColor(pix, Rgb48Color(RgbColor(col))); break;
      case I_32_I2_UCS_4: if (_useParallelI2S) (static_cast<NeoBus(RgbwUcs8904, Esp32, I2s1X8, 800Kbps)*>(busPtr))->SetPixelColor(pix, Rgbw64Color(col)); else (static_cast<NeoBus(RgbwUcs8904, Esp32, I2s1, 800Kbps)*>(busPtr))->SetPixelColor(pix, Rgbw64Color(col)); break;
      case I_32_I2_APA106_3: if (_useParallelI2S) (static_cast<NeoBus(Grb, Esp32, I2s1X8, Apa106)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); else (static_cast<NeoBus(Grb, Esp32, I2s1, Apa106)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_32_I2_FW6_5: if (_useParallelI2S) (static_cast<NeoBus(Grbcwx, Esp32, I2s1X8, 800Kbps)*>(busPtr))->SetPixelColor(pix, RgbwwColor(col.R, col.G, col.B, cctWW, cctCW)); else (static_cast<NeoBus(Grbcwx, Esp32, I2s1, 800Kbps)*>(busPtr))->SetPixelColor(pix, RgbwwColor(col.R, col.G, col.B, cctWW, cctCW)); break;
      case I_32_I2_2805_5: if (_useParallelI2S) (static_cast<NeoBus(Grbww, Esp32, I2s1X8, Ws2805)*>(busPtr))->SetPixelColor(pix, RgbwwColor(col.R, col.G, col.B, cctWW, cctCW)); else (static_cast<NeoBus(Grbww, Esp32, I2s1, Ws2805)*>(busPtr))->SetPixelColor(pix, RgbwwColor(col.R, col.G, col.B, cctWW, cctCW)); break;
      case I_32_I2_TM1914_3: if (_useParallelI2S) (static_cast<NeoBus(GrbTm1914, Esp32, I2s1X8, Tm1914)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); else (static_cast<NeoBus(GrbTm1914, Esp32, I2s1, Tm1914)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_32_I2_SM16825_5: if (_useParallelI2S) (static_cast<NeoBus(RgbcwSm16825e, Esp32, I2s1X8, Ws2812x)*>(busPtr))->SetPixelColor(pix, Rgbww80Color(col.R*257, col.G*257, col.B*257, cctWW*257, cctCW*257)); else (static_cast<NeoBus(RgbcwSm16825e, Esp32, I2s1, Ws2812x)*>(busPtr))->SetPixelColor(pix, Rgbww80Color(col.R*257, col.G*257, col.B*257, cctWW*257, cctCW*257)); break;
      case I_32_I2_NEODUAL_4: if (_useParallelI2S) (static_cast<NeoBus(Rgbwxx, Esp32, I2s1X8, Ws2812x)*>(busPtr))->SetPixelColor(pix, col); else (static_cast<NeoBus(Rgbwxx, Esp32, I2s1, Ws2812x)*>(busPtr))->SetPixelColor(pix, col); break;
      #endif
    #endif
      case I_HS_DOT_3: (static_cast<TwoPinBus(DotStarBgr, DotStarSpiHz)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_SS_DOT_3: (static_cast<TwoPinBus(DotStarBgr, DotStar)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_HS_LPD_3: (static_cast<TwoPinBus(Lpd8806Grb, Lpd8806SpiHz)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_SS_LPD_3: (static_cast<TwoPinBus(Lpd8806Grb, Lpd8806)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_HS_LPO_3: (static_cast<TwoPinBus(Lpd6803Grb, Lpd6803SpiHz)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_SS_LPO_3: (static_cast<TwoPinBus(Lpd6803Grb, Lpd6803)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_HS_WS1_3: (static_cast<TwoPinBus(NeoRbg, Ws2801SpiHz)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_SS_WS1_3: (static_cast<TwoPinBus(NeoRbg, Ws2801)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_HS_P98_3: (static_cast<TwoPinBus(P9813Bgr, P9813SpiHz)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_SS_P98_3: (static_cast<TwoPinBus(P9813Bgr, P9813)*>(busPtr))->SetPixelColor(pix, RgbColor(col)); break;
      case I_HS_HD1_3: (static_cast<TwoPinBus(NeoBgr48, Hd108SpiHz)*>(busPtr))->SetPixelColor(pix, Rgb48Color(col)); break;
      case I_SS_HD1_3: (static_cast<TwoPinBus(NeoBgr48, Hd108)*>(busPtr))->SetPixelColor(pix, Rgb48Color(col)); break;
    }
  }

  [[gnu::hot]] static uint32_t getPixelColor(void* busPtr, uint8_t busType, uint16_t pix, uint8_t co) {
    RgbwColor col(0,0,0,0);
    switch (busType) {
      case I_NONE: break;
    #ifdef ESP8266
      case I_8266_U0_NEO_3: col = (static_cast<NeoBus(Grb, Esp8266, Uart0, Ws2813)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_U1_NEO_3: col = (static_cast<NeoBus(Grb, Esp8266, Uart1, Ws2813)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_DM_NEO_3: col = (static_cast<NeoBus(Grb, Esp8266, Dma, 800Kbps)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_U0_NEO_4: col = (static_cast<NeoBus(Grbw, Esp8266, Uart0, Ws2813)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_U1_NEO_4: col = (static_cast<NeoBus(Grbw, Esp8266, Uart1, Ws2813)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_DM_NEO_4: col = (static_cast<NeoBus(Grbw, Esp8266, Dma, 800Kbps)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_U0_400_3: col = (static_cast<NeoBus(Grb, Esp8266, Uart0, 400Kbps)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_U1_400_3: col = (static_cast<NeoBus(Grb, Esp8266, Uart1, 400Kbps)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_DM_400_3: col = (static_cast<NeoBus(Grb, Esp8266, Dma, 400Kbps)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_U0_TM1_4: col = (static_cast<NeoBus(WrgbTm1814, Esp8266, Uart0, Tm1814)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_U1_TM1_4: col = (static_cast<NeoBus(WrgbTm1814, Esp8266, Uart1, Tm1814)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_DM_TM1_4: col = (static_cast<NeoBus(WrgbTm1814, Esp8266, Dma, Tm1814)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_U0_TM2_3: col = (static_cast<NeoBus(Brg, Esp8266, Uart0, Tm1829)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_U1_TM2_3: col = (static_cast<NeoBus(Brg, Esp8266, Uart1, Tm1829)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_DM_TM2_3: col = (static_cast<NeoBus(Brg, Esp8266, Dma, Tm1829)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_U0_UCS_3: { Rgb48Color c = (static_cast<NeoBus(RgbUcs8903, Esp8266, Uart0, Ws2813)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R>>8,c.G>>8,c.B>>8,0); } break;
      case I_8266_U1_UCS_3: { Rgb48Color c = (static_cast<NeoBus(RgbUcs8903, Esp8266, Uart1, Ws2813)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R>>8,c.G>>8,c.B>>8,0); } break;
      case I_8266_DM_UCS_3: { Rgb48Color c = (static_cast<NeoBus(RgbUcs8903, Esp8266, Dma, 800Kbps)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R>>8,c.G>>8,c.B>>8,0); } break;
      case I_8266_U0_UCS_4: { Rgbw64Color c = (static_cast<NeoBus(RgbwUcs8904, Esp8266, Uart0, Ws2813)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R>>8,c.G>>8,c.B>>8,c.W>>8); } break;
      case I_8266_U1_UCS_4: { Rgbw64Color c = (static_cast<NeoBus(RgbwUcs8904, Esp8266, Uart1, Ws2813)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R>>8,c.G>>8,c.B>>8,c.W>>8); } break;
      case I_8266_DM_UCS_4: { Rgbw64Color c = (static_cast<NeoBus(RgbwUcs8904, Esp8266, Dma, 800Kbps)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R>>8,c.G>>8,c.B>>8,c.W>>8); } break;
      case I_8266_U0_APA106_3: col = (static_cast<NeoBus(Rbg, Esp8266, Uart0, Apa106)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_U1_APA106_3: col = (static_cast<NeoBus(Rbg, Esp8266, Uart1, Apa106)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_DM_APA106_3: col = (static_cast<NeoBus(Rbg, Esp8266, Dma, Apa106)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_U0_FW6_5: { RgbwwColor c = (static_cast<NeoBus(Grbcwx, Esp8266, Uart0, Ws2813)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R,c.G,c.B,max(c.WW,c.CW)); } break; // will not return original W
      case I_8266_U1_FW6_5: { RgbwwColor c = (static_cast<NeoBus(Grbcwx, Esp8266, Uart1, Ws2813)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R,c.G,c.B,max(c.WW,c.CW)); } break; // will not return original W
      case I_8266_DM_FW6_5: { RgbwwColor c = (static_cast<NeoBus(Grbcwx, Esp8266, Dma, 800Kbps)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R,c.G,c.B,max(c.WW,c.CW)); } break; // will not return original W
      case I_8266_U0_2805_5: { RgbwwColor c = (static_cast<NeoBus(Grbww, Esp8266, Uart0, Ws2805)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R,c.G,c.B,max(c.WW,c.CW)); } break; // will not return original W
      case I_8266_U1_2805_5: { RgbwwColor c = (static_cast<NeoBus(Grbww, Esp8266, Uart1, Ws2805)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R,c.G,c.B,max(c.WW,c.CW)); } break; // will not return original W
      case I_8266_DM_2805_5: { RgbwwColor c = (static_cast<NeoBus(Grbww, Esp8266, Dma, Ws2805)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R,c.G,c.B,max(c.WW,c.CW)); } break; // will not return original W
      case I_8266_U0_TM1914_3: col = (static_cast<NeoBus(RgbTm1914, Esp8266, Uart0, Tm1914)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_U1_TM1914_3: col = (static_cast<NeoBus(RgbTm1914, Esp8266, Uart1, Tm1914)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_DM_TM1914_3: col = (static_cast<NeoBus(RgbTm1914, Esp8266, Dma, Tm1914)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_U0_SM16825_5: { Rgbww80Color c = (static_cast<NeoBus(RgbwcSm16825e, Esp8266, Uart0, Ws2813)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R,c.G,c.B,max(c.WW,c.CW)); } break; // will not return original W
      case I_8266_U1_SM16825_5: { Rgbww80Color c = (static_cast<NeoBus(RgbwcSm16825e, Esp8266, Uart1, Ws2813)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R,c.G,c.B,max(c.WW,c.CW)); } break; // will not return original W
      case I_8266_DM_SM16825_5: { Rgbww80Color c = (static_cast<NeoBus(RgbwcSm16825e, Esp8266, Dma, 800Kbps)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R,c.G,c.B,max(c.WW,c.CW)); } break; // will not return original W
      case I_8266_U0_NEODUAL_4: col = (static_cast<NeoBus(Rgbwxx, Esp8266, Uart0, Ws2813)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_U1_NEODUAL_4: col = (static_cast<NeoBus(Rgbwxx, Esp8266, Uart1, Ws2813)*>(busPtr))->GetPixelColor(pix); break;
      case I_8266_DM_NEODUAL_4: col = (static_cast<NeoBus(Rgbwxx, Esp8266, Dma, 800Kbps)*>(busPtr))->GetPixelColor(pix); break;
    #endif
    #ifdef ARDUINO_ARCH_ESP32
      // RMT buses
      case I_32_RN_NEO_3: col = (static_cast<NeoBus(Grb, Esp32, RmtN, Ws2812x)*>(busPtr))->GetPixelColor(pix); break;
      case I_32_RN_NEO_4: col = (static_cast<NeoBus(Grbw, Esp32, RmtN, Sk6812)*>(busPtr))->GetPixelColor(pix); break;
      case I_32_RN_400_3: col = (static_cast<NeoBus(Grb, Esp32, RmtN, 400Kbps)*>(busPtr))->GetPixelColor(pix); break;
      case I_32_RN_TM1_4: col = (static_cast<NeoBus(WrgbTm1814, Esp32, RmtN, Tm1814)*>(busPtr))->GetPixelColor(pix); break;
      case I_32_RN_TM2_3: col = (static_cast<NeoBus(Brg, Esp32, RmtN, Tm1829)*>(busPtr))->GetPixelColor(pix); break;
      case I_32_RN_UCS_3: { Rgb48Color c = (static_cast<NeoBus(RgbUcs8903, Esp32, RmtN, Ws2812x)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R>>8,c.G>>8,c.B>>8,0); } break;
      case I_32_RN_UCS_4: { Rgbw64Color c = (static_cast<NeoBus(RgbwUcs8904, Esp32, RmtN, Ws2812x)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R>>8,c.G>>8,c.B>>8,c.W>>8); } break;
      case I_32_RN_APA106_3: col = (static_cast<NeoBus(Grb, Esp32, RmtN, Apa106)*>(busPtr))->GetPixelColor(pix); break;
      case I_32_RN_FW6_5: { RgbwwColor c = (static_cast<NeoBus(Grbcwx, Esp32, RmtN, Ws2812x)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R,c.G,c.B,max(c.WW,c.CW)); } break; // will not return original W
      case I_32_RN_2805_5: { RgbwwColor c = (static_cast<NeoBus(Grbww, Esp32, RmtN, Ws2805)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R,c.G,c.B,max(c.WW,c.CW)); } break; // will not return original W
      case I_32_RN_TM1914_3: col = (static_cast<NeoBus(GrbTm1914, Esp32, RmtN, Tm1914)*>(busPtr))->GetPixelColor(pix); break;
      case I_32_RN_SM16825_5: { Rgbww80Color c = (static_cast<NeoBus(RgbcwSm16825e, Esp32, RmtN, Ws2812x)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R/257,c.G/257,c.B/257,max(c.WW,c.CW)/257); } break; // will not return original W
      case I_32_RN_NEODUAL_4: col = (static_cast<NeoBus(Rgbwxx, Esp32, RmtN, Ws2812x)*>(busPtr))->GetPixelColor(pix); break;
      // I2S1 bus or paralell buses
      #ifndef CONFIG_IDF_TARGET_ESP32C3
      case I_32_I2_NEO_3: col = (_useParallelI2S) ? (static_cast<NeoBus(Grb, Esp32, I2s1X8, Ws2812x)*>(busPtr))->GetPixelColor(pix) : (static_cast<NeoBus(Grb, Esp32, I2s1, Ws2812x)*>(busPtr))->GetPixelColor(pix); break;
      case I_32_I2_NEO_4: col = (_useParallelI2S) ? (static_cast<NeoBus(Grbw, Esp32, I2s1X8, Sk6812)*>(busPtr))->GetPixelColor(pix) : (static_cast<NeoBus(Grbw, Esp32, I2s1, Sk6812)*>(busPtr))->GetPixelColor(pix); break;
      case I_32_I2_400_3: col = (_useParallelI2S) ? (static_cast<NeoBus(Grb, Esp32, I2s1X8, 400Kbps)*>(busPtr))->GetPixelColor(pix) : (static_cast<NeoBus(Grb, Esp32, I2s1, 400Kbps)*>(busPtr))->GetPixelColor(pix); break;
      case I_32_I2_TM1_4: col = (_useParallelI2S) ? (static_cast<NeoBus(WrgbTm1814, Esp32, I2s1X8, Tm1814)*>(busPtr))->GetPixelColor(pix) : (static_cast<NeoBus(WrgbTm1814, Esp32, I2s1, Tm1814)*>(busPtr))->GetPixelColor(pix); break;
      case I_32_I2_TM2_3: col = (_useParallelI2S) ? (static_cast<NeoBus(Brg, Esp32, I2s1X8, Tm1829)*>(busPtr))->GetPixelColor(pix) : (static_cast<NeoBus(Brg, Esp32, I2s1, Tm1829)*>(busPtr))->GetPixelColor(pix); break;
      case I_32_I2_UCS_3: { Rgb48Color c = (_useParallelI2S) ? (static_cast<NeoBus(RgbUcs8903, Esp32, I2s1X8, 800Kbps)*>(busPtr))->GetPixelColor(pix) : (static_cast<NeoBus(RgbUcs8903, Esp32, I2s1, 800Kbps)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R/257,c.G/257,c.B/257,0); } break;
      case I_32_I2_UCS_4: { Rgbw64Color c = (_useParallelI2S) ? (static_cast<NeoBus(RgbwUcs8904, Esp32, I2s1X8, 800Kbps)*>(busPtr))->GetPixelColor(pix) : (static_cast<NeoBus(RgbwUcs8904, Esp32, I2s1, 800Kbps)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R/257,c.G/257,c.B/257,c.W/257); } break;
      case I_32_I2_APA106_3: col = (_useParallelI2S) ? (static_cast<NeoBus(Grb, Esp32, I2s1X8, Apa106)*>(busPtr))->GetPixelColor(pix) : (static_cast<NeoBus(Grb, Esp32, I2s1, Apa106)*>(busPtr))->GetPixelColor(pix); break;
      case I_32_I2_FW6_5: { RgbwwColor c = (_useParallelI2S) ? (static_cast<NeoBus(Grbcwx, Esp32, I2s1X8, 800Kbps)*>(busPtr))->GetPixelColor(pix) : (static_cast<NeoBus(Grbcwx, Esp32, I2s1, 800Kbps)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R,c.G,c.B,max(c.WW,c.CW)); } break; // will not return original W
      case I_32_I2_2805_5: { RgbwwColor c = (_useParallelI2S) ? (static_cast<NeoBus(Grbww, Esp32, I2s1X8, Ws2805)*>(busPtr))->GetPixelColor(pix) : (static_cast<NeoBus(Grbww, Esp32, I2s1, Ws2805)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R,c.G,c.B,max(c.WW,c.CW)); } break; // will not return original W
      case I_32_I2_TM1914_3: col = (_useParallelI2S) ? (static_cast<NeoBus(GrbTm1914, Esp32, I2s1X8, Tm1914)*>(busPtr))->GetPixelColor(pix) : (static_cast<NeoBus(GrbTm1914, Esp32, I2s1, Tm1914)*>(busPtr))->GetPixelColor(pix); break;
      case I_32_I2_SM16825_5: { Rgbww80Color c = (_useParallelI2S) ? (static_cast<NeoBus(RgbcwSm16825e, Esp32, I2s1X8, Ws2812x)*>(busPtr))->GetPixelColor(pix) : (static_cast<NeoBus(RgbcwSm16825e, Esp32, I2s1, Ws2812x)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R/257,c.G/257,c.B/257,max(c.WW,c.CW)/257); } break; // will not return original W
      case I_32_I2_NEODUAL_4: col = (_useParallelI2S) ? (static_cast<NeoBus(Rgbwxx, Esp32, I2s1X8, Ws2812x)*>(busPtr))->GetPixelColor(pix) : (static_cast<NeoBus(Rgbwxx, Esp32, I2s1, Ws2812x)*>(busPtr))->GetPixelColor(pix); break;
      #endif
    #endif
      case I_HS_DOT_3: col = (static_cast<TwoPinBus(DotStarBgr, DotStarSpiHz)*>(busPtr))->GetPixelColor(pix); break;
      case I_SS_DOT_3: col = (static_cast<TwoPinBus(DotStarBgr, DotStar)*>(busPtr))->GetPixelColor(pix); break;
      case I_HS_LPD_3: col = (static_cast<TwoPinBus(Lpd8806Grb, Lpd8806SpiHz)*>(busPtr))->GetPixelColor(pix); break;
      case I_SS_LPD_3: col = (static_cast<TwoPinBus(Lpd8806Grb, Lpd8806)*>(busPtr))->GetPixelColor(pix); break;
      case I_HS_LPO_3: col = (static_cast<TwoPinBus(Lpd6803Grb, Lpd6803SpiHz)*>(busPtr))->GetPixelColor(pix); break;
      case I_SS_LPO_3: col = (static_cast<TwoPinBus(Lpd6803Grb, Lpd6803)*>(busPtr))->GetPixelColor(pix); break;
      case I_HS_WS1_3: col = (static_cast<TwoPinBus(NeoRbg, Ws2801SpiHz)*>(busPtr))->GetPixelColor(pix); break;
      case I_SS_WS1_3: col = (static_cast<TwoPinBus(NeoRbg, Ws2801)*>(busPtr))->GetPixelColor(pix); break;
      case I_HS_P98_3: col = (static_cast<TwoPinBus(P9813Bgr, P9813SpiHz)*>(busPtr))->GetPixelColor(pix); break;
      case I_SS_P98_3: col = (static_cast<TwoPinBus(P9813Bgr, P9813)*>(busPtr))->GetPixelColor(pix); break;
      case I_HS_HD1_3: { Rgb48Color c = (static_cast<TwoPinBus(NeoBgr48, Hd108SpiHz)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R>>8,c.G>>8,c.B>>8,0); } break;
      case I_SS_HD1_3: { Rgb48Color c = (static_cast<TwoPinBus(NeoBgr48, Hd108)*>(busPtr))->GetPixelColor(pix); col = RGBW32(c.R>>8,c.G>>8,c.B>>8,0); } break;
    }

    // upper nibble contains W swap information
    uint8_t w = col.W;
    switch (co >> 4) {
      case 1: col.W = col.B; col.B = w; break; // swap W & B
      case 2: col.W = col.G; col.G = w; break; // swap W & G
      case 3: col.W = col.R; col.R = w; break; // swap W & R
    }
    switch (co & 0x0F) {
      //                    W               G              R               B
      default: return ((col.W << 24) | (col.G << 8) | (col.R << 16) | (col.B)); //0 = GRB, default
      case  1: return ((col.W << 24) | (col.R << 8) | (col.G << 16) | (col.B)); //1 = RGB, common for WS2811
      case  2: return ((col.W << 24) | (col.B << 8) | (col.R << 16) | (col.G)); //2 = BRG
      case  3: return ((col.W << 24) | (col.B << 8) | (col.G << 16) | (col.R)); //3 = RBG
      case  4: return ((col.W << 24) | (col.R << 8) | (col.B << 16) | (col.G)); //4 = BGR
      case  5: return ((col.W << 24) | (col.G << 8) | (col.B << 16) | (col.R)); //5 = GBR
    }
    return 0;
  }

  static void cleanup(void* busPtr, uint8_t busType) {
    if (busPtr == nullptr) return;
    switch (busType) {
      case I_NONE: break;
    #ifdef ESP8266
      case I_8266_U0_NEO_3: delete (static_cast<NeoBus(Grb, Esp8266, Uart0, Ws2813)*>(busPtr)); break;
      case I_8266_U1_NEO_3: delete (static_cast<NeoBus(Grb, Esp8266, Uart1, Ws2813)*>(busPtr)); break;
      case I_8266_DM_NEO_3: delete (static_cast<NeoBus(Grb, Esp8266, Dma, 800Kbps)*>(busPtr)); break;
      case I_8266_U0_NEO_4: delete (static_cast<NeoBus(Grbw, Esp8266, Uart0, Ws2813)*>(busPtr)); break;
      case I_8266_U1_NEO_4: delete (static_cast<NeoBus(Grbw, Esp8266, Uart1, Ws2813)*>(busPtr)); break;
      case I_8266_DM_NEO_4: delete (static_cast<NeoBus(Grbw, Esp8266, Dma, 800Kbps)*>(busPtr)); break;
      case I_8266_U0_400_3: delete (static_cast<NeoBus(Grb, Esp8266, Uart0, 400Kbps)*>(busPtr)); break;
      case I_8266_U1_400_3: delete (static_cast<NeoBus(Grb, Esp8266, Uart1, 400Kbps)*>(busPtr)); break;
      case I_8266_DM_400_3: delete (static_cast<NeoBus(Grb, Esp8266, Dma, 400Kbps)*>(busPtr)); break;
      case I_8266_U0_TM1_4: delete (static_cast<NeoBus(WrgbTm1814, Esp8266, Uart0, Tm1814)*>(busPtr)); break;
      case I_8266_U1_TM1_4: delete (static_cast<NeoBus(WrgbTm1814, Esp8266, Uart1, Tm1814)*>(busPtr)); break;
      case I_8266_DM_TM1_4: delete (static_cast<NeoBus(WrgbTm1814, Esp8266, Dma, Tm1814)*>(busPtr)); break;
      case I_8266_U0_TM2_3: delete (static_cast<NeoBus(Brg, Esp8266, Uart0, Tm1829)*>(busPtr)); break;
      case I_8266_U1_TM2_3: delete (static_cast<NeoBus(Brg, Esp8266, Uart1, Tm1829)*>(busPtr)); break;
      case I_8266_DM_TM2_3: delete (static_cast<NeoBus(Brg, Esp8266, Dma, Tm1829)*>(busPtr)); break;
      case I_8266_U0_UCS_3: delete (static_cast<NeoBus(RgbUcs8903, Esp8266, Uart0, Ws2813)*>(busPtr)); break;
      case I_8266_U1_UCS_3: delete (static_cast<NeoBus(RgbUcs8903, Esp8266, Uart1, Ws2813)*>(busPtr)); break;
      case I_8266_DM_UCS_3: delete (static_cast<NeoBus(RgbUcs8903, Esp8266, Dma, 800Kbps)*>(busPtr)); break;
      case I_8266_U0_UCS_4: delete (static_cast<NeoBus(RgbwUcs8904, Esp8266, Uart0, Ws2813)*>(busPtr)); break;
      case I_8266_U1_UCS_4: delete (static_cast<NeoBus(RgbwUcs8904, Esp8266, Uart1, Ws2813)*>(busPtr)); break;
      case I_8266_DM_UCS_4: delete (static_cast<NeoBus(RgbwUcs8904, Esp8266, Dma, 800Kbps)*>(busPtr)); break;
      case I_8266_U0_APA106_3: delete (static_cast<NeoBus(Rbg, Esp8266, Uart0, Apa106)*>(busPtr)); break;
      case I_8266_U1_APA106_3: delete (static_cast<NeoBus(Rbg, Esp8266, Uart1, Apa106)*>(busPtr)); break;
      case I_8266_DM_APA106_3: delete (static_cast<NeoBus(Rbg, Esp8266, Dma, Apa106)*>(busPtr)); break;
      case I_8266_U0_FW6_5: delete (static_cast<NeoBus(Grbcwx, Esp8266, Uart0, Ws2813)*>(busPtr)); break;
      case I_8266_U1_FW6_5: delete (static_cast<NeoBus(Grbcwx, Esp8266, Uart1, Ws2813)*>(busPtr)); break;
      case I_8266_DM_FW6_5: delete (static_cast<NeoBus(Grbcwx, Esp8266, Dma, 800Kbps)*>(busPtr)); break;
      case I_8266_U0_2805_5: delete (static_cast<NeoBus(Grbww, Esp8266, Uart0, Ws2805)*>(busPtr)); break;
      case I_8266_U1_2805_5: delete (static_cast<NeoBus(Grbww, Esp8266, Uart1, Ws2805)*>(busPtr)); break;
      case I_8266_DM_2805_5: delete (static_cast<NeoBus(Grbww, Esp8266, Dma, Ws2805)*>(busPtr)); break;
      case I_8266_U0_TM1914_3: delete (static_cast<NeoBus(RgbTm1914, Esp8266, Uart0, Tm1914)*>(busPtr)); break;
      case I_8266_U1_TM1914_3: delete (static_cast<NeoBus(RgbTm1914, Esp8266, Uart1, Tm1914)*>(busPtr)); break;
      case I_8266_DM_TM1914_3: delete (static_cast<NeoBus(RgbTm1914, Esp8266, Dma, Tm1914)*>(busPtr)); break;
      case I_8266_U0_SM16825_5: delete (static_cast<NeoBus(RgbwcSm16825e, Esp8266, Uart0, Ws2813)*>(busPtr)); break;
      case I_8266_U1_SM16825_5: delete (static_cast<NeoBus(RgbwcSm16825e, Esp8266, Uart1, Ws2813)*>(busPtr)); break;
      case I_8266_DM_SM16825_5: delete (static_cast<NeoBus(RgbwcSm16825e, Esp8266, Dma, 800Kbps)*>(busPtr)); break;
      case I_8266_U0_NEODUAL_4: delete (static_cast<NeoBus(Rgbwxx, Esp8266, Uart0, Ws2813)*>(busPtr)); break;
      case I_8266_U1_NEODUAL_4: delete (static_cast<NeoBus(Rgbwxx, Esp8266, Uart1, Ws2813)*>(busPtr)); break;
      case I_8266_DM_NEODUAL_4: delete (static_cast<NeoBus(Rgbwxx, Esp8266, Dma, 800Kbps)*>(busPtr)); break;
    #endif
    #ifdef ARDUINO_ARCH_ESP32
      // RMT buses
      case I_32_RN_NEO_3: delete (static_cast<NeoBus(Grb, Esp32, RmtN, Ws2812x)*>(busPtr)); break;
      case I_32_RN_NEO_4: delete (static_cast<NeoBus(Grbw, Esp32, RmtN, Sk6812)*>(busPtr)); break;
      case I_32_RN_400_3: delete (static_cast<NeoBus(Grb, Esp32, RmtN, 400Kbps)*>(busPtr)); break;
      case I_32_RN_TM1_4: delete (static_cast<NeoBus(WrgbTm1814, Esp32, RmtN, Tm1814)*>(busPtr)); break;
      case I_32_RN_TM2_3: delete (static_cast<NeoBus(Brg, Esp32, RmtN, Tm1829)*>(busPtr)); break;
      case I_32_RN_UCS_3: delete (static_cast<NeoBus(RgbUcs8903, Esp32, RmtN, Ws2812x)*>(busPtr)); break;
      case I_32_RN_UCS_4: delete (static_cast<NeoBus(RgbwUcs8904, Esp32, RmtN, Ws2812x)*>(busPtr)); break;
      case I_32_RN_APA106_3: delete (static_cast<NeoBus(Grb, Esp32, RmtN, Apa106)*>(busPtr)); break;
      case I_32_RN_FW6_5: delete (static_cast<NeoBus(Grbcwx, Esp32, RmtN, Ws2812x)*>(busPtr)); break;
      case I_32_RN_2805_5: delete (static_cast<NeoBus(Grbww, Esp32, RmtN, Ws2805)*>(busPtr)); break;
      case I_32_RN_TM1914_3: delete (static_cast<NeoBus(GrbTm1914, Esp32, RmtN, Tm1914)*>(busPtr)); break;
      case I_32_RN_SM16825_5: delete (static_cast<NeoBus(RgbcwSm16825e, Esp32, RmtN, Ws2812x)*>(busPtr)); break;
      case I_32_RN_NEODUAL_4: delete (static_cast<NeoBus(Rgbwxx, Esp32, RmtN, Ws2812x)*>(busPtr)); break;
      // I2S1 bus or paralell buses
      #ifndef CONFIG_IDF_TARGET_ESP32C3
      case I_32_I2_NEO_3: if (_useParallelI2S) delete (static_cast<NeoBus(Grb, Esp32, I2s1X8, Ws2812x)*>(busPtr)); else delete (static_cast<NeoBus(Grb, Esp32, I2s1, Ws2812x)*>(busPtr)); break;
      case I_32_I2_NEO_4: if (_useParallelI2S) delete (static_cast<NeoBus(Grbw, Esp32, I2s1X8, Sk6812)*>(busPtr)); else delete (static_cast<NeoBus(Grbw, Esp32, I2s1, Sk6812)*>(busPtr)); break;
      case I_32_I2_400_3: if (_useParallelI2S) delete (static_cast<NeoBus(Grb, Esp32, I2s1X8, 400Kbps)*>(busPtr)); else delete (static_cast<NeoBus(Grb, Esp32, I2s1, 400Kbps)*>(busPtr)); break;
      case I_32_I2_TM1_4: if (_useParallelI2S) delete (static_cast<NeoBus(WrgbTm1814, Esp32, I2s1X8, Tm1814)*>(busPtr)); else delete (static_cast<NeoBus(WrgbTm1814, Esp32, I2s1, Tm1814)*>(busPtr)); break;
      case I_32_I2_TM2_3: if (_useParallelI2S) delete (static_cast<NeoBus(Brg, Esp32, I2s1X8, Tm1829)*>(busPtr)); else delete (static_cast<NeoBus(Brg, Esp32, I2s1, Tm1829)*>(busPtr)); break;
      case I_32_I2_UCS_3: if (_useParallelI2S) delete (static_cast<NeoBus(RgbUcs8903, Esp32, I2s1X8, 800Kbps)*>(busPtr)); else delete (static_cast<NeoBus(RgbUcs8903, Esp32, I2s1, 800Kbps)*>(busPtr)); break;
      case I_32_I2_UCS_4: if (_useParallelI2S) delete (static_cast<NeoBus(RgbwUcs8904, Esp32, I2s1X8, 800Kbps)*>(busPtr)); else delete (static_cast<NeoBus(RgbwUcs8904, Esp32, I2s1, 800Kbps)*>(busPtr)); break;
      case I_32_I2_APA106_3: if (_useParallelI2S) delete (static_cast<NeoBus(Grb, Esp32, I2s1X8, Apa106)*>(busPtr)); else delete (static_cast<NeoBus(Grb, Esp32, I2s1, Apa106)*>(busPtr)); break;
      case I_32_I2_FW6_5: if (_useParallelI2S) delete (static_cast<NeoBus(Grbcwx, Esp32, I2s1X8, 800Kbps)*>(busPtr)); else delete (static_cast<NeoBus(Grbcwx, Esp32, I2s1, 800Kbps)*>(busPtr)); break;
      case I_32_I2_2805_5: if (_useParallelI2S) delete (static_cast<NeoBus(Grbww, Esp32, I2s1X8, Ws2805)*>(busPtr)); else delete (static_cast<NeoBus(Grbww, Esp32, I2s1, Ws2805)*>(busPtr)); break;
      case I_32_I2_TM1914_3: if (_useParallelI2S) delete (static_cast<NeoBus(GrbTm1914, Esp32, I2s1X8, Tm1914)*>(busPtr)); else delete (static_cast<NeoBus(GrbTm1914, Esp32, I2s1, Tm1914)*>(busPtr)); break;
      case I_32_I2_SM16825_5: if (_useParallelI2S) delete (static_cast<NeoBus(RgbcwSm16825e, Esp32, I2s1X8, Ws2812x)*>(busPtr)); else delete (static_cast<NeoBus(RgbcwSm16825e, Esp32, I2s1, Ws2812x)*>(busPtr)); break;
      case I_32_I2_NEODUAL_4: if (_useParallelI2S) delete (static_cast<NeoBus(Rgbwxx, Esp32, I2s1X8, Ws2812x)*>(busPtr)); else delete (static_cast<NeoBus(Rgbwxx, Esp32, I2s1, Ws2812x)*>(busPtr)); break;
      #endif
    #endif
      case I_HS_DOT_3: delete (static_cast<TwoPinBus(DotStarBgr, DotStarSpiHz)*>(busPtr)); break;
      case I_SS_DOT_3: delete (static_cast<TwoPinBus(DotStarBgr, DotStar)*>(busPtr)); break;
      case I_HS_LPD_3: delete (static_cast<TwoPinBus(Lpd8806Grb, Lpd8806SpiHz)*>(busPtr)); break;
      case I_SS_LPD_3: delete (static_cast<TwoPinBus(Lpd8806Grb, Lpd8806)*>(busPtr)); break;
      case I_HS_LPO_3: delete (static_cast<TwoPinBus(Lpd6803Grb, Lpd6803SpiHz)*>(busPtr)); break;
      case I_SS_LPO_3: delete (static_cast<TwoPinBus(Lpd6803Grb, Lpd6803)*>(busPtr)); break;
      case I_HS_WS1_3: delete (static_cast<TwoPinBus(NeoRbg, Ws2801SpiHz)*>(busPtr)); break;
      case I_SS_WS1_3: delete (static_cast<TwoPinBus(NeoRbg, Ws2801)*>(busPtr)); break;
      case I_HS_P98_3: delete (static_cast<TwoPinBus(P9813Bgr, P9813SpiHz)*>(busPtr)); break;
      case I_SS_P98_3: delete (static_cast<TwoPinBus(P9813Bgr, P9813)*>(busPtr)); break;
      case I_HS_HD1_3: delete (static_cast<TwoPinBus(NeoBgr48, Hd108SpiHz)*>(busPtr)); break;
      case I_SS_HD1_3: delete (static_cast<TwoPinBus(NeoBgr48, Hd108)*>(busPtr)); break;
    }
  }

  static unsigned getDataSize(void* busPtr, uint8_t busType) {
    unsigned size = 0;
    switch (busType) {
      case I_NONE: break;
    #ifdef ESP8266
      case I_8266_U0_NEO_3: size = (static_cast<NeoBus(Grb, Esp8266, Uart0, Ws2813)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_U1_NEO_3: size = (static_cast<NeoBus(Grb, Esp8266, Uart1, Ws2813)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_DM_NEO_3: size = (static_cast<NeoBus(Grb, Esp8266, Dma, 800Kbps)*>(busPtr))->PixelsSize()*5; break;
      case I_8266_U0_NEO_4: size = (static_cast<NeoBus(Grbw, Esp8266, Uart0, Ws2813)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_U1_NEO_4: size = (static_cast<NeoBus(Grbw, Esp8266, Uart1, Ws2813)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_DM_NEO_4: size = (static_cast<NeoBus(Grbw, Esp8266, Dma, 800Kbps)*>(busPtr))->PixelsSize()*5; break;
      case I_8266_U0_400_3: size = (static_cast<NeoBus(Grb, Esp8266, Uart0, 400Kbps)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_U1_400_3: size = (static_cast<NeoBus(Grb, Esp8266, Uart1, 400Kbps)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_DM_400_3: size = (static_cast<NeoBus(Grb, Esp8266, Dma, 400Kbps)*>(busPtr))->PixelsSize()*5; break;
      case I_8266_U0_TM1_4: size = (static_cast<NeoBus(WrgbTm1814, Esp8266, Uart0, Tm1814)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_U1_TM1_4: size = (static_cast<NeoBus(WrgbTm1814, Esp8266, Uart1, Tm1814)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_DM_TM1_4: size = (static_cast<NeoBus(WrgbTm1814, Esp8266, Dma, Tm1814)*>(busPtr))->PixelsSize()*5; break;
      case I_8266_U0_TM2_3: size = (static_cast<NeoBus(Brg, Esp8266, Uart0, Tm1829)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_U1_TM2_3: size = (static_cast<NeoBus(Brg, Esp8266, Uart1, Tm1829)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_DM_TM2_3: size = (static_cast<NeoBus(Brg, Esp8266, Dma, Tm1829)*>(busPtr))->PixelsSize()*5; break;
      case I_8266_U0_UCS_3: size = (static_cast<NeoBus(RgbUcs8903, Esp8266, Uart0, Ws2813)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_U1_UCS_3: size = (static_cast<NeoBus(RgbUcs8903, Esp8266, Uart1, Ws2813)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_DM_UCS_3: size = (static_cast<NeoBus(RgbUcs8903, Esp8266, Dma, 800Kbps)*>(busPtr))->PixelsSize()*5; break;
      case I_8266_U0_UCS_4: size = (static_cast<NeoBus(RgbwUcs8904, Esp8266, Uart0, Ws2813)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_U1_UCS_4: size = (static_cast<NeoBus(RgbwUcs8904, Esp8266, Uart1, Ws2813)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_DM_UCS_4: size = (static_cast<NeoBus(RgbwUcs8904, Esp8266, Dma, 800Kbps)*>(busPtr))->PixelsSize()*5; break;
      case I_8266_U0_APA106_3: size = (static_cast<NeoBus(Rbg, Esp8266, Uart0, Apa106)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_U1_APA106_3: size = (static_cast<NeoBus(Rbg, Esp8266, Uart1, Apa106)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_DM_APA106_3: size = (static_cast<NeoBus(Rbg, Esp8266, Dma, Apa106)*>(busPtr))->PixelsSize()*5; break;
      case I_8266_U0_FW6_5: size = (static_cast<NeoBus(Grbcwx, Esp8266, Uart0, Ws2813)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_U1_FW6_5: size = (static_cast<NeoBus(Grbcwx, Esp8266, Uart1, Ws2813)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_DM_FW6_5: size = (static_cast<NeoBus(Grbcwx, Esp8266, Dma, 800Kbps)*>(busPtr))->PixelsSize()*5; break;
      case I_8266_U0_2805_5: size = (static_cast<NeoBus(Grbww, Esp8266, Uart0, Ws2805)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_U1_2805_5: size = (static_cast<NeoBus(Grbww, Esp8266, Uart1, Ws2805)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_DM_2805_5: size = (static_cast<NeoBus(Grbww, Esp8266, Dma, Ws2805)*>(busPtr))->PixelsSize()*5; break;
      case I_8266_U0_TM1914_3: size = (static_cast<NeoBus(RgbTm1914, Esp8266, Uart0, Tm1914)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_U1_TM1914_3: size = (static_cast<NeoBus(RgbTm1914, Esp8266, Uart1, Tm1914)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_DM_TM1914_3: size = (static_cast<NeoBus(RgbTm1914, Esp8266, Dma, Tm1914)*>(busPtr))->PixelsSize()*5; break;
      case I_8266_U0_SM16825_5: size = (static_cast<NeoBus(RgbwcSm16825e, Esp8266, Uart0, Ws2813)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_U1_SM16825_5: size = (static_cast<NeoBus(RgbwcSm16825e, Esp8266, Uart1, Ws2813)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_DM_SM16825_5: size = (static_cast<NeoBus(RgbwcSm16825e, Esp8266, Dma, 800Kbps)*>(busPtr))->PixelsSize()*5; break;
      case I_8266_U0_NEODUAL_4: size = (static_cast<NeoBus(Rgbwxx, Esp8266, Uart0, Ws2813)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_U1_NEODUAL_4: size = (static_cast<NeoBus(Rgbwxx, Esp8266, Uart1, Ws2813)*>(busPtr))->PixelsSize()*2; break;
      case I_8266_DM_NEODUAL_4: size = (static_cast<NeoBus(Rgbwxx, Esp8266, Dma, 800Kbps)*>(busPtr))->PixelsSize()*5; break;
    #endif
    #ifdef ARDUINO_ARCH_ESP32
      // RMT buses (front + back + small system managed RMT)
      case I_32_RN_NEO_3: size = (static_cast<NeoBus(Grb, Esp32, RmtN, Ws2812x)*>(busPtr))->PixelsSize()*2; break;
      case I_32_RN_NEO_4: size = (static_cast<NeoBus(Grbw, Esp32, RmtN, Sk6812)*>(busPtr))->PixelsSize()*2; break;
      case I_32_RN_400_3: size = (static_cast<NeoBus(Grb, Esp32, RmtN, 400Kbps)*>(busPtr))->PixelsSize()*2; break;
      case I_32_RN_TM1_4: size = (static_cast<NeoBus(WrgbTm1814, Esp32, RmtN, Tm1814)*>(busPtr))->PixelsSize()*2; break;
      case I_32_RN_TM2_3: size = (static_cast<NeoBus(Brg, Esp32, RmtN, Tm1829)*>(busPtr))->PixelsSize()*2; break;
      case I_32_RN_UCS_3: size = (static_cast<NeoBus(RgbUcs8903, Esp32, RmtN, Ws2812x)*>(busPtr))->PixelsSize()*2; break;
      case I_32_RN_UCS_4: size = (static_cast<NeoBus(RgbwUcs8904, Esp32, RmtN, Ws2812x)*>(busPtr))->PixelsSize()*2; break;
      case I_32_RN_APA106_3: size = (static_cast<NeoBus(Grb, Esp32, RmtN, Apa106)*>(busPtr))->PixelsSize()*2; break;
      case I_32_RN_FW6_5: size = (static_cast<NeoBus(Grbcwx, Esp32, RmtN, Ws2812x)*>(busPtr))->PixelsSize()*2; break;
      case I_32_RN_2805_5: size = (static_cast<NeoBus(Grbww, Esp32, RmtN, Ws2805)*>(busPtr))->PixelsSize()*2; break;
      case I_32_RN_TM1914_3: size = (static_cast<NeoBus(GrbTm1914, Esp32, RmtN, Tm1914)*>(busPtr))->PixelsSize()*2; break;
      case I_32_RN_SM16825_5: size = (static_cast<NeoBus(RgbcwSm16825e, Esp32, RmtN, Ws2812x)*>(busPtr))->PixelsSize()*2; break;
      case I_32_RN_NEODUAL_4: size = (static_cast<NeoBus(Rgbwxx, Esp32, RmtN, Ws2812x)*>(busPtr))->PixelsSize()*2; break;
      // I2S1 bus or paralell buses (front + DMA; DMA = front * cadence, aligned to 4 bytes)
      #ifndef CONFIG_IDF_TARGET_ESP32C3
      case I_32_I2_NEO_3: size = (_useParallelI2S) ? (static_cast<NeoBus(Grb, Esp32, I2s1X8, Ws2812x)*>(busPtr))->PixelsSize()*4 : (static_cast<NeoBus(Grb, Esp32, I2s1, Ws2812x)*>(busPtr))->PixelsSize()*4; break;
      case I_32_I2_NEO_4: size = (_useParallelI2S) ? (static_cast<NeoBus(Grbw, Esp32, I2s1X8, Sk6812)*>(busPtr))->PixelsSize()*4 : (static_cast<NeoBus(Grbw, Esp32, I2s1, Sk6812)*>(busPtr))->PixelsSize()*4; break;
      case I_32_I2_400_3: size = (_useParallelI2S) ? (static_cast<NeoBus(Grb, Esp32, I2s1X8, 400Kbps)*>(busPtr))->PixelsSize()*4 : (static_cast<NeoBus(Grb, Esp32, I2s1, 400Kbps)*>(busPtr))->PixelsSize()*4; break;
      case I_32_I2_TM1_4: size = (_useParallelI2S) ? (static_cast<NeoBus(WrgbTm1814, Esp32, I2s1X8, Tm1814)*>(busPtr))->PixelsSize()*4 : (static_cast<NeoBus(WrgbTm1814, Esp32, I2s1, Tm1814)*>(busPtr))->PixelsSize()*4; break;
      case I_32_I2_TM2_3: size = (_useParallelI2S) ? (static_cast<NeoBus(Brg, Esp32, I2s1X8, Tm1829)*>(busPtr))->PixelsSize()*4 : (static_cast<NeoBus(Brg, Esp32, I2s1, Tm1829)*>(busPtr))->PixelsSize()*4; break;
      case I_32_I2_UCS_3: size = (_useParallelI2S) ? (static_cast<NeoBus(RgbUcs8903, Esp32, I2s1X8, 800Kbps)*>(busPtr))->PixelsSize()*4 : (static_cast<NeoBus(RgbUcs8903, Esp32, I2s1, 800Kbps)*>(busPtr))->PixelsSize()*4; break;
      case I_32_I2_UCS_4: size = (_useParallelI2S) ? (static_cast<NeoBus(RgbwUcs8904, Esp32, I2s1X8, 800Kbps)*>(busPtr))->PixelsSize()*4 : (static_cast<NeoBus(RgbwUcs8904, Esp32, I2s1, 800Kbps)*>(busPtr))->PixelsSize()*4; break;
      case I_32_I2_APA106_3: size = (_useParallelI2S) ? (static_cast<NeoBus(Grb, Esp32, I2s1X8, Apa106)*>(busPtr))->PixelsSize()*4 : (static_cast<NeoBus(Grb, Esp32, I2s1, Apa106)*>(busPtr))->PixelsSize()*4; break;
      case I_32_I2_FW6_5: size = (_useParallelI2S) ? (static_cast<NeoBus(Grbcwx, Esp32, I2s1X8, 800Kbps)*>(busPtr))->PixelsSize()*4 : (static_cast<NeoBus(Grbcwx, Esp32, I2s1, 800Kbps)*>(busPtr))->PixelsSize()*4; break;
      case I_32_I2_2805_5: size = (_useParallelI2S) ? (static_cast<NeoBus(Grbww, Esp32, I2s1X8, Ws2805)*>(busPtr))->PixelsSize()*4 : (static_cast<NeoBus(Grbww, Esp32, I2s1, Ws2805)*>(busPtr))->PixelsSize()*4; break;
      case I_32_I2_TM1914_3: size = (_useParallelI2S) ? (static_cast<NeoBus(GrbTm1914, Esp32, I2s1X8, Tm1914)*>(busPtr))->PixelsSize()*4 : (static_cast<NeoBus(GrbTm1914, Esp32, I2s1, Tm1914)*>(busPtr))->PixelsSize()*4; break;
      case I_32_I2_SM16825_5: size = (_useParallelI2S) ? (static_cast<NeoBus(RgbcwSm16825e, Esp32, I2s1X8, Ws2812x)*>(busPtr))->PixelsSize()*4 : (static_cast<NeoBus(RgbcwSm16825e, Esp32, I2s1, Ws2812x)*>(busPtr))->PixelsSize()*4; break;
      case I_32_I2_NEODUAL_4: size = (_useParallelI2S) ? (static_cast<NeoBus(Rgbwxx, Esp32, I2s1X8, Ws2812x)*>(busPtr))->PixelsSize()*4 : (static_cast<NeoBus(Rgbwxx, Esp32, I2s1, Ws2812x)*>(busPtr))->PixelsSize()*4; break;
      #endif
    #endif
      case I_HS_DOT_3: size = (static_cast<TwoPinBus(DotStarBgr, DotStarSpiHz)*>(busPtr))->PixelsSize()*2; break;
      case I_SS_DOT_3: size = (static_cast<TwoPinBus(DotStarBgr, DotStar)*>(busPtr))->PixelsSize()*2; break;
      case I_HS_LPD_3: size = (static_cast<TwoPinBus(Lpd8806Grb, Lpd8806SpiHz)*>(busPtr))->PixelsSize()*2; break;
      case I_SS_LPD_3: size = (static_cast<TwoPinBus(Lpd8806Grb, Lpd8806)*>(busPtr))->PixelsSize()*2; break;
      case I_HS_LPO_3: size = (static_cast<TwoPinBus(Lpd6803Grb, Lpd6803SpiHz)*>(busPtr))->PixelsSize()*2; break;
      case I_SS_LPO_3: size = (static_cast<TwoPinBus(Lpd6803Grb, Lpd6803)*>(busPtr))->PixelsSize()*2; break;
      case I_HS_WS1_3: size = (static_cast<TwoPinBus(NeoRbg, Ws2801SpiHz)*>(busPtr))->PixelsSize()*2; break;
      case I_SS_WS1_3: size = (static_cast<TwoPinBus(NeoRbg, Ws2801)*>(busPtr))->PixelsSize()*2; break;
      case I_HS_P98_3: size = (static_cast<TwoPinBus(P9813Bgr, P9813SpiHz)*>(busPtr))->PixelsSize()*2; break;
      case I_SS_P98_3: size = (static_cast<TwoPinBus(P9813Bgr, P9813)*>(busPtr))->PixelsSize()*2; break;
      case I_HS_HD1_3: size = (static_cast<TwoPinBus(NeoBgr48, Hd108SpiHz)*>(busPtr))->PixelsSize()*2; break;
      case I_SS_HD1_3: size = (static_cast<TwoPinBus(NeoBgr48, Hd108)*>(busPtr))->PixelsSize()*2; break;
    }
    return size;
  }

  static unsigned memUsage(unsigned count, unsigned busType) {
    unsigned size = count*3;  // let's assume 3 channels, we will add count or 2*count below for 4 channels or 5 channels
    switch (busType) {
      case I_NONE: size = 0; break;
    #ifdef ESP8266
      // UART methods have front + back buffers + small UART
      case I_8266_U0_NEO_4    : // fallthrough
      case I_8266_U1_NEO_4    : // fallthrough
      case I_8266_U0_TM1_4    : // fallthrough
      case I_8266_U1_TM1_4    : size = (size + count);       break; // 4 channels
      case I_8266_U0_NEODUAL_4: // fallthrough; 4 channels, dual 3-ch chip
      case I_8266_U1_NEODUAL_4: // fallthrough; 4 channels, dual 3-ch chip
      case I_8266_U0_UCS_3    : // fallthrough
      case I_8266_U1_UCS_3    : size *= 2;                   break; // 16 bit
      case I_8266_U0_UCS_4    : // fallthrough
      case I_8266_U1_UCS_4    : size = (size + count)*2;     break; // 16 bit 4 channels
      case I_8266_U0_FW6_5    : // fallthrough
      case I_8266_U1_FW6_5    : // fallthrough
      case I_8266_U0_2805_5   : // fallthrough
      case I_8266_U1_2805_5   : size = (size + 2*count);     break; // 5 channels
      case I_8266_U0_SM16825_5: // fallthrough
      case I_8266_U1_SM16825_5: size = (size + 2*count)*2;   break; // 16 bit 5 channels
      // DMA methods have front + DMA buffer = ((1+(3+1)) * channels; exact value is a bit of mistery - needs a dig into NPB)
      case I_8266_DM_NEO_3    : // fallthrough
      case I_8266_DM_400_3    : // fallthrough
      case I_8266_DM_TM2_3    : // fallthrough
      case I_8266_DM_APA106_3 : // fallthrough
      case I_8266_DM_TM1914_3 : size *= 5;                   break;
      case I_8266_DM_NEO_4    : // fallthrough
      case I_8266_DM_TM1_4    : size = (size + count)*5;     break;
      case I_8266_DM_NEODUAL_4: // fallthrough; 4 channels, dual 3-ch chip
      case I_8266_DM_UCS_3    : size *= 2*5;                 break;
      case I_8266_DM_UCS_4    : size = (size + count)*2*5;   break;
      case I_8266_DM_FW6_5    : // fallthrough
      case I_8266_DM_2805_5   : size = (size + 2*count)*5;   break;
      case I_8266_DM_SM16825_5: size = (size + 2*count)*2*5; break;
    #else
      // RMT buses (1x front and 1x back buffer, does not include small RMT buffer)
      case I_32_RN_NEO_4    : // fallthrough
      case I_32_RN_TM1_4    : size = (size + count)*2;     break; // 4 channels
      case I_32_RN_NEODUAL_4: // fallthrough; 4 channels, dual 3-ch chip
      case I_32_RN_UCS_3    : size *= 2*2;                 break; // 16bit
      case I_32_RN_UCS_4    : size = (size + count)*2*2;   break; // 16bit, 4 channels
      case I_32_RN_FW6_5    : // fallthrough
      case I_32_RN_2805_5   : size = (size + 2*count)*2;   break; // 5 channels
      case I_32_RN_SM16825_5: size = (size + 2*count)*2*2; break; // 16bit, 5 channels
      // I2S1 bus or paralell I2S1 buses (1x front, does not include DMA buffer which is front*cadence, a bit(?) more for LCD)
      #ifndef CONFIG_IDF_TARGET_ESP32C3
      case I_32_I2_NEO_3    : // fallthrough
      case I_32_I2_400_3    : // fallthrough
      case I_32_I2_TM2_3    : // fallthrough
      case I_32_I2_APA106_3 :                              break; // do nothing, I2S uses single buffer + DMA buffer
      case I_32_I2_NEO_4    : // fallthrough
      case I_32_I2_TM1_4    : size = (size + count);       break; // 4 channels
      case I_32_I2_NEODUAL_4: // fallthrough; 4 channels, dual 3-ch chip
      case I_32_I2_UCS_3    : size *= 2;                   break; // 16 bit
      case I_32_I2_UCS_4    : size = (size + count)*2;     break; // 16 bit, 4 channels
      case I_32_I2_FW6_5    : // fallthrough
      case I_32_I2_2805_5   : size = (size + 2*count);     break; // 5 channels
      case I_32_I2_SM16825_5: size = (size + 2*count)*2;   break; // 16 bit, 5 channels
      #endif
      default               : size *= 2;                   break; // everything else uses 2 buffers
    #endif
      case I_HS_HD1_3: // fallthrough; 16 bit
      case I_SS_HD1_3: size *= 2; break;
    }
    return size;
  }

  //gives back the internal type index (I_XX_XXX_X above) for the input
  static uint8_t getI(uint8_t busType, const uint8_t* pins, uint8_t num = 0) {
    if (!Bus::isDigital(busType)) return I_NONE;
    if (Bus::is2Pin(busType)) { //SPI LED chips
      bool isHSPI = false;
      #ifdef ESP8266
      if (pins[0] == P_8266_HS_MOSI && pins[1] == P_8266_HS_CLK) isHSPI = true;
      #else
      // temporary hack to limit use of hardware SPI to a single SPI peripheral (HSPI): only allow ESP32 hardware serial on segment 0
      // SPI global variable is normally linked to VSPI on ESP32 (or FSPI C3, S3)
      if (!num) isHSPI = true;
      #endif
      uint8_t t = I_NONE;
      switch (busType) {
        case TYPE_APA102:  t = I_SS_DOT_3; break;
        case TYPE_LPD8806: t = I_SS_LPD_3; break;
        case TYPE_LPD6803: t = I_SS_LPO_3; break;
        case TYPE_WS2801:  t = I_SS_WS1_3; break;
        case TYPE_P9813:   t = I_SS_P98_3; break;
        case TYPE_HD108:   t = I_SS_HD1_3; break;
      }
      if (t > I_NONE && isHSPI) t--; //hardware SPI has one smaller ID than software
      return t;
    } else {
      #ifdef ESP8266
      uint8_t offset = pins[0] -1; //for driver: (GPIO1)0 = uart0, (GPIO2)1 = uart1, (GPIO3)2 = dma, >=3 = bitbang (no longer supported)
      if (offset > 2) return I_NONE;
      switch (busType) {
        case TYPE_WS2812_1CH_X3:
        case TYPE_WS2812_2CH_X3:
        case TYPE_WS2812_RGB:
        case TYPE_WS2812_WWA:
          return I_8266_U0_NEO_3 + offset;
        case TYPE_SK6812_RGBW:
          return I_8266_U0_NEO_4 + offset;
        case TYPE_WS2811_400KHZ:
          return I_8266_U0_400_3 + offset;
        case TYPE_TM1814:
          return I_8266_U0_TM1_4 + offset;
        case TYPE_TM1829:
          return I_8266_U0_TM2_3 + offset;
        case TYPE_UCS8903:
          return I_8266_U0_UCS_3 + offset;
        case TYPE_UCS8904:
          return I_8266_U0_UCS_4 + offset;
        case TYPE_APA106:
          return I_8266_U0_APA106_3 + offset;
        case TYPE_FW1906:
          return I_8266_U0_FW6_5 + offset;
        case TYPE_WS2805:
          return I_8266_U0_2805_5 + offset;
        case TYPE_TM1914:
          return I_8266_U0_TM1914_3 + offset;
        case TYPE_SM16825:
          return I_8266_U0_SM16825_5 + offset;
        case TYPE_WS281X_DUAL:
          return I_8266_U0_NEODUAL_4 + offset;
      }
      #else //ESP32
      uint8_t offset = 0; // 0 = RMT (num 1-8), 1 = I2S1 [I2S0 is used by Audioreactive]
      #if defined(CONFIG_IDF_TARGET_ESP32S2)
      // ESP32-S2 only has 4 RMT channels
      if (_useParallelI2S) {
        if (num > 11) return I_NONE;
        if (num < 8) offset = 1;  // use x8 parallel I2S0 channels then RMT
      } else {
        if (num > 4) return I_NONE;
        if (num > 3) offset = 1;  // only one I2S0 (use last to allow Audioreactive)
      }
      #elif defined(CONFIG_IDF_TARGET_ESP32C3)
      // On ESP32-C3 only the first 2 RMT channels are usable for transmitting
      if (num > 1) return I_NONE;
      //if (num > 1) offset = 1; // I2S not supported yet (only 1 I2S)
      #elif defined(CONFIG_IDF_TARGET_ESP32S3)
      // On ESP32-S3 only the first 4 RMT channels are usable for transmitting
      if (_useParallelI2S) {
        if (num > 11) return I_NONE;
        if (num < 8) offset = 1;    // use x8 parallel I2S LCD channels
      } else {
        if (num > 3) return I_NONE; // do not use single I2S (as it is not supported)
      }
      #else
      // standard ESP32 has 8 RMT and x1/x8 I2S1 channels
      if (_useParallelI2S) {
        if (num > 15) return I_NONE;
        if (num < 8) offset = 1;  // 8 I2S followed by 8 RMT
      } else {
        if (num > 9) return I_NONE;
        if (num == 0) offset = 1; // prefer I2S1 for 1st bus (less flickering but more RAM needed)
      }
      #endif
      switch (busType) {
        case TYPE_WS2812_1CH_X3:
        case TYPE_WS2812_2CH_X3:
        case TYPE_WS2812_RGB:
        case TYPE_WS2812_WWA:
          return I_32_RN_NEO_3 + offset;
        case TYPE_SK6812_RGBW:
          return I_32_RN_NEO_4 + offset;
        case TYPE_WS2811_400KHZ:
          return I_32_RN_400_3 + offset;
        case TYPE_TM1814:
          return I_32_RN_TM1_4 + offset;
        case TYPE_TM1829:
          return I_32_RN_TM2_3 + offset;
        case TYPE_UCS8903:
          return I_32_RN_UCS_3 + offset;
        case TYPE_UCS8904:
          return I_32_RN_UCS_4 + offset;
        case TYPE_APA106:
          return I_32_RN_APA106_3 + offset;
        case TYPE_FW1906:
          return I_32_RN_FW6_5 + offset;
        case TYPE_WS2805:
          return I_32_RN_2805_5 + offset;
        case TYPE_TM1914:
          return I_32_RN_TM1914_3 + offset;
        case TYPE_SM16825:
          return I_32_RN_SM16825_5 + offset;
        case TYPE_WS281X_DUAL:
          return I_32_RN_NEODUAL_4 + offset;
      }
      #endif
    }
    return I_NONE;
  }
};
#endif
