#include <gr/ground.h>
#include <ft/fighter.h>
#include <wp/weapon.h>
#include <sc/scene.h>
#include <reloc_data.h>
#include <string.h>
#include <sys/objanim.h>
#ifdef PORT
#include "port_scene_heap.h"
extern void *func_800269C0_275C0(u16 id);
#endif
#if defined(PORT) && defined(SSB64_NETMENU)
extern sb32 syNetplayRollbackSemanticsActive(void);
extern u32 syNetInputGetTick(void);
extern void port_log(const char *fmt, ...);
extern char *getenv(const char *name);
extern int atoi(const char *s);
#include <sys/netplay_sim_quantize.h>
#include <mp/map.h>
#include <mp/mpcollision.h>

/* Env-gated: SSB64_NETPLAY_SECTOR_ARWING_CARRY_DIAG=1. See
 * docs/bugs/netplay_sector_z_arwing_carry_speed_fork_2026-07-01.md. */
static sb32 grSectorArwingCarryDiagEnabled(void)
{
	static int s_env_cache = -999;
	const char *e;

	if (s_env_cache != -999)
	{
		return (s_env_cache != 0) ? TRUE : FALSE;
	}
	e = getenv("SSB64_NETPLAY_SECTOR_ARWING_CARRY_DIAG");
	s_env_cache = ((e != NULL) && (e[0] != '\0') && (atoi(e) != 0)) ? 1 : 0;
	return (s_env_cache != 0) ? TRUE : FALSE;
}
#endif

// // // // // // // // // // // //
//                               //
//       EXTERNAL VARIABLES      //
//                               //
// // // // // // // // // // // //

extern void syInterpQuad(void*, void*, f32);
extern void syInterpCubic(void*, void*, f32);

// // // // // // // // // // // //
//                               //
//       INITIALIZED DATA        //
//                               //
// // // // // // // // // // // //

// 0x8012E940
intptr_t dGRSectorArwingSectorDescs[/* */] =
{
#ifdef PORT
    llGRSectorMapArwing0SectorDesc,
    llGRSectorMapArwing1SectorDesc,
    llGRSectorMapArwing2SectorDesc,
    llGRSectorMapArwing3SectorDesc,
    llGRSectorMapArwing4SectorDesc,
    llGRSectorMapArwing5SectorDesc,
    llGRSectorMapArwing6SectorDesc,
    llGRSectorMapArwing7SectorDesc
#else
    &llGRSectorMapArwing0SectorDesc,
    &llGRSectorMapArwing1SectorDesc,
    &llGRSectorMapArwing2SectorDesc,
    &llGRSectorMapArwing3SectorDesc,
    &llGRSectorMapArwing4SectorDesc,
    &llGRSectorMapArwing5SectorDesc,
    &llGRSectorMapArwing6SectorDesc,
    &llGRSectorMapArwing7SectorDesc
#endif
};

// 0x8012E960
intptr_t dGRSectorArwingAnimJoints[/* */] =
{
#ifdef PORT
    llGRSectorMapArwing0AnimJoint,
    llGRSectorMapArwing1AnimJoint,
    llGRSectorMapArwing2AnimJoint,
    llGRSectorMapArwing3AnimJoint,
    llGRSectorMapArwing4AnimJoint,
    llGRSectorMapArwing5AnimJoint
#else
    &llGRSectorMapArwing0AnimJoint,
    &llGRSectorMapArwing1AnimJoint,
    &llGRSectorMapArwing2AnimJoint,
    &llGRSectorMapArwing3AnimJoint,
    &llGRSectorMapArwing4AnimJoint,
    &llGRSectorMapArwing5AnimJoint
#endif
};

// 0x8012E978
s16 dGRSectorArwingMapPositionsX[/* */] =
{
    -3000,
        0,
     9000
}; // Arwing collision vertex / spawn positions? There should be another 0x0000, but the next array is u8, does it still get padded?

// 0x8012E980
u8 dGRSectorArwingLaserCounts[/* */] =
{
    0x02,
    0x02,
    0x02,
    0x02,
    0x02,
    0x00,
    0x00,
    0x00
};

// 0x8012E988
u8 dGRSectorArwingPilotIDs[/* */] =
{
    0x01,
    0x01,
    0x02,
    0x03,
    0x04,
    0x04,
    0x05,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02,
    0x03,
    0x04,
    0x04,
    0x05,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01,
    0x03,
    0x04,
    0x04,
    0x05,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01,
    0x02,
    0x04,
    0x04,
    0x05,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01,
    0x02,
    0x03,
    0x05,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01,
    0x02,
    0x03,
    0x04,
    0x04,
    0x00
};

// 0x8012E9C0
u8 dGRSectorArwingPilotWaitTimers[/* */][2] =
{
    {  0,  7 },
    {  7,  9 },
    { 16, 10 },
    { 26, 10 },
    { 36,  9 },
    { 45, 10 }
};

// 0x8012E9CC
DObjTransformTypes dGRSectorArwingTransformKinds[/* */] =
{
#if defined(REGION_US)
    { 0x53, nGCMatrixKindNull, 0x00 },
#else
    { 0x52, nGCMatrixKindNull, 0x00 },
#endif
    { nGCMatrixKindTraRotRpyR, nGCMatrixKindNull, 0x00 },
    { nGCMatrixKindTra, 0x2C, 0x01 },
    { nGCMatrixKindTra, 0x2C, 0x01 },
    { nGCMatrixKindTra, 0x2C, 0x00 },
    { nGCMatrixKindTra, 0x2C, 0x00 },
    { nGCMatrixKindTra, nGCMatrixKindNull, 0x01 },
    { nGCMatrixKindTra, nGCMatrixKindNull, 0x00 },
    { nGCMatrixKindTra, 0x2C, 0x00 },
    { nGCMatrixKindTra, nGCMatrixKindNull, 0x00 },
    { nGCMatrixKindTra, 0x2C, 0x01 },
    { nGCMatrixKindNull, nGCMatrixKindNull, 0x00 } // This might just be padding
};

// 0x8012E9F0
WPDesc dGRSectorArwingWeaponLaser2DWeaponDesc =
{
    0,                                          // Render flags?
    nWPKindArwingLaser2D,                       // Weapon Kind
    &gGRCommonStruct.sector.weapon_head,        // Pointer to character's loaded files?
#ifdef PORT
    llGRSectorMapArwingLaser2DWeaponAttributes,    // Offset of weapon attributes in loaded files
#else
    &llGRSectorMapArwingLaser2DWeaponAttributes,    // Offset of weapon attributes in loaded files
#endif
    
    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyR,                // Main matrix transformations
        nGCMatrixKindNull,                      // Secondary matrix transformations?
        0                                       // ???
    },

    NULL,                                       // Proc Update
    grSectorArwingWeaponLaser2DProcMap,         // Proc Map
    grSectorArwingWeaponLaser2DProcHit,         // Proc Hit
    grSectorArwingWeaponLaser2DProcHit,         // Proc Shield
    grSectorArwingWeaponLaser2DProcHop,         // Proc Hop
    grSectorArwingWeaponLaser2DProcHit,         // Proc Set-Off
    grSectorArwingWeaponLaser2DProcReflector,   // Proc Reflector
    grSectorArwingWeaponLaser2DProcHit          // Proc Absorb
};

// 0x8012EA24
WPDesc dGRSectorArwingWeaponLaser3DWeaponDesc =
{
    0,                                          // Render flags?
    nWPKindArwingLaser3D,                       // Weapon Kind
    &gGRCommonStruct.sector.weapon_head,        // Pointer to character's loaded files?
#ifdef PORT
    llGRSectorMapArwingLaser3DWeaponAttributes,    // Offset of weapon attributes in loaded files
#else
    &llGRSectorMapArwingLaser3DWeaponAttributes,    // Offset of weapon attributes in loaded files
#endif
    
    // DObj transformation struct
    {
        nGCMatrixKindTraRotRpyR,                // Main matrix transformations
        nGCMatrixKindNull,                      // Secondary matrix transformations?
        0                                       // ???
    },

    NULL,                                       // Proc Update
    grSectorArwingWeaponLaser3DProcMap,         // Proc Map
    grSectorArwingWeaponLaser3DProcHit,         // Proc Hit
    grSectorArwingWeaponLaser3DProcHit,         // Proc Shield
    grSectorArwingWeaponLaser3DProcHit,         // Proc Hop
    grSectorArwingWeaponLaser3DProcHit,         // Proc Set-Off
    grSectorArwingWeaponLaser3DProcHit,         // Proc Reflector
    grSectorArwingWeaponLaser3DProcAbsorb       // Proc Absorb
};

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

