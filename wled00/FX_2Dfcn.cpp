/*
  FX_2Dfcn.cpp contains all 2D utility functions

  Copyright (c) 2022  Blaz Kristan (https://blaz.at/home)
  Licensed under the EUPL v. 1.2 or later
  Adapted from code originally licensed under the MIT license

  Parts of the code adapted from WLED Sound Reactive
*/
#include "wled.h"

// setUpMatrix() - constructs ledmap array from matrix of panels with WxH pixels
// this converts physical (possibly irregular) LED arrangement into well defined
// array of logical pixels: fist entry corresponds to left-topmost logical pixel
// followed by horizontal pixels, when Segment::maxWidth logical pixels are added they
// are followed by next row (down) of Segment::maxWidth pixels (and so forth)
// note: matrix may be comprised of multiple panels each with different orientation
// but ledmap takes care of that. ledmap is constructed upon initialization
// so matrix should disable regular ledmap processing
// WARNING: effect drawing has to be suspended (strip.suspend()) or must be called from loop() context
void WS2812FX::setUpMatrix() {
#ifndef WLED_DISABLE_2D
  // isMatrix is set in cfg.cpp or set.cpp
  if (isMatrix) {
    // calculate width dynamically because it may have gaps
    Segment::maxWidth = 1;
    Segment::maxHeight = 1;
    for (const Panel &p : panel) {
      if (p.xOffset + p.width > Segment::maxWidth) {
        Segment::maxWidth = p.xOffset + p.width;
      }
      if (p.yOffset + p.height > Segment::maxHeight) {
        Segment::maxHeight = p.yOffset + p.height;
      }
    }

    // safety check
    if (Segment::maxWidth * Segment::maxHeight > MAX_LEDS || Segment::maxWidth > 255 || Segment::maxHeight > 255 || Segment::maxWidth <= 1 || Segment::maxHeight <= 1) {
      DEBUG_PRINTLN(F("2D Bounds error."));
      isMatrix = false;
      Segment::maxWidth = _length;
      Segment::maxHeight = 1;
      panel.clear(); // release memory allocated by panels
      panel.shrink_to_fit(); // release memory if allocated
      resetSegments();
      return;
    }

    customMappingSize = 0; // prevent use of mapping if anything goes wrong
    d_free(customMappingTable);

    // we will try to load a "gap" array (a JSON file)
    // the array has to have the same amount of values as mapping array (or larger)
    // "gap" array is used while building ledmap (mapping array)
    // and discarded afterwards as it has no meaning after the process
    // content of the file is just raw JSON array in the form of [val1,val2,val3,...]
    // there are no other "key":"value" pairs in it
    // allowed values are: -1 (missing pixel/no LED attached), 0 (inactive/unused pixel), 1 (active/used pixel)
    const unsigned matrixSize = Segment::maxWidth * Segment::maxHeight; // less or equal to getLengthTotal()
    char    fileName[32]; strcpy_P(fileName, PSTR("/2d-gaps.json"));
    bool    isFile = WLED_FS.exists(fileName);
    size_t  gapSize = 0;
    int8_t *gapTable = nullptr;

    if (isFile) {
      DEBUG_PRINTLN(F("Loading gaps."));
      File f = WLED_FS.open(fileName, "r");
      if (f && f.find("[")) {
        size_t pos = f.position();
        // count elements first to know how much to allocate
        char token[32]; strcpy_P(token, PSTR(" \n\r\t,-01"));
        //while (f.available()) if (f.read() == ',') gapSize++;
        while (f.available()) {
          char c = f.read();
          if (strchr(token, c) == nullptr) break; // invalid character, stop
          if (c == ',') gapSize++;
        }
        gapSize++;  // there's one more entry than there is commas
        if (gapSize >= matrixSize) {
          f.seek(pos);
          gapTable = static_cast<int8_t*>(d_malloc(gapSize));
          if (gapTable) {
            memset(gapTable, 1, gapSize);
            pos = 0;
            while (f.available() && pos < gapSize) {
              size_t n = f.readBytesUntil(',', token, sizeof(token)-1);
              token[n] = '\0';
              if (n < sizeof(token)-1) gapTable[pos++] = (int8_t)constrain(strtol(token, nullptr, 10), -1, 1);
              if (strchr(token, ']') != nullptr) break; // end of array
            }
            DEBUG_PRINTLN(F("Gaps loaded."));
          } else {
            DEBUG_PRINTLN(F("Out of memory."));
          }
        } else {
          DEBUG_PRINTLN(F("Gapfile too small."));
        }
        f.close();
      }
    } else if (panel.size() == 1 && !panel[0].bottomStart && !panel[0].rightStart && !panel[0].serpentine && !panel[0].vertical) {
      // if we only have one panel that starts at top-left and is not serpentine and vertically oriented then don't bother with ledmap
      DEBUG_PRINTLN(F("Ledmap not needed."));
      return;
    }

    DEBUG_PRINTLN(F("Creating 2D ledmap"));
    // Segment::maxWidth and Segment::maxHeight are set according to panel layout
    // and the product will include at least all leds in matrix
    // if actual LEDs are more, getLengthTotal() will return correct number of LEDs
    customMappingTable = static_cast<uint16_t*>(allocate_buffer(sizeof(uint16_t)*getLengthTotal(), BFRALLOC_PREFER_PSRAM));

    if (customMappingTable) {
      customMappingSize = getLengthTotal();
      DEBUG_PRINTF_P(PSTR("ledmap allocated: %uB @ %p\n"), customMappingSize * sizeof(uint16_t), customMappingTable);

      // fill with empty in case we don't fill the entire matrix
      for (unsigned i = 0; i < matrixSize; i++) customMappingTable[i] = 0xFFFFU;
      for (unsigned i = matrixSize; i < customMappingSize; i++) customMappingTable[i] = i; // trailing LEDs for ledmap (after matrix) if it exist

      // create ledmap using gap file if it exists 
      unsigned x, y, pix=0; //pixel
      for (const Panel &p : panel) {
        unsigned h = p.vertical ? p.height : p.width;
        unsigned v = p.vertical ? p.width  : p.height;
        for (size_t j = 0; j < v; j++){
          for(size_t i = 0; i < h; i++) {
            y = (p.vertical?p.rightStart:p.bottomStart) ? v-j-1 : j;
            x = (p.vertical?p.bottomStart:p.rightStart) ? h-i-1 : i;
            x = p.serpentine && j%2 ? h-x-1 : x;
            size_t index = (p.yOffset + (p.vertical?x:y)) * Segment::maxWidth + p.xOffset + (p.vertical?y:x);
            if (!gapTable || (gapTable && gapTable[index] >  0)) customMappingTable[index] = pix; // a useful pixel (otherwise -1 is retained)
            if (!gapTable || (gapTable && gapTable[index] >= 0)) pix++; // not a missing pixel
          }
        }
      }

      #ifdef WLED_DEBUG
      DEBUG_PRINTLN(F("Matrix ledmap:"));
      for (unsigned i = 0; i < customMappingSize; i++) {
        DEBUG_PRINTF_P(PSTR("%4d,%c"), (int)(int16_t)customMappingTable[i], (i+1)%Segment::maxWidth ? ' ' : '\n');
      }
      DEBUG_PRINTLN();
      #endif
    } else { // memory allocation error
      DEBUG_PRINTLN(F("ERROR 2D LED map allocation error."));
      isMatrix = false;
      panel.clear();
      Segment::maxWidth = _length;
      Segment::maxHeight = 1;
      resetSegments();
    }

    // delete gap array as we no longer need it
    d_free(gapTable);
  }
#else
  isMatrix = false; // no matter what config says
#endif
}


