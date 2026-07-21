#include <ft/fighter.h>
#include <sc/scene.h>
#ifdef PORT
#include <ft/ftmanager.h>
#include <if/ifcommon.h>
#endif
#if defined(PORT) && defined(SSB64_NETMENU)
#include <sys/netplay_rebirth_gate.h>
#include <sys/netplay_sim_quantize.h>
#include <sys/netrollbacksnapshot.h>
#endif

// // // // // // // // // // // //
//                               //
//       INITIALIZED DATA        //
//                               //
// // // // // // // // // // // //

// 0x80188490
f32 dFTCommonRebirthOffsetsX[/* */] = { 0.0F, -1000.0F, 1000.0F, -2000.0F };

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

// 0x8013CF60
void ftCommonRebirthDownSetStatus(GObj *this_gobj)
{
    FTStruct *this_fp = ftGetStruct(this_gobj);
    FTDesc rebirth_vars = dFTManagerDefaultFighterDesc;
    GObj *other_gobj;
    FTStruct *other_fp;
    s32 halo_number;
    s32 halo_mapobj;
    Vec3f halo_spawn_pos;

    rebirth_vars.lr = this_fp->lr;
    rebirth_vars.damage = 0;

    mpCollisionGetMapObjIDsKind(nMPMapObjKindRebirth, &halo_mapobj);
    mpCollisionGetMapObjPositionID(halo_mapobj, &halo_spawn_pos);

    halo_number = 0;

loop: // This makes no sense
    other_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];

    while (other_gobj != NULL)
    {
        if (other_gobj != this_gobj)
        {
            other_fp = ftGetStruct(other_gobj);

            if
            (
                (other_fp->status_id >= nFTCommonStatusRebirthDown) && 
                (other_fp->status_id <= nFTCommonStatusRebirthWait)
            )
            {
                if (halo_number == ftStatusVarsRebirth(other_fp)->halo_number)
                {
                    halo_number++;

                    goto loop;
                }
            }
            else goto next_gobj;
        }
    next_gobj:
        other_gobj = other_gobj->link_next;
    }
    rebirth_vars.pos.x = dFTCommonRebirthOffsetsX[halo_number] + halo_spawn_pos.x;
    rebirth_vars.pos.y = gMPCollisionGroundData->map_bound_top;
    rebirth_vars.pos.z = 0.0F;
#ifdef PORT
    gFTManagerInitFighterSkipFloorProject = TRUE;
#endif

    ftManagerInitFighter(this_gobj, &rebirth_vars);
#ifdef PORT
    gFTManagerInitFighterSkipFloorProject = FALSE;
    {
        Vec3f *root_translate = &DObjGetStruct(this_gobj)->translate.vec.f;

        *root_translate = rebirth_vars.pos;
        this_fp->coll_data.pos_prev = rebirth_vars.pos;
        this_fp->coll_data.pos_diff.x = 0.0F;
        this_fp->coll_data.pos_diff.y = 0.0F;
        this_fp->coll_data.pos_diff.z = 0.0F;
    }
#endif
    ifCommonPlayerDamageStopBreakAnim(this_fp);
    mpCommonSetFighterGround(this_fp);

    this_fp->coll_data.floor_line_id = -2;
    this_fp->coll_data.floor_flags = MAP_VERTEX_COLL_PASS;
    this_fp->coll_data.floor_angle.y = 1.0F;
    this_fp->coll_data.floor_angle.x = 0.0F;
    this_fp->coll_data.floor_angle.z = 0.0F;

    ftMainSetStatus(this_gobj, nFTCommonStatusRebirthDown, 100.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(this_gobj);
    ftPhysicsStopVelAll(this_gobj);

    ftStatusVarsRebirth(this_fp)->halo_lower_wait = FTCOMMON_REBIRTH_HALO_LOWER_WAIT;
    ftStatusVarsRebirth(this_fp)->halo_despawn_wait = FTCOMMON_REBIRTH_HALO_DESPAWN_WAIT;
#ifdef PORT
    ftStatusVarsRebirth(this_fp)->pos = rebirth_vars.pos;
#else
    ftStatusVarsRebirth(this_fp)->pos = DObjGetStruct(this_gobj)->translate.vec.f;
#endif
    ftStatusVarsRebirth(this_fp)->halo_offset.x = dFTCommonRebirthOffsetsX[halo_number] + halo_spawn_pos.x;
    ftStatusVarsRebirth(this_fp)->halo_offset.y = halo_spawn_pos.y;
    ftStatusVarsRebirth(this_fp)->halo_offset.z = 0.0F;

    this_fp->is_menu_ignore = TRUE;
    this_fp->is_ghost = TRUE;
    this_fp->is_shadow_hide = TRUE;
    this_fp->is_rebirth = TRUE;
    this_fp->camera_mode = nFTCameraModeGhost;

    ftStatusVarsRebirth(this_fp)->halo_number = halo_number;

    this_fp->camera_zoom_range = 0.6F;

#if defined(PORT) && defined(SSB64_NETMENU)
    syNetRbSnapReclaimStaleEffectShellsForRebirthHalo(NULL, NULL);
#endif
    if (efManagerRebirthHaloMakeEffect(this_gobj, this_fp->attr->halo_size) != NULL)
    {
        this_fp->is_effect_attach = TRUE;
    }
#if defined(PORT) && defined(SSB64_NETMENU)
    syNetplayCanonicalizeRebirthFighterMapPose(this_gobj);
#endif
    ftParamCheckSetFighterColAnimID(this_gobj, nGMColAnimFighterRebirth, 0);
    ftParamSetPlayerTagWait(this_gobj, 1);
#if defined(PORT) && defined(SSB64_NETMENU)
    syNetplayRebirthGateLogRebirthDownSetStatus(this_gobj, this_fp, halo_number);
#endif
}

