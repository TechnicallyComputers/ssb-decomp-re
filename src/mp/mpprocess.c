#include <sys/obj.h>
#include <mp/map.h>

#if defined(PORT) && defined(SSB64_NETMENU)
#include <stdlib.h>
#include <string.h>
#include <ft/fighter.h>
#include <ft/ftstatusvars.h>
#include <wp/wptypes.h>
#include <sys/netplay_sim_quantize.h>
extern void port_log(const char *fmt, ...);

/* Set for the duration of mpProcessUpdateMain so AdjNew suppress can scope by fighter. */
static GObj *sMPProcessNetplayCollGObj;

/*
 * SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only.
 * Soft-lip residual (PASS|CLIFF floor_flags): under-edge walls still register as
 * direct L/R AdjNew line hits even after wall-from-floor treats CLIFF like PASS.
 * Cross-ISA Diff float arms that path on one peer only
 * (soak1 1324417498 gut=516 DamageFly fline=-1 — Y matched, TopN.x Δ→61.6;
 *  soak1 262102584 DamageFall gut=5314+ after SetCollProjectFloorID left a
 *  projected CLIFF fline≠-1 — Y matched, TopN.x +2u/frame → Δ15 lock → FC@5389).
 * Do not require floor_line_id==-1: Damage/air paths call SetCollProjectFloorID
 * after the floor miss, so the next wall CheckTest often sees a real projected
 * CLIFF/PASS line id with the same soft-lip flags.
 *
 * Ceil AdjNew edge paths also snap translate->x onto under-edge L/R walls
 * (RunCeilCollisionAdjNew + RunCeilEdgeAdjust). Wall CheckTest suppress alone
 * left soak1 1627652882 DamageFall@3344–3357 Y/fdist matched, TopN.x +0.04/frame
 * → PEER_SNAPSHOT_DIVERGE@3373. Same gate skips those X snaps. AdjNew is
 * air/damage-only; CliffCatch uses CheckTestL/RCliffCollision.
 *
 * Ness PK jibaku/bound need mask_curr wall/ceil for procmap bounce and ledge
 * logic — blanket suppress caused clip-through (soak1 @422–484). For jibaku on
 * soft-lip: keep wall/ceil *detection*, quantize translate snaps instead of raw
 * float (soak1 122093103 FC@1419 jibaku landing topn_tx/ty inputs MATCH).
 * See docs/bugs/netplay_airborne_cliff_lip_jibaku_fc_drift_2026-07-18.md.
 *
 * Non-jibaku JumpAerial on Dream Land CLIFF lip (soak1 1828471508 @2918): Y matched
 * while TopN.x forked +2u vs −1u with fflags=CLIFF fline=-1, then locked through
 * SpecialHiHold → PEER_SNAPSHOT_DIVERGE@3157. Residual coll_data flags alone are not
 * enough when the wall-from-floor sweep sees PASS|CLIFF but residual was cleared —
 * OR swept soft-lip flags into suppress; also skip FloorEdge / landing-edge X snaps
 * (CeilEdge already gated). Quantize *any* soft-lip AdjNew translate snap (not only
 * jibaku) so residual paths that still write TopN cannot keep raw cross-ISA floats.
 * See docs/bugs/netplay_airborne_cliff_lip_jumpaerial_fc_drift_2026-07-18.md.
 *
 * Sticky soft-lip (soak1 1775005817 @514): walls run *before* floor CheckTest, so
 * SoftLipEx reads residual floor_flags from the prior tick's project. Cross-ISA
 * project can leave one peer without PASS|CLIFF residual while MpLanding still
 * logs CLIFF after this tick's floor sweep — Linux then keeps AdjNew wall (X
 * stalls ~JumpAerial vel) while Android free-flies (~−27u/frame) with Y matched.
 * Latch PASS|CLIFF when observed; clear on grounded FLOOR *or* sticky TTL expiry.
 * SoftLipEx ORs the latch so suppress survives residual clear. See
 * docs/bugs/netplay_airborne_cliff_lip_jumpaerial_sticky_softlip_2026-07-19.md.
 *
 * Under-stage clip (soak1 seed 2412131430 gut≈671–747): SetCollProjectFloorID
 * fail left stale PASS floor_flags; floor StickyNote + SoftLipEx re-latched every
 * frame so suppress stayed armed under the stage body (rwall_suppress @732–737)
 * and JumpAerial rose through the underside onto fline=3. On project fail, clear
 * PASS|CLIFF residual; only Note sticky from a live soft floor (fline!=-1) or a
 * successful PASS|CLIFF project; expire sticky after SOFTLIP_STICKY_TTL UpdateMains
 * without a refresh. See docs/bugs/netplay_softlip_understage_wall_passthrough_2026-07-28.md.
 */
static u32 sMPProcessNetplaySoftLipStickyFlags[GMCOMMON_PLAYERS_MAX];
/* Frames of sticky remaining; StickyNote refreshes. 0 => sticky inert. */
static u8 sMPProcessNetplaySoftLipStickyTtl[GMCOMMON_PLAYERS_MAX];
#define MPPROCESS_NETPLAY_SOFTLIP_STICKY_TTL 24

static sb32 mpProcessNetplaySoftLipFloorFlags(u32 floor_flags)
{
	return ((floor_flags & (MAP_VERTEX_COLL_PASS | MAP_VERTEX_COLL_CLIFF)) != 0U) ? TRUE : FALSE;
}

static sb32 mpProcessNetplayCollGObjIsFighter(GObj *gobj)
{
	return ((gobj != NULL) && (gobj->link_id == nGCCommonLinkIDFighter)) ? TRUE : FALSE;
}

static s32 mpProcessNetplaySoftLipPlayerIndex(void)
{
	FTStruct *fp;

	/*
	 * Sticky is per-fighter. Weapon/item UpdateMain also sets CollGObj — do not
	 * ftGetStruct a WPStruct (soak 128512323: PK Thunder head CLIFF map).
	 */
	if (mpProcessNetplayCollGObjIsFighter(sMPProcessNetplayCollGObj) == FALSE)
	{
		return -1;
	}
	fp = ftGetStruct(sMPProcessNetplayCollGObj);
	if ((fp == NULL) || (fp->player < 0) || (fp->player >= GMCOMMON_PLAYERS_MAX))
	{
		return -1;
	}
	return fp->player;
}

/* domain ft|wp|it|?; player port; kind = status_id (ft) or wp->kind / -1 */
static void mpProcessNetplayCollIdentity(GObj *gobj, const char **domain_out, s32 *player_out,
					 s32 *kind_out)
{
	const char *domain = "?";
	s32 player = -1;
	s32 kind = -1;

	if (gobj == NULL)
	{
		gobj = sMPProcessNetplayCollGObj;
	}
	if (gobj != NULL)
	{
		if (gobj->link_id == nGCCommonLinkIDFighter)
		{
			FTStruct *fp = ftGetStruct(gobj);

			domain = "ft";
			if (fp != NULL)
			{
				player = (s32)fp->player;
				kind = (s32)fp->status_id;
			}
		}
		else if (gobj->link_id == nGCCommonLinkIDWeapon)
		{
			WPStruct *wp = (WPStruct *)gobj->user_data.p;

			domain = "wp";
			if (wp != NULL)
			{
				player = (s32)wp->player;
				kind = (s32)wp->kind;
			}
		}
		else if (gobj->link_id == nGCCommonLinkIDItem)
		{
			domain = "it";
		}
	}
	if (domain_out != NULL)
	{
		*domain_out = domain;
	}
	if (player_out != NULL)
	{
		*player_out = player;
	}
	if (kind_out != NULL)
	{
		*kind_out = kind;
	}
}

static void mpProcessNetplaySoftLipStickyClearPlayer(s32 player)
{
	if ((player < 0) || (player >= GMCOMMON_PLAYERS_MAX))
	{
		return;
	}
	sMPProcessNetplaySoftLipStickyFlags[player] = 0U;
	sMPProcessNetplaySoftLipStickyTtl[player] = 0U;
}

static void mpProcessNetplaySoftLipStickyNote(u32 floor_flags)
{
	s32 player;

	if (syNetplayRollbackSemanticsActive() == FALSE)
	{
		return;
	}
	if (mpProcessNetplaySoftLipFloorFlags(floor_flags) == FALSE)
	{
		return;
	}
	player = mpProcessNetplaySoftLipPlayerIndex();
	if (player < 0)
	{
		return;
	}
	sMPProcessNetplaySoftLipStickyFlags[player] =
	    floor_flags & (MAP_VERTEX_COLL_PASS | MAP_VERTEX_COLL_CLIFF);
	sMPProcessNetplaySoftLipStickyTtl[player] = (u8)MPPROCESS_NETPLAY_SOFTLIP_STICKY_TTL;
}

static void mpProcessNetplaySoftLipStickyTick(void)
{
	s32 player;

	if (syNetplayRollbackSemanticsActive() == FALSE)
	{
		return;
	}
	player = mpProcessNetplaySoftLipPlayerIndex();
	if (player < 0)
	{
		return;
	}
	if (sMPProcessNetplaySoftLipStickyTtl[player] == 0U)
	{
		sMPProcessNetplaySoftLipStickyFlags[player] = 0U;
		return;
	}
	sMPProcessNetplaySoftLipStickyTtl[player]--;
	if (sMPProcessNetplaySoftLipStickyTtl[player] == 0U)
	{
		sMPProcessNetplaySoftLipStickyFlags[player] = 0U;
	}
}

static void mpProcessNetplaySoftLipStickyClearIfGrounded(MPCollData *coll_data)
{
	s32 player;

	if ((coll_data == NULL) || (syNetplayRollbackSemanticsActive() == FALSE))
	{
		return;
	}
	if ((coll_data->mask_stat & MAP_FLAG_FLOOR) == 0U)
	{
		return;
	}
	/*
	 * Soak 1623281430 @2058: JumpAerial over CLIFF matched 20+ frames then Linux
	 * AdjNew-clamped X while Android free-flew. Cross-ISA can briefly set FLOOR in
	 * mask_stat on the lip while floor_flags are still PASS|CLIFF — clearing sticky
	 * then leaves the next tick's wall CheckTest without residual or latch.
	 * Keep the latch while the project still reports soft-lip floor flags.
	 */
	if (mpProcessNetplaySoftLipFloorFlags(coll_data->floor_flags) != FALSE)
	{
		return;
	}
	player = mpProcessNetplaySoftLipPlayerIndex();
	if (player < 0)
	{
		return;
	}
	mpProcessNetplaySoftLipStickyClearPlayer(player);
}

static sb32 mpProcessNetplaySoftLipStickyActive(void)
{
	s32 player;

	if (syNetplayRollbackSemanticsActive() == FALSE)
	{
		return FALSE;
	}
	player = mpProcessNetplaySoftLipPlayerIndex();
	if (player < 0)
	{
		return FALSE;
	}
	if ((sMPProcessNetplaySoftLipStickyFlags[player] == 0U) ||
	    (sMPProcessNetplaySoftLipStickyTtl[player] == 0U))
	{
		return FALSE;
	}
	return TRUE;
}

u32 mpProcessNetplaySoftLipStickyGet(s32 player)
{
	if ((player < 0) || (player >= GMCOMMON_PLAYERS_MAX))
	{
		return 0U;
	}
	return sMPProcessNetplaySoftLipStickyFlags[player];
}

void mpProcessNetplaySoftLipStickySet(s32 player, u32 flags)
{
	if ((player < 0) || (player >= GMCOMMON_PLAYERS_MAX))
	{
		return;
	}
	/*
	 * Rollback blob restore: sticky must match the captured tick so the first
	 * wall CheckTest after load (before this tick's floor Note) suppresses the
	 * same way on both peers. Soak 1328818035: emergency_restore@992 matched
	 * TopN through 993 then forked X@994 (Linux AdjNew +15.7, Android −3.4).
	 * See docs/bugs/netplay_airborne_cliff_lip_jumpaerial_softlip_snapshot_2026-07-19.md.
	 * Full TTL after load so the restored latch survives the first post-load
	 * UpdateMain ticks the same way on both peers (TTL itself is not snapshotted).
	 */
	sMPProcessNetplaySoftLipStickyFlags[player] =
	    flags & (MAP_VERTEX_COLL_PASS | MAP_VERTEX_COLL_CLIFF);
	sMPProcessNetplaySoftLipStickyTtl[player] =
	    (sMPProcessNetplaySoftLipStickyFlags[player] != 0U)
		? (u8)MPPROCESS_NETPLAY_SOFTLIP_STICKY_TTL
		: 0U;
}

static sb32 mpProcessNetplayUnattachedSoftLipActive(MPCollData *coll_data)
{
	if (coll_data == NULL)
	{
		return FALSE;
	}
	/*
	 * Residual PASS|CLIFF only counts with a live floor line. Stale bits with
	 * fline==-1 (project miss) are not a soft lip — sticky TTL covers the
	 * wall-before-floor / brief lip window instead.
	 */
	if ((coll_data->floor_line_id != -1) &&
	    (mpProcessNetplaySoftLipFloorFlags(coll_data->floor_flags) != FALSE))
	{
		return TRUE;
	}
	return mpProcessNetplaySoftLipStickyActive();
}

static sb32 mpProcessNetplayJibakuSoftLipHardenSnaps(MPCollData *coll_data)
{
	FTStruct *fp;

	if ((coll_data == NULL) || (syNetplayRollbackSemanticsActive() == FALSE))
	{
		return FALSE;
	}
	if (mpProcessNetplayUnattachedSoftLipActive(coll_data) == FALSE)
	{
		return FALSE;
	}
	if (sMPProcessNetplayCollGObj == NULL)
	{
		return FALSE;
	}
	fp = ftGetStruct(sMPProcessNetplayCollGObj);
	return ((fp != NULL) && (syNetplayFighterInNessPKJibakuSimScope(fp) != FALSE)) ? TRUE : FALSE;
}

static void mpProcessNetplayHardenAdjNewTranslateSnap(MPCollData *coll_data, Vec3f *translate)
{
	if ((translate == NULL) || (syNetplayRollbackSemanticsActive() == FALSE))
	{
		return;
	}
	if (mpProcessNetplayUnattachedSoftLipActive(coll_data) == FALSE)
	{
		return;
	}
	if (syNetplaySimQuantizeActive() != FALSE)
	{
		translate->x = syNetplayQuantizeF32(translate->x);
		translate->y = syNetplayQuantizeF32(translate->y);
	}
}

static sb32 mpProcessNetplaySuppressAdjNewWallOnUnattachedSoftLip(MPCollData *coll_data)
{
	if ((coll_data == NULL) || (syNetplayRollbackSemanticsActive() == FALSE))
	{
		return FALSE;
	}
	if (mpProcessNetplayJibakuSoftLipHardenSnaps(coll_data) != FALSE)
	{
		return FALSE;
	}
	return mpProcessNetplayUnattachedSoftLipActive(coll_data);
}