///////////////////////////////////////////////////////////
// Segment:: routines
///////////////////////////////////////////////////////////

#ifndef WLED_DISABLE_2D
// pixel is clipped if it falls outside clipping range
// if clipping start > stop the clipping range is inverted
bool Segment::isPixelXYClipped(unsigned x, unsigned y) const {
  if (transitionStyle != TRANSITION_FADE && isInTransition() && _clipStart != _clipStop) {
    const bool invertX = _clipStart  > _clipStop;
    const bool invertY = _clipStartY > _clipStopY;
    const unsigned cStartX = invertX ? _clipStop   : _clipStart;
    const unsigned cStopX  = invertX ? _clipStart  : _clipStop;
    const unsigned cStartY = invertY ? _clipStopY  : _clipStartY;
    const unsigned cStopY  = invertY ? _clipStartY : _clipStopY;
    if (transitionStyle == TRANSITION_FAIRY_DUST) {
      const unsigned width = cStopX - cStartX;          // assumes full segment width (faster than virtualWidth())
      const unsigned len = width * (cStopY - cStartY);  // assumes full segment height (faster than virtualHeight())
      if (len < 2) return false;
      const unsigned shuffled = hashInt(x + y * width) % len;
      const unsigned pos = (shuffled * 0xFFFFU) / len;
      return progress() <= pos;
    }
    if (transitionStyle == TRANSITION_CIRCULAR_IN || transitionStyle == TRANSITION_CIRCULAR_OUT) {
      const unsigned cx   = (cStopX-cStartX+1) / 2;
      const unsigned cy   = (cStopY-cStartY+1) / 2;
      const bool     out  = (transitionStyle == TRANSITION_CIRCULAR_OUT);
      const unsigned prog = out ? progress() : 0xFFFFU - progress();
      unsigned radius2    = max(cx, cy) * prog / 0xFFFF;
      radius2 = 2 * radius2 * radius2;
      if (radius2 == 0) return out;
      const int dx = x - cx;
      const int dy = y - cy;
      const bool outside = (dx * dx + dy * dy) > radius2;
      return out ? outside : !outside;
    }
    bool xInside = (x >= cStartX && x < cStopX); if (invertX) xInside = !xInside;
    bool yInside = (y >= cStartY && y < cStopY); if (invertY) yInside = !yInside;
    const bool clip = transitionStyle == TRANSITION_OUTSIDE_IN ? xInside || yInside : xInside && yInside;
    return !clip;
  }
  return false;
}

