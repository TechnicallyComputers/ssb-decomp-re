#ifndef _GRYOSTER_H_
#define _GRYOSTER_H_

#include <ssb_types.h>
#include <sys/objdef.h>
#include <gr/grdef.h>
#include <ef/efdef.h>

extern intptr_t lGRYosterParticleScriptBankLo;  // 0x00B22980
extern intptr_t lGRYosterParticleScriptBankHi;  // 0x00B22A00
extern intptr_t lGRYosterParticleTextureBankLo; // 0x00B22A00
extern intptr_t lGRYosterParticleTextureBankHi; // 0x00B22C30

extern u8 dGRYosterCloudLineIDs[/* */];

extern LBGenerator* grYosterCloudVaporMakeEffect(Vec3f *pos);
#if defined(PORT) && defined(SSB64_NETMENU)
extern void grYosterRebindCloudDobjs(s32 cloud_id);
extern void grYosterRepairCloudPresentation(s32 cloud_id);
extern void grYosterAnchorCloudRootTranslate(s32 cloud_id);
extern void grYosterGetCloudSpawnTranslate(s32 cloud_id, Vec3f *out);
extern GObj* grYosterGetCloudGobj(s32 cloud_id);
extern sb32 grYosterCloudReestablishedThisTick(s32 cloud_id);
#endif
extern sb32 grYosterCloudMatAnimIsIdle(MObj *mobj);
extern sb32 grYosterCloudPressureGateOpen(const GRYosterCloud *cloud, MObj *mobj);
extern sb32 grYosterCheckFighterCloudStand(s32 cloud_id);
extern void grYosterUpdateCloudSolid(s32 cloud_id);
extern void grYosterUpdateCloudEvaporate(s32 cloud_id);
extern void grYosterUpdateCloudAnim(s32 cloud_id);
extern void grYosterProcUpdate(GObj *ground_gobj);
extern void grYosterInitAll(void);
extern GObj* grYosterMakeGround(void);

#endif
