# SM64 Development Guide for AI Agents

This document provides essential information for AI agents working with the Super Mario 64 decompilation project.

## Build Commands

### Primary Build Commands

```bash
# Build default version (US) with 4 parallel jobs
make -j4

# Build specific version
make VERSION=us -j4    # North America
make VERSION=jp -j4    # Japan
make VERSION=eu -j4    # Europe
make VERSION=sh -j4    # Shindou (Japan, rumble support)
make VERSION=cn -j4    # iQue Player (China)

# Build without ROM hash comparison (for modifications)
make COMPARE=0 -j4

# Build with non-matching C implementations (safer for mods)
make NON_MATCHING=1 -j4

# Clean build
make clean

# Format code
./format.sh                    # Format all files
./format.sh file.c file2.c      # Format specific files
```

### Build Options

- `VERSION`: jp/us/eu/sh/cn (default: us)
- `COMPARE`: 1/0 (default: 1) - Compare ROM hash with original
- `NON_MATCHING`: Use functionally equivalent C implementations
- `COMPILER`: ido/gcc (default: ido for matching builds)
- `GRUCODE`: f3d_old/f3d_new/f3dex/f3dex2/f3dzex

## Code Style Guidelines

### Formatting (clang-format)

- Indent width: 4 spaces
- Column limit: 104 characters
- Pointer alignment: Right (`int *ptr` not `int* ptr`)
- Brace style: Attach (`if (cond) {` on same line)
- Use tabs: Never (spaces only)
- Sort includes: false (preserve order)

### Naming Conventions

- **Functions**: `snake_case` (e.g., `goomba_act_walk`)
- **Variables**: `snake_case` (e.g., `oGoombaRelativeSpeed`)
- **Constants**: `UPPER_SNAKE_CASE` (e.g., `GOOMBA_ACT_WALK`)
- **Structs**: `PascalCase` (e.g., `ObjectHitbox`)
- **Macros**: `UPPER_SNAKE_CASE` (e.g., `ARRAY_COUNT`)
- **File names**: `snake_case.c` for source, `snake_case.h` for headers

### Type System

- **Fixed-width types**: Use N64 types (`s32`, `u32`, `f32`, etc.)
- **Boolean**: Use `TRUE`/`FALSE` or `1`/`0`
- **Common types**:
  - `Vec3f` - 3D float vector [x, y, z]
  - `Vec3s` - 3D short vector
  - `Mat4` - 4x4 float matrix
  - `s16` - 16-bit signed integer
  - `u32` - 32-bit unsigned integer
  - `f32` - 32-bit float

### Include Order

1. System headers (`#include <ultra64.h>`)
2. Project headers (alphabetical)
3. Local headers (quotes, alphabetical)

```c
#include <PR/ultratypes.h>
#include "sm64.h"
#include "area.h"
#include "audio/external.h"
#include "engine/math_util.h"
#include "game_init.h"
```

### Function Documentation

Use doxygen-style comments for functions and structures:

```c
/**
 * Walk around randomly occasionally jumping. If mario comes within range,
 * chase him.
 */
static void goomba_act_walk(void) {
    // Implementation
}

/**
 * Hitbox for goomba.
 */
static struct ObjectHitbox sGoombaHitbox = {
    /* interactType:      */ INTERACT_BOUNCE_TOP,
    /* downOffset:        */ 0,
    /* damageOrCoinValue: */ 1,
};
```

### Error Handling

- Use `ASSERT` for critical conditions
- Return appropriate error codes from functions
- Check pointer validity before dereferencing
- Use `STATIC_ASSERT` for compile-time checks

### Memory Management

- Use `alloc_*` functions for dynamic allocation
- Always pair with appropriate `free_*` functions
- Check return values for NULL
- Respect N64 memory constraints (4MB RAM)

### Game-Specific Patterns

#### Object Behaviors

- Behavior files use `.inc.c` extension
- Use `bhv_` prefix for behavior functions
- Object fields accessed via `o->oFieldName`
- Common pattern: switch on `o->oAction`

#### Level Scripts

- Level commands use specific macros
- Follow existing level structure in `levels/` directory
- Each level has `header.h`, `geo.c`, `leveldata.c`, etc.

#### Actors

- Actor behaviors in `src/game/behaviors/`
- Actor data in `actors/` directory
- Group headers for related actors

### Constants and Magic Numbers

- Define named constants for repeated values
- Use existing constants from header files
- Fractional values use `f32` notation (e.g., `4.0f / 3.0f`)

### Testing and Verification

- Build with `COMPARE=0` for modifications
- Test with multiple ROM versions if applicable
- Verify game logic changes don't break existing functionality
- NEVER use emulator testing via `make test`

### Common Pitfalls

- Don't exceed 104 character line limit
- Maintain N64 alignment requirements (8-byte for DMA)
- Avoid undefined behavior (use `NON_MATCHING=1` if necessary)
- Don't change ROM offsets in matching builds
- Preserve existing data structures and interfaces

### File Organization

```
src/
├── game/           # Game logic, behaviors, etc.
├── audio/          # Audio system
├── engine/         # Core engine systems
├── buffers/        # Memory management
├── goddard/        # Mario intro screen
└── menu/           # Menu systems

levels/             # Level-specific data
actors/             # Actor models and behaviors
include/            # Header files
enhancements/       # Example modifications
```

When making changes, always run `./format.sh` before committing to ensure code style compliance.

