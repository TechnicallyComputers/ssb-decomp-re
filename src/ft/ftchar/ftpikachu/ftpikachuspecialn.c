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

#define FTPIKACHU_SPECIALN_STATUS_FLAGS (FTSTATUS_PRESERVE_TEXTUREPART | FTSTATUS_PRESERVE_HITSTATUS | FTSTATUS_PRESERVE_EFFECT | FTSTATUS_PRESERVE_COLANIM)

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

static void ftPikachuSpecialNProcAccessoryVanilla(GObj *fighter_gobj)
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

        gmCollisionGetFighterPartsWorldPosition(fp->joints[FTPIKACHU_THUNDERJOLT_SPAWN_JOINT], &pos);

        vel.x = __cosf(FTPIKACHU_THUNDERJOLT_SPAWN_ANGLE) * FTPIKACHU_THUNDERJOLTVEL * fp->lr;
        vel.y = __sinf(FTPIKACHU_THUNDERJOLT_SPAWN_ANGLE) * FTPIKACHU_THUNDERJOLTVEL;
        vel.z = 0.0F;

        wpPikachuThunderJoltAirMakeWeapon(fighter_gobj, &pos, &vel);
        ftParamCheckSetFighterColAnimID(fighter_gobj, nGMColAnimFighterPikachuSpecialN, 0);
    }
}

// 0x80151B50
void ftPikachuSpecialNProcUpdate(GObj *fighter_gobj)
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

// 0x80151B58
void ftPikachuSpecialAirNProcUpdate(GObj *fighter_gobj)
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

// 0x80151B50
void ftPikachuSpecialNProcAccessory(GObj *fighter_gobj)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    /* Netplay rollback only: snapshot-driven thunder jolt spawn; offline uses vanilla below. */
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        syNetRbSnapTrySpawnThunderJoltFromAccessory(fighter_gobj);
        return;
    }

#endif
    ftPikachuSpecialNProcAccessoryVanilla(fighter_gobj);
}

// 0x80151C14
void ftPikachuSpecialNProcMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ftPikachuSpecialNSwitchStatusAir);
}

// 0x80151C38
void ftPikachuSpecialAirNProcMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ftPikachuSpecialAirNSwitchStatusGround);
}

// 0x80151C5C
void ftPikachuSpecialAirNSwitchStatusGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, nFTPikachuStatusSpecialN, fighter_gobj->anim_frame, 1.0F, FTPIKACHU_SPECIALN_STATUS_FLAGS);

#if defined(PORT) && defined(SSB64_NETMENU)
    /* Netplay rollback only: preserve throw latch after spawn. */
    if ((syNetplayRollbackSemanticsActive() != FALSE) && (fighter_gobj->anim_frame < 21.0F) &&
        (syNetRbSnapThunderJoltOwnedByFighter(fighter_gobj) == FALSE))
    {
        fp->motion_vars.flags.flag1 = 0;
    }

#endif
    fp->proc_accessory = ftPikachuSpecialNProcAccessory;
}

// 0x80151CB0
void ftPikachuSpecialNSwitchStatusAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, nFTPikachuStatusSpecialAirN, fighter_gobj->anim_frame, 1.0F, FTPIKACHU_SPECIALN_STATUS_FLAGS);
    ftPhysicsClampAirVelXMax(fp);

#if defined(PORT) && defined(SSB64_NETMENU)
    /* Netplay rollback only: preserve throw latch after spawn. */
    if ((syNetplayRollbackSemanticsActive() != FALSE) && (fighter_gobj->anim_frame < 21.0F) &&
        (syNetRbSnapThunderJoltOwnedByFighter(fighter_gobj) == FALSE))
    {
        fp->motion_vars.flags.flag1 = 0;
    }

#endif
    fp->proc_accessory = ftPikachuSpecialNProcAccessory;
}

// 0x80151D0C
void ftPikachuSpecialNInitStatusVars(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->motion_vars.flags.flag0 = 0;
#if defined(PORT) && defined(SSB64_NETMENU)
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        fp->motion_vars.flags.flag1 = 0;
    }

#endif
    fp->proc_accessory = ftPikachuSpecialNProcAccessory;
}

// 0x80151D24
void ftPikachuSpecialNSetStatus(GObj *fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, nFTPikachuStatusSpecialN, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftPikachuSpecialNInitStatusVars(fighter_gobj);
}

// 0x80151D64
void ftPikachuSpecialAirNSetStatus(GObj *fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, nFTPikachuStatusSpecialAirN, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftPikachuSpecialNInitStatusVars(fighter_gobj);
}
