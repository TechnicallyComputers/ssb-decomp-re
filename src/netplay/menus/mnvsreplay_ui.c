#include <sc/scene.h>
#include <sys/controller.h>
#include <mn/menu.h>
#include <sys/controller.h>
#include <sys/rdp.h>
#include <reloc_data.h>
#include <sys/netinput.h>

#include "mnvsreplay_ui.h"

u32 dMNVSReplayUiFileIDs[/* */] =
{
	llMNCommonFontsFileID,
	llIFCommonDigitsFileID
};

static s32 mnVSReplayUiGetCharacterID(const char c)
{
	switch (c)
	{
	case ' ':
		return 0x1D;
	default:
		if ((c >= 'A') && (c <= 'Z'))
		{
			return c - 'A';
		}
		if ((c >= 'a') && (c <= 'z'))
		{
			return c - 'a';
		}
		return 0x1D;
	}
}

static void mnVSReplayUiMakeText(GObj *gobj, const char *str, f32 x, f32 y, u32 *color, f32 digit_scale, void *files[2])
{
	intptr_t chars[/* */] =
	{
		llMNCommonFontsLetterASprite, llMNCommonFontsLetterBSprite,
		llMNCommonFontsLetterCSprite, llMNCommonFontsLetterDSprite,
		llMNCommonFontsLetterESprite, llMNCommonFontsLetterFSprite,
		llMNCommonFontsLetterGSprite, llMNCommonFontsLetterHSprite,
		llMNCommonFontsLetterISprite, llMNCommonFontsLetterJSprite,
		llMNCommonFontsLetterKSprite, llMNCommonFontsLetterLSprite,
		llMNCommonFontsLetterMSprite, llMNCommonFontsLetterNSprite,
		llMNCommonFontsLetterOSprite, llMNCommonFontsLetterPSprite,
		llMNCommonFontsLetterQSprite, llMNCommonFontsLetterRSprite,
		llMNCommonFontsLetterSSprite, llMNCommonFontsLetterTSprite,
		llMNCommonFontsLetterUSprite, llMNCommonFontsLetterVSprite,
		llMNCommonFontsLetterWSprite, llMNCommonFontsLetterXSprite,
		llMNCommonFontsLetterYSprite, llMNCommonFontsLetterZSprite,
		llMNCommonFontsSymbolApostropheSprite,
		llMNCommonFontsSymbolPercentSprite,
		llMNCommonFontsSymbolPeriodSprite
	};
	intptr_t digits[/* */] =
	{
		llIFCommonDigits0Sprite, llIFCommonDigits1Sprite,
		llIFCommonDigits2Sprite, llIFCommonDigits3Sprite,
		llIFCommonDigits4Sprite, llIFCommonDigits5Sprite,
		llIFCommonDigits6Sprite, llIFCommonDigits7Sprite,
		llIFCommonDigits8Sprite, llIFCommonDigits9Sprite
	};
	SObj *sobj;
	f32 start_x;
	s32 i;

	if ((gobj == NULL) || (str == NULL) || (files == NULL))
	{
		return;
	}
	start_x = x;
	for (i = 0; str[i] != 0; i++)
	{
		if ((str[i] >= '0') && (str[i] <= '9'))
		{
			Sprite *sprite = lbRelocGetFileData(Sprite *, files[1], digits[str[i] - '0']);

			if (sprite == NULL)
			{
				continue;
			}
			sobj = lbCommonMakeSObjForGObj(gobj, sprite);
			if (digit_scale != 1.0F)
			{
				sobj->sprite.scalex = digit_scale;
				sobj->sprite.scaley = digit_scale;
			}
			sobj->pos.x = start_x;
			sobj->pos.y = y;
			sobj->sprite.red = (u8)color[0];
			sobj->sprite.green = (u8)color[1];
			sobj->sprite.blue = (u8)color[2];
			start_x += ((f32)sobj->sprite.width * digit_scale) + ((digit_scale < 1.0F) ? 0.5F : 1.0F);
		}
		else
		{
			s32 cid = mnVSReplayUiGetCharacterID(str[i]);
			Sprite *sprite;

			if (cid == 0x1D)
			{
				start_x += (digit_scale < 1.0F) ? 3.0F : 4.0F;
				continue;
			}
			sprite = lbRelocGetFileData(Sprite *, files[0], chars[cid]);
			if (sprite == NULL)
			{
				continue;
			}
			sobj = lbCommonMakeSObjForGObj(gobj, sprite);
			sobj->pos.x = start_x;
			sobj->pos.y = y;
			sobj->sprite.red = (u8)color[0];
			sobj->sprite.green = (u8)color[1];
			sobj->sprite.blue = (u8)color[2];
			start_x += (f32)sobj->sprite.width + 1.0F;
		}
	}
}