// 0x80106730
void func_ovl2_80106730(DObj *arg0, Vec3f *vec1, Vec3f *vec2, Vec3f *vec3)
{
    DObj *sp54 = gGRCommonStruct.sector.map_dobjs[11];
    AObj *aobj = arg0->aobj;
    f32 vlen;

    while (aobj != NULL)
    {
        if ((aobj->kind != nGCAnimKindNone) && !(arg0->parent_gobj->flags & GOBJ_FLAG_NOANIM) && (aobj->track == nGCAnimTrackTraI))
        {
            vlen = gcGetAObjValue(aobj);

            if (vlen < 0.0F)
            {
                vlen = 0.0F;
            }
            else if (vlen > 1.0F)
            {
                vlen = 1.0F;
            }
            syInterpQuad(vec1, aobj->interpolate, vlen);
        }
        aobj = aobj->next;
    }
    if ((sp54->anim_wait != AOBJ_ANIM_NULL) && (gGRCommonStruct.sector.arwing_laser_count == 0))
    {
        aobj = sp54->aobj;

        while (aobj != NULL)
        {
            if ((aobj->kind != nGCAnimKindNone) && !(arg0->parent_gobj->flags & GOBJ_FLAG_NOANIM) && (aobj->track == nGCAnimTrackTraI))
            {
                syInterpCubic(vec3, aobj->interpolate, vlen);
            }
            aobj = aobj->next;
        }
        syVectorNorm3D(vec3);
    }
    lbCommonCross3D(vec3, vec1, vec2);
    lbCommonCross3D(vec1, vec2, vec3);
    
    syVectorNorm3D(vec1);
    syVectorNorm3D(vec2);
    syVectorNorm3D(vec3);
}

// 0x80106904
sb32 grSectorArwingLaser3DFuncMatrix(Mtx *mtx, DObj *dobj, Gfx **dls)
{
    f32 sx;
    Vec3f sp80;
    Vec3f sp74;
    Vec3f sp68;
    Mtx44f f;
    f32 tx;
    f32 ty;
    f32 tz;

    tx = dobj->translate.vec.f.x;
    ty = dobj->translate.vec.f.y;
    tz = dobj->translate.vec.f.z;

    sp80.x = -1.0F;
    sp80.y = 0.0F;
    sp80.z = 0.0F;
    sp68.x = 0.0F;
    sp68.y = 1.0F;
    sp68.z = 0.0F;

    if (gGRCommonStruct.sector.arwing_laser_count == 2)
    {
        sp74.x = sp74.y = 0.0F;
        sp74.z = 1;
    }
    else func_ovl2_80106730(dobj, &sp80, &sp74, &sp68);

    f[0][0] = sp74.x; // sp28
    f[0][1] = sp74.y; // sp2C
    f[0][2] = sp74.z; // sp30
    f[1][0] = sp68.x; // sp38
    f[1][1] = sp68.y; // sp3C
    f[1][2] = sp68.z; // sp40
    f[2][0] = sp80.x; // sp48
    f[2][1] = sp80.y; // sp4C
    f[2][2] = sp80.z; // sp50

    f[0][3] = f[1][3] = f[2][3] = 0.0F;                     // sp34, sp44, sp54

    f[3][0] = tx + gGRCommonStruct.sector.arwing_target_x;  // sp58
    f[3][1] = ty;                                           // sp5C
    f[3][2] = tz;                                           // sp60

    f[3][3] = 1.0F;                                         // sp64

    guMtxF2L(f, mtx);

    return 0;
}

// 0x80106A40
void grSectorArwingAddAnim(DObj *dobj, AObjEvent32 *anim_joint, f32 unused)
{
    if (anim_joint != NULL)
    {
        gcAddDObjAnimJoint(dobj, anim_joint, 0.0F);

        dobj->is_anim_root = FALSE;

        gcParseDObjAnimJoint(dobj);
        gcPlayDObjAnimJoint(dobj);
    }
    else
    {
        dobj->anim_wait = AOBJ_ANIM_NULL;
        dobj->is_anim_root = FALSE;
    }
}

// 0x80106A98
void grSectorArwingUpdateSleep(void)
{
    if (gSCManagerBattleState->game_status != nSCBattleGameStatusWait)
    {
        gGRCommonStruct.sector.arwing_status = nGRSectorArwingStatusWait;
    }
}

// 0x80106AC0
void grSectorArwingUpdateWait(void)
{
    s32 random;

    if (gGRCommonStruct.sector.arwing_appear_timer != 0)
    {
        gGRCommonStruct.sector.arwing_appear_timer--;
    }
    else
    {
        gGRCommonStruct.sector.arwing_target_x = 0.0F;

        if (gGRCommonStruct.sector.arwing_type_cycle != 0)
        {
            random = syUtilsRandIntRange(5);

            if (random == 4)
            {
                gGRCommonStruct.sector.arwing_target_x = dGRSectorArwingMapPositionsX[syUtilsRandIntRange(ARRAY_COUNT(dGRSectorArwingMapPositionsX))];
            }
            gGRCommonStruct.sector.arwing_type_cycle--;
            gGRCommonStruct.sector.arwing_state_timer = syUtilsRandIntRange(540) + 180;
            gGRCommonStruct.sector.arwing_pilot_curr = -1;
        }
        else
        {
            random = syUtilsRandIntRange(3) + 5;

            gGRCommonStruct.sector.unk_sector_0x4C = ((syUtilsRandUShort() % 2) != 0) ? random - 5 : -1;

            gGRCommonStruct.sector.arwing_type_cycle = 3;
            gGRCommonStruct.sector.arwing_pilot_curr = -2;
        }
        gGRCommonStruct.sector.arwing_flight_pattern = random;
        gGRCommonStruct.sector.arwing_last_flight_pattern = (s8)random;
        gGRCommonStruct.sector.arwing_status = 2;
        gGRCommonStruct.sector.unk_sector_0x4E = 0x3C;

        gGRCommonStruct.sector.map_dobjs[1]->translate.vec.f.x =
        gGRCommonStruct.sector.map_dobjs[1]->translate.vec.f.y =
        gGRCommonStruct.sector.map_dobjs[1]->translate.vec.f.z = 0.0F;

        gGRCommonStruct.sector.map_dobjs[1]->rotate.vec.f.x =
        gGRCommonStruct.sector.map_dobjs[1]->rotate.vec.f.y =
        gGRCommonStruct.sector.map_dobjs[1]->rotate.vec.f.z = 0.0F;

        gGRCommonStruct.sector.is_arwing_line_collision = FALSE;
        gGRCommonStruct.sector.is_arwing_line_active = TRUE;
        gGRCommonStruct.sector.is_arwing_z_collision = FALSE;
        gGRCommonStruct.sector.arwing_laser_ammo = 0;

        func_800269C0_275C0(nSYAudioFGMSectorAmbient1);
#if defined(PORT) && defined(SSB64_NETMENU)
        gGRCommonStruct.sector.arwing_target_x = syNetplayQuantizeF32(gGRCommonStruct.sector.arwing_target_x);

#endif
    }
}

// 0x80106C28
void grSectorArwingDecideZNear(void)
{
    if (ABSF(gGRCommonStruct.sector.map_dobjs[0]->translate.vec.f.z) < 200.0F)
    {
        gGRCommonStruct.sector.is_arwing_z_near = TRUE;
    }
    else gGRCommonStruct.sector.is_arwing_z_near = FALSE;
}

// 0x80106C88
void func_ovl2_80106C88(void)
{
    switch (gGRCommonStruct.sector.arwing_state_timer)
    {
    case 0:
        gGRCommonStruct.sector.is_arwing_line_active = FALSE;
        break;

    case 88:
        gGRCommonStruct.sector.is_arwing_line_active = TRUE;
        break;
    }
}

// 0x80106CC4
void func_ovl2_80106CC4(void)
{
    switch (gGRCommonStruct.sector.arwing_state_timer)
    {
    case 0:
        gGRCommonStruct.sector.is_arwing_line_active = FALSE;
        break;

    case 178:
        gGRCommonStruct.sector.is_arwing_line_active = TRUE;
        break;
    }
}

// 0x80106D00
void func_ovl2_80106D00(void)
{
    if (gGRCommonStruct.sector.arwing_state_timer == 0)
    {
        gGRCommonStruct.sector.map_dobjs[7]->anim_wait = AOBJ_ANIM_NULL;
        gGRCommonStruct.sector.map_dobjs[7]->flags = DOBJ_FLAG_NONE;
        gGRCommonStruct.sector.map_dobjs[9]->anim_wait = AOBJ_ANIM_NULL;
        gGRCommonStruct.sector.map_dobjs[9]->flags = DOBJ_FLAG_HIDDEN;

#ifdef PORT
        grSectorArwingAddAnim(gGRCommonStruct.sector.map_dobjs[8], lbRelocGetFileData(AObjEvent32*, gGRCommonStruct.sector.map_file, llFoxSpecial3_2EB4_AnimJoint), 0.0F);
#else
        grSectorArwingAddAnim(gGRCommonStruct.sector.map_dobjs[8], lbRelocGetFileData(AObjEvent32*, gGRCommonStruct.sector.map_file, &llFoxSpecial3_2EB4_AnimJoint), 0.0F);
#endif
    }
    else if (gGRCommonStruct.sector.map_dobjs[8]->anim_wait == AOBJ_ANIM_NULL)
    {
        gGRCommonStruct.sector.map_dobjs[7]->flags = DOBJ_FLAG_HIDDEN;
        gGRCommonStruct.sector.map_dobjs[9]->flags = DOBJ_FLAG_NONE;
    }
    if (gGRCommonStruct.sector.map_dobjs[1]->anim_wait == AOBJ_ANIM_NULL)
    {
        gGRCommonStruct.sector.map_dobjs[0]->anim_wait = AOBJ_ANIM_NULL;
    }
}

