#include <ft/fighter.h>
#include <wp/weapon.h>
#if defined(PORT) && defined(SSB64_NETMENU)
#include <sys/netrollbacksnapshot.h>
#include <sys/netplay_sim_quantize.h>
/*
 * Netplay rollback forward-sim: gate policy on syNetplayRollbackSemanticsActive().
 * See docs/netplay_rollback_refactor_contracts.md.
 */

#endif
// // // // // // // // // // // //
//                               //
//             MACROS            //
//                               //
// // // // // // // // // // // //

#define FTKIRBY_COPYNESS_SPECIALN_STATUS_FLAGS (FTSTATUS_PRESERVE_TEXTUREPART | FTSTATUS_PRESERVE_HITSTATUS | FTSTATUS_PRESERVE_EFFECT | FTSTATUS_PRESERVE_COLANIM)

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

static void ftKirbyCopyNessSpecialNProcAccessoryVanilla(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Vec3f pos;
    Vec3f vel;
    f32 angle;

    if (fp->motion_vars.flags.flag0 != FALSE)
    {
        fp->motion_vars.flags.flag0 = FALSE;

        pos.x = 0.0F;
        pos.y = 0.0F;
        pos.z = 0.0F;

        gmCollisionGetFighterPartsWorldPosition(fp->joints[FTKIRBY_COPYNESS_PKFIRE_SPAWN_JOINT], &pos);

        pos.x += FTKIRBY_COPYNESS_PKFIRE_SPAWN_OFF_X * fp->lr;
        pos.y += FTKIRBY_COPYNESS_PKFIRE_SPAWN_OFF_Y;

        if (fp->ga == nMPKineticsAir)
        {
            vel.z = 0.0F;
            angle = FTKIRBY_COPYNESS_PKFIRE_SPARK_ANGLE_AIR;
            vel.x = __cosf(FTKIRBY_COPYNESS_PKFIRE_SPARK_ANGLE_AIR) * FTKIRBY_COPYNESS_PKFIRE_SPARK_VEL_AIR * fp->lr;
            vel.y = __sinf(FTKIRBY_COPYNESS_PKFIRE_SPARK_ANGLE_AIR) * FTKIRBY_COPYNESS_PKFIRE_SPARK_VEL_AIR;
        }
        else
        {
            vel.z = 0.0F;
            angle = FTKIRBY_COPYNESS_PKFIRE_SPARK_ANGLE_GROUND;
            vel.x = __cosf(FTKIRBY_COPYNESS_PKFIRE_SPARK_ANGLE_GROUND) * FTKIRBY_COPYNESS_PKFIRE_SPARK_VEL_GROUND * fp->lr;
            vel.y = __sinf(FTKIRBY_COPYNESS_PKFIRE_SPARK_ANGLE_GROUND) * FTKIRBY_COPYNESS_PKFIRE_SPARK_VEL_GROUND;
        }

        wpNessPKFireMakeWeapon(fighter_gobj, &pos, &vel, angle);
    }
}

// 0x80155B40
void ftKirbyCopyNessSpecialNProcUpdate(GObj *fighter_gobj)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((syNetRbSnapFireballProcAccessoryWillRun(fighter_gobj) == FALSE) ||
            ((fp != NULL) && (fp->proc_accessory == NULL)))
        {
            syNetRbSnapTrySpawnPKFireFromAccessory(fighter_gobj);
        }
    }

#endif
    ftAnimEndCheckSetStatus(fighter_gobj, mpCommonSetFighterWaitOrFall);
}

void ftKirbyCopyNessSpecialNProcAccessory(GObj *fighter_gobj)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        syNetRbSnapTrySpawnPKFireFromAccessory(fighter_gobj);
        return;
    }

#endif
    ftKirbyCopyNessSpecialNProcAccessoryVanilla(fighter_gobj);
}

// 0x80155CAC
void ftKirbyCopyNessSpecialNProcMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnFloor(fighter_gobj, ftKirbyCopyNessSpecialNSwitchStatusAir);
}

// 0x80155CD0
void ftKirbyCopyNessSpecialAirNProcMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ftKirbyCopyNessSpecialAirNSwitchStatusGround);
}

// 0x80155CF4
void ftKirbyCopyNessSpecialAirNSwitchStatusGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, nFTKirbyStatusCopyNessSpecialN, fighter_gobj->anim_frame, 1.0F, FTKIRBY_COPYNESS_SPECIALN_STATUS_FLAGS);

    fp->proc_accessory = ftKirbyCopyNessSpecialNProcAccessory;
}

// 0x80155D48
void ftKirbyCopyNessSpecialNSwitchStatusAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, nFTKirbyStatusCopyNessSpecialAirN, fighter_gobj->anim_frame, 1.0F, FTKIRBY_COPYNESS_SPECIALN_STATUS_FLAGS);
    ftPhysicsClampAirVelXMax(fp);

    fp->proc_accessory = ftKirbyCopyNessSpecialNProcAccessory;
}

// 0x80155DA4
void ftKirbyCopyNessSpecialNInitStatusVars(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;

    fp->proc_accessory = ftKirbyCopyNessSpecialNProcAccessory;
}

// 0x80155DBC
void ftKirbyCopyNessSpecialNSetStatus(GObj *fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, nFTKirbyStatusCopyNessSpecialN, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftKirbyCopyNessSpecialNInitStatusVars(fighter_gobj);
}

// 0x80155DFC
void ftKirbyCopyNessSpecialAirNSetStatus(GObj *fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, nFTKirbyStatusCopyNessSpecialAirN, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftKirbyCopyNessSpecialNInitStatusVars(fighter_gobj);
}
