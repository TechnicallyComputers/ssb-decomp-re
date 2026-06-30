#include <ef/effect.h>
#include <ft/fighter.h>
#include <it/item.h>
#include <wp/weapon.h>
#include <wp/wpness/wpnesspkthunder.h>
#include <sc/scene.h>
#include <sys/utils.h>
#include <reloc_data.h>
#ifdef PORT
#include "fighter_registry.h"
#endif
extern void *func_800269C0_275C0(u16 id);
#ifdef PORT
/* For the defensive NULL-file_head guard's one-shot warning. */
#include <stdlib.h>
extern void port_log(const char *fmt, ...);
/* Effect jitter is cosmetic; in netplay it must not advance gameplay RNG during rollback/resim. */
#define syUtilsRandFloat syUtilsRandFloatCosmetic
#define syUtilsRandIntRange syUtilsRandIntRangeCosmetic
#endif
#if defined(PORT) && defined(SSB64_NETMENU)
#include <lb/lbparticle.h>
#include <sys/objman.h>
extern u32 syNetInputGetTick(void);

static sb32 efManagerNetplaySnapshotEffectDiagEnabled(void)
{
	static int s_env_cache = -999;
	const char *e;

	if (s_env_cache != -999)
	{
		return (s_env_cache != 0) ? TRUE : FALSE;
	}
	e = getenv("SSB64_NETPLAY_SNAPSHOT_EFFECT_DIAG");
	s_env_cache = ((e != NULL) && (e[0] != '\0') && (e[0] != '0')) ? 1 : 0;
	return (s_env_cache != 0) ? TRUE : FALSE;
}

static sb32 efManagerNetplayEffectXfIsLive(GObj *effect_gobj, LBTransform *xf, const char **out_reason)
{
	LBParticle *pc;

	if (effect_gobj == NULL)
	{
		*out_reason = "null_gobj";
		return FALSE;
	}
	if (xf == NULL)
	{
		*out_reason = "null_xf";
		return FALSE;
	}
	if (lbParticleTransformIsOnFreeList(xf) != FALSE)
	{
		*out_reason = "xf_on_free_list";
		return FALSE;
	}
	if (lbParticleTransformIsAllocated(xf) == FALSE)
	{
		*out_reason = "xf_not_allocated";
		return FALSE;
	}
	if (xf->users_num == 0U)
	{
		*out_reason = "xf_zero_users";
		return FALSE;
	}
	if (xf->transform_status > nLBTransformStatusFinished)
	{
		*out_reason = "xf_bad_status";
		return FALSE;
	}
	if (xf->effect_gobj != effect_gobj)
	{
		*out_reason = "owner_mismatch";
		return FALSE;
	}
	pc = lbParticleFindStructForEffectGobj(effect_gobj);
	if (pc == NULL)
	{
		*out_reason = "no_particle";
		return FALSE;
	}
	if (pc->xf != xf)
	{
		*out_reason = "particle_xf_mismatch";
		return FALSE;
	}
	return TRUE;
}

static void efManagerNetplayEjectStaleXfEffect(
	GObj *effect_gobj, EFStruct *ep, LBTransform *xf, const char *proc_tag, const char *reason)
{
	static u32 s_stale_log_budget = 64U;
	sb32 verbose;

	verbose = efManagerNetplaySnapshotEffectDiagEnabled();
	if ((verbose != FALSE) || (s_stale_log_budget > 0U))
	{
		if (s_stale_log_budget > 0U)
		{
			s_stale_log_budget--;
		}
		port_log(
		    "SSB64 NetRbSnapshot: effect_xf_stale tick=%u proc=%s reason=%s effect_gobj_id=%u xf=%p xf_owner=%p "
		    "particle=%p free=%d alloc=%d users=%u\n",
		    (unsigned int)syNetInputGetTick(), proc_tag, reason, (unsigned int)effect_gobj->id, (void *)xf,
		    (xf != NULL) ? (void *)xf->effect_gobj : NULL,
		    (void *)lbParticleFindStructForEffectGobj(effect_gobj),
		    (xf != NULL) ? (int)lbParticleTransformIsOnFreeList(xf) : -1,
		    (xf != NULL) ? (int)lbParticleTransformIsAllocated(xf) : -1,
		    (unsigned int)((xf != NULL) ? xf->users_num : 0U));
	}
	if (ep != NULL)
	{
		efManagerSetPrevStructAlloc(ep);
	}
	gcEjectGObj(effect_gobj);
}
#endif

// // // // // // // // // // // //
//                               //
//       INITIALIZED DATA        //
//                               //
// // // // // // // // // // // //

// 0x8012DE80
f32 dEFManagerDamageSpawnSparksAngles[/* */] = { 18.0F, 0.0F, -18.0F };

// 0x8012DE8C
f32 dEFManagerDamageSpawnMDustAngles[/* */] = { 18.0F, 0.0F, -18.0F };

// 0x8012DE98
u8 dEFManagerDamageNormalHeavyPrimColorR[/* */] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// 0x8012DEA0
u8 dEFManagerDamageNormalHeavyPrimColorG[/* */] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// 0x8012DEA8
u8 dEFManagerDamageNormalHeavyPrimColorB[/* */] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// 0x8012DEB0
u8 dEFManagerDamageNormalHeavyEnvColorR[/* */] = { 0xFF, 0x00, 0x00, 0x78, 0xFF };

// 0x8012DEB8
u8 dEFManagerDamageNormalHeavyEnvColorG[/* */] = { 0x00, 0xFF, 0x00, 0x78, 0xFF };

// 0x8012DEC0
u8 dEFManagerDamageNormalHeavyEnvColorB[/* */] = { 0x00, 0x00, 0xFF, 0x78, 0xFF };

// 0x8012DEC8
u8 dEFManagerImpactWavePrimColorR[/* */] = { 0xFF, 0x00, 0x00, 0xFF, 0xFF };

// 0x8012DED0
u8 dEFManagerImpactWavePrimColorG[/* */] = { 0x00, 0xFF, 0x00, 0xFF, 0xFF };

// 0x8012DED8
u8 dEFManagerImpactWavePrimColorB[/* */] = { 0x00, 0x00, 0xFF, 0x00, 0xFF };

// 0x8012DEE0
u8 dEFManagerImpactWaveEnvColorR[/* */] = { 0x00, 0x00, 0x00, 0x00, 0x00 };

// 0x8012DEE8
u8 dEFManagerImpactWaveEnvColorG[/* */] = { 0x00, 0x00, 0x00, 0x00, 0x00 };

// 0x8012DEF0
u8 dEFManagerImpactWaveEnvColorB[/* */] = { 0x00, 0x00, 0x00, 0x00, 0x00 };

// 0x8012DEF8
u8 dEFManagerDeadExplodeEnvColorSiblingR[/* */] = { 0xFF, 0x00, 0xFF, 0x00 };

// 0x8012DEFC
u8 dEFManagerDeadExplodeEnvColorSiblingG[/* */] = { 0x62, 0x7E, 0xFF, 0xFF };

// 0x8012DF00
u8 dEFManagerDeadExplodeEnvColorSiblingB[/* */] = { 0x4B, 0xFF, 0x00, 0x00 };

// 0x8012DF04
u8 dEFManagerDeadExplodeEnvColorChildR[/* */] = { 0xA6, 0x1F, 0x3E, 0xFB };

// 0x8012DF08
u8 dEFManagerDeadExplodeEnvColorChildG[/* */] = { 0x62, 0xFF, 0x6D, 0x66 };

// 0x8012DF0C
u8 dEFManagerDeadExplodeEnvColorChildB[/* */] = { 0x21, 0xA1, 0xFF, 0xC7 };

// 0x8012DF10
f32 dEFManagerDeadExplodeRotateD[/* */] = { 0.0F, 90.0F, 180.0F, 270.0F };

// 0x8012DF20
u8 dEFManagerDamageNormalLightIDs[/* */] = { 0x49, 0x4A, 0x4B, 0x4C };

