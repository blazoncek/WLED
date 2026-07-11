#include "wled.h"

/*
 * LED methods
 */

 // applies chosen setment properties to legacy values
void setValuesFromSegment(uint8_t s) {
  const Segment& seg = strip.getSegment(s);
  colPri[0] = seg.colors[0].r;
  colPri[1] = seg.colors[0].g;
  colPri[2] = seg.colors[0].b;
  colPri[3] = seg.colors[0].a;
  colSec[0] = seg.colors[1].r;
  colSec[1] = seg.colors[1].g;
  colSec[2] = seg.colors[1].b;
  colSec[3] = seg.colors[1].a;
  effectCurrent   = seg.mode;
  effectSpeed     = seg.speed;
  effectIntensity = seg.intensity;
  effectPalette   = seg.palette;
}


// applies global legacy values (colPri, colSec, effectCurrent...) to each selected segment
void applyValuesToSelectedSegs() {
  for (unsigned i = 0; i < strip.getSegmentsNum(); i++) {
    Segment& seg = strip.getSegment(i);
    if (!(seg.isActive() && seg.isSelected())) continue;
    if (effectSpeed     != seg.speed)     {seg.speed     = effectSpeed;     stateChanged = true;}
    if (effectIntensity != seg.intensity) {seg.intensity = effectIntensity; stateChanged = true;}
    if (effectPalette   != seg.palette)   {seg.setPalette(effectPalette);}
    if (effectCurrent   != seg.mode)      {seg.setMode(effectCurrent);}
    uint32_t col0 = RGBW32(colPri[0], colPri[1], colPri[2], colPri[3]);
    uint32_t col1 = RGBW32(colSec[0], colSec[1], colSec[2], colSec[3]);
    if (col0 != seg.colors[0])            {seg.setColor(0, col0);}
    if (col1 != seg.colors[1])            {seg.setColor(1, col1);}
  }
}


// starts on/off transition
void toggleOnOff() {
  if (bri == 0) {
    bri = briLast;
    strip.restartRuntime();
    // we need to switch relay on immediately for delay to work properly
    toggleRelay(true);
  } else {
    // set all segments to transition otherwise we may get a single-frame blackout (black flash) in non-fade transitions
    // (race condition bri=0 but segment is not yet in transition yielding empty clipping) wled#5726
    strip.setTransitionMode(true);
    briLast = bri;
    bri = 0;
    // we will switch off relay in handleBrightness() when transition finishes
  }
  stateChanged = true;
}


//applies global temporary brightness (briT) to strip (during on/off transition)
void applyBri() {
  if (realtimeOverride || !(realtimeMode && arlsForceMaxBri)) {
    //DEBUG_PRINTF_P(PSTR("Applying strip brightness: %d (%d,%d)\n"), (int)briT, (int)bri, (int)briOld);
    strip.setBrightness(briT);
  }
}


//applies global brightness and sets it as the "current" brightness (no transition)
void applyFinalBri() {
  briOld = bri;
  briT = bri;
  applyBri();
  strip.trigger(); // force one last update
}


//called after every state changes, schedules interface updates, handles brightness transition and nightlight activation
//unlike colorUpdated(), does NOT apply any colors or FX to segments
void stateUpdated(byte callMode) {
  DEBUG_PRINTF_P(PSTR("State updated called. %d %d\n"), (int)transitionActive, (int)nightlightActive);
  //call for notifier -> 0: init 1: direct change 2: button 3: notification 4: nightlight 5: other (No notification)
  //                     6: fx changed 7: hue 8: preset cycle 9: blynk 10: alexa 11: ws send only 12: button preset
  setValuesFromFirstSelectedSeg();  // a much better approach would be to use main segment: setValuesFromMainSeg()

  // notifications
  if (bri != briOld || stateChanged) {    // bri != briOld must be checked since changing brightness does not always set stateChanged
    if (stateChanged) currentPreset = 0; //something changed, so we are no longer in the preset

    if (callMode != CALL_MODE_NOTIFICATION && callMode != CALL_MODE_NO_NOTIFY) notify(callMode);
    if (bri != briOld && nodeBroadcastEnabled) sendSysInfoUDP(); // update "on" state

    //set flag to update ws and mqtt
    interfaceUpdateCallMode = callMode;
  }

  // notify usermods of state change
  UsermodManager::onStateChange(callMode);

  if (nightlightActive) return;

  // off
  if (briT == 0) {
    if (callMode != CALL_MODE_NOTIFICATION) strip.resetTimebase(); //effect start from beginning
  }

  if (bri > 0) briLast = bri;

  // state was updated, notifications were sent, determine if we need to set segments into transition mode and fade brightness
  if (strip.getTransition() == 0) {
    // no transition, just apply desitred brightness immediately (even if not changed)
    jsonTransitionOnce = false;
    transitionActive = false;
    applyFinalBri();
    strip.trigger();
  } else {
    if (bri != briOld) {
      unsigned long now = millis();
      if (transitionActive && transitionStyle == TRANSITION_FADE) {
        // already active, just update briOld to reflect current state (no further notifications sent during brightness change fade)
        // this will unfortunately cause all pixels to become instantly visible in non-fade transitions
        if (now - transitionStartTime > 9) {
          // transition may have started in paralel from handleBrightness() so only update briOld if enough time has passed (>MIN_SHOW_DELAY)
          briOld = briT;  // briT is updated in handleBrightness()
          DEBUG_PRINTLN(F("-- Updating briOld."));
        }
      }
      // if relay is in idle state (!rlyStartTime) then we have change in brightness; start global brightness transition (may be to "off")
      // if relay is not in idle state (rlyStartTime>0) then we have an global "off" to "on" transition and LEDs start
      // with a delay (this delay is handled in handleBrightness())
      if (!rlyStartTime) strip.setTransitionMode(true); // force all segments to transition mode (for clipping to work)
      transitionActive = true;
      transitionStartTime = now;
    }
  }
  stateChanged = false;
}


