/*
  FXparticleSystem.cpp

  Particle system with functions for particle generation, particle movement and particle rendering to RGB matrix.
  by DedeHai (Damian Schneider) 2013-2024

  Copyright (c) 2024  Damian Schneider
  Licensed under the EUPL v. 1.2 or later
*/

#ifdef WLED_DISABLE_2D
#define WLED_DISABLE_PARTICLESYSTEM2D
#endif

#include <stdint.h>
#include "colors.h"

#define PS_P_MAXSPEED 120 // maximum speed a particle can have (vx/vy is int8)
#define MAX_MEMIDLE 10 // max idle time (in frames) before memory is deallocated (if deallocated during an effect, it will crash!)

//#define WLED_DEBUG_PS // note: enabling debug uses ~3k of flash

#ifdef WLED_DEBUG_PS
  #define PSPRINT(x) DEBUGOUT.print(x)
  #define PSPRINTLN(x) DEBUGOUT.println(x)
  #define PSPRINTF(x...) DEBUGOUT.printf_P(x)
#else
  #define PSPRINT(x)
  #define PSPRINTLN(x)
  #define PSPRINTF(x...)
#endif

#ifndef WLED_DISABLE_PARTICLESYSTEM2D
// memory allocation (based on reasonable segment size and available FX memory)
#ifdef ESP8266
  #define MAXPARTICLES_2D 256
  #define MAXSOURCES_2D 24
  #define SOURCEREDUCTIONFACTOR 8
#elif ARDUINO_ARCH_ESP32S2
  #define MAXPARTICLES_2D 1024
  #define MAXSOURCES_2D 64
  #define SOURCEREDUCTIONFACTOR 6
#else
  #define MAXPARTICLES_2D 2048
  #define MAXSOURCES_2D 128
  #define SOURCEREDUCTIONFACTOR 4
#endif

// particle dimensions (subpixel division)
#define PS_P_DIAMETER 64            // 1 pixel diameter, each pixel is divided by this for particle movement (must be a power of 2)
#define PS_P_RADIUS (PS_P_DIAMETER >> 1)
#define PS_P_HALFRADIUS (PS_P_DIAMETER >> 2)
#define PS_P_SHIFT 6                // shift for 2.6 fixed point notation (2^PS_P_SHIFT = PS_P_DIAMETER)
#define PS_P_SURFACE 12             // shift: 2^PS_P_SURFACE = (PS_P_DIAMETER)^2
#define PS_P_MINSURFACEHARDNESS 128 // minimum hardness used in collision impulse calculation, below this hardness, particles become sticky

// struct for PS settings (shared for 1D and 2D class)
struct PSsettings2D {
  union {
    struct{ // one byte bit field for 2D settings
      bool wrapX           : 1; // LSB
      bool wrapY           : 1;
      bool bounceX         : 1;
      bool bounceY         : 1;
      bool killoutofbounds : 1; // if set, out of bound particles are killed immediately
      bool useGravity      : 1; // set to 1 if gravity is used, disables bounceY at the top
      bool useCollisions   : 1;
      bool colorByAge      : 1; // if set, particle hue is set by ttl value in render function
    };
    uint8_t asByte; // access as a byte, order is: LSB is first entry in the list above
  };
  PSsettings2D(uint8_t byteVal = 0) : asByte(byteVal) {} // constructor to set from byte
};

