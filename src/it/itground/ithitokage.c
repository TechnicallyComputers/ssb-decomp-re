#include <it/item.h>
#include <wp/weapon.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <reloc_data.h>
#ifdef PORT
extern void *func_800269C0_275C0(u16 id);
#endif
#if defined(PORT) && defined(SSB64_NETMENU)
#include <sys/netrollbacksnapshot.h>
#include <sys/netrollback.h>
static void itHitokageFlameNoteParticlesEmitted(WPStruct *wp);
#else
extern void *func_800269C0_275C0(u16 id);

#endif

extern s32 dGRYamabukiMonsterAttackKind;

// // // // // // // // // // // //
//                               //
//       INITIALIZED DATA        //
//                               //
// // // // // // // // // // // //

ITDesc dITHitokageItemDesc =
{
    nITKindHitokage,                        // Item Kind
    &gGRCommonStruct.yamabuki.item_head,    // Pointer to item file data?
#ifdef PORT
    llGRYamabukiMapHitokageItemAttributes, // Offset of item attributes in file?
#else
    &llGRYamabukiMapHitokageItemAttributes, // Offset of item attributes in file?
#endif

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyR,            // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0                                   // ???
    },

    nGMAttackStateNew,                      // Hitbox Update State
    itHitokageCommonProcUpdate,             // Proc Update
    NULL,                                   // Proc Map
    NULL,                                   // Proc Hit
    NULL,                                   // Proc Shield
    NULL,                                   // Proc Hop
    NULL,                                   // Proc Set-Off
    NULL,                                   // Proc Reflector
    itHitokageCommonProcDamage              // Proc Damage
};

ITStatusDesc dITHitokageStatusDescs[/* */] =
{
    // Status 0 (Neutral Damage)
    {
        itHitokageDamagedProcUpdate,        // Proc Update
        NULL,                               // Proc Map
        NULL,                               // Proc Hit
        NULL,                               // Proc Shield
        NULL,                               // Proc Hop
        NULL,                               // Proc Set-Off
        NULL,                               // Proc Reflector
        NULL                                // Proc Damage
    }
};

WPDesc dITHitokageWeaponFlameWeaponDesc =
{
    0x00,                                   // Render flags?
    nWPKindHitokageFlame,                   // Weapon Kind
    &gGRCommonStruct.yamabuki.item_head,    // Pointer to character's loaded files?
#ifdef PORT
    llGRYamabukiMapHitokageFlameWeaponAttributes,// Offset of weapon attributes in loaded files
#else
    &llGRYamabukiMapHitokageFlameWeaponAttributes,// Offset of weapon attributes in loaded files
#endif

    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyRSca,         // Main matrix transformations
        nGCMatrixKindNull,                  // Secondary matrix transformations?
        0                                   // ???
    },

    itHitokageWeaponFlameProcUpdate,        // Proc Update
    itHitokageWeaponFlameProcMap,           // Proc Map
    itHitokageWeaponFlameProcHit,           // Proc Hit
    itHitokageWeaponFlameProcHit,           // Proc Shield
    NULL,                                   // Proc Hop
    itHitokageWeaponFlameProcHit,           // Proc Set-Off
    itHitokageWeaponFlameProcReflector,     // Proc Reflector
    NULL                                    // Proc Absorb
};

// // // // // // // // // // // //
//                               //
//          ENUMERATORS          //
//                               //
// // // // // // // // // // // //

enum itHitokageStatus
{
    itHitokageStatusDamaged,
    itHitokageStatusEnumCount
};

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

// 0x80183DA0
void itHitokageDamagedSetStatus(GObj *item_gobj)
{
    itMainSetStatus(item_gobj, dITHitokageStatusDescs, itHitokageStatusDamaged);

    itGetStruct(item_gobj)->proc_dead = itHitokageDamagedProcDead;
}