// 0x8013D1D4
void ftCommonRebirthCommonUpdateHaloWait(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (ftStatusVarsRebirth(fp)->halo_despawn_wait != 0)
    {
        ftStatusVarsRebirth(fp)->halo_despawn_wait--;
    }
    if (ftStatusVarsRebirth(fp)->halo_lower_wait != 0)
    {
        ftStatusVarsRebirth(fp)->halo_lower_wait--;
    }
}

// 0x8013D200
void ftCommonRebirthDownProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftCommonRebirthCommonUpdateHaloWait(fighter_gobj);

    if (ftStatusVarsRebirth(fp)->halo_despawn_wait == (FTCOMMON_REBIRTH_HALO_DESPAWN_WAIT - FTCOMMON_REBIRTH_HALO_UNK_WAIT))
    {
        fp->camera_mode = nFTCameraModeDefault;
    }
    if (ftStatusVarsRebirth(fp)->halo_despawn_wait == (FTCOMMON_REBIRTH_HALO_DESPAWN_WAIT - FTCOMMON_REBIRTH_HALO_STAND_WAIT))
    {
        ftCommonRebirthStandSetStatus(fighter_gobj);
    }
}

// 0x8013D264
void ftCommonRebirthCommonProcMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    DObjGetStruct(fighter_gobj)->translate.vec.f.y = (((ftStatusVarsRebirth(fp)->pos.y - ftStatusVarsRebirth(fp)->halo_offset.y) / 8100.0F) *
                                               SQUARE(ftStatusVarsRebirth(fp)->halo_lower_wait)) + ftStatusVarsRebirth(fp)->halo_offset.y;
#if defined(PORT) && defined(SSB64_NETMENU)
    syNetplayCanonicalizeRebirthFighterMapPose(fighter_gobj);
#endif
}

// 0x8013D2AC
void ftCommonRebirthStandProcUpdate(GObj *fighter_gobj)
{
    ftCommonRebirthCommonUpdateHaloWait(fighter_gobj);
    ftAnimEndCheckSetStatus(fighter_gobj, ftCommonRebirthWaitSetStatus);
}

// 0x8013D2DC
void ftCommonRebirthStandSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTCommonStatusRebirthStand, 0.0F, 1.0F, (FTSTATUS_PRESERVE_PLAYERTAG | FTSTATUS_PRESERVE_EFFECT | FTSTATUS_PRESERVE_COLANIM));
    ftMainPlayAnimEventsAll(fighter_gobj);

    fp->is_menu_ignore = TRUE;
    fp->is_ghost = TRUE;
    fp->is_shadow_hide = TRUE;
    fp->is_rebirth = TRUE;

    fp->camera_zoom_range = 0.6F;
}

// 0x8013D358
void ftCommonRebirthWaitProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftCommonRebirthCommonUpdateHaloWait(fighter_gobj);

    if (ftStatusVarsRebirth(fp)->halo_despawn_wait == 0)
    {
#if defined(PORT) && defined(SSB64_NETMENU)
        /* SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only. */
        syNetplayRebirthGateLogLeaveStick(fighter_gobj, fp, "halo_timer", fp->status_id);
#endif
        ftParamSetTimedHitStatusInvincible(fp, FTCOMMON_REBIRTH_INVINCIBLE_FRAMES);
        ftCommonFallSetStatus(fighter_gobj);
    }
}

// 0x8013D3A4
void ftCommonRebirthWaitProcInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
#if defined(PORT) && defined(SSB64_NETMENU)
    s32 status_before = fp->status_id;
#endif

    if (ftCommonGroundCheckInterrupt(fighter_gobj))
    {
#if defined(PORT) && defined(SSB64_NETMENU)
        /* SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only. */
        /* Stick/ground leave — soak 1174892281 RebirthWait→Fall stick-Y peer fork. */
        syNetplayRebirthGateLogLeaveStick(fighter_gobj, fp, "ground_interrupt", status_before);
#endif
        ftParamSetTimedHitStatusInvincible(fp, FTCOMMON_REBIRTH_INVINCIBLE_FRAMES);
    }
}

// 0x8013D518
void ftCommonRebirthWaitSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTCommonStatusRebirthWait, 0.0F, 1.0F, (FTSTATUS_PRESERVE_PLAYERTAG | FTSTATUS_PRESERVE_EFFECT | FTSTATUS_PRESERVE_COLANIM));

    fp->is_ghost = TRUE;
    fp->is_shadow_hide = TRUE;
    fp->is_rebirth = TRUE;

    fp->camera_zoom_range = 0.6F;
}
