#include <mn/menu.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <gm/gmsound.h>
#include <sc/scene.h>
#include <lb/library.h>
#include <sys/video.h>
#include <sys/controller.h>
#include <sys/rdp.h>
#include <sys/audio.h>
#include <reloc_data.h>
#include <sys/netreplay.h>
#include <sys/netplay_save.h>

extern void *func_800269C0_275C0(u16 id);

#define mnVSReplaysCheckGetOptionButtonInput(is_button, mask) \
	mnCommonCheckGetOptionButtonInput(sMNVSReplaysChangeWait, is_button, mask)

#define mnVSReplaysCheckGetOptionStickInputUD(stick_range, min, b) \
	mnCommonCheckGetOptionStickInputUD(sMNVSReplaysChangeWait, stick_range, min, b)

#define mnVSReplaysCheckGetOptionStickInputLR(stick_range, min, b) \
	mnCommonCheckGetOptionStickInputLR(sMNVSReplaysChangeWait, stick_range, min, b)

#define mnVSReplaysSetOptionChangeWaitP(is_button, stick_range, div) \
	mnCommonSetOptionChangeWaitP(sMNVSReplaysChangeWait, is_button, stick_range, div)

#define mnVSReplaysSetOptionChangeWaitN(is_button, stick_range, div) \
	mnCommonSetOptionChangeWaitN(sMNVSReplaysChangeWait, is_button, stick_range, div)

#define MN_VSREPLAYS_VISIBLE_ROWS 8
#define MN_VSREPLAYS_FILE_PLAYER_TAGS 5
#define MN_VSREPLAYS_FIGHTER_NAME_X 190.0F
#define MN_VSREPLAYS_LIST_BOX_X0 24.0F
#define MN_VSREPLAYS_LIST_BOX_Y0 52.0F
#define MN_VSREPLAYS_LIST_TEXT_X (MN_VSREPLAYS_LIST_BOX_X0 + 4.0F)
#define MN_VSREPLAYS_LIST_ROW_Y0 56.0F
#define MN_VSREPLAYS_DIGIT_SCALE 0.55F
#define MN_VSREPLAYS_DIGIT_SPACE 4.0F
#define MN_VSREPLAYS_UNDERSCORE_ADVANCE 4.0F
#define MN_VSREPLAYS_ACTIONS_PANEL_X0 225
#define MN_VSREPLAYS_ACTIONS_PANEL_Y0 143
#define MN_VSREPLAYS_ACTIONS_PANEL_X1 310
#define MN_VSREPLAYS_ACTIONS_PANEL_Y1 230
#define MN_VSREPLAYS_ACTIONS_TEXT_X 232.0F
#define MN_VSREPLAYS_ACTIONS_OPTION0_Y 168.0F
#define MN_VSREPLAYS_ACTIONS_OPTION1_Y 192.0F
#define MN_VSREPLAYS_DELETE_ALL_TEXT_X 152.0F
#define MN_VSREPLAYS_DELETE_ALL_HIGHLIGHT_X0 150
#define MN_VSREPLAYS_DELETE_ALL_HIGHLIGHT_Y0 24
#define MN_VSREPLAYS_DELETE_ALL_HIGHLIGHT_X1 305
#define MN_VSREPLAYS_DELETE_ALL_HIGHLIGHT_Y1 40
#define MN_VSREPLAYS_CONFIRM_PANEL_X0 158
#define MN_VSREPLAYS_CONFIRM_PANEL_Y0 8
#define MN_VSREPLAYS_CONFIRM_PANEL_X1 305
#define MN_VSREPLAYS_CONFIRM_PANEL_Y1 78
#define MN_VSREPLAYS_CONFIRM_TITLE_X 165.0F
#define MN_VSREPLAYS_CONFIRM_TITLE_Y 14.0F
#define MN_VSREPLAYS_CONFIRM_NO_X 168.0F
#define MN_VSREPLAYS_CONFIRM_YES_X 248.0F
#define MN_VSREPLAYS_CONFIRM_OPTION_Y 52.0F

typedef enum MNVSReplaysFocusKind
{
	nMNVSReplaysFocusToggle,
	nMNVSReplaysFocusDeleteAll,
	nMNVSReplaysFocusList,
	nMNVSReplaysFocusActions

} MNVSReplaysFocusKind;

typedef enum MNVSReplaysActionChoice
{
	nMNVSReplaysActionPlayback,
	nMNVSReplaysActionDelete

} MNVSReplaysActionChoice;

u32 dMNVSReplaysFileIDs[/* */] =
{
	llMNCommonFileID,
	llMNDataFileID,
	llMNPlayersCommonFileID,
	llMNMapsFileID,
	llMNCommonFontsFileID,
	llIFCommonPlayerTagsFileID
};

Lights1 dMNVSReplaysLights1 = gdSPDefLights1(0x20, 0x20, 0x20, 0xFF, 0xFF, 0xFF, 0x3C, 0x3C, 0x3C);

Gfx dMNVSReplaysDisplayList[/* */] =
{
	gsSPSetGeometryMode(G_LIGHTING),
	gsSPSetLights1(dMNVSReplaysLights1),
	gsSPEndDisplayList()
};

void *sMNVSReplaysFiles[ARRAY_COUNT(dMNVSReplaysFileIDs)];
LBFileNode sMNVSReplaysStatusBuffer[32];
GObj *sMNVSReplaysListGObj;
GObj *sMNVSReplaysMetaGObj;
GObj *sMNVSReplaysToggleGObj;
GObj *sMNVSReplaysActionsGObj;
GObj *sMNVSReplaysConfirmGObj;
s32 sMNVSReplaysFocus;
s32 sMNVSReplaysActionChoice;
sb32 sMNVSReplaysListEntered;
sb32 sMNVSReplaysConfirmActive;
sb32 sMNVSReplaysConfirmYesOrNo;
s32 sMNVSReplaysCursorIndex;
s32 sMNVSReplaysScrollOffset;
s32 sMNVSReplaysFileCount;
s32 sMNVSReplaysChangeWait;
s32 sMNVSReplaysScrollWait;
sb32 sMNVSReplaysSaveEnabled;
char sMNVSReplaysFileNames[SYNETREPLAY_USER_MAX_FILES][SYNETREPLAY_USER_FILENAME_MAX];
SYNetInputReplayMetadata sMNVSReplaysHighlightMetadata;
sb32 sMNVSReplaysHighlightValid;

static sb32 mnVSReplaysRefreshHighlight(void);
static void mnVSReplaysMakeActions(void);
static void mnVSReplaysRefreshListUi(void);
static void mnVSReplaysMakeTopRow(void);
static void mnVSReplaysMakeConfirm(void);
static void mnVSReplaysEjectConfirm(void);
static void mnVSReplaysDismissConfirm(void);
static void mnVSReplaysUpdateConfirmMenu(void);
static sb32 mnVSReplaysIsTopRowFocus(void);

static size_t mnVSReplaysStrLen(const char *s)
{
	size_t n;

	n = 0U;
	if (s == NULL)
	{
		return 0U;
	}
	while (s[n] != '\0')
	{
		n++;
	}
	return n;
}

