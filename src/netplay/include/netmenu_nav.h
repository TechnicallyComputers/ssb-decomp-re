#ifndef NETMENU_NAV_H
#define NETMENU_NAV_H

#include <sc/scene.h>

/*
 * Resolve the VS menu scene to return to when backing out of shared downstream
 * screens (CSS, VS Options). Offline classic tier returns to itself; everything
 * else returns to the netmenu VS hub (nSCKindVSMode / mnvsmodenet).
 */
static inline u8 syNetmenuVsMenuReturnScene(void)
{
	if (gSCManagerSceneData.scene_prev == nSCKindVSOfflineClassic)
	{
		return nSCKindVSOfflineClassic;
	}
	return nSCKindVSMode;
}

#endif /* NETMENU_NAV_H */
