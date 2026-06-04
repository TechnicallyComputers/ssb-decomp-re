#include <ft/fighter.h>
#include <wp/weapon.h>
#if defined(PORT) && defined(SSB64_NETMENU)
#include <sys/netrollbacksnapshot.h>
#include <sys/netplay_sim_quantize.h>
/*
 * SSB64_NETMENU compile gate: stripped from offline (NETMENU=OFF) builds.
 * Runtime: syNetplayRollbackSemanticsActive() gates active VS / resim only.
 * See CLAUDE.md §7 and docs/netplay_rollback_refactor_contracts.md.
 */
#endif

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

static void ftMarioSpecialNProcAccessoryVanilla(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Vec3f pos;
    s32 fireball_item_id; // 0 = Mario, 1 = Luigi

    if (fp->motion_vars.flags.flag0 != 0)
    {
        fp->motion_vars.flags.flag0 = 0;

        pos.x = 0.0F;
        pos.y = 0.0F;
        pos.z = 0.0F;

        gmCollisionGetFighterPartsWorldPosition(fp->joints[FTMARIO_FIREBALL_SPAWN_JOINT], &pos);

        switch (fp->fkind) // jtbl at 0x8018C630
        {
        case nFTKindMario:
        case nFTKindMMario:
        case nFTKindNMario:
            fireball_item_id = 0;
            break;

        default:
            #if defined (AVOID_UB)
                return; // This prevents the UB by returning from the function if an unwanted character somehow slips through.
            #else
                break; // Undefined behavior here, var is uninitialized, but projectile spawn function still runs
            #endif

        case nFTKindLuigi:
        case nFTKindNLuigi:
            fireball_item_id = 1;
            break;
        }
        wpMarioFireballMakeWeapon(fighter_gobj, &pos, fireball_item_id);
    }
}

// 0x80155E40
void ftMarioSpecialNProcUpdate(GObj *fighter_gobj)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only. */
    /* Netplay rollback only: rollback fireball spawn when vanilla ProcAccessory is skipped. */
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((syNetRbSnapFireballProcAccessoryWillRun(fighter_gobj) == FALSE) ||
            ((fp != NULL) && (fp->proc_accessory == NULL)))
        {
            syNetRbSnapTrySpawnFireballFromAccessory(fighter_gobj);
        }
    }
#endif
    ftAnimEndCheckSetStatus(fighter_gobj, mpCommonSetFighterWaitOrFall);
}

// 0x80155E64
void ftMarioSpecialNProcAccessory(GObj *fighter_gobj)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only. */
    /* Netplay rollback only: snapshot-driven fireball spawn; offline uses vanilla below. */
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        syNetRbSnapTrySpawnFireballFromAccessory(fighter_gobj);
        return;
    }
#endif
    ftMarioSpecialNProcAccessoryVanilla(fighter_gobj);
}

// 0x80155F04
void ftMarioSpecialNProcMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ftMarioSpecialNSwitchStatusAir);
}

// 0x80155F28
void ftMarioSpecialAirNProcMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ftMarioSpecialAirNSwitchStatusGround);
}

// 0x80155F4C
void ftMarioSpecialAirNSwitchStatusGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);

    ftMainSetStatus(fighter_gobj, nFTMarioStatusSpecialN, fighter_gobj->anim_frame, 1.0F, FTSTATUS_PRESERVE_COLANIM);

    fp->proc_accessory = ftMarioSpecialNProcAccessory;
}

// 0x80155FA0
void ftMarioSpecialNSwitchStatusAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, nFTMarioStatusSpecialAirN, fighter_gobj->anim_frame, 1.0F, FTSTATUS_PRESERVE_COLANIM);
    ftPhysicsClampAirVelXMax(fp);

    fp->proc_accessory = ftMarioSpecialNProcAccessory;
}

// 0x80155FFC
void ftMarioSpecialNInitStatusVars(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->motion_vars.flags.flag0 = FALSE;
#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only. */
    /* Netplay rollback only: clear motion flag1 during active rollback session. */
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        fp->motion_vars.flags.flag1 = 0;
    }
#endif
    fp->proc_accessory = ftMarioSpecialNProcAccessory;
}

// 0x80156014
void ftMarioSpecialNSetStatus(GObj *fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, nFTMarioStatusSpecialN, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftMarioSpecialNInitStatusVars(fighter_gobj);
}

// 0x80156054
void ftMarioSpecialAirNSetStatus(GObj *fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, nFTMarioStatusSpecialAirN, 0.0F, 1.0F, FTSTATUS_PRESERVE_FASTFALL);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ftMarioSpecialNInitStatusVars(fighter_gobj);
}