// 0x80106DD8
void func_ovl2_80106DD8(void)
{
    if (gGRCommonStruct.sector.arwing_pilot_curr != -2)
    {
        if (gGRCommonStruct.sector.arwing_pilot_curr >= 0)
        {
            switch (gGRCommonStruct.sector.arwing_pilot_curr)
            {
            case 1:
                func_ovl2_80106C88();
                break;

            case 4:
                func_ovl2_80106CC4();
                break;

            case 5:
                func_ovl2_80106D00();
                break;
            }
            if (gGRCommonStruct.sector.map_dobjs[1]->anim_wait == AOBJ_ANIM_NULL)
            {
                gGRCommonStruct.sector.arwing_pilot_curr = -1;
                gGRCommonStruct.sector.arwing_state_timer = 120;
            }
            else gGRCommonStruct.sector.arwing_state_timer++;
        }
        else
        {
            gGRCommonStruct.sector.arwing_state_timer--;

            if (gGRCommonStruct.sector.arwing_state_timer == 0)
            {
                u8 *random = &dGRSectorArwingPilotWaitTimers[gGRCommonStruct.sector.arwing_pilot_prev][0];
                s32 pilot_id = dGRSectorArwingPilotIDs[random[0] + syUtilsRandIntRange(random[1])];

                if (pilot_id != 0)
                {
                    grSectorArwingAddAnim(gGRCommonStruct.sector.map_dobjs[1], (AObjEvent32*) ((intptr_t)dGRSectorArwingAnimJoints[pilot_id] + (uintptr_t)gGRCommonStruct.sector.map_head), 0.0F);
                }
                gGRCommonStruct.sector.arwing_pilot_prev = gGRCommonStruct.sector.arwing_pilot_curr = pilot_id;
            }
        }
    }
}

// 0x80106F2
s32 grSectorArwingPrepareLaserCount(void)
{
    if (syUtilsRandIntRange(3) >= 3)
    {
        return 2;
    }
    else return 4;
}

// 0x80106F5C
s32 grSectorArwingGetLaserAmmoCount(void)
{
    GObj *fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
    f32 pos_x = gGRCommonStruct.sector.map_dobjs[0]->translate.vec.f.x + gGRCommonStruct.sector.arwing_target_x;
    f32 pos_y = gGRCommonStruct.sector.map_dobjs[0]->translate.vec.f.y + gGRCommonStruct.sector.map_dobjs[1]->translate.vec.f.y;

    while (fighter_gobj != NULL)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if (gGRCommonStruct.sector.arwing_laser_count == 2)
        {
            DObj *joint = fp->joints[nFTPartsJointTopN];

            if (joint->translate.vec.f.x < pos_x)
            {
                if ((joint->translate.vec.f.y < (pos_y + 300.0F)) && (joint->translate.vec.f.y > (pos_y + -500.0F)))
                {
                    return grSectorArwingPrepareLaserCount();
                }
            }
        }
        fighter_gobj = fighter_gobj->link_next;
    }
    return 0;
}

// 0x80107030
sb32 grSectorArwingWeaponLaser2DProcMap(GObj *weapon_gobj)
{
    if (wpMapTestAllCheckCollEnd(weapon_gobj) != FALSE)
    {
        efManagerDustExpandSmallMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f, 1.0F);

        return TRUE;
    }
    else return FALSE;
}

// 0x80107074
sb32 grSectorArwingWeaponLaser2DProcHit(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    efManagerImpactShockMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f, wp->attack_coll.damage);

    return TRUE;
}

// 0x801070A4
void func_ovl2_801070A4(Vec3f *rotate, Vec3f *direction, Vec3f *vec3, Vec3f *vec4)
{
#ifdef PORT
    /* Port: same fragility as gmcollision.c func_ovl2_800EDA0C — vec3 is
     * built from chained lbCommonCross3D + syVectorNorm3D so its components
     * compose to ~0.99999 on modern toolchains. Exact equality misses the
     * gimbal-lock branch, the general extraction runs atan2(noise, noise),
     * and Sector Z weapon orientation comes out wrong by up to 90°.
     * Same threshold (~0.81° band around pure ±90° yaw). See
     * docs/bugs/grab_pose_eulerextract_gimbal_2026-05-23.md. */
    if ((vec3->z <= -0.9999F) || (vec3->z >= 0.9999F))
#else
    if ((vec3->z == -1.0F) || (vec3->z == 1.0F))
#endif
    {
#ifdef PORT
        if (vec3->z <= -0.9999F)
#else
        if (vec3->z == -1.0F)
#endif
        {
            rotate->y = F_CST_DTOR32(90.0F);
            rotate->x = syUtilsArcTan2(vec4->x, vec4->y);
        }
        else
        {
            rotate->y = F_CST_DTOR32(-90.0F);
            rotate->x = syUtilsArcTan2(-vec4->x, vec4->y);
        }
        rotate->z = 0.0F;
    }
    else
    {
        rotate->y = syUtilsArcSin(-vec3->z);
        rotate->x = syUtilsArcTan2(vec4->z, direction->z);
        rotate->z = syUtilsArcTan2(vec3->y, vec3->x);
    }
}

// 0x8010719C
void func_ovl2_8010719C(Vec3f *vel, Vec3f *rotate)
{
    Vec3f sp2C;
    Vec3f sp20;
    f32 unused;
    f32 rot_z;

    sp20.x = 0.0F;

    rot_z = gGRCommonStruct.sector.map_dobjs[1]->rotate.vec.f.z + F_CST_DTOR32(90.0F);

    sp20.y = __sinf(rot_z);
    sp20.z = __cosf(rot_z);

    lbCommonCross3D(&sp20, vel, &sp2C);
    lbCommonCross3D(vel, &sp2C, &sp20);
    syVectorNorm3D(&sp2C);
    syVectorNorm3D(&sp20);
    func_ovl2_801070A4(rotate, vel, &sp2C, &sp20);
}

// 0x80107238
sb32 grSectorArwingWeaponLaser2DProcHop(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    Vec3f vel;

    syVectorRotateAbout3D(&wp->physics.vel_air, &wp->shield_collide_dir, wp->shield_collide_angle * 2);

    vel = wp->physics.vel_air;

    syVectorNorm3D(&vel);
    func_ovl2_8010719C(&vel, &DObjGetStruct(weapon_gobj)->rotate.vec.f);

    return FALSE;
}

// 0x801072C0
sb32 grSectorArwingWeaponLaser2DProcReflector(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    FTStruct *fp = ftGetStruct(wp->owner_gobj);
    Vec3f vel;

    wpMainReflectorSetLR(wp, fp);

    vel = wp->physics.vel_air;

    syVectorNorm3D(&vel);
    func_ovl2_8010719C(&vel, &DObjGetStruct(weapon_gobj)->rotate.vec.f);

    return FALSE;
}

// 0x80107330
void grSectorArwingWeaponLaser2DMakeWeapon(void)
{
    GObj *weapon_gobj;
    WPStruct *wp;
    Vec3f sp54;
    Vec3f sp48;
    Vec3f pos;
    Vec3f rotate;
    Vec3f vel;
    f32 zero = 0.0F;

    sp54.x = gGRCommonStruct.sector.map_dobjs[0]->translate.vec.f.x + gGRCommonStruct.sector.arwing_target_x;
    sp54.y = gGRCommonStruct.sector.map_dobjs[0]->translate.vec.f.y + gGRCommonStruct.sector.map_dobjs[1]->translate.vec.f.y;

    sp48 = gGRCommonStruct.sector.map_dobjs[2]->translate.vec.f;

    syVectorRotate3D(&sp48, SYVECTOR_AXIS_Z, gGRCommonStruct.sector.map_dobjs[1]->rotate.vec.f.z);

    pos.x = (sp54.x - sp48.z) - 566.0F;
    pos.y = sp54.y + sp48.y;
    pos.z = zero + sp48.x;

    weapon_gobj = wpManagerMakeWeapon(NULL, &dGRSectorArwingWeaponLaser2DWeaponDesc, &pos, WEAPON_FLAG_PARENT_GROUND);

    if (weapon_gobj != NULL)
    {
        wp = wpGetStruct(weapon_gobj);

        wp->physics.vel_air.x = -230.0F;

        vel.y = vel.z = 0.0F;
        vel.x = -1.0F;

        func_ovl2_8010719C(&vel, &rotate);

        DObjGetStruct(weapon_gobj)->rotate.vec.f = rotate;

        sp48 = gGRCommonStruct.sector.map_dobjs[3]->translate.vec.f;

        syVectorRotate3D(&sp48, 4, gGRCommonStruct.sector.map_dobjs[1]->rotate.vec.f.z);

        pos.x = (sp54.x - sp48.z) - 566.0F;
        pos.y = sp54.y + sp48.y;
        pos.z = zero + sp48.x;

        weapon_gobj = wpManagerMakeWeapon(NULL, &dGRSectorArwingWeaponLaser2DWeaponDesc, &pos, WEAPON_FLAG_PARENT_GROUND);

        if (weapon_gobj != NULL)
        {
            wp = wpGetStruct(weapon_gobj);

            wp->physics.vel_air.x = -230.0F;

            DObjGetStruct(weapon_gobj)->rotate.vec.f = rotate;
        }
    }
}

