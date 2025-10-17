#pragma once
#ifndef WLED_COLORS_H
#define WLED_COLORS_H

#include <FastLED.h>

//color mangling macros
#define RGBW32(r,g,b,w) (uint32_t((byte(w) << 24) | (byte(r) << 16) | (byte(g) << 8) | (byte(b))))
#define R(c) (byte((c) >> 16))
#define G(c) (byte((c) >> 8))
#define B(c) (byte(c))
#define W(c) (byte((c) >> 24))

struct CHSV32; // forward declaration
struct CRGBA;  // forward declaration

// similar to NeoPixelBus NeoGammaTableMethod but allows dynamic changes (superseded by NPB::NeoGammaDynamicTableMethod)
class NeoGammaWLEDMethod {
  public:
    static inline uint8_t Correct(uint8_t value)        { return gammaT[value]; };  // apply Gamma to single channel
    [[gnu::hot]] static uint32_t Correct32(uint32_t color);                         // apply Gamma to RGBW32 color (WLED specific, not used by NPB)
    [[gnu::hot]] static uint32_t inverseGamma32(uint32_t color);                    // apply inverse Gamma to RGBW32 color
    static void calcGammaTable(float gamma);                                        // re-calculates & fills gamma tables
    static inline uint8_t rawGamma8(uint8_t val)        { return gammaT[val]; }     // get value from Gamma table (WLED specific, not used by NPB)
    static inline uint8_t rawInverseGamma8(uint8_t val) { return gammaT_inv[val]; } // get value from inverse Gamma table (WLED specific, not used by NPB)
  private:
    static uint8_t gammaT[];
    static uint8_t gammaT_inv[];
};
#define gamma32(c) NeoGammaWLEDMethod::Correct32(c)
#define gamma8(c)  NeoGammaWLEDMethod::rawGamma8(c)
#define gamma32inv(c) NeoGammaWLEDMethod::inverseGamma32(c)
#define gamma8inv(c)  NeoGammaWLEDMethod::rawInverseGamma8(c)

// addidion, blending & scaling
[[gnu::hot, gnu::pure]] uint32_t color_blend(uint32_t c1, uint32_t c2 , uint8_t blend);
inline uint32_t color_blend16(uint32_t c1, uint32_t c2, uint16_t b) { return color_blend(c1, c2, (b >> 8) + ((uint8_t)b >> 7)); }; // add a bit of rounding
[[gnu::hot, gnu::pure]] uint32_t color_add(uint32_t c1, uint32_t c2, bool preserveCR=false);
[[gnu::hot]] void fast_color_add(uint32_t &c1, uint32_t c2, uint8_t scale = 255);
[[gnu::hot]] void fast_color_scale(uint32_t &c1, uint8_t scale);
[[gnu::hot, gnu::pure]] uint32_t color_fade(uint32_t c1, uint8_t amount, bool video=false);
#ifndef FASTLED_VERSION
inline uint8_t scale8(uint8_t i, uint8_t scale) { return (uint16_t(i) * (1 + scale)) >> 8; }
#endif

// palette functions
[[gnu::hot, gnu::pure]] CRGBA ColorFromPaletteWLED(const CRGBPalette16 &pal, uint8_t index, uint8_t brightness = (uint8_t)255U, TBlendType blendType = LINEARBLEND);
CRGBPalette16 generateHarmonicRandomPalette(const CRGBPalette16 &basepalette);
CRGBPalette16 generateRandomPalette();
void loadCustomPalettes();
#define getPaletteCount() (FIXED_PALETTE_COUNT + customPalettes.size())

// color conversion functions
inline uint32_t colorFromRgbw(byte* rgbw) { return uint32_t((byte(rgbw[3]) << 24) | (byte(rgbw[0]) << 16) | (byte(rgbw[1]) << 8) | (byte(rgbw[2]))); }
void hsv2rgb(const CHSV32& hsv, uint32_t& rgb);
void colorHStoRGB(uint16_t hue, byte sat, byte* rgb);
void rgb2hsv(const CRGBA& rgb, CHSV32& hsv);
void colorKtoRGB(uint16_t kelvin, byte* rgb);
void colorCTtoRGB(uint16_t mired, byte* rgb); //white spectrum to rgb
void colorXYtoRGB(float x, float y, byte* rgb); // only defined if huesync disabled TODO
void colorRGBtoXY(const byte* rgb, float* xy); // only defined if huesync disabled TODO
void colorFromDecOrHexString(byte* rgb, const char* in);
bool colorFromHexString(byte* rgb, const char* in);
uint32_t colorBalanceFromKelvin(uint16_t kelvin, uint32_t rgb);
uint16_t approximateKelvinFromRGB(uint32_t rgb);
void setRandomColor(byte* rgb);

