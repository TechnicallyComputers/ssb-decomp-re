#include <ft/fighter.h>
#include <wp/weapon.h>
#ifdef PORT
#include <ft/ftcommon/ftcommonfunctions.h>
#endif
#if defined(PORT) && defined(SSB64_NETMENU)
#include <wp/wpvars.h>
#include <ft/ftchar/ftsamus/ftsamus.h>
extern wpSamusChargeShotAttributes dWPSamusChargeShotWeaponAttributes[];
#endif
#if defined(PORT) && defined(SSB64_NETMENU)
#include <sys/netrollbacksnapshot.h>
#include <sys/netplay_sim_quantize.h>
/*
 * SSB64_NETMENU compile gate: stripped from offline builds.
 * Runtime: syNetplayRollbackSemanticsActive() gates reacquire/cull only.
 */
#endif

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

#if defined(PORT) && defined(SSB64_NETMENU)
static sb32 ftSamusSpecialNPortChargeShotIsCharging(WPStruct *wp)
{
	if (wp == NULL)
	{
		return FALSE;
	}
	return (wp->weapon_vars.charge_shot.is_release == FALSE) ? TRUE : FALSE;
}

static void ftSamusSpecialNPortRefreshChargeShotGfx(FTStruct *fp)
{
	GObj *charge_gobj;
	WPStruct *wp;
	f32 scale;

	if (fp == NULL)
	{
		return;
	}
	charge_gobj = fp->status_vars.samus.specialn.charge_gobj;
	if (charge_gobj == NULL)
	{
		return;
	}
	wp = wpGetStruct(charge_gobj);
	if ((wp == NULL) || (wp->kind != nWPKindChargeShot))
	{
		return;
	}
	if (wp->weapon_vars.charge_shot.is_release != FALSE)
	{
		return;
	}
	wp->weapon_vars.charge_shot.charge_size = fp->passive_vars.samus.charge_level;
	scale = dWPSamusChargeShotWeaponAttributes[wp->weapon_vars.charge_shot.charge_size].gfx_size /
	        WPCHARGESHOT_GFX_SIZE_DIV;
	DObjGetStruct(charge_gobj)->scale.vec.f.x = scale;
	DObjGetStruct(charge_gobj)->scale.vec.f.y = scale;
}

static void ftSamusSpecialNPortValidateCoupledCharge(FTStruct *fp)
{
	WPStruct *wp;

	if (fp == NULL)
	{
		return;
	}
	if (fp->status_vars.samus.specialn.charge_gobj == NULL)
	{
		return;
	}
	wp = wpGetStruct(fp->status_vars.samus.specialn.charge_gobj);
	if (ftSamusSpecialNPortChargeShotIsCharging(wp) == FALSE)
	{
		fp->status_vars.samus.specialn.charge_gobj = NULL;
	}
}

static void ftSamusSpecialNPortCleanupChargeShots(GObj *fighter_gobj)
{
	ftSamusSpecialNDestroyChargeShot(fighter_gobj);
}

static void ftSamusSpecialNPortSetStatusWaitOrFall(GObj *fighter_gobj)
{
	ftSamusSpecialNPortCleanupChargeShots(fighter_gobj);
	mpCommonSetFighterWaitOrFall(fighter_gobj);
}

static void ftSamusSpecialNPortSetStatusWait(GObj *fighter_gobj)
{
	ftSamusSpecialNPortCleanupChargeShots(fighter_gobj);
	ftCommonWaitSetStatus(fighter_gobj);
}

static sb32 ftSamusSpecialNPortIsChargeCoupleStatus(const FTStruct *fp)
{
	if (fp == NULL)
	{
		return FALSE;
	}
	return ((fp->status_id == nFTSamusStatusSpecialNStart) || (fp->status_id == nFTSamusStatusSpecialNLoop) ||
	        (fp->status_id == nFTSamusStatusSpecialAirNStart)) ?
	           TRUE :
	           FALSE;
}

sb32 ftSamusSpecialNPortReconcileMaxChargeLoopIfNeeded(GObj *fighter_gobj)
{
	FTStruct *fp;

	if ((fighter_gobj == NULL) || (syNetplayRollbackSemanticsActive() == FALSE))
	{
		return FALSE;
	}
	fp = ftGetStruct(fighter_gobj);
	if ((fp == NULL) || (fp->status_id != nFTSamusStatusSpecialNLoop) ||
	    (fp->passive_vars.samus.charge_level < FTSAMUS_CHARGE_MAX))
	{
		return FALSE;
	}
	ftParamCheckSetFighterColAnimID(fighter_gobj, nGMColAnimFighterCommonSpecialNCharge, 0);
	ftSamusSpecialNPortSetStatusWait(fighter_gobj);
	return TRUE;
}

