#include <ft/fighter.h>
#include <wp/weapon.h>
#ifdef PORT
#include <sys/netrollbacksnapshot.h>
#endif

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

#ifdef PORT
static sb32 ftYoshiSpecialHiPortEggIsCharging(WPStruct *wp)
{
	if (wp == NULL)
	{
		return FALSE;
	}
	return ((wp->attack_coll.attack_state == nGMAttackStateOff) &&
	        (wp->weapon_vars.egg_throw.is_throw == FALSE) && (wp->weapon_vars.egg_throw.is_spin == FALSE)) ?
	           TRUE :
	           FALSE;
}

static void ftYoshiSpecialHiPortValidateCoupledEgg(FTStruct *fp)
{
	WPStruct *wp;

	if (fp == NULL)
	{
		return;
	}
	if (fp->status_vars.yoshi.specialhi.egg_gobj == NULL)
	{
		return;
	}
	wp = wpGetStruct(fp->status_vars.yoshi.specialhi.egg_gobj);
	if (ftYoshiSpecialHiPortEggIsCharging(wp) == FALSE)
	{
		fp->status_vars.yoshi.specialhi.egg_gobj = NULL;
	}
}

static void ftYoshiSpecialHiPortCleanupChargeEggs(GObj *fighter_gobj)
{
	FTStruct *fp = ftGetStruct(fighter_gobj);
	WPStruct *wp;

	if (fp == NULL)
	{
		return;
	}
	if (fp->status_vars.yoshi.specialhi.egg_gobj != NULL)
	{
		wp = wpGetStruct(fp->status_vars.yoshi.specialhi.egg_gobj);
		if (ftYoshiSpecialHiPortEggIsCharging(wp) != FALSE)
		{
			wpMainDestroyWeapon(fp->status_vars.yoshi.specialhi.egg_gobj);
		}
		fp->status_vars.yoshi.specialhi.egg_gobj = NULL;
	}
	syNetRbSnapCullYoshiChargeEggsForFighter(fighter_gobj, NULL);
}

static void ftYoshiSpecialHiPortSetStatusWait(GObj *fighter_gobj)
{
    ftYoshiSpecialHiPortCleanupChargeEggs(fighter_gobj);
    ftCommonWaitSetStatus(fighter_gobj);
}

static void ftYoshiSpecialHiPortSetStatusFall(GObj *fighter_gobj)
{
    ftYoshiSpecialHiPortCleanupChargeEggs(fighter_gobj);
    ftCommonFallSetStatus(fighter_gobj);
}
#endif

// 0x8015E980
void ftYoshiSpecialHiProcDamage(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->status_vars.yoshi.specialhi.egg_gobj != NULL)
    {
        wpMainDestroyWeapon(fp->status_vars.yoshi.specialhi.egg_gobj);
    }
}

// 0x8015E9B0
void ftYoshiSpecialHiGetEggPosition(FTStruct *fp, Vec3f *pos)
{
    pos->x = 0.0F;
    pos->y = 0.0F;
    pos->z = 0.0F;

    gmCollisionGetFighterPartsWorldPosition(fp->joints[FTYOSHI_EGGTHROW_JOINT], pos);
}

// 0x8015E9E0
void ftYoshiSpecialHiUpdateEggVectors(FTStruct *fp)
{
    Vec3f pos;
    WPStruct *wp;

#ifdef PORT
    ftYoshiSpecialHiPortValidateCoupledEgg(fp);
#endif
    if (fp->status_vars.yoshi.specialhi.egg_gobj != NULL)
    {
#ifdef PORT
        wp = wpGetStruct(fp->status_vars.yoshi.specialhi.egg_gobj);
        if (ftYoshiSpecialHiPortEggIsCharging(wp) == FALSE)
        {
            return;
        }
#endif
        ftYoshiSpecialHiGetEggPosition(fp, &pos);

        DObjGetStruct(fp->status_vars.yoshi.specialhi.egg_gobj)->translate.vec.f = pos;

        DObjGetStruct(fp->status_vars.yoshi.specialhi.egg_gobj)->scale.vec.f = fp->joints[FTYOSHI_EGGTHROW_JOINT]->scale.vec.f;
    }
}