void Segment::setStripPixelColorXY(unsigned x, unsigned y, const CRGBA &c) const {
  // TODO: for now ignore W, CCT and opacity
  const auto XY = [](unsigned X, unsigned Y){ return X + Y*Segment::maxWidth; };
  const unsigned revX = reverse   ? vWidth()  - x - 1 : x;
  const unsigned revY = reverse_y ? vHeight() - y - 1 : y;
  const unsigned baseX = start  + revX;
  const unsigned baseY = startY + revY;
  const uint32_t col = hasWhite() ? c.color32 : (uint32_t)c;  // explicit cast strips alpha/white channel
  size_t indx = XY(baseX, baseY); // absolute address on strip
  // TODO: for now ignore W, CCT and opacity
  strip.setPixelColor(indx, col);
  // Apply mirroring
  if (mirror || mirror_y) {
    const unsigned mirrorX = start  + width()  - revX - 1;
    const unsigned mirrorY = startY + height() - revY - 1;
    const size_t idxMX = XY(transpose ? baseX : mirrorX, transpose ? mirrorY : baseY);
    const size_t idxMY = XY(transpose ? mirrorX : baseX, transpose ? baseY : mirrorY);
    const size_t idxMM = XY(mirrorX, mirrorY);
    if (mirror)             strip.setPixelColor(idxMX, col);
    if (mirror_y)           strip.setPixelColor(idxMY, col);
    if (mirror && mirror_y) strip.setPixelColor(idxMM, col);
  }
}

void Segment::setPixelColorXY(unsigned x, unsigned y, CRGBA col) const {
  if (!isActive() || x >= vWidth() || y >= vHeight()) return; // if segment is inactive or pixel would fall out of virtual segment just exit
  if (pixels) setPixelColorXYRaw(x, y, col);
  else setStripPixelColorXY(x, y, col);
}

// returns RGB values of pixel
CRGBA Segment::getPixelColorXY(unsigned x, unsigned y) const {
  if (!isActive() || x >= vWidth() || y >= vHeight()) return 0; // if segment is inactive or pixel would fall out of virtual segment just exit
  if (pixels) return getPixelColorXYRaw(x, y);
  else {
    const auto XY = [](unsigned X, unsigned Y){ return X + Y*Segment::maxWidth; };
    const unsigned revX = reverse   ? vWidth()  - x - 1 : x;
    const unsigned revY = reverse_y ? vHeight() - y - 1 : y;
    const unsigned baseX = start  + revX;
    const unsigned baseY = startY + revY;
    size_t indx = XY(baseX, baseY); // absolute address on strip
    return strip.getPixelColor(indx);
  }
}

// 2D blurring, can be asymmetrical
void Segment::blur2D(uint8_t blur_x, uint8_t blur_y, bool smear) const {
  if (!isActive()) return; // not active
  const unsigned cols = vWidth();
  const unsigned rows = vHeight();

  // support bufferless segment
  void  (Segment::*setPixelXY)(unsigned, unsigned, const CRGBA&) const = pixels ? &Segment::setPixelColorXYRaw : &Segment::setStripPixelColorXY;
  CRGBA (Segment::*getPixelXY)(unsigned, unsigned) const               = pixels ? &Segment::getPixelColorXYRaw : &Segment::getPixelColorXY;

  if (blur_x) {
    const uint8_t keepx = smear ? 255 : 255 - blur_x;
    const uint8_t seepx = blur_x >> (1 + smear);
    for (unsigned row = 0; row < rows; row++) { // blur rows (x direction)
      CRGBA carryover = BLACK;
      for (unsigned x = 0; x < cols; x++) {
        CRGBA cur = (this->*getPixelXY)(x, row);
        CRGBA part = cur.scale8(seepx); // we are assuming RGBW pixels here as we also want to blur alpha channel
        cur.nscale8(keepx);
        cur += carryover;
        if (x > 0) (this->*setPixelXY)(x - 1, row, (this->*getPixelXY)(x - 1, row).add(part));
        (this->*setPixelXY)(x, row, cur); // first pixel
        carryover = part;
      }
    }
  }
  if (blur_y) {
    const uint8_t keepy = smear ? 255 : 255 - blur_y;
    const uint8_t seepy = blur_y >> (1 + smear);
    for (unsigned col = 0; col < cols; col++) {
      CRGBA carryover = BLACK;
      for (unsigned y = 0; y < rows; y++) {
        CRGBA cur = (this->*getPixelXY)(col, y);
        CRGBA part = cur.scale8(seepy);
        cur.nscale8(keepy);
        cur += carryover;
        if (y > 0) (this->*setPixelXY)(col, y - 1, (this->*getPixelXY)(col, y - 1).add(part));
        (this->*setPixelXY)(col, y, cur); // first pixel
        carryover = part;
      }
    }
  }
}

