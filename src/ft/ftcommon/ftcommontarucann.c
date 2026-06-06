#include <ft/fighter.h>
#include <it/item.h>
#include <gr/ground.h>
#include <sc/scene.h>
#include <reloc_data.h>
#if defined(PORT) && defined(SSB64_NETMENU)
#include <sys/netinput.h>
#include <sys/netplay_sim_quantize.h>
/*
 * SSB64_NETMENU compile gate: stripped from offline (NETMENU=OFF) builds.
 * Runtime: syNetplayRollbackSemanticsActive() gates active VS / resim only.
 * See docs/netplay_rollback_refactor_contracts.md.
 */
#endif
#ifdef PORT
extern void *func_800269C0_275C0(u16 id);
#endif

// 0x80143E10
void ftCommonTaruCannProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (ftStatusVarsTaruCann(fp)->shoot_wait != 0)
    {
        ftStatusVarsTaruCann(fp)->shoot_wait--;

        if (ftStatusVarsTaruCann(fp)->shoot_wait == (FTCOMMON_TARUCANN_SHOOT_WAIT / 2))
        {
            func_800269C0_275C0(nSYAudioFGMJungleTaruCannShoot);
        }
        if (ftStatusVarsTaruCann(fp)->shoot_wait == 0)
        {
            ftCommonTaruCannShootFighter(fighter_gobj);

            return;
        }
    }
    ftStatusVarsTaruCann(fp)->release_wait++;

    if ((ftStatusVarsTaruCann(fp)->release_wait >= FTCOMMON_TARUCANN_RELEASE_WAIT) && (ftStatusVarsTaruCann(fp)->shoot_wait == 0))
    {
        ftStatusVarsTaruCann(fp)->shoot_wait = FTCOMMON_TARUCANN_SHOOT_WAIT;

        grJungleTaruCannAddAnimShoot(ftStatusVarsTaruCann(fp)->tarucann_gobj);
    }
}

// 0x80143EB0
void ftCommonTaruCannProcInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((ftStatusVarsTaruCann(fp)->shoot_wait == 0) && (fp->input.pl.button_tap & (fp->input.button_mask_a | fp->input.button_mask_b)))
    {
        ftStatusVarsTaruCann(fp)->shoot_wait = FTCOMMON_TARUCANN_SHOOT_WAIT;

        grJungleTaruCannAddAnimShoot(ftStatusVarsTaruCann(fp)->tarucann_gobj);
    }
}

// 0x80143F04
void ftCommonTaruCannProcPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *tarucann_gobj = ftStatusVarsTaruCann(fp)->tarucann_gobj;

#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only. */
    /* Netplay rollback only: re-establish barrel pointer after snapshot coupled-GObj scrub. */
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        if (tarucann_gobj == NULL)
        {
            tarucann_gobj = grJungleGetTaruCannGobj();
            if (tarucann_gobj != NULL)
            {
                ftStatusVarsTaruCann(fp)->tarucann_gobj = tarucann_gobj;
            }
        }
        if ((tarucann_gobj == NULL) || (grJungleEnsureTaruCannCoupling(tarucann_gobj) == FALSE))
        {
            return;
        }
    }
#endif
    if (tarucann_gobj == NULL)
    {
        return;
    }

    DObjGetStruct(fighter_gobj)->translate.vec.f = DObjGetStruct(tarucann_gobj)->translate.vec.f;
#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only. */
    /* Netplay rollback only: F32 grid for rider/barrel translate (no-op unless syNetplaySimQuantizeActive()). */
    syNetplayQuantizeDObjTranslate(DObjGetStruct(tarucann_gobj));
    syNetplayQuantizeDObjTranslate(DObjGetStruct(fighter_gobj));
#endif
}

