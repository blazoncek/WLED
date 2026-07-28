/*
  FXparticleSystem.cpp

  Particle system with functions for particle generation, particle movement and particle rendering to RGB matrix.
  by DedeHai (Damian Schneider) 2013-2024

  Copyright (c) 2024  Damian Schneider
  Licensed under the EUPL v. 1.2 or later
*/

#include "wled.h"
#include "FXparticleSystem.h"

#if !defined(WLED_DISABLE_PARTICLESYSTEM2D)
//////////////////////////////
// Shared Utility Functions //
//////////////////////////////

// limit speed of particles (used in 1D and 2D)
static inline constexpr int32_t limitSpeed(const int32_t speed) {
  return constrain(speed, -PS_P_MAXSPEED, PS_P_MAXSPEED); // note: this is slightly faster than using min/max at the cost of 50bytes of flash
}

// calculate the delta speed (dV) value and update the counter for force calculation (is used several times, function saves on codesize)
// force is in 3.4 fixedpoint notation, +/-127
static int32_t calcForce_dv(const int8_t force, uint8_t &counter) {
  int32_t dv = 0;
  if (force != 0) {
    // for small forces, need to use a delay counter
    int32_t force_abs = force < 0 ? -force : force; // absolute value (faster than lots of if's only 7 instructions)
    uint8_t fraction = force_abs & 0x0F; // fractional part (4 LSBs)
    /*if (force_abs > 15)*/ dv = force / 16; // MSBs
    // for fraction, need to use a delay counter, apply force only if it overflows
    counter += fraction;
    if (counter > 15) {
      counter -= 16;
      dv += force < 0 ? -1 : 1; // force is either 1 or -1 if it is small (zero force is handled above)
    }
  }
  return dv;
}

// check if particle is out of bounds and wrap it around if required, returns false if out of bounds
static bool checkBoundsAndWrap(int32_t &position, int32_t max, const int32_t particleRadius, const bool wrap) {
  if ((uint32_t)position > (uint32_t)max) { // check if particle reached an edge
    if (wrap) {
      max++; // max is either maxX or maxY, need to increase by one for wrapping calculation
      while (position >= max) position -= max;
      while (position < 0)    position += max;
    } else if (position + particleRadius < 0 || position - particleRadius > max) // particle is leaving boundaries, out of bounds if it has fully left
      return false; // out of bounds
  }
  return true; // particle is in bounds
}

// ParticleSystem2D class functions
// constructor
ParticleSystem2D::ParticleSystem2D(const Segment &seg, uint32_t numberofparticles, uint32_t numberofsources, bool sizecontrol)
: _segment(seg)
{
  PSPRINTLN("ParticleSystem2D constructor");
  numSources = numberofsources; // number of sources allocated in init
  numParticles = numberofparticles; // number of particles allocated in init
  usedParticles = numParticles; // use all particles by default
  advPartSize = nullptr; // will be set in updatePSpointers if sizecontrol is true
  sizeControl = sizecontrol; // size control only makes sense if advanced properties are used
  updatePSpointers(); // initialize pointers to particle data
  setWallHardness(255); // set default wall hardness to max
  setWallRoughness(0); // smooth walls by default
  setGravity(0); //gravity disabled by default
  setMotionBlur(0); //no fading by default
  setSmearBlur(0); //no smearing by default
  emitIndex = 0;
  collisionStartIdx = 0;

  //initialize some default non-zero values most FX use
  for (uint32_t i = 0; i < numParticles; i++) {
    particles[i].sat = 255; // full saturation
    particles[i].size = 1; // default size is antialiased pixel
  }
  for (uint32_t i = 0; i < numSources; i++) {
    sources[i].source.sat = 255; //set saturation to max by default
    sources[i].source.ttl = 1; //set source alive
    sources[i].source.flagsAsByte = 0; // all flags disabled
  }
}

// update function applies gravity, moves the particles, handles collisions and renders the particles
void ParticleSystem2D::update() {
  //apply gravity globally if enabled
  if (particlesettings.useGravity) applyGravity();

  //update size settings before handling collisions
  if (sizeControl) {
    for (uint32_t i = 0; i < usedParticles; i++) {
      if (updateSize(particles[i], &advPartSize[i]) == false) { // if particle shrinks to 0 size
        particles[i].ttl = 0; // kill particle
      }
    }
  }

  // handle collisions (can push particles, must be done before updating particles or they can render out of bounds)
  if (particlesettings.useCollisions) handleCollisions();

  //move all particles
  for (uint32_t i = 0; i < usedParticles; i++) {
    particleMoveUpdate(particles[i], particlesettings); // note: splitting this into two loops is slower and uses more flash
  }

  render();
}

// set percentage of used particles as uint8_t i.e 127 means 50% for example
void ParticleSystem2D::setUsedParticles(uint8_t percentage) {
  usedParticles = (numParticles * ((int)percentage+1)) >> 8; // number of particles to use (percentage is 0-255, 255 = 100%)
  //PSPRINTF(PSTR("SetUsedpaticles: allocated particles: %d, used particles: %d\n"), numParticles, usedParticles);
}

void ParticleSystem2D::setMatrixSize(uint32_t x, uint32_t y) {
  maxXpixel = x - 1; // last physical pixel that can be drawn to
  maxYpixel = y - 1;
  maxX = (x << PS_P_SHIFT) - 1;  // particle system boundary for movements
  maxY = (y << PS_P_SHIFT) - 1;  // this value is often needed (also by FX) to calculate positions
}

// enable/disable gravity, optionally, set the force (force=8 is default) can be -127 to +127, 0 is disable
// if enabled, gravity is applied to all particles in ParticleSystemUpdate()
// force is in 3.4 fixed point notation so force=16 means apply v+1 each frame default of 8 is every other frame (gives good results)
void ParticleSystem2D::setGravity(int8_t force) {
  particlesettings.useGravity = (force != 0);
  gforce = force;
}

void ParticleSystem2D::enableParticleCollisions(bool enable, uint8_t hardness) { // enable/disable gravity, optionally, set the force (force=8 is default) can be 1-255, 0 is also disable
  particlesettings.useCollisions = enable;
  collisionHardness = hardness;
}

// emit one particle with variation, returns index of emitted particle (or -1 if no particle emitted)
int32_t ParticleSystem2D::sprayEmit(const PSsource &emitter) {
  for (uint32_t i = 0; i < usedParticles; i++) {
    if (++emitIndex >= usedParticles) emitIndex = 0;
    if (particles[emitIndex].ttl == 0) { // find a dead particle
      int32_t dx;
      int32_t dy;
      do { // use circular random distribution for large variance to generate nicer "explosions"
        dx = hw_random16(emitter.var << 1) - emitter.var;
        dy = hw_random16(emitter.var << 1) - emitter.var;
      } while (emitter.var > 5 && dx*dx + dy*dy > emitter.var*emitter.var); // reject points outside circle
      particles[emitIndex].vx = emitter.vx + dx;
      particles[emitIndex].vy = emitter.vy + dy;
      // transfer other properties from emitter source particle
      particles[emitIndex].x = emitter.source.x;
      particles[emitIndex].y = emitter.source.y;
      particles[emitIndex].hue = emitter.source.hue;
      particles[emitIndex].sat = emitter.source.sat;
      particles[emitIndex].flagsAsByte = emitter.source.flagsAsByte;
      particles[emitIndex].ttl = hw_random16(emitter.minLife, emitter.maxLife) + emitter.source.ttl;
      particles[emitIndex].size = emitter.source.size;
      particles[emitIndex].mass = emitter.source.mass;
      return emitIndex;
    }
  }
  return -1;
}

// Emits a particle at given angle and speed, angle is from 0-65535 (=0-360deg), speed is also affected by emitter->var
// angle = 0 means in positive x-direction (i.e. to the right)
int32_t ParticleSystem2D::angleEmit(PSsource &emitter, const uint16_t angle, const int32_t speed) {
  emitter.vx = ((int32_t)cos16_t(angle) * speed) / (int32_t)32600; // cos16_t() and sin16_t() return signed 16bit, division should be 32767 but 32600 gives slightly better rounding
  emitter.vy = ((int32_t)sin16_t(angle) * speed) / (int32_t)32600; // note: cannot use bit shifts as bit shifting is asymmetrical for positive and negative numbers and this needs to be accurate!
  return sprayEmit(emitter);
}

// particle moves, decays and dies, if killoutofbounds is set, out of bounds particles are set to ttl=0
// uses passed settings to set bounce or wrap, if useGravity is enabled, it will never bounce at the top and killoutofbounds is not applied over the top
void ParticleSystem2D::particleMoveUpdate(PSparticle &part, PSsettings2D &options) {
  if (part.ttl > 0) {
    if (!part.perpetual) part.ttl--; // age
    if (options.colorByAge) part.hue = min(part.ttl, (uint16_t)255); //set color to ttl

    int32_t newX = part.x + (int32_t)part.vx;
    int32_t newY = part.y + (int32_t)part.vy;
    part.outofbounds = false; // reset out of bounds (in case particle was created outside the matrix and is now moving into view) note: moving this to checks below adds code and is not faster

    // note on particle radius and rendering/bouncing/collisions: original code used particleHardRadius for bouncing and collisions
    // and renderradius for rendering bounds check. The code set particleHardRadius to min(PS_P_PARTICLEHARDRADIUS, particle.size) which
    // was a bit inconsistent (as there was no real definition what particle.size means apart form vague 0=single pixel, 1=2x2 pixels
    // and >1 meant size should be interpreted as 1.015 pixel radius (2) up to 4.5 pixels radius (255))
    // it also set renderradius to be PS_P_HALFRADIUS or particleHardRadius depending on advanced properties being used or not.
    // again very inconsistent or at least not very well defined.
    // this was (probably) meant as a diferentiation between "soft" and "hard" particles (soft boundary), however there was no description
    // what hard radius really meant for rendering or collisions.
    //
    // Simplifying the use of particle's radius for rendering size and collisions/bouncing has disadvantge that there may not be
    // any overlapping of colliding particles (i.e. if they have soft boundary). What is "soft boundary is yet to be defined.
    // particleRadius (as used here, 32..288 in subpixels; 64 subpixels == 1 pixel) is used for both bouncing and collision detection.
    int32_t particleRadius = part.size + (PS_P_RADIUS-1) + (part.size==0); // update radius

    // note: if wall collisions are enabled, bounce them before they reach the edge, it looks much nicer if the particle does not go half out of view
    if (options.bounceY) {
      if ((newY < particleRadius) || ((newY > (maxY - particleRadius)) && !options.useGravity)) { // reached floor / ceiling
         bounce(part.vy, part.vx, newY, maxY, particleRadius);
      }
    } // no else here!
    if (!checkBoundsAndWrap(newY, maxY, particleRadius, options.wrapY)) { // check out of bounds  note: this must not be skipped. if gravity is enabled, particles will never bounce at the top
      part.outofbounds = true;
      if (options.killoutofbounds) {
        if (newY < 0) // if gravity is enabled, only kill particles below ground
          part.ttl = 0;
        else if (!options.useGravity)
          part.ttl = 0;
      }
    }

    if (part.ttl) { //check x direction only if still alive
      if (options.bounceX) {
        if ((newX < particleRadius) || (newX > (maxX - particleRadius))) {// reached a wall
          bounce(part.vx, part.vy, newX, maxX, particleRadius);
        }
      }
      else if (!checkBoundsAndWrap(newX, maxX, particleRadius, options.wrapX)) { // check out of bounds
        part.outofbounds = true;
        if (options.killoutofbounds)
          part.ttl = 0;
      }
    }

    part.x = (int16_t)newX; // set new position
    part.y = (int16_t)newY; // set new position
  }
}