// CRGBA is like FastLED's CRGB but has an alpha channel
// in some cases it can be used as RGBW (where W is alpha channel)
// in such case opacity functions will not work as expected
struct CRGBA {
  union {
      uint32_t color32; // Access as a 32-bit value (0xAARRGGBB)
      struct {
        union {
          uint8_t b;
          uint8_t blue;
        };
        union {
          uint8_t g;
          uint8_t green;
        };
        union {
          uint8_t r;
          uint8_t red;
        };
        union {
          uint8_t a;
          uint8_t alpha;
        };
      };
      uint8_t raw[4];   // Access as an array in the order B, G, R, alpha
  };

  // Default constructor
  inline CRGBA() __attribute__((always_inline)) = default;

  // Constructor from a 32-bit color (0xAARRGGBB or 0xWWRRGGBB)
  inline CRGBA(uint32_t color) __attribute__((always_inline)) : color32(color|0xFF000000) {} // assumes no alpha or white is encoded in highest byte

  // Constructor with r, g, b, alpha values (use this constructor to create RGBW color, where W is encoded in alpha channel)
  inline CRGBA(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) __attribute__((always_inline)) : b(blue), g(green), r(red), a(alpha) {}

  // Constructor from CHSV32
  inline CRGBA(const CHSV32& hsv) __attribute__((always_inline)) { hsv2rgb(hsv, color32); a = 255; }

  // set opacity (alpha channel) to a specific value (0-255)
  inline CRGBA& setOpacity(uint8_t alpha) __attribute__((always_inline)) { a = alpha; return *this; }
  inline bool   isOpaque() const __attribute__((always_inline)) { return a == 255; }
  inline bool   isTransparent() const __attribute__((always_inline)) { return a == 0; }

  // maintain compatibility with FastLED
  CRGBA& nscale8(uint8_t scale);
  inline CRGBA& nscale8_white(uint8_t scale) __attribute__((always_inline)) { fast_color_scale(color32, scale); return *this; }
  inline CRGBA  scale8(uint8_t scale) const __attribute__((always_inline)) { CRGBA c = *this; c.nscale8(scale); return c; }
  inline CRGBA  scale8_white(uint8_t scale) const __attribute__((always_inline)) { CRGBA c = *this; c.nscale8_white(scale); return c; }

  // maintain compatibility with FastLED
  inline CRGBA& nscale8_video(uint8_t scale) __attribute__((always_inline)) {
    uint32_t remains = ((uint32_t)(r && scale)<<16) | ((uint32_t)(g && scale)<<8) | (b && scale);
    nscale8(scale);
    color32 += remains; // add back the remains (1 per non-zero channel)
    return *this;
  }
  inline CRGBA& nscale8_video_white(uint8_t scale) __attribute__((always_inline)) {
    uint32_t remains = ((uint32_t)(a && scale)<<24) | ((uint32_t)(r && scale)<<16) | ((uint32_t)(g && scale)<<8) | (b && scale);
    nscale8_white(scale);
    color32 += remains; // add back the remains (1 per non-zero channel)
    return *this;
  }
  inline CRGBA scale8_video(uint8_t scale) const __attribute__((always_inline)) { CRGBA c = *this; c.nscale8_video(scale); return c; }
  inline CRGBA scale8_video_white(uint8_t scale) const __attribute__((always_inline)) { CRGBA c = *this; c.nscale8_video_white(scale); return c; }

  // blend c2 into this color by amount (0-255; 0 = no c2, 255 = full c2)
  inline CRGBA& nblend(CRGBA c2, uint8_t amount) __attribute__((always_inline)) { color32 = color_blend(color32, c2.color32, amount); return *this; }
  inline CRGBA& nblend(CRGBA c2, uint16_t amount) __attribute__((always_inline)) { return nblend(c2, (uint8_t)((amount>>8)+((amount>>7)&1))); }
  inline CRGBA  blend(const CRGBA c2, uint8_t amount) const __attribute__((always_inline)) { CRGBA c = *this; c.nblend(c2, amount); return c; }
  inline CRGBA  blend(const CRGBA c2, uint16_t amount) const __attribute__((always_inline)) { CRGBA c = *this; c.nblend(c2, (uint8_t)((amount>>8)+((amount>>7)&1))); return c; }