static sb32 mnVSReplaysStrEqual(const char *a, const char *b)
{
	size_t i;

	if ((a == NULL) || (b == NULL))
	{
		return FALSE;
	}
	for (i = 0U; (a[i] != '\0') || (b[i] != '\0'); i++)
	{
		if (a[i] != b[i])
		{
			return FALSE;
		}
	}
	return TRUE;
}

static void mnVSReplaysStrCopy(char *out, size_t cap, const char *src)
{
	size_t i;

	if ((out == NULL) || (cap == 0U))
	{
		return;
	}
	if (src == NULL)
	{
		out[0] = '\0';
		return;
	}
	for (i = 0U; (i < (cap - 1U)) && (src[i] != '\0'); i++)
	{
		out[i] = src[i];
	}
	out[i] = '\0';
}

static s32 mnVSReplaysGetCharacterID(const char c)
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

static void mnVSReplaysMakeText(GObj *gobj, const char *str, f32 x, f32 y, u32 *color, sb32 use_tight_digits)
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
	intptr_t menu_digits[/* */] =
	{
		llMNCommonDigit0Sprite, llMNCommonDigit1Sprite,
		llMNCommonDigit2Sprite, llMNCommonDigit3Sprite,
		llMNCommonDigit4Sprite, llMNCommonDigit5Sprite,
		llMNCommonDigit6Sprite, llMNCommonDigit7Sprite,
		llMNCommonDigit8Sprite, llMNCommonDigit9Sprite
	};
	SObj *sobj;
	f32 start_x;
	s32 i;

	if ((gobj == NULL) || (str == NULL))
	{
		return;
	}
	start_x = x;
	for (i = 0; str[i] != 0; i++)
	{
		if ((str[i] >= '0') && (str[i] <= '9'))
		{
			Sprite *sprite;

			if (use_tight_digits == FALSE)
			{
				continue;
			}
			sprite = lbRelocGetFileData(Sprite *, sMNVSReplaysFiles[0], menu_digits[str[i] - '0']);
			if (sprite == NULL)
			{
				continue;
			}
			sobj = lbCommonMakeSObjForGObj(gobj, sprite);
			sobj->sprite.attr &= ~SP_FASTCOPY;
			sobj->sprite.attr |= SP_TRANSPARENT;
			sobj->sprite.scalex = MN_VSREPLAYS_DIGIT_SCALE;
			sobj->sprite.scaley = MN_VSREPLAYS_DIGIT_SCALE;
			sobj->pos.x = start_x;
			sobj->pos.y = y;
			sobj->sprite.red = 0xFF;
			sobj->sprite.green = 0xFF;
			sobj->sprite.blue = 0xFF;
			start_x += ((f32)sprite->width * MN_VSREPLAYS_DIGIT_SCALE) + 0.5F;
		}
		else if ((use_tight_digits != FALSE) && (str[i] == '_'))
		{
			start_x += MN_VSREPLAYS_UNDERSCORE_ADVANCE * MN_VSREPLAYS_DIGIT_SCALE;
		}
		else
		{
			s32 cid = mnVSReplaysGetCharacterID(str[i]);

			if (cid == 0x1D)
			{
				start_x += (use_tight_digits != FALSE) ?
				               (MN_VSREPLAYS_DIGIT_SPACE * MN_VSREPLAYS_DIGIT_SCALE) :
				               4.0F;
				continue;
			}
			{
				Sprite *sprite = lbRelocGetFileData(Sprite *, sMNVSReplaysFiles[4], chars[cid]);

				if (sprite == NULL)
				{
					continue;
				}
				sobj = lbCommonMakeSObjForGObj(gobj, sprite);
			}
			sobj->pos.x = start_x;
			sobj->pos.y = y;
			sobj->sprite.red = (u8)color[0];
			sobj->sprite.green = (u8)color[1];
			sobj->sprite.blue = (u8)color[2];
			start_x += (f32)sobj->sprite.width + 1.0F;
		}
	}
}

static void mnVSReplaysFormatDisplayName(const char *filename, char *out, size_t cap)
{
	size_t len;
	size_t ext_len;

	if ((filename == NULL) || (out == NULL) || (cap == 0U))
	{
		return;
	}
	mnVSReplaysStrCopy(out, cap, filename);
	ext_len = mnVSReplaysStrLen(SYNETREPLAY_USER_FILE_EXT);
	len = mnVSReplaysStrLen(out);
	if ((len > ext_len) && (mnVSReplaysStrEqual(out + len - ext_len, SYNETREPLAY_USER_FILE_EXT) != FALSE))
	{
		out[len - ext_len] = '\0';
	}
}

static void mnVSReplaysFormatDateTimeFromFilename(const char *filename, char *out, size_t cap)
{
	const char *base;
	size_t len;
	size_t ext_len;

	if ((filename == NULL) || (out == NULL) || (cap == 0U))
	{
		return;
	}
	mnVSReplaysStrCopy(out, cap, "UNKNOWN");
	base = filename;
	len = mnVSReplaysStrLen(base);
	ext_len = mnVSReplaysStrLen(SYNETREPLAY_USER_FILE_EXT);
	if ((len > ext_len) && (mnVSReplaysStrEqual(base + len - ext_len, SYNETREPLAY_USER_FILE_EXT) != FALSE))
	{
		len -= ext_len;
	}
	if (len != 15U)
	{
		return;
	}
	if (cap < 21U)
	{
		return;
	}
	/* YYYY MM DD  HH MM SS */
	out[0] = base[0];
	out[1] = base[1];
	out[2] = base[2];
	out[3] = base[3];
	out[4] = ' ';
	out[5] = base[4];
	out[6] = base[5];
	out[7] = ' ';
	out[8] = base[6];
	out[9] = base[7];
	out[10] = ' ';
	out[11] = ' ';
	out[12] = base[9];
	out[13] = base[10];
	out[14] = ' ';
	out[15] = base[11];
	out[16] = base[12];
	out[17] = ' ';
	out[18] = base[13];
	out[19] = base[14];
	out[20] = '\0';
}

static sb32 mnVSReplaysRefreshHighlight(void)
{
	char path[SYNETREPLAY_USER_PATH_MAX];

	sMNVSReplaysHighlightValid = FALSE;
	if ((sMNVSReplaysFileCount <= 0) || (sMNVSReplaysCursorIndex < 0) ||
	    (sMNVSReplaysCursorIndex >= sMNVSReplaysFileCount))
	{
		return FALSE;
	}
	if (syNetReplayResolveUserFilePath(sMNVSReplaysFileNames[sMNVSReplaysCursorIndex], path, sizeof(path)) == FALSE)
	{
		return FALSE;
	}
	if (syNetReplayReadMetadataOnly(path, &sMNVSReplaysHighlightMetadata) == FALSE)
	{
		return FALSE;
	}
	sMNVSReplaysHighlightValid = TRUE;
	return TRUE;
}