// 0x80107518
sb32 grSectorArwingWeaponLaserExplodeProcUpdate(GObj *weapon_gobj)
{
    if (wpMainDecLifeCheckExpire(wpGetStruct(weapon_gobj)) != FALSE)
    {
        return TRUE;
    }
    else return FALSE;
}

// 0x80107544
void grSectorArwingWeaponLaserExplodeInitVars(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    wp->lifetime = 16;

    wp->attack_coll.can_reflect = FALSE;
    wp->attack_coll.can_absorb = TRUE;
    wp->attack_coll.can_shield = FALSE;

    wp->physics.vel_air.x = wp->physics.vel_air.y = wp->physics.vel_air.z = 0.0F;

    wp->attack_coll.size = 200.0F;

    DObjGetStruct(weapon_gobj)->dl = NULL;

    wpMainClearAttackRecord(wp);

    wp->proc_update = grSectorArwingWeaponLaserExplodeProcUpdate;

    wp->proc_map        =
    wp->proc_hit        =
    wp->proc_shield     =
    wp->proc_hop        =
    wp->proc_setoff     =
    wp->proc_hop        =
    wp->proc_reflector  =  NULL;
}

// 0x801075E0
sb32 grSectorArwingWeaponLaser3DProcMap(GObj *weapon_gobj)
{
    DObj *dobj = DObjGetStruct(weapon_gobj);

    if (ABSF(dobj->translate.vec.f.z) < 1000.0F)
    {
        if (wpMapTestAllCheckCollEnd(weapon_gobj) != FALSE)
        {
            func_800269C0_275C0(nSYAudioFGMExplodeS);
            efManagerSparkleWhiteMultiExplodeMakeEffect(&dobj->translate.vec.f);
            grSectorArwingWeaponLaserExplodeInitVars(weapon_gobj);
        }
    }
    return FALSE;
}

// 0x80107670
sb32 grSectorArwingWeaponLaser3DProcHit(GObj *weapon_gobj)
{
    func_800269C0_275C0(nSYAudioFGMExplodeS);
    efManagerSparkleWhiteMultiExplodeMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f);
    grSectorArwingWeaponLaserExplodeInitVars(weapon_gobj);

    return FALSE;
}

// 0x801076B0
sb32 grSectorArwingWeaponLaser3DProcAbsorb(GObj *weapon_gobj)
{
    func_800269C0_275C0(nSYAudioFGMExplodeS);
    efManagerSparkleWhiteMultiExplodeMakeEffect(&DObjGetStruct(weapon_gobj)->translate.vec.f);

    return TRUE;
}

// 0x801076E8
void grSectorArwingWeaponLaser3DMakeWeapon(void)
{
    GObj *weapon_gobj;
    GObj *fighter_gobj;
    s32 random;
    s32 player;
    FTStruct *fp;
    WPStruct *wp;
    Vec3f wp_pos;
    Vec3f ft_pos;
    Vec3f sp94;
    Vec3f sp88;
    Vec3f sp7C;
    Vec3f wp_angle;
    Mtx44f mtx;
    DObj *dobj;

    dobj = gGRCommonStruct.sector.map_dobjs[0];

    func_ovl2_80106730(dobj, &sp94, &sp88, &sp7C);

    mtx[0][0] = sp88.x; // sp30
    mtx[0][1] = sp88.y; // sp34
    mtx[0][2] = sp88.z; // sp38

    mtx[1][0] = sp7C.x; // sp40
    mtx[1][1] = sp7C.y; // sp44
    mtx[1][2] = sp7C.z; // sp48

    mtx[2][0] = sp94.x; // sp50
    mtx[2][1] = sp94.y; // sp54
    mtx[2][2] = sp94.z; // sp58

    mtx[0][3] = mtx[1][3] = mtx[2][3] = 0.0F;// sp3C, sp4C, sp5C

    mtx[3][0] = dobj->translate.vec.f.x + gGRCommonStruct.sector.arwing_target_x; // sp60
    mtx[3][1] = dobj->translate.vec.f.y; // sp64
    mtx[3][2] = dobj->translate.vec.f.z;

    mtx[3][3] = 1.0F;

    wp_pos.y = 0.0F;
    wp_pos.x = 0;

    wp_pos.z = 666.0F;

    gmCollisionGetWorldPosition(mtx, &wp_pos);

    random = syUtilsRandIntRange(gSCManagerBattleState->pl_count + gSCManagerBattleState->cp_count);

    fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];

    for (player = 0; player < random; player++)
    {
        fighter_gobj = fighter_gobj->link_next;
    }
    fp = ftGetStruct(fighter_gobj);

    if ((fp->coll_data.floor_line_id == -1) || (fp->coll_data.floor_line_id == -2))
    {
        ft_pos.x = ft_pos.y = ft_pos.z = 0;
    }
    else
    {
        ft_pos = fp->joints[nFTPartsJointTopN]->translate.vec.f;

        ft_pos.y += fp->coll_data.floor_dist;
    }
    wp_angle.x = ft_pos.x - wp_pos.x;
    wp_angle.y = ft_pos.y - wp_pos.y;
    wp_angle.z = ft_pos.z - wp_pos.z;

    syVectorNorm3D(&wp_angle);

    weapon_gobj = wpManagerMakeWeapon(NULL, &dGRSectorArwingWeaponLaser3DWeaponDesc, &wp_pos, WEAPON_FLAG_PARENT_GROUND);

    if (weapon_gobj != NULL)
    {
        wp = wpGetStruct(weapon_gobj);

        wp->physics.vel_air.x = wp_angle.x * 230.0F;
        wp->physics.vel_air.y = wp_angle.y * 230.0F;
        wp->physics.vel_air.z = wp_angle.z * 230.0F;

        func_ovl2_8010719C(&wp_angle, &DObjGetStruct(weapon_gobj)->rotate.vec.f);
    }
}

// 0x80107910
void func_ovl2_80107910(void)
{
    if (gGRCommonStruct.sector.arwing_laser_count == 2)
    {
        grSectorArwingWeaponLaser2DMakeWeapon();
    }
    else grSectorArwingWeaponLaser3DMakeWeapon();

    func_800269C0_275C0(nSYAudioFGMSectorArwingLaser);
}

// 0x80107958
void func_ovl2_80107958(void)
{
    s32 ammo;
    void *mh1, *mh2;

    if (gGRCommonStruct.sector.arwing_laser_ammo == 0)
    {
        ammo = 0;

        if (gGRCommonStruct.sector.arwing_pilot_curr == -2)
        {
            if 
            (
                ((gGRCommonStruct.sector.unk_sector_0x4C == 0) && (gGRCommonStruct.sector.arwing_appear_timer == 5)) || 
                ((gGRCommonStruct.sector.unk_sector_0x4C == 1) && (gGRCommonStruct.sector.arwing_appear_timer == 5))
            )
            {
                ammo = grSectorArwingPrepareLaserCount();
            }
        }
        else if (gGRCommonStruct.sector.is_arwing_z_near == 0)
        {
            gGRCommonStruct.sector.unk_sector_0x4E = 60;
            gGRCommonStruct.sector.arwing_laser_ammo = 0;
        }
        else
        {
            gGRCommonStruct.sector.unk_sector_0x4E--;

            if (gGRCommonStruct.sector.unk_sector_0x4E == 0)
            {
                ammo = grSectorArwingGetLaserAmmoCount();

                gGRCommonStruct.sector.unk_sector_0x4E = 60;
            }
        }
        if (ammo != 0)
        {
            gGRCommonStruct.sector.arwing_laser_ammo = ammo;
            gGRCommonStruct.sector.arwing_laser_timer = 0;
            gGRCommonStruct.sector.unk_sector_0x52 = 0;
        }
    }
    else
    {
        if (gGRCommonStruct.sector.arwing_laser_timer == 0)
        {
            if (gGRCommonStruct.sector.unk_sector_0x52 == 0)
            {
                mh1 = gGRCommonStruct.sector.map_head;

#ifdef PORT
                grSectorArwingAddAnim(gGRCommonStruct.sector.map_dobjs[4], lbRelocGetFileData(AObjEvent32*, mh1, llFoxSpecial3_1B84_AnimJoint), 0.0F);
                grSectorArwingAddAnim(gGRCommonStruct.sector.map_dobjs[5], lbRelocGetFileData(AObjEvent32*, mh1, llFoxSpecial3_1B84_AnimJoint), 0.0F);
#else
                grSectorArwingAddAnim(gGRCommonStruct.sector.map_dobjs[4], lbRelocGetFileData(AObjEvent32*, mh1, &llFoxSpecial3_1B84_AnimJoint), 0.0F);
                grSectorArwingAddAnim(gGRCommonStruct.sector.map_dobjs[5], lbRelocGetFileData(AObjEvent32*, mh1, &llFoxSpecial3_1B84_AnimJoint), 0.0F);
#endif

                gGRCommonStruct.sector.unk_sector_0x52++;
            }
            else if (gGRCommonStruct.sector.map_dobjs[4]->anim_wait == AOBJ_ANIM_NULL)
            {
                mh2 = gGRCommonStruct.sector.map_head;

                func_ovl2_80107910();

#ifdef PORT
                grSectorArwingAddAnim(gGRCommonStruct.sector.map_dobjs[2], lbRelocGetFileData(AObjEvent32*, mh2, llFoxSpecial3_1B34_AnimJoint), 0.0F);
                grSectorArwingAddAnim(gGRCommonStruct.sector.map_dobjs[3], lbRelocGetFileData(AObjEvent32*, mh2, llFoxSpecial3_1B34_AnimJoint), 0.0F);
#else
                grSectorArwingAddAnim(gGRCommonStruct.sector.map_dobjs[2], lbRelocGetFileData(AObjEvent32*, mh2, &llFoxSpecial3_1B34_AnimJoint), 0.0F);
                grSectorArwingAddAnim(gGRCommonStruct.sector.map_dobjs[3], lbRelocGetFileData(AObjEvent32*, mh2, &llFoxSpecial3_1B34_AnimJoint), 0.0F);
#endif

                gGRCommonStruct.sector.arwing_laser_timer = 30;
                gGRCommonStruct.sector.arwing_laser_ammo--;
            }
        }
        if (gGRCommonStruct.sector.arwing_laser_timer)
        {
            gGRCommonStruct.sector.arwing_laser_timer--;
        }
        if (gGRCommonStruct.sector.arwing_laser_ammo == 0)
        {
            gGRCommonStruct.sector.unk_sector_0x4E = 240;
        }
    }
}

