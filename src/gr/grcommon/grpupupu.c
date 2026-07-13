#include <gr/ground.h>
#include <gr/grcommon/grpupupu.h>
#include <ft/fighter.h>
#include <ef/effect.h>
#include <sc/scene.h>
#include <reloc_data.h>
#ifdef PORT
extern void *func_800269C0_275C0(u16 id);
#endif
#if defined(PORT) && defined(SSB64_NETMENU)
#include <stdlib.h>
#include <string.h>
#include <ef/efdisplay.h>
#include <ef/efparticle.h>
#include <lb/lbparticle.h>
#include <sc/scmanager.h>
#include <sys/netplay_sim_quantize.h>
#include <sys/netrollback.h>
#include <sys/objtypes.h>
extern void port_log(const char *fmt, ...);

static sb32 grPupupuWhispyXfParticleBroken(const LBTransform *xf, s32 particle_link)
{
	f32 max_size;
	s32 drawable;
	s32 structs;

	if ((xf == NULL) || (lbParticleTransformIsAllocated(xf) == FALSE))
	{
		return FALSE;
	}
	drawable = lbParticleCountDrawableForGeneratorID(xf->generator_id, particle_link);
	if (drawable > 0)
	{
		return FALSE;
	}
	max_size = lbParticleGetMaxDrawSizeForGeneratorID(xf->generator_id, particle_link);
	structs = lbParticleCountStructsForGeneratorID(xf->generator_id, particle_link);
	if ((structs > 0) && (max_size > 0.0F))
	{
		/* Live structs warming up — not a broken shell. */
		return FALSE;
	}
	if ((structs > 0) && (max_size <= 0.0F))
	{
		return TRUE;
	}
	if ((structs == 0) && (max_size <= 0.0F))
	{
		return TRUE;
	}
	return FALSE;
}

static sb32 grPupupuWhispyXfNeedsRespawn(const LBTransform *xf)
{
	if (xf == NULL)
	{
		return TRUE;
	}
	return (lbParticleTransformIsAllocated(xf) == FALSE) ? TRUE : FALSE;
}

static sb32 grPupupuWhispyXfNeedsRespawnForLink(const LBTransform *xf, s32 particle_link)
{
	if (grPupupuWhispyXfNeedsRespawn(xf) != FALSE)
	{
		return TRUE;
	}
	return grPupupuWhispyXfParticleBroken(xf, particle_link);
}

static sb32 grPupupuWhispyRepairDiagEnabled(void)
{
	const char *e = getenv("SSB64_NETPLAY_WHISPY_REPAIR_DIAG");

	return (e != NULL) && (e[0] != '\0') && (strcmp(e, "0") != 0);
}

static void grPupupuWhispyLogEffectDisplayGObjs(s32 leaves_link, s32 dust_link)
{
	GObj *gobj;
	u32 leaves_render_mask = 0U;
	u32 leaves_render_size = 0U;
	u32 leaves_render_drawn = 0U;
	u32 leaves_render_culled = 0U;
	u32 leaves_render_tex_miss = 0U;
	u32 dust_render_mask = 0U;
	u32 dust_render_size = 0U;
	u32 dust_render_drawn = 0U;
	u32 dust_render_culled = 0U;
	u32 dust_render_tex_miss = 0U;

	lbParticleWhispyRenderDiagGetLink(
	    leaves_link, &leaves_render_mask, &leaves_render_size, &leaves_render_drawn,
	    &leaves_render_culled, &leaves_render_tex_miss);
	lbParticleWhispyRenderDiagGetLink(
	    dust_link, &dust_render_mask, &dust_render_size, &dust_render_drawn, &dust_render_culled,
	    &dust_render_tex_miss);
	port_log(
	    "SSB64 WhispyRepair: render_tick leaves_link=%d dust_link=%d "
	    "leaves_mask_pass=%u leaves_size_pass=%u leaves_drawn=%u leaves_culled=%u leaves_tex_miss=%u "
	    "dust_mask_pass=%u dust_size_pass=%u dust_drawn=%u dust_culled=%u dust_tex_miss=%u\n",
	    leaves_link,
	    dust_link,
	    leaves_render_mask,
	    leaves_render_size,
	    leaves_render_drawn,
	    leaves_render_culled,
	    leaves_render_tex_miss,
	    dust_render_mask,
	    dust_render_size,
	    dust_render_drawn,
	    dust_render_culled,
	    dust_render_tex_miss);
	for (gobj = gGCCommonLinks[nGCCommonLinkIDEffect]; gobj != NULL; gobj = gobj->link_next)
	{
		u32 mask_lo;
		u32 link_bit;
		sb32 is_infra;

		if (gobj->proc_display == NULL)
		{
			continue;
		}
		is_infra = efDisplayIsInfrastructureGObj(gobj);
		mask_lo = (u32)(gobj->camera_mask & 0xFFFFULL);
		link_bit = (u32)(((1U << leaves_link) | (1U << dust_link)) & mask_lo);
		if ((is_infra == FALSE) && (link_bit == 0U))
		{
			continue;
		}
		if ((is_infra == FALSE) && (gobj->dl_link_id != 15U) && (gobj->dl_link_id != 18U))
		{
			continue;
		}
		port_log(
		    "SSB64 WhispyRepair: display_gobj gobj=%p id=%u dl_link=%u infra=%d mask_lo=0x%X flags=0x%X "
		    "norun=%d hidden=%d frame_draw_last=%u\n",
		    (void *)gobj,
		    (u32)gobj->id,
		    (u32)gobj->dl_link_id,
		    (int)is_infra,
		    mask_lo,
		    (unsigned int)gobj->flags,
		    (int)((gobj->flags & GOBJ_FLAG_NORUN) != 0U),
		    (int)((gobj->flags & GOBJ_FLAG_HIDDEN) != 0U),
		    (unsigned int)gobj->frame_draw_last);
	}
}
#endif

// // // // // // // // // // // //
//                               //
//          ENUMERATORS          //
//                               //
// // // // // // // // // // // //

enum grPupupuWhispyMouthTexture
{
    nGRPupupuWhispyMouthTextureOpen,
    nGRPupupuWhispyMouthTextureBlow,
    nGRPupupuWhispyMouthTextureClose,
    nGRPupupuWhispyMouthTextureEnumCount
};

enum grPupupuWhispyEyesStatus
{
	nGRPupupuWhispyEyesStatusTurn,
	nGRPupupuWhispyEyesStatusBlink,
    nGRPupupuWhispyEyesStatusEnumCount
};

enum grPupupuWhispyEyesTexture
{
    nGRPupupuWhispyEyesTexture0,
    nGRPupupuWhispyEyesTexture1,
    nGRPupupuWhispyEyesTexture2,
    nGRPupupuWhispyEyesTextureEnumCount
};

// // // // // // // // // // // //
//                               //
//       INITIALIZED DATA        //
//                               //
// // // // // // // // // // // //

// 0x8012E870
intptr_t dGRPupupuWhispyEyesAnims[/* */][nGRPupupuWhispyEyesStatusEnumCount][2] =
{
#ifdef PORT
    { { llGRPupupuMapWhispyEyesLeftTurnAnimJoint, llGRPupupuMapWhispyEyesLeftTurnMatAnimJoint }, { llGRPupupuMapWhispyEyesLeftBlinkAnimJoint, 0x0 } },
    { { llGRPupupuMapWhispyEyesRightTurnAnimJoint, llGRPupupuMapWhispyEyesRightTurnMatAnimJoint }, { llGRPupupuMapWhispyEyesRightBlinkAnimJoint, 0x0 } }
#else
    { { &llGRPupupuMapWhispyEyesLeftTurnAnimJoint, &llGRPupupuMapWhispyEyesLeftTurnMatAnimJoint }, { &llGRPupupuMapWhispyEyesLeftBlinkAnimJoint, 0x0 } },
    { { &llGRPupupuMapWhispyEyesRightTurnAnimJoint, &llGRPupupuMapWhispyEyesRightTurnMatAnimJoint }, { &llGRPupupuMapWhispyEyesRightBlinkAnimJoint, 0x0 } }
#endif
};