/*
// 2D Box blur
void Segment::box_blur(unsigned radius, bool smear) {
  if (!isActive() || radius == 0) return; // not active
  if (radius > 3) radius = 3;
  const unsigned d = (1 + 2*radius) * (1 + 2*radius); // averaging divisor
  const unsigned cols = vWidth();
  const unsigned rows = vHeight();
  uint16_t *tmpRSum = new uint16_t[cols*rows];
  uint16_t *tmpGSum = new uint16_t[cols*rows];
  uint16_t *tmpBSum = new uint16_t[cols*rows];
  // fill summed-area table (https://en.wikipedia.org/wiki/Summed-area_table)
  for (unsigned x = 0; x < cols; x++) {
    unsigned rS, gS, bS;
    unsigned index;
    rS = gS = bS = 0;
    for (unsigned y = 0; y < rows; y++) {
      index = x * cols + y;
      if (x > 0) {
        unsigned index2 = (x - 1) * cols + y;
        tmpRSum[index] = tmpRSum[index2];
        tmpGSum[index] = tmpGSum[index2];
        tmpBSum[index] = tmpBSum[index2];
      } else {
        tmpRSum[index] = 0;
        tmpGSum[index] = 0;
        tmpBSum[index] = 0;
      }
      CRGBA c = getPixelColorXYRaw(x, y);
      rS += c.r;
      gS += c.g;
      bS += c.b;
      tmpRSum[index] += rS;
      tmpGSum[index] += gS;
      tmpBSum[index] += bS;
    }
  }
  // do a box blur using pre-calculated sums
  for (unsigned x = 0; x < cols; x++) {
    for (unsigned y = 0; y < rows; y++) {
      // sum = D + A - B - C where k = (x,y)
      // +----+-+---- (x)
      // |    | |
      // +----A-B
      // |    |k|
      // +----C-D
      // |
      //(y)
      unsigned x0 = x < radius ? 0 : x - radius;
      unsigned y0 = y < radius ? 0 : y - radius;
      unsigned x1 = x >= cols - radius ? cols - 1 : x + radius;
      unsigned y1 = y >= rows - radius ? rows - 1 : y + radius;
      unsigned A = x0 * cols + y0;
      unsigned B = x1 * cols + y0;
      unsigned C = x0 * cols + y1;
      unsigned D = x1 * cols + y1;
      unsigned r = tmpRSum[D] + tmpRSum[A] - tmpRSum[C] - tmpRSum[B];
      unsigned g = tmpGSum[D] + tmpGSum[A] - tmpGSum[C] - tmpGSum[B];
      unsigned b = tmpBSum[D] + tmpBSum[A] - tmpBSum[C] - tmpBSum[B];
      setPixelColorXY(x, y, CRGBA(r/d, g/d, b/d));
    }
  }
  delete[] tmpRSum;
  delete[] tmpGSum;
  delete[] tmpBSum;
}
*/

// moveX() - move all pixels in X direction delta number of pixels
void Segment::moveX(int delta, bool wrap) const {
  if (!isActive() || !delta) return; // not active
  const int vW = vWidth();   // segment width in logical pixels (can be 0 if segment is inactive)
  const int vH = vHeight();  // segment height in logical pixels (is always >= 1)
  int absDelta = abs(delta);
  if (absDelta >= vW) return;

  // support bufferless segment
  void  (Segment::*setPixelXY)(unsigned, unsigned, const CRGBA&) const = pixels ? &Segment::setPixelColorXYRaw : &Segment::setStripPixelColorXY;
  CRGBA (Segment::*getPixelXY)(unsigned, unsigned) const               = pixels ? &Segment::getPixelColorXYRaw : &Segment::getPixelColorXY;

  CRGBA newPxCol[vW];
  int newDelta;
  int stop = vW;
  int start = 0;
  if (wrap) newDelta = (delta + vW) % vW; // +cols in case delta < 0
  else {
    if (delta < 0) start = absDelta;
    stop = vW - absDelta;
    newDelta = delta > 0 ? delta : 0;
  }
  for (int y = 0; y < vH; y++) {
    for (int x = 0; x < stop; x++) {
      int srcX = x + newDelta;
      if (wrap) srcX %= vW; // Wrap using modulo when `wrap` is true
      newPxCol[x] = (this->*getPixelXY)(srcX, y);
    }
    for (int x = 0; x < stop; x++) (this->*setPixelXY)(x + start, y, newPxCol[x]);
  }
}

// moveY() - move all pixels in Y direction delta number of pixels
void Segment::moveY(int delta, bool wrap) const {
  if (!isActive() || !delta) return; // not active
  const int vW = vWidth();   // segment width in logical pixels (can be 0 if segment is inactive)
  const int vH = vHeight();  // segment height in logical pixels (is always >= 1)
  int absDelta = abs(delta);
  if (absDelta >= vH) return;

  // support bufferless segment
  void  (Segment::*setPixelXY)(unsigned, unsigned, const CRGBA&) const = pixels ? &Segment::setPixelColorXYRaw : &Segment::setStripPixelColorXY;
  CRGBA (Segment::*getPixelXY)(unsigned, unsigned) const               = pixels ? &Segment::getPixelColorXYRaw : &Segment::getPixelColorXY;

  CRGBA newPxCol[vH];
  int newDelta;
  int stop = vH;
  int start = 0;
  if (wrap) newDelta = (delta + vH) % vH; // +rows in case delta < 0
  else {
    if (delta < 0) start = absDelta;
    stop = vH - absDelta;
    newDelta = delta > 0 ? delta : 0;
  }
  for (int x = 0; x < vW; x++) {
    for (int y = 0; y < stop; y++) {
      int srcY = y + newDelta;
      if (wrap) srcY %= vH; // Wrap using modulo when `wrap` is true
      newPxCol[y] = (this->*getPixelXY)(x, srcY);
    }
    for (int y = 0; y < stop; y++) (this->*setPixelXY)(x, y + start, newPxCol[y]);
  }  
}

