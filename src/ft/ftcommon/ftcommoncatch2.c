#include <ft/fighter.h>

#if defined(PORT) && defined(SSB64_NETMENU)
#include <stdio.h>
#include <sys/netinput.h>
#include <sys/net_debug_agent_log.h>
#include <sys/netplay_guard_grab_diag.h>
#endif

// // // // // // // // // // // //
//                               //
//       INITIALIZED DATA        //
//                               //
// // // // // // // // // // // //

// 0x801886D0
Vec3f dFTCommonCatchPullEffectOffset = { 0.0F, 0.0F, 0.0F };

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

// 0x80149EC0
void ftCommonCatchPullProcUpdate(GObj *fighter_gobj)
{
    FTStruct *this_fp = ftGetStruct(fighter_gobj);
#if defined(PORT) && defined(SSB64_NETMENU)
    /*
     * Capture the anim-end input before the check consumes it: ftAnimEndCheckSetStatus is
     * purely gobj->anim_frame <= 0, and this edge is the only thing that sets the victim's
     * capture.is_goto_pulled_wait — i.e. the only thing that makes a grab hold. Sampled
     * here so the value is the pre-transition frame on every pass, replay included.
     */
    f32 ssb64_animend_frame = fighter_gobj->anim_frame;
#endif

    if (ftAnimEndCheckSetStatus(fighter_gobj, ftCommonCatchWaitSetStatus) != FALSE)
    {
        FTStruct *catch_fp = ftGetStruct(this_fp->catch_gobj);

#if defined(PORT) && defined(SSB64_NETMENU)
        syNetplayGuardGrabDiagLogCatchPullAnimEnd(fighter_gobj, TRUE, ssb64_animend_frame,
                                                  this_fp->catch_gobj);
        /*
         * catch_gobj is scrubbed by the snapshot layer when is_catch_or_capture is FALSE
         * (syNetRbSnapClearCoupledGObjPointers…), so a replayed pass can reach here with it
         * cleared. Vanilla dereferences unconditionally; guard the netmenu build the same
         * way ftcommoncapturepulled.c already guards its capture_gobj use.
         */
        if (catch_fp == NULL)
        {
            return;
        }
#endif
        ftStatusVarsCapture(catch_fp)->is_goto_pulled_wait = TRUE;
    }
#if defined(PORT) && defined(SSB64_NETMENU)
    else
    {
        syNetplayGuardGrabDiagLogCatchPullAnimEnd(fighter_gobj, FALSE, ssb64_animend_frame,
                                                  this_fp->catch_gobj);
    }
#endif
}

// 0x80149F04
void ftCommonCatchPullProcCatch(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Vec3f pos;

    ftMainSetStatus(fighter_gobj, nFTCommonStatusCatchPull, ftStatusVarsCatchMain(fp)->catch_pull_frame_begin, 1.0F, (FTSTATUS_PRESERVE_SLOPECONTOUR | FTSTATUS_PRESERVE_EFFECT));

    fp->catch_gobj = fp->search_gobj;

    fp->is_catch_or_capture = FALSE;

    ftParamSetCaptureImmuneMask(fp, FTCATCHKIND_MASK_ALL);

    if (fp->proc_slope != NULL)
    {
        fp->proc_slope(fighter_gobj);
    }
    pos = dFTCommonCatchPullEffectOffset;

    gmCollisionGetFighterPartsWorldPosition(fp->joints[fp->attr->joint_itemheavy_id], &pos);
    efManagerCatchSwirlMakeEffect(&pos);
    ftParamMakeRumble(fp, 9, 0);
}

// 0x80149FCC
void ftCommonCatchWaitProcInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (ftStatusVarsCatchWait(fp)->throw_wait != 0)
    {
        ftStatusVarsCatchWait(fp)->throw_wait--;
    }
    ftCommonThrowCheckInterruptCatchWait(fighter_gobj);
}

// 0x8014A000
void ftCommonCatchWaitSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTCommonStatusCatchWait, 0.0F, 1.0F, FTSTATUS_PRESERVE_SLOPECONTOUR);

    ftStatusVarsCatchWait(fp)->throw_wait = FTCOMMON_CATCH_THROW_WAIT;

#if defined(PORT) && defined(SSB64_NETMENU)
    // #region agent log
    {
        char agent_data[256];

        snprintf(agent_data, sizeof(agent_data),
                 "{\"tick\":%u,\"player\":%d,\"fkind\":%d,\"throw_wait\":%d}",
                 (unsigned int)syNetInputGetTick(), (int)fp->player, (int)fp->fkind,
                 (int)ftStatusVarsCatchWait(fp)->throw_wait);
        net_debug_agent_log_line("G", "ftcommoncatch2.c:CatchWaitSetStatus", "catchwait_set_status", agent_data);
    }
    // #endregion
#endif

    ftParamSetCaptureImmuneMask(fp, FTCATCHKIND_MASK_ALL);

    if ((fp->fkind == nFTKindLink) || (fp->fkind == nFTKindNLink))
    {
        ftParamSetModelPartID(fighter_gobj, 21, 0);
        ftParamSetModelPartID(fighter_gobj, 19, -1);
    }
    else if ((fp->fkind == nFTKindYoshi) || (fp->fkind == nFTKindNYoshi))
    {
        ftParamSetModelPartID(fighter_gobj, 7, 1);
    }
}