void mnVSReplayUiInitTwoOptionMenu(MNVSReplayUiTwoOptionMenu *menu, const char *opt0, const char *opt1, f32 digit_scale)
{
	if (menu == NULL)
	{
		return;
	}
	menu->active = TRUE;
	menu->choice = 0;
	menu->change_wait = 0;
	menu->option0 = opt0;
	menu->option1 = opt1;
	menu->panel_x0 = 60.0F;
	menu->panel_y0 = 72.0F;
	menu->panel_x1 = 260.0F;
	menu->panel_y1 = 168.0F;
	menu->text_x = 72.0F;
	menu->option0_y = 96.0F;
	menu->option1_y = 128.0F;
	menu->text_color[0] = 0x00;
	menu->text_color[1] = 0x00;
	menu->text_color[2] = 0x00;
	menu->digit_scale = digit_scale;
}

void mnVSReplayUiSetTwoOptionPanel(MNVSReplayUiTwoOptionMenu *menu, f32 x0, f32 y0, f32 x1, f32 y1, f32 text_x,
                                   f32 option0_y, f32 option1_y)
{
	if (menu == NULL)
	{
		return;
	}
	menu->panel_x0 = x0;
	menu->panel_y0 = y0;
	menu->panel_x1 = x1;
	menu->panel_y1 = y1;
	menu->text_x = text_x;
	menu->option0_y = option0_y;
	menu->option1_y = option1_y;
}

void mnVSReplayUiLoadFiles(void *files_out[2], LBFileNode *status_buffer, size_t status_count)
{
	LBRelocSetup rl_setup;

	if ((files_out == NULL) || (status_buffer == NULL))
	{
		return;
	}
	rl_setup.table_addr = (uintptr_t)&lLBRelocTableAddr;
	rl_setup.table_files_num = (u32)llRelocFileCount;
	rl_setup.file_heap = NULL;
	rl_setup.file_heap_size = 0;
	rl_setup.status_buffer = status_buffer;
	rl_setup.status_buffer_size = status_count;
	rl_setup.force_status_buffer = NULL;
	rl_setup.force_status_buffer_size = 0;
	lbRelocInitSetup(&rl_setup);
	lbRelocLoadFilesListed(dMNVSReplayUiFileIDs, files_out);
}

void mnVSReplayUiLoadFilesInto(void *files_out[2])
{
	if (files_out == NULL)
	{
		return;
	}
	lbRelocLoadFilesListed(dMNVSReplayUiFileIDs, files_out);
}

void mnVSReplayUiDrawModalDim(void)
{
	gDPPipeSync(gSYTaskmanDLHeads[0]++);
	gDPSetCycleType(gSYTaskmanDLHeads[0]++, G_CYC_1CYCLE);
	gDPSetPrimColor(gSYTaskmanDLHeads[0]++, 0, 0, 0x00, 0x00, 0x00, 0xA0);
	gDPSetCombineLERP(gSYTaskmanDLHeads[0]++, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0,
	                  0, PRIMITIVE);
	gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
	gDPFillRectangle(gSYTaskmanDLHeads[0]++, 10, 10, 310, 230);
	gDPPipeSync(gSYTaskmanDLHeads[0]++);
	gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
	gDPSetCycleType(gSYTaskmanDLHeads[0]++, G_CYC_1CYCLE);
}

void mnVSReplayUiDrawTwoOptionPanel(GObj *gobj, MNVSReplayUiTwoOptionMenu *menu, void *files[2])
{
	f32 highlight_y0;
	f32 highlight_y1;

	(void)files;

	if (menu == NULL)
	{
		return;
	}
	mnVSReplayUiDrawModalDim();
	gDPPipeSync(gSYTaskmanDLHeads[0]++);
	gDPSetCycleType(gSYTaskmanDLHeads[0]++, G_CYC_1CYCLE);
	gDPSetPrimColor(gSYTaskmanDLHeads[0]++, 0, 0, 0x28, 0x28, 0x28, 0xF0);
	gDPSetCombineLERP(gSYTaskmanDLHeads[0]++, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0,
	                  0, PRIMITIVE);
	gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
	gDPFillRectangle(gSYTaskmanDLHeads[0]++, (s32)menu->panel_x0, (s32)menu->panel_y0, (s32)menu->panel_x1,
	                 (s32)menu->panel_y1);
	highlight_y0 = (menu->choice == 0) ? menu->option0_y - 2.0F : menu->option1_y - 2.0F;
	highlight_y1 = highlight_y0 + 16.0F;
	gDPSetPrimColor(gSYTaskmanDLHeads[0]++, 0, 0, 0xFF, 0xFF, 0xFF, 0x70);
	gDPFillRectangle(gSYTaskmanDLHeads[0]++, (s32)menu->panel_x0 + 4, (s32)highlight_y0, (s32)menu->panel_x1 - 4,
	                 (s32)highlight_y1);
	gDPPipeSync(gSYTaskmanDLHeads[0]++);
	gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
	gDPSetCycleType(gSYTaskmanDLHeads[0]++, G_CYC_1CYCLE);
	lbCommonClearExternSpriteParams();
	lbCommonDrawSObjAttr(gobj);
}