// 0x8012DF24
EFDesc dEFManagerDamageSlashEffectDesc = 
{
    0x4 | 0x1,                              // Flags
    18,                                     // DL Link
    &gEFManagerFiles[0],                      // Texture file

    // DObj transformation struct 1
    {
        0x28,                               // Main matrix transformations   
        0x45,                               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations  
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerHaveStructProcUpdate,     // Proc Update
    gcDrawDObjTreeDLLinksForGObj,         // Proc Render

    llEFCommonEffects1DamageSlashDObjDesc,    // DObj Setup attributes offset (?)
    llEFCommonEffects1DamageSlashMObjSub,     // MObjSub offset
    llEFCommonEffects1DamageSlashAnimJoint,   // AnimJoint offset
    llEFCommonEffects1DamageSlashMatAnimJoint // MatAnimJoint offset
};

// 0x8012DF4C
EFDesc dEFManagerShockSmallEffectDesc =
{
    EFFECT_FLAG_USERDATA,                   // Flags
    18,                                     // DL Link
    &gEFManagerFiles[1],                      // Texture file

    // DObj transformation struct 1
    {
        0x28,                               // Main matrix transformations   
        0x45,                               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindNull,               // Main matrix transformations  
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerVelAddDestroyAnimEnd,           // Proc Update
    lbCommonDObjScaleXProcDisplay,           // Proc Render

    llEFCommonEffects2ShockSmallDObjDesc,     // DObj Setup attributes offset (?)
    llEFCommonEffects2ShockSmallMObjSub,      // MObjSub offset
    0x0,                                       // AnimJoint offset
    llEFCommonEffects2ShockSmallMatAnimJoint  // MatAnimJoint offset
};

// 0x8012DF74
EFDesc dEFManagerDamageFlyOrbsEffectDesc =
{
    EFFECT_FLAG_USERDATA | 0x1,             // Flags
    15,                                     // DL Link
    &gEFManagerFiles[0],                      // Texture file

    // DObj transformation struct 1
    {
        0x28,                               // Main matrix transformations   
        nGCMatrixKindSca,                // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindSca,                // Main matrix transformations  
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerDamageFlyOrbsProcUpdate,    // Proc Update
    lbCommonDObjScaleXProcDisplay,       // Proc Render

    llEFCommonEffects1FlyOrbsDObjDesc,     // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    llEFCommonEffects1FlyOrbsAnimJoint,    // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012DF9C
EFDesc dEFManagerDamageSpawnOrbsEffectDesc =
{
    EFFECT_FLAG_USERDATA,                   // Flags
    0,                                      // DL Link
    &gEFManagerFiles[0],                      // Texture file

    // DObj transformation struct 1
    {
        nGCMatrixKindNull,               // Main matrix transformations   
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindNull,               // Main matrix transformations  
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerDamageSpawnOrbsProcUpdate,  // Proc Update
    NULL,                                   // Proc Render

    0x0,                                    // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    0x0,                                    // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012DFC4
EFDesc dEFManagerImpactWaveEffectDesc =
{
    EFFECT_FLAG_USERDATA,                   // Flags
    10,                                     // DL Link
    &gEFManagerFiles[0],                      // Texture file

    // DObj transformation struct 1
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations   
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindNull,               // Main matrix transformations  
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerImpactWaveProcUpdate,       // Proc Update
    efManagerImpactWaveProcDisplay,      // Proc Render

    llEFCommonEffects1ImpactWaveDObjDesc,    // DObj Setup attributes offset (?)
    llEFCommonEffects1ImpactWaveMObjSub,     // MObjSub offset
    llEFCommonEffects1ImpactWaveAnimJoint,   // AnimJoint offset
    llEFCommonEffects1ImpactWaveMatAnimJoint // MatAnimJoint offset
};

// 0x8012DFEC
EFDesc dEFManagerStarRodSparkEffectDesc =
{
    EFFECT_FLAG_USERDATA | 0x1,             // Flags
    15,                                     // DL Link
    &gEFManagerFiles[0],                      // Texture file

    // DObj transformation struct 1
    {
        0x28,                               // Main matrix transformations   
        0x45,                               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        0x45,                               // Main matrix transformations  
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerStarRodSparkProcUpdate,     // Proc Update
    lbCommonDObjScaleXProcDisplay,       // Proc Render

    llEFCommonEffects1CommonSparkDObjDesc,    // DObj Setup attributes offset (?)
    llEFCommonEffects1CommonSparkMObjSub,     // MObjSub offset
    llEFCommonEffects1CommonSparkAnimJoint,   // AnimJoint offset
    llEFCommonEffects1CommonSparkMatAnimJoint // MatAnimJoint offset
};

// 0x8012E014
EFDesc dEFManagerDamageFlySparksEffectDesc =
{
    EFFECT_FLAG_USERDATA | 0x1,             // Flags
    15,                                     // DL Link
    &gEFManagerFiles[0],                      // Texture file

    // DObj transformation struct 1
    {
        0x28,                               // Main matrix transformations   
        nGCMatrixKindRotRpyR,            // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        0x45,                               // Main matrix transformations  
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerDamageFlySparksProcUpdate,  // Proc Update
    lbCommonDObjScaleXProcDisplay,       // Proc Render

    llEFCommonEffects1CommonSparkDObjDesc,    // DObj Setup attributes offset (?)
    llEFCommonEffects1CommonSparkMObjSub,     // MObjSub offset
    llEFCommonEffects1CommonSparkAnimJoint,   // AnimJoint offset
    llEFCommonEffects1CommonSparkMatAnimJoint // MatAnimJoint offset
};

// 0x8012E03C
EFDesc dEFManagerDamageSpawnSparksEffectDesc =
{
    EFFECT_FLAG_USERDATA,                   // Flags
    0,                                      // DL Link
    &gEFManagerFiles[0],                      // Texture file

    // DObj transformation struct 1
    {
        nGCMatrixKindNull,               // Main matrix transformations   
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindNull,               // Main matrix transformations  
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerDamageSpawnSparksProcUpdate,// Proc Update
    NULL,                                   // Proc Render

    0x0,                                    // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    0x0,                                    // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E064
EFDesc dEFManagerDamageFlyMDustEffectDesc = 
{
    EFFECT_FLAG_USERDATA | 0x1,             // Flags
    15,                                     // DL Link
    &gEFManagerFiles[0],                      // Texture file

    // DObj transformation struct 1
    {
        0x28,                               // Main matrix transformations   
        nGCMatrixKindRotRpyR,            // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        0x44,                               // Main matrix transformations  
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerDamageFlySparksProcUpdate,  // Proc Update
    gcDrawDObjTreeDLLinksForGObj,        // Proc Render

    llEFCommonEffects1DamageFlyMDustDObjDesc,     // DObj Setup attributes offset (?)
    llEFCommonEffects1DamageFlyMDustMObjSub,      // MObjSub offset
    llEFCommonEffects1DamageFlyMDustAnimJoint,    // AnimJoint offset
    llEFCommonEffects1DamageFlyMDustMatAnimJoint  // MatAnimJoint offset
};

// 0x8012E08C
EFDesc dEFManagerDamageSpawnMDustEffectDesc =
{
    EFFECT_FLAG_USERDATA,                   // Flags
    0,                                      // DL Link
    &gEFManagerFiles[0],                      // Texture file

    // DObj transformation struct 1
    {
        nGCMatrixKindNull,               // Main matrix transformations   
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindNull,               // Main matrix transformations  
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerDamageSpawnMDustProcUpdate, // Proc Update
    NULL,                                   // Proc Render

    0x0,                                    // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    0x0,                                    // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E0B4
EFDesc dEFManagerFireSparkEffectDesc =
{
    0x4 | EFFECT_FLAG_USERDATA,             // Flags
    15,                                     // DL Link
    &gEFManagerFiles[1],                      // Texture file

    // DObj transformation struct 1
    {
        0x50,                               // Main matrix transformations   
        0x49,                               // Secondary matrix transformations
        0x12                                // ???
    },

    // DObj transformation struct 2
    {
        0x45,                               // Main matrix transformations  
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerHaveStructProcUpdate,     // Proc Update
    lbCommonDObjScaleXProcDisplay,     // Proc Render

    llEFCommonEffects2FireSparkDObjDesc,    // DObj Setup attributes offset (?)
    llEFCommonEffects2FireSparkMObjSub,     // MObjSub offset
    llEFCommonEffects2FireSparkAnimJoint,   // AnimJoint offset
    llEFCommonEffects2FireSparkMatAnimJoint // MatAnimJoint offset
};

// 0x8012E0DC
intptr_t dEFManagerFoxReflectorAnimJointOffsets[/* */] = 
{ 
    llFoxSpecial2ReflectorStartAnimJoint, 
    llFoxSpecial2ReflectorLoopAnimJoint, 
    llFoxSpecial2ReflectorHitAnimJoint, 
    llFoxSpecial2ReflectorEndAnimJoint 
};

// 0x8012E0EC
EFDesc dEFManagerFoxReflectorEffectDesc = 
{
    0x4 | EFFECT_FLAG_USERDATA,             // Flags
    15,                                     // DL Link
    &gFTDataFoxSpecial2,                    // Texture file

    // DObj transformation struct 1
    {
        0x4F,                               // Main matrix transformations   
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTra,                // Main matrix transformations  
        0x2C,                               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerFoxReflectorProcUpdate,      // Proc Update
    gcDrawDObjTreeForGObj,                // Proc Render

    llFoxSpecial2ReflectorDObjDesc,        // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    llFoxSpecial2ReflectorStartAnimJoint,  // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E114
SYColorRGBPair dEFManagerShieldColors[/* */] =
{
    { { 0xFF, 0xFF, 0xFF }, { 0xFF, 0x00, 0x00 } }, // Player 1
    { { 0xFF, 0xFF, 0xFF }, { 0x00, 0xFF, 0x00 } }, // Player 2
    { { 0xFF, 0xFF, 0xFF }, { 0x00, 0x00, 0xFF } }, // Player 3
    { { 0xFF, 0xFF, 0xFF }, { 0x00, 0x00, 0x00 } }, // Player 4 / CPU
    { { 0xFF, 0xFF, 0xFF }, { 0xC0, 0xC0, 0xC0 } }  // Shield Damage
};

// 0x8012E134
EFDesc dEFManagerShieldEffectDesc =
{
    0x4 | EFFECT_FLAG_USERDATA,             // Flags
    15,                                     // DL Link
    &gFTManagerCommonFile,                         // Texture file

    // DObj transformation struct 1
    {
        0x4F,                               // Main matrix transformations   
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        0x2C,                               // Main matrix transformations  
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerShieldProcUpdate,                     // Proc Update
    efManagerShieldProcDisplay,                    // Proc Render

    llFTManagerCommonShieldDObjDesc,       // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    0x0,                                    // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E15C
EFDesc dEFManagerYoshiShieldEffectDesc =
{
    EFFECT_FLAG_USERDATA,                   // Flags
    15,                                     // DL Link
    &gFTDataYoshiModel,                     // Texture file

    // DObj transformation struct 1
    {
        0x50,                               // Main matrix transformations   
        0x2C,                               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindNull,               // Main matrix transformations  
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerShieldProcUpdate,                     // Proc Update
    efManagerYoshiShieldProcDisplay,               // Proc Render

    llYoshiModelShieldDObjDesc,            // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    0x0,                                    // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E184
EFDesc dEFManagerCatchSwirlEffectDesc = 
{
    0x4 | 0x1,                              // Flags
    18,                                     // DL Link
    &gEFManagerFiles[1],                      // Texture file

    // DObj transformation struct 1
    {
        0x28,                               // Main matrix transformations   
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations  
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerHaveStructProcUpdate,     // Proc Update
    gcDrawDObjTreeDLLinksForGObj,         // Proc Render

    llEFCommonEffects2CatchSwirlMObjSub,                // DObj Setup attributes offset (?)
    llEFCommonEffects2CatchSwirlDObjDesc,                  // MObjSub offset
    llEFCommonEffects2CatchSwirlAnimJoint,                // AnimJoint offset
    llEFCommonEffects2CatchSwirlMatAnimJoint              // MatAnimJoint offset
};

// 0x8012E1AC
EFDesc dEFManagerReflectBreakEffectDesc =
{
    0x4 | 0x1,                              // Flags
    18,                                     // DL Link
    &gEFManagerFiles[1],                      // Texture file

    // DObj transformation struct 1
    {
        0x28,                               // Main matrix transformations   
        nGCMatrixKindRotRpyR,            // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations  
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerHaveStructProcUpdate,     // Proc Update
    gcDrawDObjTreeDLLinksForGObj,         // Proc Render

    llEFCommonEffects2ReflectBreakMObjSub,              // DObj Setup attributes offset (?)
    llEFCommonEffects2ReflectBreakDObjDesc,                // MObjSub offset
    llEFCommonEffects2ReflectBreakAnimJoint,              // AnimJoint offset
    llEFCommonEffects2ReflectBreakMatAnimJoint            // MatAnimJoint offset
};

// 0x8012E1D4
EFDesc dEFManagerPikachuUnkEffectDesc =
{
    0x4 | EFFECT_FLAG_USERDATA | 0x1,       // Flags
    15,                                     // DL Link
    &gFTDataPikachuSpecial2,                // Texture file

    // DObj transformation struct 1
    {
        0x28,                               // Main matrix transformations   
        nGCMatrixKindRotRpyR,            // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations  
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    func_ovl2_801017E8,                     // Proc Update
    gcDrawDObjTreeForGObj,                // Proc Render

    llPikachuSpecial2UnkDObjDesc,                // DObj Setup attributes offset (?)
    llPikachuSpecial2UnkMObjSub,                  // MObjSub offset
    llPikachuSpecial2UnkAnimJoint,                // AnimJoint offset
    llPikachuSpecial2UnkMatAnimJoint              // MatAnimJoint offset
};

// 0x8012E1FC
EFDesc dEFManagerPikachuThunderShockEffectDesc =
{
    0x4 | EFFECT_FLAG_USERDATA,             // Flags
    15,                                     // DL Link
    &gFTDataPikachuSpecial2,                // Texture file

    // DObj transformation struct 1
    {
        0x4F,                               // Main matrix transformations   
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations  
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerHaveStructProcUpdate,     // Proc Update
    gcDrawDObjTreeDLLinksForGObj,         // Proc Render

    llPikachuSpecial2ThunderShockDObjDesc,              // DObj Setup attributes offset (?)
    llPikachuSpecial2ThunderShockMObjSub,                // MObjSub offset
    llPikachuSpecial2ThunderShock0AnimJoint,              // AnimJoint offset
    llPikachuSpecial2ThunderShock0MatAnimJoint            // MatAnimJoint offset
};

// 0x8012E224
EFDesc dEFManagerPikachuThunderTrailEffectDesc =
{
    EFFECT_FLAG_USERDATA,                   // Flags
    15,                                     // DL Link
    &gFTDataPikachuModel,                   // Texture file

    // DObj transformation struct 1
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindNull,               // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerPikachuThunderTrailProcUpdate,     // Proc Update
    efManagerPikachuThunderTrailProcDisplay,     // Proc Render

    llPikachuModelThunderTrailDObjDesc,              // DObj Setup attributes offset (?)
    llPikachuModelThunderTrailMObjSub,                // MObjSub offset
    0x0,                                    // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E24C
EFDesc dEFManagerThunderJoltEffectDesc =
{
    0x4,                                    // Flags
    15,                                     // DL Link
    &gFTDataPikachuSpecial3,                // Texture file

    // DObj transformation struct 1
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerHaveStructProcUpdate,     // Proc Update
    gcDrawDObjTreeForGObj,                // Proc Render

    llPikachuSpecial3ThunderJoltDObjDesc,               // DObj Setup attributes offset (?)
    llPikachuSpecial3ThunderJoltMObjSub,                 // MObjSub offset
    llPikachuSpecial3ThunderJoltAnimJoint,               // AnimJoint offset
    llPikachuSpecial3ThunderJoltMatAnimJoint             // MatAnimJoint offset
};

// 0x8012E274
EFDesc dEFManagerVulcanJabEffectDesc =
{
    0x4 | EFFECT_FLAG_USERDATA | 0x1,       // Flags
    15,                                     // DL Link
    &gFTDataKirbySpecial2,                  // Texture file

    // DObj transformation struct 1
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindRotRpyR,            // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerKirbyVulcanJabProcUpdate,        // Proc Update
    gcDrawDObjTreeDLLinksForGObj,         // Proc Render

    llKirbySpecial2VulcanJabDObjDesc,                 // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    0x0,                                    // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E29C
EFDesc dEFManagerSamusGrappleBeamEffectDesc =
{
    0x4 | EFFECT_FLAG_USERDATA,             // Flags
    15,                                     // DL Link
    &gFTDataSamusSpecial2,                  // Texture file

    // DObj transformation struct 1
    {
        0x4F,                               // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        0x2E,                               // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    gcPlayAnimAll,                          // Proc Update (WHAT IS THIS FUNCTION???)
    gcDrawDObjTreeDLLinksForGObj,         // Proc Render

    llSamusSpecial2GrappleBeamDObjDesc,               // DObj Setup attributes offset (?)
    llSamusSpecial2GrappleBeamMObjSub,                 // MObjSub offset
    llSamusSpecial2GrappleBeamAnimJoint,               // AnimJoint offset
    llSamusSpecial2GrappleBeamMatAnimJoint             // MatAnimJoint offset
};

// 0x8012E2C4
EFDesc dEFManagerCaptainFalconKickEffectDesc =
{
    0x4 | EFFECT_FLAG_USERDATA,             // Flags
    15,                                     // DL Link
    &gFTDataCaptainSpecial2,                // Texture file

    // DObj transformation struct 1
    {
        0x50,                               // Main matrix transformations
        nGCMatrixKindRotRpyR,            // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerNoEjectProcUpdate,                     // Proc Update
    gcDrawDObjTreeForGObj,                // Proc Render

    llCaptainSpecial2FalconKickDObjDesc,                // DObj Setup attributes offset (?)
    llCaptainSpecial2FalconKickMObjSub,                  // MObjSub offset
    llCaptainSpecial2FalconKickAnimJoint,                // AnimJoint offset
    llCaptainSpecial2FalconKickMatAnimJoint              // MatAnimJoint offset
};

// 0x8012E2EC
EFDesc dEFManagerCaptainFalconPunchEffectDesc =
{
    EFFECT_FLAG_USERDATA,                   // Flags
    15,                                     // DL Link
    &gFTDataCaptainSpecial3,                // Texture file

    // DObj transformation struct 1
    {
        0x50,                               // Main matrix transformations
        nGCMatrixKindRotRpyR,            // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindNull,               // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerNoEjectProcUpdate,                     // Proc Update
    lbCommonDObjScaleXProcDisplay,                     // Proc Render

    llCaptainSpecial3FalconPunchDObjDesc,               // DObj Setup attributes offset (?)
    llCaptainSpecial3FalconPunchMObjSub,                 // MObjSub offset
    0x0,                                    // AnimJoint offset
    llCaptainSpecial3FalconPunchMatAnimJoint             // MatAnimJoint offset
};

// 0x8012E314
EFDesc dEFManagerPurinSingEffectDesc =
{
    0x4 | EFFECT_FLAG_USERDATA,             // Flags
    15,                                     // DL Link
    &gFTDataPurinSpecial2,                  // Texture file

    // DObj transformation struct 1
    {
        0x4F,                               // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTra,                // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerHaveStructProcUpdate,          // Proc Update
    gcDrawDObjTreeDLLinksForGObj,         // Proc Render

    llPurinSpecial2SingDObjDesc,                 // DObj Setup attributes offset (?)
    llPurinSpecial2SingMObjSub,                   // MObjSub offset
    llPurinSpecial2SingAnimJoint,                 // AnimJoint offset
    llPurinSpecial2SingMatAnimJoint               // MatAnimJoint offset
};

// 0x8012E33C
EFDesc dEFManagerDeadExplodeEffectDesc =
{
    EFFECT_FLAG_SPECIALLINK | 0x4,          // Flags
    18,                                     // DL Link
    &gEFManagerFiles[1],                      // Texture file

    // DObj transformation struct 1
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerHaveStructProcUpdate,          // Proc Update
    gcDrawDObjTreeDLLinksForGObj,         // Proc Render

    llEFCommonEffects2DeadExplodeDefaultMObjSub,        // DObj Setup attributes offset (?)
    llEFCommonEffects2DeadExplodeDefaultDObjDesc,          // MObjSub offset
    llEFCommonEffects2DeadExplodeDefaultAnimJoint,        // AnimJoint offset
    llEFCommonEffects2DeadExplodeDefaultMatAnimJoint      // MatAnimJoint offset
};

// 0x8012E364
u8 dEFManagerDeadExplodeGenID[/* */] = { 0x2D, 0x2C, 0x2B, 0x2A, 0x3F, 0x3E, 0x3D, 0x3C };

// 0x8012E36C
intptr_t dEFManagerDeadExplodeMatAnimJoints[/* */] =
{
    llEFCommonEffects2DeadExplode1MatAnimJoint,
    llEFCommonEffects2DeadExplode2MatAnimJoint,
    llEFCommonEffects2DeadExplode3MatAnimJoint,
    llEFCommonEffects2DeadExplode4MatAnimJoint
};

// 0x8012E37C
EFDesc dEFManagerKirbyCutterUpEffectDesc =
{
    0x4 | EFFECT_FLAG_USERDATA,             // Flags
    15,                                     // DL Link
    &gFTDataKirbySpecial2,                  // Texture file

    // DObj transformation struct 1
    {
        0x50,                               // Main matrix transformations
        nGCMatrixKindRotRpyR,            // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerNoEjectProcUpdate,                     // Proc Update
    gcDrawDObjTreeForGObj,                // Proc Render

    llKirbySpecial2CutterUpDObjDesc,             // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    llKirbySpecial2CutterUpAnimJoint,             // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E3A4
EFDesc dEFManagerKirbyCutterDownEffectDesc =
{
    0x4 | EFFECT_FLAG_USERDATA,             // Flags
    15,                                     // DL Link
    &gFTDataKirbySpecial2,                  // Texture file

    // DObj transformation struct 1
    {
        0x50,                               // Main matrix transformations
        nGCMatrixKindRotRpyR,            // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerNoEjectProcUpdate,                     // Proc Update
    gcDrawDObjTreeForGObj,                // Proc Render

    llKirbySpecial2CutterDownDObjDesc,           // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    llKirbySpecial2CutterDownAnimJoint,           // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E3CC
EFDesc dEFManagerKirbyCutterDrawEffectDesc =
{
    0x4 | EFFECT_FLAG_USERDATA,             // Flags
    15,                                     // DL Link
    &gFTDataKirbySpecial2,                  // Texture file

    // DObj transformation struct 1
    {
        0x4F,                               // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerNoEjectProcUpdate,                     // Proc Update
    gcDrawDObjTreeForGObj,                // Proc Render

    llKirbySpecial2CutterDrawDObjDesc,           // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    0x0,                                    // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E3F4
EFDesc dEFManagerKirbyCutterTrailEffectDesc =
{
    0x4 | EFFECT_FLAG_USERDATA,             // Flags
    15,                                     // DL Link
    &gFTDataKirbySpecial2,                  // Texture file

    // DObj transformation struct 1
    {
        0x50,                               // Main matrix transformations
        nGCMatrixKindRotRpyR,            // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerNoEjectProcUpdate,                     // Proc Update
    gcDrawDObjTreeDLLinksForGObj,                // Proc Render

    llKirbySpecial2CutterTrailDObjDesc,          // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    llKirbySpecial2CutterTrailAnimJoint,          // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E41C
EFDesc dEFManagerNessPsychicMagnetEffectDesc =
{
    0x4 | EFFECT_FLAG_USERDATA,             // Flags
    15,                                     // DL Link
    &gFTNessFileSpecial2,                   // Texture file

    // DObj transformation struct 1
    {
        0x50,                               // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTra,                // Main matrix transformations
        0x2E,                               // Secondary matrix transformations
        0x00                                // ???
    },

    gcPlayAnimAll,                          // Proc Update
    gcDrawDObjTreeDLLinksForGObj,         // Proc Render

    llNessSpecial2PsychicMagnetDObjDesc,             // DObj Setup attributes offset (?)
    llNessSpecial2PsychicMagnetMObjSub,               // MObjSub offset
    llNessSpecial2PsychicMagnetAnimJoint,             // AnimJoint offset
    llNessSpecial2PsychicMagnetMatAnimJoint           // MatAnimJoint offset
};

// 0x8012E444
EFDesc dEFManagerNessPKThunderTrailEffectDesc =
{
    EFFECT_FLAG_USERDATA,                   // Flags
    15,                                     // DL Link
    &gFTNessFileModel,                      // Texture file

    // DObj transformation struct 1
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindNull,               // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerNessPKThunderTrailProcUpdate,   // Proc Update
    efManagerNessPKThunderTrailProcDisplay,   // Proc Render

    llNessModelPKThunderTrailDObjDesc,            // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    0x0,                                    // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E46C
EFDesc dEFManagerNessPKReflectTrailEffectDesc =
{
    EFFECT_FLAG_USERDATA,                   // Flags
    18,                                     // DL Link
    &gFTNessFileModel,                      // Texture file

    // DObj transformation struct 1
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindNull,               // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerNessPKReflectTrailProcUpdate,   // Proc Update
    efManagerNessPKThunderTrailProcDisplay,   // Proc Render

    llNessModelPKThunderTrailDObjDesc,            // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    0x0,                                    // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E494
EFDesc dEFManagerNessPKThunderWaveEffectDesc =
{
    0x4 | EFFECT_FLAG_USERDATA,             // Flags
    15,                                     // DL Link
    &gFTNessFileModel,                      // Texture file

    // DObj transformation struct 1
    {
        0x50,                               // Main matrix transformations
        nGCMatrixKindRotRpyR,            // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTra,                // Main matrix transformations
        0x2E,                               // Secondary matrix transformations
        0x00                                // ???
    },

    gcPlayAnimAll,                          // Proc Update
    gcDrawDObjTreeDLLinksForGObj,         // Proc Render

    llNessModelPKThunderWaveDObjDesc,             // DObj Setup attributes offset (?)
    llNessModelPKThunderWaveMObjSub,               // MObjSub offset
    llNessModelPKThunderWaveAnimJoint,             // AnimJoint offset
    llNessModelPKThunderWaveMatAnimJoint           // MatAnimJoint offset
};

// 0x8012E4BC
EFDesc dEFManagerNessPKFlashEffectDesc =
{
    0x4,                                    // Flags
    18,                                     // DL Link
    &gEFManagerFiles[1],                      // Texture file

    // DObj transformation struct 1
    {
        0x28,                               // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerHaveStructProcUpdate,     // Proc Update
    gcDrawDObjTreeForGObj,                // Proc Render

    llEFCommonEffects2NessPKFlashMObjSub,                   // DObj Setup attributes offset (?)
    llEFCommonEffects2NessPKFlashDObjDesc,                     // MObjSub offset
    llEFCommonEffects2NessPKFlashAnimJoint,                   // AnimJoint offset
    llEFCommonEffects2NessPKFlashMatAnimJoint                 // MatAnimJoint offset
};

// 0x8012E4E4
EFDesc dEFManagerLinkEntryWaveEffectDesc =
{
    0x4,                                    // Flags
    10,                                     // DL Link
    &gFTDataLinkSpecial2,                   // Texture file

    // DObj transformation struct 1
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindNull,               // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    gcPlayAnimAll,                          // Proc Update
    gcDrawDObjTreeDLLinksForGObj,         // Proc Render

    llLinkSpecial2EntryWaveDObjDesc,             // DObj Setup attributes offset (?)
    llLinkSpecial2EntryWaveMObjSub,               // MObjSub offset
    llLinkSpecial2EntryWaveAnimJoint,             // AnimJoint offset
    llLinkSpecial2EntryWaveMatAnimJoint           // MatAnimJoint offset
};

// 0x8012E50C
EFDesc dEFManagerLinkEntryBeamEffectDesc =
{
    0x4,                                    // Flags
    10,                                     // DL Link
    &gFTDataLinkSpecial2,                   // Texture file

    // DObj transformation struct 1
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindNull,               // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    gcPlayAnimAll,                          // Proc Update
    gcDrawDObjTreeDLLinksForGObj,         // Proc Render

    llLinkSpecial2EntryBeamDObjDesc,             // DObj Setup attributes offset (?)
    llLinkSpecial2EntryBeamMObjSub,               // MObjSub offset
    llLinkSpecial2EntryBeamAnimJoint,             // AnimJoint offset
    llLinkSpecial2EntryBeamMatAnimJoint           // MatAnimJoint offset
};

// 0x8012E534
EFDesc dEFManagerKirbyEntryStarEffectDesc =
{
    0x4 | 0x1,                              // Flags
    10,                                     // DL Link
    &gFTDataKirbySpecial2,                  // Texture file

    // DObj transformation struct 1
    {
        nGCMatrixKindTra,                // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerHaveStructProcUpdate,     // Proc Update
    gcDrawDObjTreeDLLinksForGObj,         // Proc Render

    llKirbySpecial2EntryStarDObjDesc,            // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    llKirbySpecial2EntryStarLAnimJoint,           // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E55C
EFDesc dEFManagerMBallRaysEffectDesc =
{
    0x4,                                    // Flags
    10,                                     // DL Link
    &gEFManagerFiles[2],                      // Texture file

    // DObj transformation struct 1
    {
        0x44,                               // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerHaveStructProcUpdate,     // Proc Update
    gcDrawDObjTreeDLLinksForGObj,         // Proc Render

    llEFCommonEffects3MBallRaysDObjDesc,                 // DObj Setup attributes offset (?)
    llEFCommonEffects3MBallRaysMObjSub,                   // MObjSub offset
    llEFCommonEffects3MBallRaysAnimJoint,                 // AnimJoint offset
    llEFCommonEffects3MBallRaysMatAnimJoint               // MatAnimJoint offset
};

// 0x8012E584
EFDesc dEFManagerMBallThrownEffectDesc =
{
    0x4 | EFFECT_FLAG_USERDATA,             // Flags
    20,                                     // DL Link
    &gITManagerCommonData,                           // Texture file

    // DObj transformation struct 1
    {
        0x44,                               // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerMBallThrownProcUpdate,                // Proc Update
    gcDrawDObjTreeForGObj,                // Proc Render

    llITCommonDataMBallThrownDObjDesc,               // DObj Setup attributes offset (?)
    llITCommonDataMBallThrownMObjSub,                 // MObjSub offset
    llITCommonDataMBallThrownLAnimJoint,              // AnimJoint offset
    llITCommonDataMBallThrownLMatAnimJoint            // MatAnimJoint offset
};

// 0x8012E5AC
EFDesc dEFManagerYoshiEntryEggEffectDesc =
{
    0x1,                                    // Flags
    10,                                     // DL Link
    &gFTDataYoshiSpecial2,                  // Texture file

    // DObj transformation struct 1
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerHaveStructProcUpdate,     // Proc Update
    lbCommonDObjScaleXProcDisplay,                     // Proc Render

    llYoshiSpecial2EntryEggDObjDesc,             // DObj Setup attributes offset (?)
    llYoshiSpecial2EntryEggMObjSub,               // MObjSub offset
    llYoshiSpecial2EntryEggAnimJoint,             // AnimJoint offset
    llYoshiSpecial2EntryEggMatAnimJoint           // MatAnimJoint offset
};

// 0x8012E5D4
intptr_t dEFManagerYoshiEggLayAnimJoints[/* */] = { llYoshiSpecial3EggLayWaitAnimJoint, llYoshiSpecial3EggLayBreakAnimJoint };

// 0x8012E5DC
EFDesc dEFManagerYoshiEggLayEffectDesc =
{
    0x4 | EFFECT_FLAG_USERDATA | 0x1,       // Flags
    10,                                     // DL Link
    &gFTDataYoshiSpecial3,                  // Texture file

    // DObj transformation struct 1
    {
        0x50,                               // Main matrix transformations
        nGCMatrixKindSca,                // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerYoshiEggLayProcUpdate,                     // Proc Update
    gcDrawDObjTreeForGObj,                // Proc Render

    llYoshiSpecial3EggLayDObjDesc,               // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    llYoshiSpecial3EggLayThrowAnimJoint,          // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E604
EFDesc dEFManagerYoshiEggEscapeEffectDesc =
{
    EFFECT_FLAG_USERDATA,                   // Flags
    15,                                     // DL Link
    &gFTDataYoshiModel,                     // Texture file

    // DObj transformation struct 1
    {
        0x50,                               // Main matrix transformations
        0x4A,                               // Secondary matrix transformations
        0x2E                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindNull,               // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    NULL,                                   // Proc Update
    efManagerYoshiShieldProcDisplay,        // Proc Render

    llYoshiModelShieldDObjDesc,            // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    0x0,                                    // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E62C
EFDesc dEFManagerLinkSpinAttackEffectDesc =
{
    0x4 | EFFECT_FLAG_USERDATA,             // Flags
    15,                                     // DL Link
    &gFTDataLinkSpecial2,                   // Texture file

    // DObj transformation struct 1
    {
        0x50,                               // Main matrix transformations
        nGCMatrixKindRotRpyR,            // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerHaveStructProcUpdate,     // Proc Update
    gcDrawDObjTreeDLLinksForGObj,         // Proc Render

    llLinkSpecial2SpinAttackDObjDesc,                // DObj Setup attributes offset (?)
    llLinkSpecial2SpinAttackMObjSub,                  // MObjSub offset
    llLinkSpecial2SpinAttackAnimJoint,                // AnimJoint offset
    llLinkSpecial2SpinAttackMatAnimJoint              // MatAnimJoint offset
};

// 0x8012E654
EFDesc dEFManagerDonkeyEntryTaruEffectDesc =
{
    0x4,                                    // Flags
    10,                                     // DL Link
    &gFTDataDonkeySpecial2,                 // Texture file

    // DObj transformation struct 1
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerHaveStructProcUpdate,     // Proc Update
    gcDrawDObjTreeForGObj,                // Proc Render

    llDonkeySpecial2EntryTaruDObjDesc,           // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    llDonkeySpecial2EntryTaruAnimJoint,           // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E67C
EFDesc dEFManagerSamusEntryPointEffectDesc =
{
    0x4,                                    // Flags
    10,                                     // DL Link
    &gFTDataSamusSpecial2,                  // Texture file

    // DObj transformation struct 1
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerHaveStructProcUpdate,     // Proc Update
    gcDrawDObjTreeDLLinksForGObj,         // Proc Render

    llSamusSpecial2EntryPointDObjDesc,           // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    llSamusSpecial2EntryPointAnimJoint,           // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E6A4
EFDesc dEFManagerCaptainEntryCarEffectDesc =
{
    0x4 | EFFECT_FLAG_USERDATA,             // Flags
    10,                                     // DL Link
    &gFTDataCaptainSpecial2,                // Texture file

    // DObj transformation struct 1
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerCaptainEntryCarProcUpdate,            // Proc Update
    gcDrawDObjTreeDLLinksForGObj,         // Proc Render

    llCaptainSpecial2EntryCarDObjDesc,           // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    0x0,                                    // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E6CC
EFDesc dEFManagerMarioEntryDokanEffectDesc =
{
    0x4,                                    // Flags
    10,                                     // DL Link
    &gFTMarioFileSpecial2,                  // Texture file

    // DObj transformation struct 1
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerHaveStructProcUpdate,     // Proc Update
    gcDrawDObjTreeForGObj,                // Proc Render

    llMarioSpecial2EntryDokanDObjDesc,           // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    llMarioSpecial2EntryDokanAnimJoint,           // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E6F4
EFDesc dEFManagerFoxEntryArwingEffectDesc =
{
    0x4 | EFFECT_FLAG_USERDATA | 0x1,       // Flags
    10,                                     // DL Link
    &gFTDataFoxSpecial3,                    // Texture file

    // DObj transformation struct 1
    {
        nGCMatrixKindTraRotRpyR,         // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerFoxEntryArwingProcUpdate,             // Proc Update
    gcDrawDObjTreeDLLinksForGObj,         // Proc Render

    llFoxSpecial3EntryArwingDObjDesc,            // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    0x0,                                    // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E71C
u8 dEFManagerMusicNoteScriptIDs[/* */] = { 0x40, 0x41, 0x42 };

// 0x8012E720
EFDesc dEFManagerCaptureKirbyStarEffectDesc =
{
    EFFECT_FLAG_USERDATA | 0x1,             // Flags
    15,                                     // DL Link
    &gITManagerCommonData,                           // Texture file

    // DObj transformation struct 1
    {
        nGCMatrixKindTra,                // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        0x50,                               // Main matrix transformations
        0x2E,                               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerCaptureKirbyStarProcUpdate,    // Proc Update
    lbCommonDObjScaleXProcDisplay,                     // Proc Render

    llITCommonDataKirbyStarDObjDesc,                 // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    0x0,                                    // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E748
EFDesc dEFManagerLoseKirbyStarEffectDesc =
{
    EFFECT_FLAG_USERDATA | 0x1,             // Flags
    15,                                     // DL Link
    &gITManagerCommonData,                           // Texture file

    // DObj transformation struct 1
    {
        nGCMatrixKindTra,                // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTra,                // Main matrix transformations
        0x2E,                               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerLoseKirbyStarProcUpdate,       // Proc Update
    lbCommonDObjScaleXProcDisplay,                     // Proc Render

    llITCommonDataKirbyStarDObjDesc,                 // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    0x0,                                    // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E770
EFDesc dEFManagerRebirthHaloEffectDesc =
{
    0x4 | EFFECT_FLAG_USERDATA,             // Flags
    10,                                     // DL Link
    &gEFManagerFiles[2],                      // Texture file

    // DObj transformation struct 1
    {
        0x50,                               // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        0x00,                               // Secondary matrix transformations
        0x00                                // ???
    },

    gcPlayAnimAll,                          // Proc Update
    gcDrawDObjTreeDLLinksForGObj,         // Proc Render

    llEFCommonEffects3RebirthHaloDObjDesc,               // DObj Setup attributes offset (?)
    0x0,                                    // MObjSub offset
    llEFCommonEffects3RebirthHaloAnimJoint,               // AnimJoint offset
    0x0                                     // MatAnimJoint offset
};

// 0x8012E798
EFDesc dEFManagerItemGetSwirlEffectDesc =
{
    0x4 | 0x1,                              // Flags
    18,                                     // DL Link
    &gEFManagerFiles[2],                      // Texture file

    // DObj transformation struct 1
    {
        0x28,                               // Main matrix transformations
        nGCMatrixKindNull,               // Secondary matrix transformations
        0x00                                // ???
    },

    // DObj transformation struct 2
    {
        nGCMatrixKindTraRotRpyRSca,      // Main matrix transformations
        0x00,                               // Secondary matrix transformations
        0x00                                // ???
    },

    efManagerHaveStructProcUpdate,     // Proc Update
    gcDrawDObjTreeDLLinksForGObj,         // Proc Render

    llEFCommonEffects3ItemGetSwirlDObjDesc,              // DObj Setup attributes offset (?)
    llEFCommonEffects3ItemGetSwirlMObjSub,                // MObjSub offset
    llEFCommonEffects3ItemGetSwirlAnimJoint,              // AnimJoint offset
    llEFCommonEffects3ItemGetSwirlMatAnimJoint            // MatAnimJoint offset
};

// // // // // // // // // // // //
//                               //
//   GLOBAL / STATIC VARIABLES   //
//                               //
// // // // // // // // // // // //

// 0x801313B0
void *gEFManagerFiles[3];

// 0x801313BC
EFStruct *sEFManagerStructsAllocFree;

// 0x801313C0
s32 sEFManagerStructsFreeNum;

// 0x801313C4
s32 gEFManagerParticleBankID;

#if defined(PORT) && defined(SSB64_NETMENU)
/* SSB64_NETMENU: stripped from offline builds. Base of the EFStruct pool,
 * captured at init so the netplay rollback free-guard can range-check pointers
 * returned to the free list. See efManagerNetSafeFreeStruct. */
static EFStruct *sEFManagerNetPoolBase;
#endif

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

/* 0x800FD300 - OLD NOTE: To match this, lbRelocGetExternHeapFile and lbRelocGetFileSize must take intptr_t or other int type as first argument
 *              NEW NOTE: Not entirely correct, their types do need to be identical however
 */
void efManagerInitEffects(void)
{
    EFStruct *ep;
    s32 i;
    s32 unused;

    sEFManagerStructsAllocFree = ep = syTaskmanMalloc(sizeof(EFStruct) * EFFECT_ALLOC_NUM, 0x8);
    sEFManagerStructsFreeNum = EFFECT_ALLOC_NUM;
#if defined(PORT) && defined(SSB64_NETMENU)
    sEFManagerNetPoolBase = ep;
#endif

    for (i = 0; i < (EFFECT_ALLOC_NUM - 1); i++)
    {
        ep[i].next = &ep[i + 1];
    }
    if (ep != NULL)
    {
        ep[i].next = NULL;
    }
    efDisplayMakeCLD();
    efDisplayMakeXLU();

    gEFManagerFiles[0] = lbRelocGetExternHeapFile(llEFCommonEffects1FileID, syTaskmanMalloc(lbRelocGetFileSize(llEFCommonEffects1FileID), 0x10));
    gEFManagerFiles[1] = lbRelocGetExternHeapFile(llEFCommonEffects2FileID, syTaskmanMalloc(lbRelocGetFileSize(llEFCommonEffects2FileID), 0x10));
    gEFManagerFiles[2] = lbRelocGetExternHeapFile(llEFCommonEffects3FileID, syTaskmanMalloc(lbRelocGetFileSize(llEFCommonEffects3FileID), 0x10));

    efDisplayInitAll();
}

// 0x800FD43C
EFStruct* efManagerGetNextStructAlloc(sb32 is_force_return)
{
    EFStruct *ep;

    if ((is_force_return == FALSE) && (sEFManagerStructsFreeNum < 5))
    {
        return NULL;
    }
    ep = sEFManagerStructsAllocFree;

    if (ep == NULL)
    {
#if defined(PORT) && defined(SSB64_NETMENU)
        if (sEFManagerStructsFreeNum > 0)
        {
            if (efManagerNetAuditPool(TRUE, "get_next_struct_alloc") != FALSE)
            {
                ep = sEFManagerStructsAllocFree;
            }
        }
#endif
        if (ep == NULL)
        {
            return NULL;
        }
    }
    sEFManagerStructsAllocFree = ep->next;

    ep->fighter_gobj = NULL;
    ep->xf = NULL;
    ep->is_pause_effect = FALSE;
    ep->proc_update = NULL;

    sEFManagerStructsFreeNum--;

    return ep;
}

// 0x800FD4B8
EFStruct* efManagerGetEffectNoForce(void)
{
    return efManagerGetNextStructAlloc(FALSE);
}

// 0x800FD4D8
EFStruct* efManagerGetEffectForce(void)
{
    return efManagerGetNextStructAlloc(TRUE);
}

#if defined(PORT) && defined(SSB64_NETMENU)
s32 efManagerGetEffectStructFreeCount(void)
{
    return sEFManagerStructsFreeNum;
}

static sb32 efManagerNetDiagEnabled(void)
{
    const char *env;

    env = getenv("SSB64_NETPLAY_REBIRTH_GATE_DIAG");
    return ((env != NULL) && (env[0] != '\0') && (env[0] != '0')) ? TRUE : FALSE;
}

static sb32 efManagerNetEpPoolIndex(EFStruct *ep, s32 *index_out)
{
    EFStruct *base = sEFManagerNetPoolBase;
    s32 index;

    if ((base == NULL) || (ep == NULL) || (ep < base) || (ep >= &base[EFFECT_ALLOC_NUM]) ||
        ((((u8 *)ep - (u8 *)base) % sizeof(EFStruct)) != 0))
    {
        return FALSE;
    }
    index = (s32)(ep - base);
    if (index_out != NULL)
    {
        *index_out = index;
    }
    return TRUE;
}

static void efManagerNetMarkPoolInUseFromEffectLinks(sb32 *in_use, s32 *in_use_out, s32 *live_gobj_out)
{
    s32 pass;
    s32 in_use_count = 0;
    s32 live_gobj_count = 0;

    if (in_use_out != NULL)
    {
        *in_use_out = 0;
    }
    if (live_gobj_out != NULL)
    {
        *live_gobj_out = 0;
    }
    for (pass = 0; pass < 2; pass++)
    {
        GObj *link_head = gGCCommonLinks[(pass == 0) ? nGCCommonLinkIDEffect : nGCCommonLinkIDSpecialEffect];
        GObj *gobj;

        for (gobj = link_head; gobj != NULL; gobj = gobj->link_next)
        {
            EFStruct *ep;
            s32 index;

            live_gobj_count++;
            ep = efGetStruct(gobj);
            if (efManagerNetEpPoolIndex(ep, &index) == FALSE)
            {
                continue;
            }
            if (in_use[index] == FALSE)
            {
                in_use[index] = TRUE;
                in_use_count++;
            }
        }
    }
    if (in_use_out != NULL)
    {
        *in_use_out = in_use_count;
    }
    if (live_gobj_out != NULL)
    {
        *live_gobj_out = live_gobj_count;
    }
}

static s32 efManagerNetCountFreeListNodes(void)
{
    EFStruct *cur = sEFManagerStructsAllocFree;
    s32 list_len = 0;
    s32 guard;

    for (guard = 0; (cur != NULL) && (guard <= EFFECT_ALLOC_NUM); guard++)
    {
        list_len++;
        cur = cur->next;
    }
    return list_len;
}

static sb32 efManagerNetRebuildFreeList(void)
{
    EFStruct *base = sEFManagerNetPoolBase;
    sb32 in_use[EFFECT_ALLOC_NUM];
    EFStruct *head = NULL;
    s32 free_count = 0;
    s32 i;

    if (base == NULL)
    {
        return FALSE;
    }
    for (i = 0; i < EFFECT_ALLOC_NUM; i++)
    {
        in_use[i] = FALSE;
    }
    efManagerNetMarkPoolInUseFromEffectLinks(in_use, NULL, NULL);
    for (i = EFFECT_ALLOC_NUM - 1; i >= 0; i--)
    {
        if (in_use[i] == FALSE)
        {
            base[i].fighter_gobj = NULL;
            base[i].xf = NULL;
            base[i].is_pause_effect = FALSE;
            base[i].proc_update = NULL;
            base[i].next = head;
            head = &base[i];
            free_count++;
        }
    }
    sEFManagerStructsAllocFree = head;
    sEFManagerStructsFreeNum = free_count;
    return TRUE;
}

static sb32 efManagerNetPoolMismatch(sb32 *list_len_out, s32 *in_use_out, s32 *live_gobj_out)
{
    sb32 in_use[EFFECT_ALLOC_NUM];
    s32 list_len;
    s32 in_use_count = 0;
    s32 live_gobj_count = 0;
    s32 i;

    for (i = 0; i < EFFECT_ALLOC_NUM; i++)
    {
        in_use[i] = FALSE;
    }
    list_len = efManagerNetCountFreeListNodes();
    efManagerNetMarkPoolInUseFromEffectLinks(in_use, &in_use_count, &live_gobj_count);
    if (list_len_out != NULL)
    {
        *list_len_out = list_len;
    }
    if (in_use_out != NULL)
    {
        *in_use_out = in_use_count;
    }
    if (live_gobj_out != NULL)
    {
        *live_gobj_out = live_gobj_count;
    }
    if (list_len != sEFManagerStructsFreeNum)
    {
        return TRUE;
    }
    if ((sEFManagerStructsFreeNum > 0) && (sEFManagerStructsAllocFree == NULL))
    {
        return TRUE;
    }
    if ((sEFManagerStructsFreeNum == 0) && (sEFManagerStructsAllocFree != NULL))
    {
        return TRUE;
    }
    if ((in_use_count + list_len) != EFFECT_ALLOC_NUM)
    {
        return TRUE;
    }
    return FALSE;
}

sb32 efManagerNetAuditPool(sb32 try_repair, const char *site)
{
    s32 list_len = 0;
    s32 in_use_count = 0;
    s32 live_gobj_count = 0;
    sb32 mismatch;

    if (sEFManagerNetPoolBase == NULL)
    {
        return TRUE;
    }
    mismatch = efManagerNetPoolMismatch(&list_len, &in_use_count, &live_gobj_count);
    if ((mismatch != FALSE) && (try_repair != FALSE))
    {
        port_log(
            "SSB64 EfManager: pool_audit mismatch site=%s free_num=%d list_len=%d alloc_free=%p in_use=%d live_eff_gobjs=%d -> repair\n",
            (site != NULL) ? site : "?",
            (int)sEFManagerStructsFreeNum,
            (int)list_len,
            (void *)sEFManagerStructsAllocFree,
            (int)in_use_count,
            (int)live_gobj_count);
        efManagerNetRebuildFreeList();
        mismatch = efManagerNetPoolMismatch(&list_len, &in_use_count, &live_gobj_count);
        if (mismatch == FALSE)
        {
            port_log(
                "SSB64 EfManager: pool_audit repaired site=%s free_num=%d list_len=%d alloc_free=%p in_use=%d live_eff_gobjs=%d\n",
                (site != NULL) ? site : "?",
                (int)sEFManagerStructsFreeNum,
                (int)list_len,
                (void *)sEFManagerStructsAllocFree,
                (int)in_use_count,
                (int)live_gobj_count);
        }
    }
    else if ((mismatch != FALSE) && (efManagerNetDiagEnabled() != FALSE))
    {
        port_log(
            "SSB64 EfManager: pool_audit mismatch site=%s free_num=%d list_len=%d alloc_free=%p in_use=%d live_eff_gobjs=%d\n",
            (site != NULL) ? site : "?",
            (int)sEFManagerStructsFreeNum,
            (int)list_len,
            (void *)sEFManagerStructsAllocFree,
            (int)in_use_count,
            (int)live_gobj_count);
    }
    return (mismatch == FALSE) ? TRUE : FALSE;
}

static void efManagerLogMakeEffectAllocFail(const char *step, const EFDesc *effect_desc, const void *caller)
{
    s32 list_len = 0;
    s32 in_use_count = 0;
    s32 live_gobj_count = 0;

    if (efManagerNetDiagEnabled() == FALSE)
    {
        return;
    }
    (void)efManagerNetPoolMismatch(&list_len, &in_use_count, &live_gobj_count);
    port_log(
        "SSB64 EfManager: make_effect_fail step=%s free=%d list_len=%d alloc_free=%p in_use=%d live_eff_gobjs=%d desc=%p caller=%p\n",
        (step != NULL) ? step : "?",
        (int)sEFManagerStructsFreeNum,
        (int)list_len,
        (void *)sEFManagerStructsAllocFree,
        (int)in_use_count,
        (int)live_gobj_count,
        (const void *)effect_desc,
        caller);
}
#endif

// 0x800FD4F8
void efManagerSetPrevStructAlloc(EFStruct *ep)
{
    ep->proc_update = NULL;
    ep->next = sEFManagerStructsAllocFree;

    sEFManagerStructsAllocFree = ep;

    sEFManagerStructsFreeNum++;
}

#if defined(PORT) && defined(SSB64_NETMENU)
/* SSB64_NETMENU: stripped from offline builds. Netplay rollback only.
 *
 * Validate an EFStruct before returning it to the singly-linked free list.
 * The rollback effect-reconcile path (port/net/sys/netrollbacksnapshot.c) can
 * otherwise push a stale/aliased EFStruct (two GObjs sharing one user_data.p)
 * or an out-of-pool pointer onto the free list; a later allocation then
 * dereferences ->next off that bad head and SIGSEGVs (observed in
 * efManagerQuakeMakeEffect during forward sim). Reject + log the offending
 * eject site instead of corrupting the pool. Returns TRUE if the struct was
 * actually returned to the pool. See
 * docs/bugs/netplay_quake_effect_pool_free_2026-06-28.md.
 *
 * Does NOT touch the decomp allocator's matching semantics — offline builds
 * (SSB64_NETMENU undefined) never compile this and free via the bare
 * efManagerSetPrevStructAlloc as before. */
sb32 efManagerNetSafeFreeStruct(EFStruct *ep, GObj *effect_gobj, const char *site)
{
    EFStruct *base = sEFManagerNetPoolBase;
    EFStruct *cur;
    s32 guard;
    u32 gobj_id = (effect_gobj != NULL) ? effect_gobj->id : 0U;

    if (ep == NULL)
    {
        return FALSE;
    }
    if (base != NULL)
    {
        if ((ep < base) || (ep >= &base[EFFECT_ALLOC_NUM]) ||
            ((((u8 *)ep - (u8 *)base) % sizeof(EFStruct)) != 0))
        {
            port_log("SSB64: ef_pool_free_reject reason=out_of_range site=%s gobj=%p id=%u ep=%p base=%p\n",
                     (site != NULL) ? site : "?", (void *)effect_gobj, gobj_id, (void *)ep, (void *)base);
            return FALSE;
        }
    }
    /* Walk the free list (bounded by pool size to survive a pre-existing cycle)
     * to reject a double-free before it can corrupt the chain. */
    cur = sEFManagerStructsAllocFree;
    for (guard = 0; (cur != NULL) && (guard <= EFFECT_ALLOC_NUM); guard++)
    {
        if (cur == ep)
        {
            port_log("SSB64: ef_pool_free_reject reason=double_free site=%s gobj=%p id=%u ep=%p free_num=%d\n",
                     (site != NULL) ? site : "?", (void *)effect_gobj, gobj_id, (void *)ep,
                     sEFManagerStructsFreeNum);
            return FALSE;
        }
        cur = cur->next;
    }
    efManagerSetPrevStructAlloc(ep);
    return TRUE;
}
#endif

// 0x800FD524
void efManagerNoStructProcUpdate(GObj *effect_gobj)
{
    gcPlayAnimAll(effect_gobj);

    if (effect_gobj->anim_frame <= 0.0F)
    {
        gcEjectGObj(effect_gobj);
    }
}

// 0x800FD568
void efManagerHaveStructProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);

    if (!(ep->is_pause_effect))
    {
        gcPlayAnimAll(effect_gobj);

        if (effect_gobj->anim_frame <= 0.0F)
        {
            efManagerSetPrevStructAlloc(efGetStruct(effect_gobj));

            gcEjectGObj(effect_gobj);
        }
    }
}

// New file? Unused
void func_ovl2_800FD5D0(void)
{
    return;
}

// 0x800FD5D8
void efManagerNoEjectProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);

    if (!(ep->is_pause_effect))
    {
        gcPlayAnimAll(effect_gobj);
    }
}

// 0x800FD60C
void efManagerSortZNeg(DObj *dobj)
{
    GObj *parent_gobj;

    if (dobj->translate.vec.f.z < -1000.0F)
    {
        parent_gobj = dobj->parent_gobj;

        if (parent_gobj->dl_link_id != 2)
        {
            gcMoveGObjDL(parent_gobj, 2, 2);
        }
    }
    else
    {
        parent_gobj = dobj->parent_gobj;

        if (parent_gobj->dl_link_id != 20)
        {
            gcMoveGObjDL(parent_gobj, 20, 2);
        }
    }
}

// 0x800FD68C
void efManagerSortZPos(DObj *dobj)
{
    GObj *parent_gobj;

    if (dobj->translate.vec.f.z > 1000.0F)
    {
        parent_gobj = dobj->parent_gobj;

        if (parent_gobj->dl_link_id != 2)
        {
            gcMoveGObjDL(parent_gobj, 2, 2);
        }
    }
    else
    {
        parent_gobj = dobj->parent_gobj;

        if (parent_gobj->dl_link_id != 20)
        {
            gcMoveGObjDL(parent_gobj, 20, 2);
        }
    }
}

// Another unused func
void func_ovl2_800FD70C(void)
{
    return;
}

// 0x800FD714
void efManagerFuncRun(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);

    if (ep != NULL)
    {
        if (ep->proc_update != NULL)
        {
            gcAddGObjProcess(effect_gobj, ep->proc_update, nGCProcessKindFunc, 3);
        }
    }
    else gcAddGObjProcess(effect_gobj, efManagerNoStructProcUpdate, nGCProcessKindFunc, 3);

    effect_gobj->func_run = NULL;
}

#ifdef PORT
/*
 * Pointer-plausibility guard for effect descriptors and their resolved
 * resource bases. A stale GObj or a mis-resolved effect descriptor can reach
 * efManagerMakeEffect holding a pointer whose low 32 bits were stomped with a
 * 0xFFFFFFFF sentinel (a 32-bit -1 written into a 64-bit pointer — benign on
 * the N64's 32-bit address space, fatal on LP64). This was observed as a
 * SIGSEGV inside efManagerMakeEffect during a Link bomb explosion
 * (fault_addr low 32 bits = 0xFFFFFFFF). The existing *file_head==NULL guard
 * below only catches the cleanly-NULLed case; this also rejects the stomped
 * non-NULL variant. Cheap heuristic: a usable pointer is past the NULL page
 * and does not carry an all-ones low word / all-ones value.
 */
static int syEfDescPtrPlausible(const void *p)
{
    uintptr_t v = (uintptr_t) p;

    if (v < 0x1000) {
        return 0; /* NULL / near-NULL page */
    }
    if ((v & 0xFFFFFFFFu) == 0xFFFFFFFFu) {
        return 0; /* low 32 bits stomped with a -1 sentinel */
    }
    if (v == (uintptr_t) -1) {
        return 0;
    }
    return 1;
}

static void efManagerLogImplausiblePtr(const char *what, const void *ptr, const void *caller)
{
    static int sWarnCount = 0;

    if (sWarnCount < 32) {
        sWarnCount++;
        port_log("SSB64: efManagerMakeEffect bail — implausible %s=%p caller=%p\n",
                 what, ptr, caller);
    }
}
#endif

// 0x800FD778
GObj* efManagerMakeEffect(EFDesc *effect_desc, sb32 is_force_return)
{
    GObj *effect_gobj;
    DObj *other_dobj;
    EFStruct *ep;
    s32 unused;
    DObj *main_dobj;
    DObj *child_dobj;
    DObjTransformTypes *transform_types1;
    DObjTransformTypes *transform_types2;
    u8 effect_flags;
    uintptr_t addr;
    uintptr_t o_mobjsub;
    uintptr_t o_anim_joint;
    uintptr_t o_matanim_joint;

#ifdef PORT
    /* Validate the descriptor itself before any dereference: effect_desc->flags
     * on the next line is the first read and faults if effect_desc is garbage.
     * Bail cleanly (nothing allocated yet) and log the caller so the next soak
     * names the offending effect. See docs/bugs/. */
    if (!syEfDescPtrPlausible(effect_desc)) {
        efManagerLogImplausiblePtr("effect_desc", effect_desc, __builtin_return_address(0));
        return NULL;
    }
#endif

    effect_flags = effect_desc->flags;

    if (effect_flags & EFFECT_FLAG_USERDATA)
    {
        ep = efManagerGetNextStructAlloc(is_force_return);

        if (ep == NULL)
        {
#if defined(PORT) && defined(SSB64_NETMENU)
            efManagerLogMakeEffectAllocFail("ef_struct", effect_desc, __builtin_return_address(0));
#endif
            return NULL;
        }
        ep->proc_update = effect_desc->proc_update;
    }
    else ep = NULL;

    effect_gobj = gcMakeGObjSPAfter(nGCCommonKindEffect, efManagerFuncRun, (effect_flags & EFFECT_FLAG_SPECIALLINK) ? nGCCommonLinkIDSpecialEffect : nGCCommonLinkIDEffect, GOBJ_PRIORITY_DEFAULT);

    if (effect_gobj == NULL)
    {
        if (ep != NULL)
        {
            efManagerSetPrevStructAlloc(ep);
        }
#if defined(PORT) && defined(SSB64_NETMENU)
        efManagerLogMakeEffectAllocFail("gobj", effect_desc, __builtin_return_address(0));
#endif
        return NULL;
    }
    effect_gobj->user_data.p = ep;

    if (effect_desc->proc_display == NULL)
    {
        return effect_gobj;
    }
    gcAddGObjDisplay(effect_gobj, effect_desc->proc_display, effect_desc->dl_link, 2, -1);

    o_mobjsub = effect_desc->o_mobjsub;
    o_anim_joint = effect_desc->o_anim_joint;
    o_matanim_joint = effect_desc->o_matanim_joint;
#ifdef PORT
    /* Defensive guard: the per-fighter data file global (e.g.
     * gFTDataKirbySpecial2) is dereferenced via *effect_desc->file_head to
     * locate the DObjDesc array. If the global has been NULLed because the
     * file isn't currently loaded — or, on Linux/glibc, never reset between
     * scene-arena reuse and stale-GObj firing — addr ends up pointing into
     * freed memory and downstream gcSetupCustomDObjs reads garbage tokens.
     * This was hit by a stale Kirby fighter from a prior auto-demo cycle
     * trying to spawn the Cutter Draw effect after the Kirby Special2 file
     * was unloaded (Linux-only SIGSEGV; Win/Mac allocators happened to leave
     * compatible memory at the same address). Bail safely if the global is
     * NULL/garbage so the caller's `!= NULL` check skips the effect cleanly.
     *
     * The file_head pointer itself is validated *before* the deref below, and
     * the resolved base (addr) is validated after, to also catch the LP64
     * 0xFFFFFFFF-stomped variant seen during the Link bomb explosion. */
    if (!syEfDescPtrPlausible(effect_desc->file_head)) {
        efManagerLogImplausiblePtr("file_head", effect_desc->file_head, __builtin_return_address(0));
        if (ep != NULL) {
            efManagerSetPrevStructAlloc(ep);
        }
        gcEjectGObj(effect_gobj);
        return NULL;
    }
    if (*effect_desc->file_head == NULL || !syEfDescPtrPlausible(*effect_desc->file_head)) {
        static int sBadFileHeadWarnCount = 0;
        if (sBadFileHeadWarnCount < 10) {
            sBadFileHeadWarnCount++;
            port_log("SSB64: efManagerMakeEffect bail — *file_head=%p (effect_desc=%p caller=%p)\n",
                     *effect_desc->file_head, effect_desc, __builtin_return_address(0));
        }
        if (ep != NULL) {
            efManagerSetPrevStructAlloc(ep);
        }
        gcEjectGObj(effect_gobj);
        return NULL;
    }
#endif
    addr = (uintptr_t) *effect_desc->file_head;

    transform_types1 = &effect_desc->transform_types1;

    if (effect_flags & 0x1)
    {
        main_dobj = gcAddDObjForGObj(effect_gobj, NULL);

        lbCommonInitDObj3Transforms(main_dobj, transform_types1->tk1, transform_types1->tk2, transform_types1->tk3);

        transform_types2 = &effect_desc->transform_types2;

        if (effect_flags & 0x4)
        {
            lbCommonSetupTreeDObjs(main_dobj, (DObjDesc*) (addr + effect_desc->o_dobjsetup), NULL, transform_types2->tk1, transform_types2->tk2, transform_types2->tk3);

            main_dobj = main_dobj->child;
        }
        else
        {
            main_dobj = gcAddChildForDObj(main_dobj, (void*) (addr + effect_desc->o_dobjsetup));

            lbCommonInitDObj3Transforms(main_dobj, transform_types2->tk1, transform_types2->tk2, transform_types2->tk3);
        }
        if (o_mobjsub != 0)
        {
            lbCommonAddMObjForTreeDObjs(main_dobj, (MObjSub***) (addr + o_mobjsub));
        }
        if ((o_anim_joint != 0) || (o_matanim_joint != 0))
        {
            lbCommonAddTreeDObjsAnimAll(main_dobj,
                (o_anim_joint != 0) ? (AObjEvent32**) (addr + o_anim_joint) : NULL,
                (o_matanim_joint != 0) ? (AObjEvent32***) (addr + o_matanim_joint) : NULL,
                0.0F);
            gcPlayAnimAll(effect_gobj);
        }
    }
    else
    {
        transform_types1 = &effect_desc->transform_types1;

        if (effect_flags & 0x4)
        {
            gcSetupCustomDObjs(effect_gobj, (void*)(addr + effect_desc->o_dobjsetup), NULL, nGCMatrixKindNull, nGCMatrixKindNull, nGCMatrixKindNull);

            other_dobj = DObjGetStruct(effect_gobj);

            gcAddDObj3TransformsKind(other_dobj, transform_types1->tk1, transform_types1->tk2, transform_types1->tk3);

            transform_types2 = &effect_desc->transform_types2;

            main_dobj = other_dobj->child;

            while (main_dobj != NULL)
            {
                gcAddDObj3TransformsKind(main_dobj, transform_types2->tk1, transform_types2->tk2, transform_types2->tk3);

                main_dobj = lbCommonGetTreeDObjNextFromRoot(main_dobj, other_dobj);
            }
            gcSetDObjTransformsForGObj(effect_gobj, (DObjDesc*) (addr + effect_desc->o_dobjsetup));
        }
        else
        {
            transform_types1 = &effect_desc->transform_types1;

            lbCommonInitDObj3Transforms(gcAddDObjForGObj(effect_gobj, (void*) (addr + effect_desc->o_dobjsetup)), transform_types1->tk1, transform_types1->tk2, transform_types1->tk3);
        }
        if (o_mobjsub != 0)
        {
            gcAddMObjAll(effect_gobj, (MObjSub***) (addr + o_mobjsub));
        }
        if ((o_anim_joint != 0) || (o_matanim_joint != 0))
        {
            gcAddAnimAll(effect_gobj, (o_anim_joint != 0) ? (AObjEvent32**) (addr + o_anim_joint) : NULL, (o_matanim_joint != 0) ? (AObjEvent32***) (addr + o_matanim_joint) : NULL, 0.0F);
            gcPlayAnimAll(effect_gobj);
        }
    }
    return effect_gobj;
}

// 0x800FDAFC
GObj* efManagerMakeEffectNoForce(EFDesc *effect_desc)
{
    return efManagerMakeEffect(effect_desc, FALSE);
}

// 0x800FDB1C
GObj* efManagerMakeEffectForce(EFDesc *effect_desc)
{
    return efManagerMakeEffect(effect_desc, TRUE);
}

// 0x800FDB3C - Destroy effect GObj and particle too if applicable
LBParticle* efManagerDestroyParticleGObj(LBParticle *pc, GObj *effect_gobj)
{
    if (pc != NULL)
    {
        lbParticleEjectStruct(pc);
    }
    if (efGetStruct(effect_gobj) != NULL)
    {
        EFStruct *ep = efGetStruct(effect_gobj);

        efManagerSetPrevStructAlloc(ep);
    }
    gcEjectGObj(effect_gobj);

    return NULL;
}

// 0x800FDB88
void efManagerDefaultProcDead(LBTransform *xf)
{
    if (efGetStruct(xf->effect_gobj) != NULL)
    {
        EFStruct *ep = efGetStruct(xf->effect_gobj);

        efManagerSetPrevStructAlloc(ep);
    }
    gcEjectGObj(xf->effect_gobj);
}

// 0x800FDBCC
void efManagerDefaultProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);

#if defined(PORT) && defined(SSB64_NETMENU)
    /* Netplay rollback/reconcile can leave a non-NULL but stale LBTransform pointer. */
    if (ep == NULL)
    {
        efManagerNetplayEjectStaleXfEffect(effect_gobj, NULL, NULL, "DefaultProcUpdate", "null_ep");
        return;
    }
    {
        LBTransform *xf = ep->effect_vars.common.xf;
        const char *reason;

        if (efManagerNetplayEffectXfIsLive(effect_gobj, xf, &reason) == FALSE)
        {
            efManagerNetplayEjectStaleXfEffect(effect_gobj, ep, xf, "DefaultProcUpdate", reason);
            return;
        }
        if ((xf->effect_gobj != effect_gobj) || (lbParticleTransformIsAllocated(xf) == FALSE))
        {
            efManagerNetplayEjectStaleXfEffect(effect_gobj, ep, xf, "DefaultProcUpdate", "pre_write_race");
            return;
        }
        xf->translate.x += ep->effect_vars.common.vel.x;
        xf->translate.y += ep->effect_vars.common.vel.y;
        return;
    }
#endif
#ifdef PORT
    /* SR-character hit effects spawned from collision-resolve paths
     * (gmCollisionResolveAttackerWeapon and siblings) can race past the
     * LBParticleAddTransformForStruct that normally populates
     * effect_vars.common.xf. The vanilla allocator runs LBParticleProcess-
     * Struct synchronously and the user_num check rejects the spawn when
     * xf isn't set, but synth-fkind weapon spawns route through paths
     * where effect_vars never gets initialized at all (different EFStruct
     * variant, common is union-overlapped with damage_normal_heavy.size).
     * The bare-pointer access here AVs on first frame of any synth hit. */
    if (ep == NULL || ep->effect_vars.common.xf == NULL) return;
    /* effect_vars is a union that overlays common.xf with several other
     * variants (damage_normal_heavy.size and friends). For synth-fighter
     * weapon spawns the union is initialized to a size variant, so xf
     * dereferences a non-NULL but invalid pointer. The corrupt pointer
     * pattern observed: high 32 bits leak from an adjacent heap page
     * address (~0x024F...) and low 32 bits are 0xFFFFFFFF garbage. Reject
     * anything below the low-userspace threshold, anything with the
     * low-16 pattern that screams "garbage" (low 16 bits all set, or
     * misaligned for a struct pointer). Real LBTransform pointers come
     * from a pool aligned at least to 4 bytes. */
    {
        uintptr_t xf_addr = (uintptr_t)ep->effect_vars.common.xf;
        if (xf_addr < 0x10000u) return;
        if ((xf_addr & 0xFFFFu) == 0xFFFFu) return;
        if ((xf_addr & 0x3u) != 0u) return;
    }
#endif

#if defined(PORT) && defined(SSB64_NETMENU)
    /* Netplay rollback/reconcile can leave a non-NULL but stale LBTransform pointer. */
    if (ep == NULL)
    {
        efManagerNetplayEjectStaleXfEffect(effect_gobj, NULL, NULL, "DefaultProcUpdate", "null_ep");
        return;
    }
    {
        LBTransform *xf = ep->effect_vars.common.xf;
        const char *reason;

        if (efManagerNetplayEffectXfIsLive(effect_gobj, xf, &reason) == FALSE)
        {
            efManagerNetplayEjectStaleXfEffect(effect_gobj, ep, xf, "DefaultProcUpdate", reason);
            return;
        }
        if ((xf->effect_gobj != effect_gobj) || (lbParticleTransformIsAllocated(xf) == FALSE))
        {
            efManagerNetplayEjectStaleXfEffect(effect_gobj, ep, xf, "DefaultProcUpdate", "pre_write_race");
            return;
        }
        xf->translate.x += ep->effect_vars.common.vel.x;
        xf->translate.y += ep->effect_vars.common.vel.y;
        return;
    }
#endif
    ep->effect_vars.common.xf->translate.x += ep->effect_vars.common.vel.x;
    ep->effect_vars.common.xf->translate.y += ep->effect_vars.common.vel.y;
}

// 0x800FDBFC - Unused
void func_ovl2_800FDBFC(void)
{
    return;
}

// 0x800FDC04
LBParticle* efManagerDamageNormalLightMakeEffect(Vec3f *pos, s32 player, s32 size, sb32 is_static)
{
    GObj *effect_gobj;
    LBParticle *pc;
    LBTransform *xf;
    EFStruct *ep;
    f32 angle;
    f32 vel;
    f32 scale;

    ep = efManagerGetEffectNoForce();

    if (ep == NULL)
    {
        return NULL;
    }
    effect_gobj = gcMakeGObjSPAfter(nGCCommonKindEffect, NULL, nGCCommonLinkIDEffect, GOBJ_PRIORITY_DEFAULT);

    if (effect_gobj == NULL)
    {
        efManagerSetPrevStructAlloc(ep);

        return NULL;
    }
    effect_gobj->user_data.p = ep;

    pc = lbParticleMakeScriptID(gEFManagerParticleBankID, dEFManagerDamageNormalLightIDs[
#ifdef PORT
        /* sibling per-player tables (Heavy/ImpactWave) have 5 entries to handle attacker_player == 4
           (boss / no-attacker / 5th-slot); this IDs table was authored 4-wide. clamp instead of OOB. */
        (u32)player < ARRAY_COUNT(dEFManagerDamageNormalLightIDs) ? player : 0
#else
        player
#endif
    ]);

    if (pc != NULL)
    {
        xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusDefault);

        if (xf != NULL)
        {
            xf->effect_gobj = effect_gobj;
            xf->proc_dead = efManagerDefaultProcDead;

            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            gcAddGObjProcess(effect_gobj, efManagerDefaultProcUpdate, nGCProcessKindFunc, 3);

            ep->effect_vars.common.xf = xf;

            xf->translate = *pos;

            vel = (is_static != FALSE) ? 0.0F : ((syUtilsRandFloat() * 38.0F) + 12.0F);

            angle = syUtilsRandFloat() * F_CLC_DTOR32(360.0F);

            ep->effect_vars.common.vel.x = __cosf(angle) * vel;
            ep->effect_vars.common.vel.y = __sinf(angle) * vel;

            scale = (size < 10) ? (((10 - size) * -0.05F) + 1.0F) : (((size - 10) * 0.13F) + 1.0F);

            xf->scale.x = xf->scale.y = xf->scale.z = scale;
        }
        else pc = efManagerDestroyParticleGObj(pc, effect_gobj);
    }
    else efManagerDestroyParticleGObj(NULL, effect_gobj);

    return pc;
}

// 0x800FDE3C
void efManagerDamageNormalHeavyProcDead(LBTransform *xf)
{
    EFStruct *ep = efGetStruct(xf->effect_gobj);
    Vec3f pos = xf->translate;

    efManagerDamageNormalLightMakeEffect(&pos, ep->effect_vars.damage_normal_heavy.player, ep->effect_vars.damage_normal_heavy.size, FALSE);
    efManagerSetPrevStructAlloc(efGetStruct(xf->effect_gobj));
    gcEjectGObj(xf->effect_gobj);
}

// 0x800FDEAC
LBParticle* efManagerDamageNormalHeavyMakeEffect(Vec3f *pos, s32 player, s32 size)
{
    GObj *effect_gobj;
    LBParticle *pc;
    LBTransform *xf;
    EFStruct *ep;

    ep = efManagerGetEffectNoForce();

    if (ep == NULL)
    {
        return NULL;
    }
    effect_gobj = gcMakeGObjSPAfter(nGCCommonKindEffect, NULL, nGCCommonLinkIDEffect, GOBJ_PRIORITY_DEFAULT);

    if (effect_gobj == NULL)
    {
        efManagerSetPrevStructAlloc(ep);

        return NULL;
    }
    effect_gobj->user_data.p = ep;

    pc = lbParticleMakeScriptID(gEFManagerParticleBankID, 0x64);

    if (pc != NULL)
    {
        xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusDefault);

        if (xf != NULL)
        {
            xf->effect_gobj = effect_gobj;
            xf->proc_dead = efManagerDamageNormalHeavyProcDead;

            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return FALSE;
            }
            ep->effect_vars.common.xf = xf; // WHAT (This overlaps with damage_normal_heavy.size and is eventually overwritten with the correct value)

            xf->translate = *pos;

            ep->effect_vars.damage_normal_heavy.pos = *pos;
            ep->effect_vars.damage_normal_heavy.player = player;
            ep->effect_vars.damage_normal_heavy.size = size;

            pc->primcolor.r = dEFManagerDamageNormalHeavyPrimColorR[player];
            pc->primcolor.g = dEFManagerDamageNormalHeavyPrimColorG[player];
            pc->primcolor.b = dEFManagerDamageNormalHeavyPrimColorB[player];
            pc->primcolor.a = 0xFF;

            pc->envcolor.r = dEFManagerDamageNormalHeavyEnvColorR[player];
            pc->envcolor.g = dEFManagerDamageNormalHeavyEnvColorG[player];
            pc->envcolor.b = dEFManagerDamageNormalHeavyEnvColorB[player];
        }
        else pc = efManagerDestroyParticleGObj(pc, effect_gobj);
    }
    else efManagerDestroyParticleGObj(NULL, effect_gobj);

    return pc;
}

// 0x800FE068
LBParticle* efManagerImpactShockMakeEffect(Vec3f *pos, s32 size)
{
    GObj *effect_gobj;
    LBParticle *pc;
    LBTransform *xf;
    EFStruct *ep;
    f32 angle;
    f32 vel;
    f32 scale;

    ep = efManagerGetEffectNoForce();

    if (ep == NULL)
    {
        return NULL;
    }
    effect_gobj = gcMakeGObjSPAfter(nGCCommonKindEffect, NULL, nGCCommonLinkIDEffect, GOBJ_PRIORITY_DEFAULT);

    if (effect_gobj == NULL)
    {
        efManagerSetPrevStructAlloc(ep);

        return NULL;
    }
    effect_gobj->user_data.p = ep;

    pc = lbParticleMakeScriptID(gEFManagerParticleBankID, 0x25);

    if (pc != NULL)
    {
        xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusDefault);

        if (xf != NULL)
        {
            xf->effect_gobj = effect_gobj;
            xf->proc_dead = efManagerDefaultProcDead;

            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            gcAddGObjProcess(effect_gobj, efManagerDefaultProcUpdate, nGCProcessKindFunc, 3);

            ep->effect_vars.common.xf = xf;

            xf->translate = *pos;

            vel = ((syUtilsRandFloat() * 8.0F) + 2.0F);

            angle = syUtilsRandFloat() * F_CLC_DTOR32(360.0F);

            ep->effect_vars.common.vel.x = __cosf(angle) * vel;
            ep->effect_vars.common.vel.y = __sinf(angle) * vel;

            scale = (size < 10) ? (((10 - size) * -0.05F) + 1.0F) : (((size - 10) * 0.15F) + 1.0F);

            xf->scale.x = xf->scale.y = xf->scale.z = scale;
        }
        else pc = efManagerDestroyParticleGObj(pc, effect_gobj);
    }
    else efManagerDestroyParticleGObj(NULL, effect_gobj);

    return pc;
}

// 0x800FE260
void efManagerVelAddDestroyAnimEnd(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);
    DObj *dobj = DObjGetStruct(effect_gobj);

    gcPlayAnimAll(effect_gobj);

    if (dobj->mobj->anim_frame <= 0.0F)
    {
        efManagerSetPrevStructAlloc(ep);
        gcEjectGObj(effect_gobj);
    }
    else
    {
        dobj->translate.vec.f.x += ep->effect_vars.common.vel.x;
        dobj->translate.vec.f.y += ep->effect_vars.common.vel.y;
    }
}

// 0x800FE2F4
LBParticle* efManagerDamageFireMakeEffect(Vec3f *pos, s32 size)
{
    GObj *effect_gobj;
    LBParticle *pc;
    LBTransform *xf;
    EFStruct *ep;
    f32 angle;
    f32 vel;
    f32 scale;

    ep = efManagerGetEffectNoForce();

    if (ep == NULL)
    {
        return NULL;
    }
    effect_gobj = gcMakeGObjSPAfter(nGCCommonKindEffect, NULL, nGCCommonLinkIDEffect, GOBJ_PRIORITY_DEFAULT);

    if (effect_gobj == NULL)
    {
        efManagerSetPrevStructAlloc(ep);

        return NULL;
    }
    effect_gobj->user_data.p = ep;

    pc = lbParticleMakeScriptID(gEFManagerParticleBankID, 0x4D);

    if (pc != NULL)
    {
        xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusDefault);

        if (xf != NULL)
        {
            xf->effect_gobj = effect_gobj;
            xf->proc_dead = efManagerDefaultProcDead;

            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            gcAddGObjProcess(effect_gobj, efManagerDefaultProcUpdate, nGCProcessKindFunc, 3);

            ep->effect_vars.common.xf = xf;

            xf->translate = *pos;

            vel = ((syUtilsRandFloat() * 18.0F) + 12.0F);

            angle = syUtilsRandFloat() * F_CLC_DTOR32(360.0F);

            ep->effect_vars.common.vel.x = __cosf(angle) * vel;
            ep->effect_vars.common.vel.y = __sinf(angle) * vel;

            scale = (size < 10) ? (((10 - size) * -0.05F) + 1.0F) : (((size - 10) * 0.15F) + 1.0F);

            xf->scale.x = xf->scale.y = xf->scale.z = scale;
        }
        else pc = efManagerDestroyParticleGObj(pc, effect_gobj);
    }
    else efManagerDestroyParticleGObj(NULL, effect_gobj);

    return pc;
}

// 0x800FE4EC
LBParticle* efManagerDamageElectricMakeEffect(Vec3f *pos, s32 size)
{
    GObj *effect_gobj;
    LBParticle *pc;
    LBTransform *xf;
    EFStruct *ep;
    f32 angle;
    f32 vel;
    f32 scale;

    ep = efManagerGetEffectNoForce();

    if (ep == NULL)
    {
        return NULL;
    }
    effect_gobj = gcMakeGObjSPAfter(nGCCommonKindEffect, NULL, nGCCommonLinkIDEffect, GOBJ_PRIORITY_DEFAULT);

    if (effect_gobj == NULL)
    {
        efManagerSetPrevStructAlloc(ep);

        return NULL;
    }
    effect_gobj->user_data.p = ep;

    pc = lbParticleMakeScriptID(gEFManagerParticleBankID, 0x53);

    if (pc != NULL)
    {
        xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusDefault);

        if (xf != NULL)
        {
            xf->effect_gobj = effect_gobj;
            xf->proc_dead = efManagerDefaultProcDead;

            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            gcAddGObjProcess(effect_gobj, efManagerDefaultProcUpdate, nGCProcessKindFunc, 3);

            ep->effect_vars.common.xf = xf;

            xf->translate = *pos;

            vel = (syUtilsRandFloat() * 7.0F) + 3.0F;

            angle = syUtilsRandFloat() * F_CLC_DTOR32(360.0F);

            ep->effect_vars.common.vel.x = __cosf(angle) * vel;
            ep->effect_vars.common.vel.y = __sinf(angle) * vel;

            scale = (size < 5) ? (((5 - size) * -0.08F) + 1.0F) : (((size - 5) * 0.15F) + 1.0F);

            xf->scale.x = xf->scale.y = xf->scale.z = scale;
        }
        else pc = efManagerDestroyParticleGObj(pc, effect_gobj);
    }
    else efManagerDestroyParticleGObj(NULL, effect_gobj);

    return pc;
}

// 0x800FE6E4
GObj* efManagerDamageSlashMakeEffect(Vec3f *pos, s32 size, f32 rotate)
{
    GObj *effect_gobj;
    DObj *dobj;
    f32 scale;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerDamageSlashEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);

    dobj->translate.vec.f = *pos;

    dobj->rotate.vec.f.z = rotate;

    scale = (size < 5) ? (((5 - size) * -0.08F) + 1.0F) : (((size - 5) * 0.18F) + 1.0F);

    dobj->scale.vec.f.x = dobj->scale.vec.f.y = scale;

    return effect_gobj;
}

// 0x800FE9B4
LBParticle* efManagerFlameLRMakeEffect(Vec3f *pos, s32 lr)
{
    GObj *effect_gobj;
    LBParticle *pc;
    LBTransform *xf;
    EFStruct *ep;
    f32 angle;
    f32 vel;
    f32 scale;

    ep = efManagerGetEffectNoForce();

    if (ep == NULL)
    {
        return NULL;
    }
    effect_gobj = gcMakeGObjSPAfter(nGCCommonKindEffect, NULL, nGCCommonLinkIDEffect, GOBJ_PRIORITY_DEFAULT);

    if (effect_gobj == NULL)
    {
        efManagerSetPrevStructAlloc(ep);

        return NULL;
    }
    effect_gobj->user_data.p = ep;

    pc = lbParticleMakeScriptID(gEFManagerParticleBankID, 0x12);

    if (pc != NULL)
    {
        xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusDefault);

        if (xf != NULL)
        {
            xf->effect_gobj = effect_gobj;
            xf->proc_dead = efManagerDefaultProcDead;

            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            gcAddGObjProcess(effect_gobj, efManagerDefaultProcUpdate, nGCProcessKindFunc, 3);

            ep->effect_vars.common.xf = xf;

            xf->translate = *pos;

            xf->translate.x += ((syUtilsRandFloat() * EFCOMMON_FLAMELR_OFF_X_BASE) + EFCOMMON_FLAMELR_OFF_X_ADD);
            xf->translate.y += ((syUtilsRandFloat() * EFCOMMON_FLAMELR_OFF_Y_BASE) + EFCOMMON_FLAMELR_OFF_Y_ADD);

            angle = syUtilsRandFloat() * F_CLC_DTOR32(90.0F);

            ep->effect_vars.common.vel.x = __cosf(angle) * EFCOMMON_FLAMELR_VEL_BASE * -lr;
            ep->effect_vars.common.vel.y = __sinf(angle) * EFCOMMON_FLAMELR_VEL_BASE;

            xf->scale.x = xf->scale.y = xf->scale.z = (syUtilsRandFloat() * 1) + 1.0F;
        }
        else pc = efManagerDestroyParticleGObj(pc, effect_gobj);
    }
    else efManagerDestroyParticleGObj(NULL, effect_gobj);

    return pc;
}

// 0x800FE9B4
LBParticle* efManagerFlameRandgcMakeEffect(Vec3f *pos)
{
    GObj *effect_gobj;
    LBParticle *pc;
    LBTransform *xf;
    EFStruct *ep;
    f32 angle;
    f32 vel;
    f32 scale;

    ep = efManagerGetEffectNoForce();

    if (ep == NULL)
    {
        return NULL;
    }
    effect_gobj = gcMakeGObjSPAfter(nGCCommonKindEffect, NULL, nGCCommonLinkIDEffect, GOBJ_PRIORITY_DEFAULT);

    if (effect_gobj == NULL)
    {
        efManagerSetPrevStructAlloc(ep);

        return NULL;
    }
    effect_gobj->user_data.p = ep;

    pc = lbParticleMakeScriptID(gEFManagerParticleBankID, 0x55);

    if (pc != NULL)
    {
        xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusDefault);

        if (xf != NULL)
        {
            xf->effect_gobj = effect_gobj;
            xf->proc_dead = efManagerDefaultProcDead;

            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            gcAddGObjProcess(effect_gobj, efManagerDefaultProcUpdate, nGCProcessKindFunc, 3);

            ep->effect_vars.common.xf = xf;

            xf->translate = *pos;

            angle = (syUtilsRandFloat() * EFCOMMON_FLAMERANDOM_ANGLE_BASE) + EFCOMMON_FLAMERANDOM_ANGLE_ADD;

            ep->effect_vars.common.vel.x = __cosf(angle) * EFCOMMON_FLAMERANDOM_VEL_BASE;
            ep->effect_vars.common.vel.y = __sinf(angle) * EFCOMMON_FLAMERANDOM_VEL_BASE;

            xf->scale.x = xf->scale.y = xf->scale.z = (syUtilsRandFloat() * 1) + 1.0F;
        }
        else pc = efManagerDestroyParticleGObj(pc, effect_gobj);
    }
    else efManagerDestroyParticleGObj(NULL, effect_gobj);

    return pc;
}

// 0x800FEB58
LBParticle* efManagerFlameStaticMakeEffect(Vec3f *pos)
{
    GObj *effect_gobj;
    LBParticle *pc;
    LBTransform *xf;
    EFStruct *ep;
    f32 angle;
    f32 vel;
    f32 scale;

    ep = efManagerGetEffectNoForce();

    if (ep == NULL)
    {
        return NULL;
    }
    effect_gobj = gcMakeGObjSPAfter(nGCCommonKindEffect, NULL, nGCCommonLinkIDEffect, GOBJ_PRIORITY_DEFAULT);

    if (effect_gobj == NULL)
    {
        efManagerSetPrevStructAlloc(ep);

        return NULL;
    }
    effect_gobj->user_data.p = ep;

    pc = lbParticleMakeScriptID(gEFManagerParticleBankID, 0x55);

    if (pc != NULL)
    {
        xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusDefault);

        if (xf != NULL)
        {
            xf->effect_gobj = effect_gobj;
            xf->proc_dead = efManagerDefaultProcDead;

            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            gcAddGObjProcess(effect_gobj, efManagerDefaultProcUpdate, nGCProcessKindFunc, 3);

            ep->effect_vars.common.xf = xf;

            xf->translate = *pos;

            ep->effect_vars.common.vel.x = 0.0F;
            ep->effect_vars.common.vel.y = 0.0F;

            xf->scale.x = xf->scale.y = xf->scale.z = (syUtilsRandFloat() * 1) + 1.0F;
        }
        else pc = efManagerDestroyParticleGObj(pc, effect_gobj);
    }
    else efManagerDestroyParticleGObj(NULL, effect_gobj);

    return pc;
}

// 0x800FECBC - Called only by Venusaur and Link's Boomerang?
LBParticle* efManagerDustCollideMakeEffect(Vec3f *pos)
{
    GObj *effect_gobj;
    LBParticle *pc;
    LBTransform *xf;
    EFStruct *ep;
    f32 angle;
    f32 vel;
    f32 scale;

    ep = efManagerGetEffectNoForce();

    if (ep == NULL)
    {
        return NULL;
    }
    effect_gobj = gcMakeGObjSPAfter(nGCCommonKindEffect, NULL, nGCCommonLinkIDEffect, GOBJ_PRIORITY_DEFAULT);

    if (effect_gobj == NULL)
    {
        efManagerSetPrevStructAlloc(ep);

        return NULL;
    }
    effect_gobj->user_data.p = ep;

    pc = lbParticleMakeScriptID(gEFManagerParticleBankID, 0x55);

    if (pc != NULL)
    {
        xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusDefault);

        if (xf != NULL)
        {
            xf->effect_gobj = effect_gobj;
            xf->proc_dead = efManagerDefaultProcDead;

            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            gcAddGObjProcess(effect_gobj, efManagerDefaultProcUpdate, nGCProcessKindFunc, 3);

            ep->effect_vars.common.xf = xf;

            xf->translate = *pos;

            xf->translate.x += ((syUtilsRandFloat() * EFCOMMON_DUSTCOLL_OFF_BASE) + EFCOMMON_DUSTCOLL_OFF_ADD);
            xf->translate.y += ((syUtilsRandFloat() * EFCOMMON_DUSTCOLL_OFF_BASE) + EFCOMMON_DUSTCOLL_OFF_ADD);

            angle = (syUtilsRandFloat() * EFCOMMON_DUSTCOLL_ANGLE_BASE) + EFCOMMON_DUSTCOLL_ANGLE_ADD; // F_CLC_DTOR32(90.0F), QUART_PI32

            ep->effect_vars.common.vel.x = __cosf(angle) * EFCOMMON_DUSTCOLL_VEL_BASE;
            ep->effect_vars.common.vel.y = __sinf(angle) * EFCOMMON_DUSTCOLL_VEL_BASE;

            xf->scale.x = xf->scale.y = xf->scale.z = (syUtilsRandFloat() * 1) + 1.0F;
        }
        else pc = efManagerDestroyParticleGObj(pc, effect_gobj);
    }
    else efManagerDestroyParticleGObj(NULL, effect_gobj);

    return pc;
}

// 0x800FEEB0
GObj* efManagerShockSmallMakeEffect(Vec3f *pos)
{
    GObj *effect_gobj;
    DObj *dobj;
    EFStruct *ep;
    f32 scale;
    f32 angle;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerShockSmallEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);
    ep = efGetStruct(effect_gobj);

    pos->x += (syUtilsRandFloat() * EFCOMMON_SHOCKSMALL_OFF_BASE) + EFCOMMON_SHOCKSMALL_OFF_ADD;
    pos->y += (syUtilsRandFloat() * EFCOMMON_SHOCKSMALL_OFF_BASE) + EFCOMMON_SHOCKSMALL_OFF_ADD;

    dobj->translate.vec.f = *pos;

    /*
        The following float random and multiplication, cos and sin subroutine calls are nothing but a waste of CPU cycles.
        Guarded by the preprocessor flag DAIRANTOU_OPT0, a codebase optimization level that ignores redundant lines of code.
    */

#if !defined (DAIRANTOU_OPT0)
    angle = syUtilsRandFloat() * F_CLC_DTOR32(360.0F); // F_CLC_DTOR32(360.0F)
#endif

#if !defined (DAIRANTOU_OPT0)
    __cosf(angle);
#endif

    ep->effect_vars.common.vel.x = 0.0F;

#if !defined (DAIRANTOU_OPT0)
    __sinf(angle);
#endif

    ep->effect_vars.common.vel.y = 0.0F;

    scale = (syUtilsRandFloat() * EFCOMMON_SHOCKSMALL_SCALE_BASE) + EFCOMMON_SHOCKSMALL_SCALE_ADD;

    dobj->scale.vec.f.x = dobj->scale.vec.f.y = scale;

    dobj->rotate.vec.f.z = syUtilsRandFloat() * F_CLC_DTOR32(360.0F); // F_CLC_DTOR32(360.0F)

    return effect_gobj;
}

// 0x800FEFE0
void efManagerDustLightProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);

#if defined(PORT) && defined(SSB64_NETMENU)
    if (ep == NULL)
    {
        efManagerNetplayEjectStaleXfEffect(effect_gobj, NULL, NULL, "DustLightProcUpdate", "null_ep");
        return;
    }
    {
        LBTransform *xf = ep->effect_vars.dust_light.xf;
        const char *reason;

        if (efManagerNetplayEffectXfIsLive(effect_gobj, xf, &reason) == FALSE)
        {
            efManagerNetplayEjectStaleXfEffect(effect_gobj, ep, xf, "DustLightProcUpdate", reason);
            return;
        }
        if ((xf->effect_gobj != effect_gobj) || (lbParticleTransformIsAllocated(xf) == FALSE))
        {
            efManagerNetplayEjectStaleXfEffect(effect_gobj, ep, xf, "DustLightProcUpdate", "pre_write_race");
            return;
        }
        xf->translate.x += ep->effect_vars.dust_light.vel1.x;
        xf->translate.y += ep->effect_vars.dust_light.vel1.y;

        if (ep->effect_vars.dust_light.lifetime != 0)
        {
            ep->effect_vars.dust_light.lifetime--;

            ep->effect_vars.dust_light.vel1.x += ep->effect_vars.dust_light.vel2.x;
            ep->effect_vars.dust_light.vel1.y += ep->effect_vars.dust_light.vel2.y;
        }
        return;
    }
#endif
    ep->effect_vars.dust_light.xf->translate.x += ep->effect_vars.dust_light.vel1.x;
    ep->effect_vars.dust_light.xf->translate.y += ep->effect_vars.dust_light.vel1.y;

    if (ep->effect_vars.dust_light.lifetime != 0)
    {
        ep->effect_vars.dust_light.lifetime--;

        ep->effect_vars.dust_light.vel1.x += ep->effect_vars.dust_light.vel2.x;
        ep->effect_vars.dust_light.vel1.y += ep->effect_vars.dust_light.vel2.y;
    }
}

// 0x800FF048
LBParticle* efManagerDustLightMakeEffect(Vec3f *pos, s32 lr, f32 f_index)
{
    GObj *effect_gobj;
    LBParticle *pc;
    LBTransform *xf;
    EFStruct *ep;
    f32 angle;
    f32 vel;
    f32 scale;

    ep = efManagerGetEffectNoForce();

    if (ep == NULL)
    {
        return NULL;
    }
    effect_gobj = gcMakeGObjSPAfter(nGCCommonKindEffect, NULL, nGCCommonLinkIDEffect, GOBJ_PRIORITY_DEFAULT);

    if (effect_gobj == NULL)
    {
        efManagerSetPrevStructAlloc(ep);

        return NULL;
    }
    effect_gobj->user_data.p = ep;

    pc = (f_index == 2.0F) ? lbParticleMakeScriptID(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0x56) : lbParticleMakeScriptID(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0x55);

    if (pc != NULL)
    {
        xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusDefault);

        if (xf != NULL)
        {
            xf->effect_gobj = effect_gobj;
            xf->proc_dead = efManagerDefaultProcDead;

            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            gcAddGObjProcess(effect_gobj, efManagerDustLightProcUpdate, nGCProcessKindFunc, 3);

            ep->effect_vars.dust_light.xf = xf;

            xf->translate = *pos;

            xf->translate.y += EFCOMMON_DUSTNORMAL_OFF_Y;

            xf->rotate.z = syUtilsRandFloat() * F_CLC_DTOR32(360.0F); // F_CLC_DTOR32(360.0F)

            angle = (syUtilsRandFloat() * EFCOMMON_DUSTNORMAL_ANGLE_BASE) + EFCOMMON_DUSTNORMAL_ANGLE_ADD;

            ep->effect_vars.dust_light.vel1.x = __cosf(angle) * EFCOMMON_DUSTNORMAL_VEL_BASE;

            if (lr == +1)
            {
                ep->effect_vars.dust_light.vel1.x = -ep->effect_vars.dust_light.vel1.x;
            }
            ep->effect_vars.dust_light.vel1.y = __sinf(angle) * EFCOMMON_DUSTNORMAL_VEL_BASE;

            ep->effect_vars.dust_light.lifetime = EFCOMMON_DUSTNORMAL_LIFETIME;

            ep->effect_vars.dust_light.vel2.x = -ep->effect_vars.dust_light.vel1.x * EFCOMMON_DUSTNORMAL_SCATTER;
            ep->effect_vars.dust_light.vel2.y = -ep->effect_vars.dust_light.vel1.y * EFCOMMON_DUSTNORMAL_SCATTER;
        }
        else pc = efManagerDestroyParticleGObj(pc, effect_gobj);
    }
    else efManagerDestroyParticleGObj(NULL, effect_gobj);

    return pc;
}

// 0x800FF278
LBParticle* efManagerDustHeavyMakeEffect(Vec3f *pos, s32 lr)
{
    GObj *effect_gobj;
    LBParticle *pc;
    LBTransform *xf;
    EFStruct *ep;
    f32 angle;
    f32 vel;
    f32 scale;

    effect_gobj = gcMakeGObjSPAfter(nGCCommonKindEffect, NULL, nGCCommonLinkIDEffect, GOBJ_PRIORITY_DEFAULT);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    effect_gobj->user_data.p = NULL;

    pc = lbParticleMakeScriptID(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0x58);

    if (pc != NULL)
    {
        xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusDefault);

        if (xf != NULL)
        {
            xf->effect_gobj = effect_gobj;

            xf->translate = *pos;

            xf->translate.y += EFCOMMON_DUSTHEAVY_OFF_Y;

            if (lr == -1)
            {
                xf->rotate.y = F_CLC_DTOR32(180.0F);
            }
            xf->proc_dead = efManagerDefaultProcDead;
        }
        else pc = efManagerDestroyParticleGObj(pc, effect_gobj);
    }
    else efManagerDestroyParticleGObj(NULL, effect_gobj);

    return pc;
}

// 0x800FF384
void efManagerDustHeavyDoubleProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);
    s32 unused;

#if defined(PORT) && defined(SSB64_NETMENU)
    if (ep == NULL)
    {
        efManagerNetplayEjectStaleXfEffect(effect_gobj, NULL, NULL, "DustHeavyDoubleProcUpdate", "null_ep");
        return;
    }
    {
        LBTransform *xf = ep->effect_vars.dust_heavy.xf;
        const char *reason;

        if (efManagerNetplayEffectXfIsLive(effect_gobj, xf, &reason) == FALSE)
        {
            efManagerNetplayEjectStaleXfEffect(effect_gobj, ep, xf, "DustHeavyDoubleProcUpdate", reason);
            return;
        }
        if ((xf->effect_gobj != effect_gobj) || (lbParticleTransformIsAllocated(xf) == FALSE))
        {
            efManagerNetplayEjectStaleXfEffect(effect_gobj, ep, xf, "DustHeavyDoubleProcUpdate", "pre_write_race");
            return;
        }
        ep->effect_vars.dust_heavy.anim_frame++;

        if (ep->effect_vars.dust_heavy.anim_frame == 2)
        {
            Vec3f pos = xf->translate;

            pos.y -= 126.0F;

            efManagerDustHeavyMakeEffect(&pos, -ep->effect_vars.dust_heavy.lr);
        }
        return;
    }
#endif
    ep->effect_vars.dust_heavy.anim_frame++;

    if (ep->effect_vars.dust_heavy.anim_frame == 2)
    {
        Vec3f pos = ep->effect_vars.dust_heavy.xf->translate;

        pos.y -= 126.0F;

        efManagerDustHeavyMakeEffect(&pos, -ep->effect_vars.dust_heavy.lr);
    }
}

// 0x800FF3F4
LBParticle* efManagerDustHeavyDoubleMakeEffect(Vec3f *pos, s32 lr, f32 f_index)
{
    GObj *effect_gobj;
    LBParticle *pc;
    LBTransform *xf;
    EFStruct *ep;
    f32 angle;
    f32 vel;
    f32 scale;

    ep = efManagerGetEffectNoForce();

    if (ep == NULL)
    {
        return NULL;
    }
    effect_gobj = gcMakeGObjSPAfter(nGCCommonKindEffect, NULL, nGCCommonLinkIDEffect, GOBJ_PRIORITY_DEFAULT);

    if (effect_gobj == NULL)
    {
        efManagerSetPrevStructAlloc(ep);

        return NULL;
    }
    effect_gobj->user_data.p = ep;

    pc = (f_index == 1.7F) ? lbParticleMakeScriptID(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0x59) : lbParticleMakeScriptID(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0x58); // Why such a specific check when a bool could've worked?

    if (pc != NULL)
    {
        xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusDefault);

        if (xf != NULL)
        {
            gcAddGObjProcess(effect_gobj, efManagerDustHeavyDoubleProcUpdate, nGCProcessKindFunc, 3);

            xf->effect_gobj = effect_gobj;

            ep->effect_vars.dust_heavy.xf = xf;

            xf->translate = *pos;

            xf->translate.y += EFCOMMON_DUSTHEAVY_OFF_Y;

            ep->effect_vars.dust_heavy.pos = *pos;

            ep->effect_vars.dust_heavy.anim_frame = 0;

            ep->effect_vars.dust_heavy.lr = lr;

            if (lr == -1)
            {
                xf->rotate.y = F_CLC_DTOR32(180.0F);
            }
            xf->proc_dead = efManagerDefaultProcDead;
        }
        else pc = efManagerDestroyParticleGObj(pc, effect_gobj);
    }
    else efManagerDestroyParticleGObj(NULL, effect_gobj);

    return pc;
}

// 0x800FF590
LBParticle* efManagerDustExpandLargeMakeEffect(Vec3f *pos)
{
    LBParticle *pc = lbParticleMakeScriptID(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0x57);

    if (pc != NULL)
    {
        LBTransform *xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf != NULL)
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            xf->translate = *pos;

            xf->scale.x = EFCOMMON_DUSTEXPANDLARGE_SCALE;
            xf->scale.y = EFCOMMON_DUSTEXPANDLARGE_SCALE;
            xf->scale.z = EFCOMMON_DUSTEXPANDLARGE_SCALE;
        }
        else
        {
            lbParticleEjectStruct(pc);

            pc = NULL;
        }
    }
    return pc;
}

// 0x800FF648
LBParticle* efManagerDustExpandSmallMakeEffect(Vec3f *pos, f32 f_index)
{
    GObj *effect_gobj;
    LBParticle *pc;
    LBTransform *xf;
    EFStruct *ep;
    f32 angle;
    f32 vel;
    f32 scale;

    ep = efManagerGetEffectNoForce();

    if (ep == NULL)
    {
        return NULL;
    }
    effect_gobj = gcMakeGObjSPAfter(nGCCommonKindEffect, NULL, nGCCommonLinkIDEffect, GOBJ_PRIORITY_DEFAULT);

    if (effect_gobj == NULL)
    {
        efManagerSetPrevStructAlloc(ep);

        return NULL;
    }
    effect_gobj->user_data.p = ep;

    pc = (f_index == 2.0F) ? lbParticleMakeScriptID(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0x56) : lbParticleMakeScriptID(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0x55);

    if (pc != NULL)
    {
        xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusDefault);

        if (xf != NULL)
        {
            xf->effect_gobj = effect_gobj;
            xf->proc_dead = efManagerDefaultProcDead;

            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            gcAddGObjProcess(effect_gobj, efManagerDefaultProcUpdate, nGCProcessKindFunc, 3);

            ep->effect_vars.common.xf = xf;

            xf->translate = *pos;

            ep->effect_vars.common.vel.y = EFCOMMON_DUSTEXPANDSMALL_VEL_Y;
            ep->effect_vars.common.vel.x = EFCOMMON_DUSTEXPANDSMALL_VEL_X;
        }
        else pc = efManagerDestroyParticleGObj(pc, effect_gobj);
    }
    else efManagerDestroyParticleGObj(NULL, effect_gobj);

    return pc;
}

// 0x800FF7D8
LBParticle* efManagerDustDashMakeEffect(Vec3f *pos, s32 lr, f32 scale)
{
    LBParticle *pc;
    LBTransform *xf;
    EFStruct *ep;

    pc = lbParticleMakeScriptID(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0x5A);

    if (pc != NULL)
    {
        xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusDefault);

        if (xf != NULL)
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            xf->translate = *pos;

            xf->scale.x = scale;
            xf->scale.y = scale;
            xf->scale.z = scale;

            xf->translate.y += EFCOMMON_DUSTDASH_OFF_Y;

            if (lr == -1)
            {
                xf->rotate.y = F_CLC_DTOR32(180.0F);
            }
        }
        else
        {
            lbParticleEjectStruct(pc);

            pc = NULL;
        }
    }
    return pc;
}

// 0x800FF8C0
void efManagerDamageFlyOrbsProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);
    DObj *dobj = DObjGetStruct(effect_gobj);

    gcPlayAnimAll(effect_gobj);

    ep->effect_vars.damage_fly_orbs.lifetime--;

    if (ep->effect_vars.damage_fly_orbs.lifetime < 0)
    {
        efManagerSetPrevStructAlloc(ep);
        gcEjectGObj(effect_gobj);
    }
    else
    {
        dobj->translate.vec.f.x += ep->effect_vars.damage_fly_orbs.vel.x;
        dobj->translate.vec.f.y += ep->effect_vars.damage_fly_orbs.vel.y;

        ep->effect_vars.damage_fly_orbs.vel.y -= EFCOMMON_DAMAGEFLYORBS_VEL_SUB;
    }
}

// 0x800FF95C
void efManagerDamageSpawnOrbsProcUpdate(GObj *this_gobj)
{
    GObj *new_gobj;
    DObj *dobj;
    EFStruct *this_ep;
    EFStruct *new_ep;
    f32 vel;
    f32 angle;

    this_ep = efGetStruct(this_gobj);

    if (!(this_ep->effect_vars.damage_spawn_orbs.lifetime % EFCOMMON_DAMAGESPAWNORBS_LIFETIME_RANDOM_MOD))
    {
        new_gobj = efManagerMakeEffectNoForce(&dEFManagerDamageFlyOrbsEffectDesc);

        if (new_gobj != NULL)
        {
            dobj = DObjGetStruct(new_gobj);
            new_ep = efGetStruct(new_gobj);

            dobj->translate.vec.f = this_ep->effect_vars.damage_spawn_orbs.pos;

            dobj->scale.vec.f.x = dobj->scale.vec.f.y = (syUtilsRandFloat() * EFCOMMON_DAMAGESPAWNORBS_SCALE_BASE) + EFCOMMON_DAMAGESPAWNORBS_SCALE_ADD;

            vel = (syUtilsRandFloat() * EFCOMMON_DAMAGESPAWNORBS_VEL_BASE) + EFCOMMON_DAMAGESPAWNORBS_VEL_ADD;

            angle = (syUtilsRandFloat() * EFCOMMON_DAMAGESPAWNORBS_ANGLE_BASE) + EFCOMMON_DAMAGESPAWNORBS_ANGLE_ADD1 + EFCOMMON_DAMAGESPAWNORBS_ANGLE_ADD2;

            new_ep->effect_vars.damage_fly_orbs.vel.x = __cosf(angle) * vel;
            new_ep->effect_vars.damage_fly_orbs.vel.y = __sinf(angle) * vel;
            new_ep->effect_vars.damage_fly_orbs.lifetime = syUtilsRandIntRange(EFCOMMON_DAMAGESPAWNORBS_LIFETIME_RANDOM_MOD) + EFCOMMON_DAMAGESPAWNORBS_LIFETIME_ADD;
        }
    }
    this_ep->effect_vars.damage_spawn_orbs.lifetime--;

    if (this_ep->effect_vars.damage_spawn_orbs.lifetime < 0)
    {
        efManagerSetPrevStructAlloc(this_ep);
        gcEjectGObj(this_gobj);
    }
}

// 0x800FFAB8
GObj* efManagerDamageSpawnOrbsMakeEffect(Vec3f *pos)
{
    GObj *effect_gobj;
    EFStruct *ep;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerDamageSpawnOrbsEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);

    ep->effect_vars.damage_spawn_orbs.pos = *pos;

    ep->effect_vars.damage_spawn_orbs.lifetime = (syUtilsRandIntRange(3) * 4) + 4;

    return effect_gobj;
}

// 0x800FFB38
GObj* efManagerDamageSpawnOrbsRandgcMakeEffect(Vec3f *pos)
{
    if (syUtilsRandIntRange(4) != 0)
    {
        return NULL;
    }
    else return efManagerDamageSpawnOrbsMakeEffect(pos);
}

// 0x800FFB74
void efManagerImpactWaveProcDisplay(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);
    s32 index = ep->effect_vars.impact_wave.index;

    gDPPipeSync(gSYTaskmanDLHeads[0]++);
    gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_ZB_XLU_SURF, G_RM_AA_ZB_XLU_SURF2);
    gDPSetPrimColor(gSYTaskmanDLHeads[0]++, 0, 0, dEFManagerImpactWavePrimColorR[index], dEFManagerImpactWavePrimColorG[index], dEFManagerImpactWavePrimColorB[index], (s32)ep->effect_vars.impact_wave.alpha);

    // The following EnvColor RGB key arrays are all zeroes, so using them (and even having them occupy space in the .data section) is completely unnecessary.

#if defined (DAIRANTOU_OPT0)
    gDPSetEnvColor(gSYTaskmanDLHeads[0]++, 0x00, 0x00, 0x00, 0xFF);
#else
    gDPSetEnvColor(gSYTaskmanDLHeads[0]++, dEFManagerImpactWaveEnvColorR[index], dEFManagerImpactWaveEnvColorG[index], dEFManagerImpactWaveEnvColorB[index], 0xFF);
#endif

    gcDrawDObjDLHead0(effect_gobj);
}

// 0x800FFCA4
void efManagerImpactWaveProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);

    gcPlayAnimAll(effect_gobj);

    if (effect_gobj->anim_frame <= 0.0F)
    {
        efManagerSetPrevStructAlloc(efGetStruct(effect_gobj));
        gcEjectGObj(effect_gobj);
    }
    else
    {
        ep->effect_vars.impact_wave.alpha -= ep->effect_vars.impact_wave.decay;

        if (ep->effect_vars.impact_wave.alpha > 0xFF)
        {
            ep->effect_vars.impact_wave.alpha = 0xFF;
        }
        else if (ep->effect_vars.impact_wave.alpha < 0x00)
        {
            ep->effect_vars.impact_wave.alpha = 0x00;
        }
    }
}

// 0x800FFD58
GObj* efManagerImpactWaveMakeEffect(Vec3f *pos, s32 index, f32 rotate)
{
    GObj *effect_gobj = efManagerMakeEffectNoForce(&dEFManagerImpactWaveEffectDesc);
    DObj *dobj;
    EFStruct *ep;

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);
    ep = efGetStruct(effect_gobj);

    dobj->translate.vec.f = *pos;

    dobj->rotate.vec.f.z = rotate;

    ep->effect_vars.impact_wave.index = index;
    ep->effect_vars.impact_wave.alpha = 255.0F;
    ep->effect_vars.impact_wave.decay = 127.0F / 11.0F;

    return effect_gobj;
}

// 0x800FFDE8
GObj* efManagerImpactAirWaveMakeEffect(Vec3f *pos, s32 index)
{
    return efManagerImpactWaveMakeEffect(pos, index, 0.0F);
}

// 0x800FFE08
void efManagerStarRodSparkProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);

    gcPlayAnimAll(effect_gobj);

    if (effect_gobj->anim_frame <= 0.0F)
    {
        efManagerSetPrevStructAlloc(ep);
        gcEjectGObj(effect_gobj);

        return;
    }
    else
    {
        if (ep->effect_vars.star_rod_spark.add_timer != 0)
        {
            ep->effect_vars.star_rod_spark.add_timer--;

            ep->effect_vars.star_rod_spark.vel.x += ep->effect_vars.star_rod_spark.add.x;
        }
        DObjGetStruct(effect_gobj)->translate.vec.f.x += ep->effect_vars.star_rod_spark.vel.x;
    }
}

// 0x800FFEA4
GObj* efManagerStarRodSparkMakeEffect(Vec3f *pos, s32 lr)
{
    GObj *effect_gobj;
    DObj *dobj;
    EFStruct *ep;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerStarRodSparkEffectDesc);

    if (effect_gobj != NULL)
    {
        dobj = DObjGetStruct(effect_gobj);
        ep = efGetStruct(effect_gobj);

        dobj->translate.vec.f = *pos;

        dobj->rotate.vec.f.z = syUtilsRandFloat() * F_CLC_DTOR32(360.0F); // F_CLC_DTOR32(360.0F)

        dobj->scale.vec.f.x = EFCOMMON_STARRODSPARK_SCALE;
        dobj->scale.vec.f.y = EFCOMMON_STARRODSPARK_SCALE;

        ep->effect_vars.star_rod_spark.vel.x = lr * EFCOMMON_STARRODSPARK_VEL_BASE;
        ep->effect_vars.star_rod_spark.add.x = lr * EFCOMMON_STARRODSPARK_VEL_ADD;
        ep->effect_vars.star_rod_spark.add_timer = EFCOMMON_STARRODSPARK_ADD_TIMER;
    }
    return effect_gobj;
}

// 0x800FFF74
void efManagerDamageFlySparksProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);

    gcPlayAnimAll(effect_gobj);

    if (effect_gobj->anim_frame <= 0.0F)
    {
        efManagerSetPrevStructAlloc(ep);
        gcEjectGObj(effect_gobj);
    }
    else
    {
        DObj *dobj = DObjGetStruct(effect_gobj);

        dobj->translate.vec.f.x += ep->effect_vars.damage_fly_sparks.vel.x;
        dobj->translate.vec.f.y += ep->effect_vars.damage_fly_sparks.vel.y;

        if (ep->effect_vars.damage_fly_sparks.add_timer != 0)
        {
            ep->effect_vars.damage_fly_sparks.add_timer--;

            ep->effect_vars.damage_fly_sparks.vel.x += ep->effect_vars.damage_fly_sparks.add.x;
            ep->effect_vars.damage_fly_sparks.vel.y += ep->effect_vars.damage_fly_sparks.add.y;
        }
    }
}

// 0x80100030
void efManagerDamageSpawnSparksProcUpdate(GObj *effect_gobj)
{
    EFStruct *this_ep;
    DObj *dobj;
    EFStruct *new_ep;
    GObj *new_gobj;
    s32 lifetime;
    f32 angle;
    f32 var;
    f32 unused;

    this_ep = efGetStruct(effect_gobj);
    lifetime = this_ep->effect_vars.damage_spawn_sparks.lifetime;

    if (!(lifetime % EFCOMMON_DAMAGESPAWNSPARK_LIFETIME_MOD))
    {
        new_gobj = efManagerMakeEffectNoForce(&dEFManagerDamageFlySparksEffectDesc);

        if (new_gobj != NULL)
        {
            dobj = DObjGetStruct(new_gobj);
            new_ep = efGetStruct(new_gobj);

            dobj->translate.vec.f = this_ep->effect_vars.damage_spawn_sparks.pos;

            dobj->rotate.vec.f.z = syUtilsRandFloat() * F_CLC_DTOR32(360.0F);

            var = dEFManagerDamageSpawnSparksAngles[ -(lifetime / EFCOMMON_DAMAGESPAWNSPARK_LIFETIME_MOD) + (EFCOMMON_DAMAGESPAWNSPARK_LIFETIME_MOD / 2) ];

            angle = F_CLC_DTOR32(var);

            new_ep->effect_vars.damage_fly_sparks.vel.x = __cosf(angle) * EFCOMMON_DAMAGESPAWNSPARK_VEL_BASE * this_ep->effect_vars.damage_spawn_sparks.lr;
            new_ep->effect_vars.damage_fly_sparks.vel.y = __sinf(angle) * EFCOMMON_DAMAGESPAWNSPARK_VEL_BASE;

            new_ep->effect_vars.damage_fly_sparks.add.x = -new_ep->effect_vars.damage_fly_sparks.vel.x * EFCOMMON_DAMAGESPAWNSPARK_VEL_ADD;
            new_ep->effect_vars.damage_fly_sparks.add.y = -new_ep->effect_vars.damage_fly_sparks.vel.y * EFCOMMON_DAMAGESPAWNSPARK_VEL_ADD;

            new_ep->effect_vars.damage_fly_sparks.add_timer = EFCOMMON_DAMAGESPAWNSPARK_ADD_TIMER;
        }
    }
    this_ep->effect_vars.damage_spawn_sparks.lifetime--;

    if (this_ep->effect_vars.damage_spawn_sparks.lifetime < 0)
    {
        efManagerSetPrevStructAlloc(this_ep);
        gcEjectGObj(effect_gobj);
    }
}

// 0x801001A8
GObj* efManagerDamageSpawnSparksMakeEffect(Vec3f *pos, s32 lr)
{
    GObj *effect_gobj;
    EFStruct *ep;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerDamageSpawnSparksEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);

    ep->effect_vars.damage_spawn_sparks.pos = *pos;
    ep->effect_vars.damage_spawn_sparks.lifetime = EFCOMMON_DAMAGESPAWNSPARK_LIFETIME;
    ep->effect_vars.damage_spawn_sparks.lr = lr;

    return effect_gobj;
}

// 0x80100218
GObj* efManagerDamageSpawnSparksRandgcMakeEffect(Vec3f *pos, s32 lr)
{
    if (syUtilsRandIntRange(4) != 0)
    {
        return NULL;
    }
    else return efManagerDamageSpawnSparksMakeEffect(pos, lr);
}

// 0x80100258
void efManagerDamageSpawnMDustProcUpdate(GObj *effect_gobj)
{
    EFStruct *this_ep;
    DObj *dobj;
    EFStruct *new_ep;
    GObj *new_gobj;
    s32 lifetime;
    f32 angle;
    f32 var;
    f32 unused;

    this_ep = efGetStruct(effect_gobj);
    lifetime = this_ep->effect_vars.damage_spawn_mdust.lifetime;

    if (!(lifetime % EFCOMMON_DAMAGESPAWNMDUST_LIFETIME_MOD))
    {
        new_gobj = efManagerMakeEffectNoForce(&dEFManagerDamageFlyMDustEffectDesc);

        if (new_gobj != NULL)
        {
            dobj = DObjGetStruct(new_gobj);
            new_ep = efGetStruct(new_gobj);

            dobj->translate.vec.f = this_ep->effect_vars.damage_spawn_mdust.pos;

            dobj->rotate.vec.f.z = syUtilsRandFloat() * F_CLC_DTOR32(360.0F);

            var = dEFManagerDamageSpawnMDustAngles[ -(lifetime / EFCOMMON_DAMAGESPAWNMDUST_LIFETIME_MOD) + (EFCOMMON_DAMAGESPAWNMDUST_LIFETIME_MOD / 2) ];

            angle = F_CLC_DTOR32(var);

            new_ep->effect_vars.damage_fly_mdust.vel.x = __cosf(angle) * EFCOMMON_DAMAGESPAWNMDUSTVEL_BASE * this_ep->effect_vars.damage_spawn_mdust.lr;
            new_ep->effect_vars.damage_fly_mdust.vel.y = __sinf(angle) * EFCOMMON_DAMAGESPAWNMDUSTVEL_BASE;

            new_ep->effect_vars.damage_fly_mdust.add.x = -new_ep->effect_vars.damage_fly_mdust.vel.x * EFCOMMON_DAMAGESPAWNMDUSTVEL_ADD;
            new_ep->effect_vars.damage_fly_mdust.add.y = -new_ep->effect_vars.damage_fly_mdust.vel.y * EFCOMMON_DAMAGESPAWNMDUSTVEL_ADD;

            new_ep->effect_vars.damage_fly_mdust.add_timer = EFCOMMON_DAMAGESPAWNMDUST_ADD_TIMER;
        }
    }
    this_ep->effect_vars.damage_spawn_mdust.lifetime--;

    if (this_ep->effect_vars.damage_spawn_mdust.lifetime < 0)
    {
        efManagerSetPrevStructAlloc(this_ep);
        gcEjectGObj(effect_gobj);
    }
}

// 0x801003D0
GObj* efManagerDamageSpawnMDustMakeEffect(Vec3f *pos, s32 lr)
{
    GObj *effect_gobj;
    EFStruct *ep;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerDamageSpawnMDustEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);

    ep->effect_vars.damage_spawn_mdust.pos = *pos;
    ep->effect_vars.damage_spawn_mdust.lifetime = EFCOMMON_DAMAGESPAWNMDUST_LIFETIME;
    ep->effect_vars.damage_spawn_mdust.lr = lr;

    return effect_gobj;
}

// 0x80100440
GObj* efManagerDamageSpawnMDustRandgcMakeEffect(Vec3f *pos, s32 lr)
{
    if (syUtilsRandIntRange(4) != 0)
    {
        return NULL;
    }
    else return efManagerDamageSpawnMDustMakeEffect(pos, lr);
}

// 0x80100480
LBParticle* efManagerSparkleWhiteMakeEffect(Vec3f *pos)
{
    LBParticle *pc = lbParticleMakeScriptID(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0x73);

    if (pc != NULL)
    {
        LBTransform *xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf != NULL)
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            xf->translate = *pos;
        }
        else
        {
            lbParticleEjectStruct(pc);

            pc = NULL;
        }
    }
    return pc;
}

// 0x80100524
LBParticle* efManagerSparkleWhiteMultiMakeEffect(Vec3f *pos)
{
    LBParticle *pc = lbParticleMakeScriptID(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0x1A);

    if (pc != NULL)
    {
        LBTransform *xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf != NULL)
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            xf->translate = *pos;
        }
        else
        {
            lbParticleEjectStruct(pc);

            pc = NULL;
        }
    }
    return pc;
}

// 0x801005C8
LBParticle* efManagerSparkleWhiteMultiExplodeMakeEffect(Vec3f *pos)
{
    LBParticle *pc = lbParticleMakeScriptID(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0x22);

    if (pc != NULL)
    {
        LBTransform *xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf != NULL)
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            xf->translate = *pos;
        }
        else
        {
            lbParticleEjectStruct(pc);

            pc = NULL;
        }
    }
    return pc;
}

// 0x8010066C
LBParticle* efManagerSparkleWhiteScaleMakeEffect(Vec3f *pos, f32 scale)
{
    LBParticle *pc = lbParticleMakeScriptID(gEFManagerParticleBankID, 0x5B);

    if (pc != NULL)
    {
        LBTransform *xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf != NULL)
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            xf->translate = *pos;

            xf->scale.x = scale;
            xf->scale.y = scale;
            xf->scale.z = scale;
        }
        else
        {
            lbParticleEjectStruct(pc);

            pc = NULL;
        }
    }
    return pc;
}

// 0x80100720 - Plays when a fighter is Star KO'd
LBParticle* efManagerSparkleWhiteDeadMakeEffect(Vec3f *pos, f32 scale)
{
    LBParticle *pc = lbParticleMakeScriptID(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(1), 0x5C);

    if (pc != NULL)
    {
        LBTransform *xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf != NULL)
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            xf->translate = *pos;

            xf->scale.x = scale;
            xf->scale.y = scale;
            xf->scale.z = scale;
        }
        else
        {
            lbParticleEjectStruct(pc);

            pc = NULL;
        }
    }
    return pc;
}

// 0x801007D8
void efManagerQuakeProcUpdate(GObj *effect_gobj)
{
    DObj *dobj;
    Vec3f sub;
    Vec3f pos;
    CObj *cobj;
    f32 mag;

    gcPlayAnimAll(effect_gobj);

    if (effect_gobj->anim_frame <= 0.0F)
    {
        efManagerSetPrevStructAlloc(efGetStruct(effect_gobj));
        gcEjectGObj(effect_gobj);
    }
    else
    {
#ifdef PORT
        if (gGMCameraGObj == NULL)
        {
            return;
        }
#endif
        cobj = CObjGetStruct(gGMCameraGObj);
#ifdef PORT
        if (cobj == NULL)
        {
            return;
        }
#endif
        dobj = DObjGetStruct(effect_gobj);
#ifdef PORT
        if (dobj == NULL)
        {
            efManagerSetPrevStructAlloc(efGetStruct(effect_gobj));
            gcEjectGObj(effect_gobj);
            return;
        }
#endif
        syVectorDiff3D(&sub, &cobj->vec.at, &cobj->vec.eye);

        mag = syVectorMag3D(&sub);

        if (mag > EFCOMMON_QUAKE_MAGNITUDE)
        {
            mag = mag / EFCOMMON_QUAKE_MAGNITUDE;

            pos.x = dobj->translate.vec.f.z * mag;
            pos.y = dobj->translate.vec.f.y * mag;
        }
        else
        {
            pos.x = dobj->translate.vec.f.z;
            pos.y = dobj->translate.vec.f.y;
        }
        pos.z = 0.0F;

        gmCameraSetVelAt(&pos);
    }
}

// 0x801008B8
void efManagerQuakeFuncRun(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);

    gcAddGObjProcess(effect_gobj, efManagerQuakeProcUpdate, nGCProcessKindFunc, ep->effect_vars.quake.priority);

    effect_gobj->func_run = NULL;
}

// 0x801008F4
GObj* efManagerQuakeMakeEffect(s32 magnitude)
{
    s32 unused[2];
    EFStruct *ep;
    GObj *effect_gobj;

    ep = efManagerGetEffectNoForce();

    if (ep == NULL)
    {
        return NULL;
    }
    effect_gobj = gcMakeGObjSPAfter(nGCCommonKindEffect, efManagerQuakeFuncRun, nGCCommonLinkIDEffect, GOBJ_PRIORITY_DEFAULT);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    effect_gobj->user_data.p = ep;

    gcAddXObjForDObjFixed(gcAddDObjForGObj(effect_gobj, NULL), nGCMatrixKindTra, 0);

    switch (magnitude)
    {
    case 0:
        gcAddAnimJointAll(effect_gobj, lbRelocGetFileData(AObjEvent32**, gEFManagerFiles[0], llEFCommonEffects1QuakeMag0AnimJoint), 0.0F);
        break;

    case 1:
        gcAddAnimJointAll(effect_gobj, lbRelocGetFileData(AObjEvent32**, gEFManagerFiles[0], llEFCommonEffects1QuakeMag1AnimJoint), 0.0F);
        break;

    case 2:
        gcAddAnimJointAll(effect_gobj, lbRelocGetFileData(AObjEvent32**, gEFManagerFiles[0], llEFCommonEffects1QuakeMag2AnimJoint), 0.0F);
        break;

    case 3:
        gcAddAnimJointAll(effect_gobj, lbRelocGetFileData(AObjEvent32**, gEFManagerFiles[0], llEFCommonEffects1QuakeMag3AnimJoint), 0.0F);
        break;

    default:
        break;
    }
    gcPlayAnimAll(effect_gobj);

    ep->effect_vars.quake.priority = 3 - magnitude;

    return effect_gobj;
}

// 0x80100A58
void efManagerDamageCoinProcDead(LBTransform *xf)
{
    Vec3f pos = xf->translate;

    pos.y += 200.0F;

    efManagerDustExpandSmallMakeEffect(&pos, 2.0F);
    efManagerSetPrevStructAlloc(efGetStruct(xf->effect_gobj));
    gcEjectGObj(xf->effect_gobj);
}

// 0x80100ACC
LBParticle* efManagerDamageCoinMakeEffect(Vec3f *pos)
{
    GObj *effect_gobj;
    LBParticle *pc;
    LBTransform *xf;
    EFStruct *ep;
    f32 angle;
    f32 vel;
    f32 scale;

    ep = efManagerGetEffectNoForce();

    if (ep == NULL)
    {
        return NULL;
    }

    effect_gobj = gcMakeGObjSPAfter(nGCCommonKindEffect, NULL, nGCCommonLinkIDEffect, GOBJ_PRIORITY_DEFAULT);

    if (effect_gobj == NULL)
    {
        efManagerSetPrevStructAlloc(ep);

        return NULL;
    }
    effect_gobj->user_data.p = ep;

    pc = lbParticleMakeScriptID(gEFManagerParticleBankID, 0x60);

    if (pc != NULL)
    {
        xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusDefault);

        if (xf != NULL)
        {
            xf->effect_gobj = effect_gobj;
            xf->proc_dead = efManagerDamageCoinProcDead;

            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            ep->effect_vars.common.xf = xf;

            xf->translate = *pos;
        }
        else pc = efManagerDestroyParticleGObj(pc, effect_gobj);
    }
    else efManagerDestroyParticleGObj(NULL, effect_gobj);

    return pc;
}

// 0x80100BF0
LBParticle* efManagerSetOffMakeEffect(Vec3f *pos, s32 size)
{
    GObj *effect_gobj;
    LBParticle *pc;
    LBTransform *xf;
    EFStruct *ep;
    f32 vel;
    f32 angle;
    f32 scale;

    ep = efManagerGetEffectNoForce();

    if (ep == NULL)
    {
        return NULL;
    }
    effect_gobj = gcMakeGObjSPAfter(nGCCommonKindEffect, NULL, nGCCommonLinkIDEffect, GOBJ_PRIORITY_DEFAULT);

    if (effect_gobj == NULL)
    {
        efManagerSetPrevStructAlloc(ep);

        return NULL;
    }
    effect_gobj->user_data.p = ep;

    pc = lbParticleMakeScriptID(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0x65);

    if (pc != NULL)
    {
        xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf != NULL)
        {
            xf->effect_gobj = effect_gobj;
            xf->proc_dead = efManagerDefaultProcDead;

            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            gcAddGObjProcess(effect_gobj, efManagerDefaultProcUpdate, nGCProcessKindFunc, 3);

            ep->effect_vars.common.xf = xf;

            xf->translate = *pos;

            vel = (syUtilsRandFloat() * 18.0F) + 12.0F;
            angle = syUtilsRandFloat() * F_CLC_DTOR32(360.0F); // F_CLC_DTOR32(360.0F)

            ep->effect_vars.common.vel.x = __cosf(angle) * vel;
            ep->effect_vars.common.vel.y = __sinf(angle) * vel;

            scale = (size < 10) ? (((10 - size) * -0.05F) + 1.0F) : (((size - 10) * 0.15F) + 1.0F);

            xf->scale.x = xf->scale.y = xf->scale.z = scale;
        }
        else pc = efManagerDestroyParticleGObj(pc, effect_gobj);
    }
    else efManagerDestroyParticleGObj(NULL, effect_gobj);

    return pc;
}