// 0x80183DE0
sb32 itHitokageCommonProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    Vec3f pos;

    dobj->translate.vec.f.x += ip->item_vars.hitokage.offset.x;
    dobj->translate.vec.f.y += ip->item_vars.hitokage.offset.y;

    pos = dobj->translate.vec.f;

    pos.x += ITHITOKAGE_FLAME_SPAWN_OFF_X;

    if 
    (
        (ip->item_vars.hitokage.flags == GRYAMABUKI_MONSTER_WEAPON_INSTANT)                                                  ||
        ((ip->item_vars.hitokage.flags & GRYAMABUKI_MONSTER_WEAPON_WAIT) && (dobj->anim_frame >= ITHITOKAGE_FLAME_SPAWN_BEGIN)) &&
        (dobj->anim_frame <= ITHITOKAGE_FLAME_SPAWN_END)
    )
    {
        dobj->mobj->texture_id_curr = 1;

        if (ip->item_vars.hitokage.flame_spawn_wait <= 0)
        {
            itHitokageCommonMakeFlame(item_gobj, &pos);

            ip->item_vars.hitokage.flame_spawn_wait = ITHITOKAGE_FLAME_SPAWN_WAIT;
        }
        else ip->item_vars.hitokage.flame_spawn_wait--; 
    }
    else dobj->mobj->texture_id_curr = 0;

    if (dobj->anim_wait == AOBJ_ANIM_NULL)
    {
        grYamabukiGateSetClosedWait();

        return TRUE;
    }
    return FALSE;
}

// 0x80183F20
sb32 itHitokageDamagedProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj;

    itMainApplyGravityClampTVel(ip, ITHITOKAGE_GRAVITY, ITHITOKAGE_TVEL);

    dobj = DObjGetStruct(item_gobj);

    dobj->rotate.vec.f.z -= (ITHITOKAGE_HIT_ROTATE_Z * ip->lr);

    return FALSE;
}

// 0x80183F88
sb32 itHitokageDamagedProcDead(GObj *item_gobj)
{
    return TRUE;
}

// 0x80183F94
sb32 itHitokageCommonProcDamage(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    if (ip->damage_knockback >= ITHITOKAGE_NDAMAGE_KNOCKBACK_MIN)
    {
        f32 angle = ftCommonDamageGetKnockbackAngle(ip->damage_angle, ip->ga, ip->damage_knockback);

        ip->physics.vel_air.x = __cosf(angle) * ip->damage_knockback * -ip->damage_lr;
        ip->physics.vel_air.y = __sinf(angle) * ip->damage_knockback;

        ip->attack_coll.attack_state = nGMAttackStateOff;
        ip->damage_coll.hitstatus = nGMHitStatusNone;

        dobj->anim_wait = AOBJ_ANIM_NULL;

        grYamabukiGateClearMonsterGObj();
        itHitokageDamagedSetStatus(item_gobj);
    }
    return FALSE;
}

// 0x80184058
GObj* itHitokageMakeItem(GObj *parent_gobj, Vec3f *pos, Vec3f *vel, u32 flags)
{
    GObj *item_gobj = itManagerMakeItem(parent_gobj, &dITHitokageItemDesc, pos, vel, flags);
    s32 unused;
    DObj *dobj;
    ITStruct *ip;

    if (item_gobj != NULL)
    {
        ip = itGetStruct(item_gobj);
        dobj = DObjGetStruct(item_gobj);

        ip->item_vars.hitokage.flame_spawn_wait = 0;
        ip->item_vars.hitokage.offset = *pos;

        ip->is_allow_knockback = TRUE;

        ip->item_vars.hitokage.flags = syUtilsRandIntRange(GRYAMABUKI_MONSTER_WEAPON_MAX);

        if ((dGRYamabukiMonsterAttackKind == ip->item_vars.hitokage.flags) || (ip->item_vars.hitokage.flags & dGRYamabukiMonsterAttackKind))
        {
            ip->item_vars.hitokage.flags++;

            ip->item_vars.hitokage.flags %= GRYAMABUKI_MONSTER_WEAPON_MAX;
        }
        if (ip->item_vars.hitokage.flags == GRYAMABUKI_MONSTER_WEAPON_INSTANT)
        {
            dobj->mobj->texture_id_curr = 1;
        }
        dGRYamabukiMonsterAttackKind = ip->item_vars.hitokage.flags;

        func_800269C0_275C0(nSYAudioVoiceYamabukiHitokage);
    }
    return item_gobj;
}

// 0x8018415C
sb32 itHitokageWeaponFlameProcUpdate(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

#if defined(PORT) && defined(SSB64_NETMENU)
    itHitokageFlameWeaponSyncPresentation(weapon_gobj);
#endif
    if (wpMainDecLifeCheckExpire(wp) != FALSE)
    {
        return TRUE;
    }
    else return FALSE;
}

