#include <ft/fighter.h>
#if defined(PORT) && defined(SSB64_NETMENU)
#include <sys/netplay_sim_quantize.h>
#include <sys/netplay_branch_predict.h>
#endif

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

// 0x8013E690
void ftCommonTurnProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only. */
    /* Netplay rollback only: repair stomped turn.lr_dash / lr_turn before allow/dash. */
    syNetplayHardenTurnLrDash(fp);
    syNetplayHardenTurnLrTurn(fp);
    /* Netplay diagnostics: Turn SetFlag1 / allow gate for dash-dance. */
    syNetplayMaybeLogTurnDashWitness(fighter_gobj, "update", fp->motion_vars.flags.flag1, FALSE);
#endif

    if (fp->motion_vars.flags.flag1 != 0)
    {
        fp->motion_vars.flags.flag1 = 0;

        ftStatusVarsTurn(fp)->is_allow_turn_direction = TRUE;
        ftStatusVarsTurn(fp)->is_disable_sa_interrupts = TRUE;

        fp->lr = -fp->lr;
        fp->physics.vel_ground.x = -fp->physics.vel_ground.x;
#if defined(PORT) && defined(SSB64_NETMENU)
        /* After facing flip, re-repair so stick*lr_turn uses turn direction (== new lr). */
        syNetplayHardenTurnLrDash(fp);
        syNetplayHardenTurnLrTurn(fp);
#endif
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ftCommonWaitSetStatus(fighter_gobj);
    }
}

// Dawg what
void ftCommonTurnProcInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    sb32 is_interrupt_attacks4;

#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only. */
    /* Netplay rollback only: repair stomped turn.lr_dash / lr_turn before dash-out check. */
    syNetplayHardenTurnLrDash(fp);
    syNetplayHardenTurnLrTurn(fp);
#endif

    if (ftStatusVarsTurn(fp)->is_allow_turn_direction != FALSE)
    {
        fp->input.pl.button_tap |= ftStatusVarsTurn(fp)->button_mask;
    }

    if (ftStatusVarsTurn(fp)->is_disable_sa_interrupts == FALSE)
    {
        goto skip_interrupt_specials;
    } 
    if (ftCommonSpecialNCheckInterruptCommon(fighter_gobj) != FALSE) return;
    if (ftCommonSpecialHiCheckInterruptCommon(fighter_gobj) != FALSE) return;
    if (ftCommonSpecialLwCheckInterruptCommon(fighter_gobj) != FALSE) return;