// move() - move all pixels in desired direction delta number of pixels
// @param dir direction: 0=left, 1=left-up, 2=up, 3=right-up, 4=right, 5=right-down, 6=down, 7=left-down
// @param delta number of pixels to move
// @param wrap around
void Segment::move(unsigned dir, unsigned delta, bool wrap) const {
  if (delta==0) return;
  switch (dir) {
    case 0: moveX( delta, wrap);                      break;
    case 1: moveX( delta, wrap); moveY( delta, wrap); break;
    case 2:                      moveY( delta, wrap); break;
    case 3: moveX(-delta, wrap); moveY( delta, wrap); break;
    case 4: moveX(-delta, wrap);                      break;
    case 5: moveX(-delta, wrap); moveY(-delta, wrap); break;
    case 6:                      moveY(-delta, wrap); break;
    case 7: moveX( delta, wrap); moveY(-delta, wrap); break;
  }
}

// Draws filled ellipse or circle (with smooth edges) at (cx,cy) with given radii (in 10.6 fixed point notation) and color
void Segment::fillEllipse(int16_t cx, int16_t cy, uint16_t rx, uint16_t ry, CRGBA color, bool wrapX, bool wrapY) const {
  if (!isActive() || rx + ry == 0) return; // not active
  const unsigned vW = vWidth();   // segment width in logical pixels (can be 0 if segment is inactive)
  const unsigned vH = vHeight();  // segment height in logical pixels (is always >= 1)
  auto mul106 = [](int16_t a, int16_t b) { return ((int32_t)a * b) >> 6; };       // 10.6 fixed point multiplication
  auto int106 = [](int16_t a)            { return (int16_t)((int32_t)a >> 6); };  // convert 10.6 fixed point to integer

  // pre-calculate drawing bounds
  const int32_t pxMin = int106(cx - rx);              // minimum pixel coordinate for drawing; rounded down
  const int32_t pxMax = int106(cx + rx + (1<<6) - 1); // maximum pixel coordinate for drawing; rounded up
  const int32_t pyMin = int106(cy - ry);              // minimum pixel coordinate for drawing; rounded down
  const int32_t pyMax = int106(cy + ry + (1<<6) - 1); // maximum pixel coordinate for drawing; rounded up
  const int32_t rxSq  = mul106(rx, rx);
  const int32_t rySq  = mul106(ry, ry);

  // support bufferless segment
  void  (Segment::*setPixelXY)(unsigned, unsigned, const CRGBA&) const = pixels ? &Segment::setPixelColorXYRaw : &Segment::setStripPixelColorXY;
  CRGBA (Segment::*getPixelXY)(unsigned, unsigned) const               = pixels ? &Segment::getPixelColorXYRaw : &Segment::getPixelColorXY;

  //DEBUG_PRINTF_P(PSTR("Draw ellipse cx=%d cy=%d rx=%d ry=%d pxMin=%d pxMax=%d pyMin=%d pyMax=%d\n"), cx, cy, rx, ry, pxMin, pxMax, pyMin, pyMax);

  // traverse all pixels within bounding box and check if they are within ellipse radius
  for (int y = pyMin; y < pyMax; y++) {
    const int32_t dy = (y << 6) - cy;
    for (int x = pxMin; x < pxMax; x++) {
      const int32_t dx = (x << 6) - cx;
      const int32_t dist = (mul106(dx,dx)*256/rxSq) + (mul106(dy,dy)*256/rySq);
      if (dist > 384) continue; // outside ellipse (actually it should be 256 but that will render a smaller ellipse)
      int32_t px = x;
      int32_t py = y;
      if (wrapX) {
        while (px < 0)        px += vW;
        while (px >= (int)vW) px -= vW;
      }
      if (wrapY) {
        while (py < 0)        py += vH;
        while (py >= (int)vH) py -= vH;
      }
      if ((unsigned)px < vW && (unsigned)py < vH) {
        if (dist > 192) { // may need tweaking!
          CRGBA c = color;
          uint8_t alpha = 448 - dist; // may need tweaking!
          (this->*setPixelXY)(px, py, (this->*getPixelXY)(px, py).nblend(c, alpha));
        } else (this->*setPixelXY)(px, py, color);
      }
    }
  }
}

