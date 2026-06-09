#include <ft/fighter.h>
#if defined(PORT) && defined(SSB64_NETMENU)
#include <sys/netplay_fallspecial_pass_diag.h>
#include <sys/netplay_fallspecial_pass_gate.h>
#endif

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

// 0x80143730
void ftCommonFallSpecialProcInterrupt(GObj *fighter_gobj)
{
    ftCommonJumpAerialCheckInterruptCommon(fighter_gobj);
}

// 0x80143750
void ftCommonFallSpecialProcPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    ftPhysicsCheckSetFastFall(fp);

    if (fp->is_fastfall)
    {
        ftPhysicsApplyFastFall(fp, attr);
    }
    else if (ftStatusVarsFallSpecial(fp)->is_fall_accelerate != FALSE) // Accelerate until fighter reaches terminal velocity?
    {
        ftPhysicsApplyGravityDefault(fp, attr);
    }
    else ftPhysicsApplyGravityClampTVel(fp, attr->gravity, attr->tvel_fast);

    if (ftPhysicsCheckClampAirVelXDec(fp, ftStatusVarsFallSpecial(fp)->drift) == FALSE)
    {
        ftPhysicsClampAirVelXStickRange(fp, FTPHYSICS_AIRDRIFT_CLAMP_RANGE_MIN, attr->air_accel, ftStatusVarsFallSpecial(fp)->drift);
        ftPhysicsApplyAirVelXFriction(fp, attr);
    }
}

// 0x80143808
sb32 ftCommonFallSpecialProcPass(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    sb32 block;

#if defined(PORT) && defined(SSB64_NETMENU)
    syNetplayFallSpecialPassGateHardenAllowPass(fighter_gobj);
#endif
    if ((ftStatusVarsFallSpecial(fp)->is_allow_pass == FALSE) || !(fp->coll_data.floor_flags & MAP_VERTEX_COLL_PASS) || (fp->input.pl.stick_range.y >= FTCOMMON_FALLSPECIAL_PASS_STICK_RANGE_MIN))
    {
        block = TRUE;
    }
    else block = FALSE;

#if defined(PORT) && defined(SSB64_NETMENU)
    syNetplayFallSpecialPassDiagLogProcPass(fighter_gobj, "fallspecial", block);
#endif
    return block;
}

// 0x8014384C
void ftCommonFallSpecialProcMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (mpCommonCheckFighterPassCliff(fighter_gobj, ftCommonFallSpecialProcPass) != FALSE)
    {
        if (fp->coll_data.mask_stat & MAP_FLAG_CLIFF_MASK)
        {
            ftCommonCliffCatchSetStatus(fighter_gobj);
        }
        else
        {
#if defined(PORT) && defined(SSB64_NETMENU)
            syNetplayFallSpecialPassDiagLogPassCliff(fighter_gobj, "fallspecial_map");
#endif
            if ((ftStatusVarsFallSpecial(fp)->is_goto_landing != FALSE) || (fp->physics.vel_air.y < FTCOMMON_FALLSPECIAL_SKIPLANDING_VEL_Y_MAX))
            {
                ftCommonLandingFallSpecialSetStatus(fighter_gobj, ftStatusVarsFallSpecial(fp)->is_allow_interrupt, ftStatusVarsFallSpecial(fp)->landing_lag);
            }
            else ftCommonWaitSetStatus(fighter_gobj);
        }
    }
}

// 0x801438F0
void ftCommonFallSpecialSetStatus(GObj *fighter_gobj, f32 drift, sb32 unknown, sb32 is_fall_accelerate, sb32 is_goto_landing, f32 landing_lag, sb32 is_allow_interrupt)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    ftMainSetStatus(fighter_gobj, nFTCommonStatusFallSpecial, 0.0F, 1.0F, FTSTATUS_PRESERVE_FASTFALL);

    ftStatusVarsFallSpecial(fp)->drift = (attr->air_speed_max_x * drift);

    ftPhysicsClampAirVelX(fp, ftStatusVarsFallSpecial(fp)->drift);

    if (fp->ga == nMPKineticsGround)
    {
        mpCommonSetFighterAir(fp);
    }
    fp->jumps_used = attr->jumps_max;

    ftStatusVarsFallSpecial(fp)->is_allow_pass = TRUE;
    ftStatusVarsFallSpecial(fp)->is_goto_landing = is_goto_landing;
    ftStatusVarsFallSpecial(fp)->landing_lag = landing_lag;
    ftStatusVarsFallSpecial(fp)->is_allow_interrupt = is_allow_interrupt;
    ftStatusVarsFallSpecial(fp)->is_fall_accelerate = is_fall_accelerate;

    ftParamCheckSetFighterColAnimID(fighter_gobj, nGMColAnimFighterFallSpecial, 0);
    ftPublicTryPlayFallSpecialReact(fighter_gobj);

    fp->is_special_interrupt = TRUE;

#if defined(PORT) && defined(SSB64_NETMENU)
    syNetplayFallSpecialPassDiagLogFallSpecialEnter(fighter_gobj, "set_status");
#endif
}