/* Suppress when residual, sticky latch, *or* this CheckTest's swept floor carries PASS|CLIFF. */
static sb32 mpProcessNetplaySuppressAdjNewWallSoftLipEx(MPCollData *coll_data, u32 swept_floor_flags)
{
	if ((coll_data == NULL) || (syNetplayRollbackSemanticsActive() == FALSE))
	{
		return FALSE;
	}
	if (mpProcessNetplayJibakuSoftLipHardenSnaps(coll_data) != FALSE)
	{
		return FALSE;
	}
	/*
	 * Live soft floor (fline!=-1): refresh sticky from residual + wall-from-floor sweep.
	 * fline==-1: do not re-latch from stale residual or incidental swept PASS — sticky TTL
	 * from the last live soft floor covers DamageFly/JumpAerial lip; under-stage must keep walls.
	 */
	if (coll_data->floor_line_id != -1)
	{
		mpProcessNetplaySoftLipStickyNote(coll_data->floor_flags);
		mpProcessNetplaySoftLipStickyNote(swept_floor_flags);
	}
	if (mpProcessNetplayUnattachedSoftLipActive(coll_data) != FALSE)
	{
		return TRUE;
	}
	if (coll_data->floor_line_id == -1)
	{
		return FALSE;
	}
	return mpProcessNetplaySoftLipFloorFlags(swept_floor_flags);
}

/*
 * Env SSB64_NETPLAY_SOFTLIP_X_DIAG=1 — log AdjNew TopN.x writers / soft-lip wall suppress so a
 * cross-peer soak can name the first asymmetric path (soak 1410199591: DamageFall CLIFF
 * Δx=+0.2/frame @3752 with mask_unk=0 → not AdjNew wall Run; suspect ceil/edge).
 */
static void mpProcessNetplaySoftLipXDiagEx(const char *path, MPCollData *coll_data, f32 x_before,
					  sb32 suppressed, sb32 force_log)
{
	static int s_softlip_x_diag = -1;
	u32 x_before_bits;
	u32 x_after_bits;
	FTStruct *fp;
	s32 player;
	u32 sticky;

	if (s_softlip_x_diag < 0)
	{
		const char *env = getenv("SSB64_NETPLAY_SOFTLIP_X_DIAG");

		s_softlip_x_diag = ((env != NULL) && (env[0] == '1')) ? 1 : 0;
	}
	if ((s_softlip_x_diag == 0) || (coll_data == NULL) || (coll_data->p_translate == NULL) || (path == NULL))
	{
		return;
	}
	if ((force_log == FALSE) && (suppressed == FALSE) && (coll_data->p_translate->x == x_before))
	{
		return;
	}
	memcpy(&x_before_bits, &x_before, sizeof(x_before_bits));
	memcpy(&x_after_bits, &coll_data->p_translate->x, sizeof(x_after_bits));
	player = mpProcessNetplaySoftLipPlayerIndex();
	sticky = (player >= 0) ? sMPProcessNetplaySoftLipStickyFlags[player] : 0U;
	fp = (sMPProcessNetplayCollGObj != NULL) ? ftGetStruct(sMPProcessNetplayCollGObj) : NULL;
	/*
	 * residual_fflags = coll_data->floor_flags at call site (may already be cleared
	 * mid-CheckTest); sticky = latched PASS|CLIFF for this player. Cross-peer first
	 * path / sticky mismatch names the asymmetric writer (soak 1315107154 / 1410199591).
	 * force_log: wall CheckTest hit but SoftLipEx did not suppress (lwall_keep / rwall_keep).
	 */
	port_log("SSB64 SoftLipX: gut=%u path=%s suppressed=%d x_before=0x%08X x_after=0x%08X "
		 "residual_fflags=0x%08X sticky=0x%08X softlip=%d fline=%d status=%d player=%d\n",
		 (unsigned int)gMPCollisionUpdateTic,
		 path,
		 (int)suppressed,
		 (unsigned int)x_before_bits,
		 (unsigned int)x_after_bits,
		 (unsigned int)coll_data->floor_flags,
		 (unsigned int)sticky,
		 (int)mpProcessNetplayUnattachedSoftLipActive(coll_data),
		 (int)coll_data->floor_line_id,
		 (fp != NULL) ? (int)fp->status_id : -1,
		 (int)player);
}

static void mpProcessNetplaySoftLipXDiag(const char *path, MPCollData *coll_data, f32 x_before,
					 sb32 suppressed)
{
	mpProcessNetplaySoftLipXDiagEx(path, coll_data, x_before, suppressed, FALSE);
}

/*
 * Soak 1747311082 @1474: JumpAerial CLIFF TopN.x forked (+25u) with SoftLipX silent
 * for status=24 — AdjNew wall CheckTest never armed, so lwall_keep/adjnew could not
 * name the writer. Phase probes pin first asymmetric stage (post_phys vs post_lwall…).
 * Same env as SoftLipX (SSB64_NETPLAY_SOFTLIP_X_DIAG=1). Gated to soft-lip / sticky /
 * JumpAerial to limit volume.
 */
void mpProcessNetplaySoftLipPhaseDiag(const char *phase, MPCollData *coll_data, GObj *gobj)
{
	static int s_softlip_x_diag = -1;
	const char *domain;
	u32 topn_bits;
	u32 vel_bits;
	u32 ja_vel_bits;
	u32 ja_drift_bits;
	u32 sticky;
	s32 player;
	s32 kind;
	sb32 softlip;
	f32 ja_vel_x;
	f32 ja_drift;
	f32 vel_x;

	if (s_softlip_x_diag < 0)
	{
		const char *env = getenv("SSB64_NETPLAY_SOFTLIP_X_DIAG");

		s_softlip_x_diag = ((env != NULL) && (env[0] == '1')) ? 1 : 0;
	}
	if ((s_softlip_x_diag == 0) || (phase == NULL) || (coll_data == NULL) ||
	    (coll_data->p_translate == NULL) || (gobj == NULL) ||
	    (syNetplayRollbackSemanticsActive() == FALSE))
	{
		return;
	}
	mpProcessNetplayCollIdentity(gobj, &domain, &player, &kind);
	if ((player < 0) || (player >= GMCOMMON_PLAYERS_MAX))
	{
		return;
	}
	sticky = sMPProcessNetplaySoftLipStickyFlags[player];
	softlip = mpProcessNetplayUnattachedSoftLipActive(coll_data);
	ja_vel_x = 0.0F;
	ja_drift = 0.0F;
	vel_x = 0.0F;
	if (gobj->link_id == nGCCommonLinkIDFighter)
	{
		FTStruct *fp = ftGetStruct(gobj);
		sb32 is_jumpaerial;

		if (fp == NULL)
		{
			return;
		}
		is_jumpaerial = ((fp->status_id == nFTCommonStatusJumpAerialF) ||
				 (fp->status_id == nFTCommonStatusJumpAerialB))
				    ? TRUE
				    : FALSE;
		if ((softlip == FALSE) && (sticky == 0U) && (is_jumpaerial == FALSE) &&
		    (mpProcessNetplaySoftLipFloorFlags(coll_data->floor_flags) == FALSE))
		{
			return;
		}
		vel_x = fp->physics.vel_air.x;
		if (is_jumpaerial != FALSE)
		{
			ja_vel_x = ftStatusVarsJumpAerial(fp)->vel_x;
			ja_drift = ftStatusVarsJumpAerial(fp)->drift;
		}
	}
	else if (gobj->link_id == nGCCommonLinkIDWeapon)
	{
		WPStruct *wp = (WPStruct *)gobj->user_data.p;

		/*
		 * Soak 128512323: PK Thunder head CLIFF map forked while fighter SoftLipPhase
		 * only named parked Hold Ness. Gate on soft-lip / sticky / PASS|CLIFF.
		 */
		if ((softlip == FALSE) && (sticky == 0U) &&
		    (mpProcessNetplaySoftLipFloorFlags(coll_data->floor_flags) == FALSE))
		{
			return;
		}
		if (wp != NULL)
		{
			vel_x = wp->physics.vel_air.x;
		}
	}
	else
	{
		return;
	}
	memcpy(&topn_bits, &coll_data->p_translate->x, sizeof(topn_bits));
	memcpy(&vel_bits, &vel_x, sizeof(vel_bits));
	memcpy(&ja_vel_bits, &ja_vel_x, sizeof(ja_vel_bits));
	memcpy(&ja_drift_bits, &ja_drift, sizeof(ja_drift_bits));
	port_log("SSB64 SoftLipPhase: gut=%u phase=%s domain=%s player=%d status=%d "
		 "topn_x=0x%08X vel_x=0x%08X ja_vel_x=0x%08X ja_drift=0x%08X sticky=0x%08X "
		 "residual_fflags=0x%08X softlip=%d fline=%d mask_curr=0x%04X\n",
		 (unsigned int)gMPCollisionUpdateTic,
		 phase,
		 domain,
		 (int)player,
		 (int)kind,
		 (unsigned int)topn_bits,
		 (unsigned int)vel_bits,
		 (unsigned int)ja_vel_bits,
		 (unsigned int)ja_drift_bits,
		 (unsigned int)sticky,
		 (unsigned int)coll_data->floor_flags,
		 (int)softlip,
		 (int)coll_data->floor_line_id,
		 (unsigned int)coll_data->mask_curr);
}
#endif

// // // // // // // // // // // //
//                               //
//   GLOBAL / STATIC VARIABLES   //
//                               //
// // // // // // // // // // // //

// 0x80130DE0
s32 sMPProcessMultiWallCollidesNum;

// 0x80130DE4
s32 sMPProcessPad0x80130DE4;

// 0x80130DE8 - Simultaneous wall collisions
s32 sMPProcessMultiWallCollideLineIDs[5];

// 0x80130DFC - Position where latest wall collision was detected
f32 sMPProcessLastWallCollidePosition;

// 0x80130E00 - Line ID of latest wall collided with
s32 sMPProcessLastWallLineID;

// 0x80130E04
u32 sMPProcessLastWallFlags;

// 0x80130E08
Vec3f sMPProcessLastWallAngle;

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

// 0x800D9510
void mpProcessResetMultiWallCount(void)
{
    sMPProcessMultiWallCollidesNum = 0;
}

// 0x800D951C
void mpProcessSetMultiWallLineID(s32 line_id)
{
    s32 i;

    for (i = 0; i < sMPProcessMultiWallCollidesNum; i++)
    {
        if (line_id == sMPProcessMultiWallCollideLineIDs[i])
        {
            return;
        }
    }
    if (sMPProcessMultiWallCollidesNum != ARRAY_COUNT(sMPProcessMultiWallCollideLineIDs))
    {
        sMPProcessMultiWallCollideLineIDs[sMPProcessMultiWallCollidesNum] = line_id;
        sMPProcessMultiWallCollidesNum++;
    }
}

// 0x800D957C
void mpProcessSetLastWallCollideLeft(void)
{
    sMPProcessLastWallCollidePosition = -65536.0F;
}

// 0x800D9590
void mpProcessSetLastWallCollideRight(void)
{
    sMPProcessLastWallCollidePosition = 65536.0F;
}

// 0x800D95A4
void mpProcessSetLastWallCollideStats(f32 pos, s32 line_id, u32 flags, Vec3f *angle)
{
    sMPProcessLastWallCollidePosition = pos;
    sMPProcessLastWallLineID          = line_id;
    sMPProcessLastWallFlags           = flags;
    sMPProcessLastWallAngle           = *angle;
}

// 0x800D95E0
void mpProcessGetLastWallCollideStats(f32 *pos, s32 *line_id, u32 *flags, Vec3f *angle)
{
    *pos     = sMPProcessLastWallCollidePosition;
    *line_id = sMPProcessLastWallLineID;
    *flags   = sMPProcessLastWallFlags;
    *angle   = sMPProcessLastWallAngle;
}

// 0x800D9628
sb32 mpProcessCheckCeilEdgeCollisionL(MPCollData *coll_data)
{
    MPObjectColl *map_coll = &coll_data->map_coll;
    Vec3f *translate = coll_data->p_translate;
    Vec3f b;
    Vec3f a;
    s32 sp2C;
    s32 sp28;

    sp28 = mpCollisionGetEdgeUpperRLineID(coll_data->ceil_line_id);

    b.x = translate->x;
    b.y = translate->y + map_coll->top;
    a.x = translate->x + map_coll->width;
    a.y = translate->y + map_coll->center;

    if ((mpCollisionCheckLWallLineCollisionSame(&b, &a, NULL, &sp2C, NULL, NULL) != FALSE) && (sp2C != sp28))
    {
        return TRUE;
    }
    else return FALSE;
}

// 0x800D96D8
void mpProcessCeilEdgeAdjustLeft(MPCollData *coll_data)
{
    MPObjectColl *map_coll = &coll_data->map_coll;
    Vec3f *translate = coll_data->p_translate;
    Vec3f sp3C;
    Vec3f object_pos;
    f32 ceil_dist;

    object_pos.x = translate->x + map_coll->width;
    sp3C.x = object_pos.x;

    object_pos.y = translate->y + map_coll->center;
    sp3C.y = object_pos.y;

    sp3C.x += (2.0F * (coll_data->ceil_angle.y * map_coll->width));
    sp3C.y += (2.0F * (-coll_data->ceil_angle.x * map_coll->width));

    if (mpCollisionCheckLWallLineCollisionSame(&sp3C, &object_pos, &coll_data->line_coll_dist, NULL, NULL, NULL) != FALSE)
    {
        object_pos.x = coll_data->line_coll_dist.x - map_coll->width;
        object_pos.y = translate->y + map_coll->top;

        if (mpCollisionGetFCCommonCeil(coll_data->ceil_line_id, &object_pos, &ceil_dist, &coll_data->ceil_flags, &coll_data->ceil_angle) != FALSE)
        {
#if defined(PORT) && defined(SSB64_NETMENU)
            f32 softlip_x_before = translate->x;
#endif
            translate->y += ceil_dist;
            translate->x = object_pos.x;
#if defined(PORT) && defined(SSB64_NETMENU)
            mpProcessNetplayHardenAdjNewTranslateSnap(coll_data, translate);
            mpProcessNetplaySoftLipXDiag("ceil_edge_l", coll_data, softlip_x_before, FALSE);
#endif
        }
    }
}

// 0x800D97F0
sb32 mpProcessCheckCeilEdgeCollisionR(MPCollData *coll_data)
{
    MPObjectColl *map_coll = &coll_data->map_coll;
    Vec3f *translate = coll_data->p_translate;
    Vec3f b;
    Vec3f a;
    s32 sp2C;
    s32 sp28;

    sp28 = mpCollisionGetEdgeUpperLLineID(coll_data->ceil_line_id);

    b.x = translate->x;
    b.y = translate->y + map_coll->top;
    a.x = translate->x - map_coll->width;
    a.y = translate->y + map_coll->center;

    if ((mpCollisionCheckRWallLineCollisionSame(&b, &a, NULL, &sp2C, NULL, NULL) != FALSE) && (sp2C != sp28))
    {
        return TRUE;
    }
    else return FALSE;
}