// 0x80107B30
void func_ovl2_80107B30(void)
{
    if ((gGRCommonStruct.sector.map_dobjs[8]->anim_wait == AOBJ_ANIM_NULL) && (gGRCommonStruct.sector.map_dobjs[7]->flags == DOBJ_FLAG_NONE))
    {
#ifdef PORT
        grSectorArwingAddAnim(gGRCommonStruct.sector.map_dobjs[8], lbRelocGetFileData(AObjEvent32*, gGRCommonStruct.sector.map_file, llFoxSpecial3_2EB4_AnimJoint), 0.0F);
#else
        grSectorArwingAddAnim(gGRCommonStruct.sector.map_dobjs[8], lbRelocGetFileData(AObjEvent32*, gGRCommonStruct.sector.map_file, &llFoxSpecial3_2EB4_AnimJoint), 0.0F);
#endif

        func_800269C0_275C0(nSYAudioFGMSectorAmbient2);
    }
}

// 0x80107BA0
#if defined(PORT) && defined(SSB64_NETMENU)
/*
 * Netplay rollback only: keep yakumono line 1 translate aligned with the flight DObj tree during
 * patrol even when is_arwing_line_active has not flipped yet for the frame. Vanilla UpdateCollisions
 * gates on line_active && z_near; without this, snapshot kin hashes and the visible mesh diverge at
 * patrol start and rollback apply pops the deck sideways.
 */
void grSectorArwingReconcileDeckYakumonoFromFlightTree(void)
{
    GRCommonGroundVarsSector *sec;
    DObj *d0;
    DObj *d1;
    Vec3f pos;

    if (syNetplaySimQuantizeActive() == FALSE)
    {
        return;
    }
    sec = &gGRCommonStruct.sector;
    if (sec->arwing_status != nGRSectorArwingStatusPatrol)
    {
        return;
    }
    if (sec->arwing_pilot_curr == -2)
    {
        return;
    }
    d0 = sec->map_dobjs[0];
    d1 = sec->map_dobjs[1];
    if ((d0 == NULL) || (d1 == NULL) || (d0->anim_wait == AOBJ_ANIM_NULL))
    {
        return;
    }
    grSectorArwingCanonicalizeSimState();
    pos.x = d0->translate.vec.f.x + sec->arwing_target_x;
    pos.y = d0->translate.vec.f.y + d1->translate.vec.f.y;
    pos.z = 0.0F;
    syNetplayQuantizeVec3f(&pos);
    /*
     * Early patrol only: line 1 is not yet live under vanilla gating. SetPos here aligns yakumono
     * translate with the flight tree for snapshot hash / mesh parity.
     *
     * When line_active && z_near, grSectorArwingUpdateCollisions must be the sole SetPos call this
     * frame — speed is (new_pos - old_translate) and a second SetPos at the same position zeroes
     * gMPCollisionSpeeds[1], so grounded fighters see a moving deck with no platform carry (ftmain).
     */
    if ((sec->is_arwing_line_active == FALSE) || (sec->is_arwing_z_near == FALSE))
    {
        mpCollisionSetYakumonoPosID(1, &pos);
    }
    else if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        /*
         * Rollback load skips deck-derived mp_yaku[1] restore; yakumono translate can still be the
         * pre-load live value. UpdateCollisions would derive gMPCollisionSpeeds[1] from that stale
         * anchor and grounded fighters slide hundreds of units during resim (Sector Z soak: Fox
         * SpecialN on line 1). Snap translate to the flight-tree pos with zero speed when the gap
         * exceeds one frame of patrol motion (~45 u); normal per-frame carry stays below that.
         */
        DObj *yakumono_dobj;
        f32 dx;
        f32 dy;
        f32 dist_sq;
        sb32 diag_on = grSectorArwingCarryDiagEnabled();

        if ((gMPCollisionYakumonoDObjs != NULL) && (gMPCollisionSpeeds != NULL))
        {
            yakumono_dobj = gMPCollisionYakumonoDObjs->dobjs[1];
            if (yakumono_dobj != NULL)
            {
                dx = pos.x - yakumono_dobj->translate.vec.f.x;
                dy = pos.y - yakumono_dobj->translate.vec.f.y;
                dist_sq = (dx * dx) + (dy * dy);
                if (dist_sq > (45.0F * 45.0F))
                {
                    if (diag_on != FALSE)
                    {
                        port_log("SSB64 GRSector: arwing_carry_snap tick=%u fired=1 dist_sq=%.6f "
                                 "old_translate=(%.6f,%.6f,%.6f) new_pos=(%.6f,%.6f,%.6f) "
                                 "old_speed=(%.6f,%.6f,%.6f)\n",
                                 syNetInputGetTick(), (double)dist_sq,
                                 (double)yakumono_dobj->translate.vec.f.x,
                                 (double)yakumono_dobj->translate.vec.f.y,
                                 (double)yakumono_dobj->translate.vec.f.z,
                                 (double)pos.x, (double)pos.y, (double)pos.z,
                                 (double)gMPCollisionSpeeds[1].x, (double)gMPCollisionSpeeds[1].y,
                                 (double)gMPCollisionSpeeds[1].z);
                    }
                    yakumono_dobj->translate.vec.f.x = pos.x;
                    yakumono_dobj->translate.vec.f.y = pos.y;
                    yakumono_dobj->translate.vec.f.z = pos.z;
                    gMPCollisionSpeeds[1].x = 0.0F;
                    gMPCollisionSpeeds[1].y = 0.0F;
                    gMPCollisionSpeeds[1].z = 0.0F;
                }
                else if (diag_on != FALSE)
                {
                    port_log("SSB64 GRSector: arwing_carry_snap tick=%u fired=0 dist_sq=%.6f\n",
                             syNetInputGetTick(), (double)dist_sq);
                }
            }
        }
    }
}
#endif

void grSectorArwingUpdateCollisions(void)
{
    Vec3f pos;

    if (gGRCommonStruct.sector.arwing_pilot_curr != -2)
    {
        if ((gGRCommonStruct.sector.is_arwing_line_active) && (gGRCommonStruct.sector.is_arwing_z_near))
        {
#if defined(PORT) && defined(SSB64_NETMENU)
            /*
             * Netplay rollback only: align flight DObjs before computing the yakumono line position.
             * mpCollisionSetYakumonoPosID derives gMPCollisionSpeeds[line] from (new_pos - old_translate);
             * call it exactly once per frame with the final position so grounded fighters receive vel_speed carry (ftmain).
             */
            if (syNetplaySimQuantizeActive() != FALSE)
            {
                grSectorArwingCanonicalizeSimState();
            }

#endif
            pos.x = gGRCommonStruct.sector.map_dobjs[0]->translate.vec.f.x + gGRCommonStruct.sector.arwing_target_x;
            pos.y = gGRCommonStruct.sector.map_dobjs[0]->translate.vec.f.y + gGRCommonStruct.sector.map_dobjs[1]->translate.vec.f.y;
            pos.z = 0.0F;
#if defined(PORT) && defined(SSB64_NETMENU)
            if (syNetplaySimQuantizeActive() != FALSE)
            {
                syNetplayQuantizeVec3f(&pos);
            }

#endif

            if ((gGRCommonStruct.sector.is_arwing_z_collision == FALSE) || (gGRCommonStruct.sector.is_arwing_line_collision == FALSE))
            {
                mpCollisionSetYakumonoOnID(1);
            }
            mpCollisionSetYakumonoPosID(1, &pos);
        }
        if (!(gGRCommonStruct.sector.is_arwing_line_active) || !(gGRCommonStruct.sector.is_arwing_z_near))
        {
            if ((gGRCommonStruct.sector.is_arwing_z_collision != FALSE) && (gGRCommonStruct.sector.is_arwing_line_collision != FALSE))
            {
                mpCollisionSetYakumonoOffID(1);
            }
        }
        gGRCommonStruct.sector.is_arwing_line_collision = gGRCommonStruct.sector.is_arwing_line_active;
        gGRCommonStruct.sector.is_arwing_z_collision = gGRCommonStruct.sector.is_arwing_z_near;
    }
}