void updateInterfaces(uint8_t callMode) {
  if (!interfaceUpdateCallMode || jsonBufferLock || millis() - lastInterfaceUpdate < INTERFACE_UPDATE_COOLDOWN) return;

  sendDataWs();
  lastInterfaceUpdate = millis();
  interfaceUpdateCallMode = CALL_MODE_INIT; //disable further updates

  if (callMode == CALL_MODE_WS_SEND) return;

  #ifndef WLED_DISABLE_ALEXA
  if (espalexaDevice != nullptr && callMode != CALL_MODE_ALEXA) {
    espalexaDevice->setValue(bri);
    espalexaDevice->setColor(colPri[0], colPri[1], colPri[2]);
  }
  #endif
  #ifndef WLED_DISABLE_MQTT
  publishMqtt();
  #endif
}


// legacy method, applies values from col, effectCurrent, ... to selected segments
void colorUpdated(byte callMode) {
  applyValuesToSelectedSegs();
  stateUpdated(callMode);
}


// handle On/Off and brightness change transitions
void handleBrightness() {
  unsigned long now = millis();
  if (rlyStartTime) {
    // relay was activated
    if (now - rlyStartTime <= rlyDelay*10) return; // don't do anything if we are waiting for relay delay
    else {
      if (strip.getTransition() == 0) {
        // no transition, just apply desitred brightness immediately (even if not changed)
        transitionActive = false;
        applyFinalBri();
      } else if (!transitionActive) {
        // relay delay has passed start actual transition
        strip.setTransitionMode(true);  // force all segments to transition mode (for clipping to work)
        transitionActive = true;
        transitionStartTime = now;      // start counting transition from the moment when relay delay finished
      }
      rlyStartTime = 0;                 // reset relay status
    }
    strip.restartRuntime();             // switching from Off to On requires effects to be restarted
  }

  // perform fade global brightness transition
  if (transitionActive && !nightlightActive) {
    int ti = now - transitionStartTime;
    int tr = strip.getTransition() + 1; // ensure non-zero just in case
    if (ti/tr > 0) {
      // restore (global) transition time if not called from UDP notifier or single/temporary transition from JSON (also playlist)
      if (jsonTransitionOnce) strip.setTransition(transitionDelay);
      transitionActive = false;
      jsonTransitionOnce = false;
      applyFinalBri();
    } else {
      // handling of global non-fade brightness transition is tricky and is done in segment blending for On/Off transitions
      // changing global brighntess (bri>0) is done in fade fashion as it is too complex to handle otherwise
      // (segments have no knowledge of global brightness as it is applied at bus level)
      byte briTO = briT;
      int deltaBri = (int)bri - (int)briOld;
      if (transitionStyle != TRANSITION_FADE && ((bool)bri ^ (bool)briOld)) {
        // changing On/Off in non-fade transitions must be immediate
        if (deltaBri > 0) briT = bri;
        if (deltaBri < 0) briT = briOld;
      } else
        briT = briOld + (deltaBri * ti / tr);
      if (briTO != briT) applyBri();
    }
  }

  // if we want to control on-board LED (ESP8266) or relay we have to do it here as the final show() may not happen until
  // next loop() cycle
  if (strip.getBrightness()) {
    // we want to be on
    lastOnTime = now;
    if (offMode) {
      // but we are off
      BusManager::on();
      offMode = false;
      // relay was switched on in toggleOnOff() or similar on/off action (deserializeState()); here, it would be too late
    }
  } else if (now - lastOnTime > 600 && !strip.needsUpdate()) {
    // for turning LED or relay off we need to wait until strip no longer needs updates (strip.trigger())
    // we want to be off
    if (!offMode) {
      // but we are on
      BusManager::off();
      offMode = true;
      toggleRelay(!offMode);  // switch off
    }
  }
}