static void ftSamusSpecialNPortEnsureCoupledChargeShot(GObj *fighter_gobj)
{
	FTStruct *fp = ftGetStruct(fighter_gobj);
	Vec3f pos;

	if ((syNetplayRollbackSemanticsActive() == FALSE) || (ftSamusSpecialNPortIsChargeCoupleStatus(fp) == FALSE) ||
	    (fp->passive_vars.samus.charge_level >= FTSAMUS_CHARGE_MAX))
	{
		return;
	}
	ftSamusSpecialNPortValidateCoupledCharge(fp);
	/* Netplay rollback only: reacquire orphaned charge shot after snapshot scrub. */
	if (fp->status_vars.samus.specialn.charge_gobj == NULL)
	{
		fp->status_vars.samus.specialn.charge_gobj = syNetRbSnapReacquireChargeShotForFP(fp);
	}
	if (fp->status_vars.samus.specialn.charge_gobj == NULL)
	{
		ftSamusSpecialNGetChargeShotPosition(fp, &pos);
		fp->status_vars.samus.specialn.charge_gobj =
		    wpSamusChargeShotMakeWeapon(fighter_gobj, &pos, fp->passive_vars.samus.charge_level, FALSE);
	}
	if (fp->status_vars.samus.specialn.charge_gobj != NULL)
	{
		/* Netplay rollback only: cull duplicate charge shots on shared grid. */
		syNetRbSnapCullSamusChargeShotsForFighter(fighter_gobj, fp->status_vars.samus.specialn.charge_gobj);
		ftSamusSpecialNPortRefreshChargeShotGfx(fp);
	}
}

#endif

// 0x8015D300
void ftSamusSpecialNDestroyChargeShot(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->status_vars.samus.specialn.charge_gobj != NULL)
    {
        wpMainDestroyWeapon(fp->status_vars.samus.specialn.charge_gobj);

        fp->status_vars.samus.specialn.charge_gobj = NULL;
    }
#if defined(PORT) && defined(SSB64_NETMENU)
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        syNetRbSnapCullSamusChargeShotsForFighter(fighter_gobj, NULL);
    }

#endif
}

// 0x8015D338 - Runs when Samus is hit out of Charge Shot
void ftSamusSpecialNProcDamage(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->passive_vars.samus.charge_level = 0;

    ftSamusSpecialNDestroyChargeShot(fighter_gobj);
}

// 0x8015D35C
void ftSamusSpecialNGetChargeShotPosition(FTStruct *fp, Vec3f *pos)
{
    pos->y = pos->z = 0.0F;
    pos->x = FTSAMUS_CHARGE_OFF_X;

    gmCollisionGetFighterPartsWorldPosition(fp->joints[FTSAMUS_CHARGE_JOINT], pos);
}

// 0x8015D394
void ftSamusSpecialNSetChargeShotPosition(FTStruct *fp)
{
    Vec3f pos;

#if defined(PORT) && defined(SSB64_NETMENU)
    ftSamusSpecialNPortValidateCoupledCharge(fp);
    /* Netplay rollback only: reacquire charge shot pointer after rollback load. */
    if ((syNetplayRollbackSemanticsActive() != FALSE) && (ftSamusSpecialNPortIsChargeCoupleStatus(fp) != FALSE) &&
        (fp->passive_vars.samus.charge_level < FTSAMUS_CHARGE_MAX) &&
        (fp->status_vars.samus.specialn.charge_gobj == NULL))
    {
        fp->status_vars.samus.specialn.charge_gobj = syNetRbSnapReacquireChargeShotForFP(fp);
    }
#endif
    if (fp->status_vars.samus.specialn.charge_gobj != NULL)
    {
        ftSamusSpecialNGetChargeShotPosition(fp, &pos);

        DObjGetStruct(fp->status_vars.samus.specialn.charge_gobj)->translate.vec.f = pos;
    }
}

// 0x8015D3EC
void ftSamusSpecialNStartProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fighter_gobj->anim_frame <= 0.0F)
    {
        if (fp->ga == nMPKineticsAir)
        {
            ftSamusSpecialAirNEndSetStatus(fighter_gobj);
        }
        else if (fp->status_vars.samus.specialn.is_release != FALSE)
        {
            ftSamusSpecialNEndSetStatus(fighter_gobj);
        }
        else ftSamusSpecialNLoopSetStatus(fighter_gobj);
    }
}

