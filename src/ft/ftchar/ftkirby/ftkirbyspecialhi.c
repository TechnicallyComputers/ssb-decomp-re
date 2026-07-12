#include <ft/fighter.h>
#include <wp/weapon.h>
#ifdef PORT
#include <sc/scmanager.h>
#include <sys/debug.h>
#endif
#if defined(PORT) && defined(SSB64_NETMENU)
#include <sys/netrollbacksnapshot.h>
#include <sys/netplay_sim_quantize.h>
/* SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only. */
#endif

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

// 0x80160BB0
void ftKirbySpecialHiUpdateEffect(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag1 != 0)
    {
        switch (fp->motion_vars.flags.flag1)
        {
        case 1:
            if (fp->is_effect_attach)
            {
                ftParamProcStopEffect(fighter_gobj);
                fp->motion_vars.flags.flag1 = 0;
            }
#if defined(PORT) && defined(SSB64_NETMENU)
            /* Attach may already be false while cutter shells still live — force clear. */
            else if (syNetplayRollbackSemanticsActive() != FALSE)
            {
                syNetRbSnapForceClearKirbyFinalCutterBlades(fighter_gobj);
                fp->motion_vars.flags.flag1 = 0;
            }
#endif
            break;

        default:
            while (TRUE)
            {
                syDebugPrintf("gcFighterSpecialHiEffectKirby : Error  Unknown value %d \n", fp->motion_vars.flags.flag1);
                scManagerRunPrintGObjStatus();
            }
        }
    }
    switch (fp->motion_vars.flags.flag2)
    {
    case 0:
        break;

    case 1:
        if (fp->is_effect_attach)
        {
            ftParamProcStopEffect(fighter_gobj);
            fp->motion_vars.flags.flag2 = 0;
        }
#if defined(PORT) && defined(SSB64_NETMENU)
        else if (syNetplayRollbackSemanticsActive() != FALSE)
        {
            syNetRbSnapForceClearKirbyFinalCutterBlades(fighter_gobj);
            fp->motion_vars.flags.flag2 = 0;
        }
#endif
        break;

    case 2:
#if defined(PORT) && defined(SSB64_NETMENU)
        /* Stop-before-mint: do not stack Draw on orphan Trail/Up/Down shells. */
        if (syNetplayRollbackSemanticsActive() != FALSE)
        {
            syNetRbSnapForceClearKirbyFinalCutterBlades(fighter_gobj);
        }
#endif
        if (efManagerKirbyCutterDrawMakeEffect(fighter_gobj) != NULL)
        {
            fp->is_effect_attach = TRUE;
            fp->motion_vars.flags.flag2 = 0;
        }
        break;

    case 3:
#if defined(PORT) && defined(SSB64_NETMENU)
        if (syNetplayRollbackSemanticsActive() != FALSE)
        {
            syNetRbSnapForceClearKirbyFinalCutterBlades(fighter_gobj);
        }
#endif
        if (efManagerKirbyCutterUpMakeEffect(fighter_gobj) != NULL)
        {
            fp->is_effect_attach = TRUE;
            fp->motion_vars.flags.flag2 = 0;
        }
        break;

    case 4:
#if defined(PORT) && defined(SSB64_NETMENU)
        if (syNetplayRollbackSemanticsActive() != FALSE)
        {
            syNetRbSnapForceClearKirbyFinalCutterBlades(fighter_gobj);
        }
#endif
        if (efManagerKirbyCutterDownMakeEffect(fighter_gobj) != NULL)
        {
            fp->is_effect_attach = TRUE;
            fp->motion_vars.flags.flag2 = 0;
        }
        break;

    case 5:
#if defined(PORT) && defined(SSB64_NETMENU)
        if (syNetplayRollbackSemanticsActive() != FALSE)
        {
            syNetRbSnapForceClearKirbyFinalCutterBlades(fighter_gobj);
        }
#endif
        if (efManagerKirbyCutterTrailMakeEffect(fighter_gobj) != NULL)
        {
            fp->is_effect_attach = TRUE;
            fp->motion_vars.flags.flag2 = 0;
        }
        break;

    default:
        while (TRUE)
        {
            syDebugPrintf("gcFighterSpecialHiEffectKirby : Error  Unknown value %d \n", fp->motion_vars.flags.flag2);
            scManagerRunPrintGObjStatus();
        }
    }
}

// 0x80160D1C
void ftKirbySpecialHiProcUpdate(GObj *fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ftKirbySpecialAirHiFallSetStatus);
}