// 0x8012E890
intptr_t dGRPupupuWhispyMouthAnims[/* */][nGRPupupuWhispyMouthStatusEnumCount][2] =
{
    // Left-facing
    {
#ifdef PORT
        { llGRPupupuMapWhispyMouthLeftStretchAnimJoint, llGRPupupuMapWhispyMouthLeftStretchMatAnimJoint },
        { llGRPupupuMapWhispyMouthLeftTurnAnimJoint, llGRPupupuMapWhispyMouthLeftTurnMatAnimJoint },
        { llGRPupupuMapWhispyMouthLeftOpenAnimJoint, llGRPupupuMapWhispyMouthLeftOpenMatAnimJoint },
        { llGRPupupuMapWhispyMouthLeftCloseAnimJoint, llGRPupupuMapWhispyMouthLeftCloseMatAnimJoint }
#else
        { &llGRPupupuMapWhispyMouthLeftStretchAnimJoint, &llGRPupupuMapWhispyMouthLeftStretchMatAnimJoint },
        { &llGRPupupuMapWhispyMouthLeftTurnAnimJoint, &llGRPupupuMapWhispyMouthLeftTurnMatAnimJoint },
        { &llGRPupupuMapWhispyMouthLeftOpenAnimJoint, &llGRPupupuMapWhispyMouthLeftOpenMatAnimJoint },
        { &llGRPupupuMapWhispyMouthLeftCloseAnimJoint, &llGRPupupuMapWhispyMouthLeftCloseMatAnimJoint }
#endif
    },

    // Right-facing
    {
#ifdef PORT
        { llGRPupupuMapWhispyMouthRightStretchAnimJoint, llGRPupupuMapWhispyMouthRightStretchMatAnimJoint },
        { llGRPupupuMapWhispyMouthRightTurnAnimJoint, llGRPupupuMapWhispyMouthRightTurnMatAnimJoint },
        { llGRPupupuMapWhispyMouthRightOpenAnimJoint, llGRPupupuMapWhispyMouthRightOpenMatAnimJoint },
        { llGRPupupuMapWhispyMouthRightCloseAnimJoint, llGRPupupuMapWhispyMouthRightCloseMatAnimJoint }
#else
        { &llGRPupupuMapWhispyMouthRightStretchAnimJoint, &llGRPupupuMapWhispyMouthRightStretchMatAnimJoint },
        { &llGRPupupuMapWhispyMouthRightTurnAnimJoint, &llGRPupupuMapWhispyMouthRightTurnMatAnimJoint },
        { &llGRPupupuMapWhispyMouthRightOpenAnimJoint, &llGRPupupuMapWhispyMouthRightOpenMatAnimJoint },
        { &llGRPupupuMapWhispyMouthRightCloseAnimJoint, &llGRPupupuMapWhispyMouthRightCloseMatAnimJoint }
#endif
    }
};

// 0x8012E8D0
intptr_t dGRPupupuWhispyMouthTextures[/* */][nGRPupupuWhispyMouthTextureEnumCount] =
{
#ifdef PORT
    { llGRPupupuMapWhispyMouthLeftOpenTexture, llGRPupupuMapWhispyMouthLeftBlowTexture, llGRPupupuMapWhispyMouthLeftCloseTexture },
    { llGRPupupuMapWhispyMouthRightOpenTexture, llGRPupupuMapWhispyMouthRightBlowTexture, llGRPupupuMapWhispyMouthRightCloseTexture }
#else
    { &llGRPupupuMapWhispyMouthLeftOpenTexture, &llGRPupupuMapWhispyMouthLeftBlowTexture, &llGRPupupuMapWhispyMouthLeftCloseTexture },
    { &llGRPupupuMapWhispyMouthRightOpenTexture, &llGRPupupuMapWhispyMouthRightBlowTexture, &llGRPupupuMapWhispyMouthRightCloseTexture }
#endif
};

// 0x8012E8E8
intptr_t dGRPupupuWhispyEyesTextures[/* */][nGRPupupuWhispyEyesTextureEnumCount] =
{
#ifdef PORT
    { llGRPupupuMapWhispyEyesLeft0Texture, llGRPupupuMapWhispyEyesLeft1Texture, llGRPupupuMapWhispyEyesLeft2Texture },
    { llGRPupupuMapWhispyEyesRight0Texture, llGRPupupuMapWhispyEyesRight1Texture, llGRPupupuMapWhispyEyesRight2Texture }
#else
    { &llGRPupupuMapWhispyEyesLeft0Texture, &llGRPupupuMapWhispyEyesLeft1Texture, &llGRPupupuMapWhispyEyesLeft2Texture },
    { &llGRPupupuMapWhispyEyesRight0Texture, &llGRPupupuMapWhispyEyesRight1Texture, &llGRPupupuMapWhispyEyesRight2Texture }
#endif
};

// 0x8012E900
GRPupupuEffect dGRPupupuWhispyLeavesEffectAttributes[/* */] =
{
    { { -715.0F, 450.0F, -696.0F }, F_CLC_DTOR32(-157.0F) },
    { { -205.0F, 450.0F, -762.0F }, F_CLC_DTOR32( -13.0F) }
};

// 0x8012E920
Vec3f dGRPupupuWhispyDustEffectPositions[/* */] =
{
    { -715.0F, 100.0F, 0.0F },
    { -205.0F, 100.0F, 0.0F }
};

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

// 0x801058E0
s32 grPupupuWhispyGetLR(GObj *ground_gobj)
{
    s32 players_rside = 0;
    s32 players_lside = 0;
    GObj *fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];

    while (fighter_gobj != NULL)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if (fp->joints[nFTPartsJointTopN]->translate.vec.f.x > GRPUPUPU_WHISPY_POS_X)
        {
            players_rside++;
        }
        else players_lside++;

        fighter_gobj = fighter_gobj->link_next;
    }
    if (players_rside == players_lside)
    {
        return -1;
    }
    else if (players_rside < players_lside)
    {
        return 0;
    }
    else return 1;
}

// 0x8010595C
void grPupupuWhispySetWindPush(void)
{
    GObj *fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
    sb32 lr_wind = gGRCommonStruct.pupupu.lr_players;

    while (fighter_gobj != NULL)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        DObj *joint = fp->joints[nFTPartsJointTopN];
        f32 dist_x;
        Vec3f push;
        f32 pos_x = joint->translate.vec.f.x, pos_y = joint->translate.vec.f.y;

        if ((pos_y <= GRPUPUPU_WHISPY_WINDBOX_TOP) && (pos_y >= GRPUPUPU_WHISPY_WINDBOX_BOTTOM))
        {
            if
            (
                ((lr_wind == 0) && (pos_x >= GRPUPUPU_WHISPY_WINDBOX_EDGELEFT)  && (pos_x <= GRPUPUPU_WHISPY_POS_X)) ||
                ((lr_wind != 0) && (pos_x <= GRPUPUPU_WHISPY_WINDBOX_EDGERIGHT) && (pos_x >= GRPUPUPU_WHISPY_POS_X))
            )
            {
                dist_x = ((pos_x < GRPUPUPU_WHISPY_POS_X) ? -(pos_x - GRPUPUPU_WHISPY_POS_X) : (pos_x - GRPUPUPU_WHISPY_POS_X));

                push.x = GRPUPUPU_WHISPY_WIND_VEL_BASE - (dist_x * GRPUPUPU_WHISPY_WIND_DIST_DECAY);

                if (push.x > 0.0F)
                {
                    push.z = 0.0F;
                    push.y = 0.0F;

                    push.x = (lr_wind == 0) ? -push.x : push.x;

                    ftParamSetVelPush(fighter_gobj, &push);
                }
            }
        }
        fighter_gobj = fighter_gobj->link_next;
    }
}

// 0x80105AF0
void grPupupuWhispyUpdateSleep(void)
{
    if (gSCManagerBattleState->game_status != nSCBattleGameStatusWait)
    {
        gGRCommonStruct.pupupu.whispy_status = nGRPupupuWhispyWindStatusWait;
    }
}

#if defined(PORT) && defined(SSB64_NETMENU)
#define GRPUPUPU_WHISPY_DUST_GEN_CACHE_MAX 8U
#define GRPUPUPU_WHISPY_DRAWABLE_STALL_RESPAWN_TICKS 30U
#define GRPUPUPU_WHISPY_BLOW_DIAG_INTERVAL 30U

static u16 sGRPupupuWhispyDustSpawnRetryWait;
static u16 sGRPupupuWhispyLeavesSpawnRetryWait;
static u16 sGRPupupuWhispyLeavesDrawableStall;
static u16 sGRPupupuWhispyDustDrawableStall;
static u16 sGRPupupuWhispyBlowDiagTick;
static u8 sGRPupupuWhispyForwardTextureFlowersF = 0xFFU;
static u8 sGRPupupuWhispyForwardTextureFlowersB = 0xFFU;
static u8 sGRPupupuWhispyDustGenCount;
static u16 sGRPupupuWhispyDustGenIds[GRPUPUPU_WHISPY_DUST_GEN_CACHE_MAX];

