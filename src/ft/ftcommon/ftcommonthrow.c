#include <ft/fighter.h>
#if defined(PORT) && defined(SSB64_NETMENU)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern void port_log(const char *fmt, ...);
#include <sys/net_debug_agent_log.h>
extern u32 syNetInputGetTick(void);
#endif

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

// 0x8014A0C0
void ftCommonThrowProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag1 != 0)
    {
        fp->motion_vars.flags.flag1 = 0;

        fp->lr = -fp->lr;

        fp->physics.vel_ground.x = -fp->physics.vel_ground.x;
    }
    if (fp->motion_vars.flags.flag2 != 0)
    {
        ftCommonThrownProcPhysics(fp->catch_gobj);
        ftCommonThrownReleaseThrownUpdateStats(fp->catch_gobj, (fp->motion_vars.flags.flag2 == 1) ? -fp->lr : fp->lr, (fp->status_id == nFTCommonStatusThrowB) ? 1 : 0, TRUE);

        fp->motion_vars.flags.flag2 = 0;

        fp->catch_gobj = NULL;

        ftParamSetCaptureImmuneMask(fp, FTCATCHKIND_MASK_NONE);
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        if ((fp->fkind == nFTKindDonkey) || (fp->fkind == nFTKindNDonkey) || (fp->fkind == nFTKindGDonkey))
        {
            if (fp->status_id == nFTCommonStatusThrowF)
            {
                ftCommonCaptureShoulderedSetStatus(fp->catch_gobj);
                ftDonkeyThrowFWaitSetStatus(fighter_gobj);

                return;
            }
        }
        mpCommonSetFighterWaitOrFall(fighter_gobj);
    }
}

// 0x8014A1E8
void ftCommonThrowSetStatus(GObj *fighter_gobj, sb32 is_throwf)
{
    FTStruct *this_fp = ftGetStruct(fighter_gobj);
    s32 status_id;
    GObj *catch_gobj;
    FTStruct *catch_fp;
    FTThrownStatus *thrown_status;

    catch_gobj = this_fp->catch_gobj;
    catch_fp = ftGetStruct(catch_gobj);

    if ((is_throwf != FALSE) || ((this_fp->input.pl.stick_range.x * this_fp->lr) >= 0))
    {
        if ((this_fp->fkind == nFTKindKirby) || (this_fp->fkind == nFTKindNKirby))
        {
            status_id = nFTKirbyStatusThrowF;

            mpCommonSetFighterAir(this_fp);
        }
        else status_id = nFTCommonStatusThrowF;
#ifdef PORT
    /* PORT (JRickey): reloc-resolved thrown_status table lookup. */
        thrown_status = &((FTThrownStatusArray*)PORT_RESOLVE(this_fp->attr->thrown_status))[catch_fp->fkind].ft_thrown[0];
#else
        thrown_status = &this_fp->attr->thrown_status[catch_fp->fkind].ft_thrown[0];
#endif
    }
    else
    {
        status_id = nFTCommonStatusThrowB;
#ifdef PORT
    /* PORT (JRickey): reloc-resolved thrown_status table lookup. */
        thrown_status = &((FTThrownStatusArray*)PORT_RESOLVE(this_fp->attr->thrown_status))[catch_fp->fkind].ft_thrown[1];
#else
        thrown_status = &this_fp->attr->thrown_status[catch_fp->fkind].ft_thrown[1];
#endif
    }
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftParamSetCaptureImmuneMask(this_fp, FTCATCHKIND_MASK_ALL);

    this_fp->motion_vars.flags.flag2 = 0;
    this_fp->motion_vars.flags.flag1 = 0;

    if ((this_fp->fkind == nFTKindSamus) || (this_fp->fkind == nFTKindNSamus))
    {
        if (efManagerSamusGrappleBeamGlowMakeEffect(fighter_gobj) != NULL)
        {
            this_fp->is_effect_attach = TRUE;
        }
    }
    if (thrown_status->status1 != -1)
    {
        ftCommonThrownSetStatusQueue(catch_gobj, thrown_status->status1, thrown_status->status2);
    }
    else ftCommonThrownSetStatusImmediate(catch_gobj, thrown_status->status2);

    if ((this_fp->fkind == nFTKindKirby) || (this_fp->fkind == nFTKindNKirby))
    {
        if (status_id == nFTKirbyStatusThrowF)
        {
            this_fp->is_ignore_dead = TRUE;
            catch_fp->is_ignore_dead = TRUE;
        }
    }
}