// update advanced particle size control, returns false if particle shrinks to 0 size
bool ParticleSystem2D::updateSize(PSparticle &particle, PSsizeControl *advsize) {
  // no need for pointer check, done in update()/updatePSpointers()
  // grow/shrink particle
  int32_t newsize = particle.size;
  uint32_t counter = advsize->sizecounter;
  uint32_t increment = 0;
  // calculate grow speed using 0-8 for low speeds and 9-15 for higher speeds
  if (advsize->grow) increment = advsize->growspeed;
  else if (advsize->shrink) increment = advsize->shrinkspeed;
  if (increment < 9) { // 8 means +1 every frame
    counter += increment;
    if (counter > 7) {
      counter -= 8;
      increment = 1;
    } else
      increment = 0;
    advsize->sizecounter = counter;
  } else {
    increment = (increment - 8) << 1; // 9 means +2, 10 means +4 etc. 15 means +14
  }

  if (advsize->grow) {
    if (newsize < advsize->maxsize) {
      newsize += increment;
      if (newsize >= advsize->maxsize) {
        advsize->grow = false; // stop growing, shrink from now on if enabled
        newsize = advsize->maxsize; // limit
        if (advsize->pulsate) advsize->shrink = true;
      }
    }
  } else if (advsize->shrink) {
    if (newsize > advsize->minsize) {
      newsize -= increment;
      if (newsize <= advsize->minsize) {
        if (advsize->minsize == 0) return false; // particle shrunk to zero
        advsize->shrink = false; // disable shrinking
        newsize = advsize->minsize; // limit
        if (advsize->pulsate) advsize->grow = true;
      }
    }
  }
  particle.size = newsize;
  // handle wobbling
  if (advsize->wobble) {
    advsize->asymdir += advsize->wobblespeed; // note: if need better wobblespeed control a counter is already in the struct
  }
  return true;
}

// calculate x and y size for asymmetrical particles (advanced size control)
void ParticleSystem2D::getParticleXYsize(int32_t size, uint32_t asymmetry, int32_t asymdir, uint32_t &xsize, uint32_t &ysize) {
  int32_t deviation = (size * asymmetry) >> 8; // deviation from symmetrical size (disallow full "size" as otherwise one size would be zero)
  // Calculate x and y size based on deviation and direction (0 is symmetrical, 64 is x, 128 is symmetrical, 192 is y)
  if (asymdir < 64) {
    deviation = (asymdir * deviation) >> 6;
  } else if (asymdir < 192) {
    deviation = ((128 - asymdir) * deviation) >> 6; // produces both positive and negative deviation
  } else {
    deviation = ((asymdir - 255) * deviation) >> 6; // produces negative deviation
  }
  // Calculate x and y size based on deviation, limit to 255 (rendering function cannot handle larger sizes)
  xsize = min((size - deviation), (int32_t)255);
  ysize = min((size + deviation), (int32_t)255);;
}

// function to bounce a particle from a wall using set parameters (wallHardness and wallRoughness)
void ParticleSystem2D::bounce(int8_t &incomingSpeed, int8_t &perpendicularSpeed, int32_t &position, const uint32_t maxPosition, const uint32_t particleRadius) {
  incomingSpeed = -incomingSpeed;
  incomingSpeed = (int8_t)(((int32_t)incomingSpeed * (int32_t)wallHardness + 128) >> 8); // reduce speed as energy is lost on non-hard surface
  if (position < (int32_t)particleRadius)
    position = particleRadius; // fast particles will never reach the edge if position is inverted, this looks better
  else
    position = maxPosition - particleRadius;
  if (wallRoughness) {
    int32_t incomingSpeed_abs = abs((int32_t)incomingSpeed);
    int32_t totalspeed = incomingSpeed_abs + abs((int32_t)perpendicularSpeed);
    // transfer the amount of incomingSpeed speed to perpendicular speed
    int32_t donatespeed = ((hw_random16(incomingSpeed_abs << 1) - incomingSpeed_abs) * (int32_t)wallRoughness) / 255; // take random portion of + or - perpendicularSpeed speed, scaled by roughness
    perpendicularSpeed = limitSpeed((int32_t)perpendicularSpeed + donatespeed);
    // give the remainder of the speed to perpendicularSpeed speed
    donatespeed = int8_t(totalspeed - abs(perpendicularSpeed)); // keep total speed the same
    incomingSpeed = incomingSpeed > 0 ? donatespeed : -donatespeed;
  }
}

// apply a force in x,y direction to individual particle
// caller needs to provide a 8bit counter (for each particle) that holds its value between calls
// force is in 3.4 fixed point notation so force=16 means apply v+1 each frame default of 8 is every other frame (gives good results)
void ParticleSystem2D::applyForce(PSparticle &part, const int8_t xforce, const int8_t yforce) {
  // for small forces, need to use a delay counter
  uint8_t xcounter = part.xcounter; // lower four bits
  uint8_t ycounter = part.ycounter; // upper four bits

  //if (SEGENV.call % 42 == 0) PSPRINTF(PSTR("Applying fx:%d fy:%d to particle @(%d,%d) with vx:%d vy:%d\n"), (int)xforce, (int)yforce, (int)part.x, (int)part.y, (int)part.vx, (int)part.vy);
  // velocity increase
  int32_t dvx = calcForce_dv(xforce, xcounter);
  int32_t dvy = calcForce_dv(yforce, ycounter);

  // save counter values back
  part.xcounter = xcounter & 0x0F; // write lower four bits, make sure not to write more than 4 bits
  part.ycounter = ycounter & 0x0F; // write upper four bits

  // apply the force to particle
  part.vx = limitSpeed((int32_t)part.vx + dvx);
  part.vy = limitSpeed((int32_t)part.vy + dvy);
  //if (SEGENV.call % 42 == 0) PSPRINTF(PSTR(" New velocity vx:%d vy:%d and counters x:%d y:%d\n"), (int)part.vx, (int)part.vy, (int)part.xcounter, (int)part.ycounter);
}

// apply a force in angular direction to single particle
// caller needs to provide a 8bit counter that holds its value between calls (if using single particles, a counter for each particle is needed)
// angle is from 0-65535 (=0-360deg) angle = 0 means in positive x-direction (i.e. to the right)
// force is in 3.4 fixed point notation so force=16 means apply v+1 each frame (useful force range is +/- 127)
void ParticleSystem2D::applyAngleForce(PSparticle &part, const int8_t force, const uint16_t angle) {
  int8_t xforce = ((int32_t)force * cos16_t(angle)) / 32767; // force is +/- 127
  int8_t yforce = ((int32_t)force * sin16_t(angle)) / 32767; // note: cannot use bit shifts as bit shifting is asymmetrical for positive and negative numbers and this needs to be accurate!
  applyForce(part, xforce, yforce);
}

// apply a force in angular direction to all particles
// angle is from 0-65535 (=0-360deg) angle = 0 means in positive x-direction (i.e. to the right)
void ParticleSystem2D::applyAngleForce(const int8_t force, const uint16_t angle) {
  int8_t xforce = ((int32_t)force * cos16_t(angle)) / 32767; // force is +/- 127
  int8_t yforce = ((int32_t)force * sin16_t(angle)) / 32767; // note: cannot use bit shifts as bit shifting is asymmetrical for positive and negative numbers and this needs to be accurate!
  applyForce(xforce, yforce);
}

// apply gravity to all particles using PS global gforce setting
// force is in 3.4 fixed point notation, see note above
// note: faster than apply force since direction is always down and counter is fixed for all particles
void ParticleSystem2D::applyGravity() {
  uint8_t tempcounter = gforcecounter;
  int32_t dv = calcForce_dv(gforce, tempcounter);
  for (uint32_t i = 0; i < usedParticles; i++) {
    if (particles[i].mass > 0) {
      int8_t adjustedforce = gforce * (particles[i].mass + 31) / 255; // adjust force by mass (32-286 will prevent gforce of 0)
      tempcounter = gforcecounter;
      dv = calcForce_dv(adjustedforce, tempcounter);
    }
    if (dv == 0) continue;
    // Note: not checking if particle is dead is faster as most are usually alive and if few are alive, rendering is fast anyways
    particles[i].vy = limitSpeed((int32_t)particles[i].vy - dv);
    // now let's apply some air resistance based on mass (heavier=less resistance) and particle size (larger=more resistance)
    if (particles[i].mass > 0) {
      int32_t friction = 128 + ((particles[i].mass * 128) >> 8); // mass 0-255 gives friction 128-255 (friction==255 means no speed change)
      friction -= (particles[i].size * 64) >> 8; // size 0-255 gives friction reduction of 0-63
      particles[i].vy = ((int32_t)particles[i].vy * friction) / 255;
    }
  }
  gforcecounter = tempcounter; // save value back
}

// apply gravity to single particle using system settings (use this for sources)
// function does not increment gravity counter, if gravity setting is disabled, this cannot be used
// sources also don't have mass so skip air resistance
void ParticleSystem2D::applyGravity(PSparticle &part, uint8_t tempcounter) {
  int32_t dv = calcForce_dv(gforce, tempcounter);
  part.vy = limitSpeed((int32_t)part.vy - dv);
}

// slow down particle by friction, the higher the speed, the higher the friction. a high friction coefficient slows them more (255 means instant stop)
// note: a coefficient smaller than 0 will speed them up (this is a feature, not a bug), coefficient larger than 255 inverts the speed, so don't do that
void ParticleSystem2D::applyFriction(PSparticle &part, const int32_t coefficient) {
  // note: not checking if particle is dead can be done by caller (or can be omitted)
  #if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(ESP8266) // use bitshifts with rounding instead of division (2x faster)
  int32_t friction = 256 - coefficient;
  part.vx = ((int32_t)part.vx * friction + (((int32_t)part.vx >> 31) & 0xFF)) >> 8; // note: (v>>31) & 0xFF)) extracts the sign and adds 255 if negative for correct rounding using shifts
  part.vy = ((int32_t)part.vy * friction + (((int32_t)part.vy >> 31) & 0xFF)) >> 8;
  #else // division is faster on ESP32, S2 and S3
  int32_t friction = 255 - coefficient;
  part.vx = ((int32_t)part.vx * friction) / 255;
  part.vy = ((int32_t)part.vy * friction) / 255;
  #endif
}

// apply friction to all particles
// note: not checking if particle is dead is faster as most are usually alive and if few are alive, rendering is fast anyways
void ParticleSystem2D::applyFriction(const int32_t coefficient) {
  #if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(ESP8266) // use bitshifts with rounding instead of division (2x faster)
  int32_t friction = 256 - coefficient;
  for (uint32_t i = 0; i < usedParticles; i++) {
    particles[i].vx = ((int32_t)particles[i].vx * friction + (((int32_t)particles[i].vx >> 31) & 0xFF)) >> 8; // note: (v>>31) & 0xFF)) extracts the sign and adds 255 if negative for correct rounding using shifts
    particles[i].vy = ((int32_t)particles[i].vy * friction + (((int32_t)particles[i].vy >> 31) & 0xFF)) >> 8;
  }
  #else // division is faster on ESP32, S2 and S3
  int32_t friction = 255 - coefficient;
  for (uint32_t i = 0; i < usedParticles; i++) {
    particles[i].vx = ((int32_t)particles[i].vx * friction) / 255;
    particles[i].vy = ((int32_t)particles[i].vy * friction) / 255;
  }
  #endif
}