// 0x80184188
sb32 itHitokageWeaponFlameProcMap(GObj *weapon_gobj)
{
    if (wpMapTestAllCheckCollEnd(weapon_gobj) != FALSE)
    {
        efManagerDustExpandSmallMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f, 1.0F);

        return TRUE;
    }
    else return FALSE;
}

// 0x801841CC
sb32 itHitokageWeaponFlameProcHit(GObj *weapon_gobj)
{
    func_800269C0_275C0(nSYAudioFGMExplodeS);
    efManagerSparkleWhiteMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f);

    return FALSE;
}

// 0x80184204
sb32 itHitokageWeaponFlameProcReflector(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    FTStruct *fp = ftGetStruct(wp->owner_gobj);
    Vec3f *translate;

    wp->lifetime = ITHITOKAGE_FLAME_LIFETIME;

    wpMainReflectorSetLR(wp, fp);

    translate = &DObjGetStruct(weapon_gobj)->translate.vec.f;

    lbParticleMakePosVel(gITManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 2, translate->x, translate->y, 0.0F, wp->physics.vel_air.x, wp->physics.vel_air.y, 0.0F);
    lbParticleMakePosVel(gITManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0, translate->x, translate->y, 0.0F, wp->physics.vel_air.x, wp->physics.vel_air.y, 0.0F);

#if defined(PORT) && defined(SSB64_NETMENU)
    itHitokageFlameNoteParticlesEmitted(wp);
#endif
    return FALSE;
}

// 0x801842C8
GObj* itHitokageWeaponFlameMakeWeapon(GObj *item_gobj, Vec3f *pos, Vec3f *vel)
{
    GObj *weapon_gobj = wpManagerMakeWeapon(item_gobj, &dITHitokageWeaponFlameWeaponDesc, pos, WEAPON_FLAG_PARENT_ITEM);
    WPStruct *wp;

    if (weapon_gobj == NULL)
    {
        return NULL;
    }
    wp = wpGetStruct(weapon_gobj);

    wp->physics.vel_air = *vel;

    wp->lifetime = ITHITOKAGE_FLAME_LIFETIME;

    wp->lr = -1;

    lbParticleMakePosVel(gITManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 2, pos->x, pos->y, 0.0F, wp->physics.vel_air.x, wp->physics.vel_air.y, 0.0F);
    lbParticleMakePosVel(gITManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0, pos->x, pos->y, 0.0F, wp->physics.vel_air.x, wp->physics.vel_air.y, 0.0F);

#if defined(PORT) && defined(SSB64_NETMENU)
    itHitokageFlameNoteParticlesEmitted(wp);
#endif
    return weapon_gobj;
}

// 0x801843C4
void itHitokageCommonMakeFlame(GObj *item_gobj, Vec3f *pos)
{
    ITStruct *ip;
    Vec3f vel;

    vel.x = __cosf(ITHITOKAGE_FLAME_SPAWN_ANGLE) * -ITHITOKAGE_FLAME_VEL_BASE;
    vel.y = __sinf(ITHITOKAGE_FLAME_SPAWN_ANGLE) * ITHITOKAGE_FLAME_VEL_BASE;
    vel.z = 0.0F;

    itHitokageWeaponFlameMakeWeapon(item_gobj, pos, &vel);

    func_800269C0_275C0(nSYAudioFGMLizardonFlame);
}

#if defined(PORT) && defined(SSB64_NETMENU)
/*
 * PORT rollback support: re-emit a tower-monster flame's visible particles at its current pose.
 *
 * The flame weapon (nWPKindHitokageFlame; nWPKindLizardonFlame uses the byte-identical spawn) is
 * collision-only — its WPDesc render flags are 0x00 and the entire visible fire is lbParticles created
 * ONCE at weapon spawn (itHitokageWeaponFlameMakeWeapon). Every rollback load wipes all particles
 * (syNetRbSnapResetParticlesForRollback -> lbParticleEjectStructAll/GeneratorAll) and nothing re-emits
 * them, so after the first rollback the flame keeps hitting fighters (the weapon GObj survives) while
 * rendering nothing. The snapshot repair calls this per live flame weapon to restore the visual.
 * Determinism-safe: lbParticle uses the cosmetic RNG (lbparticle.c) and particles are not in any rollback
 * hash, so this is pure presentation.
 */