// 0x80100DEC
GObj* efManagerFireSparkMakeEffect(GObj *fighter_gobj) // I really have no idea where this effect is used; it can only be created by script
{
    FTStruct *fp;
    EFStruct *ep;
    GObj *effect_gobj;
    DObj *dobj;

    fp = ftGetStruct(fighter_gobj);

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerFireSparkEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    fp->is_effect_attach = TRUE;

    ep = efGetStruct(effect_gobj);
    ep->fighter_gobj = fighter_gobj;

    dobj = DObjGetStruct(effect_gobj);

    dobj->translate.vec.f.y = 160.0F;
    dobj->user_data.p = fp->joints[16];

    lbCommonSetDObjTransformsForTreeDObjs(dobj->child, lbRelocGetFileData(DObjDesc*, gEFManagerFiles[1], llEFCommonEffects2FireSparkDObjDesc));

    return effect_gobj;
}

// 0x80100E84
void efManagerFoxReflectorSetAnimID(GObj *effect_gobj, s32 anim_id)
{
    EFStruct *ep = efGetStruct(effect_gobj);

    ep->effect_vars.reflector.index = anim_id;

    gcAddAnimJointAll(effect_gobj, lbRelocGetFileData(AObjEvent32**, gFTDataFoxSpecial2, dEFManagerFoxReflectorAnimJointOffsets[anim_id]), 0.0F);
    gcPlayAnimAll(effect_gobj);
}