// attracts a particle to an attractor particle using the inverse square-law
void ParticleSystem2D::pointAttractor(const uint32_t particleindex, PSparticle &attractor, const uint8_t strength, const bool swallow) {
  // Calculate the distance between the particle and the attractor
  int32_t dx = attractor.x - particles[particleindex].x;
  int32_t dy = attractor.y - particles[particleindex].y;

  // Calculate the force based on inverse square law
  int32_t distanceSquared = dx * dx + dy * dy;
  if (distanceSquared < 8192) {
    if (swallow) { // particle is close, age it fast so it fades out, do not attract further
      if (particles[particleindex].ttl > 7)
        particles[particleindex].ttl -= 8;
      else {
        particles[particleindex].ttl = 0;
        return;
      }
    }
    distanceSquared = 2 * PS_P_DIAMETER * PS_P_DIAMETER; // limit the distance to avoid very high forces
  }

  int32_t force = ((int32_t)strength << 16) / distanceSquared;
  int8_t xforce = (force * dx) / 1024; // scale to a lower value, found by experimenting
  int8_t yforce = (force * dy) / 1024; // note: if we want to use bitshifts, we'd need to add 1 if (force*dx) is negative, but that is one additional instruction
  applyForce(particles[particleindex], xforce, yforce);
}

// attracts all particles to an attractor particle using the inverse square-law
void ParticleSystem2D::pointAttractor(PSparticle &attractor, const uint8_t strength, const bool swallow) {
  for (uint32_t particleindex = 0; particleindex < usedParticles; particleindex++) {
    pointAttractor(particleindex, attractor, strength, swallow);
  }
}

// render particles to the LED buffer (uses palette to render the 8bit particle color value)
// if wrap is set, particles half out of bounds are rendered to the other side of the matrix
// warning: do not render out of bounds particles or system will crash! rendering does not check if particle is out of bounds
// firemode is only used for PS Fire FX
void ParticleSystem2D::render() {
  CRGBA baseRGB;
  uint32_t brightness; // particle brightness, fades if dying
  TBlendType blend = particlesettings.colorByAge ? LINEARBLEND_NOWRAP : LINEARBLEND; // default color rendering: wrap palette

  if (motionBlur) { // motion-blurring active (fade existing pixels before overlaying new frame)
    _segment.fadeToBlackBy(motionBlur);
  } else { // no motion blurring: clear buffer
    _segment.clear();
  }

  // go over particles and render them to the buffer
  for (uint32_t i = 0; i < usedParticles; i++) {
    if (particles[i].ttl == 0 || particles[i].outofbounds)
      continue;
    // generate RGB values for particle
    brightness = min(particles[i].ttl << 1, 255);
    baseRGB = ColorFromPaletteWLED(SEGPALETTE, particles[i].hue, 255, blend);
    if (particles[i].sat < 255) {
      CHSV32 baseHSV(baseRGB);
      baseHSV.s = min(baseHSV.s, particles[i].sat); // set the saturation but don't increase it
      baseRGB = baseHSV;  // convert HSV back to RGB (preserves opacity)
    }
    //if (gammaCorrectCol) brightness = gamma8(brightness); // apply gamma correction, used for gamma-inverted brightness distribution
    baseRGB.nscale8_video(brightness);
    if (!particles[i].hollow) baseRGB.setOpacity((particles[i].mass>>1) + 128); // use mass as opacity (128-255) for advanced particles
    renderParticle(i, baseRGB, particlesettings.wrapX, particlesettings.wrapY);
  }

  // apply 2D blur to rendered frame
  if (smearBlur) {
    _segment.blur2D(smearBlur, smearBlur, true);
  }
}

// calculate pixel positions and brightness distribution and render the particle to local buffer or global buffer
void ParticleSystem2D::renderParticle(const uint32_t particleindex, CRGBA color, const bool wrapX, const bool wrapY) {
  uint32_t particleRadius = particles[particleindex].size;

  // work with bufferless segment
  void  (Segment::*setPixelXY)(unsigned, unsigned, CRGBA) const = _segment.getPixels() ? &Segment::setPixelColorXYRaw : &Segment::setStripPixelColorXY;
  CRGBA (Segment::*getPixelXY)(unsigned, unsigned) const        = _segment.getPixels() ? &Segment::getPixelColorXYRaw : &Segment::getPixelColorXY;

  // single pixel non-antialiased rendering
  if (particleRadius == 0) {
    const int32_t pxC = (particles[particleindex].x >> PS_P_SHIFT);
    const int32_t pyC = maxYpixel - (particles[particleindex].y >> PS_P_SHIFT);  // flip y coordinate (0,0 is bottom left in PS but top left in framebuffer)
    if ((uint32_t)pxC <= (uint32_t)maxXpixel && (uint32_t)pyC <= (uint32_t)maxYpixel) (_segment.*setPixelXY)(pxC, pyC, (_segment.*getPixelXY)(pxC, pyC).add(color));
    return;
  }

  // this is faster than drawing ellipses for radius 1
  if (particleRadius == 1) {
    const int32_t pxC = particles[particleindex].x << (8 - PS_P_SHIFT);
    const int32_t pyC = (maxY - particles[particleindex].y) << (8 - PS_P_SHIFT);  // flip y coordinate (0,0 is bottom left in PS but top left in framebuffer)
    if ((uint32_t)(pxC>>8) <= (uint32_t)maxXpixel && (uint32_t)(pyC>>8) <= (uint32_t)maxYpixel) _segment.setWuPixelColor(pxC, pyC, color); // handles bufferless segment on its own
    return;
  }

  particleRadius += 31; // adjust for actual subpixel size (radius 1 means 32 in 10.6 fixed point notation, i.e. 0.5 pixel)

  // Draws filled ellipse or circle
  // Note: all coordinates and radii are in 10.6 fixed point notation
  auto drawEllipse = [&](uint16_t cx, uint16_t cy, uint16_t rx, uint16_t ry, bool hollow = false) {
    auto mul106 = [](int16_t a, int16_t b) { return ((int32_t)a * b) >> PS_P_SHIFT; };      // 10.6 fixed point multiplication
    auto int106 = [](int16_t a)            { int32_t s=(a<0?-1:1); return (int16_t)(s * ((s*a) >> PS_P_SHIFT)); }; // convert 10.6 fixed point to integer
    //auto int106 = [](int16_t a)            { return (int16_t)(a / (1<<PS_P_SHIFT)); }; // convert 10.6 fixed point to integer

    // if we want to optimize
    //if (rx + ry == 0) return; // nothing to draw
    //if (rx == 0) rx = ry; // make it a circle
    //if (ry == 0) ry = rx; // make it a circle

    // pre-calculate drawing bounds
    const int32_t pxMin = int106(cx - rx);                        // minimum pixel coordinate for drawing; rounded down
    const int32_t pxMax = int106(cx + rx + (1<<PS_P_SHIFT) - 1);  // maximum pixel coordinate for drawing; rounded up
    const int32_t pyMin = int106(cy - ry);                        // minimum pixel coordinate for drawing; rounded down
    const int32_t pyMax = int106(cy + ry + (1<<PS_P_SHIFT) - 1);  // maximum pixel coordinate for drawing; rounded up
    const int32_t rxSq  = mul106(rx, rx);                         // x radius squared
    const int32_t rySq  = mul106(ry, ry);                         // y radius squared

    // traverse all pixels within bounding box and check if they are within ellipse
    for (int y = pyMin; y < pyMax; y++) {
      const int32_t dy = (y << PS_P_SHIFT) - cy;
      for (int x = pxMin; x < pxMax; x++) {
        const int32_t dx = (x << PS_P_SHIFT) - cx;
        const int32_t dist = ((mul106(dx,dx)<<8)/rxSq) + ((mul106(dy,dy)<<8)/rySq); // dx2/rx2 + dy2/ry2 in fixed point (multiplied by 256 for better precision)
        if (dist > 384 || (hollow && dist < 64)) continue;        // outside ellipse (actually it should be 256 but that will render a smaller ellipse)
        int32_t px = x;
        int32_t py = y;
        if (wrapX) {
          while (px < 0)         px += (maxXpixel + 1);
          while (px > maxXpixel) px -= (maxXpixel + 1);
        }
        if (wrapY) {
          while (py < 0)         py += (maxYpixel + 1);
          while (py > maxYpixel) py -= (maxYpixel + 1);
        }
        if ((unsigned)px <= (unsigned)maxXpixel && (unsigned)py <= (unsigned)maxYpixel) {
          // flip y coordinate (0,0 is bottom left in PS but top left in framebuffer)
          py = maxYpixel - py;
          if (dist > 192 || (hollow && dist < 128)) { // may need tuning!
            CRGBA c = color;
            // apply antialiasing: 384>dist>192 -> 64 to 255; 64<dist<128 -> 64 to 252
            uint8_t alpha = dist > 192 ? 448 - dist : (dist*dist) >> 6; // may need tuning! (including inverse gamma according to @DedeHai)
            (_segment.*setPixelXY)(px, py, (_segment.*getPixelXY)(px, py).nblend(c, alpha)); // may need tuning!
          } else (_segment.*setPixelXY)(px, py, color);
        }
      }
    }
  };

  // using ellipse drawing for advanced size control
  uint32_t rx, ry;
  if (sizeControl && advPartSize[particleindex].asymmetry > 0) {
    getParticleXYsize(particleRadius, advPartSize[particleindex].asymmetry, advPartSize[particleindex].asymdir, rx, ry);
  } else {
    rx = ry = particleRadius;
  }
  // limit to radius from 0.5 to 5 pixels
  rx = constrain(rx, 32, 320);
  ry = constrain(ry, 32, 320);
  // use custom ellipse
  drawEllipse(particles[particleindex].x, particles[particleindex].y, rx, ry, particles[particleindex].hollow);
}