//struct for a single particle (14 bytes)
struct PSparticle {
  int16_t  x;     // x position in particle system
  int16_t  y;     // y position in particle system
  uint16_t ttl;   // time to live in frames (might be better in 1/10th seconds)
  int8_t   vx;    // horizontal velocity
  int8_t   vy;    // vertical velocity
  uint8_t  hue;   // color hue
  uint8_t  sat;   // particle color saturation
  uint8_t  size;  // particle size, 0 - use global, 1 - antialiased pixel, >1 radius in subpixels+31 (i.e. size=2 means radius=33 subpixels = ~0.5 pixel)
  uint8_t  mass;  // particle mass, used in collisions
  union {
    struct {
      uint8_t xcounter : 4;
      uint8_t ycounter : 4;
    };
    uint8_t   forcecounter; // counter for applying forces to individual particles
  };
  // nameless struct for particle flags
  union {
    struct { // 1 byte
      bool roughness   : 4; // LSB; surface roughness for collisions (0=smooth, 15=very rough)
      bool outofbounds : 1; // out of bounds flag, set to true if particle is outside of display area
      bool collide     : 1; // if set, particle takes part in collisions
      bool perpetual   : 1; // if set, particle does not age (TTL is not decremented in move function, it still dies from killoutofbounds)
      bool hollow      : 1; // is particle hollow (i.e. ring shaped)
    };
    uint8_t flagsAsByte; // access as a byte, order is: LSB is first entry in the list above
  };
  PSparticle(uint16_t t=1, uint8_t h=0, uint8_t s=255, uint8_t sz=0, uint8_t m=0) : x(0), y(0), ttl(t), vx(0), vy(0),
    hue(h), sat(s), size(sz), mass(m), forcecounter(0), flagsAsByte(0) {} // constructor to init all values
};

// struct for advanced particle size control (option)
struct PSsizeControl { // 7 bytes
  uint8_t asymmetry; // asymmetrical size (0=symmetrical, 255 fully asymmetric)
  uint8_t asymdir; // direction of asymmetry, 64 is x, 192 is y (0 and 128 is symmetrical)
  uint8_t maxsize; // target size for growing
  uint8_t minsize; // target size for shrinking
  uint8_t sizecounter   : 4; // counters used for size contol (grow/shrink/wobble)
  uint8_t wobblecounter : 4;
  uint8_t growspeed     : 4;
  uint8_t shrinkspeed   : 4;
  uint8_t wobblespeed   : 4;
  bool grow    : 1; // flags
  bool shrink  : 1;
  bool pulsate : 1; // grows & shrinks & grows & ...
  bool wobble  : 1; // alternate x and y size
  PSsizeControl() : asymmetry(0), asymdir(0), maxsize(255), minsize(1),
    sizecounter(0), wobblecounter(0), growspeed(0), shrinkspeed(0), wobblespeed(0),
    grow(false), shrink(false), pulsate(false), wobble(false) {} // constructor to init all values
};

//struct for a particle source (22 bytes)
struct PSsource {
  uint16_t   minLife; // minimum ttl of emitted particles
  uint16_t   maxLife; // maximum ttl of emitted particles
  PSparticle source;  // use a particle as the emitter source (speed, position, color, source moving speed)
  int8_t     var;     // variation of emitted speed (adds random(+/- var) to speed)
  int8_t     vx;      // emitting particle speed
  int8_t     vy;
};

