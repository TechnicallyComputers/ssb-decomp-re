#include <ft/fighter.h>
#ifdef PORT
#include <ef/efmanager.h>
extern void *func_800269C0_275C0(u16 id);
#endif
#if defined(PORT) && defined(SSB64_NETMENU)
#include <sys/netplay_sim_quantize.h>
#include <sys/netrollbacksnapshot.h>
#include <sys/netplay_guard_grab_diag.h>
/*
 * SSB64_NETMENU compile gate: stripped from offline (NETMENU=OFF) builds.
 * Runtime: syNetplayRollbackSemanticsActive() gates active VS / resim only.
 * See docs/netplay_rollback_refactor_contracts.md.
 */
#endif

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

// 0x80148E30
void ftCommonGuardSetStatusFromEscape(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTCommonStatusGuardOn, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE); // Why? It overwrites this with Guard later down.
    ftMainPlayAnimEventsAll(fighter_gobj);

    if (fp->shield_health != 0)
    {
        if (fp->fkind == nFTKindYoshi)
        {
#if defined(PORT) && defined(SSB64_NETMENU)
            /* Netplay rollback only: reuse cosmetic egg shell minted during shield escape roll. */
            if ((syNetplayRollbackSemanticsActive() != FALSE) &&
                (syNetRbSnapTryAdoptLiveYoshiShieldForEscapeEnd(fighter_gobj) == NULL))
#endif
            {
                ftStatusVarsGuard(fp)->effect_gobj = efManagerYoshiShieldMakeEffect(fighter_gobj);
            }

            ftParamHideModelPartAll(fighter_gobj);
            ftCommonGuardSetHitStatusYoshi(fighter_gobj);
        }
        else ftStatusVarsGuard(fp)->effect_gobj = efManagerShieldMakeEffect(fighter_gobj);

        fp->is_shield = TRUE;
    }
    ftCommonGuardUpdateJoints(fighter_gobj);

    ftStatusVarsGuard(fp)->release_lag = FTCOMMON_GUARD_RELEASE_LAG;
    ftStatusVarsGuard(fp)->shield_decay_wait = FTCOMMON_GUARD_DECAY_INT;
    ftStatusVarsGuard(fp)->is_release = FALSE;
    ftStatusVarsGuard(fp)->slide_tics = 0;
    ftStatusVarsGuard(fp)->is_setoff = FALSE;

    ftMainSetStatus(fighter_gobj, nFTCommonStatusGuard, 0.0F, 1.0F, (FTSTATUS_PRESERVE_MODELPART | FTSTATUS_PRESERVE_HITSTATUS | FTSTATUS_PRESERVE_EFFECT));

    ftCommonGuardInitJoints(fighter_gobj);

    fp->is_shield = TRUE;
}

// 0x80148F24
sb32 ftCommonGuardCheckInterruptEscape(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp->input.pl.button_hold & fp->input.button_mask_z) && (fp->shield_health != 0))
    {
        ftCommonGuardSetStatusFromEscape(fighter_gobj);

        return TRUE;
    }
    else return FALSE;
}

#if defined(PORT) && defined(SSB64_NETMENU)
static sb32 ftCommonGuardNetplayCatchCheckInterruptGuardDrop(GObj *fighter_gobj)
{
    /*
     * GuardOff / GuardSetOff have NULL proc_interrupt in vanilla. Offline, a release→re-press cadence
     * reaches Wait before the grab edge is consumed; netplay input-delay phase-shift can land the
     * re-press inside the drop window instead. Use vanilla-aligned catch paths only:
     *   - Common: Z held + A tap (neutral grab)
     *   - Guard:  A tap (shield-grab parity with GuardOn/Guard)
     * Do NOT use Attack11 (Z tap alone) here — that path is jab-chain-only in vanilla and caused
     * spurious Z-only grabs when pressing Z to re-shield during GuardOff (follow-up 2026-06-07).
     */
    if (ftCommonCatchCheckInterruptCommon(fighter_gobj) != FALSE)
    {
        return TRUE;
    }
    if (ftCommonCatchCheckInterruptGuard(fighter_gobj) != FALSE)
    {
        return TRUE;
    }
    return FALSE;
}
#endif