// 0x80100ED4
void efManagerFoxReflectorProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);

    gcPlayAnimAll(effect_gobj);

    if (effect_gobj->anim_frame <= 0.0F)
    {
        switch (ep->effect_vars.reflector.index)
        {
        case 1:
            break;

        case 0:
        case 2:
            efManagerFoxReflectorSetAnimID(effect_gobj, 1);
            break;

        case 3:
            efManagerSetPrevStructAlloc(ep);
            gcEjectGObj(effect_gobj);
            return;
        }
    }
    if (ep->effect_vars.reflector.status != 4)
    {
        efManagerFoxReflectorSetAnimID(effect_gobj, ep->effect_vars.reflector.status);

        ep->effect_vars.reflector.status = 4;
    }
}

// 0x80100FA4
GObj* efManagerFoxReflectorMakeEffect(GObj *fighter_gobj)
{
    GObj *effect_gobj = efManagerMakeEffectForce(&dEFManagerFoxReflectorEffectDesc);
    EFStruct *ep;

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);

    ep->fighter_gobj = fighter_gobj;

    DObjGetStruct(effect_gobj)->user_data.p = ftGetStruct(fighter_gobj)->joints[nFTPartsJointTopN];

    ep->effect_vars.reflector.index = 0;
    ep->effect_vars.reflector.status = 4;

    return effect_gobj;
}