// 0x8015EA5C
void ftYoshiSpecialHiUpdateEggVars(GObj *fighter_gobj)
{
    WPStruct *wp;
    Vec3f pos;
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag2 == 2)
    {
        fp->motion_vars.flags.flag2 = 0;

#ifdef PORT
        if (fp->status_vars.yoshi.specialhi.egg_gobj == NULL)
        {
            fp->status_vars.yoshi.specialhi.egg_gobj = syNetRbSnapReacquireYoshiChargeEgg(fighter_gobj);
        }
        if (fp->status_vars.yoshi.specialhi.egg_gobj == NULL)
        {
            ftYoshiSpecialHiGetEggPosition(fp, &pos);

            fp->status_vars.yoshi.specialhi.egg_gobj = wpYoshiEggThrowMakeWeapon(fighter_gobj, &pos);
        }
#endif
        if (fp->status_vars.yoshi.specialhi.egg_gobj != NULL)
        {
            wp = wpGetStruct(fp->status_vars.yoshi.specialhi.egg_gobj);

            wp->weapon_vars.egg_throw.is_throw = TRUE;
            wp->weapon_vars.egg_throw.throw_force = fp->status_vars.yoshi.specialhi.throw_force;
            wp->weapon_vars.egg_throw.stick_range = fp->input.pl.stick_range.x;

            mpCommonRunWeaponCollisionDefault(fp->status_vars.yoshi.specialhi.egg_gobj, fp->coll_data.p_translate, &fp->coll_data);

            fp->status_vars.yoshi.specialhi.egg_gobj = NULL;
        }
        fp->motion_vars.flags.flag1 = 1;
    }
    else if (fp->motion_vars.flags.flag2 == 1)
    {
        fp->motion_vars.flags.flag2 = 0;

#ifdef PORT
        if (fp->status_vars.yoshi.specialhi.egg_gobj == NULL)
        {
            fp->status_vars.yoshi.specialhi.egg_gobj = syNetRbSnapReacquireYoshiChargeEgg(fighter_gobj);
        }
        if (fp->status_vars.yoshi.specialhi.egg_gobj == NULL)
#endif
        {
            ftYoshiSpecialHiGetEggPosition(fp, &pos);

            fp->status_vars.yoshi.specialhi.egg_gobj = wpYoshiEggThrowMakeWeapon(fighter_gobj, &pos);
        }
#ifdef PORT
        syNetRbSnapCullYoshiChargeEggsForFighter(fighter_gobj, fp->status_vars.yoshi.specialhi.egg_gobj);
#endif
    }
}

// 0x8015EB0C
void ftYoshiSpecialHiUpdateEggThrowForce(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->input.pl.button_hold & fp->input.button_mask_b)
    {
        fp->status_vars.yoshi.specialhi.throw_force++;
    }
}

// 0x8015EB38
void ftYoshiSpecialHiProcUpdate(GObj *fighter_gobj)
{
    ftYoshiSpecialHiUpdateEggThrowForce(fighter_gobj);
    ftYoshiSpecialHiUpdateEggVars(fighter_gobj);
#ifdef PORT
    ftAnimEndCheckSetStatus(fighter_gobj, ftYoshiSpecialHiPortSetStatusWait);
#else
    ftAnimEndCheckSetStatus(fighter_gobj, ftCommonWaitSetStatus);
#endif
}

// 0x8015EB70
void ftYoshiSpecialAirHiProcUpdate(GObj *fighter_gobj)
{
    ftYoshiSpecialHiUpdateEggThrowForce(fighter_gobj);
    ftYoshiSpecialHiUpdateEggVars(fighter_gobj);
#ifdef PORT
    ftAnimEndCheckSetStatus(fighter_gobj, ftYoshiSpecialHiPortSetStatusFall);
#else
    ftAnimEndCheckSetStatus(fighter_gobj, ftCommonFallSetStatus);
#endif
}