// 0x800D98A0
void mpProcessCeilEdgeAdjustRight(MPCollData *coll_data)
{
    MPObjectColl *map_coll = &coll_data->map_coll;
    Vec3f *translate = coll_data->p_translate;
    Vec3f sp3C;
    Vec3f object_pos;
    f32 ceil_dist;

    object_pos.x = translate->x - map_coll->width;
    sp3C.x = object_pos.x;

    object_pos.y = translate->y + map_coll->center;
    sp3C.y = object_pos.y;

    sp3C.x += (2.0F * (-coll_data->ceil_angle.y * map_coll->width));
    sp3C.y += (2.0F * (coll_data->ceil_angle.x * map_coll->width));

    if (mpCollisionCheckRWallLineCollisionSame(&sp3C, &object_pos, &coll_data->line_coll_dist, NULL, NULL, NULL) != 0)
    {
        object_pos.x = coll_data->line_coll_dist.x + map_coll->width;
        object_pos.y = translate->y + map_coll->top;

        if (mpCollisionGetFCCommonCeil(coll_data->ceil_line_id, &object_pos, &ceil_dist, &coll_data->ceil_flags, &coll_data->ceil_angle) != FALSE)
        {
#if defined(PORT) && defined(SSB64_NETMENU)
            f32 softlip_x_before = translate->x;
#endif
            translate->y += ceil_dist;
            translate->x = object_pos.x;
#if defined(PORT) && defined(SSB64_NETMENU)
            mpProcessNetplayHardenAdjNewTranslateSnap(coll_data, translate);
            mpProcessNetplaySoftLipXDiag("ceil_edge_r", coll_data, softlip_x_before, FALSE);
#endif
        }
    }
}

// 0x800D99B8
void mpProcessRunCeilEdgeAdjust(MPCollData *coll_data)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    /*
     * SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only.
     * Soft-lip: CeilEdgeAdjust L/R walk under-edge walls and snap TopN.x — same
     * cross-ISA class as AdjNew direct wall. Skip entirely (Y already settled).
     */
    if (mpProcessNetplaySuppressAdjNewWallOnUnattachedSoftLip(coll_data) != FALSE)
    {
        mpProcessNetplaySoftLipXDiag("ceil_edge_skip", coll_data, coll_data->p_translate->x, TRUE);
        return;
    }
#endif
    if (mpProcessCheckCeilEdgeCollisionL(coll_data) != FALSE)
    {
        mpProcessCeilEdgeAdjustLeft(coll_data);
    }
    if (mpProcessCheckCeilEdgeCollisionR(coll_data) != FALSE)
    {
        mpProcessCeilEdgeAdjustRight(coll_data);
    }
}

// 0x800D9A00
sb32 mpProcessCheckFloorEdgeCollisionL(MPCollData *coll_data)
{
    MPObjectColl *map_coll = &coll_data->map_coll;
    Vec3f *translate = coll_data->p_translate;
    Vec3f b;
    Vec3f a;
    s32 sp2C = mpCollisionGetEdgeUnderLLineID(coll_data->floor_line_id);

    b.x = translate->x;
    b.y = translate->y + map_coll->bottom;
    a.x = translate->x + map_coll->width;
    a.y = translate->y + map_coll->center;

    if ((mpCollisionCheckLWallLineCollisionSame(&b, &a, NULL, &coll_data->ewall_line_id, NULL, NULL) != FALSE) && (sp2C != coll_data->ewall_line_id))
    {
        return TRUE;
    }
    else return FALSE;
}

// 0x800D9AB0
void mpProcessFloorEdgeLAdjust(MPCollData *coll_data)
{
    MPObjectColl *map_coll = &coll_data->map_coll;
    Vec3f *translate = coll_data->p_translate;
    Vec3f sp44;
    Vec3f sp38;
    f32 sp34;

    sp38.x = translate->x + map_coll->width;
    sp38.y = translate->y + map_coll->center;

    if (mpCollisionGetLRCommonLWall(coll_data->ewall_line_id, &sp38, NULL, NULL, NULL) != FALSE)
    {
        sp38.x = translate->x + map_coll->width;
        sp44.x = sp38.x;
        sp38.y = translate->y + map_coll->center;
        sp44.y = sp38.y;

        sp44.x += (2.0F * (-coll_data->floor_angle.y * map_coll->width));
        sp44.y += (2.0F * (coll_data->floor_angle.x * map_coll->width));

        if (mpCollisionCheckLWallLineCollisionSame(&sp44, &sp38, &coll_data->line_coll_dist, NULL, NULL, NULL) != FALSE)
        {
            sp38.x = coll_data->line_coll_dist.x - map_coll->width;
            sp38.y = translate->y + map_coll->bottom;

            if (mpCollisionGetFCCommonFloor(coll_data->floor_line_id, &sp38, &sp34, &coll_data->floor_flags, &coll_data->floor_angle) != FALSE)
            {
                translate->y += sp34;
                translate->x = sp38.x;
            }
        }
    }
    else
    {
        mpCollisionGetLWallEdgeU(coll_data->ewall_line_id, &sp44);

        sp44.x -= 2.0F;
        sp38.x = sp44.x - (2.0F * map_coll->width);
        sp38.y = sp44.y - (2.0F * (map_coll->center - map_coll->bottom));

        if (mpCollisionCheckFloorLineCollisionSame(&sp44, &sp38, &coll_data->line_coll_dist, NULL, NULL, NULL) != FALSE)
        {
            sp38.x = coll_data->line_coll_dist.x;
            sp38.y = translate->y;

            if (mpCollisionGetFCCommonFloor(coll_data->floor_line_id, &sp38, &sp34, &coll_data->floor_flags, &coll_data->floor_angle) != FALSE)
            {
                translate->y += sp34;
                translate->x = sp38.x;
            }
        }
    }
}

// 0x800D9CC0
sb32 mpProcessCheckFloorEdgeCollisionR(MPCollData *coll_data)
{
    MPObjectColl *map_coll = &coll_data->map_coll;
    Vec3f *translate = coll_data->p_translate;
    Vec3f b;
    Vec3f a;
    s32 sp2C = mpCollisionGetEdgeUnderRLineID(coll_data->floor_line_id);

    b.x = translate->x;
    b.y = translate->y + map_coll->bottom;
    a.x = translate->x - map_coll->width;
    a.y = translate->y + map_coll->center;

    if ((mpCollisionCheckRWallLineCollisionSame(&b, &a, NULL, &coll_data->ewall_line_id, NULL, NULL) != FALSE) && (sp2C != coll_data->ewall_line_id))
    {
        return TRUE;
    }
    else return FALSE;
}

// 0x800D9D70
void mpProcessFloorEdgeRAdjust(MPCollData *coll_data)
{
    MPObjectColl *map_coll = &coll_data->map_coll;
    Vec3f *translate = coll_data->p_translate;
    Vec3f sp44;
    Vec3f sp38;
    f32 floor_dist;

    sp38.x = translate->x - map_coll->width;
    sp38.y = translate->y + map_coll->center;

    if (mpCollisionGetLRCommonRWall(coll_data->ewall_line_id, &sp38, NULL, NULL, NULL) != FALSE)
    {
        sp38.x = translate->x - map_coll->width;
        sp44.x = sp38.x;
        sp38.y = translate->y + map_coll->center;
        sp44.y = sp38.y;

        sp44.x += (2.0F * (coll_data->floor_angle.y * map_coll->width));
        sp44.y += (2.0F * (-coll_data->floor_angle.x * map_coll->width));

        if (mpCollisionCheckRWallLineCollisionSame(&sp44, &sp38, &coll_data->line_coll_dist, NULL, NULL, NULL) != FALSE)
        {
            sp38.x = coll_data->line_coll_dist.x + map_coll->width;
            sp38.y = translate->y + map_coll->bottom;

            if (mpCollisionGetFCCommonFloor(coll_data->floor_line_id, &sp38, &floor_dist, &coll_data->floor_flags, &coll_data->floor_angle) != FALSE)
            {
                translate->y += floor_dist;
                translate->x = sp38.x;
            }
        }
    }
    else
    {
        mpCollisionGetRWallEdgeU(coll_data->ewall_line_id, &sp44);

        sp44.x += 2.0F;
        sp44.x = sp38.x + (2.0F * map_coll->width);
        sp44.y = sp38.y - (2.0F * (map_coll->center - map_coll->bottom));

        if (mpCollisionCheckFloorLineCollisionSame(&sp44, &sp38, &coll_data->line_coll_dist, NULL, NULL, NULL) != FALSE)
        {
            sp38.x = coll_data->line_coll_dist.x;
            sp38.y = translate->y;

            if (mpCollisionGetFCCommonFloor(coll_data->floor_line_id, &sp38, &floor_dist, &coll_data->floor_flags, &coll_data->floor_angle) != FALSE)
            {
                translate->y += floor_dist;
                translate->x = sp38.x;
            }
        }
    }
}

// 0x800D9F84
void mpProcessRunFloorEdgeAdjust(MPCollData *coll_data)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    /*
     * SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only.
     * Soft-lip: FloorEdge L/R snaps TopN.x onto under-edge walls — same class as
     * CeilEdgeAdjust (soak1 1828471508 JumpAerial CLIFF lip). Skip entirely.
     */
    if (mpProcessNetplaySuppressAdjNewWallOnUnattachedSoftLip(coll_data) != FALSE)
    {
        mpProcessNetplaySoftLipXDiag("floor_edge_skip", coll_data, coll_data->p_translate->x, TRUE);
        return;
    }
#endif
#if defined(PORT) && defined(SSB64_NETMENU)
    {
	f32 softlip_x_before = coll_data->p_translate->x;
#endif
    if (mpProcessCheckFloorEdgeCollisionL(coll_data) != FALSE)
    {
        mpProcessFloorEdgeLAdjust(coll_data);
#if defined(PORT) && defined(SSB64_NETMENU)
	mpProcessNetplaySoftLipXDiag("floor_edge_l", coll_data, softlip_x_before, FALSE);
	softlip_x_before = coll_data->p_translate->x;
#endif
    }
    if (mpProcessCheckFloorEdgeCollisionR(coll_data) != FALSE)
    {
        mpProcessFloorEdgeRAdjust(coll_data);
#if defined(PORT) && defined(SSB64_NETMENU)
	mpProcessNetplaySoftLipXDiag("floor_edge_r", coll_data, softlip_x_before, FALSE);
#endif
    }
#if defined(PORT) && defined(SSB64_NETMENU)
    }
#endif
}

// 0x800D9FCC
void mpProcessSetCollProjectFloorID(MPCollData *coll_data) // Check if object is above ground while airborne
{
    MPObjectColl *map_coll = &coll_data->map_coll;
    Vec3f *translate = coll_data->p_translate;
    Vec3f sp2C;

    sp2C.x = translate->x;
    sp2C.y = translate->y + map_coll->bottom;

    if (mpCollisionCheckProjectFloor(&sp2C, &coll_data->floor_line_id, &coll_data->floor_dist, &coll_data->floor_flags, &coll_data->floor_angle) == FALSE)
    {
        coll_data->floor_line_id = -1;
#if defined(PORT) && defined(SSB64_NETMENU)
	/*
	 * SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only.
	 * Project miss leaves floor_flags untouched — stale PASS|CLIFF would SoftLipEx
	 * re-latch forever under the stage (soak1 seed 2412131430). Drop residual; sticky
	 * TTL covers brief lip frames after the last live soft floor.
	 */
	if (syNetplayRollbackSemanticsActive() != FALSE)
	{
		coll_data->floor_flags &=
		    (u32) ~(MAP_VERTEX_COLL_PASS | MAP_VERTEX_COLL_CLIFF);
	}
#endif
    }
#if defined(PORT) && defined(SSB64_NETMENU)
    else
    {
	/* Latch PASS|CLIFF from a live project so next tick's wall CheckTest suppresses. */
	mpProcessNetplaySoftLipStickyNote(coll_data->floor_flags);
    }
#endif
}

// 0x800DA034
sb32 mpProcessUpdateMain(MPCollData *coll_data, sb32 (*proc_coll)(MPCollData*, GObj*, u32), GObj *gobj, u32 flags)
{
    Vec3f *translate = coll_data->p_translate;  // Current position
    Vec3f *pos_prev = &coll_data->pos_prev;     // Previous position
    Vec2f diff;                                 // Distance between current and previous positions
    f32 add_x;                                  // Position difference added to X-Position on each iteration
    f32 add_y;                                  // Position difference added to Y-Position on each iteration
    f32 add_z;                                  // Position difference added to Z-Position on each iteration
    s32 i;                                      // Collision update iterator
    s32 update_count;                           // Up to 10 updates in a single tic
    /*
     * The loop below runs zero times when coll_data->is_coll_end is already TRUE at entry (a prior
     * collision pass already concluded). On N64 the returned `result` then read a deterministic
     * stack/register value; on LP64 that garbage differs cross-ISA, forking landing/project/pass
     * verdicts between peers. Initialize deterministically to FALSE: a skipped pass processed no new
     * collision, so "no collision detected this call" is the honest, deterministic answer.
     *
     * NOTE: the airborne carry-landing case that previously made FALSE look wrong (DK stuck after a
     * cargo carry) was a latched is_coll_end, fixed at its source in mpCommonSetFighterAir — an
     * airborne fighter now starts with is_coll_end FALSE so this loop actually runs and proc_coll
     * sets a real verdict. A guessed floor-contact init here was the wrong layer (it returned stale
     * mask_stat and broke walking off a ledge while carrying).
     * See docs/bugs/netplay_grab_coupling_skip_anchor_masking_2026-06-29.md.
     */
    sb32 result = FALSE;                         // Result of collision test

#if defined(PORT) && defined(SSB64_NETMENU)
    sMPProcessNetplayCollGObj = gobj;
#endif
    if (translate->x < pos_prev->x)
    {
        diff.x = -(translate->x - pos_prev->x);
    }
    else diff.x = translate->x - pos_prev->x;

    if (translate->y < pos_prev->y)
    {
        diff.y = -(translate->y - pos_prev->y);
    }
    else diff.y = translate->y - pos_prev->y;

    if ((diff.x > 250.0F) || (diff.y > 250.0F))
    {
        // Divide by 250.0 if either difference is greater than it (this is essentially 1/10th of maximum knockback velocity, which is 2500.0).
        update_count = (diff.x > diff.y) ? diff.x / 250.0F : diff.y / 250.0F;
        update_count++;

        add_x = coll_data->pos_diff.x / update_count;
        add_y = coll_data->pos_diff.y / update_count;
        add_z = coll_data->pos_diff.z / update_count;
    }
    else
    {
        update_count = 1;

        add_x = coll_data->pos_diff.x;
        add_y = coll_data->pos_diff.y;
        add_z = coll_data->pos_diff.z;
    }
    *translate = *pos_prev;

    for (i = 0; (i < update_count) && (coll_data->is_coll_end == FALSE); i++)
    {
        *pos_prev = *translate;

        // Velocity from stage elements is applied only on the first update.
        if (i == 0)
        {
            translate->x += coll_data->vel_speed.x + coll_data->vel_push.x;
            translate->y += coll_data->vel_speed.y + coll_data->vel_push.y;
            translate->z += coll_data->vel_speed.z + coll_data->vel_push.z;
        }
        // Add position difference between current and previous positions to current position.
        translate->x += add_x;
        translate->y += add_y;
        translate->z += add_z;

        result = proc_coll(coll_data, gobj, flags);
    }
    coll_data->update_tic = gMPCollisionUpdateTic;

#if defined(PORT) && defined(SSB64_NETMENU)
    mpProcessNetplaySoftLipStickyClearIfGrounded(coll_data);
    mpProcessNetplaySoftLipStickyTick();
    sMPProcessNetplayCollGObj = NULL;
#endif
    return result;
}

