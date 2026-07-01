#ifndef _MNVSREPLAY_UI_H_
#define _MNVSREPLAY_UI_H_

#include <PR/ultratypes.h>
#include <lb/library.h>
#include <sys/obj.h>

#define MN_VSREPLAY_UI_FILE_COUNT 2
#define MN_VSREPLAY_UI_OVERLAY_DL_LINK 24

typedef struct MNVSReplayUiTwoOptionMenu
{
	sb32 active;
	s32 choice;
	s32 change_wait;
	const char *option0;
	const char *option1;
	f32 panel_x0;
	f32 panel_y0;
	f32 panel_x1;
	f32 panel_y1;
	f32 option0_y;
	f32 option1_y;
	f32 text_x;
	u32 text_color[3];
	f32 digit_scale;

} MNVSReplayUiTwoOptionMenu;

extern u32 dMNVSReplayUiFileIDs[/* */];

void mnVSReplayUiInitTwoOptionMenu(MNVSReplayUiTwoOptionMenu *menu, const char *opt0, const char *opt1, f32 digit_scale);
void mnVSReplayUiSetTwoOptionPanel(MNVSReplayUiTwoOptionMenu *menu, f32 x0, f32 y0, f32 x1, f32 y1, f32 text_x,
                                   f32 option0_y, f32 option1_y);
void mnVSReplayUiLoadFiles(void *files_out[2], LBFileNode *status_buffer, size_t status_count);
void mnVSReplayUiLoadFilesInto(void *files_out[2]);
void mnVSReplayUiDrawModalDim(void);
void mnVSReplayUiDrawTwoOptionPanel(GObj *gobj, MNVSReplayUiTwoOptionMenu *menu, void *files[2]);
void mnVSReplayUiMakeTwoOptionText(GObj *gobj, MNVSReplayUiTwoOptionMenu *menu, void *files[2]);
sb32 mnVSReplayUiUpdateTwoOptionMenu(MNVSReplayUiTwoOptionMenu *menu, s32 *out_choice);
sb32 mnVSReplayUiUpdateTwoOptionCancel(MNVSReplayUiTwoOptionMenu *menu);
sb32 mnVSReplayUiUpdateTwoOptionMenuHardware(MNVSReplayUiTwoOptionMenu *menu, s32 *out_choice);
sb32 mnVSReplayUiUpdateTwoOptionCancelHardware(MNVSReplayUiTwoOptionMenu *menu);

#endif /* _MNVSREPLAY_UI_H_ */