// 0x8015D464
void ftSamusSpecialNStartProcInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp->input.pl.button_tap & fp->input.button_mask_b) || (fp->input.pl.button_tap & fp->input.button_mask_a))
    {
        fp->status_vars.samus.specialn.is_release = TRUE;
    }
}

// 0x8015D49C
void ftSamusSpecialNStartProcMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ftSamusSpecialNStartSwitchStatusAir);
}

// 0x8015D4E4
void ftSamusSpecialAirNStartProcMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ftSamusSpecialAirNStartSwitchStatusGround);
}

// 0x8015D4E4
void ftSamusSpecialAirNStartSwitchStatusGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);

    ftMainSetStatus(fighter_gobj, nFTSamusStatusSpecialNStart, fighter_gobj->anim_frame, fp->joints[nFTPartsJointTopN]->anim_speed, FTSTATUS_PRESERVE_COLANIM);

    fp->proc_damage = ftSamusSpecialNProcDamage;
}

// 0x8015D540
void ftSamusSpecialNStartSwitchStatusAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, nFTSamusStatusSpecialAirNStart, fighter_gobj->anim_frame, fp->joints[nFTPartsJointTopN]->anim_speed, FTSTATUS_PRESERVE_COLANIM);
    ftPhysicsClampAirVelXMax(fp);

    fp->proc_damage = ftSamusSpecialNProcDamage;

    fp->status_vars.samus.specialn.is_release = TRUE;
}

// 0x8015D5AC
void ftSamusSpecialNLoopProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

#if defined(PORT) && defined(SSB64_NETMENU)
    if ((syNetplayRollbackSemanticsActive() != FALSE) &&
        (ftSamusSpecialNPortReconcileMaxChargeLoopIfNeeded(fighter_gobj) != FALSE))
    {
        return;
    }
    ftSamusSpecialNPortEnsureCoupledChargeShot(fighter_gobj);
#endif
    fp->status_vars.samus.specialn.charge_int--;

    if (fp->status_vars.samus.specialn.charge_int == 0)
    {
        fp->status_vars.samus.specialn.charge_int = FTSAMUS_CHARGE_INT;

        if (fp->passive_vars.samus.charge_level <= (FTSAMUS_CHARGE_MAX - 1))
        {
            fp->passive_vars.samus.charge_level++;

            if (fp->passive_vars.samus.charge_level == FTSAMUS_CHARGE_MAX)
            {
                ftParamCheckSetFighterColAnimID(fighter_gobj, nGMColAnimFighterCommonSpecialNCharge, 0);
                ftSamusSpecialNDestroyChargeShot(fighter_gobj);
#if defined(PORT) && defined(SSB64_NETMENU)
                ftSamusSpecialNPortSetStatusWait(fighter_gobj);
#else
                ftCommonWaitSetStatus(fighter_gobj);
#endif
            }
            else if (fp->status_vars.samus.specialn.charge_gobj != NULL)
            {
                WPStruct *wp = wpGetStruct(fp->status_vars.samus.specialn.charge_gobj);

                wp->weapon_vars.charge_shot.charge_size = fp->passive_vars.samus.charge_level;
            }
        }
    }
}

// 0x8015D640
void ftSamusSpecialNLoopProcInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 status_id;

    if ((fp->input.pl.button_tap & fp->input.button_mask_b) || (fp->input.pl.button_tap & fp->input.button_mask_a))
    {
        ftSamusSpecialNEndSetStatus(fighter_gobj);
        return;
    }
    status_id = ftCommonEscapeGetStatus(fp);

    if (status_id != -1)
    {
        ftSamusSpecialNDestroyChargeShot(fighter_gobj);

#ifdef PORT
        /*
         * WARNING: Undefined behavior in the original game — this call omits
         * the third argument (itemthrow_buffer_tics). PORT: pass 0 so the
         * behavior is deterministic rather than reading an uninitialized register.
         */
        ftCommonEscapeSetStatus(fighter_gobj, status_id, 0);
#else
        /* 
         * WARNING: Undefined behavior. This function expects a third argument
         * for item throw buffer frames, but never receives it.
         */ 
        ftCommonEscapeSetStatus(fighter_gobj, status_id);
#endif
    }
    else if (fp->input.pl.button_tap & fp->input.button_mask_z)
    {
        ftSamusSpecialNDestroyChargeShot(fighter_gobj);
#if defined(PORT) && defined(SSB64_NETMENU)
        ftSamusSpecialNPortSetStatusWait(fighter_gobj);
#else
        ftCommonWaitSetStatus(fighter_gobj);
#endif
    }
}