// relay control
void toggleRelay(bool on) {
  // init relay pin and switch
  if (rlyPin >= 0) {
    pinMode(rlyPin, rlyOpenDrain ? OUTPUT_OPEN_DRAIN : OUTPUT);
    digitalWrite(rlyPin, !(rlyMde ^ on)); // !XOR: 00=0, 01=1, 10=1, 11=0; we need inverse of that
  }
  rlyStartTime = on ? millis() : 0;
}


static unsigned long lastNlUpdate = 0;

void handleNightlight() {
  unsigned long now = millis();
  if (now < 100 && lastNlUpdate > 0) lastNlUpdate = 0; // take care of millis() rollover
  if (now - lastNlUpdate < 100) return; // allow only 10 NL updates per second
  lastNlUpdate = now;

  if (nightlightActive) {
    // disable global brightness and segment transitions during nighlight (nightlight is a long transition on its own)
    strip.setTransition(0);

    if (!nightlightActiveOld) { //init
      DEBUG_PRINTF_P(PSTR("Nightlight started for %u minutes.\n"), (unsigned)nightlightDelayMins);
      nightlightStartTime = now;
      nightlightActiveOld = true;
      if (nightlightMode == NL_MODE_SUN) {
        //save current
        colNlT[0] = effectCurrent;
        colNlT[1] = effectSpeed;
        colNlT[2] = effectPalette;
        // set SUNRISE effect
        effectCurrent = FX_MODE_SUNRISE;
        effectSpeed = min(60, (int)nightlightDelayMins);  //currently limited to 60 minutes
        effectPalette = 0;
        if (bri) effectSpeed += 60; //sunset if currently on (0-60 sunrise; 60-120 sunset)
        applyValuesToSelectedSegs();
        // reuse briNlT as a flag for sunrise/sunset
        briNlT = !bri; //true == sunrise, false == sunset
        if (!bri) toggleOnOff();  // restarts effects, restores last known brightness & sets timer for actually applying brightness (in handleBrightness())
        else strip.restartRuntime();
      } else {
        briNlT = bri; // starting brightness
        for (unsigned i=0; i<4; i++) colNlT[i] = colPri[i]; // remember starting color
      }
      notify(CALL_MODE_NIGHTLIGHT); // inform clients of brightness & color change
      interfaceUpdateCallMode = CALL_MODE_NIGHTLIGHT;
    }

    int tper = (now - nightlightStartTime) / (nightlightDelayMins * 60);
    if (tper >= 1000) { //nightlight duration over
      DEBUG_PRINTLN(F("Nightlight finished."));
      nightlightActive = false;
      nightlightActiveOld = false;
      switch (nightlightMode) {
        case NL_MODE_COLORFADE:
        case NL_MODE_FADE:
        case NL_MODE_SET:
          bri = nightlightTargetBri;
          if (bri == 0) briLast = briNlT; // set brigthness at the start of nightlight as briLast
          break;
        case NL_MODE_SUN:
          if (!briNlT) { //turn off if sunset
            effectCurrent = colNlT[0];
            effectSpeed = colNlT[1];
            effectPalette = colNlT[2];
            applyValuesToSelectedSegs();
            toggleOnOff();
          }
          break;
      }
      applyFinalBri();
      strip.setTransition(transitionDelay); // re-enable transitions
      notify(CALL_MODE_NIGHTLIGHT); // inform clients of brightness & color change
      interfaceUpdateCallMode = CALL_MODE_NIGHTLIGHT;
      if (macroNl > 0) applyPreset(macroNl);
    } else {
      switch (nightlightMode) {
        case NL_MODE_COLORFADE:
          for (unsigned i=0; i<4; i++) colPri[i] = colNlT[i]+ ((colSec[i] - colNlT[i])*tper)/1000;   // fading from actual color to secondary color
          applyValuesToSelectedSegs();
          // fallthrough (legacy behaviour)
        case NL_MODE_FADE: {
          uint8_t briTO = briT;
          briT = briNlT + ((nightlightTargetBri - briNlT)*tper)/1000;
          if (briT != briTO) applyBri();
          break;
        }
      }
    }
  } else if (nightlightActiveOld) { //early de-init (stopped midtransition)
    if (nightlightMode == NL_MODE_SUN) { //restore previous effect
      effectCurrent = colNlT[0];
      effectSpeed = colNlT[1];
      effectPalette = colNlT[2];
      applyValuesToSelectedSegs();
    } else {
      bri = briNlT; // restore original brightness
    }
    nightlightActiveOld = false;
    applyFinalBri();
    strip.setTransition(transitionDelay); // re-enable transitions
    notify(CALL_MODE_NIGHTLIGHT); // inform clients of brightness & color change
    interfaceUpdateCallMode = CALL_MODE_NIGHTLIGHT;
    DEBUG_PRINTLN(F("Nightlight interrupted."));
  }
}

//utility for FastLED to use our custom timer
uint32_t get_millisecond_timer() {
  return strip.now;
}
