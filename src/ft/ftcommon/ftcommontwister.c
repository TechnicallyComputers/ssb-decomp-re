#include <ft/fighter.h>
#include <it/item.h>
#include <gr/ground.h>
#include <sc/scene.h>
#include <reloc_data.h>
#include <sys/utils.h>
#if defined(PORT) && defined(SSB64_NETMENU)
#include <stdlib.h>
#include <string.h>
#include <sys/netplay_sim_quantize.h>
/*
 * Netplay rollback forward-sim: Hyrule twister rider rebind/quantize uses
 * syNetplayRollbackSemanticsActive() / syNetplaySimQuantizeActive().
 */
extern void port_log(const char *fmt, ...);
extern u32 syNetInputGetTick(void);
extern void *func_800269C0_275C0(u16 id);

static sb32 ftCommonTwisterDiagEnabled(void)
{
    const char *e = getenv("SSB64_NETPLAY_HYRULE_TWISTER_DIAG");

    return (e != NULL) && (e[0] != '\0') && (strcmp(e, "0") != 0);
}

static void ftCommonTwisterDiagShoot(GObj *fighter_gobj, const char *reason)
{
    FTStruct *fp;

    if (ftCommonTwisterDiagEnabled() == FALSE)
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    port_log(
        "SSB64 NetRbSnapshot: hyrule_twister_rider_shoot tick=%u player=%d reason=%s release_wait=%d tornado_gobj=%p\n",
        (unsigned int)syNetInputGetTick(), (int)((fp != NULL) ? fp->player : -1), (reason != NULL) ? reason : "?",
        (int)((fp != NULL) ? ftStatusVarsTwister(fp)->release_wait : -1),
        (void *)((fp != NULL) ? ftStatusVarsTwister(fp)->tornado_gobj : NULL));
}
#else
extern void *func_800269C0_275C0(u16 id);

#endif

#if defined(PORT) && defined(SSB64_NETMENU)
static void ftCommonTwisterQuantizeRiderJoints(FTStruct *fp)
{
    s32 ji;

    if (fp == NULL)
    {
        return;
    }
    for (ji = 0; ji < FTPARTS_JOINT_NUM_MAX; ji++)
    {
        if (fp->joints[ji] != NULL)
        {
            syNetplayQuantizeVec3f(&fp->joints[ji]->translate.vec.f);
        }
    }
}

static void ftCommonTwisterApplyNetplayRiderCanonical(GObj *fighter_gobj, FTStruct *fp, Vec3f *pos, Vec3f *vel,
                                                      f32 angle_d)
{
    DObj *fighter_dobj;
    f32 rot_y;

    if ((syNetplaySimQuantizeActive() == FALSE) || (fighter_gobj == NULL) || (fp == NULL) || (pos == NULL) ||
        (vel == NULL))
    {
        return;
    }
    syNetplayQuantizeVec3f(pos);
    syNetplayQuantizeVec3f(vel);
    fp->physics.vel_air = *vel;
    fighter_dobj = DObjGetStruct(fighter_gobj);
    if (fighter_dobj != NULL)
    {
        fighter_dobj->translate.vec.f = *pos;
        syNetplayQuantizeDObjTranslate(fighter_dobj);
        rot_y = (fp->lr * F_CLC_DTOR32(90.0F)) + F_CLC_DTOR32(1800.0F * angle_d);
        fighter_dobj->rotate.vec.f.y = syNetplayQuantizeF32(rot_y);
    }
    ftCommonTwisterQuantizeRiderJoints(fp);
}