// https://gamedev.stackexchange.com/questions/176036/how-to-draw-a-smoother-solid-fill-circle
// draws a circle at (cx,cy) with given radius (in 10.6 fixed point notation) and color
void Segment::drawCircle(int16_t cx, int16_t cy, uint16_t radius, CRGBA col, bool soft, bool wrapX, bool wrapY) const {
  if (!isActive() || radius == 0) return; // not active
  const unsigned vW = vWidth();   // segment width in logical pixels (can be 0 if segment is inactive)
  const unsigned vH = vHeight();  // segment height in logical pixels (is always >= 1)
  auto int106 = [](int16_t a)            { return (int16_t)((int32_t)a >> 6); };  // convert 10.6 fixed point to integer
  auto mul106 = [](int16_t a, int16_t b) { return ((int32_t)a * b) >> 6; };       // 10.6 fixed point multiplication

  if ((unsigned)int106(radius) > min(vW, vH)/2) return; // too large

  // support bufferless segment
  void  (Segment::*setPixelXY)(unsigned, unsigned, const CRGBA&) const = pixels ? &Segment::setPixelColorXYRaw : &Segment::setStripPixelColorXY;
  CRGBA (Segment::*getPixelXY)(unsigned, unsigned) const               = pixels ? &Segment::getPixelColorXYRaw : &Segment::getPixelColorXY;

  auto plot = [&](int x, int y, uint8_t b) {
    if (wrapX) {
      while (x < 0)        x += vW;
      while (x >= (int)vW) x -= vW;
    }
    if (wrapY) {
      while (y < 0)        y += vH;
      while (y >= (int)vH) y -= vH;
    }
    if ((unsigned)x < vW && (unsigned)y < vH) {
      if (b < 255) (this->*setPixelXY)(x, y, (this->*getPixelXY)(x, y).nblend(col, b)); // blendPixelColorXY
      else         (this->*setPixelXY)(x, y, col);
    }
  };

  cx = int106(cx+(1<<5)); // round to nearest integer
  cy = int106(cy+(1<<5)); // round to nearest integer

  const int r = int106(radius+(1<<5)); // round radius to nearest integer

  if (soft) {
    // Xiaolin Wu’s algorithm
    const int rSq = mul106(radius,radius); // in 10.6 fixed point
    int x = 0, y = r;
    unsigned oldFade = 0;
    while (x < y) {
      //float yf = sqrtf(float(rsq - x*x)); // needs to be floating point
      //uint8_t fade = 255 * (ceilf(yf) - yf); // how much color to keep
      const uint32_t yInt = sqrt32_bw((rSq - ((x*x)<<6))<<6); // 10.6 representation of y = sqrt(r^2 - x^2)
      const unsigned fade = 255 - ((yInt & 0x3F) << 2);       // how much color to keep (fractional part of yInt)
      if (oldFade > fade) y--;
      oldFade = fade;
      int px, py;
      // paint octants (2 pixels per octant for smoothness)
      for (size_t i = 0; i < 16; i++) {
        bool swaps = (i & 0x4);         // 0,  0,  0,  0,  1,  1,  1,  1,  0,  0,  0,  0,  1,  1,  1,  1
        bool adj   = (i >> 3);          // 0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1
        int  dx    = (i & 1) ? -1 : 1;  // 1, -1,  1, -1,  1, -1,  1, -1,  1, -1,  1, -1,  1, -1,  1, -1
        int  dy    = (i & 2) ? -1 : 1;  // 1,  1, -1, -1,  1,  1, -1, -1,  1,  1, -1, -1,  1,  1, -1, -1
        if (swaps) {
          px = cx + (y - adj) * dx;
          py = cy + x * dy;
        } else {
          px = cx + x * dx;
          py = cy + (y - adj) * dy;
        }
        plot(px, py, (uint8_t)(adj ? fade : 255 - fade));
      }
      x++;
    }
  } else {
    // Bresenham’s Algorithm
    int d = 3 - (2 * r); // decision parameter
    int y = r, x = 0;
    while (y >= x) {
      // plot the 8 octants
      //for (int i = 0; i < 4; i++) {
      //  int dx = (i & 1) ? -x : x;
      //  int dy = (i & 2) ? -y : y;
      //  plot(cx + dx, cy + dy, 255);
      //  plot(cx + dy, cy + dx, 255);
      //}
      plot(cx + x, cy + y, 255);
      plot(cx + y, cy + x, 255);
      plot(cx - x, cy + y, 255);
      plot(cx + y, cy - x, 255);
      plot(cx + x, cy - y, 255);
      plot(cx - y, cy + x, 255);
      plot(cx - x, cy - y, 255);
      plot(cx - y, cy - x, 255);
      if (d > 0) {
        d += 4 * (x - y) + 10;
        y--;
      } else {
        d += 4 * x + 6;
      }
      x++;
    }
  }
}