// class uses 72 bytes
class ParticleSystem2D {
public:
  ParticleSystem2D(const uint32_t numberofparticles, const uint32_t numberofsources, const bool sizecontrol = false); // constructor
  // note: memory is allcated in the FX function, no deconstructor needed
  void update(void); //update the particles according to set options and render to the matrix
  void updateSystem(uint32_t w, uint32_t h); // call at the beginning of every FX, updates pointers and dimensions
  void particleMoveUpdate(PSparticle &part, PSsettings2D &options); // move function
  // particle emitters
  int32_t sprayEmit(const PSsource &emitter);
  int32_t angleEmit(PSsource& emitter, const uint16_t angle, const int32_t speed);
  //particle physics
  void applyGravity(PSparticle &part, uint8_t tempcounter); // applies gravity to single particle (use this for sources)
  [[gnu::hot]] void applyForce(PSparticle &part, const int8_t xforce, const int8_t yforce);
  void applyAngleForce(PSparticle &part, const int8_t force, const uint16_t angle);
  void applyAngleForce(const int8_t force, const uint16_t angle); // apply angular force to all particles
  void applyFriction(PSparticle &part, const int32_t coefficient); // apply friction to specific particle
  void applyFriction(const int32_t coefficient); // apply friction to all used particles
  void pointAttractor(const uint32_t particleindex, PSparticle &attractor, const uint8_t strength, const bool swallow); // use this for advanced property particles
  void pointAttractor(PSparticle &attractor, const uint8_t strength, const bool swallow); // apply to all particles
  void setUsedParticles(const uint8_t percentage);  // set the percentage of particles used in the system, 255=100%
  void setMatrixSize(const uint32_t x, const uint32_t y);
  void setGravity(const int8_t force = 8);
  void enableParticleCollisions(const bool enable, const uint8_t hardness = 255);
  inline void applyForce(const uint32_t particleindex, const int8_t xforce, const int8_t yforce) { applyForce(particles[particleindex], xforce, yforce); }
  inline void applyForce(const int8_t xforce, const int8_t yforce) { for (uint32_t i = 0; i < usedParticles; i++) applyForce(particles[i], xforce, yforce); }
  inline void applyAngleForce(const uint32_t particleindex, const int8_t force, const uint16_t angle) { applyAngleForce(particles[particleindex], force, angle); }
  inline void applyGravity(PSparticle &part)         { applyGravity(part, gforcecounter); } // applies gravity to single particle with global counter (does not increment counter)
  inline void particleMoveUpdate(PSparticle &part)   { particleMoveUpdate(part, particlesettings); } // move function
  inline void setParticleSize(const uint8_t size)    { for (uint32_t i = 0; i < usedParticles; i++) particles[i].size = size; }
  inline void setWallHardness(uint8_t hardness)      { wallHardness = hardness; } // wall roughness randomizes wall collisions
  inline void setWallRoughness(uint8_t roughness)    { wallRoughness = roughness; } // hardness for bouncing on the wall if bounceXY is set
  inline void setCollisionHardness(uint8_t hardness) { collisionHardness = hardness; } // hardness for particle collisions (255 means full hard)
  inline void setWrapX(bool enable)                  { particlesettings.wrapX = enable; }
  inline void setWrapY(bool enable)                  { particlesettings.wrapY = enable; }
  inline void setBounceX(bool enable)                { particlesettings.bounceX = enable; }
  inline void setBounceY(bool enable)                { particlesettings.bounceY = enable; }
  inline void setKillOutOfBounds(bool enable)        { particlesettings.killoutofbounds = enable; } // if enabled, particles outside of matrix instantly die
  inline void setColorByAge(bool enable)             { particlesettings.colorByAge = enable; }
  inline void setMotionBlur(uint8_t bluramount)      { motionBlur = bluramount; } // motion blurring fades frame before rendering new frame on top of previous one
  inline void setSmearBlur(uint8_t bluramount)       { smearBlur = bluramount; } // enable 2D smeared blurring of full frame