static void mnVSReplaysReloadFileList(void)
{
	syNetReplayEnumerateUserFiles(sMNVSReplaysFileNames, SYNETREPLAY_USER_MAX_FILES, &sMNVSReplaysFileCount);
	if (sMNVSReplaysCursorIndex >= sMNVSReplaysFileCount)
	{
		sMNVSReplaysCursorIndex = (sMNVSReplaysFileCount > 0) ? (sMNVSReplaysFileCount - 1) : 0;
	}
	if (sMNVSReplaysScrollOffset > sMNVSReplaysCursorIndex)
	{
		sMNVSReplaysScrollOffset = sMNVSReplaysCursorIndex;
	}
	if (sMNVSReplaysCursorIndex >= (sMNVSReplaysScrollOffset + MN_VSREPLAYS_VISIBLE_ROWS))
	{
		sMNVSReplaysScrollOffset = sMNVSReplaysCursorIndex - MN_VSREPLAYS_VISIBLE_ROWS + 1;
	}
	(void)mnVSReplaysRefreshHighlight();
}

void mnVSReplaysRenderListBox(GObj *gobj)
{
	gDPPipeSync(gSYTaskmanDLHeads[0]++);
	gDPSetCycleType(gSYTaskmanDLHeads[0]++, G_CYC_1CYCLE);
	gDPSetPrimColor(gSYTaskmanDLHeads[0]++, 0, 0, 0xA0, 0x78, 0x14, 0xE6);
	gDPSetCombineLERP(gSYTaskmanDLHeads[0]++, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0,
	                  0, PRIMITIVE);
	gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
	gDPFillRectangle(gSYTaskmanDLHeads[0]++, 24, 52, 148, 196);
	if (sMNVSReplaysFocus == nMNVSReplaysFocusList)
	{
		if (sMNVSReplaysListEntered == FALSE)
		{
			gDPSetPrimColor(gSYTaskmanDLHeads[0]++, 0, 0, 0xFF, 0xFF, 0xFF, 0x80);
			gDPFillRectangle(gSYTaskmanDLHeads[0]++, 22, 50, 150, 198);
			gDPSetPrimColor(gSYTaskmanDLHeads[0]++, 0, 0, 0xA0, 0x78, 0x14, 0xE6);
			gDPFillRectangle(gSYTaskmanDLHeads[0]++, 24, 52, 148, 196);
		}
		else if ((sMNVSReplaysListEntered != FALSE) && (sMNVSReplaysFocus == nMNVSReplaysFocusList) &&
		         (sMNVSReplaysFileCount > 0))
		{
			s32 row;
			s32 y0;

			row = sMNVSReplaysCursorIndex - sMNVSReplaysScrollOffset;
			y0 = 54 + (row * 17);
			gDPSetPrimColor(gSYTaskmanDLHeads[0]++, 0, 0, 0xFF, 0xFF, 0xFF, 0x80);
			gDPFillRectangle(gSYTaskmanDLHeads[0]++, 26, y0, 146, y0 + 14);
		}
	}
	gDPPipeSync(gSYTaskmanDLHeads[0]++);
	gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
	gDPSetCycleType(gSYTaskmanDLHeads[0]++, G_CYC_1CYCLE);
	lbCommonClearExternSpriteParams();
	lbCommonDrawSObjAttr(gobj);
}

void mnVSReplaysMakeList(void)
{
	GObj *gobj;
	u32 color[3];
	char display_name[SYNETREPLAY_USER_FILENAME_MAX];
	s32 row;
	s32 file_index;

	if (sMNVSReplaysListGObj != NULL)
	{
		gcEjectGObj(sMNVSReplaysListGObj);
		sMNVSReplaysListGObj = NULL;
	}
	sMNVSReplaysListGObj = gobj = gcMakeGObjSPAfter(0, NULL, 3, GOBJ_PRIORITY_DEFAULT);
	gcAddGObjDisplay(gobj, mnVSReplaysRenderListBox, 2, GOBJ_PRIORITY_DEFAULT, ~0);
	color[0] = 0x00;
	color[1] = 0x00;
	color[2] = 0x00;
	for (row = 0; row < MN_VSREPLAYS_VISIBLE_ROWS; row++)
	{
		file_index = sMNVSReplaysScrollOffset + row;
		if (file_index >= sMNVSReplaysFileCount)
		{
			break;
		}
		mnVSReplaysFormatDisplayName(sMNVSReplaysFileNames[file_index], display_name, sizeof(display_name));
		mnVSReplaysMakeText(gobj, display_name, MN_VSREPLAYS_LIST_TEXT_X, (f32)(MN_VSREPLAYS_LIST_ROW_Y0 + (row * 17)),
		                    color, TRUE);
	}
}

static sb32 mnVSReplaysIsTopRowFocus(void)
{
	return ((sMNVSReplaysFocus == nMNVSReplaysFocusToggle) ||
	        (sMNVSReplaysFocus == nMNVSReplaysFocusDeleteAll)) ?
	           TRUE :
	           FALSE;
}

void mnVSReplaysRenderTopRow(GObj *gobj)
{
	if (sMNVSReplaysFocus == nMNVSReplaysFocusToggle)
	{
		gDPPipeSync(gSYTaskmanDLHeads[0]++);
		gDPSetCycleType(gSYTaskmanDLHeads[0]++, G_CYC_1CYCLE);
		gDPSetPrimColor(gSYTaskmanDLHeads[0]++, 0, 0, 0xFF, 0xFF, 0xFF, 0x80);
		gDPSetCombineLERP(gSYTaskmanDLHeads[0]++, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0,
		                  0, PRIMITIVE);
		gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
		gDPFillRectangle(gSYTaskmanDLHeads[0]++, 24, 24, 148, 40);
		gDPPipeSync(gSYTaskmanDLHeads[0]++);
		gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
		gDPSetCycleType(gSYTaskmanDLHeads[0]++, G_CYC_1CYCLE);
	}
	else if (sMNVSReplaysFocus == nMNVSReplaysFocusDeleteAll)
	{
		gDPPipeSync(gSYTaskmanDLHeads[0]++);
		gDPSetCycleType(gSYTaskmanDLHeads[0]++, G_CYC_1CYCLE);
		gDPSetPrimColor(gSYTaskmanDLHeads[0]++, 0, 0, 0xFF, 0xFF, 0xFF, 0x80);
		gDPSetCombineLERP(gSYTaskmanDLHeads[0]++, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0,
		                  0, PRIMITIVE);
		gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
		gDPFillRectangle(gSYTaskmanDLHeads[0]++, MN_VSREPLAYS_DELETE_ALL_HIGHLIGHT_X0,
		                 MN_VSREPLAYS_DELETE_ALL_HIGHLIGHT_Y0, MN_VSREPLAYS_DELETE_ALL_HIGHLIGHT_X1,
		                 MN_VSREPLAYS_DELETE_ALL_HIGHLIGHT_Y1);
		gDPPipeSync(gSYTaskmanDLHeads[0]++);
		gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
		gDPSetCycleType(gSYTaskmanDLHeads[0]++, G_CYC_1CYCLE);
	}
	lbCommonClearExternSpriteParams();
	lbCommonDrawSObjAttr(gobj);
}