// 0x80160D40
void ftKirbySpecialHiLandingProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Vec3f pos;

    if (fp->motion_vars.flags.flag0 != 0)
    {
        fp->motion_vars.flags.flag0 = 0;

        pos.x = 0.0F;
        pos.y = 0.0F;
        pos.z = 0.0F;

        gmCollisionGetFighterPartsWorldPosition(fp->joints[FTKIRBY_FINALCUTTER_BEAM_SPAWN_JOINT], &pos);

        if (fp->lr == +1) pos.x += FTKIRBY_FINALCUTTER_OFF_X; // Ternary doesn't match here, only if/else :(

        else pos.x -= FTKIRBY_FINALCUTTER_OFF_X;

#if defined(PORT) && defined(SSB64_NETMENU)
        /* Synctest/load verify: do not mint +1-tick beam while deferred weapon eject is held. */
        if ((syNetplayRollbackSemanticsActive() == FALSE) ||
            (syNetRbSnapDeferWeaponSimDuringLoadVerify() == FALSE))
#endif
        {
            wpKirbyCutterMakeWeapon(fighter_gobj, &pos);
        }
    }
    ftAnimEndCheckSetStatus(fighter_gobj, ftCommonWaitSetStatus);
}

// 0x80160DF0
void ftKirbySpecialHiProcPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    ftKirbySpecialHiUpdateEffect(fighter_gobj);
    ftPhysicsApplyAirVelTransNYZ(fighter_gobj);

    if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
    {
        ftPhysicsClampAirVelXStickRange(fp, FTPHYSICS_AIRDRIFT_CLAMP_RANGE_MIN, attr->air_accel * FTKIRBY_FINALCUTTER_AIR_ACCEL_MUL, attr->air_speed_max_x);
        ftPhysicsApplyAirVelXFriction(fp, attr);
    }
}

// 0x80160E70
void ftKirbySpecialHiLandingProcPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    ftKirbySpecialHiUpdateEffect(fighter_gobj);

    if (fp->ga == nMPKineticsGround)
    {
        ftPhysicsApplyGroundVelTransN(fighter_gobj);
    }

    else
    {
        ftPhysicsApplyAirVelTransNYZ(fighter_gobj);

        if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
        {
            ftPhysicsClampAirVelXStickRange(fp, FTPHYSICS_AIRDRIFT_CLAMP_RANGE_MIN, attr->air_accel * FTKIRBY_FINALCUTTER_AIR_ACCEL_MUL, attr->air_speed_max_x);
            ftPhysicsApplyAirVelXFriction(fp, attr);
        }
    }
}

// 0x80160F10
void ftKirbySpecialAirHiProcPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;
    f32 temp_scale;

    ftKirbySpecialHiUpdateEffect(fighter_gobj);

    fp->joints[nFTPartsJointTopN]->scale.vec.f.x = fp->joints[nFTPartsJointTopN]->scale.vec.f.y = fp->joints[nFTPartsJointTopN]->scale.vec.f.z = 0.8F;

    ftPhysicsApplyAirVelTransNYZ(fighter_gobj);

    fp->joints[nFTPartsJointTopN]->scale.vec.f.x = fp->joints[nFTPartsJointTopN]->scale.vec.f.y = fp->joints[nFTPartsJointTopN]->scale.vec.f.z = 1.0F;

    if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
    {
        ftPhysicsClampAirVelXStickRange(fp, FTPHYSICS_AIRDRIFT_CLAMP_RANGE_MIN, attr->air_accel * FTKIRBY_FINALCUTTER_AIR_ACCEL_MUL, attr->air_speed_max_x);
        ftPhysicsApplyAirVelXFriction(fp, attr);
    }
}

// 0x80160FD8
void ftKirbySpecialAirHiFallProcPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    ftKirbySpecialHiUpdateEffect(fighter_gobj);

    if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
    {
        ftPhysicsClampAirVelXStickRange(fp, FTPHYSICS_AIRDRIFT_CLAMP_RANGE_MIN, attr->air_accel * FTKIRBY_FINALCUTTER_AIR_ACCEL_MUL, attr->air_speed_max_x);
        ftPhysicsApplyAirVelXFriction(fp, attr);
    }
}