static void grPupupuWhispyClearDustGeneratorCache(void);

static s32 grPupupuWhispyParticleAllocLink(void)
{
	return (gGRCommonStruct.pupupu.particle_bank_id | LBPARTICLE_MASK_GENLINK(0)) >> 3;
}

static s32 grPupupuWhispyDustParticleAllocLink(void)
{
	return (gGRCommonStruct.pupupu.particle_bank_id | LBPARTICLE_MASK_GENLINK(1)) >> 3;
}

static sb32 grPupupuWhispyParticleEffectDrawable(LBTransform *xf, s32 particle_link)
{
	if (xf == NULL)
	{
		return FALSE;
	}
	return (lbParticleCountDrawableForGeneratorID(xf->generator_id, particle_link) > 0) ? TRUE : FALSE;
}

static s8 grPupupuWhispyMouthTextureForFlowerStatus(u8 flower_status)
{
	switch (flower_status)
	{
	case nGRPupupuFlowerStatusWindStart:
		return (s8)nGRPupupuWhispyMouthTextureOpen;

	case nGRPupupuFlowerStatusWindLoopStart:
	case nGRPupupuFlowerStatusWindLoop:
		return (s8)nGRPupupuWhispyMouthTextureBlow;

	case nGRPupupuFlowerStatusWindLoopEnd:
	case nGRPupupuFlowerStatusWindStop:
		return (s8)nGRPupupuWhispyMouthTextureClose;

	default:
		return -1;
	}
}

static s8 grPupupuWhispyEyesTextureForFlowerStatus(u8 flower_status)
{
	switch (flower_status)
	{
	case nGRPupupuFlowerStatusWindStart:
		return (s8)nGRPupupuWhispyEyesTexture0;

	case nGRPupupuFlowerStatusWindLoopStart:
	case nGRPupupuFlowerStatusWindLoop:
		return (s8)nGRPupupuWhispyEyesTexture1;

	case nGRPupupuFlowerStatusWindLoopEnd:
	case nGRPupupuFlowerStatusWindStop:
		return (s8)nGRPupupuWhispyEyesTexture2;

	default:
		return -1;
	}
}

void grPupupuWhispyNullParticleXfHandles(void)
{
	gGRCommonStruct.pupupu.leaves_xf = NULL;
	gGRCommonStruct.pupupu.dust_xf = NULL;
	grPupupuWhispyClearDustGeneratorCache();
	sGRPupupuWhispyDustSpawnRetryWait = 0U;
	sGRPupupuWhispyLeavesSpawnRetryWait = 0U;
	sGRPupupuWhispyLeavesDrawableStall = 0U;
	sGRPupupuWhispyDustDrawableStall = 0U;
}

void grPupupuWhispyEjectLeavesEffect(void)
{
	LBTransform *xf = gGRCommonStruct.pupupu.leaves_xf;

	if (xf != NULL)
	{
		if (lbParticleTransformIsAllocated(xf) != FALSE)
		{
			lbParticleEjectStructID(xf->generator_id, grPupupuWhispyParticleAllocLink());
		}
		gGRCommonStruct.pupupu.leaves_xf = NULL;
	}
}

static void grPupupuWhispyClearDustGeneratorCache(void)
{
	sGRPupupuWhispyDustGenCount = 0U;
}

static void grPupupuWhispyRememberDustGeneratorId(u16 generator_id)
{
	u8 i;

	if (generator_id == 0U)
	{
		return;
	}
	for (i = 0; i < sGRPupupuWhispyDustGenCount; i++)
	{
		if (sGRPupupuWhispyDustGenIds[i] == generator_id)
		{
			return;
		}
	}
	if (sGRPupupuWhispyDustGenCount < GRPUPUPU_WHISPY_DUST_GEN_CACHE_MAX)
	{
		sGRPupupuWhispyDustGenIds[sGRPupupuWhispyDustGenCount] = generator_id;
		sGRPupupuWhispyDustGenCount++;
	}
}

static void grPupupuWhispyEjectDustGeneratorOnBothLinks(u16 generator_id)
{
	lbParticleEjectStructID(generator_id, grPupupuWhispyDustParticleAllocLink());
	lbParticleEjectStructID(generator_id, grPupupuWhispyParticleAllocLink());
	lbParticleEjectGeneratorID(generator_id);
}

static void grPupupuWhispyEjectAllRememberedDustGenerators(void)
{
	u8 i;

	for (i = 0; i < sGRPupupuWhispyDustGenCount; i++)
	{
		grPupupuWhispyEjectDustGeneratorOnBothLinks(sGRPupupuWhispyDustGenIds[i]);
	}
	grPupupuWhispyClearDustGeneratorCache();
}

static void grPupupuWhispyEjectDustForWindStop(const char *reason)
{
	u8 i;

	if (gGRCommonStruct.pupupu.dust_xf != NULL)
	{
		grPupupuWhispyRememberDustGeneratorId(gGRCommonStruct.pupupu.dust_xf->generator_id);
	}
	if ((grPupupuWhispyRepairDiagEnabled() != FALSE) && (reason != NULL))
	{
		port_log(
		    "SSB64 WhispyRepair: dust_eject reason=%s gen_count=%u",
		    reason,
		    (u32)sGRPupupuWhispyDustGenCount);
		for (i = 0; i < sGRPupupuWhispyDustGenCount; i++)
		{
			port_log(" gen_id[%u]=%u", (u32)i, (u32)sGRPupupuWhispyDustGenIds[i]);
		}
		port_log("\n");
	}
	grPupupuWhispyEjectAllRememberedDustGenerators();
	gGRCommonStruct.pupupu.dust_xf = NULL;
	sGRPupupuWhispyDustSpawnRetryWait = 0U;
}

void grPupupuWhispyEjectDustEffect(void)
{
	LBTransform *xf = gGRCommonStruct.pupupu.dust_xf;
	s32 dust_link;

#if defined(PORT) && defined(SSB64_NETMENU)
	if (xf == NULL)
	{
		grPupupuWhispyEjectAllRememberedDustGenerators();
		return;
	}
	if (lbParticleTransformIsAllocated(xf) != FALSE)
	{
		grPupupuWhispyRememberDustGeneratorId(xf->generator_id);
		grPupupuWhispyEjectDustGeneratorOnBothLinks(xf->generator_id);
	}
	gGRCommonStruct.pupupu.dust_xf = NULL;
#else
	if (xf == NULL)
	{
		return;
	}
	dust_link = grPupupuWhispyParticleAllocLink();
	lbParticleEjectStructID(xf->generator_id, dust_link);
	gGRCommonStruct.pupupu.dust_xf = NULL;
#endif
}

void grPupupuWhispyRepairPresentationCosmetic(void)
{
	GRCommonGroundVarsPupupu *pu = &gGRCommonStruct.pupupu;
	s8 mouth_tex;
	s8 eyes_tex;

	if (pu->whispy_status != nGRPupupuWhispyWindStatusBlow)
	{
		return;
	}
	/* Hash-safe: derive textures from snapshotted flower status only; never write back to pu. */
	mouth_tex = grPupupuWhispyMouthTextureForFlowerStatus(pu->flowers_back_status);
	eyes_tex = grPupupuWhispyEyesTextureForFlowerStatus(pu->flowers_front_status);
	if ((mouth_tex >= 0) && (pu->map_gobj[2] != NULL))
	{
		gcAddAnimJointAll(
		    pu->map_gobj[2],
		    (AObjEvent32 **)(dGRPupupuWhispyMouthTextures[pu->lr_players][mouth_tex] + (uintptr_t)pu->map_head),
		    0.0F);
		gcPlayAnimAll(pu->map_gobj[2]);
	}
	if ((eyes_tex >= 0) && (pu->map_gobj[3] != NULL))
	{
		gcAddAnimJointAll(
		    pu->map_gobj[3],
		    (AObjEvent32 **)(dGRPupupuWhispyEyesTextures[pu->lr_players][eyes_tex] + (uintptr_t)pu->map_head),
		    0.0F);
		gcPlayAnimAll(pu->map_gobj[3]);
	}
	if (grPupupuWhispyRepairDiagEnabled() != FALSE)
	{
		port_log(
		    "SSB64 WhispyRepair: presentation flowers_f=%u flowers_b=%u mouth_tex_applied=%d eyes_tex_applied=%d\n",
		    (u32)pu->flowers_front_status,
		    (u32)pu->flowers_back_status,
		    (int)mouth_tex,
		    (int)eyes_tex);
	}
}