// 0x80107CA0
void grSectorArwingUpdatePatrol(void)
{
    grSectorArwingDecideZNear();
    func_ovl2_80106DD8();
    func_ovl2_80107958();
    func_ovl2_80107B30();
    grSectorArwingUpdateCollisions();

    if (gGRCommonStruct.sector.map_dobjs[0]->anim_wait == AOBJ_ANIM_NULL)
    {
        gGRCommonStruct.sector.map_gobj->flags = GOBJ_FLAG_HIDDEN;

        gGRCommonStruct.sector.arwing_appear_timer = syUtilsRandIntRange(1140) + 960;
        gGRCommonStruct.sector.arwing_status = nGRSectorArwingStatusWait;

        mpCollisionSetYakumonoOffID(1);
    }
    else gGRCommonStruct.sector.arwing_appear_timer++;
}

// 0x80107D50
void func_ovl2_80107D50(void)
{
    GObj *map_gobj;
    GRSectorDesc *desc;

    if (gGRCommonStruct.sector.arwing_flight_pattern != -1)
    {
        map_gobj = gGRCommonStruct.sector.map_gobj;
        gGRCommonStruct.sector.arwing_laser_count = dGRSectorArwingLaserCounts[gGRCommonStruct.sector.arwing_flight_pattern];

        desc = (GRSectorDesc*) ((intptr_t)dGRSectorArwingSectorDescs[gGRCommonStruct.sector.arwing_flight_pattern] + (uintptr_t)gGRCommonStruct.sector.map_head);

#ifdef PORT
        grSectorArwingAddAnim(gGRCommonStruct.sector.map_dobjs[0], (AObjEvent32*)PORT_RESOLVE(desc->anim_joint_0x0), 0.0F);
        grSectorArwingAddAnim(gGRCommonStruct.sector.map_dobjs[7], (AObjEvent32*)PORT_RESOLVE(desc->anim_joint_0x1C), 0.0F);
        grSectorArwingAddAnim(gGRCommonStruct.sector.map_dobjs[9], (AObjEvent32*)PORT_RESOLVE(desc->anim_joint_0x24), 0.0F);
        grSectorArwingAddAnim(gGRCommonStruct.sector.map_dobjs[11], (AObjEvent32*)PORT_RESOLVE(desc->anim_joint_0x2C), 0.0F);
#else
        grSectorArwingAddAnim(gGRCommonStruct.sector.map_dobjs[0], desc->anim_joint_0x0, 0.0F);
        grSectorArwingAddAnim(gGRCommonStruct.sector.map_dobjs[7], desc->anim_joint_0x1C, 0.0F);
        grSectorArwingAddAnim(gGRCommonStruct.sector.map_dobjs[9], desc->anim_joint_0x24, 0.0F);
        grSectorArwingAddAnim(gGRCommonStruct.sector.map_dobjs[11], desc->anim_joint_0x2C, 0.0F);
#endif

        gGRCommonStruct.sector.arwing_flight_pattern = -1;
        map_gobj->flags = GOBJ_FLAG_NONE;
    }
}

#if defined(PORT) && defined(SSB64_NETMENU)
static void grSectorArwingCanonicalizeDobjTreeWalk(DObj *dobj);

void grSectorArwingCanonicalizeSimState(void)
{
    GObj *map_gobj;
    DObj *root;

    if (syNetplaySimQuantizeActive() == FALSE)
    {
        return;
    }
    map_gobj = gGRCommonStruct.sector.map_gobj;
    if (map_gobj == NULL)
    {
        return;
    }
    root = DObjGetStruct(map_gobj);
    if (root != NULL)
    {
        grSectorArwingCanonicalizeDobjTreeWalk(root);
    }
    gGRCommonStruct.sector.arwing_target_x = syNetplayQuantizeF32(gGRCommonStruct.sector.arwing_target_x);
}

static void grSectorArwingCanonicalizeDobjTreeWalk(DObj *dobj)
{
    while (dobj != NULL)
    {
        syNetplayQuantizeDObjAnimPose(dobj);
        if (dobj->child != NULL)
        {
            grSectorArwingCanonicalizeDobjTreeWalk(dobj->child);
        }
        if (dobj->sib_next != NULL)
        {
            dobj = dobj->sib_next;
        }
        else
        {
            while (TRUE)
            {
                if (dobj->parent == DOBJ_PARENT_NULL)
                {
                    return;
                }
                if (dobj->parent->sib_next != NULL)
                {
                    dobj = dobj->parent->sib_next;
                    break;
                }
                dobj = dobj->parent;
            }
        }
    }
}

GObj *grSectorArwingWeaponLaser2DRespawnAt(const Vec3f *pos)
{
    Vec3f spawn_pos = *pos;

    return wpManagerMakeWeapon(NULL, &dGRSectorArwingWeaponLaser2DWeaponDesc, &spawn_pos,
                               WEAPON_FLAG_PARENT_GROUND);
}

GObj *grSectorArwingWeaponLaser3DRespawnAt(const Vec3f *pos)
{
    Vec3f spawn_pos = *pos;

    return wpManagerMakeWeapon(NULL, &dGRSectorArwingWeaponLaser3DWeaponDesc, &spawn_pos,
                               WEAPON_FLAG_PARENT_GROUND);
}


#endif

// 0x80107E08
void grSectorProcUpdate(GObj *ground_gobj)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    /*
     * Netplay arwing flight-anim driver. The Arwing's motion lives in the flight AnimJoint on
     * map_dobjs[0] (and the banking/laser joints on 7/9/11), stepped each frame by the standalone
     * priority-5 gcPlayAnimAll process registered on map_gobj in grSectorInitAll. Under rollback that
     * process does not advance the arwing anim cursor (observed: anim_wait pinned at its attach length,
     * anim_frame stuck at 0 for the whole patrol, so the ship never leaves its off-screen start and
     * grSectorArwingUpdatePatrol's anim_wait==NULL patrol-end never fires). grSectorProcUpdate is a
     * priority-4 process that *does* run inside gcRunAll on both forward and resim ticks (arwing_status
     * advances normally), so drive the anim from here — the same way the DK Jungle barrel advances its
     * motion from its own proc rather than relying on the standalone anim process. Stepped before the
     * status logic to preserve the original priority-5-before-priority-4 ordering (patrol-end reads the
     * freshly-stepped cursor). Gated to active rollback so offline play keeps using the engine anim
     * process untouched and we never double-step.
     */
    if ((syNetplayRollbackSemanticsActive() != FALSE) && (gGRCommonStruct.sector.map_gobj != NULL))
    {
        gcPlayAnimAll(gGRCommonStruct.sector.map_gobj);
        grSectorArwingCanonicalizeSimState();
        if (gGRCommonStruct.sector.arwing_status == nGRSectorArwingStatusPatrol)
        {
            grSectorArwingReconcileDeckYakumonoFromFlightTree();
        }
    }

#endif
    switch (gGRCommonStruct.sector.arwing_status)
    {
    case nGRSectorArwingStatusSleep:
        grSectorArwingUpdateSleep();
        break;

    case nGRSectorArwingStatusWait:
        grSectorArwingUpdateWait();
        break;

    case nGRSectorArwingStatusPatrol:
        grSectorArwingUpdatePatrol();
        break;
    }
    func_ovl2_80107D50();
}

// 0x80107E7C
void grSectorInitAll(void)
{
    GObj *map_gobj;
    void *map_file;

#ifdef PORT
    gGRCommonStruct.sector.map_head = (void*) ((uintptr_t)PORT_RESOLVE(gMPCollisionGroundData->map_nodes) - (intptr_t)llGRSectorMapMapHead);
#else
    gGRCommonStruct.sector.map_head = (void*) ((uintptr_t)gMPCollisionGroundData->map_nodes - (intptr_t)&llGRSectorMapMapHead);
#endif

#ifdef PORT
    map_file = lbRelocGetForceStatusBufferFile((intptr_t)llFoxSpecial3FileID);
#else
    map_file = lbRelocGetForceStatusBufferFile((intptr_t)&llFoxSpecial3FileID);
#endif

    gGRCommonStruct.sector.map_file = map_file;

    map_gobj = gcMakeGObjSPAfter(nGCCommonKindGround, NULL, nGCCommonLinkIDGround, GOBJ_PRIORITY_DEFAULT);

    gGRCommonStruct.sector.map_gobj = map_gobj;

    gcAddGObjDisplay(map_gobj, gcDrawDObjTreeDLLinksForGObj, 6, GOBJ_PRIORITY_DEFAULT, ~0);
#ifdef PORT
    grModelSetupGroundDObjs(map_gobj, lbRelocGetFileData(DObjDesc*, map_file, llFoxSpecial3EntryArwingDObjDesc), gGRCommonStruct.sector.map_dobjs, dGRSectorArwingTransformKinds);
#else
    grModelSetupGroundDObjs(map_gobj, lbRelocGetFileData(DObjDesc*, map_file, &llFoxSpecial3EntryArwingDObjDesc), gGRCommonStruct.sector.map_dobjs, dGRSectorArwingTransformKinds);
#endif
    gcAddGObjProcess(map_gobj, gcPlayAnimAll, nGCProcessKindFunc, 5);

    gGRCommonStruct.sector.arwing_status = 0;
    gGRCommonStruct.sector.arwing_flight_pattern = -1;
    gGRCommonStruct.sector.arwing_last_flight_pattern = -1;
    gGRCommonStruct.sector.arwing_appear_timer = 600;
    gGRCommonStruct.sector.arwing_type_cycle = 3;
    gGRCommonStruct.sector.arwing_pilot_curr = -1;
    gGRCommonStruct.sector.arwing_pilot_prev = 0;
    gGRCommonStruct.sector.arwing_target_x = 0.0F;

    map_gobj->flags = GOBJ_FLAG_HIDDEN;

#ifdef PORT
    gcAddDObjAnimJoint(gGRCommonStruct.sector.map_dobjs[10], lbRelocGetFileData(AObjEvent32*, map_file, llFoxSpecial3_2E74_AnimJoint), 0.0F);
#else
    gcAddDObjAnimJoint(gGRCommonStruct.sector.map_dobjs[10], lbRelocGetFileData(AObjEvent32*, map_file, &llFoxSpecial3_2E74_AnimJoint), 0.0F);
#endif
    gcPlayAnimAll(map_gobj);
    mpCollisionSetYakumonoOffID(1);
#ifdef PORT
    gGRCommonStruct.sector.weapon_head = (void*) ((uintptr_t)gMPCollisionGroundData - (intptr_t)llGRSectorMapMapHeader);
#else
    gGRCommonStruct.sector.weapon_head = (void*) ((uintptr_t)gMPCollisionGroundData - (intptr_t)&llGRSectorMapMapHeader);
#endif
}