void mnVSReplaysMakeTopRow(void)
{
	GObj *gobj;
	u32 color[3];
	const char *label;

	if (sMNVSReplaysToggleGObj != NULL)
	{
		gcEjectGObj(sMNVSReplaysToggleGObj);
		sMNVSReplaysToggleGObj = NULL;
	}
	sMNVSReplaysToggleGObj = gobj = gcMakeGObjSPAfter(0, NULL, 4, GOBJ_PRIORITY_DEFAULT);
	gcAddGObjDisplay(gobj, mnVSReplaysRenderTopRow, 2, GOBJ_PRIORITY_DEFAULT, ~0);
	color[0] = 0x00;
	color[1] = 0x00;
	color[2] = 0x00;
	label = (sMNVSReplaysSaveEnabled != FALSE) ? "SAVE REPLAYS ON" : "SAVE REPLAYS OFF";
	mnVSReplaysMakeText(gobj, label, MN_VSREPLAYS_LIST_TEXT_X, 28.0F, color, FALSE);
	mnVSReplaysMakeText(gobj, "DELETE ALL REPLAYS", MN_VSREPLAYS_DELETE_ALL_TEXT_X, 28.0F, color, FALSE);
}

void mnVSReplaysRenderConfirm(GObj *gobj)
{
	s32 highlight_x0;
	s32 highlight_x1;

	gDPPipeSync(gSYTaskmanDLHeads[0]++);
	gDPSetCycleType(gSYTaskmanDLHeads[0]++, G_CYC_1CYCLE);
	gDPSetPrimColor(gSYTaskmanDLHeads[0]++, 0, 0, 0xA0, 0x78, 0x14, 0xE6);
	gDPSetCombineLERP(gSYTaskmanDLHeads[0]++, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0,
	                  0, PRIMITIVE);
	gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
	gDPFillRectangle(gSYTaskmanDLHeads[0]++, MN_VSREPLAYS_CONFIRM_PANEL_X0, MN_VSREPLAYS_CONFIRM_PANEL_Y0,
	                 MN_VSREPLAYS_CONFIRM_PANEL_X1, MN_VSREPLAYS_CONFIRM_PANEL_Y1);
	if (sMNVSReplaysConfirmYesOrNo == 0)
	{
		highlight_x0 = (s32)MN_VSREPLAYS_CONFIRM_YES_X - 2;
		highlight_x1 = highlight_x0 + 36;
	}
	else
	{
		highlight_x0 = (s32)MN_VSREPLAYS_CONFIRM_NO_X - 2;
		highlight_x1 = highlight_x0 + 24;
	}
	gDPSetPrimColor(gSYTaskmanDLHeads[0]++, 0, 0, 0xFF, 0xFF, 0xFF, 0x80);
	gDPFillRectangle(gSYTaskmanDLHeads[0]++, highlight_x0, (s32)MN_VSREPLAYS_CONFIRM_OPTION_Y - 2, highlight_x1,
	                 (s32)MN_VSREPLAYS_CONFIRM_OPTION_Y + 12);
	gDPPipeSync(gSYTaskmanDLHeads[0]++);
	gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
	gDPSetCycleType(gSYTaskmanDLHeads[0]++, G_CYC_1CYCLE);
	lbCommonClearExternSpriteParams();
	lbCommonDrawSObjAttr(gobj);
}

static void mnVSReplaysEjectConfirm(void)
{
	if (sMNVSReplaysConfirmGObj != NULL)
	{
		gcEjectGObj(sMNVSReplaysConfirmGObj);
		sMNVSReplaysConfirmGObj = NULL;
	}
}

static void mnVSReplaysDismissConfirm(void)
{
	sMNVSReplaysConfirmActive = FALSE;
	mnVSReplaysEjectConfirm();
}

static void mnVSReplaysMakeConfirm(void)
{
	GObj *gobj;
	u32 color[3];

	mnVSReplaysEjectConfirm();
	sMNVSReplaysConfirmGObj = gobj = gcMakeGObjSPAfter(0, NULL, nGCCommonLinkIDPauseMenu, GOBJ_PRIORITY_DEFAULT);
	gcAddGObjDisplay(gobj, mnVSReplaysRenderConfirm, 2, GOBJ_PRIORITY_DEFAULT, ~0);
	color[0] = 0x00;
	color[1] = 0x00;
	color[2] = 0x00;
	mnVSReplaysMakeText(gobj, "ARE YOU SURE?", MN_VSREPLAYS_CONFIRM_TITLE_X, MN_VSREPLAYS_CONFIRM_TITLE_Y, color,
	                    FALSE);
	mnVSReplaysMakeText(gobj, "NO", MN_VSREPLAYS_CONFIRM_NO_X, MN_VSREPLAYS_CONFIRM_OPTION_Y, color, FALSE);
	mnVSReplaysMakeText(gobj, "YES", MN_VSREPLAYS_CONFIRM_YES_X, MN_VSREPLAYS_CONFIRM_OPTION_Y, color, FALSE);
}

static void mnVSReplaysDeleteAllReplays(void)
{
	(void)syNetReplayDeleteAllUserFiles();
	mnVSReplaysReloadFileList();
	sMNVSReplaysCursorIndex = 0;
	sMNVSReplaysScrollOffset = 0;
	sMNVSReplaysListEntered = FALSE;
	sMNVSReplaysHighlightValid = FALSE;
}

static void mnVSReplaysUpdateConfirmMenu(void)
{
	s32 stick_range;
	sb32 is_button;

	if (scSubsysControllerGetPlayerTapButtons(A_BUTTON | START_BUTTON) != FALSE)
	{
		func_800269C0_275C0(nSYAudioFGMMenuSelect);
		if (sMNVSReplaysConfirmYesOrNo == 0)
		{
			mnVSReplaysDeleteAllReplays();
			mnVSReplaysDismissConfirm();
			sMNVSReplaysFocus = nMNVSReplaysFocusDeleteAll;
			mnVSReplaysMakeTopRow();
			mnVSReplaysRefreshListUi();
		}
		else
		{
			mnVSReplaysDismissConfirm();
			sMNVSReplaysFocus = nMNVSReplaysFocusDeleteAll;
			mnVSReplaysMakeTopRow();
		}
		return;
	}
	if (scSubsysControllerGetPlayerTapButtons(B_BUTTON) != FALSE)
	{
		func_800269C0_275C0(nSYAudioFGMMenuScroll2);
		mnVSReplaysDismissConfirm();
		sMNVSReplaysFocus = nMNVSReplaysFocusDeleteAll;
		mnVSReplaysMakeTopRow();
		return;
	}
	if (sMNVSReplaysChangeWait != 0)
	{
		sMNVSReplaysChangeWait--;
		return;
	}
	if (mnVSReplaysCheckGetOptionButtonInput(is_button, R_JPAD | R_CBUTTONS) ||
	    mnVSReplaysCheckGetOptionStickInputLR(stick_range, 20, 1))
	{
		sMNVSReplaysChangeWait = ((is_button != FALSE) ? 12 : mnCommonGetOptionChangeWaitN(stick_range, 7));
		if (sMNVSReplaysConfirmYesOrNo == 1)
		{
			func_800269C0_275C0(nSYAudioFGMMenuScroll2);
			sMNVSReplaysConfirmYesOrNo = 0;
		}
		return;
	}
	if (mnVSReplaysCheckGetOptionButtonInput(is_button, L_JPAD | L_CBUTTONS) ||
	    mnVSReplaysCheckGetOptionStickInputLR(stick_range, -20, 0))
	{
		sMNVSReplaysChangeWait = ((is_button != FALSE) ? 12 : mnCommonGetOptionChangeWaitP(stick_range, 7));
		if (sMNVSReplaysConfirmYesOrNo == 0)
		{
			func_800269C0_275C0(nSYAudioFGMMenuScroll2);
			sMNVSReplaysConfirmYesOrNo = 1;
		}
	}
}

