# Super Mario 64 Codebase Documentation

Generated: January 16, 2026
Repository: n64decomp/sm64

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Core Systems](#core-systems)
3. [Directory Structure](#directory-structure)
4. [Build System](#build-system)
5. [Key Data Structures](#key-data-structures)
6. [Game Loop Flow](#game-loop-flow)
7. [Mario Size Control System](#mario-size-control-system)
8. [Object System](#object-system)
9. [Level System](#level-system)
10. [Collision System](#collision-system)
11. [Camera System](#camera-system)
12. [Graphics System](#graphics-system)
13. [Audio System](#audio-system)
14. [Behavior Scripts](#behavior-scripts)
15. [Interaction System](#interaction-system)
16. [Enhancements](#enhancements)

---

## Architecture Overview

Super Mario 64 is a complete decompilation of the original N64 game. It builds matching ROMs for all regional versions (JP, US, EU, Shindou, iQue).

### Threading Model
- **Idle Thread** - Manages video interface and system
- **Main Thread** (`thread3_main` in `src/game/main.c`) - Handles interrupts and task scheduling
- **Game Loop Thread** (`thread5_game_loop`) - Main game logic
- **Sound Thread** (`thread4_sound`) - Audio processing
- Tasks are dispatched to **RSP (Reality Signal Processor)** for graphics and audio

### Hardware Context
- **N64 Reality Signal Processor (RSP)** - Handles graphics and audio processing
- **Reality Display Processor (RDP)** - Handles rasterization
- **Ultra 64 SDK** - Nintendo's development libraries
- **IDO Compiler** - Original SGI compiler (or GCC for non-matching builds)

---

## Core Systems

### 1. Object System (`src/game/object_list_processor.c`)
- Object pool of 240 objects (`gObjectPool`)
- Objects organized into 8 object lists (0-7)
- Each object has position, rotation, behavior script, and 0x50 variable fields
- Behavior scripts drive object AI and logic

### 2. Mario System (`src/game/mario.c`, `mario_actions_*.c`)
- Mario is a special object with `MarioState` struct
- Actions system: stationary, moving, airborne, submerged, object, automatic, cutscene
- Physics: velocity, position, collision with floors/walls/ceilings
- Animation system for Mario's movements

### 3. Level System (`levels/`)
- Each level has:
  - `script.c` - Level scripts (INIT_LEVEL, LOAD_MIO0, AREA, OBJECT commands)
  - `leveldata.c` - Collision data, models, textures
  - `areas/` - Individual area definitions
- Level scripts define spawn positions, object placements, and loading commands

### 4. Collision System (`src/engine/surface_collision.c`)
- Triangle-based collision detection
- Surfaces have vertices, normals, and flags (terrain type, room, etc.)
- Walls, floors, and ceilings processed separately
- Used for both static level geometry and dynamic objects

### 5. Camera System (`src/game/camera.c` - 40KB)
- Follows Mario with different modes (normal, close, fixed)
- Smooths movement and handles zoom
- Adjusts based on terrain and situation

### 6. Graphics System (`src/engine/`)
- **Graph Nodes** (`graph_node.c`) - Hierarchical scene graph
- **Geo Layouts** (`geo_layout.c`) - Defines model structure
- **Rendering** (`rendering_graph_node.c`) - Processes display lists
- Levels and actors use display lists (GPU command buffers)

### 7. Behavior Scripts (`src/engine/behavior_script.c`)
- Custom bytecode that controls object AI
- Commands: BEGIN, CALL, RETURN, LOOP, DELAY, etc.
- Stack-based with 8-level depth per object
- 200+ behavior functions for different actors

---

## Directory Structure

```
sm64/
├── src/                    # C source code
│   ├── game/             # Game logic (Mario, camera, objects, interactions)
│   │   ├── mario.c       # Main Mario logic, animations, size control
│   │   ├── camera.c      # Camera system (40KB)
│   │   ├── interaction.c # Object interactions
│   │   ├── level_update.c # Level transitions
│   │   ├── behaviors/    # 147 behavior files
│   │   └── mario_actions_*.c # Mario action states
│   ├── engine/           # Core systems (behavior, math, collision, graph)
│   ├── audio/            # Sound and music
│   ├── menu/             # Title, file select, level select
│   ├── goddard/          # Mario face renderer
│   ├── buffers/          # Memory buffers and heaps
│   └── main.c            # Main thread and task scheduling
├── levels/               # Level data, scripts, collision, models
│   ├── bob/             # Bob-omb Battlefield (example level)
│   ├── ttm/             # Tiny Big Island
│   └── [35 levels total]
├── actors/               # 132 actor definitions (enemies, items, NPCs)
├── include/              # Headers and constants
├── lib/                  # N64 SDK library code
├── assets/               # Animations and demo data
├── sound/                # Sequences, samples, sound banks
├── textures/            # Skybox and generic textures
├── tools/                # Build tools (asset conversion, RSP assembler)
└── enhancements/         # Unofficial patches and features
```

---

## Build System

### Makefile (`Makefile`) - Complex build system:
- **Compiler Options:**
  - IDO (original compiler) for matching builds
  - GCC for non-matching builds
- **Version Selection:** JP/US/EU/SH/CN
- **Matching Builds:** Exact ROM hash reproduction
- **Asset Extraction:** Extracts assets from base ROM
- **Format Conversion:** PNGs → N64 formats
- **Compression:** MIO0 compression for data
- **ROM Generation:** ELF → ROM with N64 checksum

### Build Commands
```bash
make                    # Default US version
make VERSION=jp        # Japanese version
make VERSION=eu        # European version
make NON_MATCHING=1    # Use GCC, no hash matching
make COMPARE=0         # Skip ROM hash verification
```

---

## Key Data Structures

### Object (`include/types.h:145`)
```c
struct Object {
    struct ObjectNode header;           // Graphics node + linked list
    struct Object *parentObj;           // Parent object
    struct Object *prevObj;             // Previous object in list
    u32 collidedObjInteractTypes;      // Collision interaction types
    s16 activeFlags;                   // Active/inactive state
    s16 numCollidedObjs;               // Number of collisions
    struct Object *collidedObjs[4];     // Collided objects
    union {
        u32 asU32[0x50];              // 0x50 generic fields
        s32 asS32[0x50];
        s16 asS16[0x50][2];
        f32 asF32[0x50];
        // ... pointer variants
    } rawData;
    const BehaviorScript *curBhvCommand; // Current behavior command
    u32 bhvStackIndex;                 // Behavior stack depth
    uintptr_t bhvStack[8];             // Behavior call stack
    s16 bhvDelayTimer;                 // Behavior delay
    f32 hitboxRadius;                  // Hitbox radius
    f32 hitboxHeight;                  // Hitbox height
    f32 hurtboxRadius;                 // Hurtbox radius
    f32 hurtboxHeight;                 // Hurtbox height
    const BehaviorScript *behavior;   // Behavior script
    struct Object *platform;          // Platform object
    void *collisionData;               // Collision data pointer
    Mat4 transform;                    // Transformation matrix
    void *respawnInfo;                 // Respawn information
};
```

### Surface (`include/types.h:221`)
```c
struct Surface {
    TerrainData type;      // Surface type (terrain)
    TerrainData force;      // Surface force
    s8 flags;              // Surface flags
    RoomData room;         // Room assignment
    TerrainData lowerY;     // Lower Y bound
    TerrainData upperY;     // Upper Y bound
    Vec3Terrain vertex1;   // First vertex
    Vec3Terrain vertex2;   // Second vertex
    Vec3Terrain vertex3;   // Third vertex
    struct {
        f32 x, y, z;       // Normal vector
    } normal;
    f32 originOffset;     // Origin offset
    struct Object *object; // Associated object
};
```

### MarioState (`include/types.h:255`)
```c
struct MarioState {
    u16 unk00;
    u16 input;                    // Controller input flags
    u32 flags;                    // Mario state flags
    u32 particleFlags;            // Active particle effects
    u32 action;                   // Current action
    u32 prevAction;               // Previous action
    u32 terrainSoundAddend;       // Terrain sound modifier
    u16 actionState;              // Action state
    u16 actionTimer;              // Action timer
    u32 actionArg;                // Action argument
    f32 intendedMag;              // Intended movement magnitude
    s16 intendedYaw;              // Intended yaw
    s16 invincTimer;              // Invincibility timer
    u8 framesSinceA;              // Frames since A press
    u8 framesSinceB;              // Frames since B press
    u8 wallKickTimer;             // Wall kick timer
    u8 doubleJumpTimer;          // Double jump timer
    Vec3s faceAngle;              // Facing angles
    Vec3s angleVel;               // Angular velocity
    s16 slideYaw;                 // Slide yaw
    s16 twirlYaw;                // Twirl yaw
    Vec3f pos;                    // Position
    Vec3f vel;                    // Velocity
    f32 forwardVel;               // Forward velocity
    f32 slideVelX;                // Slide velocity X
    f32 slideVelZ;                // Slide velocity Z
    struct Surface *wall;         // Wall surface
    struct Surface *ceil;         // Ceiling surface
    struct Surface *floor;        // Floor surface
    f32 ceilHeight;               // Ceiling height
    f32 floorHeight;              // Floor height
    s16 floorAngle;               // Floor angle
    s16 waterLevel;               // Water level
    struct Object *interactObj;   // Interacting object
    struct Object *heldObj;       // Held object
    struct Object *usedObj;       // Used object
    struct Object *riddenObj;     // Ridden object
    struct Object *marioObj;      // Mario object
    struct SpawnInfo *spawnInfo;  // Spawn information
    struct Area *area;            // Current area
    struct PlayerCameraState *statusForCamera; // Camera status
    struct MarioBodyState *marioBodyState;      // Body state
    struct Controller *controller; // Controller
    struct DmaHandlerList *animList;           // Animation list
    s16 numCoins;                 // Coin count
    s16 numStars;                 // Star count
    s8 numKeys;                   // Key count (unused)
    s8 numLives;                  // Life count
    s16 health;                   // Health
    s16 unkB0;                    // Unknown
    u8 hurtCounter;               // Hurt counter
    u8 healCounter;               // Heal counter
    u8 squishTimer;               // Squish timer
    u8 fadeWarpOpacity;           // Fade warp opacity
    u16 capTimer;                 // Cap timer
    s16 prevNumStarsForDialog;    // Previous stars for dialog
    f32 peakHeight;               // Peak height
    f32 quicksandDepth;           // Quicksand depth
    f32 gettingBlownGravity;      // Wind gravity
    u16 growthTimer;              // Growth timer (unused)
};
```

---

## Game Loop Flow

### Main Loop (`src/game/main.c`)
1. **VBlank** starts (`handle_vblank`)
2. **Audio task** runs if available
3. **Display task** (graphics) runs
4. **Game loop** updates logic:
   - Process controller input
   - Update Mario physics and action
   - Process all objects (behavior scripts)
   - Update camera
   - Check collisions
5. **Display list** generated and sent to RSP
6. Frame rendered to TV

### Mario Update Loop (`src/game/mario.c:1750-1811`)
```c
// Execute action based on action group
switch (m->action & ACT_GROUP_MASK) {
    case ACT_GROUP_MOVING:     inLoop = mario_execute_moving_action(gMarioState); break;
    case ACT_GROUP_AIRBORNE:   inLoop = mario_execute_airborne_action(gMarioState); break;
    case ACT_GROUP_SUBMERGED:  inLoop = mario_execute_submerged_action(gMarioState); break;
    case ACT_GROUP_CUTSCENE:   inLoop = mario_execute_cutscene_action(gMarioState); break;
    case ACT_GROUP_AUTOMATIC:  inLoop = mario_execute_automatic_action(gMarioState); break;
    case ACT_GROUP_OBJECT:     inLoop = mario_execute_object_action(gMarioState); break;
}

// Update systems
sink_mario_in_quicksand(gMarioState);
squish_mario_model(gMarioState);
update_mario_growth(gMarioState);
set_submerged_cam_preset_and_spawn_bubbles(gMarioState);
update_mario_health(gMarioState);
update_mario_info_for_cam(gMarioState);
mario_update_hitbox_and_cap_model(gMarioState);
```

---

## Mario Size Control System

### Overview
Mario's size is controlled via graphics scale in his Object's `header.gfx.scale` vector. This affects visual appearance but not physical collision boundaries.

### Core Data Structure
```c
struct GraphNodeObject {
    Vec3f scale;  // [X, Y, Z] scale factors
    // ...
};
```

### 1. Squish System (`squishTimer`)
**Location:** `src/game/mario.c:1207-1230`

Mario gets squished when caught between floor and ceiling. The `squishTimer` controls the animation:

**Timer Values:**
- **0xFF (255)**: Special value, freezes squish state
- **1-16 frames**: Un-squishing phase (Mario expands back to normal)
- **17+ frames**: Re-squishing phase (Mario becomes flat)

**Scale Changes:**
```c
// Timer 1-16: Gradual un-squish
m->marioObj->header.gfx.scale[1] = 1.0f - ((sSquishScaleOverTime[15 - m->squishTimer] * 0.6f) / 100.0f);
m->marioObj->header.gfx.scale[0] = ((sSquishScaleOverTime[15 - m->squishTimer] * 0.4f) / 100.0f) + 1.0f;

// Timer 17+: Maximum squish
vec3f_set(m->marioObj->header.gfx.scale, 1.4f, 0.4f, 1.4f);
```

**Gameplay Effects:**
- Vertical velocity halved when squished
- Double jump and twirl disabled
- B button input blocked

### 2. Growth System (`growthTimer`) - **MODIFIED**
**Location:** `src/game/mario.c:1236-1255`

```c
#define GROWTH_INTERVAL_FRAMES 300  // 5 seconds at 60fps
#define GROWTH_INCREMENT     0.25f
#define GROWTH_MAX           8.0f
```

Every **5 seconds**, Mario grows by 0.25 on all axes, capped at 8.0 (32× normal size).

**MODIFICATIONS MADE:**
- **Growth Rate:** Changed from 60 seconds to 5 seconds (`GROWTH_INTERVAL_FRAMES`)
- **Persistence:** Growth now persists through warps and cutscenes (only resets on fresh game start)
- **Visual Feedback:** Added sparkle particles and coin sound when growth occurs

### 3. Initial Size
**Location:** `src/game/mario.c:1844`

```c
// Only reset scale on fresh game start, not on warps
if (gMarioState->action == ACT_UNINITIALIZED) {
    vec3f_set(gMarioState->marioObj->header.gfx.scale, 0.25f, 0.25f, 0.25f);
}
```

Mario initializes at **25% scale** (tiny) only on fresh game start.

### 4. Per-Frame Updates
Both systems run every frame in `update_mario_geometry_and_cap_model()`:

```c
squish_mario_model(gMarioState);  // Line 1778
update_mario_growth(gMarioState);   // Line 1779
```

### 5. Visual vs Physical
- **Visual:** Scale affects 3D model rendering
- **Physical:** Collision detection uses unscaled `pos` coordinates
- **Movement:** Squished Mario has reduced jump height
- **Growth:** Size increase is visual-only (doesn't affect collision boundaries)

### 6. Other Scale Manipulations
- **Attack animations:** Hands/feet scale during punches
- **Mirror room:** Mirror Mario has X scale = -1.0 (inverted)
- **Objects:** Many enemies animate their own scale

---

## Object System

### Object Pool Management
- **Pool Size:** 240 objects (`gObjectPool`)
- **Lists:** 8 object lists organized by type/usage
- **Allocation:** From `gFreeObjectList` when spawning
- **Deallocation:** Returns to free list when deactivated

### Object Fields
Each object has 0x50 generic fields accessed via macros:
```c
#define oPosX      OBJECT_FIELD_F32(O_POS_INDEX + 0)
#define oPosY      OBJECT_FIELD_F32(O_POS_INDEX + 1)
#define oPosZ      OBJECT_FIELD_F32(O_POS_INDEX + 2)
#define oVelX      OBJECT_FIELD_F32(0x09)
#define oVelY      OBJECT_FIELD_F32(0x0A)
#define oVelZ      OBJECT_FIELD_F32(0x0B)
#define oForwardVel OBJECT_FIELD_F32(0x0C)
#define oAction    OBJECT_FIELD_S32(0x1A)
#define oTimer     OBJECT_FIELD_S32(0x1B)
// ... many more
```

### Behavior Script System
**Location:** `src/engine/behavior_script.c`

Custom bytecode controlling object AI:

**Key Commands:**
- `BEGIN` - Start behavior
- `CALL` - Call subroutine
- `RETURN` - Return from subroutine
- `LOOP` - Loop execution
- `DELAY` - Wait frames
- `BREAK` - Exit loop

**Stack System:**
- 8-level call stack per object
- `bhvStackIndex` tracks current depth
- `bhvStack[]` stores return addresses

**Example Behavior:**
```c
const BehaviorScript bhv_goomba[] = {
    BEGIN(bhv_goomba),
    LOOP(),
        CALL(0x801CD5A0), // goomba_update
        DELAY(1),
    END(),
};
```

---

## Level System

### Level Script Commands
**Location:** `levels/[level]/script.c`

**Key Commands:**
```c
INIT_LEVEL()                    // Initialize level
LOAD_MIO0(seg, romStart, romEnd) // Load compressed segment
LOAD_RAW(seg, romStart, romEnd)  // Load raw segment
LOAD_MODEL_FROM_GEO(model, geo)  // Load model
AREA(index, geo)                 // Define area
    OBJECT(model, x, y, z, pitch, yaw, roll, bhvParam, bhv) // Spawn object
    OBJECT_WITH_ACTS(...)         // Object for specific acts
    RETURN()                      // End area
JUMP_LINK(script)               // Execute sub-script
```

### Level Structure Example (Bob-omb Battlefield)
```c
const LevelScript level_bob_entry[] = {
    INIT_LEVEL(),
    LOAD_MIO0(0x07, _bob_segment_7SegmentRomStart, _bob_segment_7SegmentRomEnd),
    LOAD_MIO0_TEXTURE(0x09, _generic_mio0SegmentRomStart, _generic_mio0SegmentRomEnd),
    LOAD_MIO0(0x0A, _water_skybox_mio0SegmentRomStart, _water_skybox_mio0SegmentRomEnd),
    LOAD_MIO0(0x05, _group3_mio0SegmentRomStart, _group3_mio0SegmentRomEnd),
    LOAD_RAW(0x0C, _group3_geoSegmentRomStart, _group3_geoSegmentRomEnd),
    ALLOC_LEVEL_POOL(),
    MARIO(MODEL_MARIO, BPARAM4(0x01), bhvMario),
    JUMP_LINK(script_func_global_1),
    JUMP_LINK(script_func_global_4),
    JUMP_LINK(script_func_global_15),
    LOAD_MODEL_FROM_GEO(MODEL_BOB_BUBBLY_TREE, bubbly_tree_geo),
    LOAD_MODEL_FROM_GEO(MODEL_BOB_CHAIN_CHOMP_GATE, bob_geo_000440),
    AREA(1, bob_geo_000488),
        JUMP_LINK(script_func_local_1),
        JUMP_LINK(script_func_local_2),
        JUMP_LINK(script_func_local_3),
        RETURN(),
    END_AREA(),
    FREE_LEVEL_POOL(),
    END(),
};
```

### Area Definitions
Each level has multiple areas with:
- Collision data
- Object placements
- Camera settings
- Environment properties

---

## Collision System

### Surface Types
**Location:** `include/surface_terrains.h`

**Main Categories:**
- **Default:** Normal ground
- **Slippery:** Ice, wet surfaces
- **Very Slippery:** Ice patches
- **Not Slippery:** Rough terrain
- **Dynamic:** Moving platforms

### Collision Detection Process
1. **Wall Collision:** `find_wall_collisions()` - Triangle-based
2. **Floor Collision:** `find_floor()` - Height-based
3. **Ceiling Collision:** `find_ceil()` - Height-based
4. **Dynamic Surfaces:** Moving platforms handled separately

### Collision Data Structure
```c
struct Surface {
    TerrainData type;      // Surface type
    TerrainData force;      // Surface force
    s8 flags;              // Surface flags
    RoomData room;         // Room assignment
    TerrainData lowerY;     // Lower Y bound
    TerrainData upperY;     // Upper Y bound
    Vec3Terrain vertex1;   // First vertex
    Vec3Terrain vertex2;   // Second vertex
    Vec3Terrain vertex3;   // Third vertex
    Vec3f normal;          // Normal vector
    f32 originOffset;     // Origin offset
    struct Object *object; // Associated object
};
```

### Collision Functions
- `find_wall_collisions()` - Process wall collisions
- `find_floor()` - Find floor height
- `find_ceil()` - Find ceiling height
- `f32_find_wall_collision()` - Simplified wall check
- `resolve_and_return_wall_collisions()` - Get most recent wall

---

## Camera System

### Camera Modes
**Location:** `src/game/camera.c`

**Main Modes:**
- `CAMERA_MODE_NORMAL` - Standard follow
- `CAMERA_MODE_CLOSE` - Close-up view
- `CAMERA_MODE_FREE` - Free roaming
- `CAMERA_MODE_C_UP` - First-person view
- `CAMERA_MODE_WATER_SURFACE` - Water surface view
- `CAMERA_MODE_BEHIND_MARIO` - Behind Mario

### Camera Data Structures
```c
struct Camera {
    s16 mode;                    // Current mode
    s16 defMode;                 // Default mode
    Vec3f pos;                   // Camera position
    Vec3f focus;                 // Camera focus point
    Vec3s rot;                   // Camera rotation
    f32 dist;                    // Distance from Mario
    f32 height;                  // Height above Mario
    struct Area *area;           // Current area
    // ... many more fields
};

struct PlayerCameraState {
    Vec3f pos;                   // Position
    Vec3s focus;                 // Focus angles
    Vec3s rot;                   // Rotation
    s16 mode;                    // Mode
    // ... additional state
};
```

### Camera Update Process
1. **Input Processing:** Handle C-up, zoom
2. **Mode Selection:** Choose appropriate mode
3. **Position Calculation:** Compute camera position
4. **Collision Avoidance:** Prevent camera clipping
5. **Smoothing:** Apply movement smoothing
6. **Area Transitions:** Handle area changes

---

## Graphics System

### Graph Node Hierarchy
**Location:** `src/engine/graph_node.c`

**Node Types:**
- `GraphNode` - Base node
- `GraphNodeObject` - 3D object
- `GraphNodeCamera` - Camera
- `GraphNodePerspective` - Perspective projection
- `GraphNodeScale` - Scale transformation
- `GraphNodeRotation` - Rotation
- `GraphNodeTranslation` - Translation
- `GraphNodeDisplayList` - Display list
- `GraphNodeSwitchCase` - Conditional rendering

### Rendering Pipeline
1. **Graph Traversal:** Walk scene graph
2. **Matrix Calculations:** Compute transformations
3. **Display Lists:** Generate GPU commands
4. **RSP Processing:** Send to Reality Signal Processor
5. **RDP Processing:** Send to Reality Display Processor
6. **Frame Output:** Display on screen

### Geo Layouts
**Location:** `src/engine/geo_layout.c`

Define model structure and rendering properties:
```c
const GeoLayout bob_geo_000440[] = {
    GEO_NODE_START(),
    GEO_OPEN_NODE(),
        GEO_ASM(0, geo_movtex_pause_menu),
        GEO_TRANSLATE_NODE(0x00, 0, 0, 0),
        GEO_ROTATION_NODE(0x00, 0, 0, 0),
        GEO_SCALE(0x00, 16384),
        GEO_DISPLAY_LIST(LAYER_OPAQUE, bob_seg4_dl_040040),
    GEO_CLOSE_NODE(),
    GEO_END(),
};
```

---

## Audio System

### Audio Components
**Location:** `src/audio/`

- **Sequences:** Music patterns
- **Samples:** Sound effects
- **Sound Banks:** Grouped sounds
- **Audio Driver:** N64 audio processing

### Audio Processing
1. **Sequence Player:** Plays music sequences
2. **Sample Player:** Plays sound samples
3. **Reverb:** Environmental effects
4. **Mixing:** Combine audio channels
5. **Output:** Send to audio hardware

### Sound Categories
- **Music:** Background music
- **SFX:** Sound effects
- **Environmental:** Ambient sounds
- **Voice:** Character voices

---

## Behavior Scripts

### Script Execution
**Location:** `src/engine/behavior_script.c`

**Execution Process:**
1. **Fetch Command:** Get next behavior command
2. **Decode:** Parse command and arguments
3. **Execute:** Call corresponding function
4. **Update:** Modify object state
5. **Loop:** Continue to next command

### Command Types
```c
// Control flow
BHv_CMD_BEGIN()
BHv_CMD_CALL(addr)
BHv_CMD_RETURN()
BHv_CMD_LOOP()
BHv_CMD_BREAK()
BHv_CMD_DELAY(frames)

// Object manipulation
BHv_CMD_SET_OBJ_FLAG(flag)
BHv_CMD_SET_OBJ_VAR(index, value)
BHv_CMD_ADD_OBJ_VAR(index, value)

// Interaction
BHv_CMD_SET_INTERACT_TYPE(type)
BHv_CMD_SET_HITBOX(radius, height)
```

### Behavior Examples
```c
// Goomba behavior
const BehaviorScript bhv_goomba[] = {
    BEGIN(bhv_goomba),
    LOOP(),
        CALL(0x801CD5A0), // goomba_update
        DELAY(1),
    END(),
};

// Coin behavior
const BehaviorScript bhv_coin[] = {
    BEGIN(bhv_coin),
    SET_HITBOX(100, 100),
    SET_INTERACT_TYPE(INTERACT_COIN),
    CALL(0x801CD6A0), // coin_update
    DELAY(1),
};
```

---

## Interaction System

### Interaction Types
**Location:** `src/game/interaction.c`

**Main Categories:**
- `INTERACT_COIN` - Collectible coins
- `INTERACT_STAR_OR_KEY` - Stars and keys
- `INTERACT_DAMAGE` - Damage sources
- `INTERACT_BREAKABLE` - Breakable objects
- `INTERACT_GRABBABLE` - Grabbable objects
- `INTERACT_TEXT` - Text interactions

### Interaction Process
1. **Collision Detection:** Check object overlaps
2. **Type Matching:** Compare interaction types
3. **Response:** Execute interaction handler
4. **State Update:** Modify object states
5. **Effects:** Apply visual/audio effects

### Interaction Handlers
```c
s32 interact_coin(struct MarioState *m, u32 interactType, struct Object *o) {
    m->numCoins++;
    o->activeFlags = ACTIVE_FLAG_DEACTIVATED;
    play_sound(SOUND_GENERAL_COIN, o->header.gfx.cameraToObject);
    return TRUE;
}

s32 interact_damage(struct MarioState *m, u32 interactType, struct Object *o) {
    m->hurtCounter += 10;
    m->flags |= MARIO_METAL_SHOCK;
    return TRUE;
}
```

---

## Enhancements

### Available Patches
**Location:** `enhancements/`

**Included Enhancements:**
- **Crash Screen** (`crash.patch`) - Hardware exception handler
- **Debug Box** (`debug_box.patch`) - 3D debug boxes
- **FPS Counter** (`fps.patch`) - Frame rate display
- **iQue Support** (`ique_support.patch`) - Cross-platform compatibility
- **Memory Error Screen** (`mem_error_screen.patch`) - Expansion Pak requirement
- **Demo Recorder** (`record_demo.patch`) - Input recording system

### Patch Application
```bash
# Apply patch
tools/apply_patch.sh crash.patch

# Remove patch
tools/revert_patch.sh crash.patch

# Create custom patch
tools/create_patch.sh my_enhancement.patch
```

---

## Key Files Reference

### Core Game Logic
- `src/game/main.c` - Main thread and task scheduling
- `src/game/mario.c` - Mario logic, animations, size control
- `src/game/camera.c` - Camera system
- `src/game/interaction.c` - Object interactions
- `src/game/level_update.c` - Level transitions

### Engine Systems
- `src/engine/behavior_script.c` - Behavior script interpreter
- `src/engine/graph_node.c` - Scene graph management
- `src/engine/geo_layout.c` - Model structure definitions
- `src/engine/surface_collision.c` - Collision detection
- `src/engine/math_util.c` - Mathematical utilities

### Data Definitions
- `include/types.h` - Core data structures
- `include/sm64.h` - Game constants and macros
- `include/object_fields.h` - Object field definitions
- `include/surface_terrains.h` - Surface type definitions

### Build Tools
- `Makefile` - Main build configuration
- `tools/` - Asset conversion and build utilities
- `extract_assets.py` - Asset extraction from ROM

---

## Development Notes

### Matching Builds
- Use IDO compiler for exact ROM reproduction
- Asset extraction required from base ROM
- Hash verification ensures accuracy

### Non-Matching Builds
- Use GCC for easier development
- No hash verification required
- More portable and maintainable

### Debug Features
- `gShowDebugText` - Debug text display
- `gShowProfiler` - Performance profiler
- Various debug commands and flags

### Memory Management
- Fixed memory pools for objects
- Dynamic allocation for temporary data
- Careful memory alignment for N64

---

## Common Patterns

### Object Spawning
```c
struct Object *obj = spawn_object(parent, MODEL_ID, bhv_script);
obj->oPosX = x;
obj->oPosY = y;
obj->oPosZ = z;
obj->oForwardVel = speed;
```

### Behavior Script Pattern
```c
const BehaviorScript bhv_example[] = {
    BEGIN(bhv_example),
    LOOP(),
        CALL(update_function),
        DELAY(1),
    END(),
};
```

### Level Script Pattern
```c
const LevelScript level_example[] = {
    INIT_LEVEL(),
    LOAD_MIO0(0x07, segment_start, segment_end),
    AREA(1, geo),
        OBJECT(MODEL_ID, x, y, z, 0, 0, 0, 0, bhv_script),
        RETURN(),
    END_AREA(),
    END(),
};
```

---

## Performance Considerations

### Frame Rate
- Target: 30 FPS (NTSC) or 25 FPS (PAL)
- VBlank synchronization
- Task scheduling for audio/graphics

### Memory Usage
- 4 MB RAM standard
- 8 MB with Expansion Pak
- Careful memory pool management

### Optimization
- Fixed-point arithmetic for performance
- Display list caching
- Efficient collision detection

---

## Conclusion

This documentation covers the core systems and patterns of the Super Mario 64 codebase. The decompilation provides a complete understanding of how the game works, from low-level hardware interaction to high-level game logic.

The codebase is well-organized with clear separation between game logic, engine systems, and data. The decompilation preserves the original structure while making it readable and modifiable.

For further exploration, examine the specific files and systems mentioned in this documentation, and refer to the extensive comments throughout the codebase for detailed explanations of complex algorithms and systems.