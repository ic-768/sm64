
/**
 * Behavior for bhvGoomba and bhvGoombaTripletSpawner,
 * Goombas can either be spawned individually, or spawned by a triplet spawner.
 * The triplet spawner comes before its spawned goombas in processing order.
 */

extern struct MarioState *gMarioState; // Access to Mario's state for jump sync

// Per-goomba jump detection to prevent shared state conflicts
static u32 sMarioPrevAction = 0;

// Forward declaration for function defined later
static void goomba_begin_jump(void);

/**
 * Check if Mario just started jumping and trigger synchronized goomba jump
 */
static void check_and_sync_mario_jump(void) {
    u32 currentAction;
    
    if (gMarioState == NULL) return;
    
    currentAction = gMarioState->action;
    
    // If Mario is airborne and this goomba is close enough, make it jump
    // with matching direction and speed
    if ((currentAction & ACT_FLAG_AIR) && o->oDistanceToMario < 3000.0f && o->oAction == GOOMBA_ACT_WALK) {
// Give goomba a moderate boost to sync with Mario's height
        o->oVelY = 25.0f * o->oGoombaScale; // Moderate jump height
        
        // Match Mario's jump direction and speed with better responsiveness
        if (gMarioState->forwardVel > 0.1f) {
            // Mario is moving forward - goomba jumps forward with matching speed
            o->oForwardVel = gMarioState->forwardVel * 1.2f; // 120% of Mario's speed for catch-up
        } else if (gMarioState->forwardVel < -0.1f) {
            // Mario is moving backward - goomba jumps backward with matching speed
            o->oForwardVel = gMarioState->forwardVel * 1.2f; // 120% speed for better tracking
        } else {
            // Mario is stationary - goomba jumps in place
            o->oForwardVel = 0.0f;
        }
        
        o->oAction = GOOMBA_ACT_JUMP;
        // Face-> same direction as Mario
        o->oGoombaTargetYaw = gMarioState->faceAngle[1];
    }
    
    // Active collision avoidance - if Mario is walking toward this goomba, move aside
    if (o->oAction == GOOMBA_ACT_WALK && !(currentAction & ACT_FLAG_AIR) && o->oDistanceToMario < 200.0f) {
        // Check if Mario is moving toward this goomba
        f32 marioToGoombaAngle = atan2s(o->oPosZ - gMarioState->pos[2], o->oPosX - gMarioState->pos[0]);
        f32 angleDiff = abs_angle_diff(marioToGoombaAngle, gMarioState->faceAngle[1]);
        
        // If Mario is facing toward this goomba and is close, move perpendicularly away
        if (angleDiff < 0x2000) { // Within 45 degrees
            // Calculate perpendicular escape direction
            f32 escapeAngle = marioToGoombaAngle + 0x4000; // 90 degrees
            
            // Move away at moderate speed
            o->oForwardVel = 20.0f * o->oGoombaScale;
            o->oGoombaTargetYaw = escapeAngle;
        }
    }
}

/**
 * Hitbox for goomba.
 */
static struct ObjectHitbox sGoombaHitbox = {
    /* interactType:      */ INTERACT_POLE, // No collision with Mario
    /* downOffset:        */ 0,
    /* damageOrCoinValue: */ 0, // No damage
    /* health:            */ 0,
    /* numLootCoins:      */ 1,
    /* radius:            */ 30, // Small radius for goomba-to-goomba collision
    /* height:            */ 30,
    /* hurtboxRadius:     */ 30, // Small hurtbox for goomba collision
    /* hurtboxHeight:     */ 30, // Small hurtbox for goomba collision
};

/**
 * Properties that vary based on goomba size.
 */
struct GoombaProperties {
    f32 scale;
    u32 deathSound;
    s16 drawDistance;
    s8 damage;
};

/**
 * Properties for regular, huge, and tiny goombas.
 */