  PSparticle *particles; // pointer to particle array
  PSsource   *sources; // pointer to sources
  PSsizeControl *advPartSize; // pointer to advanced particle size control (can be NULL)
  uint8_t    *PSdataEnd; // points to first available byte after the PSmemory, is set in setPointers(). use this for FX custom data
  int32_t    maxX, maxY; // particle system size i.e. width-1 / height-1 in subpixels, Note: all "max" variables must be signed to compare to coordinates (which are signed)
  int32_t    maxXpixel, maxYpixel; // last physical pixel that can be drawn to (FX can read this to read segment size if required), equal to width-1 / height-1
  uint32_t   numSources; // number of sources
  uint32_t   usedParticles; // number of particles used in animation, is relative to 'numParticles'
  //note: some variables are 32bit for speed and code size at the cost of ram

private:
  //rendering functions
  void render();
  [[gnu::hot]] void renderParticle(const uint32_t particleindex, CRGBA color, const bool wrapX, const bool wrapY);
  //paricle physics applied by system if flags are set
  void applyGravity(); // applies gravity to all particles
  void handleCollisions();
  [[gnu::hot]] void collideParticles(PSparticle &particle1, PSparticle &particle2, const int32_t dx, const int32_t dy, const uint32_t collDistSq, const int32_t massratio1, const int32_t massratio2);
  //utility functions
  void updatePSpointers(); // update the data pointers to current segment data space
  bool updateSize(PSparticle &particle, PSsizeControl *advsize); // advanced size control
  void getParticleXYsize(int32_t size, uint32_t asymmetry, int32_t asymdir, uint32_t &xsize, uint32_t &ysize);
  [[gnu::hot]] void bounce(int8_t &incomingSpeed, int8_t &perpendicularSpeed, int32_t &position, const uint32_t maxPosition, const uint32_t particleRadius); // bounce on a wall

  // note: variables that are accessed often are 32bit for speed
  PSsettings2D particlesettings; // settings used when updating particles (can also used by FX to move sources), do not edit properties directly, use functions above
  uint32_t numParticles;  // total number of particles allocated by this system
  uint32_t emitIndex; // index to count through particles to emit so searching for dead pixels is faster
  int32_t  collisionHardness;
  uint32_t wallHardness;
  uint32_t wallRoughness; // randomizes wall collisions
  uint16_t collisionStartIdx; // particle array start index for collision detection
  uint8_t  gforcecounter; // counter for global gravity
  int8_t   gforce; // gravity strength, default is 8 (negative is allowed, positive is downwards)
  // global particle properties for basic particles
  uint8_t  motionBlur; // motion blur, values > 100 gives smoother animations
  uint8_t  smearBlur; // 2D smeared blurring of full frame
  bool     sizeControl;
};

// initialization functions (not part of class)
bool initParticleSystem2D(ParticleSystem2D *&PartSys, const uint32_t requestedsources, const uint32_t additionalbytes = 0, const bool sizecontrol = false);
#endif // WLED_DISABLE_PARTICLESYSTEM2D