// 0x800DA294
sb32 mpProcessCheckTestLWallCollision(MPCollData *coll_data)
{
    MPObjectColl *map_coll = &coll_data->map_coll;
    MPObjectColl *p_map_coll = coll_data->p_map_coll;
    Vec3f *translate = coll_data->p_translate;
    Vec3f *pos_prev = &coll_data->pos_prev;
    Vec3f sp4C;
    Vec3f sp40;
    sb32 is_collide_lwall = FALSE;
    s32 test_line_id;
    s32 floor_line_id;
    sb32 wall_collide;

    mpProcessResetMultiWallCount();

    floor_line_id = (mpCollisionCheckExistLineID(coll_data->floor_line_id) != FALSE) ? mpCollisionGetEdgeUnderLLineID(coll_data->floor_line_id) : -1;

    sp4C.x = pos_prev->x + p_map_coll->width;
    sp4C.y = pos_prev->y + p_map_coll->center;
    sp40.x = translate->x + map_coll->width;
    sp40.y = translate->y + map_coll->center;

        wall_collide = (coll_data->update_tic != gMPCollisionUpdateTic) 
                    
                                            ? 
    
    mpCollisionCheckLWallLineCollisionDiff(&sp4C, &sp40, NULL, &test_line_id, NULL, NULL) 
        
                                            : 
        
    mpCollisionCheckLWallLineCollisionSame(&sp4C, &sp40, NULL, &test_line_id, NULL, NULL);

    if ((wall_collide != FALSE) && (test_line_id != floor_line_id))
    {
        mpProcessSetMultiWallLineID(test_line_id);

        is_collide_lwall = TRUE;
    }
    sp4C.x = pos_prev->x;
    sp4C.y = pos_prev->y + p_map_coll->bottom;
    sp40.x = translate->x;
    sp40.y = translate->y + map_coll->bottom;

        wall_collide = (coll_data->update_tic != gMPCollisionUpdateTic) 
            
                                            ? 

    mpCollisionCheckLWallLineCollisionDiff(&sp4C, &sp40, NULL, &test_line_id, NULL, NULL) 

                                            : 

    mpCollisionCheckLWallLineCollisionSame(&sp4C, &sp40, NULL, &test_line_id, NULL, NULL);

    if ((wall_collide != FALSE) && (test_line_id != floor_line_id))
    {
        mpProcessSetMultiWallLineID(test_line_id);

        is_collide_lwall = TRUE;
    }
    sp4C.x = pos_prev->x;
    sp4C.y = pos_prev->y + p_map_coll->top;
    sp40.x = translate->x;
    sp40.y = translate->y + map_coll->top;

        wall_collide = (coll_data->update_tic != gMPCollisionUpdateTic) 
            
                                            ? 

    mpCollisionCheckLWallLineCollisionDiff(&sp4C, &sp40, NULL, &test_line_id, NULL, NULL) 

                                            :

    mpCollisionCheckLWallLineCollisionSame(&sp4C, &sp40, NULL, &test_line_id, NULL, NULL);

    if ((wall_collide != FALSE) && (test_line_id != floor_line_id))
    {
        mpProcessSetMultiWallLineID(test_line_id);

        is_collide_lwall = TRUE;
    }
    sp4C.x = translate->x;
    sp4C.y = translate->y + map_coll->bottom;
    sp40.x = translate->x + map_coll->width;
    sp40.y = translate->y + map_coll->center;

    if ((mpCollisionCheckLWallLineCollisionSame(&sp4C, &sp40, NULL, &test_line_id, NULL, NULL) != FALSE) && (test_line_id != floor_line_id))
    {
        mpProcessSetMultiWallLineID(test_line_id);

        is_collide_lwall = TRUE;
    }
    sp4C.x = translate->x;
    sp4C.y = translate->y + map_coll->top;
    sp40.x = translate->x + map_coll->width;
    sp40.y = translate->y + map_coll->center;

    if ((mpCollisionCheckLWallLineCollisionSame(&sp4C, &sp40, NULL, &test_line_id, NULL, NULL) != FALSE) && (test_line_id != floor_line_id))
    {
        mpProcessSetMultiWallLineID(test_line_id);

        is_collide_lwall = TRUE;
    }
    if (is_collide_lwall != FALSE)
    {
        coll_data->mask_curr |= MAP_FLAG_LWALL;
    }
    return is_collide_lwall;
}

// 0x800DA658
void mpProcessRunLWallCollision(MPCollData *coll_data)
{
    MPObjectColl *map_coll = &coll_data->map_coll;
    Vec3f *translate = coll_data->p_translate;
    Vec3f wall_pos;
    Vec3f wall_angle;
    Vec3f vertex_pos;
    s32 i;
    s32 vertex_count;
    s32 j;
    u32 wall_flags;
    s32 wall_line_id;
    f32 last_wall_x;

    mpProcessSetLastWallCollideRight();

    for (i = 0; i < sMPProcessMultiWallCollidesNum; i++)
    {
        wall_line_id = sMPProcessMultiWallCollideLineIDs[i];

        mpCollisionGetLWallEdgeU(wall_line_id, &wall_pos);

        if (wall_pos.y < (translate->y + map_coll->bottom))
        {
            if ((wall_pos.x < sMPProcessLastWallCollidePosition) && (mpCollisionGetLRCommonLWall(wall_line_id, &wall_pos, NULL, &wall_flags, &wall_angle) != FALSE))
            {
                mpProcessSetLastWallCollideStats(wall_pos.x, wall_line_id, wall_flags, &wall_angle);
            }
        }
        else
        {
            mpCollisionGetLWallEdgeD(wall_line_id, &wall_pos);

            if ((translate->y + map_coll->top) < wall_pos.y)
            {
                if ((wall_pos.x < sMPProcessLastWallCollidePosition) && (mpCollisionGetLRCommonLWall(wall_line_id, &wall_pos, NULL, &wall_flags, &wall_angle) != FALSE))
                {
                    mpProcessSetLastWallCollideStats(wall_pos.x, wall_line_id, wall_flags, &wall_angle);
                }
            }
            else
            {
                wall_pos.x = translate->x;
                wall_pos.y = translate->y + map_coll->bottom;

                if (mpCollisionGetLRCommonLWall(wall_line_id, &wall_pos, &last_wall_x, &wall_flags, &wall_angle) != FALSE)
                {
                    if ((translate->x + last_wall_x) < sMPProcessLastWallCollidePosition)
                    {
                        mpProcessSetLastWallCollideStats((translate->x + last_wall_x), wall_line_id, wall_flags, &wall_angle);
                    }
                }
                wall_pos.x = translate->x + map_coll->width;
                wall_pos.y = translate->y + map_coll->center;

                if (mpCollisionGetLRCommonLWall(wall_line_id, &wall_pos, &last_wall_x, &wall_flags, &wall_angle) != FALSE)
                {
                    if ((translate->x + last_wall_x) < sMPProcessLastWallCollidePosition)
                    {
                        mpProcessSetLastWallCollideStats((translate->x + last_wall_x), wall_line_id, wall_flags, &wall_angle);
                    }
                }
                wall_pos.x = translate->x;
                wall_pos.y = translate->y + map_coll->top;

                if (mpCollisionGetLRCommonLWall(wall_line_id, &wall_pos, &last_wall_x, &wall_flags, &wall_angle) != FALSE)
                {
                    if ((translate->x + last_wall_x) < sMPProcessLastWallCollidePosition)
                    {
                        mpProcessSetLastWallCollideStats((translate->x + last_wall_x), wall_line_id, wall_flags, &wall_angle);
                    }
                }
                vertex_count = mpCollisionGetVertexCountLineID(wall_line_id);

                for (j = 0; j < vertex_count; j++)
                {
                    mpCollisionGetVertexPositionID(wall_line_id, j, &vertex_pos);

                    if ((translate->y + map_coll->bottom) <= vertex_pos.y)
                    {
                        if (vertex_pos.y <= (translate->y + map_coll->center))
                        {
                            last_wall_x = vertex_pos.x - (((vertex_pos.y - (translate->y + map_coll->bottom)) * map_coll->width) / (map_coll->center - map_coll->bottom));

                            goto next;
                        }
                    }
                    if ((translate->y + map_coll->center) <= vertex_pos.y)
                    {
                        if (vertex_pos.y <= (translate->y + map_coll->top))
                        {
                            last_wall_x = vertex_pos.x - ((((translate->y + map_coll->top) - vertex_pos.y) * map_coll->width) / (map_coll->top - map_coll->center));

                        next:
                            if ((last_wall_x < sMPProcessLastWallCollidePosition) && (mpCollisionGetLRCommonLWall(wall_line_id, &vertex_pos, NULL, &wall_flags, &wall_angle) != FALSE))
                            {
                                mpProcessSetLastWallCollideStats(last_wall_x, wall_line_id, wall_flags, &wall_angle);
                            }
                        }
                    }
                }
            }
        }
        continue;
    }
    mpProcessGetLastWallCollideStats(&last_wall_x, &coll_data->lwall_line_id, &coll_data->lwall_flags, &coll_data->lwall_angle);

    if (translate->x > last_wall_x)
    {
#if defined(PORT) && defined(SSB64_NETMENU)
	{
	    f32 softlip_x_before = translate->x;

	    translate->x = last_wall_x;
	    /* Non-AdjNew wall Run — SoftLipX historically only instrumented AdjNew. */
	    mpProcessNetplaySoftLipXDiag("lwall_run", coll_data, softlip_x_before, FALSE);
	}
#else
        translate->x = last_wall_x;
#endif
    }
}

// 0x800DAAA8
sb32 mpProcessCheckTestRWallCollision(MPCollData *coll_data)
{
    MPObjectColl *map_coll = &coll_data->map_coll;
    MPObjectColl *p_map_coll = coll_data->p_map_coll;
    Vec3f *translate = coll_data->p_translate;
    Vec3f *pos_prev = &coll_data->pos_prev;
    Vec3f sp4C;
    Vec3f sp40;
    sb32 is_collide_rwall = FALSE;
    s32 test_line_id;
    s32 floor_line_id;
    sb32 wall_collide;

    mpProcessResetMultiWallCount();

    floor_line_id = (mpCollisionCheckExistLineID(coll_data->floor_line_id) != FALSE) ? mpCollisionGetEdgeUnderRLineID(coll_data->floor_line_id) : -1;

    sp4C.x = pos_prev->x - p_map_coll->width;
    sp4C.y = pos_prev->y + p_map_coll->center;
    sp40.x = translate->x - map_coll->width;
    sp40.y = translate->y + map_coll->center;

        wall_collide = (coll_data->update_tic != gMPCollisionUpdateTic) 
            
                                            ? 

    mpCollisionCheckRWallLineCollisionDiff(&sp4C, &sp40, NULL, &test_line_id, NULL, NULL) 
            
                                            : 

    mpCollisionCheckRWallLineCollisionSame(&sp4C, &sp40, NULL, &test_line_id, NULL, NULL);

    if ((wall_collide != FALSE) && (test_line_id != floor_line_id))
    {
        mpProcessSetMultiWallLineID(test_line_id);

        is_collide_rwall = TRUE;
    }
    sp4C.x = pos_prev->x;
    sp4C.y = pos_prev->y + p_map_coll->bottom;
    sp40.x = translate->x;
    sp40.y = translate->y + map_coll->bottom;

        wall_collide = (coll_data->update_tic != gMPCollisionUpdateTic) 
            
                                            ? 
            
    mpCollisionCheckRWallLineCollisionDiff(&sp4C, &sp40, NULL, &test_line_id, NULL, NULL)
            
                                            : 
    
    mpCollisionCheckRWallLineCollisionSame(&sp4C, &sp40, NULL, &test_line_id, NULL, NULL);

    if ((wall_collide != FALSE) && (test_line_id != floor_line_id))
    {
        mpProcessSetMultiWallLineID(test_line_id);

        is_collide_rwall = TRUE;
    }
    sp4C.x = pos_prev->x;
    sp4C.y = pos_prev->y + p_map_coll->top;
    sp40.x = translate->x;
    sp40.y = translate->y + map_coll->top;

        wall_collide = (coll_data->update_tic != gMPCollisionUpdateTic) 
            
                                            ? 
            
    mpCollisionCheckRWallLineCollisionDiff(&sp4C, &sp40, NULL, &test_line_id, NULL, NULL) 
            
                                            : 
            
    mpCollisionCheckRWallLineCollisionSame(&sp4C, &sp40, NULL, &test_line_id, NULL, NULL);

    if ((wall_collide != FALSE) && (test_line_id != floor_line_id))
    {
        mpProcessSetMultiWallLineID(test_line_id);

        is_collide_rwall = TRUE;
    }
    sp4C.x = translate->x;
    sp4C.y = translate->y + map_coll->bottom;
    sp40.x = translate->x - map_coll->width;
    sp40.y = translate->y + map_coll->center;

    if ((mpCollisionCheckRWallLineCollisionSame(&sp4C, &sp40, NULL, &test_line_id, NULL, NULL) != FALSE) && (test_line_id != floor_line_id))
    {
        mpProcessSetMultiWallLineID(test_line_id);

        is_collide_rwall = TRUE;
    }
    sp4C.x = translate->x;
    sp4C.y = translate->y + map_coll->top;
    sp40.x = translate->x - map_coll->width;
    sp40.y = translate->y + map_coll->center;

    if ((mpCollisionCheckRWallLineCollisionSame(&sp4C, &sp40, NULL, &test_line_id, NULL, NULL) != FALSE) && (test_line_id != floor_line_id))
    {
        mpProcessSetMultiWallLineID(test_line_id);

        is_collide_rwall = TRUE;
    }
    if (is_collide_rwall != FALSE)
    {
        coll_data->mask_curr |= MAP_FLAG_RWALL;
    }
    return is_collide_rwall;
}

