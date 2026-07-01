#include <gm/gmsound.h>
#include <sc/scene.h>
#include <lb/library.h>
#include <sys/video.h>
#include <sys/controller.h>
#include <sys/audio.h>
#include <sys/rdp.h>
#include <sys/netreplay.h>
#include <sys/netpeer.h>
#include <sys/netinput.h>
#include <sys/taskman.h>

#include "mnvsreplay_ui.h"

extern void *func_800269C0_275C0(u16 id);
extern void func_800266A0_272A0(void);

#if defined(PORT) && defined(SSB64_NETMENU)

static void mnVSReplayPlaybackRenderOverlay(GObj *gobj);
static void mnVSReplayPlaybackExitToMenu(void);

void *sMNVSReplayPlaybackFiles[MN_VSREPLAY_UI_FILE_COUNT];
GObj *sMNVSReplayPlaybackOverlayGObj;
MNVSReplayUiTwoOptionMenu sMNVSReplayPlaybackHaltMenu;
sb32 sMNVSReplayPlaybackFilesLoaded;

static void mnVSReplayPlaybackEjectOverlay(void)
{
	if (sMNVSReplayPlaybackOverlayGObj != NULL)
	{
		gcEjectGObj(sMNVSReplayPlaybackOverlayGObj);
		sMNVSReplayPlaybackOverlayGObj = NULL;
	}
}

static void mnVSReplayPlaybackMakeOverlay(void)
{
	GObj *gobj;

	mnVSReplayPlaybackEjectOverlay();
	sMNVSReplayPlaybackOverlayGObj = gobj = gcMakeGObjSPAfter(0, NULL, 28, GOBJ_PRIORITY_DEFAULT);
	gcAddGObjDisplay(gobj, mnVSReplayPlaybackRenderOverlay, MN_VSREPLAY_UI_OVERLAY_DL_LINK, GOBJ_PRIORITY_DEFAULT,
	                 ~0);
	mnVSReplayUiMakeTwoOptionText(gobj, &sMNVSReplayPlaybackHaltMenu, sMNVSReplayPlaybackFiles);
}

static void mnVSReplayPlaybackRenderOverlay(GObj *gobj)
{
	mnVSReplayUiDrawTwoOptionPanel(gobj, &sMNVSReplayPlaybackHaltMenu, sMNVSReplayPlaybackFiles);
}

static void mnVSReplayPlaybackExitToMenu(void)
{
	mnVSReplayPlaybackEjectOverlay();
	syNetReplayAbortUserPlayback();
	if (syNetPeerIsVSSessionActive() != FALSE)
	{
		syNetPeerEndVSSessionLocally();
	}
	gSCManagerSceneData.scene_prev = gSCManagerSceneData.scene_curr;
	gSCManagerSceneData.scene_curr = nSCKindVSReplays;
	func_800266A0_272A0();
	syAudioStopBGMAll();
	syTaskmanSetLoadScene();
}

void mnVSReplayPlaybackInit(void)
{
	s32 file_index;

	if (gSCManagerSceneData.is_vs_replay_playback == FALSE)
	{
		return;
	}
	sMNVSReplayPlaybackOverlayGObj = NULL;
	sMNVSReplayPlaybackFilesLoaded = FALSE;
	syNetReplaySetUserPlaybackHalted(FALSE);
	for (file_index = 0; file_index < MN_VSREPLAY_UI_FILE_COUNT; file_index++)
	{
		sMNVSReplayPlaybackFiles[file_index] = NULL;
	}
	mnVSReplayUiInitTwoOptionMenu(&sMNVSReplayPlaybackHaltMenu, "RESUME PLAYBACK", "EXIT TO MENU", 1.0F);
	mnVSReplayUiSetTwoOptionPanel(&sMNVSReplayPlaybackHaltMenu, 70.0F, 80.0F, 250.0F, 160.0F, 82.0F, 102.0F, 132.0F);
	mnVSReplayUiLoadFilesInto(sMNVSReplayPlaybackFiles);
	sMNVSReplayPlaybackFilesLoaded = TRUE;
}

void mnVSReplayPlaybackUpdateHalted(void)
{
	s32 choice;
	s32 update_result;

	if ((gSCManagerSceneData.is_vs_replay_playback == FALSE) || (syNetReplayIsPlaybackLoaded() == FALSE))
	{
		return;
	}
	if (sMNVSReplayPlaybackFilesLoaded == FALSE)
	{
		return;
	}
	if (syNetReplayIsUserPlaybackHalted() == FALSE)
	{
		if (syNetInputGetPortHardwareTapButtons(START_BUTTON) != FALSE)
		{
			syNetReplaySetUserPlaybackHalted(TRUE);
			sMNVSReplayPlaybackHaltMenu.active = TRUE;
			sMNVSReplayPlaybackHaltMenu.choice = 0;
			mnVSReplayPlaybackMakeOverlay();
			func_800269C0_275C0(nSYAudioFGMMenuSelect);
		}
		return;
	}
	/* Sim tick is frozen while halted; FuncRead skips HID — refresh latch once per display frame for overlay UI. */
	syNetInputRefreshPortHardwareUiLatch();
	if (sMNVSReplayPlaybackOverlayGObj == NULL)
	{
		mnVSReplayPlaybackMakeOverlay();
	}
	if (mnVSReplayUiUpdateTwoOptionCancelHardware(&sMNVSReplayPlaybackHaltMenu) != FALSE)
	{
		syNetReplaySetUserPlaybackHalted(FALSE);
		mnVSReplayPlaybackEjectOverlay();
		sMNVSReplayPlaybackHaltMenu.active = TRUE;
		sMNVSReplayPlaybackHaltMenu.choice = 0;
		func_800269C0_275C0(nSYAudioFGMMenuScroll2);
		return;
	}
	update_result = mnVSReplayUiUpdateTwoOptionMenuHardware(&sMNVSReplayPlaybackHaltMenu, &choice);
	if (update_result == 2)
	{
		mnVSReplayPlaybackMakeOverlay();
		func_800269C0_275C0(nSYAudioFGMMenuScroll2);
		return;
	}
	if (update_result == FALSE)
	{
		return;
	}
	if (choice == 0)
	{
		syNetReplaySetUserPlaybackHalted(FALSE);
		mnVSReplayPlaybackEjectOverlay();
		sMNVSReplayPlaybackHaltMenu.active = TRUE;
		sMNVSReplayPlaybackHaltMenu.choice = 0;
		func_800269C0_275C0(nSYAudioFGMMenuSelect);
	}
	else
	{
		func_800269C0_275C0(nSYAudioFGMMenuSelect);
		mnVSReplayPlaybackExitToMenu();
	}
}

#endif /* PORT && SSB64_NETMENU */