#define ITHITOKAGE_FLAME_EMIT_TRACK_MAX 64

typedef struct ITHitokageFlameEmitTrack
{
    u32 instance_id;
    u32 particle_emit_gen;
} ITHitokageFlameEmitTrack;

static ITHitokageFlameEmitTrack sITHitokageFlameEmitTrack[ITHITOKAGE_FLAME_EMIT_TRACK_MAX];

static u32 *itHitokageFlameLookupEmitGen(u32 instance_id)
{
    u32 i;
    u32 free_slot;

    if (instance_id == 0U)
    {
        return NULL;
    }
    free_slot = ITHITOKAGE_FLAME_EMIT_TRACK_MAX;
    for (i = 0; i < ITHITOKAGE_FLAME_EMIT_TRACK_MAX; i++)
    {
        if (sITHitokageFlameEmitTrack[i].instance_id == instance_id)
        {
            return &sITHitokageFlameEmitTrack[i].particle_emit_gen;
        }
        if ((free_slot == ITHITOKAGE_FLAME_EMIT_TRACK_MAX) &&
            (sITHitokageFlameEmitTrack[i].instance_id == 0U))
        {
            free_slot = i;
        }
    }
    if (free_slot >= ITHITOKAGE_FLAME_EMIT_TRACK_MAX)
    {
        return NULL;
    }
    sITHitokageFlameEmitTrack[free_slot].instance_id = instance_id;
    sITHitokageFlameEmitTrack[free_slot].particle_emit_gen = 0U;
    return &sITHitokageFlameEmitTrack[free_slot].particle_emit_gen;
}

static void itHitokageFlameNoteParticlesEmitted(WPStruct *wp)
{
    u32 *emit_gen;

    if (wp == NULL)
    {
        return;
    }
    emit_gen = itHitokageFlameLookupEmitGen(wp->instance_id);
    if (emit_gen != NULL)
    {
        *emit_gen = syNetRbSnapGetParticleResetGeneration();
    }
}

void itHitokageReemitFlameParticles(GObj *weapon_gobj)
{
    WPStruct *wp;
    DObj *dobj;
    Vec3f *translate;

    if (weapon_gobj == NULL)
    {
        return;
    }
    wp = wpGetStruct(weapon_gobj);
    dobj = DObjGetStruct(weapon_gobj);
    if ((wp == NULL) || (dobj == NULL))
    {
        return;
    }
    translate = &dobj->translate.vec.f;
    lbParticleMakePosVel(gITManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 2, translate->x, translate->y, 0.0F, wp->physics.vel_air.x, wp->physics.vel_air.y, 0.0F);
    lbParticleMakePosVel(gITManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0, translate->x, translate->y, 0.0F, wp->physics.vel_air.x, wp->physics.vel_air.y, 0.0F);
    itHitokageFlameNoteParticlesEmitted(wp);
}

void itHitokageFlameWeaponSyncPresentation(GObj *weapon_gobj)
{
    WPStruct *wp;
    u32 current_gen;
    u32 *last_emit_gen;
    s32 flame_age;

    if (weapon_gobj == NULL)
    {
        return;
    }
    wp = wpGetStruct(weapon_gobj);
    if (wp == NULL)
    {
        return;
    }
    current_gen = syNetRbSnapGetParticleResetGeneration();
    last_emit_gen = itHitokageFlameLookupEmitGen(wp->instance_id);
    if ((last_emit_gen != NULL) && (current_gen != *last_emit_gen))
    {
        itHitokageReemitFlameParticles(weapon_gobj);
        return;
    }
    /*
     * Load-time EnsureMonsterFlame re-emits once, then resim can run many ticks before the next
     * rendered frame — particles expire (~20 ticks) while the weapon hitbox keeps simming. Refresh
     * on the spawn cadence during resim only (forward play still gets one burst per weapon spawn).
     */
    if (syNetRollbackIsResimulating() != FALSE)
    {
        flame_age = ITHITOKAGE_FLAME_LIFETIME - wp->lifetime;
        if ((flame_age > 0) && ((flame_age % ITHITOKAGE_FLAME_SPAWN_WAIT) == 0))
        {
            itHitokageReemitFlameParticles(weapon_gobj);
        }
    }
}

#endif
