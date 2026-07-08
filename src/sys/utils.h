#ifndef SYS_TRIG_RAND_H
#define SYS_TRIG_RAND_H

#include <PR/ultratypes.h>

extern f32 syUtilsTan(f32 angle);
extern f32 syUtilsArcTan(f32 yDivX);
extern f32 syUtilsArcTan2(f32 y, f32 x);
extern f32 syUtilsArcSin(f32 x);
extern f32 syUtilsArcCos(f32 x);
extern f32 syUtilsCsc(f32 x);
extern f32 syUtilsSec(f32 x);
extern f32 syUtilsCot(f32 x);
extern f32 __sinf(f32);
extern f32 __cosf(f32);

#ifdef PORT
extern void syUtilsSetRandomSeed(s32 seed);
extern s32 syUtilsRandSeed(void);
extern void syUtilsResetCosmeticRandomSeed(s32 seed);
extern u16 syUtilsRandUShortCosmetic(void);
extern f32 syUtilsRandFloatCosmetic(void);
extern f32 syUtilsRandFloatForcedCosmetic(void);
extern s32 syUtilsRandIntRangeCosmetic(s32 range);
/* Accessor for the per-peer cosmetic LCG seed. Exposed for netplay diagnostics:
 * effect manager (`ef/efmanager.c`) and particle system (`lb/lbparticle.c`)
 * read/write this seed through `syUtilsRand*Cosmetic` whenever
 * `syNetRollbackIsActive()` is TRUE (i.e. the entire duration of a netplay VS
 * session). The shared `rng` partition only hashes the *game* seed, so silent
 * drift of this cosmetic seed between peers is the smoking gun for
 * `eff`-partition divergence on stages with continuous particle activity
 * (DK Jungle TaruCann). See docs/bugs/netplay_dk_jungle_effect_pop_desync_2026-05-25.md. */
extern s32 syUtilsCosmeticRandSeed(void);
#endif
extern void syUtilsSetRandomSeedPtr(s32 *seedptr);
extern u16 syUtilsRandUShort(void);
extern f32 syUtilsRandFloat(void);
extern s32 syUtilsRandIntRange(s32 range); // Does this actually return u32? Found a function that needs to cast this to s32, and assigning to a variable first would bump the stack
extern u8 syUtilsRandTimeUChar(void);
extern f32 syUtilsRandTimeFloat(void);
extern s32 syUtilsRandTimeUCharRange(s32 range);

#endif /* SYS_TRIG_RAND_H */