// detect collisions in an array of particles and handle them
// uses binning by dividing the frame into slices in x direction which is efficient if using gravity in y direction (but less efficient for FX that use forces in x direction)
// for code simplicity, no y slicing is done, making very tall matrix configurations less efficient
// note: also tested adding y slicing, it gives diminishing returns, some FX even get slower. FX not using gravity would benefit with a 10% FPS improvement
void ParticleSystem2D::handleCollisions() {
  // note: partices are binned in x-axis, assumption is that no more than half of the particles are in the same bin
  // if they are, collisionStartIdx is increased so each particle collides at least every second frame (which still gives decent collisions)
  constexpr int binWidth = 6 * PS_P_RADIUS; // width of a bin in sub-pixels
  const uint32_t maxBinParticles = max((uint32_t)50, (usedParticles + 1) / 2); // assume no more than half of the particles are in the same bin, do not bin small amounts of particles
  const uint32_t numBins = (maxX + (binWidth - 1)) / binWidth; // number of bins in x direction
  uint16_t binIndices[maxBinParticles]; // creat array on stack for indices, 2kB max for 1024 particles (ESP32_MAXPARTICLES/2)
  uint32_t binParticleCount; // number of particles in the current bin
  uint16_t nextFrameStartIdx = hw_random16(usedParticles); // index of the first particle in the next frame (set to fixed value if bin overflow)
  uint32_t pidx = collisionStartIdx; //start index in case a bin is full, process remaining particles next frame
  //uint32_t particleRadius = particles[pidx].size + (PS_P_RADIUS-1) + (particles[pidx].size==0); // treat size=0 as size=1 for calculations (1 pixel diameter)
  constexpr int32_t overlap = (PS_P_DIAMETER+512)<<1; //particleRadius << 1; // overlap bins to include edge particles to neighbouring bins

  // fill the binIndices array for this bin
  for (uint32_t bin = 0; bin < numBins; bin++) {
    binParticleCount = 0; // reset for this bin
    int32_t binStart = bin * binWidth - overlap; // note: first bin will extend to negative, but that is ok as out of bounds particles are ignored
    int32_t binEnd = binStart + binWidth + (overlap << 1); // add twice the overlap as start is start-overlap, note: last bin can be out of bounds, see above;

    // fill the binIndices array for this bin
    for (uint32_t i = 0; i < usedParticles; i++) {
      if (particles[pidx].ttl > 0) { // is alive
        if (particles[pidx].x >= binStart && particles[pidx].x <= binEnd) { // >= and <= to include particles on the edge of the bin (overlap to ensure boarder particles collide with adjacent bins)
          if (!particles[pidx].outofbounds && particles[pidx].collide) { // particle is in frame and does collide note: checking flags is quite slow and usually these are set, so faster to check here
            if (binParticleCount >= maxBinParticles) { // bin is full, more particles in this bin so do the rest next frame
              nextFrameStartIdx = pidx; // bin overflow can only happen once as bin size is at least half of the particles (or half +1)
              break;
            }
            binIndices[binParticleCount++] = pidx;
          }
        }
      }
      pidx++;
      if (pidx >= usedParticles) pidx = 0; // wrap around
    }

    for (uint32_t i = 0; i < binParticleCount; i++) { // go though all 'higher number' particles in this bin and see if any of those are in close proximity and if they are, make them collide
      uint32_t idx_i = binIndices[i];
      for (uint32_t j = i + 1; j < binParticleCount; j++) { // check against higher number particles
        uint32_t idx_j = binIndices[j];
        uint32_t particleRadius = ((particles[idx_i].size + particles[idx_j].size) >> 1) + PS_P_RADIUS; // use average radius for collision distance
        uint32_t collDistSq = (particleRadius << 1); // collision distance is twice the average radius
        collDistSq = collDistSq * collDistSq; // square it for faster comparison
        const int32_t m1 = particles[idx_i].mass;
        const int32_t m2 = particles[idx_j].mass;
        const int32_t totalMass = m1 + m2;
        const int32_t massratio1 = totalMass ? (m2 << 8) / totalMass : 0; // massratio 1 depends on mass of particle 2, i.e. if 2 is heavier -> higher velocity impact on 1
        const int32_t massratio2 = totalMass ? (m1 << 8) / totalMass : 0; // mass ratio in 8bit fixed point
        // note: using the same logic as in 1D is much slower though it would be more accurate but it is not really needed in 2D
        int32_t dx = (particles[idx_j].x + particles[idx_j].vx) - (particles[idx_i].x + particles[idx_i].vx); // distance with lookahead
        if (dx * dx < collDistSq) { // check x direction, if close, check y direction (squaring is faster than abs() or dual compare)
          int32_t dy = (particles[idx_j].y + particles[idx_j].vy)  - (particles[idx_i].y + particles[idx_i].vy); // distance with lookahead
          if (dy * dy < collDistSq) // particles are close
            collideParticles(particles[idx_i], particles[idx_j], dx, dy, collDistSq, massratio1, massratio2);
        }
      }
    }
  }
  collisionStartIdx = nextFrameStartIdx; // set the start index for the next frame
}

// handle a collision if close proximity is detected, i.e. dx and/or dy smaller than 2*PS_P_DIAMETER
// takes two pointers to the particles to collide and the particle hardness (softer means more energy lost in collision, 255 means full hard)
void ParticleSystem2D::collideParticles(PSparticle &particle1, PSparticle &particle2, int32_t dx, int32_t dy, const uint32_t collDistSq, const int32_t massratio1, const int32_t massratio2) {
  unsigned distanceSquared = dx * dx + dy * dy;

  // Calculate relative velocity
  // note: could zero check but that does not improve overall speed but diminishes it as that is rarely the case and pushing is still required
  const int32_t v1x = (int32_t)particle1.vx;
  const int32_t v2x = (int32_t)particle2.vx;
  const int32_t v1y = (int32_t)particle1.vy;
  const int32_t v2y = (int32_t)particle2.vy;
  int32_t dVx = v2x - v1x;
  int32_t dVy = v2y - v1y;

  // if dx and dy are zero (i.e. same position) give them an offset, if speeds are also zero, also offset them (pushes particles apart if they are clumped before enabling collisions)
  if (distanceSquared == 0) {
    // Adjust positions based on relative velocity direction
    dx = -1;
    if (dVx < 0) // if true, particle2 is on the right side
      dx = 1;
    else if (dVx == 0)
      dVx = 1;

    dy = -1;
    if (dVy < 0)
      dy = 1;
    else if (dVy == 0)
      dVy = 1;

    distanceSquared = 2; // 1 + 1
  }

  // Calculate dot product of relative velocity and relative distance
  int32_t dotProduct = (dx * dVx + dy * dVy); // is always negative if moving towards each other

  if (dotProduct < 0) {// particles are moving towards each other
    /*
    // use conservation of meomentum for calculating new velocities after collision
    const int32_t m1 = particles[particle1idx].mass;
    const int32_t m2 = particles[particle2idx].mass;
    const int32_t totalMass = m1 + m2;
    const int32_t dm  = m2 - m1;
    if (totalMass > 0) {
      if (collisionHardness < 255) {
        // velocity formula (semi elastic):
        //  v1' = (m1*v1 + m2*v2 - e*m2*(v1 - v2)) / (m1 + m2); e = 0..1 (0=perfectly inelastic, 1=elastic)
        //  v2' = (m1*v1 + m2*v2 - e*m1*(v2 - v1)) / (m1 + m2)
        const int32_t momentumX = m1 * v1x + m2 * v2x;
        const int32_t momentumY = m1 * v1y + m2 * v2y;
        const int32_t eX255 = collisionHardness * dVx; // postpone /255 to increase precision
        const int32_t eY255 = collisionHardness * dVy; // postpone /255 to increase precision
        particle1.vx = (momentumX + ((m2 * ( eX255)) / 255)) / totalMass;
        particle1.vy = (momentumY + ((m2 * ( eY255)) / 255)) / totalMass;
        particle2.vx = (momentumX + ((m1 * (-eX255)) / 255)) / totalMass;
        particle2.vy = (momentumY + ((m1 * (-eY255)) / 255)) / totalMass;
      } else {
        // velocity formula (elastic): v1' = (m1 - m2)*v1/(m1 + m2) + 2*m2*v2/(m1 + m2); v2' = (m2 - m1)*v2/(m1 + m2) + 2*m1*v1/(m1 + m2);
        particle1.vx = (((-dm) * v1x) + (2 * m2 * v2x)) / totalMass;
        particle2.vx = ((( dm) * v2x) + (2 * m1 * v1x)) / totalMass;
        particle1.vy = (((-dm) * v1y) + (2 * m2 * v2y)) / totalMass;
        particle2.vy = ((( dm) * v2y) + (2 * m1 * v1y)) / totalMass;
      }
    } else { // both masses are zero, treat as equal mass
      if (collisionHardness < 255) {
        // velocity formula (semi elastic): v1' = (v1 + v2 - e*(v1 - v2)) / 2; v2' = (v1 + v2 + e*(v1 - v2)) / 2; e = 0..1 (0=perfectly inelastic, 1=elastic)
        const int32_t vX = v1x + v2x;
        const int32_t vY = v1y + v2y;
        const int32_t eX = collisionHardness * dVx / 255;
        const int32_t eY = collisionHardness * dVy / 255;
        particle1.vx = (vX + eX) / 2;
        particle2.vx = (vX - eX) / 2;
        particle1.vy = (vY + eY) / 2;
        particle2.vy = (vY - eY) / 2;
      } else {
        // velocity formula (elastic): v1' = v2; v2' = v1;
        std::swap(particle1.vx, particle2.vx);
        std::swap(particle1.vy, particle2.vy);
      }
    }
    */
    // integer math is much faster than using floats (float divisions are slow on all ESPs)
    // overflow check: dx/dy are 7bit, relativV are 8bit -> dotproduct is 15bit, dotproduct/distsquared ist 8b, multiplied by collisionhardness of 8bit. so a 16bit shift is ok, make it 15 to be sure no overflows happen
    // note: cannot use right shifts as bit shifting in right direction is asymmetrical (1>>1=0 / -1>>1=-1) and this needs to be accurate! the trick is: only shift positive numers
    // Calculate new velocities after collision
    int32_t surfacehardness = max(collisionHardness, (int32_t)PS_P_MINSURFACEHARDNESS >> 1); // if particles are soft, the impulse must stay above a limit or collisions slip through at higher speeds, 170 seems to be a good value
    int32_t impulse = (((((-dotProduct) << 15) / distanceSquared) * surfacehardness) >> 8); // note: inverting before bitshift corrects for asymmetry in right-shifts (is slightly faster)

    #if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(ESP8266) // use bitshifts with rounding instead of division (2x faster)
    int32_t ximpulse = (impulse * dx + ((dx >> 31) & 0x7FFF)) >> 15; // note: extracting sign bit and adding rounding value to correct for asymmetry in right shifts
    int32_t yimpulse = (impulse * dy + ((dy >> 31) & 0x7FFF)) >> 15;
    #else
    int32_t ximpulse = (impulse * dx) / 32767;
    int32_t yimpulse = (impulse * dy) / 32767;
    #endif
    // if particles have mass use a mass ratio
    if (massratio1 && massratio2) {
      int32_t vx1 = (int32_t)particle1.vx - ((ximpulse * massratio1) >> 7); // mass ratio is in fixed point 8bit, multiply by two to account for the fact that we distribute the impulse to both particles
      int32_t vy1 = (int32_t)particle1.vy - ((yimpulse * massratio1) >> 7);
      int32_t vx2 = (int32_t)particle2.vx + ((ximpulse * massratio2) >> 7);
      int32_t vy2 = (int32_t)particle2.vy + ((yimpulse * massratio2) >> 7);
      // limit speeds to max speed (required if a lot of impulse is transferred from a large to a small particle)
      particle1.vx = limitSpeed(vx1);
      particle1.vy = limitSpeed(vy1);
      particle2.vx = limitSpeed(vx2);
      particle2.vy = limitSpeed(vy2);
    } else {
      particle1.vx -= ximpulse; // note: impulse is inverted, so subtracting it
      particle1.vy -= yimpulse;
      particle2.vx += ximpulse;
      particle2.vy += yimpulse;
    }

    // now use particle roughness to add some randomness to the velocities after collision
    if (particle1.roughness > 0 && particle2.roughness > 0) {
      const int roughness1Factor = particle1.roughness;
      const int halfRoughness1 = particle1.roughness >> 1;
      const int roughness2Factor = particle2.roughness;
      const int halfRoughness2 = particle2.roughness >> 1;
      particle1.vx = limitSpeed((int)particle1.vx + (int)hw_random8(roughness1Factor) - halfRoughness1); // -8..+7
      particle1.vy = limitSpeed((int)particle1.vy + (int)hw_random8(roughness1Factor) - halfRoughness1);
      particle2.vx = limitSpeed((int)particle2.vx + (int)hw_random8(roughness2Factor) - halfRoughness2);
      particle2.vy = limitSpeed((int)particle2.vy + (int)hw_random8(roughness2Factor) - halfRoughness2);
    }

    // if particles are soft, they become 'sticky' i.e. apply some friction (they do pile more nicely and stop sloshing around)
    if (collisionHardness < PS_P_MINSURFACEHARDNESS && (SEGMENT.call & 0x07) == 0) {  // NOTE: using SEGMENT.call here is very unorthodox and not recommended
      const uint32_t coeff = collisionHardness + (255 - PS_P_MINSURFACEHARDNESS);
      // Note: could call applyFriction, but this is faster and speed is key here
      #if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(ESP8266) // use bitshifts with rounding instead of division (2x faster)
      particle1.vx = ((int32_t)particle1.vx * coeff + (((int32_t)particle1.vx >> 31) & 0xFF)) >> 8; // note: (v>>31) & 0xFF)) extracts the sign and adds 255 if negative for correct rounding using shifts
      particle1.vy = ((int32_t)particle1.vy * coeff + (((int32_t)particle1.vy >> 31) & 0xFF)) >> 8;
      particle2.vx = ((int32_t)particle2.vx * coeff + (((int32_t)particle2.vx >> 31) & 0xFF)) >> 8;
      particle2.vy = ((int32_t)particle2.vy * coeff + (((int32_t)particle2.vy >> 31) & 0xFF)) >> 8;
      #else // division is faster on ESP32, S2 and S3
      particle1.vx = ((int32_t)particle1.vx * coeff) / 255;
      particle1.vy = ((int32_t)particle1.vy * coeff) / 255;
      particle2.vx = ((int32_t)particle2.vx * coeff) / 255;
      particle2.vy = ((int32_t)particle2.vy * coeff) / 255;
      #endif
    }
  }

  // particles have volume, push particles apart if they are too close (min collDistSq is PS_P_DIAMETER^2 == 4096)
  // tried lots of configurations, what works best is to give one particle a little velocity. When adding hard pushing things tend to oscillate.
  // when hard pushing by offsetting position without velocity, they tend to sink into each other under gravity.
  // when using hard-pushing and velocity, there are some oscillations and softer particles do not pile nicely.
  // oscillation get worse if pushing both particles so one is chosen somewhat randomly.
  // softer collisions are not perfect on purpose: soft particles should pile up and overlap slightly, if separation is made perfect, it does not have the intended look

  if (distanceSquared < collDistSq /*&& (dVx*dVx + dVy*dVy < 50)*/) { // too close and also slow, push them apart
    bool fairlyrandom = dotProduct & 0x01; //dotprouct LSB should be somewhat random, so no need to calculate a random number
    int32_t pushamount = 1 + ((collDistSq - distanceSquared) >> 10); // found this by experimentation: it means push by 1, push more if overlapping more than 1.4 physical pixels (i.e. larger particles only)
    int8_t pushx = dx > 0 ? -pushamount : pushamount; // particle 1 is on the left
    int8_t pushy = dy > 0 ? -pushamount : pushamount; // particle 1 is below particle 2

    // if they are very soft, stop slow particles completely to make them stick to each other
    if (collisionHardness < 5) {
      if (fairlyrandom) { // do not stop them every frame to avoid groups of particles hanging mid-air
        particle1.vx = 0;
        particle1.vy = 0;
        particle2.vx = 0;
        particle2.vy = 0;
        // hard-push particle 1 only: if both are pushed, this oscillates ever so slightly
        particle1.x += pushx;
        particle1.y += pushy;
      }
    } else {
      if (fairlyrandom) {
        particle1.vx += pushx;
        //particle1.x += pushx;
        particle1.vy += pushy;
        //particle1.y += pushy;
      } else {
        particle2.vx -= pushx;
        //particle2.x -= pushx;
        particle2.vy -= pushy;
        //particle2.y -= pushy;
      }
    }
  }
}