// 0x80101008
void efManagerShieldProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);

    if (ep->effect_vars.shield.is_damage_shield != FALSE)
    {
        ep->effect_vars.shield.is_damage_shield = FALSE;
    }
}

// 0x80101024
void efManagerShieldProcDisplay(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);
    s32 id = (ep->effect_vars.shield.is_damage_shield != FALSE) ? 4 : ep->effect_vars.shield.player;

    gDPPipeSync(gSYTaskmanDLHeads[1]++);
    gDPSetPrimColor(gSYTaskmanDLHeads[1]++, 0, 0, dEFManagerShieldColors[id].prim.r, dEFManagerShieldColors[id].prim.g, dEFManagerShieldColors[id].prim.b, 0xC0);
    gDPSetEnvColor(gSYTaskmanDLHeads[1]++, dEFManagerShieldColors[id].env.r, dEFManagerShieldColors[id].env.g, dEFManagerShieldColors[id].env.b, 0xC0);

    gcDrawDObjTreeDLLinksForGObj(effect_gobj);
}

// 0x80101108
GObj* efManagerShieldMakeEffect(GObj *fighter_gobj)
{
    GObj *effect_gobj;
    EFStruct *ep;
    FTStruct *fp;

    fp = ftGetStruct(fighter_gobj);

    effect_gobj = efManagerMakeEffectForce(&dEFManagerShieldEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);

    ep->fighter_gobj = fighter_gobj;

    fp->is_effect_attach = TRUE;

    DObjGetStruct(effect_gobj)->user_data.p = fp->joints[nFTPartsJointYRotN];

    ep->effect_vars.shield.player = fp->player;
    ep->effect_vars.shield.is_damage_shield = FALSE;

    return effect_gobj;
}