  // add c2 into this color, scaling c2 by its alpha channel; if preserveCR is true, the resulting color is scaled to preserve the original color ratio
  CRGBA& add(CRGBA c2, bool preserveCR = false);
  inline CRGBA& add_white(CRGBA c2, bool preserveCR = false) __attribute__((always_inline)) { color32 = color_add(color32, c2.color32, preserveCR); return *this; }
  inline CRGBA& add(uint8_t x) __attribute__((always_inline)) { return add(CRGBA(x,x,x)); }

  // Fade alpha channel (make transparent by amount)
  inline CRGBA& fadeOut(uint8_t amount) __attribute__((always_inline)) { a = ((uint16_t)a*(256-amount)) >> 8; return *this; }

  // maintain compatibility with FastLED
  inline CRGBA& fadeToBlackBy(uint8_t amount) __attribute__((always_inline)) { return nscale8(255 - amount); }

  inline uint8_t getAverageLight() const __attribute__((always_inline)) { return (uint16_t(r) + uint16_t(g) + uint16_t(b)) * uint16_t(a) / (3*255); }

  // custom operators to shorten code (see: https://en.cppreference.com/w/cpp/language/operators.html for friend operators)

  // Access as an array
  inline uint8_t& operator[](size_t x) __attribute__((always_inline)) { return raw[x]; }

  // Assignment from 32-bit color
  inline CRGBA& operator=(uint32_t color) __attribute__((always_inline)) { color32 = color; return *this; }

  // Assignment from CHSV32
  inline CRGBA& operator=(const CHSV32& hsv) __attribute__((always_inline)) { hsv2rgb(hsv, color32); a = 255; return *this;}

  // Scaling assignment
  inline CRGBA& operator*=(uint8_t scale) __attribute__((always_inline)) { return nscale8(scale); }
  inline CRGBA& operator/=(uint8_t scale) __attribute__((always_inline)) { return nscale8(255 - scale); }

  // video scaling: make sure colors do not dim to zero if they started non-zero
  inline CRGBA& operator%=(uint8_t scale) __attribute__((always_inline)) { return nscale8_video(scale); }

  // Comparison
  inline bool operator==(const CRGBA &rhs) __attribute__((always_inline))   { return color32 == rhs.color32; }
  inline bool operator==(const uint32_t rhs) __attribute__((always_inline)) { return (color32 & 0x00FFFFFF) == (rhs & 0x00FFFFFF); } // ignore white or alpha
  inline bool operator!=(const CRGBA &rhs) __attribute__((always_inline))   { return color32 != rhs.color32; }
  inline bool operator!=(const uint32_t rhs) __attribute__((always_inline)) { return (color32 & 0x00FFFFFF) != (rhs & 0x00FFFFFF); } // ignore white or alpha

  // Addition assignment with scaling of added color
  inline CRGBA& operator+=(const CRGBA& rhs) __attribute__((always_inline)) { return add(rhs, true); }
  inline CRGBA& operator+=(uint8_t x) __attribute__((always_inline))        { return add(CRGBA(x,x,x), true); }

  inline CRGBA& operator-=(const CRGBA& rhs) __attribute__((always_inline)) {
    auto qsub8 = [](uint8_t a, uint8_t b) { return a > b ? a - b : 0; };
    r = qsub8(r, rhs.r*(rhs.a+1)>>8);
    g = qsub8(g, rhs.g*(rhs.a+1)>>8);
    b = qsub8(b, rhs.b*(rhs.a+1)>>8);
    return *this;
  }

  inline CRGBA& operator-=(uint8_t x) __attribute__((always_inline)) {
    auto qsub8 = [](uint8_t a, uint8_t b) { return a > b ? a - b : 0; };
    r = qsub8(r, x);
    g = qsub8(g, x);
    b = qsub8(b, x);
    return *this;
  }

  // Saturate channels
  inline CRGBA& operator|=(const CRGBA& rhs) __attribute__((always_inline)) {
    if (rhs.r > r) r = rhs.r;
    if (rhs.g > g) g = rhs.g;
    if (rhs.b > b) b = rhs.b;
    if (rhs.a > a) a = rhs.a;
    return *this;
  }

  inline CRGBA operator+(uint8_t x) const __attribute__((always_inline)) { CRGBA res = *this; res += CRGBA(x,x,x); return res; }
  //inline CRGBA operator+(CRGBA rhs) const __attribute__((always_inline)) { CRGBA res = *this; res += rhs;          return res; } // prefer friend version below
  inline CRGBA operator|(uint8_t x) const __attribute__((always_inline)) { CRGBA res = *this; res |= CRGBA(x,x,x); return res; }