static void grPupupuWhispyWarmupParticleEffect(LBTransform *xf, s32 particle_link, s32 ticks)
{
	if ((xf != NULL) && (ticks > 0))
	{
		(void)lbParticleWarmupGeneratorID(xf->generator_id, particle_link, ticks);
	}
}

void grPupupuWhispyWarmupLiveEffectsEx(s32 ticks)
{
	GRCommonGroundVarsPupupu *pu = &gGRCommonStruct.pupupu;

	if (pu->whispy_status != nGRPupupuWhispyWindStatusBlow)
	{
		return;
	}
	if (ticks <= 0)
	{
		ticks = 64;
	}
	grPupupuWhispyWarmupParticleEffect(pu->leaves_xf, grPupupuWhispyParticleAllocLink(), ticks);
	grPupupuWhispyWarmupParticleEffect(pu->dust_xf, grPupupuWhispyDustParticleAllocLink(), ticks);
}

void grPupupuWhispyWarmupLiveEffects(void)
{
	grPupupuWhispyWarmupLiveEffectsEx(64);
}

#if defined(PORT) && defined(SSB64_NETMENU)
static void grPupupuWhispyNetplaySpawnLeavesFresh(void)
{
	grPupupuWhispyEjectLeavesEffect();
	grPupupuWhispyLeavesMakeEffect();
	if (gGRCommonStruct.pupupu.leaves_xf != NULL)
	{
		grPupupuWhispyWarmupParticleEffect(
		    gGRCommonStruct.pupupu.leaves_xf, grPupupuWhispyParticleAllocLink(), 64);
	}
}

static void grPupupuWhispyNetplaySpawnDustFresh(void)
{
	grPupupuWhispyEjectDustEffect();
	grPupupuWhispyDustMakeEffect();
	if (gGRCommonStruct.pupupu.dust_xf != NULL)
	{
		grPupupuWhispyWarmupParticleEffect(
		    gGRCommonStruct.pupupu.dust_xf, grPupupuWhispyDustParticleAllocLink(), 64);
	}
}
#endif

static void grPupupuWhispyForwardTextureRefreshIfNeeded(void)
{
	GRCommonGroundVarsPupupu *pu = &gGRCommonStruct.pupupu;
	s8 mouth_tex;
	s8 eyes_tex;

	if (pu->whispy_status != nGRPupupuWhispyWindStatusBlow)
	{
		sGRPupupuWhispyForwardTextureFlowersF = 0xFFU;
		sGRPupupuWhispyForwardTextureFlowersB = 0xFFU;
		return;
	}
	if ((pu->whispy_mouth_texture != -1) || (pu->whispy_eyes_texture != -1))
	{
		return;
	}
	if ((pu->flowers_front_status == sGRPupupuWhispyForwardTextureFlowersF) &&
	    (pu->flowers_back_status == sGRPupupuWhispyForwardTextureFlowersB))
	{
		return;
	}
	mouth_tex = grPupupuWhispyMouthTextureForFlowerStatus(pu->flowers_back_status);
	eyes_tex = grPupupuWhispyEyesTextureForFlowerStatus(pu->flowers_front_status);
	if ((mouth_tex < 0) && (eyes_tex < 0))
	{
		return;
	}
	grPupupuWhispyRepairPresentationCosmetic();
	sGRPupupuWhispyForwardTextureFlowersF = pu->flowers_front_status;
	sGRPupupuWhispyForwardTextureFlowersB = pu->flowers_back_status;
	if (grPupupuWhispyRepairDiagEnabled() != FALSE)
	{
		port_log(
		    "SSB64 WhispyRepair: forward_texture flowers_f=%u flowers_b=%u mouth_derived=%d eyes_derived=%d\n",
		    (u32)pu->flowers_front_status,
		    (u32)pu->flowers_back_status,
		    (int)mouth_tex,
		    (int)eyes_tex);
	}
}
#endif

// 0x80105B18
void grPupupuWhispyLeavesMakeEffect(void)
{
    LBParticle *pc;
    LBTransform *xf;

    xf = NULL;
    pc = lbParticleMakeScriptID(gGRCommonStruct.pupupu.particle_bank_id | LBPARTICLE_MASK_GENLINK(0), 0);

    if (pc != NULL)
    {
        xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf == NULL)
        {
            lbParticleEjectStruct(pc);
        }
        else
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                xf = NULL;
            }
            else
            {
                xf->translate = dGRPupupuWhispyLeavesEffectAttributes[gGRCommonStruct.pupupu.lr_players].pos;
                xf->rotate.y = dGRPupupuWhispyLeavesEffectAttributes[gGRCommonStruct.pupupu.lr_players].rotate;
            }
        }
    }
    gGRCommonStruct.pupupu.leaves_xf = xf;
}

// 0x80105BE8
void grPupupuWhispyUpdateWait(void)
{
    if (gGRCommonStruct.pupupu.whispy_wind_wait != 0)
    {
        gGRCommonStruct.pupupu.whispy_wind_wait--;
    }
    else
    {
        s32 lr = grPupupuWhispyGetLR(gGRCommonStruct.pupupu.map_gobj[1]);

        if (lr == -1) // -1 = Both sides of the map have an equal number of players
        {
            lr = gGRCommonStruct.pupupu.lr_players;
        }
        if (lr != gGRCommonStruct.pupupu.lr_players)
        {
            gGRCommonStruct.pupupu.whispy_eyes_status = nGRPupupuWhispyEyesStatusTurn;
            gGRCommonStruct.pupupu.whispy_mouth_status = nGRPupupuWhispyMouthStatusTurn;
            gGRCommonStruct.pupupu.whispy_status = nGRPupupuWhispyWindStatusTurn;
            gGRCommonStruct.pupupu.lr_players = lr;
        }
        else
        {
            gGRCommonStruct.pupupu.whispy_mouth_status = nGRPupupuWhispyMouthStatusOpen;
            gGRCommonStruct.pupupu.whispy_status = nGRPupupuWhispyWindStatusOpen;
        }
    }
}

// 0x80105C70
void grPupupuWhispyUpdateTurn(void)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only. */
    /* Netplay: snap near-zero map_gobj anim so Turn→Open agrees cross-ISA. */
    if (syNetplayMapGobjAnimFrameEnded(gGRCommonStruct.pupupu.map_gobj[1]) != FALSE)
#else
    if (gGRCommonStruct.pupupu.map_gobj[1]->anim_frame <= 0.0F)
#endif
    {
        gGRCommonStruct.pupupu.whispy_mouth_status = nGRPupupuWhispyMouthStatusOpen;
        gGRCommonStruct.pupupu.whispy_status = nGRPupupuWhispyWindStatusOpen;
    }
}

// 0x80105CAC
void grPupupuWhispyUpdateOpen(void)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only. */
    if (syNetplayMapGobjAnimFrameEnded(gGRCommonStruct.pupupu.map_gobj[1]) != FALSE)
#else
    if (gGRCommonStruct.pupupu.map_gobj[1]->anim_frame <= 0.0F)
#endif
    {
        gGRCommonStruct.pupupu.whispy_status = nGRPupupuWhispyWindStatusBlow;

        gGRCommonStruct.pupupu.flowers_back_status = gGRCommonStruct.pupupu.flowers_front_status = nGRPupupuFlowerStatusWindStart;

        gGRCommonStruct.pupupu.whispy_wind_duration = syUtilsRandIntRange(GRPUPUPU_WHISPY_WIND_DURATION_RANDOM) + GRPUPUPU_WHISPY_WIND_DURATION_BASE;

        gGRCommonStruct.pupupu.rumble_wait = 0;

#if defined(PORT) && defined(SSB64_NETMENU)
        sGRPupupuWhispyDustSpawnRetryWait = 0U;
        sGRPupupuWhispyLeavesSpawnRetryWait = 0U;
        sGRPupupuWhispyLeavesDrawableStall = 0U;
        sGRPupupuWhispyDustDrawableStall = 0U;
        sGRPupupuWhispyBlowDiagTick = 0U;
        grPupupuWhispyClearDustGeneratorCache();
        efParticleGObjClearSkipID(1);
        efParticleGObjClearSkipID(2);
        if (syNetplayRollbackSemanticsActive() != FALSE)
        {
            grPupupuWhispyNetplaySpawnLeavesFresh();
        }
        else
        {
            grPupupuWhispyLeavesMakeEffect();
        }
#else
        grPupupuWhispyLeavesMakeEffect();
#endif

        func_800269C0_275C0(nSYAudioFGMPupupuWhispyWind);
    }
}