// 0x80101180
void efManagerYoshiShieldProcDisplay(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);
    FTStruct *fp = ftGetStruct(ep->fighter_gobj);
    f32 blend = 1.0F - (fp->shield_health / 55.0F);
    u8 color[3];

    if (blend < 0.0F)
    {
        blend = 0.0F;
    }
    color[0] = 0xAE * blend;
    color[1] = 0xD6 * blend;
    color[2] = 0xD6 * blend;

    gDPPipeSync(gSYTaskmanDLHeads[1]++);
    gDPSetEnvColor(gSYTaskmanDLHeads[1]++, color[0], color[1], color[2], 0x00);

    gcDrawDObjDLHead1(effect_gobj);

    efDisplayCLDProcDisplay(effect_gobj);
}

// 0x80101374
GObj* efManagerYoshiShieldMakeEffect(GObj *fighter_gobj)
{
    GObj *effect_gobj;
    EFStruct *ep;
    FTStruct *fp;

    fp = ftGetStruct(fighter_gobj);

    effect_gobj = efManagerMakeEffectForce(&dEFManagerYoshiShieldEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);

    ep->fighter_gobj = fighter_gobj;

    fp->is_effect_attach = TRUE;

    DObjGetStruct(effect_gobj)->user_data.p = fp->joints[nFTPartsJointYRotN];
    DObjGetStruct(effect_gobj)->scale.vec.f.x = DObjGetStruct(effect_gobj)->scale.vec.f.y = 1.5F;

    ep->effect_vars.shield.player = fp->player;
    ep->effect_vars.shield.is_damage_shield = FALSE;

    return effect_gobj;
}

// 0x80101408
LBParticle* efManagerThunderAmpMakeEffect(Vec3f *pos)
{
    LBParticle *pc = lbParticleMakeScriptID(gEFManagerParticleBankID, 0x74);

    if (pc != NULL)
    {
        LBTransform *xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf != NULL)
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            xf->translate = *pos;
        }
        else
        {
            lbParticleEjectStruct(pc);

            pc = NULL;
        }
    }
    return pc;
}

// 0x801014A8
LBGenerator* efManagerRippleMakeEffect(Vec3f *pos)
{
    LBGenerator *gn = lbParticleMakeGenerator(gEFManagerParticleBankID, 0x61);

    if (gn != NULL)
    {
        gn->pos.x = pos->x;
        gn->pos.y = pos->y;
        gn->pos.z = pos->z;
    }
    return gn;
}

// 0x80101500
GObj* efManagerCatchSwirlMakeEffect(Vec3f *pos)
{
    GObj *effect_gobj;
    DObj *dobj;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerCatchSwirlEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);

    dobj->translate.vec.f = *pos;

    return effect_gobj;
}

// 0x8010155C
GObj* efManagerReflectBreakMakeEffect(Vec3f *pos, s32 lr)
{
    GObj *effect_gobj;
    DObj *dobj;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerReflectBreakEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);

    dobj->translate.vec.f = *pos;

    if (lr == -1)
    {
        dobj->rotate.vec.f.y = F_CLC_DTOR32(180.0F);
    }
    return effect_gobj;
}

// 0x801015D4
LBParticle* efManagerFuraSparkleMakeEffect(Vec3f *pos)
{
    LBParticle *pc = lbParticleMakeCommon(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0);

    if (pc != NULL)
    {
        pc->pos.x = pos->x;
        pc->pos.y = pos->y;
        pc->pos.z = pos->z;
    }
    return pc;
}

// 0x80101630
LBParticle* efManagerPsionicMakeEffect(Vec3f *pos)
{
    LBParticle *pc = lbParticleMakeCommon(gEFManagerParticleBankID, 7);

    if (pc != NULL)
    {
        pc->pos.x = pos->x;
        pc->pos.y = pos->y;
        pc->pos.z = pos->z;
    }
    return pc;
}

// 0x80101688
LBParticle* efManagerFlashSmallMakeEffect(Vec3f *pos)
{
    LBParticle *pc = lbParticleMakeCommon(gEFManagerParticleBankID, 4);

    if (pc != NULL)
    {
        pc->pos.x = pos->x;
        pc->pos.y = pos->y;
        pc->pos.z = pos->z;
    }
    return pc;
}

// 0x801016E0
LBParticle* efManagerFlashMiddleMakeEffect(Vec3f *pos)
{
    LBParticle *pc = lbParticleMakeCommon(gEFManagerParticleBankID, 5);

    if (pc != NULL)
    {
        pc->pos.x = pos->x;
        pc->pos.y = pos->y;
        pc->pos.z = pos->z;
    }
    return pc;
}

// 0x80101738
LBParticle* efManagerFlashLargeMakeEffect(Vec3f *pos)
{
    LBParticle *pc = lbParticleMakeCommon(gEFManagerParticleBankID, 6);

    if (pc != NULL)
    {
        pc->pos.x = pos->x;
        pc->pos.y = pos->y;
        pc->pos.z = pos->z;
    }
    return pc;
}

// 0x80101790
LBGenerator* efManagerShieldBreakMakeEffect(Vec3f *pos)
{
    LBGenerator *gn = lbParticleMakeGenerator(gEFManagerParticleBankID, 3);

    if (gn != NULL)
    {
        gn->pos.x = pos->x;
        gn->pos.y = pos->y;
        gn->pos.z = pos->z;
    }
    return gn;
}

// 0x801017E8
void func_ovl2_801017E8(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);

    gcPlayAnimAll(effect_gobj);

    if (ep->effect_vars.unknown1.efvars_unk1_0x0 == 0)
    {
        efManagerSetPrevStructAlloc(ep);
        gcEjectGObj(effect_gobj);
    }
    else ep->effect_vars.unknown1.efvars_unk1_0x0--;
}

// 0x8010183C
GObj* func_ovl2_8010183C(Vec3f *pos, s32 arg1)
{
    GObj *effect_gobj;
    EFStruct *ep;
    DObj *dobj;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerPikachuUnkEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);

    ep->effect_vars.unknown1.efvars_unk1_0x0 = arg1;

    dobj = DObjGetStruct(effect_gobj);
    dobj->translate.vec.f = *pos;

    return effect_gobj;
}

// 0x801018A8
GObj* efManagerPikachuThunderShockMakeEffect(GObj *fighter_gobj, Vec3f *pos, s32 frame)
{
    GObj *effect_gobj;
    EFStruct *ep;
    DObj *dobj;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerPikachuThunderShockEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);

    ep->fighter_gobj = fighter_gobj;

    dobj = DObjGetStruct(effect_gobj);
    dobj->user_data.p = ftGetStruct(fighter_gobj)->joints[nFTPartsJointTopN];

    dobj->child->translate.vec.f = *pos;

    dobj->child->translate.vec.f.x = (ftGetStruct(fighter_gobj)->lr == -1) ? -pos->x : pos->x;

    gcAddXObjForDObjFixed(dobj->child->child, 0x2E, 0);

    switch (frame)
    {
    case 1:
        gcAddAnimAll
        (
            effect_gobj, 
            lbRelocGetFileData(AObjEvent32**, gFTDataPikachuSpecial2, llPikachuSpecial2ThunderShock1AnimJoint),
            lbRelocGetFileData(AObjEvent32***, gFTDataPikachuSpecial2, llPikachuSpecial2ThunderShock1MatAnimJoint),
            0.0F
        );
        gcPlayAnimAll(effect_gobj);
        break;

    case 2:
        gcAddAnimAll
        (
            effect_gobj, 
            lbRelocGetFileData(AObjEvent32**, gFTDataPikachuSpecial2, llPikachuSpecial2ThunderShock2AnimJoint),
            lbRelocGetFileData(AObjEvent32***, gFTDataPikachuSpecial2, llPikachuSpecial2ThunderShock2MatAnimJoint),
            0.0F
        );
        gcPlayAnimAll(effect_gobj);
        break;
    }
    return effect_gobj;
}

// 0x80101A08
void efManagerPikachuThunderTrailProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);

    if (ep->effect_vars.thunder_trail.lifetime == 0)
    {
        efManagerSetPrevStructAlloc(ep);
        gcEjectGObj(effect_gobj);

        return;
    }
    else ep->effect_vars.thunder_trail.lifetime--;

    if (DObjGetStruct(effect_gobj)->mobj->texture_id_curr != 3)
    {
        if (ep->effect_vars.thunder_trail.lifetime == 0)
        {
            DObjGetStruct(effect_gobj)->mobj->texture_id_curr = 3;

            DObjGetStruct(effect_gobj)->rotate.vec.f.z = F_CLC_DTOR32(180.0F);
        }
        else DObjGetStruct(effect_gobj)->mobj->texture_id_curr = syUtilsRandIntRange(3);
    }
}

// 0x80101AA8
void efManagerPikachuThunderTrailProcDisplay(GObj *effect_gobj)
{
    gDPPipeSync(gSYTaskmanDLHeads[1]++);

    gDPSetRenderMode(gSYTaskmanDLHeads[1]++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);

    gDPSetAlphaCompare(gSYTaskmanDLHeads[1]++, G_AC_NONE);

    gcDrawDObjDLLinksForGObj(effect_gobj);

    gDPPipeSync(gSYTaskmanDLHeads[1]++);

    gDPSetRenderMode(gSYTaskmanDLHeads[1]++, G_RM_CLD_SURF, G_RM_CLD_SURF2);

    gDPSetAlphaCompare(gSYTaskmanDLHeads[1]++, G_AC_THRESHOLD);
}

// 0x80101B88
GObj* efManagerPikachuThunderTrailMakeEffect(Vec3f *pos, s32 lifetime, s32 texture_index)
{
    GObj *effect_gobj;
    EFStruct *ep;
    DObj *dobj;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerPikachuThunderTrailEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);

    dobj->translate.vec.f = *pos;

    dobj->scale.vec.f.x = 0.5F;
    dobj->scale.vec.f.y = 0.5F;
    dobj->scale.vec.f.z = 0.5F;

    ep = efGetStruct(effect_gobj);

    ep->effect_vars.thunder_trail.lifetime = lifetime;

    dobj->mobj->texture_id_curr = (texture_index == 3) ? 3 : 0;

    return effect_gobj;
}

