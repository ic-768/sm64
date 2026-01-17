# Mario Growth Enhancement

## Changes Made

### ✅ **Faster Growth Rate**
- **Before:** Mario grows every 60 seconds 
- **After:** Mario grows every 5 seconds
- **Code:** `GROWTH_INTERVAL_FRAMES 300` (300 frames = 5 seconds at 60fps)

### ✅ **Persistent Growth**
- **Before:** Growth resets on warps, cutscenes, level changes
- **After:** Growth persists throughout entire game session
- **Logic:** Only resets on fresh game start (`ACT_UNINITIALIZED`)
- **Safety:** Won't run during save file loading to prevent crashes

### ✅ **Visual Feedback**
- **Added:** Sparkle particles when Mario grows
- **Added:** Coin sound effect for growth notification
- **Code:** `PARTICLE_SPARKLES` + `SOUND_GENERAL_COIN`

### ✅ **Crash Fix**
- **Problem:** Game crashed when selecting save files
- **Solution:** Added safety checks for uninitialized state
- **Code:** `if (m->marioObj == NULL || m->action == ACT_UNINITIALIZED) return;`

## Gameplay Experience

### Start of Game
- Mario begins at **25% scale** (tiny)
- Growth timer starts at 0
- First growth after 5 seconds

### During Gameplay
- Every 5 seconds: +0.25 scale on all axes
- Growth progression: 0.25 → 0.5 → 0.75 → 1.0 → 1.25 → 1.5 → 1.75 → 2.0...
- **Maximum size:** 8.0 (32× normal size) after ~3.3 minutes
- **Visual feedback:** Sparkles + coin sound each growth
- **Persistence:** Growth continues through warps, deaths, cutscenes

### Technical Details
- **Physical collision:** Unaffected (uses `pos` coordinates)
- **Visual scaling:** Affects 3D model rendering only
- **Safety checks:** Prevents crashes during initialization
- **Build status:** ✅ Compiles successfully

## Files Modified
- `src/game/mario.c` - Growth logic and initialization
- `SM64_CODEBASE_DOCUMENTATION.md` - Updated documentation

## Build Commands
```bash
make clean
make VERSION=us NON_MATCHING=1
```

The enhancement is complete and ready for testing!