// update "matrix" size and pointers (memory location and size can change dynamically)
// note: do not access the PS class in FX before running this function (or it messes up SEGENV.data)
void ParticleSystem2D::updateSystem(uint32_t w, uint32_t h) {
  setMatrixSize(w, h);
  updatePSpointers(); // needed if memory location of SEGMENT.data changed
}

// PS relies on SEGMENT.data to point to memory block containing the PSParticleSystem2D class plus particles, sprays and additional PS+FX data
// since segment (and also SEGMENT.data) can move in memory (e.g. after changing segment settings), all pointers need to be updated
// this is done in the following way:
// 1. the ParticleSystem2D class is located at SEGMENT.data (this -> SEGMENT.data) and is always copied/moved with segment
// 2. directly after the class in memory are the particles (this + 1) [1 implicitly means sizeof(ParticleSystem2D)]
// 3. directly after the particles are the sources (particles + numParticles)
// 4. directly after the sources are advanced properties (if used)
// 5. directly after the advanced properties are advanced size control (if used)
// 6. directly after that is the first available byte for FX additional data (PSdataEnd)
// when memory (segment or SEGMENT.data) moves (i.e. during reallocation or creating segment copy) the pointers are copied but may/do not point to the correct location anymore
// for particles and sources this is easy to fix as their location is always relative to the class start (this)
// but since advanced properties and advanced size control may not be used, isAdvanced and sizeControl flags are used to determine if they were used
// this is due to the fact that updatePSpointers() is called during object creation when pointers are not yet initialized
void ParticleSystem2D::updatePSpointers() {
  //PSPRINTLN("updatePSpointers");
  // Note on memory alignment:
  // a pointer MUST be 4 byte aligned. sizeof() in a struct/class is always aligned to the largest element. if it contains a 32bit, it will be padded to 4 bytes, 16bit is padded to 2byte alignment.
  // The PS is aligned to 4 bytes, a PSparticle is aligned to 2 and a struct containing only byte sized variables is not aligned at all and may need to be padded when dividing the memoryblock.
  // by making sure that the number of sources and particles is a multiple of 4, padding can be skipped here as alignent is ensured, independent of struct sizes.
  particles = reinterpret_cast<PSparticle *>(this + 1); // pointer to particles
  sources = reinterpret_cast<PSsource *>(particles + numParticles); // pointer to source(s) at data+sizeof(ParticleSystem2D)
  PSdataEnd = reinterpret_cast<uint8_t *>(sources + numSources); // pointer to first available byte after the PS for FX additional data (already aligned to 4 byte boundary)
  if (sizeControl) {
    advPartSize = reinterpret_cast<PSsizeControl *>(PSdataEnd);
    PSdataEnd = reinterpret_cast<uint8_t *>(advPartSize + numParticles);
  }
  //PSPRINTF(PSTR(" particles %p\n sources %p\n size ctrl %p\n end %p\n"), particles, sources, advPartSize, PSdataEnd);
}

//non class functions to use for initialization
static uint32_t calculateNumberOfParticles2D(const uint32_t pixels, const bool sizecontrol) {
  int numberofParticles = pixels;  // 1 particle per pixel (for example 512 particles on 32x16)
  numberofParticles = max(4, min(numberofParticles, MAXPARTICLES_2D)); // limit to 4 - MAXPARTICLES_2D
  // when using size control, reduce number of particles to use the same amount of RAM
  if (sizecontrol) numberofParticles = (numberofParticles * sizeof(PSparticle)) / (sizeof(PSparticle) + sizeof(PSsizeControl));
  // make sure it is a multiple of 4 for proper memory alignment (easier than using padding bytes)
  numberofParticles = (numberofParticles+3) & ~0x03;
  return numberofParticles;
}

static uint32_t calculateNumberOfSources2D(uint32_t pixels, uint32_t requestedsources) {
  int numberofSources = min((pixels) / SOURCEREDUCTIONFACTOR, (uint32_t)requestedsources);
  numberofSources = max(1, min(numberofSources, MAXSOURCES_2D)); // limit
  // make sure it is a multiple of 4 for proper memory alignment
  numberofSources = (numberofSources+3) & ~0x03;
  return numberofSources;
}

//allocate memory for particle system class, particles, sprays plus additional memory requested by FX //TODO: add percentofparticles like in 1D to reduce memory footprint of some FX?
static bool allocateParticleSystemMemory2D(Segment &seg, uint32_t numparticles, uint32_t numsources, bool sizecontrol = false, uint32_t additionalbytes = 0) {
  PSPRINTLN("PS 2D alloc");
  PSPRINTF(PSTR(" numparticles: %d numsources: %d additionalbytes: %d\n"), numparticles, numsources, additionalbytes);
  uint32_t requiredmemory = sizeof(ParticleSystem2D);
  // functions above make sure numparticles is a multiple of 4 bytes (to avoid alignment issues)
  requiredmemory += sizeof(PSparticle) * numparticles;
  if (sizecontrol) requiredmemory += sizeof(PSsizeControl) * numparticles;
  requiredmemory += sizeof(PSsource) * numsources;
  requiredmemory += additionalbytes;
  return seg.allocateData(requiredmemory);
}

// initialize Particle System, allocate additional bytes if needed (pointer to those bytes can be read from particle system class: PSdataEnd)
bool initParticleSystem2D(Segment &seg, ParticleSystem2D *&PartSys, uint32_t requestedsources, uint32_t additionalbytes, bool sizecontrol) {
  PSPRINTLN("PS 2D init");
  if (!strip.isMatrix) return false; // only for 2D
  uint32_t cols = seg.vWidth();
  uint32_t rows = seg.vHeight();
  uint32_t pixels = cols * rows;

  uint32_t numparticles = calculateNumberOfParticles2D(pixels, sizecontrol);
  PSPRINTF(PSTR(" segmentsize: %dx%d particles: %d\n"), cols, rows, numparticles);
  uint32_t numsources = calculateNumberOfSources2D(pixels, requestedsources);
  while (!allocateParticleSystemMemory2D(seg, numparticles, numsources, sizecontrol, additionalbytes) && numparticles >= 4) { // make sure we have at least 4 particles or quit
    numparticles = (numparticles>>1) & ~0x03; // try with less particles
    PSPRINTLN(F("PS 2D alloc failed, trying with less particles..."));
  }
  if (numparticles < 4) {
    PSPRINTLN(F("PS 2D alloc failed, not enough memory!"));
    seg.deallocateData(); // free any previously allocated memory
    return false; // allocation failed
  }

  PartSys = new (SEGENV.data) ParticleSystem2D(seg, numparticles, numsources, sizecontrol); // particle system constructor
  PartSys->setMatrixSize(cols, rows);

  PSPRINTLN(F("2D PS init done"));
  return true;
}

#endif // WLED_DISABLE_PARTICLESYSTEM2D

