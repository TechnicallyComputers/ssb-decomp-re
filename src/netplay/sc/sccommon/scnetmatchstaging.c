#include "common.h"

/*
 * Post-CSS staging: mm_matchmaking poll + syNetPeer rendezvous before VS battle.
 * Linked only when SSB64_NETMENU=ON (offline builds use decomp VS flow without this scene).
 */
#if defined(PORT) && defined(SSB64_NETMENU)

#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/netpeer.h>
#include <sys/taskman.h>
#include <sys/video.h>
#include <sys/audio.h>
#include <sys/controller.h>
#include <ef/efmanager.h>
#include <ef/efparticle.h>

extern void mnVSNetAutomatchFuncLights(Gfx **dls);
extern void mnVSNetAutomatchLoadWallpaperRelocFiles(void);
extern void mnVSNetAutomatchMakeWallpaper(void);
extern void mnVSNetAutomatchMakeWallpaperCamera(void);
extern void mnVSNetAutomatchMakeCenteredString(GObj *gobj, const char *str, f32 center_x, f32 y, u32 *colors);

extern void mnVSNetAutomatchAMStartSearch(void);
extern void mnVSNetAutomatchMatchmakingTick(void);
extern void mnVSNetAutomatchAMFinalizeVsLoad(void);
extern sb32 mnVSNetAutomatchAMConsumeStagingHandshake(void);
extern sb32 mnVSNetAutomatchAMIsError(void);
extern void mnVSNetAutomatchAMStagingReturnToAutomatch(void);
extern void mnVSNetAutomatchAMAbortToCharacterSelect(const char *reason);
extern sb32 mnVSNetAutomatchAMCanUserCancel(void);
extern sb32 mnVSNetAutomatchAMPollUserCancel(void);
extern const char *mnVSNetAutomatchAMStatusText(void);

#define MN_VS_NET_MATCH_STAGING_STATUS_Y 110.0F
#define MN_VS_NET_MATCH_STAGING_CANCEL_Y 130.0F
#define MN_VS_NET_MATCH_STAGING_CENTER_X 160.0F

static GObj *sMNVSNetMatchStagingStatusGObj;
static GObj *sMNVSNetMatchStagingCancelGObj;
static const char *sMNVSNetMatchStagingStatusShown;
static sb32 sMNVSNetMatchStagingCancelShown;

static u32 sMNVSNetMatchStagingStatusColors[3] = { 0xFF, 0xFF, 0xFF };
static u32 sMNVSNetMatchStagingCancelColors[3] = { 0xC0, 0xC0, 0xC0 };

static GObj *mnVSNetMatchStagingMakeLabel(const char *str, f32 y, u32 *colors)
{
	GObj *gobj;

	gobj = gcMakeGObjSPAfter(0, NULL, 17, GOBJ_PRIORITY_DEFAULT);
	gcAddGObjDisplay(gobj, lbCommonDrawSObjAttr, 26, GOBJ_PRIORITY_DEFAULT, ~0);
	mnVSNetAutomatchMakeCenteredString(gobj, str, MN_VS_NET_MATCH_STAGING_CENTER_X, y, colors);
	return gobj;
}

static void mnVSNetMatchStagingUpdateStatusText(void)
{
	const char *status;
	sb32 can_cancel;

	status = mnVSNetAutomatchAMStatusText();
	can_cancel = mnVSNetAutomatchAMCanUserCancel();

	if ((sMNVSNetMatchStagingStatusGObj == NULL) || (status != sMNVSNetMatchStagingStatusShown))
	{
		if (sMNVSNetMatchStagingStatusGObj != NULL)
		{
			gcEjectGObj(sMNVSNetMatchStagingStatusGObj);
			sMNVSNetMatchStagingStatusGObj = NULL;
		}
		sMNVSNetMatchStagingStatusGObj =
		    mnVSNetMatchStagingMakeLabel(status, MN_VS_NET_MATCH_STAGING_STATUS_Y, sMNVSNetMatchStagingStatusColors);
		sMNVSNetMatchStagingStatusShown = status;
	}

	if (can_cancel != FALSE)
	{
		if ((sMNVSNetMatchStagingCancelGObj == NULL) || (sMNVSNetMatchStagingCancelShown == FALSE))
		{
			if (sMNVSNetMatchStagingCancelGObj != NULL)
			{
				gcEjectGObj(sMNVSNetMatchStagingCancelGObj);
				sMNVSNetMatchStagingCancelGObj = NULL;
			}
			sMNVSNetMatchStagingCancelGObj = mnVSNetMatchStagingMakeLabel(
			    "PRESS B TO CANCEL", MN_VS_NET_MATCH_STAGING_CANCEL_Y, sMNVSNetMatchStagingCancelColors);
			sMNVSNetMatchStagingCancelShown = TRUE;
		}
	}
	else if (sMNVSNetMatchStagingCancelGObj != NULL)
	{
		gcEjectGObj(sMNVSNetMatchStagingCancelGObj);
		sMNVSNetMatchStagingCancelGObj = NULL;
		sMNVSNetMatchStagingCancelShown = FALSE;
	}
}