// 0x800DAE6C
void mpProcessRunRWallCollision(MPCollData *coll_data)
{
    MPObjectColl *map_coll = &coll_data->map_coll;
    Vec3f *translate = coll_data->p_translate;
    Vec3f wall_pos;
    Vec3f wall_angle;
    Vec3f vertex_pos;
    s32 i;
    s32 vertex_count;
    s32 j;
    u32 wall_flags;
    s32 wall_line_id;
    f32 last_wall_x;

    mpProcessSetLastWallCollideLeft();

    for (i = 0; i < sMPProcessMultiWallCollidesNum; i++)
    {
        wall_line_id = sMPProcessMultiWallCollideLineIDs[i];

        mpCollisionGetRWallEdgeU(wall_line_id, &wall_pos);

        if (wall_pos.y < (translate->y + map_coll->bottom))
        {
            if ((sMPProcessLastWallCollidePosition < wall_pos.x) && (mpCollisionGetLRCommonRWall(wall_line_id, &wall_pos, NULL, &wall_flags, &wall_angle) != FALSE))
            {
                mpProcessSetLastWallCollideStats(wall_pos.x, wall_line_id, wall_flags, &wall_angle);
            }
        }
        else
        {
            mpCollisionGetRWallEdgeD(wall_line_id, &wall_pos);

            if ((translate->y + map_coll->top) < wall_pos.y)
            {
                if ((sMPProcessLastWallCollidePosition < wall_pos.x) && (mpCollisionGetLRCommonRWall(wall_line_id, &wall_pos, NULL, &wall_flags, &wall_angle) != FALSE))
                {
                    mpProcessSetLastWallCollideStats(wall_pos.x, wall_line_id, wall_flags, &wall_angle);
                }
            }
            else
            {
                wall_pos.x = translate->x;
                wall_pos.y = translate->y + map_coll->bottom;

                if (mpCollisionGetLRCommonRWall(wall_line_id, &wall_pos, &last_wall_x, &wall_flags, &wall_angle) != FALSE)
                {
                    if ((translate->x + last_wall_x) > sMPProcessLastWallCollidePosition)
                    {
                        mpProcessSetLastWallCollideStats((translate->x + last_wall_x), wall_line_id, wall_flags, &wall_angle);
                    }
                }
                wall_pos.x = translate->x - map_coll->width;
                wall_pos.y = translate->y + map_coll->center;

                if (mpCollisionGetLRCommonRWall(wall_line_id, &wall_pos, &last_wall_x, &wall_flags, &wall_angle) != FALSE)
                {
                    if ((translate->x + last_wall_x) > sMPProcessLastWallCollidePosition)
                    {
                        mpProcessSetLastWallCollideStats((translate->x + last_wall_x), wall_line_id, wall_flags, &wall_angle);
                    }
                }
                wall_pos.x = translate->x;
                wall_pos.y = translate->y + map_coll->top;

                if (mpCollisionGetLRCommonRWall(wall_line_id, &wall_pos, &last_wall_x, &wall_flags, &wall_angle) != FALSE)
                {
                    if ((translate->x + last_wall_x) > sMPProcessLastWallCollidePosition)
                    {
                        mpProcessSetLastWallCollideStats((translate->x + last_wall_x), wall_line_id, wall_flags, &wall_angle);
                    }
                }
                vertex_count = mpCollisionGetVertexCountLineID(wall_line_id);

                for (j = 0; j < vertex_count; j++)
                {
                    mpCollisionGetVertexPositionID(wall_line_id, j, &vertex_pos);

                    if ((translate->y + map_coll->bottom) <= vertex_pos.y)
                    {
                        if (vertex_pos.y <= (translate->y + map_coll->center))
                        {
                            last_wall_x = vertex_pos.x + (((vertex_pos.y - (translate->y + map_coll->bottom)) * map_coll->width) / (map_coll->center - map_coll->bottom));

                            goto next;
                        }
                    }
                    if ((translate->y + map_coll->center) <= vertex_pos.y)
                    {
                        if (vertex_pos.y <= (translate->y + map_coll->top))
                        {
                            last_wall_x = vertex_pos.x + ((((translate->y + map_coll->top) - vertex_pos.y) * map_coll->width) / (map_coll->top - map_coll->center));

                        next:
                            if ((sMPProcessLastWallCollidePosition < last_wall_x) && (mpCollisionGetLRCommonRWall(wall_line_id, &vertex_pos, NULL, &wall_flags, &wall_angle) != FALSE))
                            {
                                mpProcessSetLastWallCollideStats(last_wall_x, wall_line_id, wall_flags, &wall_angle);
                            }
                        }
                    }
                }
            }
        }
        continue;
    }
    mpProcessGetLastWallCollideStats(&last_wall_x, &coll_data->rwall_line_id, &coll_data->rwall_flags, &coll_data->rwall_angle);

    if (translate->x < last_wall_x)
    {
#if defined(PORT) && defined(SSB64_NETMENU)
	{
	    f32 softlip_x_before = translate->x;

	    translate->x = last_wall_x;
	    mpProcessNetplaySoftLipXDiag("rwall_run", coll_data, softlip_x_before, FALSE);
	}
#else
        translate->x = last_wall_x;
#endif
    }
}

// 0x800DB2BC
sb32 mpProcessCheckTestFloorCollisionNew(MPCollData *coll_data)
{
    Vec3f *translate = coll_data->p_translate;
    s32 wall_line_id;
    Vec3f object_pos;
    s32 unused;
    sb32 is_wall_edge;
    f32 floor_dist;

    coll_data->mask_stat &= ~MAP_FLAG_FLOOR;

    object_pos.x = translate->x;
    object_pos.y = translate->y + coll_data->map_coll.bottom;

    if (mpCollisionCheckExistLineID(coll_data->floor_line_id) == FALSE)
    {
        mpProcessSetCollProjectFloorID(coll_data);

        return FALSE;
    }
    if (mpCollisionGetFCCommonFloor(coll_data->floor_line_id, &object_pos, &floor_dist, &coll_data->floor_flags, &coll_data->floor_angle) != FALSE)
    {
        translate->y += floor_dist;

        coll_data->floor_dist = 0.0F;
        coll_data->mask_stat |= MAP_FLAG_FLOOR;

        return TRUE;
    }
    is_wall_edge = FALSE;

    mpCollisionGetFloorEdgeL(coll_data->floor_line_id, &object_pos);

    if (translate->x <= object_pos.x)
    {
        wall_line_id = mpCollisionGetEdgeUnderLLineID(coll_data->floor_line_id);

        if ((wall_line_id != -1) && (mpCollisionGetLineTypeID(wall_line_id) == nMPLineKindRWall))
        {
            is_wall_edge = TRUE;
        }
    }
    else
    {
        mpCollisionGetFloorEdgeR(coll_data->floor_line_id, &object_pos);

        wall_line_id = mpCollisionGetEdgeUnderRLineID(coll_data->floor_line_id);

        if ((wall_line_id != -1) && (mpCollisionGetLineTypeID(wall_line_id) == nMPLineKindLWall))
        {
            is_wall_edge = TRUE;
        }
    }
    translate->y = object_pos.y - coll_data->map_coll.bottom;

    if (is_wall_edge != FALSE)
    {
#if defined(PORT) && defined(SSB64_NETMENU)
	{
	    f32 softlip_x_before = translate->x;

	    translate->x = object_pos.x;
	    mpProcessNetplaySoftLipXDiag("floor_new_wall_edge", coll_data, softlip_x_before, FALSE);
	}
#else
        translate->x = object_pos.x;
#endif

        mpCollisionGetFCCommonFloor(coll_data->floor_line_id, &object_pos, NULL, &coll_data->floor_flags, &coll_data->floor_angle);

        coll_data->mask_stat |= MAP_FLAG_FLOOR;
        coll_data->floor_dist = 0.0F;

        return TRUE;
    }
    mpProcessSetCollProjectFloorID(coll_data);

    return FALSE;
}

// 0x800DB474
sb32 mpProcessCheckTestFloorCollision(MPCollData *coll_data, s32 line_id)
{
    MPObjectColl *p_map_coll = coll_data->p_map_coll;
    MPObjectColl *map_coll = &coll_data->map_coll;
    Vec3f *translate = coll_data->p_translate;
    s32 is_collide_floor;
    Vec3f sp4C;
    Vec3f sp40;
    s32 floor_line_id;
    u32 floor_flags;
    Vec3f floor_angle;

    sp4C.x = coll_data->pos_prev.x;
    sp4C.y = coll_data->pos_prev.y + p_map_coll->bottom;
    sp40.x = translate->x;
    sp40.y = translate->y + map_coll->bottom;

                    is_collide_floor
                              
                            =

        (coll_data->update_tic != gMPCollisionUpdateTic)

                            ?

    mpCollisionCheckFloorLineCollisionDiff(&sp4C, &sp40, &coll_data->line_coll_dist, &floor_line_id, &floor_flags, &floor_angle)

                            :

    mpCollisionCheckFloorLineCollisionSame(&sp4C, &sp40, &coll_data->line_coll_dist, &floor_line_id, &floor_flags, &floor_angle);

    if ((is_collide_floor != FALSE) && (floor_line_id != line_id))
    {
        coll_data->mask_curr |= MAP_FLAG_FLOOR;
        coll_data->floor_line_id = floor_line_id;
        coll_data->floor_flags = floor_flags;
        coll_data->floor_angle = floor_angle;

        return TRUE;
    }
    else return FALSE;
}

// 0x800DB590
sb32 mpProcessCheckTestLCliffCollision(MPCollData *coll_data)
{
    Vec3f *translate = coll_data->p_translate;
    Vec2f *cliffcatch_coll = &coll_data->cliffcatch_coll;
    Vec3f *pos_prev = &coll_data->pos_prev;
    Vec3f sp48;
    Vec3f object_pos;
    u32 floor_flags;
    s32 is_collide_floor;

    if (*coll_data->p_lr != +1)
    {
        return FALSE;
    }

    sp48.x = pos_prev->x + cliffcatch_coll->x;
    sp48.y = pos_prev->y + cliffcatch_coll->y;
    object_pos.x = translate->x + cliffcatch_coll->x;
    object_pos.y = translate->y + cliffcatch_coll->y;

               is_collide_floor

                       =

    (coll_data->update_tic != gMPCollisionUpdateTic)

                       ?

    mpCollisionCheckFloorLineCollisionDiff(&sp48, &object_pos, &coll_data->line_coll_dist, &coll_data->cliff_id, &floor_flags, NULL)

                       :

    mpCollisionCheckFloorLineCollisionSame(&sp48, &object_pos, &coll_data->line_coll_dist, &coll_data->cliff_id, &floor_flags, NULL);


    if ((is_collide_floor != FALSE) && (floor_flags & MAP_VERTEX_COLL_CLIFF) && ((floor_flags & MAP_VERTEX_MAT_MASK) != nMPMaterial4))
    {
        mpCollisionGetFloorEdgeL(coll_data->cliff_id, &object_pos);

        if ((coll_data->line_coll_dist.x - object_pos.x) < 800.0F)
        {
            coll_data->mask_curr |= MAP_FLAG_LCLIFF;
            coll_data->mask_stat |= MAP_FLAG_LCLIFF;

            return TRUE;
        }
    }
    return FALSE;
}

// 0x800DB6F0
sb32 mpProcessCheckTestRCliffCollision(MPCollData *coll_data)
{
    Vec3f *translate = coll_data->p_translate;
    Vec2f *cliffcatch_coll = &coll_data->cliffcatch_coll;
    Vec3f *pos_prev = &coll_data->pos_prev;
    Vec3f sp48;
    Vec3f object_pos;
    u32 floor_flags;
    s32 is_collide_floor;

    if (*coll_data->p_lr != -1)
    {
        return FALSE;
    }

    sp48.x = pos_prev->x - cliffcatch_coll->x;
    sp48.y = pos_prev->y + cliffcatch_coll->y;
    object_pos.x = translate->x - cliffcatch_coll->x;
    object_pos.y = translate->y + cliffcatch_coll->y;

               is_collide_floor

                       =

    (coll_data->update_tic != gMPCollisionUpdateTic)

                       ?

    mpCollisionCheckFloorLineCollisionDiff(&sp48, &object_pos, &coll_data->line_coll_dist, &coll_data->cliff_id, &floor_flags, NULL)

                       :

    mpCollisionCheckFloorLineCollisionSame(&sp48, &object_pos, &coll_data->line_coll_dist, &coll_data->cliff_id, &floor_flags, NULL);

    if ((is_collide_floor != FALSE) && (floor_flags & MAP_VERTEX_COLL_CLIFF))
    {
        mpCollisionGetFloorEdgeR(coll_data->cliff_id, &object_pos);

        if ((object_pos.x - coll_data->line_coll_dist.x) < 800.0F)
        {
            coll_data->mask_curr |= MAP_FLAG_RCLIFF;
            coll_data->mask_stat |= MAP_FLAG_RCLIFF;

            return TRUE;
        }
    }
    return FALSE;
}