static struct GoombaProperties sGoombaProperties[] = {
    { 1.5f, SOUND_OBJ_ENEMY_DEATH_HIGH, 4000, 1 },
    { 3.5f, SOUND_OBJ_ENEMY_DEATH_LOW, 4000, 2 },
    { 0.5f, SOUND_OBJ_ENEMY_DEATH_HIGH, 1500, 0 },
};

/**
 * Attack handlers for goombas.
 */
static u8 sGoombaAttackHandlers[][6] = {
    // regular and tiny
    {
        /* ATTACK_PUNCH:                 */ ATTACK_HANDLER_KNOCKBACK,
        /* ATTACK_KICK_OR_TRIP:          */ ATTACK_HANDLER_KNOCKBACK,
        /* ATTACK_FROM_ABOVE:            */ ATTACK_HANDLER_SQUISHED,
        /* ATTACK_GROUND_POUND_OR_TWIRL: */ ATTACK_HANDLER_SQUISHED,
        /* ATTACK_FAST_ATTACK:           */ ATTACK_HANDLER_KNOCKBACK,
        /* ATTACK_FROM_BELOW:            */ ATTACK_HANDLER_KNOCKBACK,
    },
    // huge
    {
        /* ATTACK_PUNCH:                 */ ATTACK_HANDLER_SPECIAL_HUGE_GOOMBA_WEAKLY_ATTACKED,
        /* ATTACK_KICK_OR_TRIP:          */ ATTACK_HANDLER_SPECIAL_HUGE_GOOMBA_WEAKLY_ATTACKED,
        /* ATTACK_FROM_ABOVE:            */ ATTACK_HANDLER_SQUISHED,
        /* ATTACK_GROUND_POUND_OR_TWIRL: */ ATTACK_HANDLER_SQUISHED_WITH_BLUE_COIN,
        /* ATTACK_FAST_ATTACK:           */ ATTACK_HANDLER_SPECIAL_HUGE_GOOMBA_WEAKLY_ATTACKED,
        /* ATTACK_FROM_BELOW:            */ ATTACK_HANDLER_SPECIAL_HUGE_GOOMBA_WEAKLY_ATTACKED,
    },
};

/**
 * Update function for goomba triplet spawner.
 */
void bhv_goomba_triplet_spawner_update(void) {
    UNUSED u8 filler1[4];
    s16 goombaFlag;
    UNUSED u8 filler2[2];
    s32 angle;

    // If mario is close enough and the goombas aren't currently loaded, then
    // spawn them
    if (o->oAction == GOOMBA_TRIPLET_SPAWNER_ACT_UNLOADED) {
        if (o->oDistanceToMario < 3000.0f) {
            // The spawner is capable of spawning more than 3 goombas, but this
            // is not used in the game
            s32 dAngle =
                0x10000
                / (((o->oBhvParams2ndByte & GOOMBA_TRIPLET_SPAWNER_BP_EXTRA_GOOMBAS_MASK) >> 2) + 3);

            for (angle = 0, goombaFlag = 1 << 8; angle < 0xFFFF; angle += dAngle, goombaFlag <<= 1) {
                // Only spawn goombas which haven't been killed yet
                if (!(o->oBhvParams & goombaFlag)) {
                    s16 dx = 500.0f * coss(angle);
                    s16 dz = 500.0f * sins(angle);

                    spawn_object_relative((o->oBhvParams2ndByte & GOOMBA_BP_SIZE_MASK)
                                           | (goombaFlag >> 6), dx, 0, dz, o, MODEL_GOOMBA, bhvGoomba);
                }
            }

            o->oAction++;
        }
    } else if (o->oDistanceToMario > 4000.0f) {
        // If mario is too far away, enter the unloaded action. The goombas
        // will detect this and unload themselves
        o->oAction = GOOMBA_TRIPLET_SPAWNER_ACT_UNLOADED;
    }
}

/**
 * Initialization function for goomba.
 */