// see https://www.geeksforgeeks.org/dsa/midpoint-ellipse-drawing-algorithm/
void Segment::drawEllipse(int16_t cx, int16_t cy, uint16_t rx, uint16_t ry, CRGBA color, bool wrapX, bool wrapY) const {
  if (!isActive() || rx == 0 || ry == 0) return;
  const unsigned vW = vWidth();   // segment width in logical pixels (can be 0 if segment is inactive)
  const unsigned vH = vHeight();  // segment height in logical pixels (is always >= 1)
  // all coodinates and radii are in 10.6 fixed point notation
  auto int106 = [](int16_t a)            { return (int16_t)((int32_t)a >> 6); };      // convert 12.4 fixed point to integer
  auto mul106 = [](int16_t a, int16_t b) { return (int16_t)((int32_t)a * b) >> 6; };  // 12.4 fixed point multiplication

  // support bufferless segment
  void (Segment::*setPixelXY)(unsigned, unsigned, const CRGBA&) const = pixels ? &Segment::setPixelColorXYRaw : &Segment::setStripPixelColorXY;
  CRGBA (Segment::*getPixelXY)(unsigned, unsigned) const              = pixels ? &Segment::getPixelColorXYRaw : &Segment::getPixelColorXY;

  auto plot   = [&](int32_t x, int32_t y) {
    if (wrapX) {
      while (x < 0)        x += vW;
      while (x >= (int)vW) x -= vW;
    }
    if (wrapY) {
      while (y < 0)        y += vH;
      while (y >= (int)vH) y -= vH;
    }
    if ((unsigned)x < vW && (unsigned)y < vH) (this->*setPixelXY)(x, y, (this->*getPixelXY)(x, y).add(color)); // addPixelColorXY
  }; // draws a single point

  auto points = [&](int32_t x, int32_t y) { // draws 4 simertically placed points
    int32_t c_x = int106(cx);
    int32_t c_y = int106(cx);
    //for (int i = 0; i < 4; i++) {
    //  int32_t dx = (i & 1) ? -x : x;
    //  int32_t dy = (i & 2) ? -y : y;
    //  plot(c_x + dx, c_y + dy);
    //}
    plot(c_x + x, c_y + y);
    plot(c_x - x, c_y + y);
    plot(c_x + x, c_y - y);
    plot(c_x - x, c_y - y);
  };

  int32_t rxSq = mul106(rx,rx);
  int32_t rySq = mul106(ry,ry);
  int32_t x = 0, y = int106(ry+(1<<5));

  int32_t dx = 0;                     // 2 * rySq * x;
  int32_t dy = mul106(2 * rxSq, ry);  // 2 * rxSq * y;

  // Region 1
  int32_t d1 = (rySq - mul106(rxSq, ry)) + (rxSq >> 2); // initial decision parameter
  while (dx < dy) {
    points(x, y);
    if (d1 < 0) {
      x++;
      dx += (2 * rySq);
      d1 += dx + rySq;
    } else {
      x++;
      y--;
      dx += (2 * rySq);
      dy -= (2 * rxSq);
      d1 += dx - dy + rySq;
    }
  }

  // Region 2
  const int32_t x_plus_half = (x<<6) + (1<<5);
  const int32_t y_minus_1 = (y - 1)<<6;
  int32_t d2 = mul106(rySq, mul106(x_plus_half,x_plus_half)) + mul106(rxSq, mul106(y_minus_1,y_minus_1)) - mul106(rxSq, rySq);
  while (y >= 0) {
    points(x, y);
    if (d2 > 0) {
      y--;
      dy -= (2 * rxSq);
      d2 += rxSq - dy;
    } else {
      y--;
      x++;
      dx += (2 * rySq);
      dy -= (2 * rxSq);
      d2 += dx - dy + rxSq;
    }
  }
}

//line function
void Segment::drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, CRGBA c, bool soft) const {
  if (!isActive()) return; // not active
  const int vW = vWidth();   // segment width in logical pixels (can be 0 if segment is inactive)
  const int vH = vHeight();  // segment height in logical pixels (is always >= 1)
  if (x0 >= vW || x1 >= vW || y0 >= vH || y1 >= vH) return;

  const int dx = abs(int(x1)-int(x0)); // x distance
  const int dy = abs(int(y1)-int(y0)); // y distance

  // support bufferless segment
  void (Segment::*setPixelXY)(unsigned, unsigned, const CRGBA&) const = pixels ? &Segment::setPixelColorXYRaw : &Segment::setStripPixelColorXY;
  CRGBA (Segment::*getPixelXY)(unsigned, unsigned) const              = pixels ? &Segment::getPixelColorXYRaw : &Segment::getPixelColorXY;

  // single pixel (line length == 0)
  if (dx+dy == 0) {
    (this->*setPixelXY)(x0, y0, c);
    return;
  }

  if (soft) {
    // https://en.wikipedia.org/wiki/Xiaolin_Wu%27s_line_algorithm
    // Xiaolin Wu’s algorithm
    const bool steep = dy > dx;
    if (steep) {
      // we need to go along longest dimension
      std::swap(x0,y0);
      std::swap(x1,y1);
    }
    if (x0 > x1) {
      // we need to go in increasing fashion
      std::swap(x0,x1);
      std::swap(y0,y1);
    }
    int32_t grad = x1==x0 ? 1 << 8 : ((y1-y0)<<8)/(x1-x0); // gradient in 16.8 fixed point
    int32_t intY = (y0<<8); // y intersection in 16.8 fixed point
    for (int x = x0; x <= x1; x++) {
      uint8_t keep = (intY & 0xFF); // fractional part of y in 16.8 fixed point
      uint8_t seep = 0xFF - keep;
      int y = intY >> 8;
      if (steep) std::swap(x,y);  // temporaryly swap if steep
      // pixel coverage is determined by fractional part of y co-ordinate
      int x2 = x + steep;
      int y2 = y + !steep;
      if (x  >= 0 && y  >= 0 && x  < vW && y  < vH) (this->*setPixelXY)(x, y, (this->*getPixelXY)(x, y).nblend(c, seep));
      if (x2 >= 0 && y2 >= 0 && x2 < vW && y2 < vH) (this->*setPixelXY)(x2, y2, (this->*getPixelXY)(x2, y2).nblend(c, keep));
      intY += grad;
      if (steep) std::swap(x,y);  // restore if steep
    }
  } else {
    // Bresenham's algorithm
    const int sx = x0<x1 ? 1 : -1;  // x step
    const int sy = y0<y1 ? 1 : -1;  // y step
    int err = (dx>dy ? dx : -dy)/2; // error direction
    for (;;) {
      (this->*setPixelXY)(x0, y0, c);
      if (x0==x1 && y0==y1) break;
      int e2 = err;
      if (e2 >-dx) { err -= dy; x0 += sx; }
      if (e2 < dy) { err += dx; y0 += sy; }
    }
  }
}

#include "src/font/console_font_4x6.h"
#include "src/font/console_font_5x8.h"
#include "src/font/console_font_5x12.h"
#include "src/font/console_font_6x8.h"
#include "src/font/console_font_7x9.h"