// 0x8016104C
void ftKirbySpecialHiProcMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga == nMPKineticsGround)
    {
        if (mpCommonCheckFighterOnEdge(fighter_gobj) == FALSE)
        {
            fp->ga = nMPKineticsAir;
        }
    }
    else
    {
        if (mpCommonCheckFighterCeilHeavyCliff(fighter_gobj) != FALSE)
        {
            if (fp->coll_data.mask_stat & MAP_FLAG_CLIFF_MASK)
            {
                ftCommonCliffCatchSetStatus(fighter_gobj);
            }
            else if ((fp->coll_data.mask_stat & MAP_FLAG_FLOOR) && (fp->physics.vel_air.y < 0.0F))
            {
                mpCommonSetFighterGround(fp);
                ftKirbySpecialHiLandingSetStatus(fighter_gobj);
            }
        }
    }
}

// 0x80161104
void ftKirbySpecialAirHiFallProcMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (mpCommonCheckFighterCeilHeavyCliff(fighter_gobj) != FALSE)
    {
        if (fp->coll_data.mask_stat & MAP_FLAG_FLOOR)
        {
            mpCommonSetFighterGround(fp);
            /*
             * PRESERVE_NONE is load-bearing: Landing ACMD remints Draw/Up/Down/Trail and assumes
             * AirHiFall's blade was StopEffect'd at land. Netmenu r7 PRESERVE_EFFECT stacked those
             * shells (soak2 seed 1058439841: 5–7 kirby_finalcutter_blade ejects per Wait; hand
             * Trail + TopN blob). Keep vanilla clear-at-land; StopEffect tree clear (ftparam) is
             * the ghost fix. See docs/bugs/netplay_kirby_finalcutter_orphan_blade_2026-07-10.md.
             */
            ftMainSetStatus(fighter_gobj, nFTKirbyStatusSpecialHiLanding, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);

            fp->proc_lagstart = ftParamProcPauseEffect;
            fp->proc_lagend = ftParamProcResumeEffect;
#if defined(PORT) && defined(SSB64_NETMENU)
            /*
             * PRESERVE_NONE only runs StopEffect when is_effect_attach is set. Netplay often clears
             * that flag while cutter shells still live — land entry then no-ops and Landing ACMD
             * stacks remints (soak2: attach_restore on 257 then 5–7 Wait ejects). Force-clear
             * regardless of the flag. See docs/bugs/netplay_kirby_finalcutter_orphan_blade_2026-07-10.md.
             */
            if (syNetplayRollbackSemanticsActive() != FALSE)
            {
                syNetRbSnapForceClearKirbyFinalCutterBlades(fighter_gobj);
            }
#endif
        }
        else if (fp->coll_data.mask_stat & MAP_FLAG_CLIFF_MASK)
        {
            ftCommonCliffCatchSetStatus(fighter_gobj);
        }
    }
}

// 0x80161194
void ftKirbySpecialHiProcStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    
    fp->motion_vars.flags.flag0 = fp->motion_vars.flags.flag1 = fp->motion_vars.flags.flag2 = 0;
}

// 0x801611A8
void ftKirbySpecialHiSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->proc_status = ftKirbySpecialHiProcStatus;

    ftMainSetStatus(fighter_gobj, nFTKirbyStatusSpecialHi, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);

    fp->proc_lagstart = ftParamProcPauseEffect;
    fp->proc_lagend = ftParamProcResumeEffect;
}

// 0x80161210
void ftKirbySpecialHiLandingSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTKirbyStatusSpecialHiLanding, 0.0F, 1.0F, FTSTATUS_PRESERVE_EFFECT);
    ftMainPlayAnimEventsAll(fighter_gobj);

    fp->proc_lagstart = ftParamProcPauseEffect;
    fp->proc_lagend = ftParamProcResumeEffect;
}

// 0x80161270
void ftKirbySpecialAirHiSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->proc_status = ftKirbySpecialHiProcStatus;

    ftMainSetStatus(fighter_gobj, nFTKirbyStatusSpecialAirHi, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);

    fp->proc_lagstart = ftParamProcPauseEffect;
    fp->proc_lagend = ftParamProcResumeEffect;
}

// 0x801612D8
void ftKirbySpecialAirHiFallSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 vel_y_bak = fp->physics.vel_air.y;

    ftMainSetStatus(fighter_gobj, nFTKirbyStatusSpecialAirHiFall, 0.0F, 1.0F, FTSTATUS_PRESERVE_EFFECT);
    ftMainPlayAnimEventsAll(fighter_gobj);

    fp->proc_lagstart = ftParamProcPauseEffect;
    fp->proc_lagend = ftParamProcResumeEffect;

    fp->jumps_used = fp->attr->jumps_max;

    fp->physics.vel_air.y = vel_y_bak;
}