// 0x800DB838
sb32 mpProcessCheckTestLWallCollisionAdjNew(MPCollData *coll_data)
{
    MPObjectColl *map_coll = &coll_data->map_coll;
    MPObjectColl *p_map_coll = coll_data->p_map_coll;
    Vec3f *translate = coll_data->p_translate;
    Vec3f *pos_prev = &coll_data->pos_prev;
    Vec3f sp54;
    Vec3f sp48;
    sb32 is_collide_lwall;
    s32 test_line_id;
    s32 ud_line_id;
    s32 edge_line_id;
    u32 floor_flags;
    s32 line_collide;

    is_collide_lwall = FALSE;
    floor_flags = 0U;

    coll_data->mask_unk &= ~MAP_FLAG_LWALL;
    coll_data->mask_stat &= ~MAP_FLAG_LWALL;

    mpProcessResetMultiWallCount();

    sp54.x = pos_prev->x + p_map_coll->width;
    sp54.y = pos_prev->y + p_map_coll->center;
    sp48.x = translate->x + map_coll->width;
    sp48.y = translate->y + map_coll->center;

                          line_collide

                                =

    (coll_data->update_tic != gMPCollisionUpdateTic)

                                ?

    mpCollisionCheckLWallLineCollisionDiff(&sp54, &sp48, NULL, &test_line_id, NULL, NULL)

                                :

    mpCollisionCheckLWallLineCollisionSame(&sp54, &sp48, NULL, &test_line_id, NULL, NULL);

    if (line_collide != FALSE)
    {
        mpProcessSetMultiWallLineID(test_line_id);

        is_collide_lwall = TRUE;
    }
    sp54.x = pos_prev->x;
    sp54.y = pos_prev->y + p_map_coll->bottom;
    sp48.x = translate->x;
    sp48.y = translate->y + map_coll->bottom;

                      line_collide

                            =

    (coll_data->update_tic != gMPCollisionUpdateTic)

                            ?

    mpCollisionCheckLWallLineCollisionDiff(&sp54, &sp48, NULL, &test_line_id, NULL, NULL)

                            :

    mpCollisionCheckLWallLineCollisionSame(&sp54, &sp48, NULL, &test_line_id, NULL, NULL);

    if (line_collide != FALSE)
    {
        mpProcessSetMultiWallLineID(test_line_id);

        is_collide_lwall = TRUE;
    }
    sp54.x = pos_prev->x;
    sp54.y = pos_prev->y + p_map_coll->top;
    sp48.x = translate->x;
    sp48.y = translate->y + map_coll->top;

                  line_collide

                        =

    (coll_data->update_tic != gMPCollisionUpdateTic)

                        ?

    mpCollisionCheckLWallLineCollisionDiff(&sp54, &sp48, NULL, &test_line_id, NULL, NULL)

                        :

    mpCollisionCheckLWallLineCollisionSame(&sp54, &sp48, NULL, &test_line_id, NULL, NULL);

    if (line_collide != FALSE)
    {
        mpProcessSetMultiWallLineID(test_line_id);

        is_collide_lwall = TRUE;
    }
    sp54.x = translate->x;
    sp54.y = translate->y + map_coll->bottom;
    sp48.x = translate->x + map_coll->width;
    sp48.y = translate->y + map_coll->center;

    if (mpCollisionCheckLWallLineCollisionSame(&sp54, &sp48, NULL, &test_line_id, NULL, NULL) != FALSE)
    {
        mpProcessSetMultiWallLineID(test_line_id);

        is_collide_lwall = TRUE;
    }
    sp54.x = translate->x;
    sp54.y = translate->y + map_coll->top;
    sp48.x = translate->x + map_coll->width;
    sp48.y = translate->y + map_coll->center;

    if (mpCollisionCheckLWallLineCollisionSame(&sp54, &sp48, NULL, &test_line_id, NULL, NULL) != FALSE)
    {
        mpProcessSetMultiWallLineID(test_line_id);

        is_collide_lwall = TRUE;
    }
    sp54.x = pos_prev->x + p_map_coll->width;
    sp54.y = pos_prev->y + p_map_coll->center;
    sp48.x = translate->x + map_coll->width;
    sp48.y = translate->y + map_coll->center;

                  line_collide

                        =

    (coll_data->update_tic != gMPCollisionUpdateTic)

                        ?

    mpCollisionCheckCeilLineCollisionDiff(&sp54, &sp48, NULL, &test_line_id, NULL, NULL)

                        :

    mpCollisionCheckCeilLineCollisionSame(&sp54, &sp48, NULL, &test_line_id, NULL, NULL);

    if (line_collide != FALSE)
    {
        edge_line_id = mpCollisionGetEdgeUpperLLineID(test_line_id);

        if (edge_line_id != -1)
        {
            if (mpCollisionGetLineTypeID(edge_line_id) == nMPLineKindLWall)
            {
                sp54.x = pos_prev->x;
                sp54.y = pos_prev->y + p_map_coll->top;
                sp48.x = translate->x;
                sp48.y = translate->y + map_coll->top;

                                  line_collide

                                        =

                    (coll_data->update_tic != gMPCollisionUpdateTic)

                                        ?

                mpCollisionCheckCeilLineCollisionDiff(&sp54, &sp48, NULL, &ud_line_id, NULL, NULL)

                                        :

                mpCollisionCheckCeilLineCollisionSame(&sp54, &sp48, NULL, &ud_line_id, NULL, NULL);

                if ((line_collide == FALSE) || (test_line_id != ud_line_id))
                {
                    sp54.x = translate->x;
                    sp54.y = translate->y + map_coll->top;
                    sp48.x = translate->x + map_coll->width;
                    sp48.y = translate->y + map_coll->center;

                    if ((mpCollisionCheckCeilLineCollisionSame(&sp54, &sp48, NULL, &ud_line_id, NULL, NULL) == FALSE) || (test_line_id != ud_line_id))
                    {
                        mpProcessSetMultiWallLineID(edge_line_id);

                        is_collide_lwall = TRUE;
                    }
                }
            }
        }
    }
    sp54.x = pos_prev->x + p_map_coll->width;
    sp54.y = pos_prev->y + p_map_coll->center;
    sp48.x = translate->x + map_coll->width;
    sp48.y = translate->y + map_coll->center;

                       line_collide

                            =

        (coll_data->update_tic != gMPCollisionUpdateTic)

                            ?

    mpCollisionCheckFloorLineCollisionDiff(&sp54, &sp48, NULL, &test_line_id, &floor_flags, NULL)

                            :

    mpCollisionCheckFloorLineCollisionSame(&sp54, &sp48, NULL, &test_line_id, &floor_flags, NULL);

#if defined(PORT) && defined(SSB64_NETMENU)
    /*
     * SSB64_NETMENU: stripped from offline builds. Runtime: AdjNew air/damage wall path only.
     * Vanilla skips wall-from-floor when the swept floor is PASS (soft platforms). Dream Land
     * ledge lips are often CLIFF-only (0x8000) without PASS — this gate then attaches the
     * under-edge wall and Cross-ISA Diff float forks TopN.x while floor_dist stays matched
     * (soak1 session 401300810 seed 998483872 JumpB @634 after PASS→CLIFF @633). Treat CLIFF
     * like PASS here so air over the lip matches soft-platform behavior. CliffCatch still uses
     * CheckTestL/RCliffCollision. See docs/bugs/netplay_airborne_cliff_lip_wall_from_floor_fc_drift_2026-07-13.md.
     */
    if ((line_collide != FALSE) && !(floor_flags & (MAP_VERTEX_COLL_PASS | MAP_VERTEX_COLL_CLIFF)))
#else
    if ((line_collide != FALSE) && !(floor_flags & MAP_VERTEX_COLL_PASS)) // 0x4000
#endif
    {
        edge_line_id = mpCollisionGetEdgeUnderLLineID(test_line_id);

        if (edge_line_id != -1)
        {
            if (mpCollisionGetLineTypeID(edge_line_id) == nMPLineKindLWall)
            {
                sp54.x = pos_prev->x;
                sp54.y = pos_prev->y + p_map_coll->bottom;
                sp48.x = translate->x;
                sp48.y = translate->y + map_coll->bottom;

                               line_collide

                                    =

                (coll_data->update_tic != gMPCollisionUpdateTic)

                                    ?

                mpCollisionCheckFloorLineCollisionDiff(&sp54, &sp48, NULL, &ud_line_id, NULL, NULL)

                                    :

                mpCollisionCheckFloorLineCollisionSame(&sp54, &sp48, NULL, &ud_line_id, NULL, NULL);

                if ((line_collide == FALSE) || (test_line_id != ud_line_id))
                {
                    sp54.x = translate->x;
                    sp54.y = translate->y + map_coll->bottom;
                    sp48.x = translate->x + map_coll->width;
                    sp48.y = translate->y + map_coll->center;

                    if ((mpCollisionCheckFloorLineCollisionSame(&sp54, &sp48, NULL, &ud_line_id, NULL, NULL) == FALSE) || (test_line_id != ud_line_id))
                    {
                        mpProcessSetMultiWallLineID(edge_line_id);

                        is_collide_lwall = TRUE;
                    }
                }
            }
        }
    }
#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: strip direct / ceil-edge / residual wall hits on PASS|CLIFF soft lip. */
    if (is_collide_lwall != FALSE)
    {
        if (mpProcessNetplaySuppressAdjNewWallSoftLipEx(coll_data, floor_flags) != FALSE)
        {
            mpProcessNetplaySoftLipXDiag("lwall_suppress", coll_data, coll_data->p_translate->x, TRUE);
            mpProcessResetMultiWallCount();
            is_collide_lwall = FALSE;
            coll_data->mask_curr &= ~MAP_FLAG_LWALL;
        }
        else
        {
            /* Wall hit kept — SoftLipEx false (no residual/sticky). Force-log for soak bisect. */
            mpProcessNetplaySoftLipXDiagEx("lwall_keep", coll_data, coll_data->p_translate->x, FALSE, TRUE);
        }
    }
#endif
    if (is_collide_lwall != FALSE)
    {
        coll_data->mask_curr |= MAP_FLAG_LWALL;
    }
    return is_collide_lwall;
}

// 0x800DBF58
void mpProcessRunLWallCollisionAdjNew(MPCollData *coll_data)
{
    MPObjectColl *map_coll = &coll_data->map_coll;
    Vec3f *translate = coll_data->p_translate;
    Vec3f wall_pos;
    Vec3f wall_angle;
    Vec3f vertex_pos;
    s32 i;
    s32 vertex_count;
    s32 j;
    u32 wall_flags;
    s32 wall_line_id;
    f32 last_wall_x;

    mpProcessSetLastWallCollideRight();

    for (i = 0; i < sMPProcessMultiWallCollidesNum; i++)
    {
        wall_line_id = sMPProcessMultiWallCollideLineIDs[i];

        mpCollisionGetLWallEdgeU(wall_line_id, &wall_pos);

        if (wall_pos.y < (translate->y + map_coll->bottom))
        {
            if ((wall_pos.x < sMPProcessLastWallCollidePosition) && (mpCollisionGetLRCommonLWall(wall_line_id, &wall_pos, NULL, &wall_flags, &wall_angle) != FALSE))
            {
                mpProcessSetLastWallCollideStats(wall_pos.x, wall_line_id, wall_flags, &wall_angle);
            }
        }
        else
        {
            mpCollisionGetLWallEdgeD(wall_line_id, &wall_pos);

            if ((translate->y + map_coll->top) < wall_pos.y)
            {
                if ((wall_pos.x < sMPProcessLastWallCollidePosition) && (mpCollisionGetLRCommonLWall(wall_line_id, &wall_pos, NULL, &wall_flags, &wall_angle) != FALSE))
                {
                    mpProcessSetLastWallCollideStats(wall_pos.x, wall_line_id, wall_flags, &wall_angle);
                }
            }
            else
            {
                wall_pos.x = translate->x;
                wall_pos.y = translate->y + map_coll->bottom;

                if (mpCollisionGetLRCommonLWall(wall_line_id, &wall_pos, &last_wall_x, &wall_flags, &wall_angle) != FALSE)
                {
                    if ((translate->x + last_wall_x) < sMPProcessLastWallCollidePosition)
                    {
                        mpProcessSetLastWallCollideStats((translate->x + last_wall_x), wall_line_id, wall_flags, &wall_angle);
                    }
                }
                wall_pos.x = translate->x + map_coll->width;
                wall_pos.y = translate->y + map_coll->center;

                if (mpCollisionGetLRCommonLWall(wall_line_id, &wall_pos, &last_wall_x, &wall_flags, &wall_angle) != FALSE)
                {
                    if ((translate->x + last_wall_x) < sMPProcessLastWallCollidePosition)
                    {
                        mpProcessSetLastWallCollideStats((translate->x + last_wall_x), wall_line_id, wall_flags, &wall_angle);
                    }
                }
                wall_pos.x = translate->x;
                wall_pos.y = translate->y + map_coll->top;

                if (mpCollisionGetLRCommonLWall(wall_line_id, &wall_pos, &last_wall_x, &wall_flags, &wall_angle) != FALSE)
                {
                    if ((translate->x + last_wall_x) < sMPProcessLastWallCollidePosition)
                    {
                        mpProcessSetLastWallCollideStats((translate->x + last_wall_x), wall_line_id, wall_flags, &wall_angle);
                    }
                }
                vertex_count = mpCollisionGetVertexCountLineID(wall_line_id);

                for (j = 0; j < vertex_count; j++)
                {
                    mpCollisionGetVertexPositionID(wall_line_id, j, &vertex_pos);

                    if ((translate->y + map_coll->bottom) <= vertex_pos.y)
                    {
                        if (vertex_pos.y <= (translate->y + map_coll->center))
                        {
                            last_wall_x = vertex_pos.x - (((vertex_pos.y - (translate->y + map_coll->bottom)) * map_coll->width) / (map_coll->center - map_coll->bottom));

                            goto next;
                        }
                    }
                    if ((translate->y + map_coll->center) <= vertex_pos.y)
                    {
                        if (vertex_pos.y <= (translate->y + map_coll->top))
                        {
                            last_wall_x = vertex_pos.x - ((((translate->y + map_coll->top) - vertex_pos.y) * map_coll->width) / (map_coll->top - map_coll->center));

                        next:
                            if ((last_wall_x < sMPProcessLastWallCollidePosition) && (mpCollisionGetLRCommonLWall(wall_line_id, &vertex_pos, NULL, &wall_flags, &wall_angle) != FALSE))
                            {
                                mpProcessSetLastWallCollideStats(last_wall_x, wall_line_id, wall_flags, &wall_angle);
                            }
                        }
                    }
                }
            }
        }
        continue;
    }
    mpProcessGetLastWallCollideStats(&last_wall_x, &coll_data->lwall_line_id, &coll_data->lwall_flags, &coll_data->lwall_angle);

    if (translate->x > last_wall_x)
    {
#if defined(PORT) && defined(SSB64_NETMENU)
        f32 softlip_x_before = translate->x;
#endif
        translate->x = last_wall_x;

        coll_data->mask_stat |= MAP_FLAG_LWALL;
#if defined(PORT) && defined(SSB64_NETMENU)
        mpProcessNetplaySoftLipXDiag("lwall_adjnew", coll_data, softlip_x_before, FALSE);
#endif
    }
#if defined(PORT) && defined(SSB64_NETMENU)
    mpProcessNetplayHardenAdjNewTranslateSnap(coll_data, translate);
#endif
    coll_data->mask_unk |= MAP_FLAG_LWALL;
}