void bhv_goomba_init(void) {
    o->oGoombaSize = o->oBhvParams2ndByte & GOOMBA_BP_SIZE_MASK;

    o->oGoombaScale = sGoombaProperties[o->oGoombaSize].scale;
    o->oDeathSound = sGoombaProperties[o->oGoombaSize].deathSound;

    obj_set_hitbox(o, &sGoombaHitbox);

    o->oDrawingDistance = sGoombaProperties[o->oGoombaSize].drawDistance;
    o->oDamageOrCoinValue = sGoombaProperties[o->oGoombaSize].damage;

    o->oGravity = -8.0f / 3.0f * o->oGoombaScale;
}

/**
 * Enter the jump action and set initial y velocity.
 */
static void goomba_begin_jump(void) {
    cur_obj_play_sound_2(SOUND_OBJ_GOOMBA_ALERT);

    o->oAction = GOOMBA_ACT_JUMP;
    o->oForwardVel = 0.0f;
    o->oVelY = 50.0f / 3.0f * o->oGoombaScale;
}

/**
 * If spawned by a triplet spawner, mark the flag in the spawner to indicate that
 * this goomba died. This prevents it from spawning again when mario leaves and
 * comes back.
 */
static void mark_goomba_as_dead(void) {
    if (o->parentObj != o) {
        set_object_respawn_info_bits(
            o->parentObj, (o->oBhvParams2ndByte & GOOMBA_BP_TRIPLET_RESPAWN_FLAG_MASK) >> 2);

        o->parentObj->oBhvParams =
            o->parentObj->oBhvParams | (o->oBhvParams2ndByte & GOOMBA_BP_TRIPLET_RESPAWN_FLAG_MASK) << 6;
    }
}

/**
 * Follow Mario around, stopping when close to him.
 */
static void goomba_act_walk(void) {
    f32 followDistance = 150.0f; // Distance at which goomba stops following
    f32 stopDistance = 200.0f;   // Distance at which goomba starts moving again
    
    o->oGoombaTargetYaw = o->oAngleToMario;

    // Check distance to Mario and adjust behavior
    if (o->oDistanceToMario < followDistance) {
        // Close to Mario - stop and wait
        o->oGoombaRelativeSpeed = 0.0f;
        o->oForwardVel = 0.0f;
        
        // Face Mario while waiting
        cur_obj_rotate_yaw_toward(o->oGoombaTargetYaw, 0x2000);
    } else if (o->oDistanceToMario < stopDistance) {
        // Medium distance - slow down for gentle approach
        f32 speedFactor = (o->oDistanceToMario - followDistance) / (stopDistance - followDistance);
        o->oGoombaRelativeSpeed = 40.0f * speedFactor; // Variable speed up to 40
        o->oForwardVel = o->oGoombaRelativeSpeed * o->oGoombaScale;
        
        // Still turn toward Mario
        cur_obj_rotate_yaw_toward(o->oGoombaTargetYaw, 0x2000);
        
        // Only play footstep sounds when actually moving
        if (o->oForwardVel > 1.0f) {
            cur_obj_play_sound_at_anim_range(2, 17, SOUND_OBJ_GOOMBA_WALK);
        }
    } else {
        // Far from Mario - chase at normal speed
        o->oGoombaRelativeSpeed = 40.0f; // Moderate follow speed
        o->oForwardVel = o->oGoombaRelativeSpeed * o->oGoombaScale;

        // Play footstep sounds when moving
        cur_obj_play_sound_at_anim_range(2, 17, SOUND_OBJ_GOOMBA_WALK);
    }

    // Handle collision and turning only when moving
    if (o->oForwardVel > 0.0f) {
        if (o->oGoombaTurningAwayFromWall) {
            o->oGoombaTurningAwayFromWall = obj_resolve_collisions_and_turn(o->oGoombaTargetYaw, 0x2000);
        } else {
            if (!(o->oGoombaTurningAwayFromWall =
                      obj_bounce_off_walls_edges_objects(&o->oGoombaTargetYaw))) {
                // Turn toward Mario with moderate precision when moving
                cur_obj_rotate_yaw_toward(o->oGoombaTargetYaw, 0x2000);
}
}
    }
}

