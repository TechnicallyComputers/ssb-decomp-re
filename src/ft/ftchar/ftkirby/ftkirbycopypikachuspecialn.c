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

#define FTKIRBY_COPYPIKACHU_SPECIALN_STATUS_FLAGS (FTSTATUS_PRESERVE_TEXTUREPART | FTSTATUS_PRESERVE_HITSTATUS | FTSTATUS_PRESERVE_EFFECT | FTSTATUS_PRESERVE_COLANIM)

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

static void ftKirbyCopyPikachuSpecialNProcAccessoryVanilla(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Vec3f pos;
    Vec3f vel;

    if (fp->motion_vars.flags.flag0 != 0)
    {
        fp->motion_vars.flags.flag0 = 0;

        pos.x = 0.0F;
        pos.y = 0.0F;
        pos.z = 0.0F;

        gmCollisionGetFighterPartsWorldPosition(fp->joints[FTKIRBY_COPYPIKACHU_THUNDERJOLT_SPAWN_JOINT], &pos);

        pos.x += FTKIRBY_COPYPIKACHU_THUNDERJOLT_SPAWN_OFF_X * fp->lr;
        pos.y += FTKIRBY_COPYPIKACHU_THUNDERJOLT_SPAWN_OFF_Y;

        vel.x = __cosf(FTKIRBY_COPYPIKACHU_THUNDERJOLT_SPAWN_ANGLE) * FTKIRBY_COPYPIKACHU_THUNDERJOLTVEL * fp->lr;
        vel.y = __sinf(FTKIRBY_COPYPIKACHU_THUNDERJOLT_SPAWN_ANGLE) * FTKIRBY_COPYPIKACHU_THUNDERJOLTVEL;
        vel.z = 0.0F;

        wpPikachuThunderJoltAirMakeWeapon(fighter_gobj, &pos, &vel);
        ftParamCheckSetFighterColAnimID(fighter_gobj, nGMColAnimFighterPikachuSpecialN, 0);
    }
}

// 0x801536C0
void ftKirbyCopyPikachuSpecialNProcUpdate(GObj *fighter_gobj)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    /* Netplay rollback only: rollback thunder jolt spawn when vanilla ProcAccessory is skipped. */
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((syNetRbSnapThunderJoltProcAccessoryWillRun(fighter_gobj) == FALSE) ||
            ((fp != NULL) && (fp->proc_accessory == NULL)))
        {
            syNetRbSnapTrySpawnThunderJoltFromAccessory(fighter_gobj);
        }
    }

#endif
    ftAnimEndSetWait(fighter_gobj);
}

// 0x801536C8
void ftKirbyCopyPikachuSpecialAirNProcUpdate(GObj *fighter_gobj)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    /* Netplay rollback only: rollback thunder jolt spawn when vanilla ProcAccessory is skipped. */
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((syNetRbSnapThunderJoltProcAccessoryWillRun(fighter_gobj) == FALSE) ||
            ((fp != NULL) && (fp->proc_accessory == NULL)))
        {
            syNetRbSnapTrySpawnThunderJoltFromAccessory(fighter_gobj);
        }
    }

#endif
    ftAnimEndSetFall(fighter_gobj);
}

// 0x801536C0
void ftKirbyCopyPikachuSpecialNProcAccessory(GObj *fighter_gobj)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    /* Netplay rollback only: snapshot-driven thunder jolt spawn; offline uses vanilla below. */
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        syNetRbSnapTrySpawnThunderJoltFromAccessory(fighter_gobj);
        return;
    }

#endif
    ftKirbyCopyPikachuSpecialNProcAccessoryVanilla(fighter_gobj);
}

// 0x801537B8
void ftKirbyCopyPikachuSpecialNProcMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnFloor(fighter_gobj, ftKirbyCopyPikachuSpecialNSwitchStatusAir);
}

// 0x801537DC
void ftKirbyCopyPikachuSpecialAirNProcMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ftKirbyCopyPikachuSpecialAirNSwitchStatusGround);
}

// 0x80153800
void ftKirbyCopyPikachuSpecialAirNSwitchStatusGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, nFTKirbyStatusCopyPikachuSpecialN, fighter_gobj->anim_frame, 1.0F, FTKIRBY_COPYPIKACHU_SPECIALN_STATUS_FLAGS);

#if defined(PORT) && defined(SSB64_NETMENU)
    /* Netplay rollback only: preserve throw latch after spawn. */
    if ((syNetplayRollbackSemanticsActive() != FALSE) && (fighter_gobj->anim_frame < 21.0F) &&
        (syNetRbSnapThunderJoltOwnedByFighter(fighter_gobj) == FALSE))
    {
        fp->motion_vars.flags.flag1 = 0;
    }

#endif
    fp->proc_accessory = ftKirbyCopyPikachuSpecialNProcAccessory;
}

// 0x80153854
void ftKirbyCopyPikachuSpecialNSwitchStatusAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, nFTKirbyStatusCopyPikachuSpecialAirN, fighter_gobj->anim_frame, 1.0F, FTKIRBY_COPYPIKACHU_SPECIALN_STATUS_FLAGS);
    ftPhysicsClampAirVelXMax(fp);

#if defined(PORT) && defined(SSB64_NETMENU)
    /* Netplay rollback only: preserve throw latch after spawn. */
    if ((syNetplayRollbackSemanticsActive() != FALSE) && (fighter_gobj->anim_frame < 21.0F) &&
        (syNetRbSnapThunderJoltOwnedByFighter(fighter_gobj) == FALSE))
    {
        fp->motion_vars.flags.flag1 = 0;
    }

#endif
    fp->proc_accessory = ftKirbyCopyPikachuSpecialNProcAccessory;
}

// 0x801538B0
void ftKirbyCopyPikachuSpecialNInitStatusVars(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->motion_vars.flags.flag0 = 0;
#if defined(PORT) && defined(SSB64_NETMENU)
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        fp->motion_vars.flags.flag1 = 0;
    }

#endif
    fp->proc_accessory = ftKirbyCopyPikachuSpecialNProcAccessory;
}

// 0x801538C8
void ftKirbyCopyPikachuSpecialNSetStatus(GObj *fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, nFTKirbyStatusCopyPikachuSpecialN, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftKirbyCopyPikachuSpecialNInitStatusVars(fighter_gobj);
}

// 0x80153908
void ftKirbyCopyPikachuSpecialAirNSetStatus(GObj *fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, nFTKirbyStatusCopyPikachuSpecialAirN, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftKirbyCopyPikachuSpecialNInitStatusVars(fighter_gobj);
}
