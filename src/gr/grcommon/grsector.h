#ifndef _GRSECTOR_H_
#define _GRSECTOR_H_

#include <ssb_types.h>
#include <sys/objdef.h>
#include <gr/grdef.h>

extern void func_ovl2_80106730(DObj *arg0, Vec3f *vec1, Vec3f *vec2, Vec3f *vec3);
extern sb32 grSectorArwingLaser3DFuncMatrix(Mtx *mtx, DObj *dobj, Gfx **dls);
extern void grSectorArwingAddAnim(DObj *dobj, AObjEvent32 *anim_joint, f32 unused);
extern void grSectorArwingUpdateSleep(void);
extern void grSectorArwingUpdateWait(void);
extern void func_ovl2_80106C88(void);
extern void func_ovl2_80106CC4(void);
extern void func_ovl2_80106D00(void);
extern void func_ovl2_80106DD8(void);
extern s32 grSectorArwingPrepareLaserCount(void);
extern sb32 grSectorArwingWeaponLaser2DProcMap(GObj *weapon_gobj);
extern sb32 grSectorArwingWeaponLaser2DProcHit(GObj *weapon_gobj);
extern void func_ovl2_801070A4(Vec3f *rotate, Vec3f *direction, Vec3f *vec3, Vec3f *vec4);
extern void func_ovl2_8010719C(Vec3f *vel, Vec3f *rotate);
extern sb32 grSectorArwingWeaponLaser2DProcHop(GObj *weapon_gobj);
extern sb32 grSectorArwingWeaponLaser2DProcReflector(GObj *weapon_gobj);
extern void grSectorArwingWeaponLaser2DMakeWeapon(void);
extern sb32 grSectorArwingWeaponLaserExplodeProcUpdate(GObj *weapon_gobj);
extern void grSectorArwingWeaponLaserExplodeInitVars(GObj *weapon_gobj);
extern sb32 grSectorArwingWeaponLaser3DProcMap(GObj *weapon_gobj);
extern sb32 grSectorArwingWeaponLaser3DProcHit(GObj *weapon_gobj);
extern sb32 grSectorArwingWeaponLaser3DProcAbsorb(GObj *weapon_gobj);
extern void grSectorArwingWeaponLaser3DMakeWeapon(void);
extern void func_ovl2_80107910(void);
extern void func_ovl2_80107958(void);
extern void func_ovl2_80107B30(void);
extern void grSectorArwingUpdateCollisions(void);
extern void grSectorArwingUpdatePatrol(void);
extern void func_ovl2_80107D50(void);
extern void grSectorProcUpdate(GObj *ground_gobj);
extern void grSectorInitAll(void);
extern GObj* grSectorMakeGround(void);

typedef enum grSectorArwingStatus
{
	nGRSectorArwingStatusSleep,
	nGRSectorArwingStatusWait,
	nGRSectorArwingStatusPatrol
} grSectorArwingStatus;

#ifdef PORT
typedef struct GRSectorArwingPresentationDiag
{
	sb32 root_matches_d0;
	s32 drawable_dobj_count;
	sb32 dl_valid_root;
	sb32 dl_valid_mesh;
	u32 tree_child_count;
	void *proc_display;
	u8 dl_link_id;
} GRSectorArwingPresentationDiag;

extern sb32 grSectorReestablishArwingVisualTree(void);
extern void grSectorSyncArwingMapGObjFlags(u32 snap_map_gobj_flags);
extern void grSectorArwingFillPresentationDiag(GRSectorArwingPresentationDiag *out);
extern s8 grSectorInferFlightPatternIdx(void);
extern void grSectorArwingReattachFlightAnims(s8 flight_pattern_idx);
extern void grSectorArwingApplyAnimTransforms(GObj *map_gobj);
extern void grSectorRepairArwingPresentation(sb32 tree_was_reestablished, s8 flight_pattern_idx,
                                             const Vec3f *dobj_translate, const Vec3f *dobj_rotate,
                                             u16 dobj_valid_mask);
#if defined(SSB64_NETMENU)
extern void grSectorArwingCanonicalizeSimState(void);
extern void grSectorArwingReconcileDeckYakumonoFromFlightTree(void);
/*
 * Netplay rollback respawn: recreate a single stage-owned Arwing laser (owner_gobj == NULL) at a
 * captured position. The vanilla grSectorArwingWeaponLaser{2,3}DMakeWeapon spawn lasers in pairs at
 * stage-derived positions, which is wrong for per-weapon resim; these helpers wrap wpManagerMakeWeapon
 * so syNetRbSnapSpawnWeaponFromBlob can restore exactly one laser, then apply its blob.
 */
extern GObj *grSectorArwingWeaponLaser2DRespawnAt(const Vec3f *pos);
extern GObj *grSectorArwingWeaponLaser3DRespawnAt(const Vec3f *pos);
#endif
#endif

#endif