// 0x80101C34
GObj* efManagerPikachuThunderJoltMakeEffect(Vec3f *pos, f32 rotate)
{
    GObj *effect_gobj = efManagerMakeEffectNoForce(&dEFManagerThunderJoltEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    DObjGetStruct(effect_gobj)->translate.vec.f = *pos;

    DObjGetStruct(effect_gobj)->rotate.vec.f.z = rotate;

    return effect_gobj;
}

// 0x80101CA0
void efManagerKirbyVulcanJabProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);
    DObj *dobj = DObjGetStruct(effect_gobj);

    if (ep->effect_vars.vulcan_jab.lifetime != 0)
    {
        ep->effect_vars.vulcan_jab.vel.x += ep->effect_vars.vulcan_jab.add.x;
        dobj->translate.vec.f.x += ep->effect_vars.vulcan_jab.vel.x;

        ep->effect_vars.vulcan_jab.vel.y += ep->effect_vars.vulcan_jab.add.y;
        dobj->translate.vec.f.y += ep->effect_vars.vulcan_jab.vel.y;

        ep->effect_vars.vulcan_jab.lifetime--;
    }
    else
    {
        efManagerSetPrevStructAlloc(ep);
        gcEjectGObj(effect_gobj);
    }
}

// 0x80101D34
GObj* efManagerKirbyVulcanJabMakeEffect(Vec3f *pos, s32 lr, f32 rotate, f32 vel, f32 add)
{
    GObj *effect_gobj;
    DObj *dobj;
    EFStruct *ep;
    f32 sin, cos;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerVulcanJabEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);
    ep = efGetStruct(effect_gobj);

    dobj->translate.vec.f = *pos;

    if (lr == -1)
    {
        dobj->rotate.vec.f.y = F_CLC_DTOR32(180.0F);

        rotate = -rotate;
        vel = -vel;
        add = -add;
    }
    gcAddXObjForDObjFixed(dobj->child->child, 0x46, 0);

    dobj->rotate.vec.f.z = F_CLC_DTOR32(rotate);

    sin = lbCommonSin(dobj->rotate.vec.f.z);
    cos = lbCommonCos(dobj->rotate.vec.f.z);

    ep->effect_vars.vulcan_jab.lifetime = 6;

    ep->effect_vars.vulcan_jab.vel.x = vel * cos;
    ep->effect_vars.vulcan_jab.vel.y = vel * sin;
    ep->effect_vars.vulcan_jab.vel.z = 0.0F;

    ep->effect_vars.vulcan_jab.add.x = add * cos;
    ep->effect_vars.vulcan_jab.add.y = add * sin;
    ep->effect_vars.vulcan_jab.add.z = 0.0F;

    return effect_gobj;
}

// 0x80101E80
GObj* efManagerSamusGrappleBeamGlowMakeEffect(GObj *fighter_gobj)
{
    GObj *effect_gobj = efManagerMakeEffectNoForce(&dEFManagerSamusGrappleBeamEffectDesc);
    EFStruct *ep;
    DObj *dobj;

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);

    ep->fighter_gobj = fighter_gobj;

    dobj = DObjGetStruct(effect_gobj);

    dobj->user_data.p = ftGetStruct(fighter_gobj)->joints[23];

    return effect_gobj;
}

// 0x80101ED8
GObj* efManagerCaptainFalconKickMakeEffect(GObj *fighter_gobj)
{
    GObj *effect_gobj;
    EFStruct *ep;
    FTStruct *fp;
    DObj *dobj;

    effect_gobj = efManagerMakeEffectForce(&dEFManagerCaptainFalconKickEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);

    ep->fighter_gobj = fighter_gobj;

    fp = ftGetStruct(fighter_gobj);
    dobj = DObjGetStruct(effect_gobj);

    dobj->user_data.p = ftGetStruct(fighter_gobj)->joints[23];

    dobj->rotate.vec.f.y = fp->lr * F_CLC_DTOR32(90.0F);

    if (fp->status_id == nFTCaptainStatusSpecialAirLw)
    {
        dobj->rotate.vec.f.z = -fp->lr * F_CLC_DTOR32(60.0F);
    }
    return effect_gobj;
}

// 0x80101F84
GObj* efManagerCaptainFalconPunchMakeEffect(GObj *fighter_gobj)
{
    FTStruct *fp;
    EFStruct *ep;
    DObj *dobj, *joint;
    GObj *effect_gobj;

    effect_gobj = efManagerMakeEffectForce(&dEFManagerCaptainFalconPunchEffectDesc);

    if (effect_gobj == NULL)
    {
        return FALSE;
    }
    ep = efGetStruct(effect_gobj);

    ep->fighter_gobj = fighter_gobj;

    fp = ftGetStruct(fighter_gobj);

    dobj = DObjGetStruct(effect_gobj);

    joint = ((fp->fkind == nFTKindCaptain) || (fp->fkind == nFTKindNCaptain)) ? fp->joints[16] : fp->joints[30];

    dobj->user_data.p = joint;

    dobj->rotate.vec.f.y = fp->lr * F_CLC_DTOR32(-90.0F);

    return effect_gobj;
}

// 0x80102018
LBGenerator* efManagerKirbyStarMakeEffect(Vec3f *pos)
{
    LBGenerator *gn = lbParticleMakeGenerator(gEFManagerParticleBankID, 0xF);

    if (gn != NULL)
    {
        gn->pos.x = pos->x;
        gn->pos.y = pos->y;
        gn->pos.z = pos->z;
    }
    return gn;
}

// 0x80102070
LBGenerator* efManagerStarSplashMakeEffect(Vec3f *pos, s32 lr)
{
    LBGenerator *gn = (lr == -1) ? lbParticleMakeGenerator(gEFManagerParticleBankID, 0x10) : lbParticleMakeGenerator(gEFManagerParticleBankID, 0x11);

    if (gn != NULL)
    {
        gn->pos.x = pos->x;
        gn->pos.y = pos->y;
        gn->pos.z = pos->z;
    }
    return gn;
}

// 0x801020F4
GObj* efManagerPurinSingMakeEffect(GObj *fighter_gobj)
{
    GObj *effect_gobj;
    DObj *dobj, *sibling_dobj;
    EFStruct *ep;

    effect_gobj = efManagerMakeEffectForce(&dEFManagerPurinSingEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);
    ep->fighter_gobj = fighter_gobj;

    dobj = DObjGetStruct(effect_gobj);
    dobj->user_data.p = ftGetStruct(fighter_gobj)->joints[nFTPartsJointTopN];

    sibling_dobj = dobj->child;

    gcAddXObjForDObjFixed(sibling_dobj, 0x46, 0);

    sibling_dobj = dobj->child->sib_next;

    gcAddXObjForDObjFixed(sibling_dobj, nGCMatrixKindRotRpyR, 0);

    sibling_dobj = sibling_dobj->child;

    gcAddXObjForDObjFixed(sibling_dobj, 0x2A, 0);

    sibling_dobj = sibling_dobj->sib_next;

    gcAddXObjForDObjFixed(sibling_dobj, 0x2A, 0);

    sibling_dobj = sibling_dobj->sib_next;

    gcAddXObjForDObjFixed(sibling_dobj, 0x2A, 0);

    return effect_gobj;
}

// 0x801021C0
GObj* efManagerDeadExplodeMakeEffect(Vec3f *pos, s32 player, u32 type)
{
    s32 unused[4];
    GObj *effect_gobj;
    LBParticle *pc;
    LBTransform *xf;
    DObj *dobj;
    DObj *child_dobj;
    DObj *sibling_dobj;
    u8 index = ((type % 2) * GMCOMMON_PLAYERS_MAX) + player; // WARNING: dEFManagerDeadExplodeGenID should be u8[2][GMCOMMON_PLAYERS_MAX], but it will not match this way; UB-risk

    pc = lbParticleMakeScriptID(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(1), dEFManagerDeadExplodeGenID[index]);

    if (pc != NULL)
    {
        xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf != NULL)
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            xf->translate = *pos;

            xf->rotate.z = F_CLC_DTOR32(dEFManagerDeadExplodeRotateD[type]);
        }
        else lbParticleEjectStruct(pc);
    }
    dEFManagerDeadExplodeEffectDesc.o_matanim_joint = dEFManagerDeadExplodeMatAnimJoints[player];

    effect_gobj = efManagerMakeEffectForce(&dEFManagerDeadExplodeEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);
    dobj->translate.vec.f = *pos;

    child_dobj = dobj->child;

    dobj->rotate.vec.f.z = F_CLC_DTOR32(dEFManagerDeadExplodeRotateD[type]);

    sibling_dobj = dobj->child->sib_next->sib_next;

    sibling_dobj->mobj->sub.envcolor.s.r = dEFManagerDeadExplodeEnvColorSiblingR[player];
    sibling_dobj->mobj->sub.envcolor.s.g = dEFManagerDeadExplodeEnvColorSiblingG[player];
    sibling_dobj->mobj->sub.envcolor.s.b = dEFManagerDeadExplodeEnvColorSiblingB[player];

    sibling_dobj->mobj->sub.flags |= MOBJ_FLAG_ENVCOLOR;

    child_dobj->mobj->sub.envcolor.s.r = dEFManagerDeadExplodeEnvColorChildR[player];
    child_dobj->mobj->sub.envcolor.s.g = dEFManagerDeadExplodeEnvColorChildG[player];
    child_dobj->mobj->sub.envcolor.s.b = dEFManagerDeadExplodeEnvColorChildB[player];

    child_dobj->mobj->sub.flags |= MOBJ_FLAG_ENVCOLOR;

    return effect_gobj;
}

// 0x801023D4
void func_ovl2_801023D4(Vec3f *pos) // Unused?
{
    pos->y += 8000.0F;
    pos->x = 4000.0F;

    efManagerDeadExplodeMakeEffect(pos, 2, 1);
}

// 0x80102418
GObj* efManagerKirbyCutterUpMakeEffect(GObj *fighter_gobj)
{
    GObj *effect_gobj;
    EFStruct *ep;
    FTStruct *fp;
    DObj *dobj;

    effect_gobj = efManagerMakeEffectForce(&dEFManagerKirbyCutterUpEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);
    ep->fighter_gobj = fighter_gobj;

    fp = ftGetStruct(fighter_gobj);
    dobj = DObjGetStruct(effect_gobj);

    dobj->user_data.p = ftGetStruct(fighter_gobj)->joints[nFTPartsJointTopN];
    dobj->rotate.vec.f.y = fp->lr * F_CLC_DTOR32(90.0F);

    return effect_gobj;
}

// 0x80102490
GObj* efManagerKirbyCutterDownMakeEffect(GObj *fighter_gobj)
{
    GObj *effect_gobj;
    EFStruct *ep;
    FTStruct *fp;
    DObj *dobj;

    effect_gobj = efManagerMakeEffectForce(&dEFManagerKirbyCutterDownEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);
    ep->fighter_gobj = fighter_gobj;

    fp = ftGetStruct(fighter_gobj);
    dobj = DObjGetStruct(effect_gobj);

    dobj->user_data.p = ftGetStruct(fighter_gobj)->joints[nFTPartsJointTopN];
    dobj->rotate.vec.f.y = fp->lr * F_CLC_DTOR32(90.0F);

    return effect_gobj;
}

// 0x80102508
GObj* efManagerKirbyCutterDrawMakeEffect(GObj *fighter_gobj)
{
    GObj *effect_gobj;
    EFStruct *ep;
    DObj *dobj;

    effect_gobj = efManagerMakeEffectForce(&dEFManagerKirbyCutterDrawEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);

    ep->fighter_gobj = fighter_gobj;

    dobj = DObjGetStruct(effect_gobj);

    dobj->user_data.p = ftGetStruct(fighter_gobj)->joints[17];

    return effect_gobj;
}

// 0x80102560
GObj* efManagerKirbyCutterTrailMakeEffect(GObj *fighter_gobj)
{
    GObj *effect_gobj;
    EFStruct *ep;
    FTStruct *fp;
    DObj *dobj;

    effect_gobj = efManagerMakeEffectForce(&dEFManagerKirbyCutterTrailEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);
    ep->fighter_gobj = fighter_gobj;

    fp = ftGetStruct(fighter_gobj);
    dobj = DObjGetStruct(effect_gobj);

    dobj->user_data.p = ftGetStruct(fighter_gobj)->joints[17];
    dobj->rotate.vec.f.y = fp->lr * F_CLC_DTOR32(90.0F);

    return effect_gobj;
}

// 0x801025D8
GObj* efManagerNessPsychicMagnetMakeEffect(GObj *fighter_gobj)
{
    GObj *effect_gobj;
    EFStruct *ep;
    DObj *dobj;

    effect_gobj = efManagerMakeEffectForce(&dEFManagerNessPsychicMagnetEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);

    ep->fighter_gobj = fighter_gobj;

    dobj = DObjGetStruct(effect_gobj);

    dobj->user_data.p = ftGetStruct(fighter_gobj)->joints[nFTPartsJointTopN];

    return effect_gobj;
}

// 0x80102630
void efManagerNessPKThunderTrailProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);
    FTStruct *fp;
    s32 index;

    if (ep->effect_vars.pkthunder.status & nWPNessPKThunderStatusDestroy)
    {
        efManagerSetPrevStructAlloc(ep);
        gcEjectGObj(effect_gobj);

        return;
    }
    fp = ftGetStruct(ep->effect_vars.pkthunder.owner_gobj);

    index = fp->passive_vars.ness.pkthunder_trail_id - (FTNESS_PKTHUNDER_TRAIL_POS_COUNT - 2);

    if (index < 0)
    {
        index += FTNESS_PKTHUNDER_TRAIL_POS_COUNT;
    }
    DObjGetStruct(effect_gobj)->translate.vec.f.x = fp->passive_vars.ness.pkthunder_trail_x[index];
    DObjGetStruct(effect_gobj)->translate.vec.f.y = fp->passive_vars.ness.pkthunder_trail_y[index];

    if (index > 0)
    {
        DObjGetStruct(effect_gobj)->rotate.vec.f.z = syUtilsArcTan2((fp->passive_vars.ness.pkthunder_trail_y[index] - fp->passive_vars.ness.pkthunder_trail_y[index - 1]), (fp->passive_vars.ness.pkthunder_trail_x[index] - fp->passive_vars.ness.pkthunder_trail_x[index - 1]));
    }
    else
    {
        DObjGetStruct(effect_gobj)->rotate.vec.f.z = syUtilsArcTan2((fp->passive_vars.ness.pkthunder_trail_y[index] - fp->passive_vars.ness.pkthunder_trail_y[FTNESS_PKTHUNDER_TRAIL_POS_COUNT - 1]), (fp->passive_vars.ness.pkthunder_trail_x[index] - fp->passive_vars.ness.pkthunder_trail_x[FTNESS_PKTHUNDER_TRAIL_POS_COUNT - 1]));
    }
    DObjGetStruct(effect_gobj)->rotate.vec.f.z -= F_CLC_DTOR32(90.0F);
}

// 0x80102768
void efManagerNessPKThunderTrailProcDisplay(GObj *effect_gobj)
{
    gDPPipeSync(gSYTaskmanDLHeads[1]++);
    gDPSetRenderMode(gSYTaskmanDLHeads[1]++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
    gDPSetAlphaCompare(gSYTaskmanDLHeads[1]++, G_AC_NONE);

    gcDrawDObjDLLinksForGObj(effect_gobj);

    gDPPipeSync(gSYTaskmanDLHeads[1]++);
    gDPSetRenderMode(gSYTaskmanDLHeads[1]++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
    gDPSetAlphaCompare(gSYTaskmanDLHeads[1]++, G_AC_THRESHOLD);
}

// 0x80102848
GObj* efManagerNessPKThunderTrailMakeEffect(GObj *fighter_gobj)
{
    GObj *effect_gobj;
    FTStruct *fp;
    EFStruct *ep;
    WPStruct *wp;
    GObj *head_gobj;
    s32 effect_slot;

    if (fighter_gobj == NULL)
    {
        return NULL;
    }
    fp = ftGetStruct(fighter_gobj);
    if (fp == NULL)
    {
        return NULL;
    }
    head_gobj = fp->status_vars.ness.specialhi.pkthunder_gobj;
    if (wpNessPKThunderGObjIsLiveWeapon(head_gobj) == FALSE)
    {
        return NULL;
    }
    wp = wpGetStruct(head_gobj);
    if (wp == NULL)
    {
        return NULL;
    }
    effect_slot = ARRAY_COUNT(wp->weapon_vars.pkthunder.trail_gobj) - 1;
    if ((wp->weapon_vars.pkthunder.trail_gobj[effect_slot] != NULL) &&
        (wpNessPKThunderGObjIsLiveEffect(wp->weapon_vars.pkthunder.trail_gobj[effect_slot]) != FALSE))
    {
        return wp->weapon_vars.pkthunder.trail_gobj[effect_slot];
    }

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerNessPKThunderTrailEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);

    ep->effect_vars.pkthunder.owner_gobj = fighter_gobj;
    ep->effect_vars.pkthunder.status = nWPNessPKThunderStatusActive;

    DObjGetStruct(effect_gobj)->translate.vec.f.z = 0.0F;

    wp->weapon_vars.pkthunder.trail_gobj[effect_slot] = effect_gobj;

    efManagerNessPKThunderTrailProcUpdate(effect_gobj);

    return effect_gobj;
}

// 0x801028C0
void efManagerNessPKReflectTrailProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);

    if (ep->effect_vars.pkthunder.status & nWPNessPKThunderStatusDestroy)
    {
        efManagerSetPrevStructAlloc(ep);
        gcEjectGObj(effect_gobj);
    }
    else
    {
        WPStruct *wp = wpGetStruct(ep->effect_vars.pkthunder.owner_gobj);

        DObjGetStruct(effect_gobj)->translate.vec.f.x = (DObjGetStruct(ep->effect_vars.pkthunder.owner_gobj)->translate.vec.f.x - wp->physics.vel_air.x * 5.0F * 2);
        DObjGetStruct(effect_gobj)->translate.vec.f.y = (DObjGetStruct(ep->effect_vars.pkthunder.owner_gobj)->translate.vec.f.y - wp->physics.vel_air.y * 5.0F * 2);
    }
}

// 0x80102968
GObj* efManagerNessPKReflectTrailMakeEffect(GObj *weapon_gobj)
{
    GObj *effect_gobj;
    WPStruct *wp;
    EFStruct *ep;
    s32 effect_slot;

    if (wpNessPKThunderGObjIsLiveWeapon(weapon_gobj) == FALSE)
    {
        return NULL;
    }
    wp = wpGetStruct(weapon_gobj);
    if (wp == NULL)
    {
        return NULL;
    }
    effect_slot = ARRAY_COUNT(wp->weapon_vars.pkthunder.trail_gobj) - 1;
    if ((wp->weapon_vars.pkthunder.trail_gobj[effect_slot] != NULL) &&
        (wpNessPKThunderGObjIsLiveEffect(wp->weapon_vars.pkthunder.trail_gobj[effect_slot]) != FALSE))
    {
        return wp->weapon_vars.pkthunder.trail_gobj[effect_slot];
    }

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerNessPKReflectTrailEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);

    ep->effect_vars.pkthunder.owner_gobj = weapon_gobj;
    ep->effect_vars.pkthunder.status = nWPNessPKThunderStatusActive;

    DObjGetStruct(effect_gobj)->translate.vec.f.z = 0.0F;

    DObjGetStruct(effect_gobj)->rotate.vec.f.z = DObjGetStruct(weapon_gobj)->rotate.vec.f.z - F_CLC_DTOR32(90.0F);

    wp->weapon_vars.pkthunder.trail_gobj[ARRAY_COUNT(wp->weapon_vars.pkthunder.trail_gobj) - 1] = effect_gobj;

    efManagerNessPKReflectTrailProcUpdate(effect_gobj);

    return effect_gobj;
}

// 0x801029F8
GObj* efManagerNessPKThunderWaveMakeEffect(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *effect_gobj = efManagerMakeEffectNoForce(&dEFManagerNessPKThunderWaveEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    else
    {
        EFStruct *ep = efGetStruct(effect_gobj);

        ep->fighter_gobj = fighter_gobj;

        DObjGetStruct(effect_gobj)->user_data.p = fp->joints[5];

        DObjGetStruct(effect_gobj)->rotate.vec.f.y = fp->lr * F_CLC_DTOR32(90.0F);
        DObjGetStruct(effect_gobj)->translate.vec.f.z = 0.0F;

        return effect_gobj;
    }
}

// 0x80102A88 - Unused? (Edit 6/4/2024: PK FLASH? WTF???)
GObj* efManagerNessPKFlashMakeEffect(Vec3f *pos)
{
    GObj *effect_gobj;
    DObj *dobj;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerNessPKFlashEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);

    dobj->translate.vec.f = *pos;

    return effect_gobj;
}

// 0x80102AE4
GObj* efManagerLinkEntryWaveMakeEffect(Vec3f *pos)
{
    GObj *effect_gobj;
    DObj *dobj;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerLinkEntryWaveEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);

    dobj->translate.vec.f = *pos;

    return effect_gobj;
}

// 0x80102B40
GObj* efManagerLinkEntryBeamMakeEffect(Vec3f *pos)
{
    GObj *effect_gobj;
    DObj *dobj;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerLinkEntryBeamEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);

    dobj->translate.vec.f = *pos;

    return effect_gobj;
}

// 0x80102B9C
GObj* efManagerKirbyEntryStarMakeEffect(Vec3f *pos, s32 lr)
{
    GObj *effect_gobj;
    DObj *dobj;

    dEFManagerKirbyEntryStarEffectDesc.o_anim_joint = (lr == +1) ? llKirbySpecial2EntryStarRAnimJoint : llKirbySpecial2EntryStarLAnimJoint;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerKirbyEntryStarEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);

    dobj->translate.vec.f = *pos;

    return effect_gobj;
}

// 0x80102C28
GObj* efManagerMBallRaysMakeEffect(Vec3f *pos)
{
    GObj *effect_gobj;
    DObj *dobj;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerMBallRaysEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);

    dobj->translate.vec.f = *pos;

    return effect_gobj;
}

// 0x80102C84
void efManagerMBallThrownProcUpdate(GObj *effect_gobj)
{
    DObj *dobj = DObjGetStruct(effect_gobj)->child;

    if (dobj->translate.vec.f.z > 1000.0F)
    {
        if (dobj->parent_gobj->dl_link_id != 20)
        {
            gcMoveGObjDL(dobj->parent_gobj, 20, 2);
        }
    }
    else if (dobj->parent_gobj->dl_link_id != 10)
    {
        gcMoveGObjDL(dobj->parent_gobj, 10, 2);
    }
    efManagerHaveStructProcUpdate(effect_gobj);
}

// 0x80102D14
GObj* efManagerMBallThrownMakeEffect(Vec3f *pos, s32 lr)
{
    GObj *effect_gobj;
    DObj *dobj;
#ifdef PORT
    // PORT reloc writes a 4-byte token into the pointer slot, not an 8-byte
    // void*; read it as u32 and resolve, or *p_file slurps the next slot's
    // token too and produces a garbage pointer (e.g. 0x3a3000003a2).
    u32 *p_file;
#else
    void **p_file;
#endif
    void *file;

    dEFManagerMBallThrownEffectDesc.file_head = &file;

#ifdef PORT
    p_file = lbRelocGetFileData(u32*, gITManagerCommonData, llITCommonDataMBallThrownFileHead);
    file = (void *)((uintptr_t)PORT_RESOLVE(*p_file) - (intptr_t)llITCommonDataMBallThrownDObjDesc);
#else
    p_file = lbRelocGetFileData(void**, gITManagerCommonData, llITCommonDataMBallThrownFileHead);
    file = (void *)((uintptr_t)*p_file - (intptr_t)llITCommonDataMBallThrownDObjDesc);
#endif

    if (lr == +1)
    {
        dEFManagerMBallThrownEffectDesc.o_anim_joint = (intptr_t)llITCommonDataMBallThrownRAnimJoint;
        dEFManagerMBallThrownEffectDesc.o_matanim_joint = (intptr_t)llITCommonDataMBallThrownRMatAnimJoint;
    }
    else
    {
        dEFManagerMBallThrownEffectDesc.o_anim_joint = (intptr_t)llITCommonDataMBallThrownLAnimJoint;
        dEFManagerMBallThrownEffectDesc.o_matanim_joint = (intptr_t)llITCommonDataMBallThrownLMatAnimJoint;
    }
    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerMBallThrownEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);

    dobj->translate.vec.f = *pos;

    efManagerSortZNeg(dobj->child);

    return effect_gobj;
}

// 0x80102DEC
LBParticle* efManagerFireGrindMakeEffect(Vec3f *pos)
{
    LBParticle *pc = lbParticleMakeScriptID(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0xB);

    if (pc != NULL)
    {
        LBTransform *xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf != NULL)
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            xf->translate = *pos;
        }
        else
        {
            lbParticleEjectStruct(pc);

            pc = NULL;
        }
    }
    return pc;
}

// 0x80102E90
LBParticle* efManagerHealSparklesMakeEffect(Vec3f *pos)
{
    LBParticle *pc = lbParticleMakeScriptID(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0xE);

    if (pc != NULL)
    {
        LBTransform *xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf != NULL)
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            xf->translate = *pos;
        }
        else
        {
            lbParticleEjectStruct(pc);

            pc = NULL;
        }
    }
    return pc;
}

// 0x80102F34
GObj* efManagerYoshiEntryEggMakeEffect(Vec3f *pos)
{
    GObj *effect_gobj;
    DObj *dobj;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerYoshiEntryEggEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);

    dobj->translate.vec.f = *pos;

    return effect_gobj;
}

// 0x80102F90
void efManagerYoshiEggLaySetAnim(GObj *effect_gobj, s32 index)
{
    EFStruct *ep = efGetStruct(effect_gobj);

    ep->effect_vars.yoshi_egg_lay.index = index;

    lbCommonAddDObjAnimJointAll(DObjGetStruct(effect_gobj)->child, lbRelocGetFileData(AObjEvent32**, gFTDataYoshiSpecial3, dEFManagerYoshiEggLayAnimJoints[index]), 1.0F);
}