void mnVSReplayUiMakeTwoOptionText(GObj *gobj, MNVSReplayUiTwoOptionMenu *menu, void *files[2])
{
	if ((gobj == NULL) || (menu == NULL))
	{
		return;
	}
	mnVSReplayUiMakeText(gobj, menu->option0, menu->text_x, menu->option0_y, menu->text_color, menu->digit_scale,
	                     files);
	mnVSReplayUiMakeText(gobj, menu->option1, menu->text_x, menu->option1_y, menu->text_color, menu->digit_scale,
	                     files);
}

sb32 mnVSReplayUiUpdateTwoOptionMenu(MNVSReplayUiTwoOptionMenu *menu, s32 *out_choice)
{
	s32 stick_range;
	s32 is_button;

	if ((menu == NULL) || (menu->active == FALSE))
	{
		return FALSE;
	}
	if (menu->change_wait != 0)
	{
		menu->change_wait--;
	}
	if (scSubsysControllerGetPlayerTapButtons(A_BUTTON | START_BUTTON) != FALSE)
	{
		if (out_choice != NULL)
		{
			*out_choice = menu->choice;
		}
		menu->active = FALSE;
		return TRUE;
	}
	if (mnCommonCheckGetOptionButtonInput(menu->change_wait, is_button, U_JPAD | U_CBUTTONS) ||
	    mnCommonCheckGetOptionStickInputUD(menu->change_wait, stick_range, 20, 1))
	{
		mnCommonSetOptionChangeWaitP(menu->change_wait, is_button, stick_range, 7);
		if (menu->choice != 0)
		{
			menu->choice = 0;
			return 2;
		}
		return FALSE;
	}
	if (mnCommonCheckGetOptionButtonInput(menu->change_wait, is_button, D_JPAD | D_CBUTTONS) ||
	    mnCommonCheckGetOptionStickInputUD(menu->change_wait, stick_range, -20, 0))
	{
		mnCommonSetOptionChangeWaitN(menu->change_wait, is_button, stick_range, 7);
		if (menu->choice != 1)
		{
			menu->choice = 1;
			return 2;
		}
		return FALSE;
	}
	return FALSE;
}

sb32 mnVSReplayUiUpdateTwoOptionCancel(MNVSReplayUiTwoOptionMenu *menu)
{
	if ((menu == NULL) || (menu->active == FALSE))
	{
		return FALSE;
	}
	if (scSubsysControllerGetPlayerTapButtons(B_BUTTON) != FALSE)
	{
		menu->active = FALSE;
		return TRUE;
	}
	return FALSE;
}

#if defined(PORT) && defined(SSB64_NETMENU)

#define mnVSReplayUiCheckHardwareButtonInput(wait, is_button, mask) \
	(((wait) == 0) && ((is_button) = syNetInputGetPortHardwareHoldButtons(mask), (is_button) != FALSE))

#define mnVSReplayUiCheckHardwareStickInputUD(wait, stick_range, min, b) \
	(((wait) == 0) && ((stick_range) = syNetInputGetPortHardwareStickUD(min, b), (stick_range) != 0))

sb32 mnVSReplayUiUpdateTwoOptionMenuHardware(MNVSReplayUiTwoOptionMenu *menu, s32 *out_choice)
{
	s32 stick_range;
	s32 is_button;

	if ((menu == NULL) || (menu->active == FALSE))
	{
		return FALSE;
	}
	if (menu->change_wait != 0)
	{
		menu->change_wait--;
	}
	if (syNetInputGetPortHardwareTapButtons(A_BUTTON | START_BUTTON) != FALSE)
	{
		if (out_choice != NULL)
		{
			*out_choice = menu->choice;
		}
		menu->active = FALSE;
		return TRUE;
	}
	if (mnVSReplayUiCheckHardwareButtonInput(menu->change_wait, is_button, U_JPAD | U_CBUTTONS) ||
	    mnVSReplayUiCheckHardwareStickInputUD(menu->change_wait, stick_range, 20, 1))
	{
		mnCommonSetOptionChangeWaitP(menu->change_wait, is_button, stick_range, 7);
		if (menu->choice != 0)
		{
			menu->choice = 0;
			return 2;
		}
		return FALSE;
	}
	if (mnVSReplayUiCheckHardwareButtonInput(menu->change_wait, is_button, D_JPAD | D_CBUTTONS) ||
	    mnVSReplayUiCheckHardwareStickInputUD(menu->change_wait, stick_range, -20, 0))
	{
		mnCommonSetOptionChangeWaitN(menu->change_wait, is_button, stick_range, 7);
		if (menu->choice != 1)
		{
			menu->choice = 1;
			return 2;
		}
		return FALSE;
	}
	return FALSE;
}

sb32 mnVSReplayUiUpdateTwoOptionCancelHardware(MNVSReplayUiTwoOptionMenu *menu)
{
	if ((menu == NULL) || (menu->active == FALSE))
	{
		return FALSE;
	}
	if (syNetInputGetPortHardwareTapButtons(B_BUTTON) != FALSE)
	{
		menu->active = FALSE;
		return TRUE;
	}
	return FALSE;
}

#endif /* PORT && SSB64_NETMENU */