static void mnVSNetMatchStagingFuncRun(GObj *gobj)
{
	(void)gobj;

	mnVSNetMatchStagingUpdateStatusText();

	if (mnVSNetAutomatchAMPollUserCancel() != FALSE)
	{
		mnVSNetAutomatchAMAbortToCharacterSelect("cancelled");
		return;
	}

	mnVSNetAutomatchMatchmakingTick();

	if (mnVSNetAutomatchAMIsError() != FALSE)
	{
		mnVSNetAutomatchAMStagingReturnToAutomatch();
		return;
	}

	if (mnVSNetAutomatchAMConsumeStagingHandshake() != FALSE)
	{
		/* Stage rendezvous now gates visible VS transition to a synchronized go moment. */
		mnVSNetAutomatchAMFinalizeVsLoad();
		return;
	}

	mnVSNetMatchStagingUpdateStatusText();
}

static void mnVSNetMatchStagingFuncStart(void)
{
	sMNVSNetMatchStagingStatusGObj = NULL;
	sMNVSNetMatchStagingCancelGObj = NULL;
	sMNVSNetMatchStagingStatusShown = NULL;
	sMNVSNetMatchStagingCancelShown = FALSE;

	mnVSNetAutomatchLoadWallpaperRelocFiles();

	gcMakeGObjSPAfter(nGCCommonKindPlayerSelect, mnVSNetMatchStagingFuncRun, 15, GOBJ_PRIORITY_DEFAULT);
	gcMakeDefaultCameraGObj(16, GOBJ_PRIORITY_DEFAULT, 100, COBJ_FLAG_ZBUFFER, GPACK_RGBA8888(0x00, 0x00, 0x00, 0x00));
	efParticleInitAll();
	efManagerInitEffects();
	mnVSNetAutomatchMakeWallpaperCamera();
	mnVSNetAutomatchMakeWallpaper();
	scSubsysFighterSetLightParams(45.0F, 45.0F, 0xFF, 0xFF, 0xFF, 0xFF);

	mnVSNetAutomatchAMStartSearch();
	mnVSNetMatchStagingUpdateStatusText();

	if (gSCManagerSceneData.scene_prev != nSCKindMaps)
	{
		syAudioPlayBGM(0, nSYAudioBGMBattleSelect);
	}
}

SYVideoSetup dMNVSNetMatchStagingVideoSetup = SYVIDEO_SETUP_DEFAULT();

SYTaskmanSetup dMNVSNetMatchStagingTaskmanSetup =
{
    {
        0,
        gcRunAll,
        gcDrawAll,
        &ovl27_BSS_END,
        0,
        1,
        2,
        sizeof(Gfx) * 2375,
        sizeof(Gfx) * 64,
        0,
        0,
        0x8000,
        2,
        0x8000,
        mnVSNetAutomatchFuncLights,
        syControllerFuncRead,
    },

    0,
    sizeof(u64) * 32,
    0,
    0,
    0,
    0,
    sizeof(GObj),
    0,
    dLBCommonFuncMatrixList,
    NULL,
    0,
    0,
    0,
    sizeof(DObj),
    0,
    sizeof(SObj),
    0,
    sizeof(CObj),

    mnVSNetMatchStagingFuncStart
};

void mnVSNetMatchStagingStartScene(void)
{
	dMNVSNetMatchStagingVideoSetup.zbuffer = SYVIDEO_ZBUFFER_START(320, 240, 0, 10, u16);
	syVideoInit(&dMNVSNetMatchStagingVideoSetup);

	dMNVSNetMatchStagingTaskmanSetup.scene_setup.arena_size = (size_t)((uintptr_t)&ovl1_VRAM - (uintptr_t)&ovl27_BSS_END);
	syTaskmanStartTask(&dMNVSNetMatchStagingTaskmanSetup);
}

#endif /* PORT && SSB64_NETMENU */