static s32 mnVSReplaysGetMapSpriteOffset(s32 gkind)
{
	switch (gkind)
	{
	case nGRKindCastle:
		return 0;
	case nGRKindSector:
		return 1;
	case nGRKindJungle:
		return 2;
	case nGRKindZebes:
		return 3;
	case nGRKindHyrule:
		return 4;
	case nGRKindYoster:
		return 5;
	case nGRKindPupupu:
		return 6;
	case nGRKindYamabuki:
		return 7;
	case nGRKindInishie:
		return 8;
	default:
		return 0;
	}
}

static s32 mnVSReplaysGetFighterSpriteOffset(s32 fkind)
{
	if ((fkind < 0) || (fkind >= nFTKindEnumCount))
	{
		return -1;
	}
	return (s32)fkind;
}

void mnVSReplaysMakeMetadata(void)
{
	GObj *gobj;
	SObj *sobj;
	intptr_t player_labels[/* */] =
	{
		llIFCommonPlayerTags1PSprite, llIFCommonPlayerTags2PSprite,
		llIFCommonPlayerTags3PSprite, llIFCommonPlayerTags4PSprite
	};
	intptr_t fighter_names[/* */] =
	{
		llMNPlayersCommonMarioTextSprite, llMNPlayersCommonFoxTextSprite,
		llMNPlayersCommonDKTextSprite, llMNPlayersCommonSamusTextSprite,
		llMNPlayersCommonLuigiTextSprite, llMNPlayersCommonLinkTextSprite,
		llMNPlayersCommonYoshiTextSprite, llMNPlayersCommonCaptainFalconTextSprite,
		llMNPlayersCommonKirbyTextSprite, llMNPlayersCommonPikachuTextSprite,
		llMNPlayersCommonJigglypuffTextSprite, llMNPlayersCommonNessTextSprite
	};
	intptr_t map_names[/* */] =
	{
		llMNMapsPeachsCastleTextSprite, llMNMapsSectorZTextSprite,
		llMNMapsCongoJungleTextSprite, llMNMapsPlanetZebesTextSprite,
		llMNMapsHyruleCastleTextSprite, llMNMapsYoshisIslandTextSprite,
		llMNMapsDreamLandTextSprite, llMNMapsSaffronCityTextSprite,
		llMNMapsMushroomKingdomTextSprite
	};
	u32 color[3];
	s32 player;
	f32 y;
	s32 map_offset;
	s32 fighter_offset;
	char datetime[32];

	if (sMNVSReplaysMetaGObj != NULL)
	{
		gcEjectGObj(sMNVSReplaysMetaGObj);
		sMNVSReplaysMetaGObj = NULL;
	}
	sMNVSReplaysMetaGObj = gobj = gcMakeGObjSPAfter(0, NULL, 3, GOBJ_PRIORITY_DEFAULT);
	gcAddGObjDisplay(gobj, lbCommonDrawSObjAttr, 2, GOBJ_PRIORITY_DEFAULT, ~0);
	color[0] = 0x00;
	color[1] = 0x00;
	color[2] = 0x00;
	y = 56.0F;
	if (sMNVSReplaysHighlightValid == FALSE)
	{
		mnVSReplaysMakeText(gobj, "NO REPLAY SELECTED", 162.0F, y, color, FALSE);
		return;
	}
	for (player = 0; player < MAXCONTROLLERS; player++)
	{
		sobj = lbCommonMakeSObjForGObj(gobj,
		                               lbRelocGetFileData(Sprite *, sMNVSReplaysFiles[MN_VSREPLAYS_FILE_PLAYER_TAGS],
		                                                  player_labels[player]));
		sobj->sprite.attr &= ~SP_FASTCOPY;
		sobj->sprite.attr |= SP_TRANSPARENT;
		sobj->pos.x = 162.0F;
		sobj->pos.y = y;
		if (sMNVSReplaysHighlightMetadata.player_kinds[player] != nFTPlayerKindNot)
		{
			fighter_offset = mnVSReplaysGetFighterSpriteOffset((s32)sMNVSReplaysHighlightMetadata.fighter_kinds[player]);
			if (fighter_offset >= 0)
			{
				sobj = lbCommonMakeSObjForGObj(gobj,
				                                 lbRelocGetFileData(Sprite *, sMNVSReplaysFiles[2],
				                                                    fighter_names[fighter_offset]));
				sobj->pos.x = MN_VSREPLAYS_FIGHTER_NAME_X;
				sobj->pos.y = y;
			}
		}
		y += 18.0F;
	}
	y += 8.0F;
	map_offset = mnVSReplaysGetMapSpriteOffset((s32)sMNVSReplaysHighlightMetadata.stage_kind);
	sobj = lbCommonMakeSObjForGObj(gobj, lbRelocGetFileData(Sprite *, sMNVSReplaysFiles[3], map_names[map_offset]));
	sobj->pos.x = 162.0F;
	sobj->pos.y = y;
	y += 24.0F;
	if (sMNVSReplaysFileCount > 0)
	{
		mnVSReplaysFormatDateTimeFromFilename(sMNVSReplaysFileNames[sMNVSReplaysCursorIndex], datetime,
		                                      sizeof(datetime));
		mnVSReplaysMakeText(gobj, datetime, 162.0F, y, color, TRUE);
	}
}

static void mnVSReplaysRefreshListUi(void)
{
	mnVSReplaysMakeList();
	mnVSReplaysMakeMetadata();
	mnVSReplaysMakeActions();
}

static void mnVSReplaysStartPlayback(void);

static void mnVSReplaysDeleteSelectedReplay(void)
{
	if ((sMNVSReplaysFileCount <= 0) || (sMNVSReplaysCursorIndex < 0) ||
	    (sMNVSReplaysCursorIndex >= sMNVSReplaysFileCount))
	{
		return;
	}
	if (syNetReplayDeleteUserFile(sMNVSReplaysFileNames[sMNVSReplaysCursorIndex]) == FALSE)
	{
		return;
	}
	mnVSReplaysReloadFileList();
	if (sMNVSReplaysCursorIndex >= sMNVSReplaysFileCount)
	{
		sMNVSReplaysCursorIndex = (sMNVSReplaysFileCount > 0) ? (sMNVSReplaysFileCount - 1) : 0;
	}
	if (sMNVSReplaysScrollOffset > sMNVSReplaysCursorIndex)
	{
		sMNVSReplaysScrollOffset = sMNVSReplaysCursorIndex;
	}
	(void)mnVSReplaysRefreshHighlight();
}