// 0x800DC3C8
sb32 mpProcessCheckTestRWallCollisionAdjNew(MPCollData *coll_data)
{
    MPObjectColl *map_coll = &coll_data->map_coll;
    MPObjectColl *p_map_coll = coll_data->p_map_coll;
    Vec3f *translate = coll_data->p_translate;
    Vec3f *pos_prev = &coll_data->pos_prev;
    Vec3f sp54;
    Vec3f sp48;
    sb32 is_collide_rwall;
    s32 test_line_id;
    s32 ud_line_id;
    s32 edge_line_id;
    u32 floor_flags;
    s32 line_collide;

    is_collide_rwall = FALSE;
    floor_flags = 0U;

    coll_data->mask_unk &= ~MAP_FLAG_RWALL;
    coll_data->mask_stat &= ~MAP_FLAG_RWALL;

    mpProcessResetMultiWallCount();

    sp54.x = pos_prev->x - p_map_coll->width;
    sp54.y = pos_prev->y + p_map_coll->center;
    sp48.x = translate->x - map_coll->width;
    sp48.y = translate->y + map_coll->center;

                          line_collide

                                =

    (coll_data->update_tic != gMPCollisionUpdateTic)

                                ?

    mpCollisionCheckRWallLineCollisionDiff(&sp54, &sp48, NULL, &test_line_id, NULL, NULL)

                                :

    mpCollisionCheckRWallLineCollisionSame(&sp54, &sp48, NULL, &test_line_id, NULL, NULL);

    if (line_collide != FALSE)
    {
        mpProcessSetMultiWallLineID(test_line_id);

        is_collide_rwall = TRUE;
    }
    sp54.x = pos_prev->x;
    sp54.y = pos_prev->y + p_map_coll->bottom;
    sp48.x = translate->x;
    sp48.y = translate->y + map_coll->bottom;

                      line_collide

                            =

    (coll_data->update_tic != gMPCollisionUpdateTic)

                            ?

    mpCollisionCheckRWallLineCollisionDiff(&sp54, &sp48, NULL, &test_line_id, NULL, NULL)

                            :

    mpCollisionCheckRWallLineCollisionSame(&sp54, &sp48, NULL, &test_line_id, NULL, NULL);

    if (line_collide != FALSE)
    {
        mpProcessSetMultiWallLineID(test_line_id);

        is_collide_rwall = TRUE;
    }
    sp54.x = pos_prev->x;
    sp54.y = pos_prev->y + p_map_coll->top;
    sp48.x = translate->x;
    sp48.y = translate->y + map_coll->top;

                   line_collide

                        =

    (coll_data->update_tic != gMPCollisionUpdateTic)

                        ?

    mpCollisionCheckRWallLineCollisionDiff(&sp54, &sp48, NULL, &test_line_id, NULL, NULL)

                        :

    mpCollisionCheckRWallLineCollisionSame(&sp54, &sp48, NULL, &test_line_id, NULL, NULL);

    if (line_collide != FALSE)
    {
        mpProcessSetMultiWallLineID(test_line_id);

        is_collide_rwall = TRUE;
    }
    sp54.x = translate->x;
    sp54.y = translate->y + map_coll->bottom;
    sp48.x = translate->x - map_coll->width;
    sp48.y = translate->y + map_coll->center;

    if (mpCollisionCheckRWallLineCollisionSame(&sp54, &sp48, NULL, &test_line_id, NULL, NULL) != FALSE)
    {
        mpProcessSetMultiWallLineID(test_line_id);

        is_collide_rwall = TRUE;
    }
    sp54.x = translate->x;
    sp54.y = translate->y + map_coll->top;
    sp48.x = translate->x - map_coll->width;
    sp48.y = translate->y + map_coll->center;

    if (mpCollisionCheckRWallLineCollisionSame(&sp54, &sp48, NULL, &test_line_id, NULL, NULL) != FALSE)
    {
        mpProcessSetMultiWallLineID(test_line_id);

        is_collide_rwall = TRUE;
    }
    sp54.x = pos_prev->x - p_map_coll->width;
    sp54.y = pos_prev->y + p_map_coll->center;
    sp48.x = translate->x - map_coll->width;
    sp48.y = translate->y + map_coll->center;

                  line_collide

                        =

    (coll_data->update_tic != gMPCollisionUpdateTic)

                        ?

    mpCollisionCheckCeilLineCollisionDiff(&sp54, &sp48, NULL, &test_line_id, NULL, NULL)

                        :

    mpCollisionCheckCeilLineCollisionSame(&sp54, &sp48, NULL, &test_line_id, NULL, NULL);

    if (line_collide != FALSE)
    {
        edge_line_id = mpCollisionGetEdgeUpperRLineID(test_line_id);

        if (edge_line_id != -1)
        {
            if (mpCollisionGetLineTypeID(edge_line_id) == nMPLineKindRWall)
            {
                sp54.x = pos_prev->x;
                sp54.y = pos_prev->y + p_map_coll->top;
                sp48.x = translate->x;
                sp48.y = translate->y + map_coll->top;

                                  line_collide

                                        =

                (coll_data->update_tic != gMPCollisionUpdateTic)

                                        ?

                mpCollisionCheckCeilLineCollisionDiff(&sp54, &sp48, NULL, &ud_line_id, NULL, NULL)

                                        :

                mpCollisionCheckCeilLineCollisionSame(&sp54, &sp48, NULL, &ud_line_id, NULL, NULL);

                if ((line_collide == FALSE) || (test_line_id != ud_line_id))
                {
                    sp54.x = translate->x;
                    sp54.y = translate->y + map_coll->top;
                    sp48.x = translate->x - map_coll->width;
                    sp48.y = translate->y + map_coll->center;

                    if ((mpCollisionCheckCeilLineCollisionSame(&sp54, &sp48, 0, &ud_line_id, 0, 0) == 0) || (test_line_id != ud_line_id))
                    {
                        mpProcessSetMultiWallLineID(edge_line_id);

                        is_collide_rwall = TRUE;
                    }
                }
            }
        }
    }
    sp54.x = pos_prev->x - p_map_coll->width;
    sp54.y = pos_prev->y + p_map_coll->center;
    sp48.x = translate->x - map_coll->width;
    sp48.y = translate->y + map_coll->center;

                      line_collide

                            =

    (coll_data->update_tic != gMPCollisionUpdateTic)

                            ?

    mpCollisionCheckFloorLineCollisionDiff(&sp54, &sp48, NULL, &test_line_id, &floor_flags, NULL)

                            :

    mpCollisionCheckFloorLineCollisionSame(&sp54, &sp48, NULL, &test_line_id, &floor_flags, NULL);

#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: same CLIFF-as-PASS wall-from-floor skip as LWall AdjNew above. */
    if ((line_collide != FALSE) && !(floor_flags & (MAP_VERTEX_COLL_PASS | MAP_VERTEX_COLL_CLIFF)))
#else
    if ((line_collide != FALSE) && !(floor_flags & MAP_VERTEX_COLL_PASS))
#endif
    {
        edge_line_id = mpCollisionGetEdgeUnderRLineID(test_line_id);

        if (edge_line_id != -1)
        {
            if (mpCollisionGetLineTypeID(edge_line_id) == nMPLineKindRWall)
            {
                sp54.x = pos_prev->x;
                sp54.y = pos_prev->y + p_map_coll->bottom;
                sp48.x = translate->x;
                sp48.y = translate->y + map_coll->bottom;

                              line_collide

                                    =

                (coll_data->update_tic != gMPCollisionUpdateTic)

                                    ?

                mpCollisionCheckFloorLineCollisionDiff(&sp54, &sp48, NULL, &ud_line_id, NULL, NULL)

                                    :

                mpCollisionCheckFloorLineCollisionSame(&sp54, &sp48, NULL, &ud_line_id, NULL, NULL);

                if ((line_collide == FALSE) || (test_line_id != ud_line_id))
                {
                    sp54.x = translate->x;
                    sp54.y = translate->y + map_coll->bottom;
                    sp48.x = translate->x - map_coll->width;
                    sp48.y = translate->y + map_coll->center;

                    if ((mpCollisionCheckFloorLineCollisionSame(&sp54, &sp48, NULL, &ud_line_id, NULL, NULL) == FALSE) || (test_line_id != ud_line_id))
                    {
                        mpProcessSetMultiWallLineID(edge_line_id);

                        is_collide_rwall = TRUE;
                    }
                }
            }
        }
    }
#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: same PASS|CLIFF soft-lip AdjNew wall suppress as LWall above. */
    if (is_collide_rwall != FALSE)
    {
        if (mpProcessNetplaySuppressAdjNewWallSoftLipEx(coll_data, floor_flags) != FALSE)
        {
            mpProcessNetplaySoftLipXDiag("rwall_suppress", coll_data, coll_data->p_translate->x, TRUE);
            mpProcessResetMultiWallCount();
            is_collide_rwall = FALSE;
            coll_data->mask_curr &= ~MAP_FLAG_RWALL;
        }
        else
        {
            mpProcessNetplaySoftLipXDiagEx("rwall_keep", coll_data, coll_data->p_translate->x, FALSE, TRUE);
        }
    }
#endif
    if (is_collide_rwall != FALSE)
    {
        coll_data->mask_curr |= MAP_FLAG_RWALL;
    }
    return is_collide_rwall;
}

// 0x800DCAE8
void mpProcessRunRWallCollisionAdjNew(MPCollData *coll_data)
{
    MPObjectColl *map_coll = &coll_data->map_coll;
    Vec3f *translate = coll_data->p_translate;
    Vec3f wall_pos;
    Vec3f wall_angle;
    Vec3f vertex_pos;
    s32 i;
    s32 vertex_count;
    s32 j;
    u32 wall_flags;
    s32 wall_line_id;
    f32 last_wall_x;

    mpProcessSetLastWallCollideLeft();

    for (i = 0; i < sMPProcessMultiWallCollidesNum; i++)
    {
        wall_line_id = sMPProcessMultiWallCollideLineIDs[i];

        mpCollisionGetRWallEdgeU(wall_line_id, &wall_pos);

        if (wall_pos.y < (translate->y + map_coll->bottom))
        {
            if ((sMPProcessLastWallCollidePosition < wall_pos.x) && (mpCollisionGetLRCommonRWall(wall_line_id, &wall_pos, NULL, &wall_flags, &wall_angle) != FALSE))
            {
                mpProcessSetLastWallCollideStats(wall_pos.x, wall_line_id, wall_flags, &wall_angle);
            }
        }
        else
        {
            mpCollisionGetRWallEdgeD(wall_line_id, &wall_pos);

            if ((translate->y + map_coll->top) < wall_pos.y)
            {
                if ((sMPProcessLastWallCollidePosition < wall_pos.x) && (mpCollisionGetLRCommonRWall(wall_line_id, &wall_pos, NULL, &wall_flags, &wall_angle) != FALSE))
                {
                    mpProcessSetLastWallCollideStats(wall_pos.x, wall_line_id, wall_flags, &wall_angle);
                }
            }
            else
            {
                wall_pos.x = translate->x;
                wall_pos.y = translate->y + map_coll->bottom;

                if (mpCollisionGetLRCommonRWall(wall_line_id, &wall_pos, &last_wall_x, &wall_flags, &wall_angle) != FALSE)
                {
                    if ((translate->x + last_wall_x) > sMPProcessLastWallCollidePosition)
                    {
                        mpProcessSetLastWallCollideStats((translate->x + last_wall_x), wall_line_id, wall_flags, &wall_angle);
                    }
                }
                wall_pos.x = translate->x - map_coll->width;
                wall_pos.y = translate->y + map_coll->center;

                if (mpCollisionGetLRCommonRWall(wall_line_id, &wall_pos, &last_wall_x, &wall_flags, &wall_angle) != FALSE)
                {
                    if ((translate->x + last_wall_x) > sMPProcessLastWallCollidePosition)
                    {
                        mpProcessSetLastWallCollideStats((translate->x + last_wall_x), wall_line_id, wall_flags, &wall_angle);
                    }
                }
                wall_pos.x = translate->x;
                wall_pos.y = translate->y + map_coll->top;

                if (mpCollisionGetLRCommonRWall(wall_line_id, &wall_pos, &last_wall_x, &wall_flags, &wall_angle) != FALSE)
                {
                    if ((translate->x + last_wall_x) > sMPProcessLastWallCollidePosition)
                    {
                        mpProcessSetLastWallCollideStats((translate->x + last_wall_x), wall_line_id, wall_flags, &wall_angle);
                    }
                }
                vertex_count = mpCollisionGetVertexCountLineID(wall_line_id);

                for (j = 0; j < vertex_count; j++)
                {
                    mpCollisionGetVertexPositionID(wall_line_id, j, &vertex_pos);

                    if ((translate->y + map_coll->bottom) <= vertex_pos.y)
                    {
                        if (vertex_pos.y <= (translate->y + map_coll->center))
                        {
                            last_wall_x = vertex_pos.x + (((vertex_pos.y - (translate->y + map_coll->bottom)) * map_coll->width) / (map_coll->center - map_coll->bottom));

                            goto next;
                        }
                    }
                    if ((translate->y + map_coll->center) <= vertex_pos.y)
                    {
                        if (vertex_pos.y <= (translate->y + map_coll->top))
                        {
                            last_wall_x = vertex_pos.x + ((((translate->y + map_coll->top) - vertex_pos.y) * map_coll->width) / (map_coll->top - map_coll->center));

                        next:
                            if ((sMPProcessLastWallCollidePosition < last_wall_x) && (mpCollisionGetLRCommonRWall(wall_line_id, &vertex_pos, NULL, &wall_flags, &wall_angle) != FALSE))
                            {
                                mpProcessSetLastWallCollideStats(last_wall_x, wall_line_id, wall_flags, &wall_angle);
                            }
                        }
                    }
                }
            }
        }
        continue;
    }
    mpProcessGetLastWallCollideStats(&last_wall_x, &coll_data->rwall_line_id, &coll_data->rwall_flags, &coll_data->rwall_angle);

    if (translate->x < last_wall_x)
    {
#if defined(PORT) && defined(SSB64_NETMENU)
        f32 softlip_x_before = translate->x;
#endif
        translate->x = last_wall_x;

        coll_data->mask_stat |= MAP_FLAG_RWALL;
#if defined(PORT) && defined(SSB64_NETMENU)
        mpProcessNetplaySoftLipXDiag("rwall_adjnew", coll_data, softlip_x_before, FALSE);
#endif
    }
#if defined(PORT) && defined(SSB64_NETMENU)
    mpProcessNetplayHardenAdjNewTranslateSnap(coll_data, translate);
#endif
    coll_data->mask_unk |= MAP_FLAG_RWALL;
}

// 0x800DCF58
sb32 mpProcessCheckTestCeilCollisionAdjNew(MPCollData *coll_data)
{
    MPObjectColl *map_coll = &coll_data->map_coll;
    MPObjectColl *p_map_coll = coll_data->p_map_coll;
    Vec3f *translate = coll_data->p_translate;
    s32 unused;
    Vec3f sp4C;
    Vec3f sp40;
    sb32 ceil_collide;
    f32 ceil_dist;
    s32 line_id;

    coll_data->mask_stat &= ~MAP_FLAG_CEIL;

    sp4C.x = coll_data->pos_prev.x;
    sp4C.y = coll_data->pos_prev.y + p_map_coll->top;

    sp40.x = translate->x;
    sp40.y = translate->y + map_coll->top;

    ceil_collide

        = 
    (coll_data->update_tic != gMPCollisionUpdateTic)

        ?

    mpCollisionCheckCeilLineCollisionDiff(&sp4C, &sp40, &coll_data->line_coll_dist, &coll_data->ceil_line_id, &coll_data->ceil_flags, &coll_data->ceil_angle)

        :

    mpCollisionCheckCeilLineCollisionSame(&sp4C, &sp40, &coll_data->line_coll_dist, &coll_data->ceil_line_id, &coll_data->ceil_flags, &coll_data->ceil_angle);

    if (ceil_collide != FALSE)
    {
        coll_data->mask_curr |= MAP_FLAG_CEIL;

        return TRUE;
    }
    if (coll_data->mask_unk & MAP_FLAG_LWALL)
    {
        line_id = mpCollisionGetEdgeRightULineID(coll_data->lwall_line_id);

        if ((line_id != -1) && (mpCollisionGetLineTypeID(line_id) == nMPLineKindCeil) && (mpCollisionGetFCCommonCeil(line_id, &sp40, &ceil_dist, &coll_data->ceil_flags, &coll_data->ceil_angle) != FALSE) && (ceil_dist < 0.0F))
        {
            coll_data->ceil_line_id = line_id;
            coll_data->mask_curr |= MAP_FLAG_CEIL;

            return TRUE;
        }
    }
    else if (coll_data->mask_unk & MAP_FLAG_RWALL)
    {
        line_id = mpCollisionGetEdgeLeftULineID(coll_data->rwall_line_id);

        if ((line_id != -1) && (mpCollisionGetLineTypeID(line_id) == nMPLineKindCeil) && (mpCollisionGetFCCommonCeil(line_id, &sp40, &ceil_dist, &coll_data->ceil_flags, &coll_data->ceil_angle) != FALSE) && (ceil_dist < 0.0F))
        {
            coll_data->ceil_line_id = line_id;
            coll_data->mask_curr |= MAP_FLAG_CEIL;

            return TRUE;
        }
    }
    return FALSE;
}