// 0x80105D20
void grPupupuWhispyUpdateWindRumble(void)
{
    if (gGRCommonStruct.pupupu.rumble_wait == 0)
    {
        efManagerQuakeMakeEffect(0);

        gGRCommonStruct.pupupu.rumble_wait = GRPUPUPU_WHISPY_WIND_RUMBLE_WAIT;
    }
    gGRCommonStruct.pupupu.rumble_wait--;
}

// 0x80105D6C
void grPupupuWhispyUpdateBlow(void)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    /* Netplay rollback only: snapshot load wipes LBParticles; xf handles can stay non-NULL. */
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        GRCommonGroundVarsPupupu *pu = &gGRCommonStruct.pupupu;
        sb32 needs_leaves =
            grPupupuWhispyXfNeedsRespawnForLink(pu->leaves_xf, grPupupuWhispyParticleAllocLink());
        sb32 needs_dust = FALSE;

        if ((needs_leaves != FALSE) &&
            (grPupupuWhispyParticleEffectDrawable(pu->leaves_xf, grPupupuWhispyParticleAllocLink()) != FALSE))
        {
            needs_leaves = FALSE;
        }
        if ((needs_leaves == FALSE) && (pu->leaves_xf != NULL) &&
            (grPupupuWhispyXfNeedsRespawnForLink(pu->leaves_xf, grPupupuWhispyParticleAllocLink()) != FALSE))
        {
            needs_leaves = TRUE;
        }
        if (pu->flowers_front_status >= (u8)nGRPupupuFlowerStatusWindLoopStart)
        {
            needs_dust =
                grPupupuWhispyXfNeedsRespawnForLink(pu->dust_xf, grPupupuWhispyDustParticleAllocLink());
            if ((needs_dust != FALSE) &&
                (grPupupuWhispyParticleEffectDrawable(pu->dust_xf, grPupupuWhispyDustParticleAllocLink()) != FALSE))
            {
                needs_dust = FALSE;
            }
            if ((needs_dust != FALSE) && (pu->dust_xf != NULL))
            {
                s32 dust_link = grPupupuWhispyDustParticleAllocLink();
                s32 dust_structs =
                    lbParticleCountStructsForGeneratorID(pu->dust_xf->generator_id, dust_link);
                f32 dust_max_size =
                    lbParticleGetMaxDrawSizeForGeneratorID(pu->dust_xf->generator_id, dust_link);

                if ((dust_structs > 0) && (dust_max_size > 0.0F))
                {
                    needs_dust = FALSE;
                }
            }
            if ((needs_dust == FALSE) && (pu->dust_xf != NULL) &&
                (grPupupuWhispyXfNeedsRespawnForLink(pu->dust_xf, grPupupuWhispyDustParticleAllocLink()) != FALSE))
            {
                needs_dust = TRUE;
            }
        }
        if (needs_leaves != FALSE)
        {
            if (sGRPupupuWhispyLeavesSpawnRetryWait > 0U)
            {
                sGRPupupuWhispyLeavesSpawnRetryWait--;
                needs_leaves = FALSE;
            }
            else
            {
                grPupupuWhispyNetplaySpawnLeavesFresh();
                if (pu->leaves_xf == NULL)
                {
                    sGRPupupuWhispyLeavesSpawnRetryWait = 30U;
                    needs_leaves = FALSE;
                }
                else
                {
                    needs_leaves = FALSE;
                }
            }
        }
        if (needs_dust != FALSE)
        {
            if (sGRPupupuWhispyDustSpawnRetryWait > 0U)
            {
                sGRPupupuWhispyDustSpawnRetryWait--;
                needs_dust = FALSE;
            }
            else
            {
                grPupupuWhispyNetplaySpawnDustFresh();
                if (pu->dust_xf == NULL)
                {
                    sGRPupupuWhispyDustSpawnRetryWait = 30U;
                    needs_dust = FALSE;
                }
                else
                {
                    needs_dust = FALSE;
                }
            }
        }
        {
            s32 leaves_link = grPupupuWhispyParticleAllocLink();
            s32 dust_link = grPupupuWhispyDustParticleAllocLink();
            s32 leaves_drawable = 0;
            s32 dust_drawable = 0;

            if (pu->leaves_xf != NULL)
            {
                leaves_drawable =
                    lbParticleCountDrawableForGeneratorID(pu->leaves_xf->generator_id, leaves_link);
                if (leaves_drawable == 0)
                {
                    grPupupuWhispyWarmupParticleEffect(pu->leaves_xf, leaves_link, 32);
                    leaves_drawable =
                        lbParticleCountDrawableForGeneratorID(pu->leaves_xf->generator_id, leaves_link);
                }
            }
            if (pu->dust_xf != NULL)
            {
                dust_drawable =
                    lbParticleCountDrawableForGeneratorID(pu->dust_xf->generator_id, dust_link);
                if (dust_drawable == 0)
                {
                    grPupupuWhispyWarmupParticleEffect(pu->dust_xf, dust_link, 32);
                    dust_drawable =
                        lbParticleCountDrawableForGeneratorID(pu->dust_xf->generator_id, dust_link);
                }
            }
            if (leaves_drawable > 0)
            {
                sGRPupupuWhispyLeavesDrawableStall = 0U;
            }
            else if (pu->leaves_xf != NULL)
            {
                sGRPupupuWhispyLeavesDrawableStall++;
                if (sGRPupupuWhispyLeavesDrawableStall >= GRPUPUPU_WHISPY_DRAWABLE_STALL_RESPAWN_TICKS)
                {
                    grPupupuWhispyNetplaySpawnLeavesFresh();
                    grPupupuWhispyWarmupParticleEffect(pu->leaves_xf, leaves_link, 128);
                    sGRPupupuWhispyLeavesDrawableStall = 0U;
                }
            }
            if (dust_drawable > 0)
            {
                sGRPupupuWhispyDustDrawableStall = 0U;
            }
            else if ((pu->dust_xf != NULL) &&
                     (pu->flowers_front_status >= (u8)nGRPupupuFlowerStatusWindLoopStart))
            {
                sGRPupupuWhispyDustDrawableStall++;
                if (sGRPupupuWhispyDustDrawableStall >= GRPUPUPU_WHISPY_DRAWABLE_STALL_RESPAWN_TICKS)
                {
                    grPupupuWhispyNetplaySpawnDustFresh();
                    grPupupuWhispyWarmupParticleEffect(pu->dust_xf, dust_link, 128);
                    sGRPupupuWhispyDustDrawableStall = 0U;
                }
            }
            sGRPupupuWhispyBlowDiagTick++;
            if ((grPupupuWhispyRepairDiagEnabled() != FALSE) &&
                ((sGRPupupuWhispyBlowDiagTick <= 5U) ||
                 ((sGRPupupuWhispyBlowDiagTick % GRPUPUPU_WHISPY_BLOW_DIAG_INTERVAL) == 0U)))
            {
                s32 leaves_structs = 0;
                s32 dust_structs = 0;
                f32 leaves_max_size = -1.0F;
                f32 dust_max_size = -1.0F;
                u32 struct_skip_flags = 0U;
                u32 gen_skip_flags = 0U;

                if (gEFParticleStructsGObj != NULL)
                {
                    struct_skip_flags = gEFParticleStructsGObj->flags;
                }
                if (gEFParticleGeneratorsGObj != NULL)
                {
                    gen_skip_flags = gEFParticleGeneratorsGObj->flags;
                }
                if (pu->leaves_xf != NULL)
                {
                    leaves_structs =
                        lbParticleCountStructsForGeneratorID(pu->leaves_xf->generator_id, leaves_link);
                    leaves_max_size =
                        lbParticleGetMaxDrawSizeForGeneratorID(pu->leaves_xf->generator_id, leaves_link);
                    leaves_drawable =
                        lbParticleCountDrawableForGeneratorID(pu->leaves_xf->generator_id, leaves_link);
                }
                if (pu->dust_xf != NULL)
                {
                    dust_structs =
                        lbParticleCountStructsForGeneratorID(pu->dust_xf->generator_id, dust_link);
                    dust_max_size =
                        lbParticleGetMaxDrawSizeForGeneratorID(pu->dust_xf->generator_id, dust_link);
                    dust_drawable =
                        lbParticleCountDrawableForGeneratorID(pu->dust_xf->generator_id, dust_link);
                }
                port_log(
                    "SSB64 WhispyRepair: forward_tick status=%u flowers_f=%u needs_leaves=%d needs_dust=%d "
                    "leaves_xf=%p dust_xf=%p leaves_alloc=%d dust_alloc=%d "
                    "leaves_drawable=%d dust_drawable=%d leaves_structs=%d dust_structs=%d "
                    "leaves_max_size=%f dust_max_size=%f leaves_stall=%u dust_stall=%u "
                    "struct_skip=0x%X gen_skip=0x%X\n",
                    (u32)pu->whispy_status,
                    (u32)pu->flowers_front_status,
                    (int)needs_leaves,
                    (int)needs_dust,
                    (void *)pu->leaves_xf,
                    (void *)pu->dust_xf,
                    (int)lbParticleTransformIsAllocated(pu->leaves_xf),
                    (int)lbParticleTransformIsAllocated(pu->dust_xf),
                    leaves_drawable,
                    dust_drawable,
                    leaves_structs,
                    dust_structs,
                    (double)leaves_max_size,
                    (double)dust_max_size,
                    (u32)sGRPupupuWhispyLeavesDrawableStall,
                    (u32)sGRPupupuWhispyDustDrawableStall,
                    (unsigned int)struct_skip_flags,
                    (unsigned int)gen_skip_flags);
                grPupupuWhispyLogEffectDisplayGObjs(leaves_link, dust_link);
            }
        }
        grPupupuWhispyForwardTextureRefreshIfNeeded();
    }
