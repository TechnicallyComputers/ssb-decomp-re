#include <ft/fighter.h>
#include <wp/weapon.h>
#ifdef PORT
#include <sys/netrollbacksnapshot.h>
#endif
// // // // // // // // // // // //
//                               //
//             MACROS            //
//                               //
// // // // // // // // // // // //

/*
 * Kirby Copy Mario / Copy Luigi fireball SpecialN (one implementation; Luigi uses
 * CopyLuigi status IDs and fireball index 1 via syNetRbSnapResolveFireballIndex).
 */
#define FTKIRBY_COPY_FIREBALL_IS_MARIO_KIND(copy_id) \
    (((copy_id) == nFTKindMario) || ((copy_id) == nFTKindNMario) || ((copy_id) == nFTKindMMario))

#define FTKIRBY_COPY_FIREBALL_IS_LUIGI_KIND(copy_id) \
    (((copy_id) == nFTKindLuigi) || ((copy_id) == nFTKindNLuigi))

#define FTKIRBY_COPYMARIO_FIREBALL_CHECK_FTKIND(fp, id_mario, id_luigi) \
    (FTKIRBY_COPY_FIREBALL_IS_LUIGI_KIND((fp)->passive_vars.kirby.copy_id) ? (id_luigi) : (id_mario))

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

// 0x801569B0
void ftKirbyCopyMarioSpecialNProcUpdate(GObj *fighter_gobj)
{
#ifdef PORT
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((syNetRbSnapFireballProcAccessoryWillRun(fighter_gobj) == FALSE) ||
        ((fp != NULL) && (fp->proc_accessory == NULL)))
    {
        syNetRbSnapTrySpawnFireballFromAccessory(fighter_gobj);
    }
#endif
    ftAnimEndCheckSetStatus(fighter_gobj, mpCommonSetFighterWaitOrFall);
}

// 0x801569D4
void ftKirbyCopyMarioSpecialNProcAccessory(GObj *fighter_gobj)
{
#ifdef PORT
    syNetRbSnapTrySpawnFireballFromAccessory(fighter_gobj);
#else
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Vec3f pos;
    s32 fireball_kind;

    if (fp->motion_vars.flags.flag0 != 0)
    {
        fp->motion_vars.flags.flag0 = 0;

        pos.x = 0.0F;
        pos.y = 0.0F;
        pos.z = 0.0F;

        gmCollisionGetFighterPartsWorldPosition(fp->joints[FTKIRBY_COPYMARIO_FIREBALL_SPAWN_JOINT], &pos);

        switch (fp->passive_vars.kirby.copy_id) // jtbl at 0x8018C6A0
        {
        case nFTKindMario:
        case nFTKindMMario:
        case nFTKindNMario:
            fireball_kind = 0;
            break;

        #if defined (AVOID_UB)
            return; // This prevents the UB by returning from the function if an unwanted character somehow slips through.
        #else
            break; // Undefined behavior here, var is uninitialized, but projectile spawn function still runs
        #endif

        case nFTKindLuigi:
        case nFTKindNLuigi:
            fireball_kind = 1;
            break;
        }
        wpMarioFireballMakeWeapon(fighter_gobj, &pos, fireball_kind);
    }
#endif
}

// 0x80156A74
void ftKirbyCopyMarioSpecialNProcMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ftKirbyCopyMarioSpecialNSwitchStatusAir);
}

// 0x80156A98
void ftKirbyCopyMarioSpecialAirNProcMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ftKirbyCopyMarioSpecialAirNSwitchStatusGround);
}

// 0x80156ABC
void ftKirbyCopyMarioSpecialAirNSwitchStatusGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);

    ftMainSetStatus(fighter_gobj, FTKIRBY_COPYMARIO_FIREBALL_CHECK_FTKIND(fp, nFTKirbyStatusCopyMarioSpecialN, nFTKirbyStatusCopyLuigiSpecialN), fighter_gobj->anim_frame, 1.0F, FTSTATUS_PRESERVE_COLANIM);

    fp->proc_accessory = ftKirbyCopyMarioSpecialNProcAccessory;
}

// 0x80156B38
void ftKirbyCopyMarioSpecialNSwitchStatusAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);

    ftMainSetStatus(fighter_gobj, FTKIRBY_COPYMARIO_FIREBALL_CHECK_FTKIND(fp, nFTKirbyStatusCopyMarioSpecialAirN, nFTKirbyStatusCopyLuigiSpecialAirN), fighter_gobj->anim_frame, 1.0F, FTSTATUS_PRESERVE_COLANIM);

    ftPhysicsClampAirVelXMax(fp);

    fp->proc_accessory = ftKirbyCopyMarioSpecialNProcAccessory;
}

// 0x80156BB8
void ftKirbyCopyMarioSpecialNInitStatusVars(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->motion_vars.flags.flag0 = 0;
#ifdef PORT
    fp->motion_vars.flags.flag1 = 0;
#endif
    fp->proc_accessory = ftKirbyCopyMarioSpecialNProcAccessory;
}

// 0x80156BD0
void ftKirbyCopyMarioSpecialNSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, FTKIRBY_COPYMARIO_FIREBALL_CHECK_FTKIND(fp, nFTKirbyStatusCopyMarioSpecialN, nFTKirbyStatusCopyLuigiSpecialN), 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftKirbyCopyMarioSpecialNInitStatusVars(fighter_gobj);
}

// 0x80156C38
void ftKirbyCopyMarioSpecialAirNSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, FTKIRBY_COPYMARIO_FIREBALL_CHECK_FTKIND(fp, nFTKirbyStatusCopyMarioSpecialAirN, nFTKirbyStatusCopyLuigiSpecialAirN), 0.0F, 1.0F, FTSTATUS_PRESERVE_FASTFALL);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftKirbyCopyMarioSpecialNInitStatusVars(fighter_gobj);
}
