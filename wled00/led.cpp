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
void toggleOnOff()
{
  if (bri == 0)
  {
    bri = briLast;
    strip.restartRuntime();
    // we need to switch relay on immediately for delay to work properly
    toggleRelay(true);
  } else
  {
    briLast = bri;
    bri = 0;
    // we will switch off relay in handleOnOff() when transition finishes
  }
  stateChanged = true;
}


//applies global temporary brightness (briT) to strip (during on/off transition)
void applyBri() {
  if (realtimeOverride || !(realtimeMode && arlsForceMaxBri))
  {
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
  //call for notifier -> 0: init 1: direct change 2: button 3: notification 4: nightlight 5: other (No notification)
  //                     6: fx changed 7: hue 8: preset cycle 9: blynk 10: alexa 11: ws send only 12: button preset
  setValuesFromFirstSelectedSeg();  // a much better approach would be to use main segment: setValuesFromMainSeg()

  if (bri != briOld || stateChanged) {
    if (stateChanged) currentPreset = 0; //something changed, so we are no longer in the preset

    if (callMode != CALL_MODE_NOTIFICATION && callMode != CALL_MODE_NO_NOTIFY) notify(callMode);
    if (bri != briOld && nodeBroadcastEnabled) sendSysInfoUDP(); // update on state

    //set flag to update ws and mqtt
    interfaceUpdateCallMode = callMode;
  } else {
    if (nightlightActive && !nightlightActiveOld && callMode != CALL_MODE_NOTIFICATION && callMode != CALL_MODE_NO_NOTIFY) {
      notify(CALL_MODE_NIGHTLIGHT);
      interfaceUpdateCallMode = CALL_MODE_NIGHTLIGHT;
    }
  }

  unsigned long now = millis();
  if (callMode != CALL_MODE_NO_NOTIFY && nightlightActive && (nightlightMode == NL_MODE_FADE || nightlightMode == NL_MODE_COLORFADE)) {
    briNlT = bri;
    nightlightDelayMs -= (now - nightlightStartTime);
    nightlightStartTime = now;
  }
  if (briT == 0) {
    if (callMode != CALL_MODE_NOTIFICATION) strip.resetTimebase(); //effect start from beginning
  }

  if (bri > 0) briLast = bri;

  //deactivate nightlight if target brightness is reached
  if (bri == nightlightTargetBri && callMode != CALL_MODE_NO_NOTIFY && nightlightMode != NL_MODE_SUN) nightlightActive = false;

  // notify usermods of state change
  UsermodManager::onStateChange(callMode);

  // state was updated, notifications were sent, determine if we need to set segments into transition mode and fade brightness
  if (strip.getTransition() == 0) {
    // no transition, just apply desitred brightness immediately (even if not changed)
    //if (rlyStartTime) rlyStartTime = 0;
    jsonTransitionOnce = false;
    transitionActive = false;
    applyFinalBri();
    strip.trigger();
  } else {
    if (transitionActive) {
      // already active, just update briOld to reflect current state (no further notifications sent during on/off fade)
      briOld = briT;
    } else {
      // since not all effects are updated each frame we will need to force all segments into transtiton mode
      // but we will do that after relay delay has passed
      if (!rlyStartTime && (bri != briOld || stateChanged)) strip.setTransitionMode(true); // force all segments to transition mode
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


void handleNightlight() {
  unsigned long now = millis();
  if (now < 100 && lastNlUpdate > 0) lastNlUpdate = 0; // take care of millis() rollover
  if (now - lastNlUpdate < 100) return; // allow only 10 NL updates per second
  lastNlUpdate = now;

  if (nightlightActive)
  {
    if (!nightlightActiveOld) //init
    {
      nightlightStartTime = now;
      nightlightDelayMs = (unsigned)(nightlightDelayMins*60000);
      nightlightActiveOld = true;
      briNlT = bri;
      for (unsigned i=0; i<4; i++) colNlT[i] = colPri[i]; // remember starting color
      if (nightlightMode == NL_MODE_SUN)
      {
        //save current
        colNlT[0] = effectCurrent;
        colNlT[1] = effectSpeed;
        colNlT[2] = effectPalette;

        strip.getFirstSelectedSeg().setMode(FX_MODE_STATIC); // make sure seg runtime is reset if it was in sunrise mode
        effectCurrent = FX_MODE_SUNRISE;            // colorUpdated() will take care of assigning that to all selected segments
        effectSpeed = nightlightDelayMins;
        effectPalette = 0;
        if (effectSpeed > 60) effectSpeed = 60; //currently limited to 60 minutes
        if (bri) effectSpeed += 60; //sunset if currently on
        briNlT = !bri; //true == sunrise, false == sunset
        if (!bri) bri = briLast;
        colorUpdated(CALL_MODE_NO_NOTIFY);
      }
    }
    float nper = (now - nightlightStartTime)/((float)nightlightDelayMs);
    if (nightlightMode == NL_MODE_FADE || nightlightMode == NL_MODE_COLORFADE)
    {
      bri = briNlT + ((nightlightTargetBri - briNlT)*nper);
      if (nightlightMode == NL_MODE_COLORFADE)                                         // color fading only is enabled with "NF=2"
      {
        for (unsigned i=0; i<4; i++) colPri[i] = colNlT[i]+ ((colSec[i] - colNlT[i])*nper);   // fading from actual color to secondary color
      }
      colorUpdated(CALL_MODE_NO_NOTIFY);
    }
    if (nper >= 1) //nightlight duration over
    {
      nightlightActive = false;
      if (nightlightMode == NL_MODE_SET)
      {
        bri = nightlightTargetBri;
        colorUpdated(CALL_MODE_NO_NOTIFY);
      }
      if (bri == 0) briLast = briNlT;
      if (nightlightMode == NL_MODE_SUN)
      {
        if (!briNlT) { //turn off if sunset
          effectCurrent = colNlT[0];
          effectSpeed = colNlT[1];
          effectPalette = colNlT[2];
          toggleOnOff();
          applyFinalBri();
        }
      }

      if (macroNl > 0)
        applyPreset(macroNl);
      nightlightActiveOld = false;
    }
  } else if (nightlightActiveOld) //early de-init
  {
    if (nightlightMode == NL_MODE_SUN) { //restore previous effect
      effectCurrent = colNlT[0];
      effectSpeed = colNlT[1];
      effectPalette = colNlT[2];
      colorUpdated(CALL_MODE_NO_NOTIFY);
    }
    nightlightActiveOld = false;
  }
}

//utility for FastLED to use our custom timer
uint32_t get_millisecond_timer() {
  return strip.now;
}