// draws a raster font character on canvas
// only supports: 4x6=24, 5x8=40, 5x12=60, 6x8=48 and 7x9=63 fonts ATM
void Segment::drawCharacter(unsigned char chr, int16_t x, int16_t y, uint8_t w, uint8_t h, CRGBA color, CRGBA col2, int8_t rotate, uint8_t fade) const {
  if (!isActive()) return; // not active
  if (chr < 32 || chr > 126) return; // only ASCII 32-126 supported
  chr -= 32; // align with font table entries
  const int font = w*h;

  // support bufferless segment
  void (Segment::*setPixelXY)(unsigned, unsigned, const CRGBA&) const = pixels ? &Segment::setPixelColorXYRaw : &Segment::setStripPixelColorXY;
  CRGBPalette16 grad = col2 != BLACK ? CRGBPalette16(CRGB(color), CRGB(col2)) : Segment::getCurrentPalette(); // selected palette as gradient

  for (int i = 0; i<h; i++) { // character height
    uint8_t bits = 0;
    switch (font) {
      case 24: bits = pgm_read_byte_near(&console_font_4x6[(chr * h) + i]); break;  // 4x6 font
      case 40: bits = pgm_read_byte_near(&console_font_5x8[(chr * h) + i]); break;  // 5x8 font
      case 48: bits = pgm_read_byte_near(&console_font_6x8[(chr * h) + i]); break;  // 6x8 font
      case 63: bits = pgm_read_byte_near(&console_font_7x9[(chr * h) + i]); break;  // 7x9 font
      case 60: bits = pgm_read_byte_near(&console_font_5x12[(chr * h) + i]); break; // 5x12 font
      default: return;
    }
    CRGBA c = ColorFromPaletteWLED(grad, (i+1)*255/h, 255, LINEARBLEND_NOWRAP);
    for (int j = 0; j<w; j++) { // character width
      int x0, y0;
      switch (rotate) {
        case -1: x0 = x + (h-1) - i; y0 = y + (w-1) - j; break; // -90 deg
        case -2:
        case  2: x0 = x + j;         y0 = y + (h-1) - i; break; // 180 deg
        case  1: x0 = x + i;         y0 = y + j;         break; // +90 deg
        default: x0 = x + (w-1) - j; y0 = y + i;         break; // no rotation
      }
      if (x0 < 0 || x0 >= (int)vWidth() || y0 < 0 || y0 >= (int)vHeight()) continue; // drawing off-screen
      CRGBA c_a = c;
      // apply fading to sides (available only on segments without white channel)
      if (fade) {
        int offset = ((fade+1) * vWidth()) >> 9;
        if (x0 < offset) c_a.nfadeOut(map(x0, 0, offset, 255, 0));
        if (x0 > (int)vWidth() - offset) c_a.nfadeOut(map(vWidth() - x0, 0, offset, 255, 0));
      }
      if (((bits>>(j+(8-w))) & 0x01)) { // bit set
        (this->*setPixelXY)(x0, y0, c_a);
      }
    }
  }
}

// awesome wu_pixel procedure by https://reddit.com/u/sutaburosu
// https://gist.github.com/sutaburosu/32a203c2efa2bb584f4b846a91066583#file-cakeday_hack-ino-L154
// @param x 16.8 fixed point
// @param y 16.8 fixed point
void Segment::setWuPixelColor(uint32_t x, uint32_t y, CRGBA c) const {
  if (!isActive()) return; // not active
  const unsigned vW = vWidth();   // segment width in logical pixels (can be 0 if segment is inactive)
  const unsigned vH = vHeight();  // segment height in logical pixels (is always >= 1)
  // support bufferless segment
  void (Segment::*setPixelXY)(unsigned, unsigned, const CRGBA&) const = pixels ? &Segment::setPixelColorXYRaw : &Segment::setStripPixelColorXY;
  CRGBA (Segment::*getPixelXY)(unsigned, unsigned) const              = pixels ? &Segment::getPixelColorXYRaw : &Segment::getPixelColorXY;
  // extract the fractional parts and derive their inverses
  const unsigned xx = x & 0xff, yy = y & 0xff, ix = 255 - xx, iy = 255 - yy;
  x >>= 8; // integer part of x
  y >>= 8; // integer part of y
  if (x >= vW || y >= vH) return;  // if pixel would fall out of virtual segment just exit
  // calculate the intensities for each affected pixel
  auto WU_WEIGHT = [](uint16_t a, uint16_t b) { return (uint8_t)((a * b + a + b) >> 8); };
  const uint8_t wu[4] = {WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy), WU_WEIGHT(ix, yy), WU_WEIGHT(xx, yy)};
  const int step = x+1 < vW ? 1 : 2; // skip right pixels if out of bounds
  const int maxI = y+1 < vH ? 4 : 2; // skip bottom pixels if out of bounds
  // multiply the intensities by the colour, and saturating-add them to the pixels
  for (int i = 0; i < maxI; i += step) {
    int wu_x = x + (i & 1);        // precalculate x
    int wu_y = y + ((i >> 1) & 1); // precalculate y
    (this->*setPixelXY)(wu_x, wu_y, (this->*getPixelXY)(wu_x, wu_y).nblend(c, wu[i]));  // blendPixelColorXY
  }
}

#endif // WLED_DISABLE_2D