// 0x80107FCC
GObj* grSectorMakeGround(void)
{
    GObj *map_gobj = gcMakeGObjSPAfter(nGCCommonKindGround, NULL, nGCCommonLinkIDGround, GOBJ_PRIORITY_DEFAULT);

    grSectorInitAll();
    gcAddGObjProcess(map_gobj, grSectorProcUpdate, nGCProcessKindFunc, 4);

    return map_gobj;
}

#if defined(PORT) && defined(SSB64_NETMENU)
/*
 * Hard cap on DObj-tree-walk iterations. The Arwing flight deck is ~18-32 nodes; this is a generous
 * upper bound used purely as a watchdog so a malformed/cyclic child/sib_next/parent chain bails instead
 * of wedging the game thread. Defensive only — the synctest emergency-restore hang originally suspected
 * here turned out to be a u32 tick-window overflow elsewhere
 * (docs/bugs/netrollback_emergency_restore_sparkle_window_overflow_2026-06-27.md); this cap stays as a
 * cheap safety net for the unguarded rollback Arwing tree walks.
 */
#define GR_SECTOR_ARWING_TREE_WALK_MAX_NODES 512U

static sb32 grSectorArwingDobjHasDrawableDllink(DObj *dobj)
{
    if (dobj == NULL)
    {
        return FALSE;
    }
    return (portDObjDLLinkChainLooksValid(dobj->dv) != 0) ? TRUE : FALSE;
}

static void grSectorArwingCountTreeDObjsBounded(DObj *dobj, s32 *drawable_out, u32 *nodes_out, u32 *budget)
{
    while (dobj != NULL)
    {
        if (*budget == 0U)
        {
            return; /* netplay: malformed/cyclic DObj tree — bail rather than wedge the game thread */
        }
        (*budget)--;
        if (nodes_out != NULL)
        {
            (*nodes_out)++;
        }
        if (grSectorArwingDobjHasDrawableDllink(dobj) != FALSE)
        {
            (*drawable_out)++;
        }
        if (dobj->child != NULL)
        {
            grSectorArwingCountTreeDObjsBounded(dobj->child, drawable_out, nodes_out, budget);
        }
        if (dobj->sib_next != NULL)
        {
            dobj = dobj->sib_next;
        }
        else
        {
            while (TRUE)
            {
                if (*budget == 0U)
                {
                    return;
                }
                (*budget)--;
                if (dobj->parent == DOBJ_PARENT_NULL)
                {
                    return;
                }
                if (dobj->parent->sib_next != NULL)
                {
                    dobj = dobj->parent->sib_next;
                    break;
                }
                dobj = dobj->parent;
            }
        }
    }
}

static void grSectorArwingCountTreeDObjs(DObj *dobj, s32 *drawable_out, u32 *nodes_out)
{
    u32 budget = GR_SECTOR_ARWING_TREE_WALK_MAX_NODES;

    grSectorArwingCountTreeDObjsBounded(dobj, drawable_out, nodes_out, &budget);
}

static sb32 grSectorArwingVisualTreeNeedsRebuild(GObj *map_gobj, DObj *root, DObj *d0)
{
    s32 drawable_count;
    u32 node_count;

    if ((root == NULL) || (d0 == NULL))
    {
        return TRUE;
    }
    if (root != d0)
    {
        return TRUE;
    }
    if (d0->parent_gobj != map_gobj)
    {
        return TRUE;
    }
    drawable_count = 0;
    node_count = 0;
    grSectorArwingCountTreeDObjs(root, &drawable_count, &node_count);
    if (drawable_count == 0)
    {
        return TRUE;
    }
    return FALSE;
}

static void grSectorEnsureArwingMapGObjDisplay(GObj *map_gobj)
{
    if (map_gobj == NULL)
    {
        return;
    }
    if (map_gobj->proc_display == NULL)
    {
        gcAddGObjDisplay(map_gobj, gcDrawDObjTreeDLLinksForGObj, 6, GOBJ_PRIORITY_DEFAULT, ~0);
    }
}

/*
 * Rebuild the Arwing map GObj DObj tree when rollback left map_dobjs[] decoupled from map_gobj, or
 * DObjDLLink chains are stale/invalid (draw uses gcDrawDObjTreeDLLinksForGObj on map_gobj's tree).
 * Returns TRUE if the tree was rebuilt this call; FALSE if already drawable.
 */
sb32 grSectorReestablishArwingVisualTree(void)
{
    GObj *map_gobj;
    void *map_file;
    DObj *root;
    DObj *d0;
    s32 i;

    map_gobj = gGRCommonStruct.sector.map_gobj;
    map_file = gGRCommonStruct.sector.map_file;
    if ((map_gobj == NULL) || (map_file == NULL))
    {
        return FALSE;
    }
    root = DObjGetStruct(map_gobj);
    d0 = gGRCommonStruct.sector.map_dobjs[0];
    if (grSectorArwingVisualTreeNeedsRebuild(map_gobj, root, d0) == FALSE)
    {
        grSectorEnsureArwingMapGObjDisplay(map_gobj);
        return FALSE;
    }
    gcRemoveDObjAll(map_gobj);
    for (i = 0; i < (s32)ARRAY_COUNT(gGRCommonStruct.sector.map_dobjs); i++)
    {
        gGRCommonStruct.sector.map_dobjs[i] = NULL;
    }
    grModelSetupGroundDObjs(
        map_gobj,
        lbRelocGetFileData(DObjDesc*, map_file, llFoxSpecial3EntryArwingDObjDesc),
        gGRCommonStruct.sector.map_dobjs,
        dGRSectorArwingTransformKinds);
    gcAddDObjAnimJoint(
        gGRCommonStruct.sector.map_dobjs[10],
        lbRelocGetFileData(AObjEvent32*, map_file, llFoxSpecial3_2E74_AnimJoint),
        0.0F);
    grSectorEnsureArwingMapGObjDisplay(map_gobj);
    gcPlayAnimAll(map_gobj);
    return TRUE;
}

void grSectorArwingFillPresentationDiag(GRSectorArwingPresentationDiag *out)
{
    GRCommonGroundVarsSector *sec;
    GObj *map_gobj;
    DObj *root;
    DObj *d0;
    s32 drawable_count;
    u32 node_count;
    u32 di;

    if (out == NULL)
    {
        return;
    }
    memset(out, 0, sizeof(*out));
    sec = &gGRCommonStruct.sector;
    map_gobj = sec->map_gobj;
    root = (map_gobj != NULL) ? DObjGetStruct(map_gobj) : NULL;
    d0 = sec->map_dobjs[0];
    out->proc_display = (map_gobj != NULL) ? (void *)map_gobj->proc_display : NULL;
    out->dl_link_id = (map_gobj != NULL) ? map_gobj->dl_link_id : 0xFFU;
    out->root_matches_d0 = ((root != NULL) && (d0 != NULL) && (root == d0)) ? TRUE : FALSE;
    out->dl_valid_root = grSectorArwingDobjHasDrawableDllink(d0);
    drawable_count = 0;
    node_count = 0;
    if (root != NULL)
    {
        grSectorArwingCountTreeDObjs(root, &drawable_count, &node_count);
    }
    out->drawable_dobj_count = drawable_count;
    out->tree_child_count = node_count;
    for (di = 1; di < (u32)ARRAY_COUNT(sec->map_dobjs); di++)
    {
        if (grSectorArwingDobjHasDrawableDllink(sec->map_dobjs[di]) != FALSE)
        {
            out->dl_valid_mesh = TRUE;
            break;
        }
    }
}