  // friend operators (to allow optimization of chained operators)
  friend inline CRGBA operator*(CRGBA lhs, const uint8_t scale) { lhs *= scale; return lhs; }
  friend inline CRGBA operator/(CRGBA lhs, const uint8_t scale) { lhs /= scale; return lhs; }
  friend inline CRGBA operator%(CRGBA lhs, const uint8_t scale) { lhs %= scale; return lhs; }
  friend inline CRGBA operator+(CRGBA lhs, const CRGBA& rhs)    { lhs += rhs;   return lhs; }
  friend inline CRGBA operator+(CRGBA lhs, const uint8_t x)     { lhs += x;     return lhs; }
  friend inline CRGBA operator-(CRGBA lhs, const CRGBA& rhs)    { lhs -= rhs;   return lhs; }
  friend inline CRGBA operator-(CRGBA lhs, const uint8_t x)     { lhs -= x;     return lhs; }
  friend inline CRGBA operator|(CRGBA lhs, const CRGBA& rhs)    { lhs |= rhs;   return lhs; }

  // Conversion operator to uint32_t (!!! will strip alpha channel, producing 0x00RRGGBB)
  explicit inline operator uint32_t() const __attribute__((always_inline)) { return color32; }

#ifdef FASTLED_VERSION
  // Constructor from CRGB
  inline CRGBA(const CRGB &rgb) __attribute__((always_inline)) : b(rgb.b), g(rgb.g), r(rgb.r), a(255) {}

  // Assignment from CRGB
  inline CRGBA& operator=(const CRGB& rgb) __attribute__((always_inline)) { b = rgb.b; g = rgb.g; r = rgb.r; a = 255; return *this; }
  inline CRGBA& operator=(CRGB&& rgb) __attribute__((always_inline)) { b = rgb.b; g = rgb.g; r = rgb.r; a = 255; rgb = (uint32_t)0; return *this; }

  // Addition from CRGB
  friend inline CRGBA operator+(CRGBA lhs, const CRGB& rhs) __attribute__((always_inline)) { lhs += rhs; return lhs; }

  // Conversion operator to CRGB
  inline operator CRGB() const __attribute__((always_inline)) { return CRGB(r, g, b); }
#endif
};

struct CHSV32 { // 32bit HSV color with 16bit hue for more accurate conversions
  union {
    struct {
      union {
        uint16_t h;  // hue
        uint16_t hue;
      };
      union {
        uint8_t s;   // saturation
        uint8_t saturation;
      };
      union {
        uint8_t v;   // value
        uint8_t value;
      };
    };
    uint32_t raw;    // 32bit access
  };
  inline CHSV32() __attribute__((always_inline)) = default; // default constructor
  /// Allow construction from hue, saturation, and value
  /// @param ih input hue
  /// @param is input saturation
  /// @param iv input value
  inline CHSV32(uint16_t ih, uint8_t is, uint8_t iv) __attribute__((always_inline)) // constructor from 16bit h, s, v
    : h(ih), s(is), v(iv) {}
  inline CHSV32(uint8_t ih, uint8_t is, uint8_t iv) __attribute__((always_inline)) // constructor from 8bit h, s, v
    : h((uint16_t)ih << 8), s(is), v(iv) {}
  inline CHSV32(const CHSV& chsv) __attribute__((always_inline))  // constructor from CHSV
    : h((uint16_t)chsv.h << 8), s(chsv.s), v(chsv.v) {}
  inline CHSV32(const CRGBA& rgb) __attribute__((always_inline)) { rgb2hsv(rgb.color32, *this); } // constructor from CRGBA
  inline CHSV32(uint32_t rgb) __attribute__((always_inline)) { rgb2hsv(rgb, *this); } // constructor from uint32_t (represented as 0x00RRGGBB)

  #ifdef FASTLED_VERSION
  explicit inline operator CHSV() const { return CHSV((uint8_t)(h >> 8), s, v); } // typecast to CHSV
  #endif
};

#ifdef FASTLED_VERSION
inline CHSV rgb2hsv(const CRGB c) { CHSV32 hsv; rgb2hsv(CRGBA(c).color32, hsv); return CHSV(hsv); } // CRGB to hsv
#endif

// palettes
extern const TProgmemRGBPalette16* const fastledPalettes[];
extern const uint8_t* const gGradientPalettes[];

#endif