// 0x800DD160
void mpProcessRunCeilCollisionAdjNew(MPCollData *coll_data)
{
    MPObjectColl *map_coll = &coll_data->map_coll;
    Vec3f *translate = coll_data->p_translate;
    Vec3f object_pos;
    s32 line_id;
    sb32 is_collide_ceil;
    f32 ceil_dist;

    object_pos.x = translate->x;
    object_pos.y = translate->y + map_coll->top;

    if (mpCollisionGetFCCommonCeil(coll_data->ceil_line_id, &object_pos, &ceil_dist, &coll_data->ceil_flags, &coll_data->ceil_angle) != FALSE)
    {
        translate->y += ceil_dist;
        coll_data->mask_stat |= MAP_FLAG_CEIL;

        return;
    }
    is_collide_ceil = FALSE;

    mpCollisionGetCeilEdgeL(coll_data->ceil_line_id, &object_pos);

    if (translate->x <= object_pos.x)
    {
        line_id = mpCollisionGetEdgeUpperLLineID(coll_data->ceil_line_id);

        if ((line_id != -1) && (mpCollisionGetLineTypeID(line_id) == nMPLineKindRWall))
        {
            is_collide_ceil = TRUE;
        }
    }
    else
    {
        mpCollisionGetCeilEdgeR(coll_data->ceil_line_id, &object_pos);

        line_id = mpCollisionGetEdgeUpperRLineID(coll_data->ceil_line_id);

        if ((line_id != -1) && (mpCollisionGetLineTypeID(line_id) == nMPLineKindLWall))
        {
            is_collide_ceil = TRUE;
        }
    }
    translate->y = object_pos.y - map_coll->top;

    if (is_collide_ceil != FALSE)
    {
#if defined(PORT) && defined(SSB64_NETMENU)
        /*
         * SSB64_NETMENU: stripped from offline builds. Runtime: active VS/resim only.
         * Soft-lip: edge→L/RWall attach snaps TopN.x; keep Y settle, skip X.
         * See docs/bugs/netplay_airborne_cliff_lip_ceil_edge_fc_drift_2026-07-18.md.
         */
        if (mpProcessNetplaySuppressAdjNewWallOnUnattachedSoftLip(coll_data) == FALSE)
#endif
        {
#if defined(PORT) && defined(SSB64_NETMENU)
            f32 softlip_x_before = translate->x;
#endif
            translate->x = object_pos.x;
#if defined(PORT) && defined(SSB64_NETMENU)
            mpProcessNetplayHardenAdjNewTranslateSnap(coll_data, translate);
            mpProcessNetplaySoftLipXDiag("ceil_coll_x", coll_data, softlip_x_before, FALSE);
#endif
        }
#if defined(PORT) && defined(SSB64_NETMENU)
        else
        {
            mpProcessNetplaySoftLipXDiag("ceil_coll_x_skip", coll_data, translate->x, TRUE);
        }
#endif

        mpCollisionGetFCCommonCeil(coll_data->ceil_line_id, &object_pos, NULL, &coll_data->ceil_flags, &coll_data->ceil_angle);

        coll_data->mask_stat |= MAP_FLAG_CEIL;
    }
}

// 0x800DD2C8
sb32 mpProcessCheckTestFloorCollisionAdjNew(MPCollData *coll_data, sb32(*proc_map)(GObj*), GObj *gobj)
{
    MPObjectColl *p_map_coll = coll_data->p_map_coll;
    MPObjectColl *map_coll = &coll_data->map_coll;
    Vec3f *translate = coll_data->p_translate;
    Vec3f *pos_prev = &coll_data->pos_prev;
    Vec3f sp4C;
    Vec3f sp40;
    s32 line_id;
    f32 floor_dist;
    s32 var_v0;

    coll_data->mask_stat &= ~MAP_FLAG_FLOOR;

    sp4C.x = pos_prev->x;
    sp4C.y = pos_prev->y + p_map_coll->bottom;

    sp40.x = translate->x;
    sp40.y = translate->y + map_coll->bottom;

                                                                         var_v0 
                                                                            
                                                                            = 
                                        
                                                        (coll_data->update_tic != gMPCollisionUpdateTic)

                                                                            ?

    mpCollisionCheckFloorLineCollisionDiff(&sp4C, &sp40, &coll_data->line_coll_dist, &coll_data->floor_line_id, &coll_data->floor_flags, &coll_data->floor_angle)

                                                                            :

    mpCollisionCheckFloorLineCollisionSame(&sp4C, &sp40, &coll_data->line_coll_dist, &coll_data->floor_line_id, &coll_data->floor_flags, &coll_data->floor_angle);

#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: stripped from offline builds. Netplay diagnostic only.
     * Floor-landing verdict (var_v0) drives ground/air status transitions
     * (e.g. ThrowFFall->ThrowFLanding). It is computed by Diff vs Same selected
     * purely by (update_tic != gMPCollisionUpdateTic); if two peers (or a
     * pre/post-rollback run) take different branches at the landing-contact
     * frame the verdict forks with bit-identical geometry. This probe records
     * the branch + result so a cross-peer DK cargo-carry desync can be confirmed
     * as a Diff/Same divergence vs a residual cross-ISA float compare in Diff.
     * Enable with SSB64_NETPLAY_LANDING_BRANCH_DIAG=1. */
    {
        static int s_landing_branch_diag = -1;

        if (s_landing_branch_diag < 0)
        {
            const char *env = getenv("SSB64_NETPLAY_LANDING_BRANCH_DIAG");

            s_landing_branch_diag = ((env != NULL) && (env[0] == '1')) ? 1 : 0;
        }
        if (s_landing_branch_diag != 0)
        {
            u32 tr_x_bits;
            u32 tr_y_bits;
            u32 pp_y_bits;
            u32 fdist_bits;

            memcpy(&tr_x_bits, &translate->x, sizeof(tr_x_bits));
            memcpy(&tr_y_bits, &translate->y, sizeof(tr_y_bits));
            memcpy(&pp_y_bits, &pos_prev->y, sizeof(pp_y_bits));
            memcpy(&fdist_bits, &coll_data->floor_dist, sizeof(fdist_bits));

            {
                const char *domain;
                s32 coll_player;
                s32 coll_kind;

                mpProcessNetplayCollIdentity(gobj, &domain, &coll_player, &coll_kind);
                port_log("SSB64 MpLanding: landing_branch gut=%u upt=%u domain=%s player=%d "
                         "kind=%d branch=%s vv0=%d fline=%d ignore=%d fflags=0x%08X "
                         "mask_unk=0x%04X gated=%d fdist=0x%08X tr_x=0x%08X tr_y=0x%08X "
                         "pp_y=0x%08X\n",
                         (unsigned int)gMPCollisionUpdateTic,
                         (unsigned int)coll_data->update_tic,
                         domain,
                         (int)coll_player,
                         (int)coll_kind,
                         (coll_data->update_tic != gMPCollisionUpdateTic) ? "diff" : "same",
                         (int)var_v0,
                         (int)coll_data->floor_line_id,
                         (int)coll_data->ignore_line_id,
                         (unsigned int)coll_data->floor_flags,
                         (unsigned int)coll_data->mask_unk,
                         (int)((var_v0 != 0) &&
                               (!(coll_data->floor_flags & MAP_VERTEX_COLL_PASS) ||
                                (coll_data->floor_line_id != coll_data->ignore_line_id))),
                         (unsigned int)fdist_bits,
                         (unsigned int)tr_x_bits,
                         (unsigned int)tr_y_bits,
                         (unsigned int)pp_y_bits);
            }
        }
    }
    /*
     * Only latch from a live soft floor line. Floor sweep can leave stale PASS|CLIFF
     * bits on miss (fline==-1); noting those kept suppress armed under the stage.
     */
    if (coll_data->floor_line_id != -1)
    {
	mpProcessNetplaySoftLipStickyNote(coll_data->floor_flags);
    }
#endif

    if ((var_v0 != 0) && (!(coll_data->floor_flags & MAP_VERTEX_COLL_PASS) || (coll_data->floor_line_id != coll_data->ignore_line_id)) && ((proc_map == NULL) || (proc_map(gobj) != FALSE)))
    {
        coll_data->mask_curr |= MAP_FLAG_FLOOR;

        return TRUE;
    }
    if (coll_data->mask_unk & MAP_FLAG_LWALL)
    {
        line_id = mpCollisionGetEdgeRightDLineID(coll_data->lwall_line_id);

        if (line_id != -1)
        {
            if ((mpCollisionGetLineTypeID(line_id) == nMPLineKindFloor) && (mpCollisionGetFCCommonFloor(line_id, &sp40, &floor_dist, &coll_data->floor_flags, &coll_data->floor_angle) != 0) && (floor_dist > 0.0F))
            {
                coll_data->floor_line_id = line_id;

                if (!(coll_data->floor_flags & MAP_VERTEX_COLL_PASS) || (coll_data->floor_line_id != coll_data->ignore_line_id))
                {
                    if ((proc_map == NULL) || (proc_map(gobj) != FALSE))
                    {
                        coll_data->mask_curr |= MAP_FLAG_FLOOR;

                        return TRUE;
                    }
                }
            }
        }
    }
    else if (coll_data->mask_unk & MAP_FLAG_RWALL)
    {
        line_id = mpCollisionGetEdgeLeftDLineID(coll_data->rwall_line_id);

        if (line_id != -1)
        {
            if ((mpCollisionGetLineTypeID(line_id) == nMPLineKindFloor) && (mpCollisionGetFCCommonFloor(line_id, &sp40, &floor_dist, &coll_data->floor_flags, &coll_data->floor_angle) != 0) && (floor_dist > 0.0F))
            {
                coll_data->floor_line_id = line_id;

                if (!(coll_data->floor_flags & MAP_VERTEX_COLL_PASS) || (coll_data->floor_line_id != coll_data->ignore_line_id))
                {
                    if ((proc_map == NULL) || (proc_map(gobj) != FALSE))
                    {
                        coll_data->mask_curr |= MAP_FLAG_FLOOR;

                        return TRUE;
                    }
                }
            }
        }
    }
    return FALSE;
}

// 0x800DD578
sb32 mpProcessRunFloorCollisionAdjNewNULL(MPCollData *coll_data)
{
    return mpProcessCheckTestFloorCollisionAdjNew(coll_data, NULL, NULL);
}

// 0x800DD59C
void mpProcessSetLandingFloor(MPCollData *coll_data)
{
    MPObjectColl *map_coll = &coll_data->map_coll;
    Vec3f *translate = coll_data->p_translate;
    Vec3f object_pos;
    f32 floor_dist;

    object_pos.x = translate->x;
    object_pos.y = translate->y + map_coll->bottom;

    if (mpCollisionGetFCCommonFloor(coll_data->floor_line_id, &object_pos, &floor_dist, &coll_data->floor_flags, &coll_data->floor_angle) != FALSE)
    {
        translate->y += floor_dist;
    }
    else
    {
        mpCollisionGetFloorEdgeL(coll_data->floor_line_id, &object_pos);

        if (object_pos.x <= translate->x)
        {
            mpCollisionGetFloorEdgeR(coll_data->floor_line_id, &object_pos);
        }
        translate->y = object_pos.y - map_coll->bottom;
        mpCollisionGetFCCommonFloor(coll_data->floor_line_id, &object_pos, NULL, &coll_data->floor_flags, &coll_data->floor_angle);
#if defined(PORT) && defined(SSB64_NETMENU)
        /*
         * SSB64_NETMENU: soft-lip edge landing snaps TopN.x — keep Y settle, skip X
         * (same class as RunCeilCollisionAdjNew). Flags refreshed above before gate.
         * See jumpaerial_fc_drift 2026-07-18.
         */
        if (mpProcessNetplaySuppressAdjNewWallOnUnattachedSoftLip(coll_data) == FALSE)
#endif
        {
#if defined(PORT) && defined(SSB64_NETMENU)
            f32 softlip_x_before = translate->x;
#endif
            translate->x = object_pos.x;
#if defined(PORT) && defined(SSB64_NETMENU)
            mpProcessNetplayHardenAdjNewTranslateSnap(coll_data, translate);
            mpProcessNetplaySoftLipXDiag("landing_floor_x", coll_data, softlip_x_before, FALSE);
#endif
        }
    }
    coll_data->mask_stat |= MAP_FLAG_FLOOR;
    coll_data->floor_dist = 0.0F;
#if defined(PORT) && defined(SSB64_NETMENU)
    mpProcessNetplaySoftLipStickyClearIfGrounded(coll_data);
#endif
}

// 0x800DD6A8
void mpProcessSetCollideFloor(MPCollData *coll_data)
{
    MPObjectColl *map_coll = &coll_data->map_coll;
    Vec3f *translate = coll_data->p_translate;
    Vec3f object_pos;
    s32 edge_line_id;
    s32 is_collide_floor;
    f32 floor_dist;

    object_pos.x = translate->x;
    object_pos.y = translate->y + map_coll->bottom;

    if (mpCollisionGetFCCommonFloor(coll_data->floor_line_id, &object_pos, &floor_dist, &coll_data->floor_flags, &coll_data->floor_angle) != FALSE)
    {
        translate->y += floor_dist;

        coll_data->mask_stat |= MAP_FLAG_FLOOR;
        coll_data->floor_dist = 0.0F;
#if defined(PORT) && defined(SSB64_NETMENU)
        mpProcessNetplaySoftLipStickyClearIfGrounded(coll_data);
#endif
        return;
    }
    is_collide_floor = FALSE;

    mpCollisionGetFloorEdgeL(coll_data->floor_line_id, &object_pos);

    if (translate->x <= object_pos.x)
    {
        edge_line_id = mpCollisionGetEdgeUnderLLineID(coll_data->floor_line_id);

        if ((edge_line_id != -1) && (mpCollisionGetLineTypeID(edge_line_id) == nMPLineKindRWall))
        {
            is_collide_floor = TRUE;
        }
    }
    else
    {
        mpCollisionGetFloorEdgeR(coll_data->floor_line_id, &object_pos);

        edge_line_id = mpCollisionGetEdgeUnderRLineID(coll_data->floor_line_id);

        if ((edge_line_id != -1) && (mpCollisionGetLineTypeID(edge_line_id) == nMPLineKindLWall))
        {
            is_collide_floor = TRUE;
        }
    }
    translate->y = object_pos.y - map_coll->bottom;

    if (is_collide_floor != FALSE)
    {
        mpCollisionGetFCCommonFloor(coll_data->floor_line_id, &object_pos, NULL, &coll_data->floor_flags, &coll_data->floor_angle);
#if defined(PORT) && defined(SSB64_NETMENU)
        /* SSB64_NETMENU: soft-lip collide-floor edge X snap — see SetLandingFloor. */
        if (mpProcessNetplaySuppressAdjNewWallOnUnattachedSoftLip(coll_data) == FALSE)
#endif
        {
#if defined(PORT) && defined(SSB64_NETMENU)
            f32 softlip_x_before = translate->x;
#endif
            translate->x = object_pos.x;
#if defined(PORT) && defined(SSB64_NETMENU)
            mpProcessNetplayHardenAdjNewTranslateSnap(coll_data, translate);
            mpProcessNetplaySoftLipXDiag("collide_floor_x", coll_data, softlip_x_before, FALSE);
#endif
        }

        coll_data->mask_stat |= MAP_FLAG_FLOOR;
        coll_data->floor_dist = 0.0F;
#if defined(PORT) && defined(SSB64_NETMENU)
        mpProcessNetplaySoftLipStickyClearIfGrounded(coll_data);
#endif
    }
}