void ftCommonTwisterReconcileRiderAfterRollback(GObj *fighter_gobj)
{
    FTStruct *fp;
    GObj *tornado_gobj;
    Vec3f pos;
    Vec3f vel;
    f32 mul;
    f32 angle_d;
    f32 mag;

    if (fighter_gobj == NULL)
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    if ((fp == NULL) || (fp->status_id != nFTCommonStatusTwister))
    {
        return;
    }
    tornado_gobj = ftStatusVarsTwister(fp)->tornado_gobj;
    if ((tornado_gobj == NULL) || (DObjGetStruct(tornado_gobj) == NULL))
    {
        tornado_gobj = grHyruleGetTwisterGobj();
        if (tornado_gobj != NULL)
        {
            ftStatusVarsTwister(fp)->tornado_gobj = tornado_gobj;
        }
    }
    if ((tornado_gobj == NULL) || (DObjGetStruct(tornado_gobj) == NULL) ||
        (DObjGetStruct(fighter_gobj) == NULL))
    {
        return;
    }
    pos = DObjGetStruct(tornado_gobj)->translate.vec.f;
    angle_d = (ftStatusVarsTwister(fp)->release_wait * 0.016666668F);
    mul = (((400.0F * angle_d) + 100.0F) * 0.5F);
    pos.x += (mul * lbCommonCos(F_CLC_DTOR32(1800.0F * angle_d)));
    pos.z += (mul * lbCommonSin(F_CLC_DTOR32(1800.0F * angle_d)));
    pos.y += 500.0F * angle_d;
    syVectorDiff3D(&vel, &pos, &DObjGetStruct(fighter_gobj)->translate.vec.f);
    mag = syVectorMag3D(&vel);
    if (mag > 50.0F)
    {
        syVectorScale3D(&vel, 50.0F / mag);
    }
    ftCommonTwisterApplyNetplayRiderCanonical(fighter_gobj, fp, &pos, &vel, angle_d);
}

#endif

// 0x801439D0
void ftCommonTwisterProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftStatusVarsTwister(fp)->release_wait++;

    if (ftStatusVarsTwister(fp)->release_wait >= FTCOMMON_TORNADO_RELEASE_WAIT)
    {
#if defined(PORT) && defined(SSB64_NETMENU)
        ftCommonTwisterDiagShoot(fighter_gobj, "release_wait");
#endif
        ftCommonTwisterShootFighter(fighter_gobj);
    }
}

// 0x80143A20
void ftCommonTwisterProcPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *tornado_gobj = ftStatusVarsTwister(fp)->tornado_gobj;

#if defined(PORT) && defined(SSB64_NETMENU)
    /* Netplay rollback only: stale tornado_gobj rebind + early shoot guard. */
    if ((tornado_gobj == NULL) || (DObjGetStruct(tornado_gobj) == NULL))
    {
        /* Netplay rollback only: rebind twister after snapshot coupled-GObj scrub. */
        if (syNetplayRollbackSemanticsActive() != FALSE)
        {
            tornado_gobj = grHyruleGetTwisterGobj();
            if (tornado_gobj != NULL)
            {
                ftStatusVarsTwister(fp)->tornado_gobj = tornado_gobj;
                if (ftCommonTwisterDiagEnabled() != FALSE)
                {
                    port_log(
                        "SSB64 NetRbSnapshot: hyrule_twister_rider_rebind tick=%u player=%d gobj=%p\n",
                        (unsigned int)syNetInputGetTick(), (int)fp->player, (void *)tornado_gobj);
                }
            }
        }
    }
    if ((tornado_gobj == NULL) || (DObjGetStruct(tornado_gobj) == NULL))
    {
        ftCommonTwisterDiagShoot(fighter_gobj, "stale_tornado_gobj");
        ftCommonTwisterShootFighter(fighter_gobj);
        return;
    }
#endif

    Vec3f pos = DObjGetStruct(tornado_gobj)->translate.vec.f;
    Vec3f vel;
    f32 mul;
    f32 angle_d;
    f32 mag;
    f32 unused[2];

    angle_d = (ftStatusVarsTwister(fp)->release_wait * 0.016666668F);
    mul = (((400.0F * angle_d) + 100.0F) * 0.5F);

    pos.x += (mul * lbCommonCos(F_CLC_DTOR32(1800.0F * angle_d)));
    pos.z += (mul * lbCommonSin(F_CLC_DTOR32(1800.0F * angle_d)));
    pos.y += 500.0F * angle_d;

    syVectorDiff3D(&vel, &pos, &DObjGetStruct(fighter_gobj)->translate.vec.f);

    mag = syVectorMag3D(&vel);

    if (mag > 50.0F)
    {
        syVectorScale3D(&vel, 50.0F / mag);
    }