#endif
    gGRCommonStruct.pupupu.whispy_wind_duration--;

    if (gGRCommonStruct.pupupu.whispy_wind_duration == 0)
    {
        gGRCommonStruct.pupupu.whispy_mouth_status = nGRPupupuWhispyMouthStatusClose;

        gGRCommonStruct.pupupu.flowers_back_status = gGRCommonStruct.pupupu.flowers_front_status = nGRPupupuFlowerStatusWindLoopEnd;

        gGRCommonStruct.pupupu.whispy_status = nGRPupupuWhispyWindStatusStop;

#if defined(PORT) && defined(SSB64_NETMENU)
        grPupupuWhispyEjectLeavesEffect();
        grPupupuWhispyEjectDustForWindStop("wind_duration");
        sGRPupupuWhispyLeavesSpawnRetryWait = 0U;
#else
        if (gGRCommonStruct.pupupu.leaves_xf != NULL)
        {
            lbParticleEjectStructID(gGRCommonStruct.pupupu.leaves_xf->generator_id, 1);
        }
#endif
    }
    grPupupuWhispyUpdateWindRumble();
}

// 0x80105DD8
void grPupupuWhispyUpdateStop(void)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only. */
    if (syNetplayMapGobjAnimFrameEnded(gGRCommonStruct.pupupu.map_gobj[1]) != FALSE)
#else
    if (gGRCommonStruct.pupupu.map_gobj[1]->anim_frame <= 0.0F)
#endif
    {
        gGRCommonStruct.pupupu.whispy_wind_wait = syUtilsRandIntRange(GRPUPUPU_WHISPY_WAIT_DURATION_RANDOM) + GRPUPUPU_WHISPY_WAIT_DURATION_BASE;
        gGRCommonStruct.pupupu.whispy_status = nGRPupupuWhispyWindStatusWait;
    }
}

// 0x80105E34
void grPupupuWhispyUpdateBlink(void)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only. */
    /*
     * Blink wait is presentation timing. Under netplay:
     * 1) Post-blink lockout (-9..-1) must keep ticking even if map_gobj[0]
     *    anim_frame leftovers flap above zero — otherwise one ISA freezes at
     *    -9, skips the -10 reseed, and forks the gameplay LCG (soak1 FC@600
     *    diverged=rng, inputs MATCH; Android blink=-9 vs Linux blink=244).
     * 2) Reseed uses ForcedCosmetic so a residual one-tick timing skew cannot
     *    advance gSYMainRandom. Wind wait/duration stay on the game LCG.
     * See docs/bugs/netplay_pupupu_whispy_blink_rng_fc_2026-07-12.md.
     */
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        sb32 eyes_anim_ended;
        s16 blink_wait;
        sb32 in_post_blink_lockout;

        if (gGRCommonStruct.pupupu.whispy_eyes_status != -1)
        {
            return;
        }
        blink_wait = gGRCommonStruct.pupupu.whispy_blink_wait;
        in_post_blink_lockout = ((blink_wait < 0) && (blink_wait > -10)) ? TRUE : FALSE;
        eyes_anim_ended = syNetplayMapGobjAnimFrameEnded(gGRCommonStruct.pupupu.map_gobj[0]);
        if ((in_post_blink_lockout == FALSE) && (eyes_anim_ended == FALSE))
        {
            return;
        }
        gGRCommonStruct.pupupu.whispy_blink_wait--;

        if ((gGRCommonStruct.pupupu.whispy_blink_wait == 0) || (gGRCommonStruct.pupupu.whispy_blink_wait == -10))
        {
            gGRCommonStruct.pupupu.whispy_eyes_status = nGRPupupuWhispyEyesStatusBlink;

            /*
             * Do not Stretch the mouth under netplay. Stretch restarts map_gobj[1] and can
             * desync Open→Blow (gameplay LCG wind_duration) when one ISA thinks the mouth
             * anim has ended and the other does not. Eyes blink alone is enough presentation.
             * See docs/bugs/netplay_pupupu_whispy_open_blow_rng_fc_2026-07-12.md.
             */
            if (gGRCommonStruct.pupupu.whispy_blink_wait != 0)
            {
                gGRCommonStruct.pupupu.whispy_blink_wait =
                    syUtilsRandIntRangeForcedCosmetic(GRPUPUPU_WHISPY_BLINK_WAIT_RANDOM) +
                    GRPUPUPU_WHISPY_BLINK_WAIT_BASE;
            }
        }
        return;
    }
    if ((gGRCommonStruct.pupupu.whispy_eyes_status == -1) &&
        (syNetplayMapGobjAnimFrameEnded(gGRCommonStruct.pupupu.map_gobj[0]) != FALSE))
#else
    if ((gGRCommonStruct.pupupu.whispy_eyes_status == -1) && (gGRCommonStruct.pupupu.map_gobj[0]->anim_frame <= 0.0F))
#endif
    {
        gGRCommonStruct.pupupu.whispy_blink_wait--;

        if ((gGRCommonStruct.pupupu.whispy_blink_wait == 0) || (gGRCommonStruct.pupupu.whispy_blink_wait == -10))
        {
            gGRCommonStruct.pupupu.whispy_eyes_status = nGRPupupuWhispyEyesStatusBlink;

#if defined(PORT) && defined(SSB64_NETMENU)
            if ((syNetplayMapGobjAnimFrameEnded(gGRCommonStruct.pupupu.map_gobj[1]) != FALSE) &&
                (gGRCommonStruct.pupupu.whispy_status != nGRPupupuWhispyWindStatusBlow))
#else
            if ((gGRCommonStruct.pupupu.map_gobj[1]->anim_frame <= 0.0F) && (gGRCommonStruct.pupupu.whispy_status != nGRPupupuWhispyWindStatusBlow))
#endif
            {
                gGRCommonStruct.pupupu.whispy_mouth_status = nGRPupupuWhispyMouthStatusStretch;
            }
            if (gGRCommonStruct.pupupu.whispy_blink_wait != 0)
            {
                gGRCommonStruct.pupupu.whispy_blink_wait = syUtilsRandIntRange(GRPUPUPU_WHISPY_BLINK_WAIT_RANDOM) + GRPUPUPU_WHISPY_BLINK_WAIT_BASE;
            }
        }
    }
}

// 0x80105EF4
void grPupupuUpdateWhispyStatus(void)
{
    switch (gGRCommonStruct.pupupu.whispy_status)
    {
    case nGRPupupuWhispyWindStatusSleep:
        grPupupuWhispyUpdateSleep();
        break;

    case nGRPupupuWhispyWindStatusWait:
        grPupupuWhispyUpdateWait();
        break;

    case nGRPupupuWhispyWindStatusTurn:
        grPupupuWhispyUpdateTurn();
        break;

    case nGRPupupuWhispyWindStatusOpen:
        grPupupuWhispyUpdateOpen();
        break;

    case nGRPupupuWhispyWindStatusBlow:
        grPupupuWhispyUpdateBlow();
        break;

    case nGRPupupuWhispyWindStatusStop:
        grPupupuWhispyUpdateStop();
        break;
    }
    grPupupuWhispyUpdateBlink();
}