skip_interrupt_specials:
    if (ftCommonCatchCheckInterruptCommon(fighter_gobj) == FALSE)
    {
        if (ftStatusVarsTurn(fp)->attacks4_buffer < 256)
        {
            ftStatusVarsTurn(fp)->attacks4_buffer++;
        } 
        is_interrupt_attacks4 = (ftStatusVarsTurn(fp)->attacks4_buffer < 6) ? ftCommonAttackS4CheckInterruptTurn(fighter_gobj) : ftCommonAttackS4CheckInterruptCommon(fighter_gobj);

        if (is_interrupt_attacks4 == FALSE)
        {
            if (ftStatusVarsTurn(fp)->is_disable_sa_interrupts == FALSE) 
            {
                goto skip_interrupt_attacks;
            }
            if (ftCommonAttackHi4CheckInterruptCommon(fighter_gobj) != FALSE) return;
            if (ftCommonAttackLw4CheckInterruptCommon(fighter_gobj) != FALSE) return;
            if (ftCommonAttackS3CheckInterruptCommon(fighter_gobj) != FALSE) return;
            if (ftCommonAttackHi3CheckInterruptCommon(fighter_gobj) != FALSE) return;
            if (ftCommonAttackLw3CheckInterruptCommon(fighter_gobj) != FALSE) return;
            if (ftCommonAttack1CheckInterruptCommon(fighter_gobj) != FALSE) return;
            
        skip_interrupt_attacks:
            if 
            (
                (ftCommonGuardOnCheckInterruptCommon(fighter_gobj) != FALSE) ||
                (ftCommonAppealCheckInterruptCommon(fighter_gobj) != FALSE)  ||
                (ftCommonKneeBendCheckInterruptCommon(fighter_gobj) != FALSE)
            )
            {
                return;
            }
            {
                sb32 will_dash = FALSE;
#if defined(PORT) && defined(SSB64_NETMENU)
                /* SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only. */
                /*
                 * Transactional branch eval: DashCheckTurn prepare writes (lr_dash,
                 * attacks4_buffer, entry sticky) + optional DashSetStatus are one unit.
                 * Under predicted remote input, discard the whole candidate. See
                 * docs/bugs/netplay_branch_sensitive_predict_2026-07-20.md.
                 */
                if (syNetplayRollbackSemanticsActive() != FALSE)
                {
                    syNetplayBranchTurnDashEvalBegin(fighter_gobj);
                }
#endif
                ftCommonDashCheckTurn(fighter_gobj);

                if ((ftStatusVarsTurn(fp)->is_allow_turn_direction != FALSE) &&
                    (ftStatusVarsTurn(fp)->lr_dash != 0) &&
                    ((fp->input.pl.stick_range.x * ftStatusVarsTurn(fp)->lr_turn) >=
                     FTCOMMON_DASH_STICK_RANGE_MIN))
                {
                    will_dash = TRUE;
                }
#if defined(PORT) && defined(SSB64_NETMENU)
                if (syNetplayRollbackSemanticsActive() != FALSE)
                {
                    will_dash = syNetplayBranchTurnDashEvalResolve(fighter_gobj, will_dash);
                }
                /* Netplay diagnostics: log Turn→Dash gate after transactional resolve. */
                syNetplayMaybeLogTurnDashWitness(fighter_gobj, "interrupt", 0, will_dash);
#endif
                if (will_dash != FALSE)
                {
                    ftCommonDashSetStatus(fighter_gobj, 0);
                }
            }
            if (fp->input.pl.button_tap & fp->input.button_mask_a)
            {
                ftStatusVarsTurn(fp)->button_mask |= fp->input.button_mask_a;
            }
            if (fp->input.pl.button_tap & fp->input.button_mask_b)
            {
                ftStatusVarsTurn(fp)->button_mask |= fp->input.button_mask_b;
            }
            if (ftStatusVarsTurn(fp)->is_allow_turn_direction != FALSE)
            {
                ftStatusVarsTurn(fp)->is_allow_turn_direction = FALSE;
            }
        }
    }
}

// 0x8013E908
void ftCommonTurnSetStatus(GObj *fighter_gobj, s32 lr_dash)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 lr_facing;
    s32 lr_turn;

    fp->motion_vars.flags.flag1 = 0;

    /*
     * Capture facing before SetStatus/PlayAnim. InvertLR already passes lr_dash == -facing;
     * prefer that so lr_turn matches even if fp->lr is briefly cleared. Center turn keeps -facing.
     */
    lr_facing = fp->lr;
    lr_turn = (lr_dash != 0) ? lr_dash : -lr_facing;

    ftMainSetStatus(fighter_gobj, nFTCommonStatusTurn, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);

    ftStatusVarsTurn(fp)->is_allow_turn_direction = FALSE;
    ftStatusVarsTurn(fp)->is_disable_sa_interrupts = FALSE;
    ftStatusVarsTurn(fp)->button_mask = 0;
    ftStatusVarsTurn(fp)->lr_dash = lr_dash;
    ftStatusVarsTurn(fp)->attacks4_buffer = (lr_dash != 0) ? 0 : 256;
    ftStatusVarsTurn(fp)->lr_turn = lr_turn;
#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only. */
    /* Pin InvertLR/Center lr_dash; re-apply if a same-frame union stomp cleared facing. */
    syNetplayTurnNoteEntryLrDash(fp, lr_dash);
    syNetplayHardenTurnLrDash(fp);
    syNetplayHardenTurnLrTurn(fp);
#endif
}

// 0x8013E988
void ftCommonTurnSetStatusCenter(GObj *fighter_gobj)
{
    ftCommonTurnSetStatus(fighter_gobj, 0);
}

// 0x8013E9A8
void ftCommonTurnSetStatusInvertLR(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftCommonTurnSetStatus(fighter_gobj, -fp->lr);
}

// 0x8013ED90
sb32 ftCommonTurnCheckInputSuccess(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp->input.pl.stick_range.x * fp->lr) <= FTCOMMON_TURN_STICK_RANGE_MIN)
    {
        return TRUE;
    }
    else return FALSE;
}

// 0x8013EA04
sb32 ftCommonTurnCheckInterruptCommon(GObj *fighter_gobj)
{
    if (ftCommonTurnCheckInputSuccess(fighter_gobj) != FALSE)
    {
        ftCommonTurnSetStatusCenter(fighter_gobj);

        return TRUE;
    }
    else return FALSE;
}