#if defined(PORT) && defined(SSB64_NETMENU)
    /* Netplay rollback only: canonical twister rider pose on F32 grid. */
    if (syNetplaySimQuantizeActive() != FALSE)
    {
        ftCommonTwisterApplyNetplayRiderCanonical(fighter_gobj, fp, &pos, &vel, angle_d);
    }
    else

#endif
    {
        fp->physics.vel_air = vel;

        DObjGetStruct(fighter_gobj)->rotate.vec.f.y =
            (fp->lr * F_CLC_DTOR32(90.0F)) + F_CLC_DTOR32(1800.0F * angle_d);
    }
}

// 0x80143BC4
void ftCommonTwisterSetStatus(GObj *fighter_gobj, GObj *tornado_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

#ifdef PORT
    if ((fp == NULL) || (fp->attr == NULL) || (fp->data == NULL) || (tornado_gobj == NULL) ||
        (DObjGetStruct(tornado_gobj) == NULL) || (DObjGetStruct(fighter_gobj) == NULL))
    {
        return;
    }
#endif

    ftParamStopVoiceRunProcDamage(fighter_gobj);

    if ((fp->item_gobj != NULL) && (itGetStruct(fp->item_gobj)->weight == nITWeightHeavy))
    {
        ftSetupDropItem(fp);
    }
    if (fp->catch_gobj != NULL)
    {
        ftCommonThrownSetStatusDamageRelease(fp->catch_gobj);

        fp->catch_gobj = NULL;
    }
    else if (fp->capture_gobj != NULL)
    {
        ftCommonThrownDecideFighterLoseGrip(fp->capture_gobj, fighter_gobj);
    }
    if (fp->ga == nMPKineticsGround)
    {
        mpCommonSetFighterAir(fp);
    }
    ftMainSetStatus(fighter_gobj, nFTCommonStatusTwister, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftPhysicsStopVelAll(fighter_gobj);

    ftStatusVarsTwister(fp)->release_wait = 0;
    ftStatusVarsTwister(fp)->tornado_gobj = tornado_gobj;

    ftParamSetCaptureImmuneMask(fp, FTCATCHKIND_MASK_ALL);
    func_800269C0_275C0(nSYAudioFGMHyruleTwisterTrapped);
}

// 0x80143CC4
void ftCommonTwisterShootFighter(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
#ifdef PORT
    FTThrowHitDesc *tornado = (FTThrowHitDesc*) (((uintptr_t)gMPCollisionGroundData - (intptr_t)llGRHyruleMapMapHeader) + (intptr_t)llGRHyruleMapTwisterThrowHitDesc);
#else
    FTThrowHitDesc *tornado = (FTThrowHitDesc*) (((uintptr_t)gMPCollisionGroundData - (intptr_t)&llGRHyruleMapMapHeader) + (intptr_t)&llGRHyruleMapTwisterThrowHitDesc);
#endif
    f32 knockback;
    s32 damage;

    DObjGetStruct(fighter_gobj)->translate.vec.f.z = 0.0F;

    knockback = ftParamGetCommonKnockback(fp->percent_damage, tornado->damage, tornado->damage, tornado->knockback_weight, tornado->knockback_scale, tornado->knockback_base, fp->attr->weight, 9, fp->handicap);

    if (ftParamGetBestHitStatusAll(fighter_gobj) != nGMHitStatusNormal)
    {
        damage = 0;
    }
    else damage = tornado->damage;

    ftCommonDamageInitDamageVars(fighter_gobj, -1, damage, knockback, tornado->angle, fp->lr, 0, tornado->element, 0, TRUE, TRUE, TRUE);
    ftParamUpdate1PGameDamageStats(fp, GMCOMMON_PLAYERS_MAX, nFTHitLogObjectGround, nGMHitEnvironmentTwister, 0, 0);

    if (damage != 0)
    {
        ftParamUpdateDamage(fp, damage);
    }
    fp->twister_wait = FTCOMMON_TORNADO_PICKUP_WAIT;
}