// 0x8014A394
sb32 ftCommonThrowCheckInterruptCatchWait(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    sb32 is_throwf = FALSE;
#if defined(PORT) && defined(SSB64_NETMENU)
    const char *catchwait_diag = getenv("SSB64_NETPLAY_CATCHWAIT_DIAG");
#endif

    if ((ftStatusVarsCatchWait(fp)->throw_wait == 0) || (fp->input.pl.button_tap & (fp->input.button_mask_a | fp->input.button_mask_b)))
    {
        is_throwf = TRUE;
    }
    else if ((fp->input.pl.stick_prev.x >= FTCOMMON_CATCH_THROW_STICK_RANGE_MIN) || (fp->input.pl.stick_range.x < FTCOMMON_CATCH_THROW_STICK_RANGE_MIN))
    {
        if ((fp->input.pl.stick_prev.x <= -FTCOMMON_CATCH_THROW_STICK_RANGE_MIN) || (fp->input.pl.stick_range.x > -FTCOMMON_CATCH_THROW_STICK_RANGE_MIN))
        {
            return FALSE;
        }
    }
#if defined(PORT) && defined(SSB64_NETMENU)
    if ((catchwait_diag != NULL) && (catchwait_diag[0] != '\0') && (strcmp(catchwait_diag, "0") != 0))
    {
        port_log(
            "SSB64 NetCatch: catchwait_throw tick=%u player=%d throw_wait=%d button_tap=0x%04X mask_a=0x%04X "
            "mask_b=0x%04X stick_prev_x=%d stick_x=%d is_throwf=%d catch_gobj=%p\n",
            (unsigned int)syNetInputGetTick(), (int)fp->player, (int)ftStatusVarsCatchWait(fp)->throw_wait,
            (unsigned int)fp->input.pl.button_tap, (unsigned int)fp->input.button_mask_a,
            (unsigned int)fp->input.button_mask_b, (int)fp->input.pl.stick_prev.x, (int)fp->input.pl.stick_range.x,
            (int)is_throwf, (void *)fp->catch_gobj);
    }
    if (is_throwf != FALSE)
    {
        // #region agent log
        char agent_data[384];

        snprintf(agent_data, sizeof(agent_data),
                 "{\"tick\":%u,\"player\":%d,\"throw_wait\":%d,\"shuffle_tics\":%d,\"button_tap\":%u,\"mask_a\":%u,\"mask_b\":%u,"
                 "\"stick_prev_x\":%d,\"stick_x\":%d,\"premature\":%d}",
                 (unsigned int)syNetInputGetTick(), (int)fp->player,
                 (int)ftStatusVarsCatchWait(fp)->throw_wait, (int)fp->shuffle_tics,
                 (unsigned int)fp->input.pl.button_tap,
                 (unsigned int)fp->input.button_mask_a, (unsigned int)fp->input.button_mask_b,
                 (int)fp->input.pl.stick_prev.x, (int)fp->input.pl.stick_range.x,
                 (ftStatusVarsCatchWait(fp)->throw_wait > 0) ? 1 : 0);
        net_debug_agent_log_line("G", "ftcommonthrow.c:CatchWait", "catchwait_throw_decision", agent_data);
        // #endregion
    }
#endif
    ftCommonThrowSetStatus(fighter_gobj, is_throwf);

    return TRUE;
}