void mnVSReplaysRenderActions(GObj *gobj)
{
	s32 highlight_y0;

	if (sMNVSReplaysFocus != nMNVSReplaysFocusActions)
	{
		return;
	}
	gDPPipeSync(gSYTaskmanDLHeads[0]++);
	gDPSetCycleType(gSYTaskmanDLHeads[0]++, G_CYC_1CYCLE);
	gDPSetPrimColor(gSYTaskmanDLHeads[0]++, 0, 0, 0xA0, 0x78, 0x14, 0xE6);
	gDPSetCombineLERP(gSYTaskmanDLHeads[0]++, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0,
	                  0, PRIMITIVE);
	gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
	gDPFillRectangle(gSYTaskmanDLHeads[0]++, MN_VSREPLAYS_ACTIONS_PANEL_X0, MN_VSREPLAYS_ACTIONS_PANEL_Y0,
	                 MN_VSREPLAYS_ACTIONS_PANEL_X1, MN_VSREPLAYS_ACTIONS_PANEL_Y1);
	highlight_y0 = (sMNVSReplaysActionChoice == nMNVSReplaysActionPlayback) ?
	                   ((s32)MN_VSREPLAYS_ACTIONS_OPTION0_Y - 2) :
	                   ((s32)MN_VSREPLAYS_ACTIONS_OPTION1_Y - 2);
	gDPSetPrimColor(gSYTaskmanDLHeads[0]++, 0, 0, 0xFF, 0xFF, 0xFF, 0x80);
	gDPFillRectangle(gSYTaskmanDLHeads[0]++, MN_VSREPLAYS_ACTIONS_PANEL_X0 + 2, highlight_y0,
	                 MN_VSREPLAYS_ACTIONS_PANEL_X1 - 2, highlight_y0 + 14);
	gDPPipeSync(gSYTaskmanDLHeads[0]++);
	gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
	gDPSetCycleType(gSYTaskmanDLHeads[0]++, G_CYC_1CYCLE);
	lbCommonClearExternSpriteParams();
	lbCommonDrawSObjAttr(gobj);
}

void mnVSReplaysMakeActions(void)
{
	GObj *gobj;
	u32 color[3];

	if (sMNVSReplaysActionsGObj != NULL)
	{
		gcEjectGObj(sMNVSReplaysActionsGObj);
		sMNVSReplaysActionsGObj = NULL;
	}
	if (sMNVSReplaysFocus != nMNVSReplaysFocusActions)
	{
		return;
	}
	sMNVSReplaysActionsGObj = gobj = gcMakeGObjSPAfter(0, NULL, 5, GOBJ_PRIORITY_DEFAULT);
	gcAddGObjDisplay(gobj, mnVSReplaysRenderActions, 2, GOBJ_PRIORITY_DEFAULT, ~0);
	color[0] = 0x00;
	color[1] = 0x00;
	color[2] = 0x00;
	mnVSReplaysMakeText(gobj, "REPLAY PLAYBACK", MN_VSREPLAYS_ACTIONS_TEXT_X, MN_VSREPLAYS_ACTIONS_OPTION0_Y, color,
	                    FALSE);
	mnVSReplaysMakeText(gobj, "DELETE REPLAY", MN_VSREPLAYS_ACTIONS_TEXT_X, MN_VSREPLAYS_ACTIONS_OPTION1_Y, color,
	                    FALSE);
}

void mnVSReplaysMakeDecals(void)
{
	GObj *gobj;
	SObj *sobj;

	gobj = gcMakeGObjSPAfter(0, NULL, 2, GOBJ_PRIORITY_DEFAULT);
	gcAddGObjDisplay(gobj, lbCommonDrawSObjAttr, 0, GOBJ_PRIORITY_DEFAULT, ~0);
	sobj = lbCommonMakeSObjForGObj(gobj, lbRelocGetFileData(Sprite *, sMNVSReplaysFiles[0], llMNCommonSmashBrosCollageSprite));
	sobj->pos.x = 10.0F;
	sobj->pos.y = 10.0F;
	sobj = lbCommonMakeSObjForGObj(gobj, lbRelocGetFileData(Sprite *, sMNVSReplaysFiles[0], llMNCommonDecalPaperSprite));
	sobj->sprite.attr &= ~SP_FASTCOPY;
	sobj->sprite.attr |= SP_TRANSPARENT;
	sobj->sprite.red = 0xA0;
	sobj->sprite.green = 0x78;
	sobj->sprite.blue = 0x14;
	sobj->pos.x = 140.0F;
	sobj->pos.y = 143.0F;
	sobj = lbCommonMakeSObjForGObj(gobj, lbRelocGetFileData(Sprite *, sMNVSReplaysFiles[0], llMNCommonDecalPaperSprite));
	sobj->sprite.attr &= ~SP_FASTCOPY;
	sobj->sprite.attr |= SP_TRANSPARENT;
	sobj->sprite.red = 0xA0;
	sobj->sprite.green = 0x78;
	sobj->sprite.blue = 0x14;
	sobj->pos.x = 225.0F;
	sobj->pos.y = 56.0F;
	sobj = lbCommonMakeSObjForGObj(gobj, lbRelocGetFileData(Sprite *, sMNVSReplaysFiles[1], llMNDataDataIconDarkSprite));
	sobj->sprite.attr &= ~SP_FASTCOPY;
	sobj->sprite.attr |= SP_TRANSPARENT;
	sobj->sprite.red = 0x99;
	sobj->sprite.green = 0x99;
	sobj->sprite.blue = 0x99;
	sobj->pos.x = 10.0F;
	sobj->pos.y = 10.0F;
}

void mnVSReplaysMakeCamera(void)
{
	CObj *cobj = CObjGetStruct(gcMakeCameraGObj(1, NULL, 1, GOBJ_PRIORITY_DEFAULT, lbCommonDrawSprite, 80,
	                                            COBJ_MASK_DLLINK(0), ~0, FALSE, nGCProcessKindFunc, NULL, 1, FALSE));

	syRdpSetViewport(&cobj->viewport, 10.0F, 10.0F, 310.0F, 230.0F);
}

void mnVSReplaysMakeUiCamera(void)
{
	CObj *cobj = CObjGetStruct(gcMakeCameraGObj(1, NULL, 1, GOBJ_PRIORITY_DEFAULT, lbCommonDrawSprite, 40,
	                                            COBJ_MASK_DLLINK(2), ~0, FALSE, nGCProcessKindFunc, NULL, 1, FALSE));

	syRdpSetViewport(&cobj->viewport, 10.0F, 10.0F, 310.0F, 230.0F);
}

void mnVSReplaysFuncLights(Gfx **dls)
{
	gSPDisplayList(dls[0]++, dMNVSReplaysDisplayList);
}