s8 grSectorInferFlightPatternIdx(void)
{
    GRCommonGroundVarsSector *sec;
    DObj *d0;
    AObjEvent32 *live_joint;
    s32 i;

    sec = &gGRCommonStruct.sector;
    if (sec->arwing_flight_pattern >= 0)
    {
        return sec->arwing_flight_pattern;
    }
    if (sec->arwing_last_flight_pattern >= 0)
    {
        return sec->arwing_last_flight_pattern;
    }
    d0 = sec->map_dobjs[0];
    if (d0 == NULL)
    {
        return -1;
    }
    live_joint = d0->anim_joint.event32;
    if (live_joint == NULL)
    {
        return -1;
    }
    for (i = 0; i < (s32)ARRAY_COUNT(dGRSectorArwingSectorDescs); i++)
    {
        GRSectorDesc *desc =
            (GRSectorDesc *)((intptr_t)dGRSectorArwingSectorDescs[i] + (uintptr_t)sec->map_head);
        AObjEvent32 *desc_joint;

#ifdef PORT
        desc_joint = (AObjEvent32 *)PORT_RESOLVE(desc->anim_joint_0x0);
#else
        desc_joint = desc->anim_joint_0x0;
#endif
        if (desc_joint == live_joint)
        {
            return (s8)i;
        }
    }
    return -1;
}

void grSectorArwingReattachFlightAnims(s8 flight_pattern_idx)
{
    GRSectorDesc *desc;
    GRCommonGroundVarsSector *sec;

    if ((flight_pattern_idx < 0) || (flight_pattern_idx >= (s8)ARRAY_COUNT(dGRSectorArwingSectorDescs)))
    {
        return;
    }
    sec = &gGRCommonStruct.sector;
    if ((sec->map_dobjs[0] == NULL) || (sec->map_head == NULL))
    {
        return;
    }
    desc = (GRSectorDesc *)((intptr_t)dGRSectorArwingSectorDescs[flight_pattern_idx] + (uintptr_t)sec->map_head);
#ifdef PORT
    grSectorArwingAddAnim(sec->map_dobjs[0], (AObjEvent32 *)PORT_RESOLVE(desc->anim_joint_0x0), 0.0F);
    grSectorArwingAddAnim(sec->map_dobjs[7], (AObjEvent32 *)PORT_RESOLVE(desc->anim_joint_0x1C), 0.0F);
    grSectorArwingAddAnim(sec->map_dobjs[9], (AObjEvent32 *)PORT_RESOLVE(desc->anim_joint_0x24), 0.0F);
    grSectorArwingAddAnim(sec->map_dobjs[11], (AObjEvent32 *)PORT_RESOLVE(desc->anim_joint_0x2C), 0.0F);
#else
    grSectorArwingAddAnim(sec->map_dobjs[0], desc->anim_joint_0x0, 0.0F);
    grSectorArwingAddAnim(sec->map_dobjs[7], desc->anim_joint_0x1C, 0.0F);
    grSectorArwingAddAnim(sec->map_dobjs[9], desc->anim_joint_0x24, 0.0F);
    grSectorArwingAddAnim(sec->map_dobjs[11], desc->anim_joint_0x2C, 0.0F);
#endif
}

static void grSectorArwingApplyAnimTransformsWalk(DObj *dobj)
{
    MObj *mobj;
    u32 budget = GR_SECTOR_ARWING_TREE_WALK_MAX_NODES;

    while (dobj != NULL)
    {
        if (budget == 0U)
        {
            return; /* netplay: malformed/cyclic DObj tree — bail rather than wedge the game thread */
        }
        budget--;
        if (dobj->anim_wait != AOBJ_ANIM_NULL)
        {
            gcPlayDObjAnimJoint(dobj);
        }
        mobj = dobj->mobj;
        while (mobj != NULL)
        {
            if (budget == 0U)
            {
                return;
            }
            budget--;
            if (mobj->anim_wait != AOBJ_ANIM_NULL)
            {
                gcPlayMObjMatAnim(mobj);
            }
            mobj = mobj->next;
        }
        if (dobj->child != NULL)
        {
            dobj = dobj->child;
        }
        else if (dobj->sib_next != NULL)
        {
            dobj = dobj->sib_next;
        }
        else
        {
            while (TRUE)
            {
                if (budget == 0U)
                {
                    return;
                }
                budget--;
                if (dobj->parent == DOBJ_PARENT_NULL)
                {
                    return;
                }
                if (dobj->parent->sib_next != NULL)
                {
                    dobj = dobj->parent->sib_next;
                    break;
                }
                dobj = dobj->parent;
            }
        }
    }
}

void grSectorArwingApplyAnimTransforms(GObj *map_gobj)
{
    DObj *dobj;

    if (map_gobj == NULL)
    {
        return;
    }
    dobj = DObjGetStruct(map_gobj);
    if (dobj == NULL)
    {
        return;
    }
    grSectorArwingApplyAnimTransformsWalk(dobj);
}

void grSectorRepairArwingPresentation(sb32 tree_was_reestablished, s8 flight_pattern_idx,
                                    const Vec3f *dobj_translate, const Vec3f *dobj_rotate,
                                    u16 dobj_valid_mask)
{
    GRCommonGroundVarsSector *sec;
    GObj *map_gobj;
    DObj *d0;
    u32 di;

    sec = &gGRCommonStruct.sector;
    map_gobj = sec->map_gobj;
    if (map_gobj == NULL)
    {
        return;
    }
    if (sec->arwing_status == nGRSectorArwingStatusPatrol)
    {
        d0 = sec->map_dobjs[0];
        if (tree_was_reestablished != FALSE)
        {
            if (flight_pattern_idx >= 0)
            {
                grSectorArwingReattachFlightAnims(flight_pattern_idx);
            }
        }
        else if ((d0 != NULL) && (flight_pattern_idx >= 0) && (d0->anim_joint.event32 == NULL) &&
                 (d0->anim_wait != AOBJ_ANIM_NULL))
        {
            grSectorArwingReattachFlightAnims(flight_pattern_idx);
        }
    }
    if ((dobj_translate != NULL) && (dobj_rotate != NULL))
    {
        for (di = 0; di < (u32)ARRAY_COUNT(sec->map_dobjs); di++)
        {
            if ((dobj_valid_mask & (u16)(1U << di)) == 0U)
            {
                continue;
            }
            if (sec->map_dobjs[di] == NULL)
            {
                continue;
            }
            sec->map_dobjs[di]->translate.vec.f = dobj_translate[di];
            sec->map_dobjs[di]->rotate.vec.f = dobj_rotate[di];
#if defined(PORT) && defined(SSB64_NETMENU)
            syNetplayQuantizeVec3f(&sec->map_dobjs[di]->translate.vec.f);
            syNetplayQuantizeVec3f(&sec->map_dobjs[di]->rotate.vec.f);

#endif
        }
    }
    grSectorArwingApplyAnimTransforms(map_gobj);
#if defined(PORT) && defined(SSB64_NETMENU)
    /*
     * ApplyAnimTransforms runs gcPlayDObjAnimJoint at the restored cursor, which re-derives root
     * translate from the flight spline (~one patrol tick behind the snapshot blob). Re-seat blob poses
     * so rollback deck kin / map hash matches the ring slot saved at end-of-tick.
     */
    if ((dobj_translate != NULL) && (dobj_rotate != NULL))
    {
        for (di = 0; di < (u32)ARRAY_COUNT(sec->map_dobjs); di++)
        {
            if ((dobj_valid_mask & (u16)(1U << di)) == 0U)
            {
                continue;
            }
            if (sec->map_dobjs[di] == NULL)
            {
                continue;
            }
            sec->map_dobjs[di]->translate.vec.f = dobj_translate[di];
            sec->map_dobjs[di]->rotate.vec.f = dobj_rotate[di];
            syNetplayQuantizeVec3f(&sec->map_dobjs[di]->translate.vec.f);
            syNetplayQuantizeVec3f(&sec->map_dobjs[di]->rotate.vec.f);
        }
    }
#endif
}

void grSectorSyncArwingMapGObjFlags(u32 snap_map_gobj_flags)
{
    GObj *map_gobj;
    DObj *root_dobj;

    map_gobj = gGRCommonStruct.sector.map_gobj;
    if (map_gobj == NULL)
    {
        return;
    }
    root_dobj = gGRCommonStruct.sector.map_dobjs[0];
    switch (gGRCommonStruct.sector.arwing_status)
    {
    case nGRSectorArwingStatusPatrol:
        if ((root_dobj != NULL) && (root_dobj->anim_wait != AOBJ_ANIM_NULL))
        {
            map_gobj->flags = GOBJ_FLAG_NONE;
        }
        else
        {
            map_gobj->flags = GOBJ_FLAG_HIDDEN;
        }
        break;

    case nGRSectorArwingStatusSleep:
        map_gobj->flags = GOBJ_FLAG_HIDDEN;
        break;

    case nGRSectorArwingStatusWait:
        if (gGRCommonStruct.sector.arwing_appear_timer != 0)
        {
            map_gobj->flags = GOBJ_FLAG_HIDDEN;
        }
        else
        {
            map_gobj->flags = snap_map_gobj_flags;
        }
        break;

    default:
        map_gobj->flags = snap_map_gobj_flags;
        break;
    }
}

#endif
