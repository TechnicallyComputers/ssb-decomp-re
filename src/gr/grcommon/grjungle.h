#ifndef _GRJUNGLE_H_
#define _GRJUNGLE_H_

#include <ssb_types.h>
#include <sys/objdef.h>
#include <gr/grdef.h>

extern void grJungleTaruCannAddAnimOffset(GObj *ground_gobj, intptr_t offset);
extern void grJungleTaruCannAddAnimFill(GObj *ground_gobj);
extern void grJungleTaruCannAddAnimShoot(GObj *ground_gobj);
extern void grJungleTaruCannUpdateMove(GObj *ground_gobj);
extern void grJungleTaruCannUpdateRotate(GObj *ground_gobj);
extern void grJungleTaruCannProcUpdate(GObj *ground_gobj);
extern void grJungleMakeTaruCann(void);
extern GObj* grJungleMakeGround(void);
extern sb32 grJungleTaruCannCheckGetDamageKind(GObj *ground_gobj, GObj *fighter_gobj, s32 *kind);
extern void grJungleTaruCannGetPosition(Vec3f *pos);
extern f32 grJungleTaruCannGetRotate(void);

#ifdef PORT
extern GObj *grJungleGetTaruCannGobj(void);
extern void grJungleRepairTaruCannPresentation(const Vec3f *translate, f32 rotate_z);
extern sb32 grJungleEnsureTaruCannCoupling(GObj *tarucann_gobj);
extern sb32 grJungleTaruCannIsChildShootAnimActive(GObj *ground_gobj);
#endif

#endif