/*
////////////////////////
// 1D Particle System //
////////////////////////
#ifndef WLED_DISABLE_PARTICLESYSTEM1D

ParticleSystem1D::ParticleSystem1D(uint32_t numberofparticles, uint32_t numberofsources, bool isadvanced) {
  numSources = numberofsources;
  numParticles = numberofparticles; // number of particles allocated in init
  usedParticles = numParticles; // use all particles by default
  advPartProps = nullptr; //make sure we start out with null pointers (just in case memory was not cleared)
  isAdvanced = isadvanced;
  updatePSpointers();
  setWallHardness(255); // set default wall hardness to max
  setGravity(0); //gravity disabled by default
  setParticleSize(0); // 1 pixel size by default
  motionBlur = 0; //no fading by default
  smearBlur = 0; //no smearing by default
  emitIndex = 0;
  collisionStartIdx = 0;

  // initialize some default non-zero values most FX use
  for (uint32_t i = 0; i < numSources; i++) {
    sources[i].source.ttl = 1; //set source alive
    sources[i].sourceFlags.asByte = 0; // all flags disabled
  }

  if (isadvanced) {
    for (uint32_t i = 0; i < numParticles; i++) {
      advPartProps[i].sat = 255; // set full saturation
    }
  }
}

// update function applies gravity, moves the particles, handles collisions and renders the particles
void ParticleSystem1D::update(void) {
  //apply gravity globally if enabled
  if (particlesettings.useGravity) //note: in 1D system, applying gravity after collisions also works but may be worse
    applyGravity();

  // handle collisions (can push particles, must be done before updating particles or they can render out of bounds, causing a crash if using local buffer for speed)
  if (particlesettings.useCollisions)
    handleCollisions();

  //move all particles
  for (uint32_t i = 0; i < usedParticles; i++) {
    particleMoveUpdate(particles[i], particleFlags[i], particlesettings, isAdvanced ? &advPartProps[i] : nullptr);
  }

  render();
}

// set percentage of used particles as uint8_t i.e 127 means 50% for example
void ParticleSystem1D::setUsedParticles(const uint8_t percentage) {
  usedParticles = (numParticles * ((int)percentage+1)) >> 8; // number of particles to use (percentage is 0-255, 255 = 100%)
  PSPRINT(" SetUsedpaticles: allocated particles: ");
  PSPRINT(numParticles);
  PSPRINT(" ,used particles: ");
  PSPRINTLN(usedParticles);
}

void ParticleSystem1D::setSize(const uint32_t x) {
  maxXpixel = x - 1; // last physical pixel that can be drawn to
  maxX = (x << PS_P_SHIFT_1D) - 1;  // particle system boundary for movements
}

// render size, 0 = 1 pixel, 1 = 2 pixel (interpolated), bigger sizes require adanced properties
void ParticleSystem1D::setParticleSize(const uint8_t size) {
  particlesize = size > 0 ? 1 : 0; // TODO: add support for global sizes? see note above (motion blur)
  particleHardRadius = PS_P_RADIUS_1D >> (!particlesize); // 2 pixel sized particles or single pixel sized particles
}

// enable/disable gravity, optionally, set the force (force=8 is default) can be -127 to +127, 0 is disable
// if enabled, gravity is applied to all particles in ParticleSystemUpdate()
// force is in 3.4 fixed point notation so force=16 means apply v+1 each frame default of 8 is every other frame (gives good results)
void ParticleSystem1D::setGravity(const int8_t force) {
  if (force) {
    gforce = force;
    particlesettings.useGravity = true;
  }
  else
    particlesettings.useGravity = false;
}

void ParticleSystem1D::enableParticleCollisions(const bool enable, const uint8_t hardness) {
  particlesettings.useCollisions = enable;
  collisionHardness = hardness;
}

// emit one particle with variation, returns index of last emitted particle (or -1 if no particle emitted)
int32_t ParticleSystem1D::sprayEmit(const PSsource1D &emitter) {
  for (uint32_t i = 0; i < usedParticles; i++) {
    emitIndex++;
    if (emitIndex >= usedParticles)
      emitIndex = 0;
    if (particles[emitIndex].ttl == 0) { // find a dead particle
      particles[emitIndex].vx = emitter.v + hw_random16(emitter.var << 1) - emitter.var; // random(-var,var)
      particles[emitIndex].x = emitter.source.x;
      particles[emitIndex].hue = emitter.source.hue;
      particles[emitIndex].ttl = hw_random16(emitter.minLife, emitter.maxLife);
      particleFlags[emitIndex].collide = emitter.sourceFlags.collide; // TODO: could just set all flags (asByte) but need to check if that breaks any of the FX
      particleFlags[emitIndex].reversegrav = emitter.sourceFlags.reversegrav;
      particleFlags[emitIndex].perpetual = emitter.sourceFlags.perpetual;
      if (isAdvanced) {
        advPartProps[emitIndex].sat = emitter.sat;
        advPartProps[emitIndex].size = emitter.size;
      }
      return emitIndex;
    }
  }
  return -1;
}

// particle moves, decays and dies, if killoutofbounds is set, out of bounds particles are set to ttl=0
// uses passed settings to set bounce or wrap, if useGravity is set, it will never bounce at the top and killoutofbounds is not applied over the top
void ParticleSystem1D::particleMoveUpdate(PSparticle1D &part, PSparticleFlags1D &partFlags, PSsettings1D &options, PSadvancedParticle1D *advancedproperties) {
  if (part.ttl > 0) {
    if (!partFlags.perpetual)
      part.ttl--; // age
    if (options.colorByAge)
      part.hue = min(part.ttl, (uint16_t)255); // set color to ttl

    int32_t renderradius = PS_P_RADIUS_1D; // used to check out of bounds, default for 2 pixel rendering
    int32_t newX = part.x + (int32_t)part.vx;
    partFlags.outofbounds = false; // reset out of bounds (in case particle was created outside the matrix and is now moving into view)

    if (advancedproperties) { // using individual particle size?
      if (advancedproperties->size > 1)
        particleHardRadius = PS_P_RADIUS_1D + (advancedproperties->size >> 1);
      else // single pixel particles use half the collision distance for walls
        particleHardRadius = PS_P_RADIUS_1D >> 1;
      renderradius = particleHardRadius; // note: for single pixel particles, it should be zero, but it does not matter as out of bounds checking is done in rendering function
    }

    // if wall collisions are enabled, bounce them before they reach the edge, it looks much nicer if the particle is not half out of view
    if (options.bounce) {
      if ((newX < (int32_t)particleHardRadius) || ((newX > (int32_t)(maxX - particleHardRadius)))) { // reached a wall
        bool bouncethis = true;
        if (options.useGravity) {
          if (partFlags.reversegrav) { // skip bouncing at x = 0
            if (newX < (int32_t)particleHardRadius)
              bouncethis = false;
          } else if (newX > (int32_t)particleHardRadius) { // skip bouncing at x = max
            bouncethis = false;
          }
        }
        if (bouncethis) {
          part.vx = -part.vx; // invert speed
          part.vx = ((int32_t)part.vx * (int32_t)wallHardness) / 255; // reduce speed as energy is lost on non-hard surface
          if (newX < (int32_t)particleHardRadius)
            newX = particleHardRadius; // fast particles will never reach the edge if position is inverted, this looks better
          else
            newX = maxX - particleHardRadius;
        }
      }
    }

    if (!checkBoundsAndWrap(newX, maxX, renderradius, options.wrap)) { // check out of bounds note: this must not be skipped or it can lead to crashes
      partFlags.outofbounds = true;
      if (options.killoutofbounds) {
        bool killthis = true;
        if (options.useGravity) { // if gravity is used, only kill below 'floor level'
          if (partFlags.reversegrav) { // skip at x = 0, do not skip far out of bounds
            if (newX < 0 || newX > maxX << 2)
              killthis = false;
          } else { // skip at x = max, do not skip far out of bounds
            if (newX > 0 &&  newX < maxX << 2)
              killthis = false;
          }
        }
        if (killthis)
          part.ttl = 0;
      }
    }

    if (!partFlags.fixed)
      part.x = newX; // set new position
    else
      part.vx = 0; // set speed to zero. note: particle can get speed in collisions, if unfixed, it should not speed away

    if (particlesettings.colorByPosition)
      part.hue = (255 * part.x) / maxX; // note: x is > 0 if not out of bounds
  }
}

// apply a force in x direction to individual particle (or source)
// caller needs to provide a 8bit counter (for each paticle) that holds its value between calls
// force is in 3.4 fixed point notation so force=16 means apply v+1 each frame default of 8 is every other frame
void ParticleSystem1D::applyForce(PSparticle1D &part, const int8_t xforce, uint8_t &counter) {
  int32_t dv = calcForce_dv(xforce, counter); // velocity increase
  part.vx = limitSpeed((int32_t)part.vx + dv);   // apply the force to particle
}

// apply a force to all particles
// force is in 3.4 fixed point notation (see above)
void ParticleSystem1D::applyForce(const int8_t xforce) {
  int32_t dv = calcForce_dv(xforce, forcecounter); // velocity increase
  for (uint32_t i = 0; i < usedParticles; i++) {
    particles[i].vx = limitSpeed((int32_t)particles[i].vx + dv);
  }
}

// apply gravity to all particles using PS global gforce setting
// gforce is in 3.4 fixed point notation, see note above
void ParticleSystem1D::applyGravity() {
  int32_t dv_raw = calcForce_dv(gforce, gforcecounter);
  for (uint32_t i = 0; i < usedParticles; i++) {
    int32_t dv = dv_raw;
    if (particleFlags[i].reversegrav) dv = -dv_raw;
    // note: not checking if particle is dead is omitted as most are usually alive and if few are alive, rendering is fast anyways
    particles[i].vx = limitSpeed((int32_t)particles[i].vx - dv);
  }
}

// apply gravity to single particle using system settings (use this for sources)
// function does not increment gravity counter, if gravity setting is disabled, this cannot be used
void ParticleSystem1D::applyGravity(PSparticle1D &part, PSparticleFlags1D &partFlags) {
  uint32_t counterbkp = gforcecounter;
  int32_t dv = calcForce_dv(gforce, gforcecounter);
  if (partFlags.reversegrav) dv = -dv;
  gforcecounter = counterbkp; //save it back
  part.vx = limitSpeed((int32_t)part.vx - dv);
}


// slow down particle by friction, the higher the speed, the higher the friction. a high friction coefficient slows them more (255 means instant stop)
// note: a coefficient smaller than 0 will speed them up (this is a feature, not a bug), coefficient larger than 255 inverts the speed, so don't do that
void ParticleSystem1D::applyFriction(int32_t coefficient) {
  #if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(ESP8266) // use bitshifts with rounding instead of division (2x faster)
  int32_t friction = 256 - coefficient;
  for (uint32_t i = 0; i < usedParticles; i++) {
    if (particles[i].ttl)
      particles[i].vx = ((int32_t)particles[i].vx * friction + (((int32_t)particles[i].vx >> 31) & 0xFF)) >> 8; // note: (v>>31) & 0xFF)) extracts the sign and adds 255 if negative for correct rounding using shifts
  }
  #else // division is faster on ESP32, S2 and S3
  int32_t friction = 255 - coefficient;
  for (uint32_t i = 0; i < usedParticles; i++) {
    if (particles[i].ttl)
      particles[i].vx = ((int32_t)particles[i].vx * friction) / 255;
  }
  #endif
}


// render particles to the LED buffer (uses palette to render the 8bit particle color value)
// if wrap is set, particles half out of bounds are rendered to the other side of the matrix
// warning: do not render out of bounds particles or system will crash! rendering does not check if particle is out of bounds
void ParticleSystem1D::render() {
  CRGBA baseRGB;
  uint32_t brightness; // particle brightness, fades if dying
  TBlendType blend = LINEARBLEND; // default color rendering: wrap palette
  if (particlesettings.colorByAge || particlesettings.colorByPosition) {
    blend = LINEARBLEND_NOWRAP;
  }

  // this deviates from 2D rendering where fadeToBlack is used
  if (motionBlur) { // blurring active
    //SEGMENT.fadeToBlackBy(motionBlur); // fade the segment buffer
    SEGMENT.fadeToSecondaryBy(motionBlur); // fade the segment buffer
  }  else { // no blurring: clear buffer with background color
    //SEGMENT.clear();
    SEGMENT.fill(SEGCOLOR(1));
  }

  // go over particles and render them to the buffer
  for (uint32_t i = 0; i < usedParticles; i++) {
    if ( particles[i].ttl == 0 || particleFlags[i].outofbounds)
      continue;

    // generate RGB values for particle
    brightness = min(particles[i].ttl << 1, (int)255);
    baseRGB = ColorFromPaletteWLED(SEGPALETTE, particles[i].hue, 255, blend);

    if (isAdvanced) { //saturation is advanced property in 1D system
      if (advPartProps[i].sat < 255) {
        CHSV32 baseHSV(baseRGB);
        baseHSV.s = min(baseHSV.s, advPartProps[i].sat); // set the saturation but don't increase it
        baseRGB = baseHSV;
      }
    }
    if(gammaCorrectCol) brightness = gamma8(brightness); // apply gamma correction, used for gamma-inverted brightness distribution
    renderParticle(i, baseRGB, brightness, particlesettings.wrap);
  }

  // apply smear-blur to rendered frame
  if (smearBlur) {
    SEGMENT.blur1D(smearBlur);
  }
}

// calculate pixel positions and brightness distribution and render the particle to local buffer or global buffer
void ParticleSystem1D::renderParticle(const uint32_t particleindex, CRGBA color, uint8_t brightness, const bool wrap) {
  uint32_t size = particlesize;
  if (isAdvanced) // use advanced size properties (1D system has no large size global rendering TODO: add large global rendering?)
    size = advPartProps[particleindex].size;

  if (size == 0) { //single pixel particle, can be out of bounds as oob checking is made for 2-pixel particles (and updating it uses more code)
    uint32_t x =  particles[particleindex].x >> PS_P_SHIFT_1D;
    if (x <= (uint32_t)maxXpixel) { //by making x unsigned there is no need to check < 0 as it will overflow
      SEGMENT.addPixelColorRaw(x, color);
    }
    return;
  }
  //render larger particles

  // add half a radius as the rendering algorithm always starts at the bottom left, this leaves things positive, so shifts can be used, then shift coordinate by a full pixel (x-- below)
  int32_t xoffset = particles[particleindex].x + PS_P_RADIUS_1D;
  int32_t dx = xoffset & (PS_P_RADIUS_1D - 1); //relativ particle position in subpixel space,  modulo replaced with bitwise AND
  int32_t x = xoffset >> PS_P_SHIFT_1D; // divide by PS_P_RADIUS, bitshift of negative number stays negative -> checking below for x < 0 works (but does not when using division)
  int32_t pxlbrightness[2];
  //calculate the brightness values for both pixels using linear interpolation (note: in standard rendering out of frame pixels could be skipped but if checks add more clock cycles over all)
  pxlbrightness[0] = (((int32_t)PS_P_RADIUS_1D - dx) * brightness) >> PS_P_SURFACE_1D;
  pxlbrightness[1] = (dx * brightness) >> PS_P_SURFACE_1D;
  // adjust brightness such that distribution is linear after gamma correction:
  // - scale brigthness with gamma correction (done in render())
  // - apply inverse gamma correction to brightness values
  // - gamma is applied again in show() -> the resulting brightness distribution is linear but gamma corrected in total
  if (gammaCorrectCol) {
    pxlbrightness[0] = gamma8inv(pxlbrightness[0]); // use look-up-table for invers gamma
    pxlbrightness[1] = gamma8inv(pxlbrightness[1]);
  }

  // check if particle has advanced size properties and buffer is available
  if (size > 1) {
    CRGBA renderbuffer[10]; // 10 pixel buffer
    memset(renderbuffer, 0, sizeof(renderbuffer)); // make sure buffer is cleared/also set transparent

    //render particle to a bigger size
    //particle size to pixels: 2 - 63 is 4 pixels, < 128 is 6pixels, < 192 is 8 pixels, bigger is 10 pixels
    //first, render the pixel to the center of the renderbuffer, then apply 1D blurring
    renderbuffer[4] += color.scale8(pxlbrightness[0]);
    renderbuffer[5] += color.scale8(pxlbrightness[1]);
    uint32_t rendersize = 2; // initialize render size, minimum is 4 pixels, it is incremented int he loop below to start with 4
    uint32_t offset = 4; // offset to zero coordinate to write/read data in renderbuffer (actually needs to be 3, is decremented in the loop below)
    uint32_t blurpasses = size/64 + 1; // number of blur passes depends on size, four passes max
    uint32_t bitshift = 0;
    for (uint32_t i = 0; i < blurpasses; i++) {
      if (i == 2) //for the last two passes, use higher amount of blur (results in a nicer brightness gradient with soft edges)
        bitshift = 1;
      rendersize += 2;
      offset--;
      blur(renderbuffer, rendersize, size << bitshift, offset);
      size = size > 64 ? size - 64 : 0;
    }

    // calculate origin coordinates to render the particle to in the framebuffer
    uint32_t xfb_orig = x - (rendersize>>1) + 1 - offset; //note: using uint is fine
    uint32_t xfb; // coordinates in frame buffer to write to note: by making this uint, only overflow has to be checked

    // transfer particle renderbuffer to framebuffer
    for (uint32_t xrb = offset; xrb < rendersize+offset; xrb++) {
      xfb = xfb_orig + xrb;
      if (xfb > (uint32_t)maxXpixel) {
        if (wrap) { // wrap x to the other side if required
          if (xfb > (uint32_t)maxXpixel << 1) // xfb is "negative"
            xfb = (maxXpixel + 1) + (int32_t)xfb; // this always overflows to within bounds
          else
            xfb = xfb % (maxXpixel + 1); // note: without the above "negative" check, this works only for powers of 2
        } else
          continue;
      }
      SEGMENT.addPixelColorRaw(xfb, renderbuffer[xrb]);
    }
  } else { // standard rendering (2 pixels per particle)
    bool pxlisinframe[2] = {true, true};
    int32_t pixco[2]; // physical pixel coordinates of the two pixels representing a particle
    // set the raw pixel coordinates
    pixco[1] = x;  // right pixel
    x--; // shift by a full pixel here, this is skipped above to not do -1 and then +1
    pixco[0] = x;  // left pixel

    // check if any pixels are out of frame
    if (x < 0) { // left pixels out of frame
      if (wrap) // wrap x to the other side if required
        pixco[0] = maxXpixel;
      else
        pxlisinframe[0] = false; // pixel is out of matrix boundaries, do not render
    } else if (pixco[1] > (int32_t)maxXpixel) { // right pixel, only has to be checked if left pixel did not overflow
      if (wrap) // wrap y to the other side if required
        pixco[1] = 0;
      else
        pxlisinframe[1] = false;
    }
    for (uint32_t i = 0; i < 2; i++) {
      if (pxlisinframe[i]) {
        SEGMENT.addPixelColorRaw(pixco[i], color.scale8(pxlbrightness[i]));
      }
    }
  }
}

// detect collisions in an array of particles and handle them
void ParticleSystem1D::handleCollisions() {
  uint32_t collisiondistance = particleHardRadius << 1;
  // note: partices are binned by position, assumption is that no more than half of the particles are in the same bin
  // if they are, collisionStartIdx is increased so each particle collides at least every second frame (which still gives decent collisions)
  constexpr int BIN_WIDTH = 32 * PS_P_RADIUS_1D; // width of each bin, a compromise between speed and accuracy (larger bins are faster but collapse more)
  int32_t overlap = particleHardRadius << 1; // overlap bins to include edge particles to neighbouring bins
  if (isAdvanced) //may be using individual particle size
    overlap += 256; // add 2 * max radius (approximately)
  uint32_t maxBinParticles = max((uint32_t)50, (usedParticles + 1) / 4); // do not bin small amounts, limit max to 1/4 of particles
  uint32_t numBins = (maxX + (BIN_WIDTH - 1)) / BIN_WIDTH; // calculate number of bins
  uint16_t binIndices[maxBinParticles]; // array to store indices of particles in a bin
  uint32_t binParticleCount; // number of particles in the current bin
  uint16_t nextFrameStartIdx = hw_random16(usedParticles); // index of the first particle in the next frame (set to fixed value if bin overflow)
  uint32_t pidx = collisionStartIdx; //start index in case a bin is full, process remaining particles next frame
  for (uint32_t bin = 0; bin < numBins; bin++) {
    binParticleCount = 0; // reset for this bin
    int32_t binStart = bin * BIN_WIDTH - overlap; // note: first bin will extend to negative, but that is ok as out of bounds particles are ignored
    int32_t binEnd = binStart + BIN_WIDTH + overlap; // note: last bin can be out of bounds, see above

    // fill the binIndices array for this bin
    for (uint32_t i = 0; i < usedParticles; i++) {
      if (particles[pidx].ttl > 0) { // alivee
        if (particles[pidx].x >= binStart && particles[pidx].x <= binEnd) { // >= and <= to include particles on the edge of the bin (overlap to ensure boarder particles collide with adjacent bins)
          if(particleFlags[pidx].outofbounds == 0 && particleFlags[pidx].collide) { // particle is in frame and does collide note: checking flags is quite slow and usually these are set, so faster to check here
            if (binParticleCount >= maxBinParticles) { // bin is full, more particles in this bin so do the rest next frame
              nextFrameStartIdx = pidx; // bin overflow can only happen once as bin size is at least half of the particles (or half +1)
              break;
            }
            binIndices[binParticleCount++] = pidx;
          }
        }
      }
      pidx++;
      if (pidx >= usedParticles) pidx = 0; // wrap around
    }

    for (uint32_t i = 0; i < binParticleCount; i++) { // go though all 'higher number' particles and see if any of those are in close proximity and if they are, make them collide
      uint32_t idx_i = binIndices[i];
      for (uint32_t j = i + 1; j < binParticleCount; j++) { // check against higher number particles
        uint32_t idx_j = binIndices[j];
        if (isAdvanced) { // use advanced size properties
          collisiondistance = (PS_P_RADIUS_1D << particlesize) + ((advPartProps[idx_i].size + advPartProps[idx_j].size) >> 1);
        }
        int32_t dx = (particles[idx_j].x + particles[idx_j].vx) - (particles[idx_i].x + particles[idx_i].vx); // distance between particles with lookahead
        uint32_t dx_abs = abs(dx);
        if (dx_abs <= collisiondistance) { // collide if close
          collideParticles(idx_i, idx_j, dx, dx_abs, collisiondistance);
        }
      }
    }
  }
  collisionStartIdx = nextFrameStartIdx; // set the start index for the next frame
}
// handle a collision if close proximity is detected, i.e. dx and/or dy smaller than 2*PS_P_RADIUS
// takes two pointers to the particles to collide and the particle hardness (softer means more energy lost in collision, 255 means full hard)
void ParticleSystem1D::collideParticles(uint32_t particle1idx, uint32_t particle2idx, const int32_t dx, const uint32_t dx_abs, const uint32_t collisiondistance) {
  PSparticle1D &particle1 = particles[particle1idx];
  const PSparticleFlags1D &particle1flags = particleFlags[particle1idx];
  PSparticle1D &particle2 = particles[particle2idx];
  const PSparticleFlags1D &particle2flags = particleFlags[particle2idx];

  const int32_t v1 = (int32_t)particle1.vx;
  const int32_t v2 = (int32_t)particle2.vx;
  const int32_t dv = v2 - v1;
  const int32_t dotProduct = (dx * dv); // is always negative if moving towards each other

  if (dotProduct < 0) { // particles are moving towards each other
    // use conservation of meomentum for calculating new velocities after collision
    if (isAdvanced) {
      const int32_t m1 = advPartProps[particle1idx].mass;
      const int32_t m2 = advPartProps[particle2idx].mass;
      const int32_t totalMass = m1 + m2;
      const int32_t dm  = m2 - m1;
      if (totalMass > 0) {
        if (collisionHardness < 255) {
          // velocity formula (semi elastic):
          //  v1' = (m1*v1 + m2*v2 - e*m2*(v1 - v2)) / (m1 + m2); e = 0..1 (0=perfectly inelastic, 1=elastic)
          //  v2' = (m1*v1 + m2*v2 - e*m1*(v2 - v1)) / (m1 + m2)
          const int32_t momentum = m1 * v1 + m2 * v2;
          const int32_t e255 = collisionHardness * dv; // postpone /255 to increase precision
          particle1.vx = (momentum + ((m2 * ( e255)) / 255)) / totalMass;
          particle2.vx = (momentum + ((m1 * (-e255)) / 255)) / totalMass;
        } else {
          // velocity formula (elastic): v1' = (m1 - m2)*v1/(m1 + m2) + 2*m2*v2/(m1 + m2); v2' = (m2 - m1)*v2/(m1 + m2) + 2*m1*v1/(m1 + m2);
          particle1.vx = (((-dm) * v1) + (2 * m2 * v2)) / totalMass;
          particle2.vx = ((( dm) * v2) + (2 * m1 * v1)) / totalMass;
        }
      } else { // both masses are zero, treat as equal mass
        // velocity formula (elastic): v1' = v2; v2' = v1;
        // velocity formula (semi elastic): v1' = (v1 + v2 - e*(v1 - v2)) / 2; v2' = (v1 + v2 + e*(v1 - v2)) / 2; e = 0..1 (0=perfectly inelastic, 1=elastic)
        if (collisionHardness < 255) {
          const int32_t v = v1 + v2;
          const int32_t ev = collisionHardness * dv / 255;
          particle1.vx = (v + ev) / 2;
          particle2.vx = (v - ev) / 2;
        } else
          std::swap(particle1.vx, particle2.vx);
      }
    } else {
      // velocity formula (elastic): v1' = v2; v2' = v1;
      // velocity formula (semi elastic): v1' = (v1 + v2 - e*(v1 - v2)) / 2; v2' = (v1 + v2 + e*(v1 - v2)) / 2; e = 0..1 (0=perfectly inelastic, 1=elastic)
      if (collisionHardness < 255) {
        const int32_t v = v1 + v2;
        const int32_t ev = collisionHardness * dv / 255;
        particle1.vx = (v + ev) / 2;
        particle2.vx = (v - ev) / 2;
      } else
        std::swap(particle1.vx, particle2.vx);
    }

    // if one of the particles is fixed, transfer the impulse back so it bounces
    if (particle1flags.fixed)
      particle2.vx = -particle1.vx;
    else if (particle2flags.fixed)
      particle1.vx = -particle2.vx;

    if (collisionHardness < PS_P_MINSURFACEHARDNESS_1D && (SEGMENT.call & 0x07) == 0) { // if particles are soft, they become 'sticky' i.e. apply some friction
      const uint32_t coeff = collisionHardness + (250 - PS_P_MINSURFACEHARDNESS_1D);
      #if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(ESP8266) // use bitshifts with rounding instead of division (2x faster)
      particle1.vx = ((int32_t)particle1.vx * coeff + (((int32_t)particle1.vx >> 31) & 0xFF)) >> 8; // note: (v>>31) & 0xFF)) extracts the sign and adds 255 if negative for correct rounding using shifts
      particle2.vx = ((int32_t)particle2.vx * coeff + (((int32_t)particle2.vx >> 31) & 0xFF)) >> 8;
      #else // division is faster on ESP32, S2 and S3
      particle1.vx = ((int32_t)particle1.vx * coeff) / 255;
      particle2.vx = ((int32_t)particle2.vx * coeff) / 255;
      #endif
    }
  }

  if (dx_abs < (collisiondistance - 8) && abs(dv) < 5) { // overlapping and moving slowly
    // particles have volume, push particles apart if they are too close
    // behaviour is different than in 2D, we need pixel accurate stacking here, push the top particle
    // note: like in 2D, pushing by a distance makes softer piles collapse, giving particles speed prevents that and looks nicer
    int32_t pushamount = 1;
    if (dx < 0)  // particle2.x < particle1.x
      pushamount = -pushamount;
    particle1.vx -= pushamount;
    particle2.vx += pushamount;

    if (dx_abs < collisiondistance >> 1) { // too close, force push particles so they dont collapse
      pushamount = 1 + ((collisiondistance - dx_abs) >> 3); // note: push amount found by experimentation

      if (particle1.x < (maxX >> 1)) { // lower half, push particle with larger x in positive direction
        if (dx < 0 && !particle1flags.fixed) {  // particle2.x < particle1.x  -> push particle 1
          particle1.vx++;// += pushamount;
          particle1.x += pushamount;
        }
        else if (!particle2flags.fixed) { // particle1.x < particle2.x  -> push particle 2
          particle2.vx++;// += pushamount;
          particle2.x += pushamount;
        }
      }
      else { // upper half, push particle with smaller x
        if (dx < 0 && !particle2flags.fixed) {  // particle2.x < particle1.x  -> push particle 2
          particle2.vx--;// -= pushamount;
          particle2.x -= pushamount;
        }
        else if (!particle1flags.fixed) { // particle1.x < particle2.x  -> push particle 1
          particle1.vx--;// -= pushamount;
          particle1.x -= pushamount;
        }
      }
    }
  }
}

// update size and pointers (memory location and size can change dynamically)
// note: do not access the PS class in FX befor running this function (or it messes up SEGENV.data)
void ParticleSystem1D::updateSystem(uint32_t len) {
  setSize(len); // update size
  updatePSpointers();
}

void ParticleSystem1D::updatePSpointers() {
  PSPRINTLN("updatePSpointers");
  // Note on memory alignment:
  // a pointer MUST be 4 byte aligned. sizeof() in a struct/class is always aligned to the largest element. if it contains a 32bit, it will be padded to 4 bytes, 16bit is padded to 2byte alignment.
  // The PS is aligned to 4 bytes, a PSparticle is aligned to 2 and a struct containing only byte sized variables is not aligned at all and may need to be padded when dividing the memoryblock.
  // by making sure that the number of sources and particles is a multiple of 4, padding can be skipped here as alignent is ensured, independent of struct sizes.
  particles = reinterpret_cast<PSparticle1D *>(this + 1); // pointer to particles
  particleFlags = reinterpret_cast<PSparticleFlags1D *>(particles + numParticles); // pointer to particle flags
  sources = reinterpret_cast<PSsource1D *>(particleFlags + numParticles); // pointer to source(s)
  PSdataEnd = reinterpret_cast<uint8_t *>(sources + numSources);   // pointer to first available byte after the PS for FX additional data (already aligned to 4 byte boundary)
  if (isAdvanced) {
    advPartProps = reinterpret_cast<PSadvancedParticle1D *>(PSdataEnd);
    PSdataEnd = reinterpret_cast<uint8_t *>(advPartProps + numParticles); // since numParticles is a multiple of 4, this is always aligned to 4 bytes. No need to add padding bytes here
  }
  PSPRINTF(PSTR(" particles %p\n sources %p\n adv. props %p\nend %p\n"), particles, sources, advPartProps, PSdataEnd);
}

//non class functions to use for initialization, fraction is uint8_t: 255 means 100%
static uint32_t calculateNumberOfParticles1D(const uint32_t fraction, const bool isadvanced) {
  uint32_t numberofParticles = SEGMENT.virtualLength();  // one particle per pixel (if possible)
  uint32_t particlelimit = MAXPARTICLES_1D; // maximum number of paticles allowed
  numberofParticles = min(numberofParticles, particlelimit); // limit to particlelimit
  if (isadvanced) // advanced property array needs ram, reduce number of particles to use the same amount
    numberofParticles = (numberofParticles * sizeof(PSparticle1D)) / (sizeof(PSparticle1D) + sizeof(PSadvancedParticle1D));
  numberofParticles = (numberofParticles * (fraction + 1)) >> 8; // calculate fraction of particles
  numberofParticles = numberofParticles < 10 ? 10 : numberofParticles; // 10 minimum
  //make sure it is a multiple of 4 for proper memory alignment (easier than using padding bytes)
  numberofParticles = (numberofParticles+3) & ~0x03; // note: with a separate particle buffer, this is probably unnecessary
  PSPRINTLN(" calc numparticles:" + String(numberofParticles));
  return numberofParticles;
}

static uint32_t calculateNumberOfSources1D(const uint32_t requestedsources) {
  int numberofSources = max(1, min((int)requestedsources,MAXSOURCES_1D)); // limit
  // make sure it is a multiple of 4 for proper memory alignment (so minimum is acutally 4)
  numberofSources = (numberofSources+3) & ~0x03;
  return numberofSources;
}

//allocate memory for particle system class, particles, sprays plus additional memory requested by FX
static bool allocateParticleSystemMemory1D(const uint32_t numparticles, const uint32_t numsources, const bool isadvanced, const uint32_t additionalbytes) {
  uint32_t requiredmemory = sizeof(ParticleSystem1D);
  // functions above make sure these are a multiple of 4 bytes (to avoid alignment issues)
  requiredmemory += sizeof(PSparticleFlags1D) * numparticles;
  requiredmemory += sizeof(PSparticle1D) * numparticles;
  requiredmemory += sizeof(PSsource1D) * numsources;
  requiredmemory += additionalbytes;
  if (isadvanced)
    requiredmemory += sizeof(PSadvancedParticle1D) * numparticles;
  return(SEGMENT.allocateData(requiredmemory));
}

// initialize Particle System, allocate additional bytes if needed (pointer to those bytes can be read from particle system class: PSdataEnd)
// note: percentofparticles is in uint8_t, for example 191 means 75%, (deafaults to 255 or 100% meaning one particle per pixel), can be more than 100% (but not recommended, can cause out of memory)
bool initParticleSystem1D(ParticleSystem1D *&PartSys, const uint32_t requestedsources, const uint8_t fractionofparticles, const uint32_t additionalbytes, const bool advanced) {
  if (SEGLEN == 1) return false; // single pixel not supported
  uint32_t numparticles = calculateNumberOfParticles1D(fractionofparticles, advanced);
  uint32_t numsources = calculateNumberOfSources1D(requestedsources);
  while (!allocateParticleSystemMemory1D(numparticles, numsources, advanced, additionalbytes) && numparticles >= 10) {
    numparticles = (numparticles>>1) & ~0x03; // cut number of particles in half and try again, must be 4 byte aligned
    PSPRINTLN(F("PS 1D alloc failed, trying with less particles..."));
  }
  if (numparticles < 10) {
    PSPRINTLN(F("PS init failed: memory depleted"));
    SEGMENT.deallocateData(); // free memory
    return false; // allocation failed
  }
  PartSys = new (SEGENV.data) ParticleSystem1D(numparticles, numsources, advanced); // particle system constructor
  PartSys->setSize(SEGLEN);

  return true;
}
#endif // WLED_DISABLE_PARTICLESYSTEM1D
*/