// 0x80105F94
void grPupupuFlowersBackWindStart(void)
{
    gGRCommonStruct.pupupu.flowers_back_wait--;

    if (gGRCommonStruct.pupupu.flowers_back_wait == 0)
    {
        gGRCommonStruct.pupupu.whispy_mouth_texture = 0;
        gGRCommonStruct.pupupu.flowers_back_status = nGRPupupuFlowerStatusWindLoopStart;
    }
}

// 0x80105FC4
void grPupupuFlowersBackLoopStart(void)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only. */
    if (syNetplayMapGobjAnimFrameEnded(gGRCommonStruct.pupupu.map_gobj[2]) != FALSE)
#else
    if (gGRCommonStruct.pupupu.map_gobj[2]->anim_frame <= 0.0F)
#endif
    {
        gGRCommonStruct.pupupu.whispy_mouth_texture = 1;
        gGRCommonStruct.pupupu.flowers_back_status = nGRPupupuFlowerStatusWindLoop;
        gGRCommonStruct.pupupu.flowers_back_wait = 15;
    }
}

// 0x80106008
void grPupupuFlowersBackLoopEnd(void)
{
    gGRCommonStruct.pupupu.flowers_back_wait--;

    if (gGRCommonStruct.pupupu.flowers_back_wait == 0)
    {
        gGRCommonStruct.pupupu.whispy_mouth_texture = 2;
        gGRCommonStruct.pupupu.flowers_back_status = nGRPupupuFlowerStatusWindStop;
        gGRCommonStruct.pupupu.flowers_back_wait = 15;
    }
}

// 0x80106044
void grPupupuFlowersBackUpdateAll(void)
{
    switch (gGRCommonStruct.pupupu.flowers_back_status)
    {
    case nGRPupupuFlowerStatusWindStart:
        grPupupuFlowersBackWindStart();
        break;

    case nGRPupupuFlowerStatusWindLoopStart:
        grPupupuFlowersBackLoopStart();
        break;

    case nGRPupupuFlowerStatusWindLoopEnd:
        grPupupuFlowersBackLoopEnd();
        break;
    }
}

// 0x801060B0
void grPupupuFlowersFrontWindStart(void)
{
    gGRCommonStruct.pupupu.flowers_front_wait--;

    if (gGRCommonStruct.pupupu.flowers_front_wait == 0)
    {
        gGRCommonStruct.pupupu.whispy_eyes_texture = 0;
        gGRCommonStruct.pupupu.flowers_front_status = nGRPupupuFlowerStatusWindLoopStart;
    }
}

// 0x801060E0
void grPupupuWhispyDustMakeEffect(void)
{
    LBParticle *pc;
    LBTransform *xf;
    s32 dust_genlink;

    xf = NULL;
#if defined(PORT) && defined(SSB64_NETMENU)
    dust_genlink = LBPARTICLE_MASK_GENLINK(1);
#else
    dust_genlink = LBPARTICLE_MASK_GENLINK(0);
#endif
    pc = lbParticleMakeScriptID(gGRCommonStruct.pupupu.particle_bank_id | dust_genlink, 1);

    if (pc != NULL)
    {
        xf = lbParticleAddTransformForStruct(pc, nLBTransformStatusReady);

        if (xf == NULL)
        {
            lbParticleEjectStruct(pc);
        }
        else
        {
            LBParticleProcessStruct(pc);

            if (xf->users_num == 0)
            {
                xf = NULL;
            }
            else
            {
                xf->translate = dGRPupupuWhispyDustEffectPositions[gGRCommonStruct.pupupu.lr_players];

                xf->rotate.y = (gGRCommonStruct.pupupu.lr_players == 1) ? 0.0F : F_CST_DTOR32(180.0F);
            }
        }
    }
    gGRCommonStruct.pupupu.dust_xf = xf;
#if defined(PORT) && defined(SSB64_NETMENU)
    if ((xf != NULL) && (pc != NULL))
    {
        grPupupuWhispyRememberDustGeneratorId(pc->generator_id);
    }
#endif
}

// 0x801061CC
void grPupupuFlowersFrontLoopStart(void)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only. */
    if (syNetplayMapGobjAnimFrameEnded(gGRCommonStruct.pupupu.map_gobj[3]) != FALSE)
#else
    if (gGRCommonStruct.pupupu.map_gobj[3]->anim_frame <= 0.0F)
#endif
    {
        gGRCommonStruct.pupupu.whispy_eyes_texture = 1;
        gGRCommonStruct.pupupu.flowers_front_status = nGRPupupuFlowerStatusWindLoop;
        gGRCommonStruct.pupupu.flowers_front_wait = 22;

#if defined(PORT) && defined(SSB64_NETMENU)
        if (syNetplayRollbackSemanticsActive() != FALSE)
        {
            grPupupuWhispyNetplaySpawnDustFresh();
        }
        else
#endif
        {
            grPupupuWhispyDustMakeEffect();
        }
    }
}

// 0x80106220
void grPupupuFlowersFrontLoopEnd(void)
{
    gGRCommonStruct.pupupu.flowers_front_wait--;

    if (gGRCommonStruct.pupupu.flowers_front_wait == 0)
    {
        gGRCommonStruct.pupupu.whispy_eyes_texture = 2;
        gGRCommonStruct.pupupu.flowers_front_status = nGRPupupuFlowerStatusWindStop;
        gGRCommonStruct.pupupu.flowers_front_wait = 22;

#if defined(PORT) && defined(SSB64_NETMENU)
        grPupupuWhispyEjectDustForWindStop("loop_end");
#else
        if (gGRCommonStruct.pupupu.dust_xf != NULL)
        {
            lbParticleEjectStructID(gGRCommonStruct.pupupu.dust_xf->generator_id, 1);
        }
#endif
    }
    else grPupupuWhispySetWindPush();
}

// 0x80106290
void grPupupuFlowersFrontUpdateAll(void)
{
    switch (gGRCommonStruct.pupupu.flowers_front_status)
    {
    case nGRPupupuFlowerStatusWindStart:
        grPupupuFlowersFrontWindStart();
        break;

    case nGRPupupuFlowerStatusWindLoopStart:
        grPupupuFlowersFrontLoopStart();
        break;

    case nGRPupupuFlowerStatusWindLoop:
        grPupupuWhispySetWindPush();
        break;

    case nGRPupupuFlowerStatusWindLoopEnd:
        grPupupuFlowersFrontLoopEnd();
        break;
    }
}