static void mnVSReplaysStartPlayback(void)
{
	char path[SYNETREPLAY_USER_PATH_MAX];

	if (syNetReplayResolveUserFilePath(sMNVSReplaysFileNames[sMNVSReplaysCursorIndex], path, sizeof(path)) == FALSE)
	{
		return;
	}
	if (syNetReplayBeginUserPlayback(path) == FALSE)
	{
		return;
	}
	gSCManagerSceneData.scene_prev = gSCManagerSceneData.scene_curr;
	gSCManagerSceneData.scene_curr = nSCKindVSBattle;
	syTaskmanSetLoadScene();
}

void mnVSReplaysFuncRun(GObj *gobj)
{
	s32 stick_range;
	s32 is_button;

	(void)gobj;

	if (sMNVSReplaysConfirmActive != FALSE)
	{
		mnVSReplaysUpdateConfirmMenu();
		return;
	}
	if (scSubsysControllerGetPlayerTapButtons(B_BUTTON) != FALSE)
	{
		if (sMNVSReplaysFocus == nMNVSReplaysFocusActions)
		{
			sMNVSReplaysFocus = nMNVSReplaysFocusList;
			mnVSReplaysRefreshListUi();
			func_800269C0_275C0(nSYAudioFGMMenuScroll2);
			return;
		}
		if (sMNVSReplaysListEntered != FALSE)
		{
			sMNVSReplaysListEntered = FALSE;
			mnVSReplaysRefreshListUi();
			func_800269C0_275C0(nSYAudioFGMMenuScroll2);
			return;
		}
		if (sMNVSReplaysFocus == nMNVSReplaysFocusList)
		{
			sMNVSReplaysFocus = nMNVSReplaysFocusToggle;
			mnVSReplaysMakeTopRow();
			mnVSReplaysRefreshListUi();
			func_800269C0_275C0(nSYAudioFGMMenuScroll2);
			return;
		}
		gSCManagerSceneData.scene_prev = gSCManagerSceneData.scene_curr;
		gSCManagerSceneData.scene_curr = nSCKindVSMode;
		syTaskmanSetLoadScene();
		return;
	}
	if (sMNVSReplaysChangeWait != 0)
	{
		sMNVSReplaysChangeWait--;
	}
	if (sMNVSReplaysScrollWait != 0)
	{
		sMNVSReplaysScrollWait--;
	}
	if (scSubsysControllerGetPlayerTapButtons(A_BUTTON | START_BUTTON) != FALSE)
	{
		if (sMNVSReplaysFocus == nMNVSReplaysFocusActions)
		{
			func_800269C0_275C0(nSYAudioFGMMenuSelect);
			if (sMNVSReplaysActionChoice == nMNVSReplaysActionPlayback)
			{
				mnVSReplaysStartPlayback();
			}
			else
			{
				mnVSReplaysDeleteSelectedReplay();
				sMNVSReplaysFocus = nMNVSReplaysFocusList;
				if (sMNVSReplaysFileCount <= 0)
				{
					sMNVSReplaysListEntered = FALSE;
				}
				mnVSReplaysRefreshListUi();
			}
		}
		else if (sMNVSReplaysFocus == nMNVSReplaysFocusToggle)
		{
			sMNVSReplaysSaveEnabled = (sMNVSReplaysSaveEnabled == FALSE) ? TRUE : FALSE;
			syNetplaySaveWriteReplaySaveEnabled(sMNVSReplaysSaveEnabled);
			mnVSReplaysMakeTopRow();
			func_800269C0_275C0(nSYAudioFGMMenuSelect);
		}
		else if (sMNVSReplaysFocus == nMNVSReplaysFocusDeleteAll)
		{
			if (sMNVSReplaysFileCount > 0)
			{
				func_800269C0_275C0(nSYAudioFGMMenuSelect);
				sMNVSReplaysConfirmActive = TRUE;
				sMNVSReplaysConfirmYesOrNo = 1;
				sMNVSReplaysChangeWait = 0;
				mnVSReplaysMakeConfirm();
			}
		}
		else if (sMNVSReplaysListEntered == FALSE)
		{
			if (sMNVSReplaysFileCount > 0)
			{
				sMNVSReplaysListEntered = TRUE;
				mnVSReplaysRefreshListUi();
				func_800269C0_275C0(nSYAudioFGMMenuSelect);
			}
		}
		else if ((sMNVSReplaysFileCount > 0) && (sMNVSReplaysCursorIndex >= 0) &&
		         (sMNVSReplaysCursorIndex < sMNVSReplaysFileCount))
		{
			sMNVSReplaysFocus = nMNVSReplaysFocusActions;
			sMNVSReplaysActionChoice = nMNVSReplaysActionPlayback;
			mnVSReplaysRefreshListUi();
			func_800269C0_275C0(nSYAudioFGMMenuSelect);
		}
		return;
	}
	if (mnVSReplaysIsTopRowFocus() != FALSE)
	{
		if (mnVSReplaysCheckGetOptionButtonInput(is_button, R_JPAD | R_CBUTTONS) ||
		    mnVSReplaysCheckGetOptionStickInputLR(stick_range, 20, 1))
		{
			if (sMNVSReplaysFocus == nMNVSReplaysFocusToggle)
			{
				mnVSReplaysSetOptionChangeWaitN(is_button, stick_range, 7);
				func_800269C0_275C0(nSYAudioFGMMenuScroll2);
				sMNVSReplaysFocus = nMNVSReplaysFocusDeleteAll;
				mnVSReplaysMakeTopRow();
			}
			return;
		}
		if (mnVSReplaysCheckGetOptionButtonInput(is_button, L_JPAD | L_CBUTTONS) ||
		    mnVSReplaysCheckGetOptionStickInputLR(stick_range, -20, 0))
		{
			if (sMNVSReplaysFocus == nMNVSReplaysFocusDeleteAll)
			{
				mnVSReplaysSetOptionChangeWaitP(is_button, stick_range, 7);
				func_800269C0_275C0(nSYAudioFGMMenuScroll2);
				sMNVSReplaysFocus = nMNVSReplaysFocusToggle;
				mnVSReplaysMakeTopRow();
			}
			return;
		}
	}
	else if (scSubsysControllerGetPlayerTapButtons(L_JPAD | L_CBUTTONS | R_JPAD | R_CBUTTONS) != FALSE)
	{
		if (sMNVSReplaysFocus == nMNVSReplaysFocusActions)
		{
			sMNVSReplaysActionChoice =
			    (sMNVSReplaysActionChoice == nMNVSReplaysActionPlayback) ? nMNVSReplaysActionDelete :
			                                                                 nMNVSReplaysActionPlayback;
			mnVSReplaysMakeActions();
			func_800269C0_275C0(nSYAudioFGMMenuScroll2);
		}
		return;
	}
	if (mnVSReplaysCheckGetOptionButtonInput(is_button, U_JPAD | U_CBUTTONS) ||
	    mnVSReplaysCheckGetOptionStickInputUD(stick_range, 20, 1))
	{
		mnVSReplaysSetOptionChangeWaitP(is_button, stick_range, 7);
		func_800269C0_275C0(nSYAudioFGMMenuScroll2);
		if (sMNVSReplaysFocus == nMNVSReplaysFocusActions)
		{
			if (sMNVSReplaysActionChoice != nMNVSReplaysActionPlayback)
			{
				sMNVSReplaysActionChoice = nMNVSReplaysActionPlayback;
				mnVSReplaysMakeActions();
			}
		}
		else if (sMNVSReplaysListEntered != FALSE)
		{
			if (sMNVSReplaysCursorIndex > 0)
			{
				sMNVSReplaysCursorIndex--;
				if (sMNVSReplaysCursorIndex < sMNVSReplaysScrollOffset)
				{
					sMNVSReplaysScrollOffset = sMNVSReplaysCursorIndex;
				}
				(void)mnVSReplaysRefreshHighlight();
				mnVSReplaysRefreshListUi();
			}
		}
		else if (sMNVSReplaysFocus == nMNVSReplaysFocusList)
		{
			sMNVSReplaysFocus = nMNVSReplaysFocusToggle;
			mnVSReplaysMakeTopRow();
			mnVSReplaysRefreshListUi();
		}
		return;
	}
	if (mnVSReplaysCheckGetOptionButtonInput(is_button, D_JPAD | D_CBUTTONS) ||
	    mnVSReplaysCheckGetOptionStickInputUD(stick_range, -20, 0))
	{
		if (sMNVSReplaysFocus == nMNVSReplaysFocusActions)
		{
			mnVSReplaysSetOptionChangeWaitN(is_button, stick_range, 7);
			func_800269C0_275C0(nSYAudioFGMMenuScroll2);
			if (sMNVSReplaysActionChoice != nMNVSReplaysActionDelete)
			{
				sMNVSReplaysActionChoice = nMNVSReplaysActionDelete;
				mnVSReplaysMakeActions();
			}
		}
		else if (sMNVSReplaysListEntered != FALSE)
		{
			mnVSReplaysSetOptionChangeWaitN(is_button, stick_range, 7);
			func_800269C0_275C0(nSYAudioFGMMenuScroll2);
			if (sMNVSReplaysCursorIndex < (sMNVSReplaysFileCount - 1))
			{
				sMNVSReplaysCursorIndex++;
				if (sMNVSReplaysCursorIndex >= (sMNVSReplaysScrollOffset + MN_VSREPLAYS_VISIBLE_ROWS))
				{
					sMNVSReplaysScrollOffset = sMNVSReplaysCursorIndex - MN_VSREPLAYS_VISIBLE_ROWS + 1;
				}
				(void)mnVSReplaysRefreshHighlight();
				mnVSReplaysRefreshListUi();
			}
		}
		else if (mnVSReplaysIsTopRowFocus() != FALSE)
		{
			mnVSReplaysSetOptionChangeWaitN(is_button, stick_range, 7);
			func_800269C0_275C0(nSYAudioFGMMenuScroll2);
			sMNVSReplaysFocus = nMNVSReplaysFocusList;
			mnVSReplaysMakeTopRow();
			mnVSReplaysRefreshListUi();
		}
		return;
	}
}