// 0x8015D700
void ftSamusSpecialNLoopProcMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

#if defined(PORT) && defined(SSB64_NETMENU)
    ftSamusSpecialNPortEnsureCoupledChargeShot(fighter_gobj);
#endif
    ftSamusSpecialNSetChargeShotPosition(fp);
    mpCommonProcFighterOnEdge(fighter_gobj, ftSamusSpecialAirNEndSetStatus);
}

// 0x8015D734
void ftSamusSpecialNLoopSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Vec3f pos;

    ftMainSetStatus(fighter_gobj, nFTSamusStatusSpecialNLoop, 0.0F, 1.0F, FTSTATUS_PRESERVE_COLANIM);

    fp->proc_damage = ftSamusSpecialNProcDamage;
    fp->status_vars.samus.specialn.charge_int = FTSAMUS_CHARGE_INT;

    ftSamusSpecialNGetChargeShotPosition(fp, &pos);
#if defined(PORT) && defined(SSB64_NETMENU)
    if ((syNetplayRollbackSemanticsActive() != FALSE) &&
        (ftSamusSpecialNPortReconcileMaxChargeLoopIfNeeded(fighter_gobj) != FALSE))
    {
        return;
    }
    ftSamusSpecialNPortEnsureCoupledChargeShot(fighter_gobj);
#else
    fp->status_vars.samus.specialn.charge_gobj = wpSamusChargeShotMakeWeapon(fighter_gobj, &pos, fp->passive_vars.samus.charge_level, FALSE);
#endif
}

// 0x8015D7AC
void ftSamusSpecialNEndProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Vec3f pos;
    WPStruct *wp;
    f32 charge_recoil_x;
    f32 charge_recoil_y;

    if (fp->motion_vars.flags.flag0 != FALSE)
    {
        fp->motion_vars.flags.flag0 = FALSE;

        ftSamusSpecialNGetChargeShotPosition(fp, &pos);

#if defined(PORT) && defined(SSB64_NETMENU)
        {
            sb32 allow_charge_spawn;

            allow_charge_spawn = TRUE;
            if ((syNetplayRollbackSemanticsActive() != FALSE) &&
                (syNetRbSnapDeferWeaponSimDuringLoadVerify() != FALSE))
            {
                allow_charge_spawn = FALSE;
            }
            if (allow_charge_spawn != FALSE)
            {
                if (syNetplayRollbackSemanticsActive() != FALSE)
                {
                    if (fp->status_vars.samus.specialn.charge_gobj == NULL)
                    {
                        fp->status_vars.samus.specialn.charge_gobj = syNetRbSnapReacquireChargeShotForFP(fp);
                    }
                }
                if (fp->status_vars.samus.specialn.charge_gobj == NULL)
                {
                    fp->status_vars.samus.specialn.charge_gobj =
                        wpSamusChargeShotMakeWeapon(fighter_gobj, &pos, fp->passive_vars.samus.charge_level, TRUE);
                }
            }
        }

#endif
        if (fp->status_vars.samus.specialn.charge_gobj != NULL)
        {
            wp = wpGetStruct(fp->status_vars.samus.specialn.charge_gobj);
            ftParamStopLoopSFX(fp);

            DObjGetStruct(fp->status_vars.samus.specialn.charge_gobj)->translate.vec.f = pos;

            wp->weapon_vars.charge_shot.is_full_charge = TRUE;
            wp->weapon_vars.charge_shot.charge_size = fp->passive_vars.samus.charge_level;

            mpCommonRunWeaponCollisionDefault(fp->status_vars.samus.specialn.charge_gobj, fp->coll_data.p_translate, &fp->coll_data);

            wp->weapon_vars.charge_shot.owner_gobj = NULL;
            fp->status_vars.samus.specialn.charge_gobj = NULL;
        }
#if defined(PORT) && defined(SSB64_NETMENU)
        else if ((syNetplayRollbackSemanticsActive() == FALSE) ||
                 (syNetRbSnapDeferWeaponSimDuringLoadVerify() == FALSE))
#endif
        {
            wpSamusChargeShotMakeWeapon(fighter_gobj, &pos, fp->passive_vars.samus.charge_level, TRUE);
        }

        if (fp->ga == nMPKineticsAir)
        {
            charge_recoil_x = (fp->passive_vars.samus.charge_level + 1);

            fp->physics.vel_air.x = ((FTSAMUS_CHARGE_RECOIL_MUL * charge_recoil_x) + FTSAMUS_CHARGE_RECOIL_BASE) * -fp->lr;

            charge_recoil_y = charge_recoil_x + FTSAMUS_CHARGE_RECOIL_ADD + (fp->passive_vars.samus.charge_recoil * -FTSAMUS_CHARGE_RECOIL_BASE);

            if (fp->physics.vel_air.y < charge_recoil_y)
            {
                fp->physics.vel_air.y = charge_recoil_y;
            }
            fp->passive_vars.samus.charge_recoil++;
        }
        else fp->physics.vel_ground.x = -((FTSAMUS_CHARGE_RECOIL_MUL * (fp->passive_vars.samus.charge_level + 1)) + FTSAMUS_CHARGE_RECOIL_BASE);
        
        fp->passive_vars.samus.charge_level = 0;
        fp->proc_damage = NULL;
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
#if defined(PORT) && defined(SSB64_NETMENU)
        ftSamusSpecialNPortSetStatusWaitOrFall(fighter_gobj);
#else
        mpCommonSetFighterWaitOrFall(fighter_gobj);
#endif
    }
}