/**
 * This action occurs when either the goomba attacks mario normally, or mario
 * attacks a huge goomba with an attack that doesn't kill it.
 */
static void goomba_act_attacked_mario(void) {
    if (o->oGoombaSize == GOOMBA_SIZE_TINY) {
        mark_goomba_as_dead();
        o->oNumLootCoins = 0;
        obj_die_if_health_non_positive();
    } else {
        // Don't jump when attacking - just set target yaw and continue chasing
        o->oGoombaTargetYaw = o->oAngleToMario;
        o->oGoombaTurningAwayFromWall = FALSE;
        // Immediately return to walk state to continue chasing
        o->oAction = GOOMBA_ACT_WALK;
    }
}

/**
 * Move until landing, and rotate toward target yaw.
 */
static void goomba_act_jump(void) {
    obj_resolve_object_collisions(NULL);

    //! If we move outside the goomba's drawing radius the frame it enters the
    //  jump action, then it will keep its velY, but it will still be counted
    //  as being on the ground.
    //  Next frame, the jump action will think it has already ended because it is
    //  still on the ground.
    //  This puts the goomba back in the walk action, but the positive velY will
    //  make it hop into the air. We can then trigger another jump.
    if (o->oMoveFlags & OBJ_MOVE_MASK_ON_GROUND) {
        o->oAction = GOOMBA_ACT_WALK;
    } else {
        cur_obj_rotate_yaw_toward(o->oGoombaTargetYaw, 0x800);
    }
}

/**
 * Attack handler for when mario attacks a huge goomba with an attack that
 * doesn't kill it.
 * From the goomba's perspective, this is the same as the goomba attacking
 * mario.
 */
void huge_goomba_weakly_attacked(void) {
    o->oAction = GOOMBA_ACT_ATTACKED_MARIO;
}

/**
 * Update function for goomba.
 */
void bhv_goomba_update(void) {
    // PARTIAL_UPDATE

    f32 animSpeed;

    if (obj_update_standard_actions(o->oGoombaScale)) {
        // If this goomba has a spawner and mario moved away from the spawner, unload
        if (o->parentObj != o) {
            if (o->parentObj->oAction == GOOMBA_TRIPLET_SPAWNER_ACT_UNLOADED) {
                obj_mark_for_deletion(o);
            }
        }

        cur_obj_scale(o->oGoombaScale);
        obj_update_blinking(&o->oGoombaBlinkTimer, 30, 50, 5);
        cur_obj_update_floor_and_walls();

        if ((animSpeed = o->oForwardVel / o->oGoombaScale * 0.4f) < 1.0f) {
            animSpeed = 1.0f;
        }

        cur_obj_init_animation_with_accel_and_sound(0, animSpeed);

        // Check for Mario jump synchronization
        check_and_sync_mario_jump();

        switch (o->oAction) {
            case GOOMBA_ACT_WALK:
                goomba_act_walk();
                break;
            case GOOMBA_ACT_ATTACKED_MARIO:
                goomba_act_attacked_mario();
                break;
            case GOOMBA_ACT_JUMP:
                goomba_act_jump();
                break;
        }

        //! @bug Weak attacks on huge goombas in a triplet mark them as dead even if they're not.
        // obj_handle_attacks returns the type of the attack, which is non-zero
        // even for Mario's weak attacks. Thus, if Mario weakly attacks a huge goomba
        // without harming it (e.g. by punching it), the goomba will be marked as dead
        // and will not respawn if Mario leaves and re-enters the spawner's radius
        // even though the goomba isn't actually dead.
        if (obj_handle_attacks(&sGoombaHitbox, GOOMBA_ACT_ATTACKED_MARIO,
                               sGoombaAttackHandlers[o->oGoombaSize & 1])) {
            mark_goomba_as_dead();
        }

        cur_obj_move_standard(-78);
    } else {
        o->oAnimState = 1;
    }
}