// 0x80106314
void grPupupuUpdateGObjAnims(void)
{
    if (gGRCommonStruct.pupupu.whispy_eyes_status != -1)
    {
        intptr_t offset = dGRPupupuWhispyEyesAnims[gGRCommonStruct.pupupu.lr_players][gGRCommonStruct.pupupu.whispy_eyes_status][1];

        gcAddAnimAll
        (
            gGRCommonStruct.pupupu.map_gobj[0],
            (AObjEvent32**)
            (dGRPupupuWhispyEyesAnims[gGRCommonStruct.pupupu.lr_players][gGRCommonStruct.pupupu.whispy_eyes_status][0] + (uintptr_t)gGRCommonStruct.pupupu.map_head),
            (AObjEvent32***)
            ((offset != 0) ? (void*) ((uintptr_t)gGRCommonStruct.pupupu.map_head + offset) : NULL),
            0.0F
        );
        gcPlayAnimAll(gGRCommonStruct.pupupu.map_gobj[0]);

        gGRCommonStruct.pupupu.whispy_eyes_status = -1;
    }
    if (gGRCommonStruct.pupupu.whispy_mouth_status != -1)
    {
        gcAddAnimAll
        (
            gGRCommonStruct.pupupu.map_gobj[1],
            (AObjEvent32**)
            (dGRPupupuWhispyMouthAnims[gGRCommonStruct.pupupu.lr_players][gGRCommonStruct.pupupu.whispy_mouth_status][0] + (uintptr_t)gGRCommonStruct.pupupu.map_head),
            (AObjEvent32***)
            (dGRPupupuWhispyMouthAnims[gGRCommonStruct.pupupu.lr_players][gGRCommonStruct.pupupu.whispy_mouth_status][1] + (uintptr_t)gGRCommonStruct.pupupu.map_head),
            0.0F
        );
        gcPlayAnimAll(gGRCommonStruct.pupupu.map_gobj[1]);

        gGRCommonStruct.pupupu.whispy_mouth_status = -1;
    }
    if (gGRCommonStruct.pupupu.whispy_mouth_texture != -1)
    {
        gcAddAnimJointAll
        (
            gGRCommonStruct.pupupu.map_gobj[2],
            (AObjEvent32**)
            (dGRPupupuWhispyMouthTextures[gGRCommonStruct.pupupu.lr_players][gGRCommonStruct.pupupu.whispy_mouth_texture] + (uintptr_t)gGRCommonStruct.pupupu.map_head),
            0.0F
        );
        gcPlayAnimAll(gGRCommonStruct.pupupu.map_gobj[2]);

        gGRCommonStruct.pupupu.whispy_mouth_texture = -1;
    }
    if (gGRCommonStruct.pupupu.whispy_eyes_texture != -1)
    {
        gcAddAnimJointAll
        (
            gGRCommonStruct.pupupu.map_gobj[3],
            (AObjEvent32**)
            (dGRPupupuWhispyEyesTextures[gGRCommonStruct.pupupu.lr_players][gGRCommonStruct.pupupu.whispy_eyes_texture] + (uintptr_t)gGRCommonStruct.pupupu.map_head),
            0.0F
        );
        gcPlayAnimAll(gGRCommonStruct.pupupu.map_gobj[3]);

        gGRCommonStruct.pupupu.whispy_eyes_texture = -1;
    }
}

// 0x80106490
void grPupupuProcUpdate(GObj *ground_gobj)
{
    grPupupuUpdateWhispyStatus();
    grPupupuFlowersBackUpdateAll();
    grPupupuFlowersFrontUpdateAll();
    grPupupuUpdateGObjAnims();
}

// 0x801064C8
GObj* grPupupuMakeMapGObj(intptr_t o_dobjdesc, intptr_t o_mobjsub, void (*proc_display)(GObj*), u8 dl_link)
{
    GObj *ground_gobj = gcMakeGObjSPAfter(nGCCommonKindGround, NULL, nGCCommonLinkIDGround, GOBJ_PRIORITY_DEFAULT);

    gcAddGObjDisplay(ground_gobj, proc_display, dl_link, GOBJ_PRIORITY_DEFAULT, ~0);

    gcSetupCustomDObjs
    (
        ground_gobj,
        (DObjDesc*)
        ((uintptr_t)gGRCommonStruct.pupupu.map_head + o_dobjdesc),
        NULL,
        nGCMatrixKindTraRotRpyRSca,
        nGCMatrixKindNull,
        nGCMatrixKindNull
    );
    if (o_mobjsub != 0)
    {
        gcAddMObjAll(ground_gobj, lbRelocGetFileData(MObjSub***, gGRCommonStruct.pupupu.map_head, o_mobjsub));
    }
    gcAddGObjProcess(ground_gobj, gcPlayAnimAll, nGCProcessKindFunc, 5);

    return ground_gobj;
}

// 0x8010658C
void grPupupuInitAll(void)
{
#ifdef PORT
    gGRCommonStruct.pupupu.map_head = (void*) ((uintptr_t)PORT_RESOLVE(gMPCollisionGroundData->map_nodes) - (intptr_t)llGRPupupuMapMapHead);
    gGRCommonStruct.pupupu.map_gobj[0] = grPupupuMakeMapGObj(llGRPupupuMapMapHead, llGRPupupuMapWhispyEyesTransformKindsMObjSub, grDisplayLayer0PriProcDisplay, 4);
    gGRCommonStruct.pupupu.map_gobj[1] = grPupupuMakeMapGObj(llGRPupupuMapWhispyMouthTransformKindsDObjDesc, llGRPupupuMapWhispyMouthTransformKindsMObjSub, grDisplayLayer0PriProcDisplay, 4);
    gGRCommonStruct.pupupu.map_gobj[2] = grPupupuMakeMapGObj(llGRPupupuMapFlowersBackTransformKindsDObjDesc, 0x0, grDisplayLayer0PriProcDisplay, 4);
    gGRCommonStruct.pupupu.map_gobj[3] = grPupupuMakeMapGObj(llGRPupupuMapFlowersFrontTransformKindsDObjDesc, 0x0, grDisplayLayer3PriProcDisplay, 16);
#else
    gGRCommonStruct.pupupu.map_head = (void*) ((uintptr_t)gMPCollisionGroundData->map_nodes - (intptr_t)&llGRPupupuMapMapHead);
    gGRCommonStruct.pupupu.map_gobj[0] = grPupupuMakeMapGObj(&llGRPupupuMapMapHead, &llGRPupupuMapWhispyEyesTransformKindsMObjSub, grDisplayLayer0PriProcDisplay, 4);
    gGRCommonStruct.pupupu.map_gobj[1] = grPupupuMakeMapGObj(&llGRPupupuMapWhispyMouthTransformKindsDObjDesc, &llGRPupupuMapWhispyMouthTransformKindsMObjSub, grDisplayLayer0PriProcDisplay, 4);
    gGRCommonStruct.pupupu.map_gobj[2] = grPupupuMakeMapGObj(&llGRPupupuMapFlowersBackTransformKindsDObjDesc, 0x0, grDisplayLayer0PriProcDisplay, 4);
    gGRCommonStruct.pupupu.map_gobj[3] = grPupupuMakeMapGObj(&llGRPupupuMapFlowersFrontTransformKindsDObjDesc, 0x0, grDisplayLayer3PriProcDisplay, 16);
#endif

    gGRCommonStruct.pupupu.whispy_eyes_status   =
    gGRCommonStruct.pupupu.whispy_mouth_status  =
    gGRCommonStruct.pupupu.whispy_mouth_texture =
    gGRCommonStruct.pupupu.whispy_eyes_texture  = -1;

    gGRCommonStruct.pupupu.whispy_status        = 0;

    gGRCommonStruct.pupupu.lr_players           = 1;

    gGRCommonStruct.pupupu.whispy_wind_wait     = syUtilsRandIntRange(GRPUPUPU_WHISPY_WAIT_DURATION_RANDOM) + GRPUPUPU_WHISPY_WAIT_DURATION_BASE;
#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: blink wait is presentation-only; keep off gameplay LCG. */
    gGRCommonStruct.pupupu.whispy_blink_wait    = syUtilsRandIntRangeForcedCosmetic(GRPUPUPU_WHISPY_BLINK_WAIT_RANDOM) + GRPUPUPU_WHISPY_BLINK_WAIT_BASE;
#else
    gGRCommonStruct.pupupu.whispy_blink_wait    = syUtilsRandIntRange(GRPUPUPU_WHISPY_BLINK_WAIT_RANDOM)    + GRPUPUPU_WHISPY_BLINK_WAIT_BASE;
#endif

    gGRCommonStruct.pupupu.flowers_back_status  =
    gGRCommonStruct.pupupu.flowers_front_status = 0;

    gGRCommonStruct.pupupu.flowers_back_wait    = 15;
    gGRCommonStruct.pupupu.flowers_front_wait   = 22;

#ifdef PORT
    gGRCommonStruct.pupupu.particle_bank_id = efParticleGetLoadBankID((uintptr_t)&lGRPupupuParticleScriptBankLo, (uintptr_t)&lGRPupupuParticleScriptBankHi, (uintptr_t)&lGRPupupuParticleTextureBankLo, (uintptr_t)&lGRPupupuParticleTextureBankHi);
#else
    gGRCommonStruct.pupupu.particle_bank_id = efParticleGetLoadBankID(&lGRPupupuParticleScriptBankLo, &lGRPupupuParticleScriptBankHi, &lGRPupupuParticleTextureBankLo, &lGRPupupuParticleTextureBankHi);
#endif
}

// 0x801066D4
GObj* grPupupuMakeGround(void)
{
    GObj *ground_gobj = gcMakeGObjSPAfter(nGCCommonKindGround, NULL, nGCCommonLinkIDGround, GOBJ_PRIORITY_DEFAULT);

    gcAddGObjProcess(ground_gobj, grPupupuProcUpdate, nGCProcessKindFunc, 4);
    grPupupuInitAll();

    return ground_gobj;
}