// 0x80102FE4
void efManagerYoshiEggLayProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);

    if (ep->effect_vars.yoshi_egg_lay.force_index != ep->effect_vars.yoshi_egg_lay.index)
    {
        efManagerYoshiEggLaySetAnim(effect_gobj, ep->effect_vars.yoshi_egg_lay.force_index);
    }
    gcPlayAnimAll(effect_gobj);

    if ((ep->effect_vars.yoshi_egg_lay.index == 2) && (effect_gobj->anim_frame <= 0.0F))
    {
        ep->effect_vars.yoshi_egg_lay.force_index = 0;
    }
}

// 0x80103060
GObj* efManagerYoshiEggLayMakeEffect(GObj *fighter_gobj)
{
    void *unused;
    FTStruct *fp;
    EFStruct *ep;
    GObj *effect_gobj;
    DObj *dobj;

    fp = ftGetStruct(fighter_gobj);

    effect_gobj = efManagerMakeEffectForce(&dEFManagerYoshiEggLayEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);
    ep->fighter_gobj = fighter_gobj;
    ep->effect_vars.yoshi_egg_lay.index = ep->effect_vars.yoshi_egg_lay.force_index = 2;

    dobj = DObjGetStruct(effect_gobj);
    dobj->user_data.p = ftGetStruct(fighter_gobj)->joints[nFTPartsJointTopN];

#ifdef PORT
    {
        ftCommonYoshiEggDesc *egg_desc = (ftCommonYoshiEggDesc *)port_fighter_yoshi_egg_damage_coll(fp->fkind);
        dobj->scale.vec.f.x = dobj->scale.vec.f.y = egg_desc->effect_size;
    }
#else
    dobj->scale.vec.f.x = dobj->scale.vec.f.y = dFTCommonYoshiEggDamageCollDescs[fp->fkind].effect_size;
#endif
    dobj->scale.vec.f.z = 1.0F;

    dobj->child->child->xobjs[0]->kind = nGCMatrixKindTra;

    gcAddXObjForDObjFixed(dobj->child->child, 0x2E, 0);
    lbCommonSetDObjTransformsForTreeDObjs(dobj->child, lbRelocGetFileData(DObjDesc*, gFTDataYoshiSpecial3, llYoshiSpecial3EggLayDObjDesc));

    return effect_gobj;
}

// 0x80103150
GObj* efManagerYoshiEggEscapeMakeEffect(GObj *fighter_gobj)
{
    FTStruct *fp;
    EFStruct *ep;
    GObj *effect_gobj;
    DObj *dobj;

    fp = ftGetStruct(fighter_gobj);

    effect_gobj = efManagerMakeEffectForce(&dEFManagerYoshiEggEscapeEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ftParamHideModelPartAll(fighter_gobj);

    fp->is_effect_attach = TRUE;

    ep = efGetStruct(effect_gobj);

    ep->fighter_gobj = fighter_gobj;

    dobj = DObjGetStruct(effect_gobj);

    dobj->scale.vec.f.x = dobj->scale.vec.f.y = 1.5F;

    dobj->user_data.p = ftGetStruct(fighter_gobj)->joints[5];

    return effect_gobj;
}

// 0x801031E0
LBParticle* func_ovl2_801031E0(Vec3f *pos)
{
    LBParticle *pc = lbParticleMakeScriptID(gFTDataKirbyParticleBankID, 2);

    if (pc != NULL)
    {
        LBTransform *xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf != NULL)
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            xf->translate = *pos;
        }
        else
        {
            lbParticleEjectStruct(pc);

            pc = NULL;
        }
    }
    return pc;
}

// 0x80103280
LBParticle* func_ovl2_80103280(Vec3f *pos)
{
    LBParticle *pc = lbParticleMakeScriptID(gFTDataKirbyParticleBankID, 5);

    if (pc != NULL)
    {
        LBTransform *xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf != NULL)
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            xf->translate = *pos;
        }
        else
        {
            lbParticleEjectStruct(pc);

            pc = NULL;
        }
    }
    return pc;
}

// 0x80103320
LBParticle* efManagerFoxBlasterGlowMakeEffect(Vec3f *pos)
{
    LBParticle *pc;

    pc = lbParticleMakeCommon(gEFManagerParticleBankID, 0x62);

    if (pc != NULL)
    {
        pc->pos.x = pos->x;
        pc->pos.y = pos->y;
        pc->pos.z = pos->z;
    }
    return pc;
}

// 0x80103378
GObj* efManagerLinkSpinAttackMakeEffect(GObj *fighter_gobj)
{
    GObj *effect_gobj;
    EFStruct *ep;
    FTStruct *fp;
    DObj *dobj;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerLinkSpinAttackEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);

    ep->fighter_gobj = fighter_gobj;

    fp = ftGetStruct(fighter_gobj);

    fp->proc_lagstart = ftParamProcPauseEffect;
    fp->proc_lagend = ftParamProcResumeEffect;

    dobj = DObjGetStruct(effect_gobj);

    dobj->user_data.p = fp->joints[nFTPartsJointTopN];

    dobj->rotate.vec.f.y = (ftGetStruct(fighter_gobj)->lr == +1) ? F_CLC_DTOR32(30.0F) : F_CLC_DTOR32(210.0F);

    return effect_gobj;
}

// 0x80103418
GObj* efManagerDonkeyEntryTaruMakeEffect(Vec3f *pos)
{
    GObj *effect_gobj;
    DObj *dobj;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerDonkeyEntryTaruEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);

    dobj->translate.vec.f = *pos;

    return effect_gobj;
}

// 0x80103474
GObj* efManagerSamusEntryPointMakeEffect(Vec3f *pos)
{
    GObj *effect_gobj;
    DObj *dobj;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerSamusEntryPointEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);

    dobj->translate.vec.f = *pos;

    return effect_gobj;
}

// 0x801034D0
void efManagerCaptainEntryCarProcUpdate(GObj *effect_gobj)
{
    DObj *dobj = DObjGetStruct(effect_gobj)->child;

    gcPlayAnimAll(effect_gobj);

    if (dobj->anim_frame <= 0.0F)
    {
        efManagerSetPrevStructAlloc(efGetStruct(effect_gobj));
        gcEjectGObj(effect_gobj);
    }
    else if (DObjGetStruct(effect_gobj)->rotate.vec.f.y == F_CLC_DTOR32(0.0F)) // This could mean trouble if the macro is changed... Need different zero literals
    {
        efManagerSortZNeg(dobj);
    }
    else efManagerSortZPos(dobj);
}

// 0x8010356C
GObj* efManagerCaptainEntryCarMakeEffect(Vec3f *pos, s32 lr)
{
    DObj *node_dobj;
    GObj *effect_gobj;
    DObj *dobj;
    s32 i;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerCaptainEntryCarEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);

    gcAddAnimJointAll(effect_gobj, lbRelocGetFileData(AObjEvent32**, gFTDataCaptainSpecial2, llCaptainSpecial2_6200_AnimJoint), 0.0F);

    node_dobj = dobj->child->child->child;

    for (i = nFTPartsJointCommonStart; i > 0; i--)
    {
        gcAddXObjForDObjFixed(node_dobj, nGCMatrixKindRecalcRotRpyRSca, 0);
        gcAddDObjAnimJoint(node_dobj, lbRelocGetFileData(AObjEvent32*, gFTDataCaptainSpecial2, llCaptainSpecial2_6518_AnimJoint), 0.0F);

        node_dobj = node_dobj->sib_next;

        gcAddDObjAnimJoint(node_dobj, lbRelocGetFileData(AObjEvent32*, gFTDataCaptainSpecial2, llCaptainSpecial2_6598_AnimJoint), 0.0F);

        node_dobj = node_dobj->sib_next;
    }
    gcPlayAnimAll(effect_gobj);

    dobj->translate.vec.f = *pos;

    if (lr == -1)
    {
        dobj->rotate.vec.f.y = F_CLC_DTOR32(180.0F);
    }
    if (DObjGetStruct(effect_gobj)->rotate.vec.f.y == F_CLC_DTOR32(0.0F))
    {
        efManagerSortZNeg(dobj->child);
    }
    else efManagerSortZPos(dobj->child);

    return effect_gobj;
}

// 0x801036EC
GObj* efManagerMarioEntryDokanMakeEffect(Vec3f *pos, s32 fkind)
{
    GObj *effect_gobj;
    DObj *dobj;

    switch (fkind)
    {
    case nFTKindMario:
        dEFManagerMarioEntryDokanEffectDesc.file_head = &gFTMarioFileSpecial2;
        break;

    case nFTKindLuigi:
        dEFManagerMarioEntryDokanEffectDesc.file_head = &gFTDataLuigiSpecial2;
        break;
    }
    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerMarioEntryDokanEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);

    dobj->translate.vec.f = *pos;

    return effect_gobj;
}

// 0x80103780
void efManagerFoxEntryArwingProcUpdate(GObj *effect_gobj)
{
    DObj *dobj = DObjGetStruct(effect_gobj)->child;

    gcPlayAnimAll(effect_gobj);

    if (dobj->anim_frame <= 0.0F)
    {
        efManagerSetPrevStructAlloc(efGetStruct(effect_gobj));
        gcEjectGObj(effect_gobj);
    }
    else efManagerSortZNeg(dobj);
}

// 0x801037EC
GObj* efManagerFoxEntryArwingMakeEffect(Vec3f *pos, s32 lr)
{
    GObj *effect_gobj;
    DObj *dobj;
    DObj *what;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerFoxEntryArwingEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);
    what = dobj->child->child->child->sib_next->sib_next->sib_next->sib_next->sib_next->sib_next->child;

    gcAddXObjForDObjFixed(what, 0x2C, 0);
    gcAddDObjAnimJoint(what, lbRelocGetFileData(AObjEvent32*, gFTDataFoxSpecial3, llFoxSpecial3_2E74_AnimJoint), 0.0F);

    if (lr == +1)
    {
        lbCommonAddDObjAnimJointAll(dobj->child, lbRelocGetFileData(AObjEvent32**, gFTDataFoxSpecial2, llFoxSpecial2EntryArwingRAnimJoint), 0.0F);
    }
    else lbCommonAddDObjAnimJointAll(dobj->child, lbRelocGetFileData(AObjEvent32**, gFTDataFoxSpecial2, llFoxSpecial2EntryArwingLAnimJoint), 0.0F);

    gcPlayAnimAll(effect_gobj);

    dobj->translate.vec.f = *pos;

    efManagerSortZNeg(dobj->child);

    return effect_gobj;
}

// 0x80103918
void efManagerStockCommonMakeEffectID(f32 pos_x, f32 pos_y, s32 script_id)
{
    pos_x *= 4.0F;
    pos_y *= 4.0F;

    lbParticleMakePosVel(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(2), script_id, pos_x, pos_y, 0.0F, 0.0F, 0.0F, 0.0F);
}

// 0x80103974
void efManagerStockSnapMakeEffect(f32 pos_x, f32 pos_y)
{
    efManagerStockCommonMakeEffectID(pos_x, pos_y, 0x26);
}

// 0x80103994
void efManagerStockStealStartMakeEffect(f32 pos_x, f32 pos_y)
{
    efManagerStockCommonMakeEffectID(pos_x, pos_y, 0x75);
}

// 0x801039B4
void efManagerStockStealEndMakeEffect(f32 pos_x, f32 pos_y)
{
    efManagerStockCommonMakeEffectID(pos_x, pos_y, 0x76);
}

// 0x801039D4
LBParticle* efManagerMusicNoteMakeEffect(Vec3f *pos)
{
    LBParticle *pc = lbParticleMakeScriptID
    (
        gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(0),
        dEFManagerMusicNoteScriptIDs[syUtilsRandIntRange(ARRAY_COUNT(dEFManagerMusicNoteScriptIDs))]
    );

    if (pc != NULL)
    {
        LBTransform *xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf != NULL)
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            xf->translate = *pos;
        }
        else
        {
            lbParticleEjectStruct(pc);

            pc = NULL;
        }
    }
    return pc;
}

// 0x80103A88
LBParticle* efManagerYoshiEggExplodeMakeEffect(Vec3f *pos)
{
    LBParticle *pc = lbParticleMakeScriptID(gFTDataYoshiParticleBankID, 3);

    if (pc != NULL)
    {
        LBTransform *xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf != NULL)
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            xf->translate = *pos;
        }
        else
        {
            lbParticleEjectStruct(pc);

            pc = NULL;
        }
    }
    return pc;
}

// 0x80103B28
void efManagerCaptureKirbyStarProcUpdate(GObj *effect_gobj)
{
    DObj *topn_dobj;
    EFStruct *ep;
    FTStruct *fp;
    FTKirbyCopy *copy;
    Vec3f pos;
    DObj *child_dobj;

    ep = efGetStruct(effect_gobj);
    fp = ftGetStruct(ep->fighter_gobj);
    topn_dobj = DObjGetStruct(effect_gobj);

    copy = lbRelocGetFileData(FTKirbyCopy*, gFTDataKirbyMainMotion, llKirbyMainMotionSpecialNFTKirbyCopy);

    child_dobj = topn_dobj->child;

    topn_dobj->translate.vec.f.z = 0.0F;

    child_dobj->rotate.vec.f.z += EFCOMMON_CAPTUREKIRBYSTAR_ROTATE_STEP;

    if (ep->effect_vars.capture_kirby_star.effect_timer % EFCOMMON_CAPTUREKIRBYSTAR_SPARK_TIMER_MOD)
    {
        f32 effect_scale = copy[fp->fkind].effect_scale;
        pos = DObjGetStruct(ep->fighter_gobj)->translate.vec.f;

        pos.y += syUtilsRandIntRange(effect_scale * EFCOMMON_CAPTUREKIRBYSTAR_SPARK_SCATTER_Y);

        if (fp->physics.vel_air.x > 0.0F)
        {
            pos.x -= syUtilsRandIntRange(effect_scale * EFCOMMON_CAPTUREKIRBYSTAR_SPARK_SCATTER_X);
            efManagerStarRodSparkMakeEffect(&pos, -1);
        }
        else
        {
            pos.x += syUtilsRandIntRange(effect_scale * EFCOMMON_CAPTUREKIRBYSTAR_SPARK_SCATTER_X);
            efManagerStarRodSparkMakeEffect(&pos, +1);
        }
    }
    ep->effect_vars.capture_kirby_star.effect_timer++;
}

// 0x80103CF8
GObj* efManagerCaptureKirbyStarMakeEffect(GObj *fighter_gobj)
{
    GObj *effect_gobj;
    EFStruct *ep;
    void *addr;
#ifdef PORT
    u32 *p_addr;
#else
    void **p_addr;
#endif
    DObj *dobj;
    FTKirbyCopy *copy;

    copy = lbRelocGetFileData(FTKirbyCopy*, gFTDataKirbyMainMotion, llKirbyMainMotionSpecialNFTKirbyCopy);

    dEFManagerCaptureKirbyStarEffectDesc.file_head = &addr;

#ifdef PORT
    // See efManagerMBallThrownMakeEffect: PORT reloc slot is u32 token.
    p_addr = lbRelocGetFileData(u32*, gITManagerCommonData, llITCommonDataStarRodWeaponAttributes);
    addr = (void*) ((uintptr_t)PORT_RESOLVE(*p_addr) - (intptr_t)llITCommonDataKirbyStarDObjDesc);
#else
    p_addr = lbRelocGetFileData(void**, gITManagerCommonData, llITCommonDataStarRodWeaponAttributes);
    addr = (void*) ((uintptr_t)*p_addr - (intptr_t)llITCommonDataKirbyStarDObjDesc);
#endif

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerCaptureKirbyStarEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);
    ep->fighter_gobj = fighter_gobj;

    dobj = DObjGetStruct(effect_gobj);

    dobj->translate.vec.f.y += EFCOMMON_CAPTUREKIRBYSTAR_SPARK_OFF_Y;

    dobj->child->user_data.p = ftGetStruct(fighter_gobj)->joints[nFTPartsJointTopN];

    dobj->child->scale.vec.f.x = dobj->child->scale.vec.f.y = copy[ftGetStruct(fighter_gobj)->fkind].effect_scale;
    dobj->child->scale.vec.f.z = 1.0F;

    ep->effect_vars.capture_kirby_star.effect_timer = 0;

    return effect_gobj;
}

// 0x80103DF8
void efManagerLoseKirbyStarProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);
    DObj *dobj = DObjGetStruct(effect_gobj)->child;
    Vec3f *translate = &dobj->translate.vec.f;

    dobj->rotate.vec.f.z += F_CLC_DTOR32(10.0F);

    dobj->translate.vec.f.x += ep->effect_vars.lose_kirby_star.vel.x;
    dobj->translate.vec.f.y += ep->effect_vars.lose_kirby_star.vel.y;

    ep->effect_vars.lose_kirby_star.vel.y -= EFCOMMON_LOSEKIRBYSTAR_GRAVITY;

    if (ep->effect_vars.lose_kirby_star.vel.y < EFCOMMON_LOSEKIRBYSTAR_TVEL)
    {
        ep->effect_vars.lose_kirby_star.vel.y = EFCOMMON_LOSEKIRBYSTAR_TVEL;
    }
    if (ep->effect_vars.lose_kirby_star.lifetime-- <= 0)
    {
        func_800269C0_275C0(nSYAudioFGMKirbyStarPing1);
        efManagerStarSplashMakeEffect(translate, ep->effect_vars.lose_kirby_star.lr);
        efManagerSetPrevStructAlloc(ep);
        gcEjectGObj(effect_gobj);
    }
    else if
    (
        (gMPCollisionGroundData->map_bound_bottom > translate->y) ||
        (gMPCollisionGroundData->map_bound_right  < translate->x) ||
        (gMPCollisionGroundData->map_bound_left   > translate->x) ||
        (gMPCollisionGroundData->map_bound_top    < translate->y)
    )
    {
        efManagerSetPrevStructAlloc(ep);
        gcEjectGObj(effect_gobj);
    }
}

// 0x80103F78
GObj* efManagerLoseKirbyStarMakeEffect(GObj *fighter_gobj)
{
    GObj *effect_gobj;
    EFStruct *ep;
    void *addr;
#ifdef PORT
    u32 *p_addr;
#else
    void **p_addr;
#endif
    DObj *dobj;

    dEFManagerLoseKirbyStarEffectDesc.file_head = &addr;

#ifdef PORT
    // See efManagerMBallThrownMakeEffect: PORT reloc slot is u32 token.
    p_addr = lbRelocGetFileData(u32*, gITManagerCommonData, llITCommonDataStarRodWeaponAttributes);
    addr = (void *)((uintptr_t)PORT_RESOLVE(*p_addr) - (intptr_t)llITCommonDataKirbyStarDObjDesc);
#else
    p_addr = lbRelocGetFileData(void**, gITManagerCommonData, llITCommonDataStarRodWeaponAttributes);
    addr = (void *)((uintptr_t)*p_addr - (intptr_t)llITCommonDataKirbyStarDObjDesc);
#endif

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerLoseKirbyStarEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);

    ep->effect_vars.lose_kirby_star.vel.x = ftGetStruct(fighter_gobj)->lr * EFCOMMON_LOSEKIRBYSTAR_VEL_X;
    ep->effect_vars.lose_kirby_star.vel.y = EFCOMMON_LOSEKIRBYSTAR_VEL_Y;

    ep->effect_vars.lose_kirby_star.lifetime = EFCOMMON_LOSEKIRBYSTAR_LIFETIME;

    ep->effect_vars.lose_kirby_star.lr = ftGetStruct(fighter_gobj)->lr;

    dobj = DObjGetStruct(effect_gobj);
    dobj->translate.vec.f.y += EFCOMMON_LOSEKIRBYSTAR_OFF_Y;

    dobj->child->translate.vec.f = DObjGetStruct(fighter_gobj)->translate.vec.f;

    return effect_gobj;
}

// 0x80104068
GObj* efManagerRebirthHaloMakeEffect(GObj *fighter_gobj, f32 scale)
{
    GObj *effect_gobj;
    EFStruct *ep;
    DObj *dobj, *child;

    effect_gobj = efManagerMakeEffectForce(&dEFManagerRebirthHaloEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    ep = efGetStruct(effect_gobj);

    ep->fighter_gobj = fighter_gobj;

    dobj = DObjGetStruct(effect_gobj);
    dobj->user_data.p = ftGetStruct(fighter_gobj)->joints[nFTPartsJointTopN];

    child = DObjGetStruct(effect_gobj)->child;
    child->scale.vec.f.x = child->scale.vec.f.y = child->scale.vec.f.z = scale;

    return effect_gobj;
}

// 0x801040E0
LBParticle* efManagerBattleScoreMakeEffect(Vec3f *pos, s32 score)
{
    LBParticle *pc = lbParticleMakeScriptID(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(2), (score > 0) ? 0x43 : 0x44);

    if (pc != NULL)
    {
        LBTransform *xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf == NULL)
        {
            lbParticleEjectStruct(pc);

            return NULL;
        }
        LBParticleProcessStruct(pc);

        if (xf->users_num == 0)
        {
            return NULL;
        }
        xf->translate = *pos;

        xf->scale.y = 0.25F;
    }
    return pc;
}

// 0x801041A0
LBParticle* efManagerEggBreakMakeEffect(Vec3f *pos)
{
    LBParticle *pc = lbParticleMakeScriptID(gEFManagerParticleBankID, 0x54);

    if (pc != NULL)
    {
        LBTransform *xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf != NULL)
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            xf->translate = *pos;
        }
        else
        {
            lbParticleEjectStruct(pc);

            pc = NULL;
        }
    }
    return pc;
}

// 0x80104240
void efManagerKirbyInhaleWindProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);
#if defined(PORT) && defined(SSB64_NETMENU)
    /* Netplay rollback only: orphan inhale-wind GObjs can outlive EFStruct pool recycle. See
     * docs/bugs/netplay_kirby_inhale_wind_orphan_effect_2026-06-07.md. */
    if ((ep == NULL) || (ep->xf == NULL) || (ep->fighter_gobj == NULL) ||
        (DObjGetStruct(ep->fighter_gobj) == NULL))
    {
        gcEjectGObj(effect_gobj);
        return;
    }
#endif
    LBTransform *xf = ep->xf;

    xf->translate = DObjGetStruct(ep->fighter_gobj)->translate.vec.f;

    xf->translate.x += ftGetStruct(ep->fighter_gobj)->lr * 800.0F;
    xf->translate.y += 230.0F;
}

// 0x801042B4
LBParticle* efManagerKirbyInhaleWindMakeEffect(GObj *fighter_gobj)
{
    LBParticle *pc;
    LBTransform *xf;
    GObj *effect_gobj;
    EFStruct *ep;

    ep = efManagerGetEffectForce();

    if (ep == NULL)
    {
        return 0;
    }
    effect_gobj = gcMakeGObjSPAfter(nGCCommonKindEffect, NULL, nGCCommonLinkIDEffect, GOBJ_PRIORITY_DEFAULT);

    if (effect_gobj == NULL)
    {
        efManagerSetPrevStructAlloc(ep);

        return NULL;
    }
    effect_gobj->user_data.p = ep;

    pc = lbParticleMakeScriptID(gFTDataKirbyParticleBankID | LBPARTICLE_MASK_GENLINK(0), 0xC);

    if (pc != NULL)
    {
        xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusDefault);

        if (xf != NULL)
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            xf->translate = DObjGetStruct(fighter_gobj)->translate.vec.f;

            xf->translate.x += ftGetStruct(fighter_gobj)->lr * 800.0F;
            xf->translate.y += 230.0F;

            xf->scale.x = 1.0F;
            xf->scale.y = 1.0F;
            xf->scale.z = 1.0F;

            xf->rotate.z = (ftGetStruct(fighter_gobj)->lr == -1) ? F_CLC_DTOR32(90.0F) : F_CLC_DTOR32(-90.0F);

            effect_gobj->user_data.p = ep; // y u do dis again

            gcAddGObjProcess(effect_gobj, efManagerKirbyInhaleWindProcUpdate, nGCProcessKindFunc, 3);

            ep->xf = pc->xf;

            ep->bank_id = pc->bank_id;

            ep->fighter_gobj = fighter_gobj;
        }
        else
        {
            lbParticleEjectStruct(pc);

            pc = NULL;
        }
    }
    return pc;
}

// 0x80104458
GObj* efManagerItemGetSwirlProcUpdate(Vec3f *pos)
{
    GObj *effect_gobj;
    DObj *dobj;

    effect_gobj = efManagerMakeEffectNoForce(&dEFManagerItemGetSwirlEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);

    dobj->translate.vec.f = *pos;

    return effect_gobj;
}

// 0x801044B4
LBParticle* efManagerItemSpawnSwirlMakeEffect(Vec3f *pos)
{
    LBParticle *pc = lbParticleMakeScriptID(gEFManagerParticleBankID, 0x69);

    if (pc != NULL)
    {
        LBTransform *xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf != NULL)
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            xf->translate = *pos;
        }
        else
        {
            lbParticleEjectStruct(pc);

            pc = NULL;
        }
    }
    return pc;
}

// 0x80104554
LBParticle* efManagerConfettiMakeEffect(Vec3f *pos, sb32 is_genlink_mask)
{
    LBParticle *pc = (is_genlink_mask != FALSE) ?
    lbParticleMakeScriptID(gEFManagerParticleBankID, 0x70) :
    lbParticleMakeScriptID(gEFManagerParticleBankID | LBPARTICLE_MASK_GENLINK(3), 0x70);

    if (pc != NULL)
    {
        LBTransform *xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf != NULL)
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                return NULL;
            }
            xf->translate = *pos;
        }
        else
        {
            lbParticleEjectStruct(pc);

            pc = NULL;
        }
    }
    return pc;
}