// 0x8015EBA8
void ftYoshiSpecialHiProcPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

#ifdef PORT
    ftYoshiSpecialHiPortValidateCoupledEgg(fp);
    if ((fp->status_vars.yoshi.specialhi.egg_gobj == NULL) && (fp->motion_vars.flags.flag1 == 0))
    {
        fp->status_vars.yoshi.specialhi.egg_gobj = syNetRbSnapReacquireYoshiChargeEgg(fighter_gobj);
    }
    if (fp->status_vars.yoshi.specialhi.egg_gobj != NULL)
    {
        syNetRbSnapCullYoshiChargeEggsForFighter(fighter_gobj, fp->status_vars.yoshi.specialhi.egg_gobj);
    }
#endif
    ftYoshiSpecialHiUpdateEggVectors(fp);
    ftPhysicsApplyGroundVelFriction(fighter_gobj);
}

// 0x8015EBD4
void ftYoshiSpecialAirHiProcPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

#ifdef PORT
    ftYoshiSpecialHiPortValidateCoupledEgg(fp);
    if ((fp->status_vars.yoshi.specialhi.egg_gobj == NULL) && (fp->motion_vars.flags.flag1 == 0))
    {
        fp->status_vars.yoshi.specialhi.egg_gobj = syNetRbSnapReacquireYoshiChargeEgg(fighter_gobj);
    }
    if (fp->status_vars.yoshi.specialhi.egg_gobj != NULL)
    {
        syNetRbSnapCullYoshiChargeEggsForFighter(fighter_gobj, fp->status_vars.yoshi.specialhi.egg_gobj);
    }
#endif
    ftYoshiSpecialHiUpdateEggVectors(fp);
    ftPhysicsApplyAirVelFriction(fighter_gobj);
}

// 0x8015EC00
void ftYoshiSpecialAirHiSwitchStatusGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, nFTYoshiStatusSpecialHi, fighter_gobj->anim_frame, 1.0F, FTSTATUS_PRESERVE_EFFECT);

    fp->proc_damage = ftYoshiSpecialHiProcDamage;
}

// 0x8015EC54
void ftYoshiSpecialHiSwitchStatusAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, nFTYoshiStatusSpecialAirHi, fighter_gobj->anim_frame, 1.0F, FTSTATUS_PRESERVE_EFFECT);

    fp->proc_damage = ftYoshiSpecialHiProcDamage;

    ftPhysicsClampAirVelXMax(fp);
}

// 0x8015ECAC
void ftYoshiSpecialHiProcMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnFloor(fighter_gobj, ftYoshiSpecialHiSwitchStatusAir);
}

// 0x8015ECD0
void ftYoshiSpecialAirHiProcMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag1 != 0)
    {
        mpCommonProcFighterCliff(fighter_gobj, ftYoshiSpecialAirHiSwitchStatusGround);
    }
    else mpCommonProcFighterLanding(fighter_gobj, ftYoshiSpecialAirHiSwitchStatusGround);
}

// 0x8015ED18
void ftYoshiSpecialHiInitStatusVars(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->proc_damage = ftYoshiSpecialHiProcDamage;
    fp->motion_vars.flags.flag2 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->status_vars.yoshi.specialhi.egg_gobj = NULL;
    fp->status_vars.yoshi.specialhi.throw_force = 0;
}

// 0x8015ED3C
void ftYoshiSpecialHiSetStatus(GObj *fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, nFTYoshiStatusSpecialHi, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftYoshiSpecialHiInitStatusVars(fighter_gobj);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

// 0x8015ED7C
void ftYoshiSpecialAirHiSetStatus(GObj *fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, nFTYoshiStatusSpecialAirHi, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftYoshiSpecialHiInitStatusVars(fighter_gobj);
    ftMainPlayAnimEventsAll(fighter_gobj);
}