// 0x80148F74
void ftCommonGuardOffProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only. */
    /* Netplay rollback only: GuardOff has no proc_interrupt; see docs/bugs/netplay_guardoff_catch_interrupt_2026-06-07.md. */
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        if (ftCommonGuardNetplayCatchCheckInterruptGuardDrop(fighter_gobj) != FALSE)
        {
            syNetplayGuardGrabDiagLogGuardDropCatch(fighter_gobj, TRUE, fp->status_id);
            return;
        }
    }
#endif
    ftCommonGuardUpdateShieldVars(fighter_gobj);

    if (fp->shield_health == 0)
    {
        ftCommonShieldBreakFlyCommonSetStatus(fighter_gobj);
    }
    else if (fighter_gobj->anim_frame <= 0.0F)
    {
        ftCommonWaitSetStatus(fighter_gobj);
    }
    else ftCommonGuardUpdateJoints(fighter_gobj);
}

// 0x80148FF0
void ftCommonGuardOffSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    ub32 flag = fp->is_shield;

    ftMainSetStatus(fighter_gobj, nFTCommonStatusGuardOff, 0.0F, 1.0F, (FTSTATUS_PRESERVE_MODELPART | FTSTATUS_PRESERVE_HITSTATUS | FTSTATUS_PRESERVE_EFFECT));
    ftMainPlayAnimEventsAll(fighter_gobj);

    fp->is_shield = flag;

    ftCommonGuardUpdateJoints(fighter_gobj);
    func_800269C0_275C0(nSYAudioFGMGuardOff);
}

// 0x80149074
void ftCommonGuardSetOffProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only. */
    /* Netplay rollback only: GuardSetOff has no proc_interrupt; see docs/bugs/netplay_guardoff_catch_interrupt_2026-06-07.md. */
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        if (ftCommonGuardNetplayCatchCheckInterruptGuardDrop(fighter_gobj) != FALSE)
        {
            return;
        }
    }
#endif
    ftCommonGuardCheckScheduleRelease(fp);

    ftStatusVarsGuard(fp)->setoff_frames--;

    if (ftStatusVarsGuard(fp)->setoff_frames <= 0.0F)
    {
        if (ftStatusVarsGuard(fp)->is_release != FALSE)
        {
            ftCommonGuardOffSetStatus(fighter_gobj);
        }
        else ftCommonGuardSetStatus(fighter_gobj);
    }
    else ftCommonGuardInitJoints(fighter_gobj);
}

// 0x80149108
void ftCommonGuardSetOffSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTCommonStatusGuardSetOff, 0.0F, 1.0F, (FTSTATUS_PRESERVE_MODELPART | FTSTATUS_PRESERVE_HITSTATUS | FTSTATUS_PRESERVE_EFFECT));

    ftStatusVarsGuard(fp)->setoff_frames = (fp->shield_damage * FTCOMMON_GUARD_SETOFF_MUL) + FTCOMMON_GUARD_SETOFF_ADD;

    fp->physics.vel_ground.x = ((fp->lr == fp->shield_lr) ? -1 : +1) * (ftStatusVarsGuard(fp)->setoff_frames * FTCOMMON_GUARD_VEL_MUL);

    if (ftStatusVarsGuard(fp)->effect_gobj != NULL)
    {
        EFStruct *ep = efGetStruct(ftStatusVarsGuard(fp)->effect_gobj);

        ep->effect_vars.shield.is_damage_shield = TRUE;
    }
    fp->is_shield = TRUE;

    ftStatusVarsGuard(fp)->is_setoff = TRUE;
}