// 0x80143F30
void ftCommonTaruCannSetStatus(GObj *fighter_gobj, GObj *tarucann_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

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
    ftMainSetStatus(fighter_gobj, nFTCommonStatusTaruCann, 0.0F, 0.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftPhysicsStopVelAll(fighter_gobj);

    ftStatusVarsTaruCann(fp)->shoot_wait = 0;
    ftStatusVarsTaruCann(fp)->release_wait = 0;
    ftStatusVarsTaruCann(fp)->tarucann_gobj = tarucann_gobj;

    ftParamSetHitStatusAll(fighter_gobj, nGMHitStatusIntangible);

    fp->is_invisible = TRUE;

    ftParamSetCaptureImmuneMask(fp, FTCATCHKIND_MASK_ALL);
    func_800269C0_275C0(nSYAudioFGMJungleTaruCannEnter);
}

// 0x80144038
void ftCommonTaruCannShootFighter(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
#ifdef PORT
    FTThrowHitDesc *tarucann = (FTThrowHitDesc*) (((uintptr_t)gMPCollisionGroundData - (intptr_t)llGRJungleMapMapHeader) + (intptr_t)llGRJungleMapTaruCannThrowHitDesc);
#else
    FTThrowHitDesc *tarucann = (FTThrowHitDesc*) (((uintptr_t)gMPCollisionGroundData - (intptr_t)&llGRJungleMapMapHeader) + (intptr_t)&llGRJungleMapTaruCannThrowHitDesc);
#endif
    f32 knockback;
    s32 angle;

    DObjGetStruct(fighter_gobj)->translate.vec.f.z = 0.0F;

    knockback = ftParamGetGroundHazardKnockback(fp->percent_damage, tarucann->damage, tarucann->damage, tarucann->knockback_weight, tarucann->knockback_scale, tarucann->knockback_base, fp->attr->weight, 9, 9);

    angle = ((I_CLC_RTOD32(grJungleTaruCannGetRotate()) * -fp->lr) + 90);
    angle -= (angle / 360) * 360;

    ftCommonDamageInitDamageVars(fighter_gobj, nFTCommonStatusDamageFlyRoll, tarucann->damage, knockback, angle, fp->lr, 0, tarucann->element, 0, TRUE, TRUE, FALSE);
    ftParamUpdate1PGameDamageStats(fp, GMCOMMON_PLAYERS_MAX, nFTHitLogObjectGround, nGMHitEnvironmentTaruCann, 0, 0);

    fp->playertag_wait = 0;
    fp->tarucann_wait = FTCOMMON_TARUCANN_PICKUP_WAIT;
}

#if defined(PORT) && defined(SSB64_NETMENU)
static sb32 ftCommonTaruCannTryRearmShootFromInputHistory(FTStruct *fp, u32 snap_tick)
{
    SYNetInputFrame cur;
    SYNetInputFrame prev;
    u16 fire_mask;

    if ((fp == NULL) || (snap_tick == 0U) || (snap_tick == 0xFFFFFFFFU))
    {
        return FALSE;
    }
    fire_mask = (u16)(fp->input.button_mask_a | fp->input.button_mask_b);
    if (fire_mask == 0U)
    {
        return FALSE;
    }
    if ((syNetInputGetHistoryFrame(fp->player, snap_tick, &cur) == FALSE) ||
        (syNetInputGetHistoryFrame(fp->player, snap_tick - 1U, &prev) == FALSE))
    {
        return FALSE;
    }
    if (((cur.buttons & fire_mask) != 0U) && ((prev.buttons & fire_mask) == 0U))
    {
        return TRUE;
    }
    return FALSE;
}

void ftCommonTaruCannReconcileShootStateAfterRollback(GObj *fighter_gobj, u32 snap_tick)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *tarucann_gobj;
    s32 release_wait;
    s32 shoot_wait;

    if ((fp == NULL) || (fp->status_id != nFTCommonStatusTaruCann))
    {
        return;
    }
    tarucann_gobj = ftStatusVarsTaruCann(fp)->tarucann_gobj;
    if (tarucann_gobj == NULL)
    {
        tarucann_gobj = grJungleGetTaruCannGobj();
    }
    if (tarucann_gobj == NULL)
    {
        return;
    }
    ftStatusVarsTaruCann(fp)->tarucann_gobj = tarucann_gobj;

    release_wait = ftStatusVarsTaruCann(fp)->release_wait;
    shoot_wait = ftStatusVarsTaruCann(fp)->shoot_wait;

    /* In-progress shot from snapshot: sync barrel anim only; ProcUpdate owns countdown + eject. */
    if (shoot_wait > 0)
    {
        grJungleTaruCannAddAnimShoot(tarucann_gobj);
        return;
    }
    /* Rollback load may restore past the auto-fire threshold without shoot_wait armed yet. */
    if (release_wait >= FTCOMMON_TARUCANN_RELEASE_WAIT)
    {
        ftStatusVarsTaruCann(fp)->shoot_wait = FTCOMMON_TARUCANN_SHOOT_WAIT;
        grJungleTaruCannAddAnimShoot(tarucann_gobj);
        return;
    }
    /* Manual fire only: confirmed tap edge on the restore tick. */
    if (ftCommonTaruCannTryRearmShootFromInputHistory(fp, snap_tick) != FALSE)
    {
        ftStatusVarsTaruCann(fp)->shoot_wait = FTCOMMON_TARUCANN_SHOOT_WAIT;
        grJungleTaruCannAddAnimShoot(tarucann_gobj);
        return;
    }
    /* Orphan shoot joint during release countdown: suppress pop; do not arm shoot_wait. */
    if (grJungleTaruCannIsChildShootAnimActive(tarucann_gobj) != FALSE)
    {
        grJungleTaruCannAddAnimFill(tarucann_gobj);
        return;
    }
    grJungleTaruCannAddAnimFill(tarucann_gobj);
}
#endif