void mnVSReplaysFuncStart(void)
{
	LBRelocSetup rl_setup;
	s32 file_index;

	sMNVSReplaysListGObj = NULL;
	sMNVSReplaysMetaGObj = NULL;
	sMNVSReplaysToggleGObj = NULL;
	sMNVSReplaysActionsGObj = NULL;
	sMNVSReplaysConfirmGObj = NULL;
	for (file_index = 0; file_index < (s32)ARRAY_COUNT(sMNVSReplaysFiles); file_index++)
	{
		sMNVSReplaysFiles[file_index] = NULL;
	}

	sMNVSReplaysSaveEnabled = syNetplaySaveGetReplaySaveEnabled();
	sMNVSReplaysFocus = nMNVSReplaysFocusToggle;
	sMNVSReplaysActionChoice = nMNVSReplaysActionPlayback;
	sMNVSReplaysListEntered = FALSE;
	sMNVSReplaysConfirmActive = FALSE;
	sMNVSReplaysConfirmYesOrNo = 1;
	sMNVSReplaysCursorIndex = 0;
	sMNVSReplaysScrollOffset = 0;
	sMNVSReplaysChangeWait = 0;
	sMNVSReplaysScrollWait = 0;
	sMNVSReplaysHighlightValid = FALSE;

	rl_setup.table_addr = (uintptr_t)&lLBRelocTableAddr;
	rl_setup.table_files_num = (u32)llRelocFileCount;
	rl_setup.file_heap = NULL;
	rl_setup.file_heap_size = 0;
	rl_setup.status_buffer = sMNVSReplaysStatusBuffer;
	rl_setup.status_buffer_size = ARRAY_COUNT(sMNVSReplaysStatusBuffer);
	rl_setup.force_status_buffer = NULL;
	rl_setup.force_status_buffer_size = 0;

	lbRelocInitSetup(&rl_setup);
	lbRelocLoadFilesListed(dMNVSReplaysFileIDs, sMNVSReplaysFiles);
	gcMakeGObjSPAfter(0, mnVSReplaysFuncRun, 0, GOBJ_PRIORITY_DEFAULT);
	gcMakeDefaultCameraGObj(0, GOBJ_PRIORITY_DEFAULT, 100, 0, GPACK_RGBA8888(0x00, 0x00, 0x00, 0x00));
	mnVSReplaysReloadFileList();
	mnVSReplaysMakeCamera();
	mnVSReplaysMakeUiCamera();
	mnVSReplaysMakeDecals();
	mnVSReplaysMakeTopRow();
	mnVSReplaysMakeList();
	mnVSReplaysMakeMetadata();
	if (gSCManagerSceneData.scene_prev != nSCKindVSBattle)
	{
		syAudioPlayBGM(0, nSYAudioBGMModeSelect);
	}
}

SYVideoSetup dMNVSReplaysVideoSetup = SYVIDEO_SETUP_DEFAULT();

SYTaskmanSetup dMNVSReplaysTaskmanSetup =
{
	{
		0,
		gcRunAll,
		gcDrawAll,
		&ovl30_BSS_END,
		0,
		1,
		2,
		sizeof(Gfx) * 7500,
		0,
		0,
		0,
		0x8000,
		2,
		0xC000,
		mnVSReplaysFuncLights,
		syControllerFuncRead,
	},
	0,
	sizeof(u64) * 192,
	0,
	0,
	0,
	0,
	sizeof(GObj),
	0,
	NULL,
	NULL,
	0,
	0,
	0,
	sizeof(DObj),
	0,
	sizeof(SObj),
	0,
	sizeof(CObj),
	mnVSReplaysFuncStart
};

void mnVSReplaysStartScene(void)
{
	dMNVSReplaysVideoSetup.zbuffer = SYVIDEO_ZBUFFER_START(320, 240, 0, 10, u16);
	syVideoInit(&dMNVSReplaysVideoSetup);

	dMNVSReplaysTaskmanSetup.scene_setup.arena_size = (size_t)((uintptr_t)&ovl1_VRAM - (uintptr_t)&ovl30_BSS_END);
	syTaskmanStartTask(&dMNVSReplaysTaskmanSetup);
}