// 0x8015D968
void ftSamusSpecialNEndProcMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ftSamusSpecialNEndSwitchStatusAir);
}

// 0x8015D98C
void ftSamusSpecialAirNEndProcMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ftSamusSpecialAirNEndSwitchStatusGround);
}

// 0x8015D9B0
void ftSamusSpecialAirNEndSwitchStatusGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, nFTSamusStatusSpecialNEnd, fighter_gobj->anim_frame, 1.0F, FTSTATUS_PRESERVE_COLANIM);

    fp->proc_damage = ftSamusSpecialNProcDamage;
}

// 0x8015DA04
void ftSamusSpecialNEndSwitchStatusAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, nFTSamusStatusSpecialAirNEnd, fighter_gobj->anim_frame, 1.0F, FTSTATUS_PRESERVE_COLANIM);
    ftPhysicsClampAirVelXMax(fp);

    fp->proc_damage = ftSamusSpecialNProcDamage;
}

// 0x8015DA60
void ftSamusSpecialNEndSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTSamusStatusSpecialNEnd, 0.0F, 1.0F, FTSTATUS_PRESERVE_COLANIM);

    fp->proc_damage = ftSamusSpecialNProcDamage;
}

// 0x8015DAA8
void ftSamusSpecialAirNEndSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga == nMPKineticsGround)
    {
        mpCommonSetFighterAir(fp);
        ftPhysicsClampAirVelXMax(fp);
    }
    ftMainSetStatus(fighter_gobj, nFTSamusStatusSpecialAirNEnd, 0.0F, 1.0F, FTSTATUS_PRESERVE_COLANIM);

    fp->proc_damage = ftSamusSpecialNProcDamage;
}

// 0x8015DB14
f32 ftSamusSpecialNStartGetAnimSpeed(FTStruct *fp)
{
    f32 ret = fp->passive_vars.samus.charge_level / (f32)FTSAMUS_CHARGE_MAX;

    ret = (-0.16000003F) * ret + 1.0F;

    return ret;
}

// 0x8015DB4C
void ftSamusSpecialNStartInitStatusVars(FTStruct *fp)
{
    fp->proc_damage = ftSamusSpecialNProcDamage;
    fp->status_vars.samus.specialn.charge_gobj = NULL;
    fp->motion_vars.flags.flag0 = FALSE;
}

// 0x8015DB64
void ftSamusSpecialNStartSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTSamusStatusSpecialNStart, 0.0F, ftSamusSpecialNStartGetAnimSpeed(fp), FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftSamusSpecialNStartInitStatusVars(fp);

    fp->status_vars.samus.specialn.is_release = (fp->passive_vars.samus.charge_level == FTSAMUS_CHARGE_MAX) ? TRUE : FALSE;
}

// 0x8015DBDC
void ftSamusSpecialAirNStartSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTSamusStatusSpecialAirNStart, 0.0F, ftSamusSpecialNStartGetAnimSpeed(fp), FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftSamusSpecialNStartInitStatusVars(fp);

    fp->status_vars.samus.specialn.is_release = TRUE;
}