/*
////////////////////////
// 1D Particle System //
////////////////////////
#ifndef WLED_DISABLE_PARTICLESYSTEM1D
// memory allocation
#ifdef ESP8266
  #define MAXPARTICLES_1D 320
  #define MAXSOURCES_1D 16
#elif ARDUINO_ARCH_ESP32S2
  #define MAXPARTICLES_1D 1300
  #define MAXSOURCES_1D 32
#else
  #define MAXPARTICLES_1D 2600
  #define MAXSOURCES_1D 64
#endif



// particle dimensions (subpixel division)
#define PS_P_DIAMETER_1D 32 // subpixel size, each pixel is divided by this for particle movement, if this value is changed, also change the shift defines (next two lines)
#define PS_P_RADIUS_1D (PS_P_DIAMETER_1D >> 1)
#define PS_P_SHIFT_1D 5 // 1 << PS_P_SHIFT = PS_P_RADIUS
#define PS_P_SURFACE_1D 5 // shift: 2^PS_P_SURFACE = PS_P_RADIUS_1D
#define PS_P_MINSURFACEHARDNESS_1D 120 // minimum hardness used in collision impulse calculation

// struct for PS settings (shared for 1D and 2D class)
typedef union {
  struct{
  // one byte bit field for 1D settings
  bool wrap : 1;
  bool bounce : 1;
  bool killoutofbounds : 1; // if set, out of bound particles are killed immediately
  bool useGravity : 1; // set to 1 if gravity is used, disables bounceY at the top
  bool useCollisions : 1;
  bool colorByAge : 1; // if set, particle hue is set by ttl value in render function
  bool colorByPosition : 1; // if set, particle hue is set by its position in the strip segment
  bool unused : 1;
  };
  byte asByte; // access as a byte, order is: LSB is first entry in the list above
} PSsettings1D;

//struct for a single particle (8 bytes)
typedef struct {
  int32_t x;  // x position in particle system
  uint16_t ttl; // time to live in frames
  int8_t vx;  // horizontal velocity
  uint8_t hue;  // color hue
} PSparticle1D;

//struct for particle flags
typedef union {
  struct { // 1 byte
    bool outofbounds : 1; // out of bounds flag, set to true if particle is outside of display area
    bool collide : 1; // if set, particle takes part in collisions
    bool perpetual : 1; // if set, particle does not age (TTL is not decremented in move function, it still dies from killoutofbounds)
    bool reversegrav : 1; // if set, gravity is reversed on this particle
    bool forcedirection : 1; // direction the force was applied, 1 is positive x-direction (used for collision stacking, similar to reversegrav) TODO: not used anymore, can be removed
    bool fixed : 1; // if set, particle does not move (and collisions make other particles revert direction),
    bool custom1 : 1; // unused custom flags, can be used by FX to track particle states
    bool custom2 : 1;
  };
  byte asByte; // access as a byte, order is: LSB is first entry in the list above
} PSparticleFlags1D;

// struct for additional particle settings (optional)
typedef struct {
  uint8_t sat; //color saturation
  uint8_t size; // particle size, 255 means 10 pixels in diameter, this overrides global size setting
  uint8_t forcecounter;
  uint8_t mass; // particle mass, used in collisions
} PSadvancedParticle1D;

//struct for a particle source (20 bytes)
typedef struct {
  uint16_t minLife; // minimum ttl of emittet particles
  uint16_t maxLife; // maximum ttl of emitted particles
  PSparticle1D source; // use a particle as the emitter source (speed, position, color)
  PSparticleFlags1D sourceFlags; // flags for the source particle
  int8_t var; // variation of emitted speed (adds random(+/- var) to speed)
  int8_t v; // emitting speed
  uint8_t sat; // color saturation (advanced property)
  uint8_t size; // particle size (advanced property)
  // note: there is 3 bytes of padding added here
} PSsource1D;

class ParticleSystem1D
{
public:
  ParticleSystem1D(const uint32_t numberofparticles, const uint32_t numberofsources, const bool isadvanced = false); // constructor
  // note: memory is allcated in the FX function, no deconstructor needed
  void update(void); //update the particles according to set options and render to the matrix
  void updateSystem(uint32_t len); // call at the beginning of every FX, updates pointers and dimensions
  // particle emitters
  int32_t sprayEmit(const PSsource1D &emitter);
  void particleMoveUpdate(PSparticle1D &part, PSparticleFlags1D &partFlags, PSsettings1D &options, PSadvancedParticle1D *advancedproperties = NULL); // move function
  //particle physics
  [[gnu::hot]] void applyForce(PSparticle1D &part, const int8_t xforce, uint8_t &counter); //apply a force to a single particle
  void applyForce(const int8_t xforce); // apply a force to all particles
  void applyGravity(PSparticle1D &part, PSparticleFlags1D &partFlags); // applies gravity to single particle (use this for sources)
  void applyFriction(const int32_t coefficient); // apply friction to all used particles
  // set options
  void setUsedParticles(const uint8_t percentage); // set the percentage of particles used in the system, 255=100%
  void setSize(const uint32_t x); //set particle system size (= strip length)
  void setParticleSize(const uint8_t size); //size 0 = 1 pixel, size 1 = 2 pixels, is overruled if advanced particle is used
  void setGravity(int8_t force = 8);
  void enableParticleCollisions(bool enable, const uint8_t hardness = 255);
  inline void particleMoveUpdate(PSparticle1D &part, PSparticleFlags1D &partFlags, PSadvancedParticle1D *advancedproperties = nullptr) { particleMoveUpdate(part, partFlags, particlesettings, advancedproperties); } // move function
  //inline void setParticleSize(const uint8_t size)     { particlesize = size; } //size 0 = 1 pixel, size 1 = 2 pixels, is overruled if advanced particle is used
  inline void setWallHardness(const uint8_t hardness) { wallHardness = hardness; }
  inline void setWrap(const bool enable)              { particlesettings.wrap = enable; }
  inline void setBounce(const bool enable)            { particlesettings.bounce = enable; }
  inline void setKillOutOfBounds(const bool enable)   { particlesettings.killoutofbounds = enable; } // if enabled, particles outside of matrix instantly die
  inline void setColorByAge(const bool enable)        { particlesettings.colorByAge = enable; }
  inline void setColorByPosition(const bool enable)   { particlesettings.colorByPosition = enable; }
  inline void setMotionBlur(const uint8_t bluramount) { if (particlesize < 2) motionBlur = bluramount; } // note: motion blur can only be used if 'particlesize' is set to zero
  inline void setSmearBlur(const uint8_t bluramount)  { smearBlur = bluramount; } // enable 1D smeared blurring of full frame

  PSparticle1D *particles; // pointer to particle array
  PSparticleFlags1D *particleFlags; // pointer to particle flags array
  PSsource1D *sources; // pointer to sources
  PSadvancedParticle1D *advPartProps; // pointer to advanced particle properties (can be NULL)
  //PSsizeControl *advPartSize; // pointer to advanced particle size control (can be NULL)
  uint8_t* PSdataEnd; // points to first available byte after the PSmemory, is set in setPointers(). use this for FX custom data
  int32_t maxX; // particle system size i.e. width-1, Note: all "max" variables must be signed to compare to coordinates (which are signed)
  int32_t maxXpixel; // last physical pixel that can be drawn to (FX can read this to read segment size if required), equal to width-1
  uint32_t numSources; // number of sources
  uint32_t usedParticles; // number of particles used in animation, is relative to 'numParticles'

private:
  //rendering functions
  void render(void);
  [[gnu::hot]] void renderParticle(const uint32_t particleindex, CRGBA color, uint8_t brightness, const bool wrap);

  //paricle physics applied by system if flags are set
  void applyGravity(); // applies gravity to all particles
  void handleCollisions();
  [[gnu::hot]] void collideParticles(uint32_t particle1idx, uint32_t particle2idx, const int32_t dx, const uint32_t dx_abs, const uint32_t collisiondistance);

  //utility functions
  void updatePSpointers(); // update the data pointers to current segment data space
  //void updateSize(PSadvancedParticle *advprops, PSsizeControl *advsize); // advanced size control

  // note: variables that are accessed often are 32bit for speed
  PSsettings1D particlesettings; // settings used when updating particles
  uint32_t numParticles;  // total number of particles allocated by this system
  uint32_t emitIndex; // index to count through particles to emit so searching for dead pixels is faster
  int32_t collisionHardness;
  uint32_t particleHardRadius; // hard surface radius of a particle, used for collision detection
  uint32_t wallHardness;
  uint8_t gforcecounter; // counter for global gravity
  int8_t gforce; // gravity strength, default is 8 (negative is allowed, positive is downwards)
  uint8_t forcecounter; // counter for globally applied forces
  uint16_t collisionStartIdx; // particle array start index for collision detection
  //global particle properties for basic particles
  uint8_t particlesize; // global particle size, 0 = 1 pixel, 1 = 2 pixels, is overruled by advanced particle size
  uint8_t motionBlur; // enable motion blur, values > 100 gives smoother animations
  uint8_t smearBlur; // smeared blurring of full frame
  bool isAdvanced;
};

bool initParticleSystem1D(ParticleSystem1D *&PartSys, const uint32_t requestedsources, const uint8_t fractionofparticles = 255, const uint32_t additionalbytes = 0, const bool advanced = false);
#endif // WLED_DISABLE_PARTICLESYSTEM1D
*/
