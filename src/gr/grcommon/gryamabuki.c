#include <gr/ground.h>
#include <ft/fighter.h>
#include <it/item.h>
#include <wp/weapon.h>
#include <sc/scene.h>
#include <reloc_data.h>
#ifdef PORT
extern void *func_800269C0_275C0(u16 id);
#include <stdlib.h>
#include <string.h>
#include <sys/objanim.h>
#include <sys/objhelper.h>
#endif
#if defined(PORT) && defined(SSB64_NETMENU)
#include <sys/netplay_sim_quantize.h>
extern void port_log(const char *fmt, ...);
extern u32 syNetInputGetTick(void);

/* Last child DObj frame logged by grYamabukiGateDiagLogProcAnimOnChange (see below). */
static f32 sGRYamabukiGateDiagLastChildAnimFrame = -1.0F;
#endif

/* Fully-open pose of the tower-door open joint (child_af 1..9 in soak logs). */
#define GRYAMABUKI_GATE_OPEN_HELD_FRAME 9.0F
/* Closed mesh pose: frame 0 on the OPEN joint (close joint frame 0 is the open mesh). */
#define GRYAMABUKI_GATE_CLOSED_HELD_FRAME 0.0F
/* Last frame of the close joint (open mesh -> closed mesh as af advances 0..9). */
#define GRYAMABUKI_GATE_CLOSE_END_FRAME 9.0F
/* PORT: while Open, gate_wait is reused as the minimum post-spawn egress grace timer. */
#define GRYAMABUKI_GATE_SPAWN_EGRESS_WAIT 60

#ifdef PORT
/*
 * 21st pass — held-pose drift detection (THE static-door fix).
 *
 * grYamabukiGateMeshIsHeldOpen/Closed decide whether SyncOpen/ClosedPresentation may skip the one-shot
 * re-seat. They trusted child->anim_frame ALONE (>=8.5 open, <=0.5 closed). Under rollback synctest the
 * gate DObj tree is torn down + rebuilt to its DObjDesc base pose by particle reset + RepairDObjTreeIfHollow,
 * which resets child->translate while anim_frame/anim_wait are restored separately (world fold) to the held
 * values. Result: anim_frame=9 (or 0) but translate sits at the wrong (base/closed) pose, MeshIsHeld* returns
 * TRUE on the frame check, the re-seat is skipped, and the door renders statically closed even though the sim
 * thinks it is held open. Offline this never happens (no tree rebuild); other stage hazards play their anim
 * every frame so they have no "trust the frame, skip the pose" path.
 *
 * Fix: remember the child translate.z at the true held pose (captured the moment we seat/confirm the hold)
 * and treat the mesh as held only if the live translate.z is still within tolerance of it. A rebuild that
 * resets translate now reads as drift and forces exactly ONE re-seat (HoldOpen/ClosedVisualAtEndFrame), which
 * the soak logs prove restores the correct pose (child_t.z ~320 open / ~0 closed). Hash-safe: translate is not
 * cross-peer hashed (only gate_anim_phase + quantized anim_frame/anim_wait are); the re-seat leaves those
 * unchanged, so it is pure presentation with zero determinism impact and cannot re-fire (post-seat translate
 * is back within tolerance).
 */
#define GRYAMABUKI_GATE_HELD_POSE_TOL 32.0F
#define GRYAMABUKI_GATE_HELD_REF_UNSET 1.0e30F
static f32 sGRYamabukiGateHeldOpenRefTz = GRYAMABUKI_GATE_HELD_REF_UNSET;
static f32 sGRYamabukiGateHeldClosedRefTz = GRYAMABUKI_GATE_HELD_REF_UNSET;

static sb32 grYamabukiGateHeldPoseTzMatches(f32 live_tz, f32 ref_tz)
{
	f32 d;

	if (ref_tz >= GRYAMABUKI_GATE_HELD_REF_UNSET)
	{
		/* No reference captured yet this cycle — fall back to the legacy frame-only trust. */
		return TRUE;
	}
	d = live_tz - ref_tz;
	if (d < 0.0F)
	{
		d = -d;
	}
	return (d <= GRYAMABUKI_GATE_HELD_POSE_TOL) ? TRUE : FALSE;
}
#endif

#ifdef PORT
static f32 grYamabukiGateQuantizeCompareF32(f32 value)
{
#if defined(SSB64_NETMENU)
	return syNetplayQuantizeF32(value);
#else
	return value;
#endif
}

#if defined(SSB64_NETMENU)
static sb32 grYamabukiGateDiagEnabled(void)
{
	const char *e = getenv("SSB64_NETPLAY_YAMABUKI_GATE_DIAG");

	return (e != NULL) && (e[0] != '\0') && (strcmp(e, "0") != 0);
}

/* Counts any live tower-monster projectile (Charmander flame 0x1E .. Venusaur razor 0x1F), not just
 * Hitokage's flame, so the diag reflects every rooftop Pokémon's weapon. */
static u32 grYamabukiGateCountMonsterWeapons(void)
{
	GObj *weapon_gobj;
	u32 count;

	count = 0U;
	for (weapon_gobj = gGCCommonLinks[nGCCommonLinkIDWeapon]; weapon_gobj != NULL; weapon_gobj = weapon_gobj->link_next)
	{
		WPStruct *wp = wpGetStruct(weapon_gobj);

		if ((wp != NULL) && (wp->kind >= nWPKindMonsterStart) && (wp->kind <= nWPKindMonsterEnd))
		{
			count++;
		}
	}
	return count;
}

static u32 grYamabukiGateCountEffectLinkGObjs(s32 link_id)
{
	GObj *effect_gobj;
	u32 count;

	count = 0U;
	for (effect_gobj = gGCCommonLinks[link_id]; effect_gobj != NULL; effect_gobj = effect_gobj->link_next)
	{
		count++;
	}
	return count;
}

static void grYamabukiGateDiagLogHitokage(void)
{
	GObj *monster_gobj;
	ITStruct *ip;
	DObj *dobj;
	f32 anim_frame;
	f32 anim_wait;
	s32 texture_id;

	monster_gobj = gGRCommonStruct.yamabuki.monster_gobj;
	if (monster_gobj == NULL)
	{
		return;
	}
	ip = itGetStruct(monster_gobj);
	if ((ip == NULL) || (ip->kind != nITKindHitokage))
	{
		return;
	}
	dobj = DObjGetStruct(monster_gobj);
	anim_frame = (dobj != NULL) ? dobj->anim_frame : -1.0F;
	anim_wait = (dobj != NULL) ? dobj->anim_wait : -1.0F;
	texture_id = ((dobj != NULL) && (dobj->mobj != NULL)) ? (s32)dobj->mobj->texture_id_curr : -1;
	port_log(
	    "SSB64 NetSync: yamabuki_hitokage tick=%u item_gobj=%p anim_frame=%.2f anim_wait=%.2f texture_id=%d "
	    "flags=%u flame_wait=%u offset=(%.2f,%.2f,%.2f) flame_weapons=%u effect_gobjs=%u special_effect_gobjs=%u\n",
	    (unsigned int)syNetInputGetTick(), (void *)monster_gobj, (f64)anim_frame, (f64)anim_wait, texture_id,
	    (unsigned int)ip->item_vars.hitokage.flags, (unsigned int)ip->item_vars.hitokage.flame_spawn_wait,
	    (f64)ip->item_vars.hitokage.offset.x, (f64)ip->item_vars.hitokage.offset.y,
	    (f64)ip->item_vars.hitokage.offset.z, (unsigned int)grYamabukiGateCountMonsterWeapons(),
	    (unsigned int)grYamabukiGateCountEffectLinkGObjs(nGCCommonLinkIDEffect),
	    (unsigned int)grYamabukiGateCountEffectLinkGObjs(nGCCommonLinkIDSpecialEffect));
}

/*
 * Walk the ENTIRE gate DObj tree (same order as gcAddAnimJointAll / gcGetTreeDObjNext) and log each
 * node's render-relevant fields. The single-node `yamabuki_gate child_*` sampling cannot tell whether
 * the node whose translate is animating is the same node that carries the visible display list. This
 * per-node dump exposes: which DObj has the DL (dl=1) / material (mobj=1), whether it is HIDDEN, and
 * whether the animating translate lands on the drawn node — i.e. "wrong node sampled" vs "matrix not
 * rebuilt" vs "node hidden/DL missing".
 */
static void grYamabukiGateDiagLogRenderTreeForGObj(const char *tag, GObj *gate_gobj)
{
	DObj *dobj;
	s32 idx;

	if (gate_gobj == NULL)
	{
		return;
	}
	idx = 0;
	for (dobj = DObjGetStruct(gate_gobj); (dobj != NULL) && (idx < 16); dobj = gcGetTreeDObjNext(dobj))
	{
		port_log(
		    "SSB64 NetSync: yamabuki_gate_node tag=%s tick=%u idx=%d dobj=%p t=(%.2f,%.2f,%.2f) rz=%.2f "
		    "af=%.2f aw=%.2f dl=%d mobj=%d flags=0x%02X anim_root=%d\n",
		    tag, (unsigned int)syNetInputGetTick(), idx, (void *)dobj, (f64)dobj->translate.vec.f.x,
		    (f64)dobj->translate.vec.f.y, (f64)dobj->translate.vec.f.z, (f64)dobj->rotate.vec.f.z,
		    (f64)dobj->anim_frame, (f64)dobj->anim_wait, (dobj->dv != NULL) ? 1 : 0,
		    (dobj->mobj != NULL) ? 1 : 0, (unsigned int)dobj->flags, (int)dobj->is_anim_root);
		idx++;
	}
}

static void grYamabukiGateDiagLogRenderTree(const char *tag)
{
	grYamabukiGateDiagLogRenderTreeForGObj(tag, gGRCommonStruct.yamabuki.gate_gobj);
}

/*
 * DRAW-TIME pose probe (21st pass). Every other yamabuki_gate diagnostic samples the door from
 * grYamabukiGateProcUpdate — the priority-4 SIM proc — which has proven the sim writes the correct
 * animated translate (proc_anim child_t.z 0..320). The on-screen door is built later, in the
 * priority-6 DISPLAY proc (gcDrawDObjTreeDLLinksForGObj), from dobj->translate at draw time. The
 * confirmed symptom is a static CLOSED door under synctest while proc_anim shows the open pose, so the
 * gap is strictly between the sim write and the matrix build. This wrapper logs the tree of the GObj the
 * display proc is ACTUALLY drawing (which also catches "drawn gobj != struct gate_gobj"), throttled to
 * frames where the drawn door panel's translate.z changes so it lines up tick-for-tick with proc_anim.
 */
extern void gcDrawDObjTreeDLLinksForGObj(GObj *gobj);

static void grYamabukiGateDiagLogDrawTree(GObj *gobj)
{
	static f32 s_last_draw_child_tz = -1.0e30F;
	static GObj *s_last_drawn_gobj = NULL;
	static GObj *s_last_struct_gobj = NULL;
	GObj *struct_gobj;
	DObj *root;
	DObj *child;
	f32 tz;

	if (grYamabukiGateDiagEnabled() == FALSE)
	{
		return;
	}
	struct_gobj = gGRCommonStruct.yamabuki.gate_gobj;
	root = (gobj != NULL) ? DObjGetStruct(gobj) : NULL;
	child = (root != NULL) ? root->child : NULL;
	tz = (child != NULL) ? child->translate.vec.f.z : -1.0e30F;
	/*
	 * Throttle to frames where the drawn pose moves OR the gate GObj identity changes. The identity
	 * change is the load-bearing signal a tz-only throttle hides: when a rollback re-resolves the
	 * struct gate_gobj to a different ground GObj (shared id == nGCCommonKindGround), the rendered
	 * GObj is left frozen at its last pose, so its tz never moves and the freeze goes unlogged.
	 */
	if ((tz == s_last_draw_child_tz) && (gobj == s_last_drawn_gobj) && (struct_gobj == s_last_struct_gobj))
	{
		return;
	}
	if ((gobj != s_last_drawn_gobj) || (struct_gobj != s_last_struct_gobj))
	{
		port_log("SSB64 NetSync: yamabuki_gate_draw_identity tick=%u drawn_gobj=%p (was %p) struct_gobj=%p (was %p) "
		         "drawn_display=%p struct_display=%p drawn_eq_struct=%d\n",
		         (unsigned int)syNetInputGetTick(), (void *)gobj, (void *)s_last_drawn_gobj, (void *)struct_gobj,
		         (void *)s_last_struct_gobj, (void *)((gobj != NULL) ? gobj->proc_display : NULL),
		         (void *)((struct_gobj != NULL) ? struct_gobj->proc_display : NULL),
		         (gobj == struct_gobj) ? 1 : 0);
	}
	s_last_draw_child_tz = tz;
	s_last_drawn_gobj = gobj;
	s_last_struct_gobj = struct_gobj;
	port_log("SSB64 NetSync: yamabuki_gate_draw tick=%u drawn_gobj=%p struct_gobj=%p child_tz=%.2f\n",
	         (unsigned int)syNetInputGetTick(), (void *)gobj, (void *)struct_gobj, (f64)tz);
	grYamabukiGateDiagLogRenderTreeForGObj("draw", gobj);
}

static void grYamabukiGateDrawWithDiag(GObj *gobj)
{
	grYamabukiGateDiagLogDrawTree(gobj);
	gcDrawDObjTreeDLLinksForGObj(gobj);
}

/*
 * Resolve the live gate GObj from the ground link by its DISPLAY proc instead of gobj->id.
 *
 * THE STATIC-CLOSED-DOOR ROOT CAUSE: across rollback, gate_gobj is captured/restored as gobj->id, but
 * gcMakeGObjSPAfter stamps EVERY ground GObj with id == nGCCommonKindGround. Two share that id on this
 * stage: the bare ground controller (grYamabukiMakeGround — priority-4 proc, no display proc, no DObj
 * tree) and the gate (grYamabukiMakeGate — priority-6 display proc grYamabukiGateDrawWithDiag + door
 * DObj tree). gcFindGObjByID(id) is order-dependent and can return the controller. The restore then
 * runs grYamabukiGateRepairDObjTreeIfHollow, which builds a phantom door tree on the controller; the
 * sim animates that invisible GObj while the real gate keeps rendering its stale (closed) pose, so the
 * door never visibly opens under synctest. proc_display == grYamabukiGateDrawWithDiag is unique to the
 * gate (the controller never gets gcAddGObjDisplay), so it pins the rendered gate exactly.
 */
GObj *grYamabukiGateResolveLiveGObj(void)
{
	static GObj *s_last_resolved = NULL;
	GObj *gobj;
	GObj *resolved = NULL;

	for (gobj = gGCCommonLinks[nGCCommonLinkIDGround]; gobj != NULL; gobj = gobj->link_next)
	{
		if ((gobj->id == (u32)nGCCommonKindGround) && (gobj->link_id == (u8)nGCCommonLinkIDGround) &&
		    (gobj->proc_display == grYamabukiGateDrawWithDiag))
		{
			resolved = gobj;
			break;
		}
	}
	if ((resolved != s_last_resolved) && (grYamabukiGateDiagEnabled() != FALSE))
	{
		port_log("SSB64 NetSync: yamabuki_gate_resolve tick=%u resolved=%p (was %p) prev_struct=%p struct_was_correct=%d\n",
		         (unsigned int)syNetInputGetTick(), (void *)resolved, (void *)s_last_resolved,
		         (void *)gGRCommonStruct.yamabuki.gate_gobj,
		         (resolved == gGRCommonStruct.yamabuki.gate_gobj) ? 1 : 0);
	}
	s_last_resolved = resolved;
	return resolved;
}

static void grYamabukiGateDiagLog(const char *tag)
{
	f32 yaku_x;
	f32 yaku_y;
	s32 yaku_status;
	DObj *root_dobj;
	DObj *child_dobj;
	f32 root_af;
	f32 root_aw;
	f32 child_af;
	f32 child_aw;
	f32 child_tx;
	f32 child_ty;
	f32 child_tz;
	f32 child_rz;

	if (grYamabukiGateDiagEnabled() == FALSE)
	{
		return;
	}
	yaku_x = 0.0F;
	yaku_y = 0.0F;
	yaku_status = -1;
	if ((gMPCollisionYakumonoDObjs != NULL) && (gMPCollisionYakumonoDObjs->dobjs[3] != NULL))
	{
		yaku_x = gMPCollisionYakumonoDObjs->dobjs[3]->translate.vec.f.x;
		yaku_y = gMPCollisionYakumonoDObjs->dobjs[3]->translate.vec.f.y;
		yaku_status = gMPCollisionYakumonoDObjs->dobjs[3]->user_data.s;
	}
	/* Door mesh is animated on the gate DObj tree; log root + first child anim cursor so a frozen
	 * (non-advancing) frame across ticks is distinguishable from a restore/seat problem. */
	root_dobj = (gGRCommonStruct.yamabuki.gate_gobj != NULL) ? DObjGetStruct(gGRCommonStruct.yamabuki.gate_gobj)
	                                                         : NULL;
	child_dobj = (root_dobj != NULL) ? root_dobj->child : NULL;
	root_af = (root_dobj != NULL) ? root_dobj->anim_frame : -1.0F;
	root_aw = (root_dobj != NULL) ? root_dobj->anim_wait : -1.0F;
	child_af = (child_dobj != NULL) ? child_dobj->anim_frame : -1.0F;
	child_aw = (child_dobj != NULL) ? child_dobj->anim_wait : -1.0F;
	child_tx = (child_dobj != NULL) ? child_dobj->translate.vec.f.x : -1.0F;
	child_ty = (child_dobj != NULL) ? child_dobj->translate.vec.f.y : -1.0F;
	child_tz = (child_dobj != NULL) ? child_dobj->translate.vec.f.z : -1.0F;
	child_rz = (child_dobj != NULL) ? child_dobj->rotate.vec.f.z : -1.0F;
	port_log(
	    "SSB64 NetSync: yamabuki_gate tag=%s tick=%u status=%u phase=%u gate_pos=(%.1f,%.1f) gate_wait=%u "
	    "monster_wait=%u gate_noentry=%u gate_gobj=%p gate_dobj=%p monster_gobj=%p yaku3=(%.1f,%.1f) yaku_st=%d "
	    "root_af=%.2f root_aw=%.2f child_af=%.2f child_aw=%.2f child_t=(%.2f,%.2f,%.2f) child_rz=%.2f\n",
	    tag, (unsigned int)syNetInputGetTick(), (unsigned int)gGRCommonStruct.yamabuki.gate_status,
	    (unsigned int)gGRCommonStruct.yamabuki.gate_anim_phase, gGRCommonStruct.yamabuki.gate_pos.x,
	    gGRCommonStruct.yamabuki.gate_pos.y, (unsigned int)gGRCommonStruct.yamabuki.gate_wait,
	    (unsigned int)gGRCommonStruct.yamabuki.monster_wait, (unsigned int)gGRCommonStruct.yamabuki.gate_noentry,
	    (void *)gGRCommonStruct.yamabuki.gate_gobj, (void *)root_dobj,
	    (void *)gGRCommonStruct.yamabuki.monster_gobj, yaku_x, yaku_y, yaku_status, (f64)root_af, (f64)root_aw,
	    (f64)child_af, (f64)child_aw, (f64)child_tx, (f64)child_ty, (f64)child_tz, (f64)child_rz);
	grYamabukiGateDiagLogRenderTree(tag);
	grYamabukiGateDiagLogHitokage();
}

/*
 * Log the door anim cursor right after the per-tick gcPlayAnimAll advance, but only when the child
 * (animated) frame actually changes. This samples the LIVE/resim-advanced pose — unlike the
 * "restore_done" tag which fires immediately after a frame-seat and always reads frame 0 — so a frozen
 * door (frame never moves) is now directly distinguishable from a working one in the logs.
 */
static void grYamabukiGateDiagLogProcAnimOnChange(void)
{
	DObj *root_dobj;
	DObj *child_dobj;
	f32 child_af;

	if (grYamabukiGateDiagEnabled() == FALSE)
	{
		return;
	}
	root_dobj = (gGRCommonStruct.yamabuki.gate_gobj != NULL) ? DObjGetStruct(gGRCommonStruct.yamabuki.gate_gobj)
	                                                         : NULL;
	child_dobj = (root_dobj != NULL) ? root_dobj->child : NULL;
	child_af = (child_dobj != NULL) ? child_dobj->anim_frame : -1.0F;
	if (child_af == sGRYamabukiGateDiagLastChildAnimFrame)
	{
		return;
	}
	sGRYamabukiGateDiagLastChildAnimFrame = child_af;
	grYamabukiGateDiagLog("proc_anim");
}

#endif /* SSB64_NETMENU */
#endif /* PORT */

// // // // // // // // // // // //
//                               //
//       INITIALIZED DATA        //
//                               //
// // // // // // // // // // // //

// 0x8012EB60
s32 dGRYamabukiMonsterAttackKind = GRYAMABUKI_MONSTER_WEAPON_MAX;

// 0x8012EB64
u16 dGRYamabukiMonsterMapObjKinds[/* */] =
{
    nMPMapObjKindMonster,
    nMPMapObjKindMonsterUnused2,
    nMPMapObjKindMonsterUnused3,
    nMPMapObjKindMonsterUnused4,
    nMPMapObjKindMonsterUnused1
};

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

static void grYamabukiGateSyncCollisionFromState(void);
static void grYamabukiGateOpenForSpawn(void);
#ifdef PORT
static void grYamabukiGateEnsureMapHead(void);
static sb32 grYamabukiGateRepairDObjTreeIfHollow(void);
static void grYamabukiGateUpdateOpenForRestore(void);
static void grYamabukiGateReseatAnimOpenAtFrame(f32 frame);
static void grYamabukiGateReseatAnimCloseAtFrame(f32 frame);
static sb32 grYamabukiGateMonsterIsLive(void);
static void grYamabukiGateHoldOpenVisualAtEndFrame(void);
static void grYamabukiGateHoldClosedVisualAtEndFrame(void);
static void grYamabukiGateReapplyChildHeldOpenPose(DObj *child);
static void grYamabukiGateReapplyChildHeldClosedPose(DObj *child);
static sb32 grYamabukiGateChildAnimIsActivelyPlaying(DObj *child);
static sb32 grYamabukiGateCollisionIsOpen(void);
static sb32 grYamabukiGateMeshIsHeldOpen(DObj *child);
static sb32 grYamabukiGateMeshIsHeldClosed(DObj *child);
static sb32 grYamabukiGateShouldStepDoorAnim(void);
static void grYamabukiGateSyncOpenPresentation(void);
static void grYamabukiGateSyncClosedPresentation(void);
static void grYamabukiGateSyncAnimPhaseFromLive(void);
static void grYamabukiGateBeginAnimOpen(void);
static void grYamabukiGateBeginAnimClose(void);
#endif

// 0x8010ACD0
void grYamabukiGateUpdateSleep(void)
{
    if (gSCManagerBattleState->game_status != nSCBattleGameStatusWait)
    {
        gGRCommonStruct.yamabuki.gate_status = nGRYamabukiGateStatusWait;
        gGRCommonStruct.yamabuki.monster_wait = syUtilsRandIntRange(1000) + 1000;
    }
}

// 0x8010AD18
sb32 grYamabukiGateCheckPlayersNear(void)
{
    GObj *fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];

    while (fighter_gobj != NULL)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if ((fp->ga == nMPKineticsGround) && ((fp->coll_data.floor_flags & MAP_VERTEX_MAT_MASK) == nMPMaterialDetect))
        {
            return TRUE;
        }
        else fighter_gobj = fighter_gobj->link_next;
    }
    return FALSE;
}

// 0x8010AD70
void grYamabukiGateMakeMonster(void)
{
    Vec3f pos;
    Vec3f vel;
    s32 mapobj;
    s32 item_id;

#if defined(PORT) && defined(SSB64_NETMENU)
    /* Netplay rollback only: block a second spawn while the prior tower Pokémon item is still live.
     * Rollback restore can leave monster_gobj set across resim spans; vanilla N64 always overwrites.
     * See docs/bugs/netplay_capsule_orphan_hold_yamabuki_double_spawn_2026-05-19.md. */
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        GObj *prior_monster_gobj = gGRCommonStruct.yamabuki.monster_gobj;

        if ((prior_monster_gobj != NULL) && (grYamabukiGateMonsterIsLive() != FALSE))
        {
            return;
        }
        if (prior_monster_gobj != NULL)
        {
            grYamabukiGateClearMonsterGObj();
        }
    }

#endif

    grYamabukiGateOpenForSpawn();

    gGRCommonStruct.yamabuki.gate_status = nGRYamabukiGateStatusOpen;
    gGRCommonStruct.yamabuki.gate_noentry = FALSE;
#if defined(PORT) && defined(SSB64_NETMENU)
    /* Netplay rollback only: grace ticks after spawn before doorway collision tracks the monster. */
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        gGRCommonStruct.yamabuki.gate_wait = GRYAMABUKI_GATE_SPAWN_EGRESS_WAIT;
    }

#endif

    mpCollisionGetMapObjIDsKind(dGRYamabukiMonsterMapObjKinds[0], &mapobj);
    mpCollisionGetMapObjPositionID(mapobj, &pos);

    vel.x = vel.y = vel.z = 0.0F;

    if ((dITManagerForceMonsterKind == 0) || (dITManagerForceMonsterKind > (nITKindGroundMonsterEnd - nITKindGroundMonsterStart + 1)))
    {
        item_id = syUtilsRandIntRange(nITKindGroundMonsterEnd - nITKindGroundMonsterStart + 1);

        if (item_id == gGRCommonStruct.yamabuki.monster_id_prev)
        {
            item_id = (item_id == (nITKindGroundMonsterEnd - nITKindGroundMonsterStart)) ? 0 : item_id + 1;
        }
        gGRCommonStruct.yamabuki.monster_id_prev = item_id;
    }
    else item_id = dITManagerForceMonsterKind - 1;

    gGRCommonStruct.yamabuki.monster_gobj = itManagerMakeItemSetupCommon(NULL, item_id + nITKindGroundMonsterStart, &pos, &vel, ITEM_FLAG_PARENT_GROUND);

    if ((gGRCommonStruct.yamabuki.monster_gobj != NULL) && (itGetStruct(gGRCommonStruct.yamabuki.monster_gobj) != NULL))
    {
        grYamabukiGateUpdateOpen();
        grYamabukiGateUpdateYakumonoPos();
    }
#if defined(PORT) && defined(SSB64_NETMENU)
    grYamabukiGateDiagLog("spawn_done");
#endif
}

// 0x8010AE3C
void grYamabukiGateSetPositionFar(void)
{
    gGRCommonStruct.yamabuki.gate_pos.x = 1600.0F;
    gGRCommonStruct.yamabuki.gate_pos.y = gMPCollisionYakumonoDObjs->dobjs[3]->translate.vec.f.y;
}

// 0x8010AE68
void grYamabukiGateSetPositionNear(void)
{
    gGRCommonStruct.yamabuki.gate_pos.x = 960.0F;
    gGRCommonStruct.yamabuki.gate_pos.y = gMPCollisionYakumonoDObjs->dobjs[3]->translate.vec.f.y;
}

// 0x8010AE94
void grYamabukiGateAddAnimOffset(intptr_t offset)
{
#ifdef PORT
    gcAddAnimJointAll(gGRCommonStruct.yamabuki.gate_gobj, (AObjEvent32 **)((uintptr_t)gGRCommonStruct.yamabuki.map_head + (intptr_t)offset), 0.0F);
#else
    gcAddAnimJointAll(gGRCommonStruct.yamabuki.gate_gobj, (uintptr_t)gGRCommonStruct.yamabuki.map_head + (intptr_t)offset, 0.0F);
#endif
    gcPlayAnimAll(gGRCommonStruct.yamabuki.gate_gobj);
}

// 0x8010AED8
void grYamabukiGateAddAnimOpen(void)
{
#ifdef PORT
    grYamabukiGateAddAnimOffset((intptr_t)llGRYamabukiMapGateOpenAnimJoint);
#else
    grYamabukiGateAddAnimOffset((intptr_t)&llGRYamabukiMapGateOpenAnimJoint);
#endif
}

// 0x8010AEFC
void grYamabukiGateAddAnimClose(void)
{
#ifdef PORT
    grYamabukiGateAddAnimOffset((intptr_t)llGRYamabukiMapGateCloseAnimJoint);
#else
    grYamabukiGateAddAnimOffset((intptr_t)&llGRYamabukiMapGateCloseAnimJoint);
#endif
}

// 0x8010AF20 - Allow entry inside Pokémon spawn hub?
void grYamabukiGateAddAnimOpenEntry(void)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    /* Netplay rollback only: held-open re-seat after synctest restore; vanilla uses AddAnimOpen. */
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        gGRCommonStruct.yamabuki.gate_noentry = FALSE;
        if ((grYamabukiGateCollisionIsOpen() != FALSE) && (grYamabukiGateMeshIsHeldOpen(NULL) != FALSE))
        {
            grYamabukiGateHoldOpenVisualAtEndFrame();
        }
        else
        {
            grYamabukiGateBeginAnimOpen();
        }
        grYamabukiGateSetPositionFar();
    }
    else
    {
        grYamabukiGateAddAnimOpen();
        grYamabukiGateSetPositionFar();
    }
#else
    grYamabukiGateAddAnimOpen();
    grYamabukiGateSetPositionFar();

#endif
}

// 0x8010AF48
void grYamabukiGateUpdateWait(void)
{
    if (gGRCommonStruct.yamabuki.gate_wait == 0)
    {
        if (grYamabukiGateCheckPlayersNear() != FALSE)
        {
            grYamabukiGateMakeMonster();

            return;
        }
    }
    else if (--gGRCommonStruct.yamabuki.gate_wait == 0)
    {
        grYamabukiGateAddAnimOpenEntry();
        grYamabukiGateUpdateYakumonoPos();
        func_800269C0_275C0(nSYAudioFGMYamabukiGate);
#if defined(PORT) && defined(SSB64_NETMENU)
        grYamabukiGateDiagLog("open_entry");
#endif
    }
    gGRCommonStruct.yamabuki.monster_wait--;

    if (gGRCommonStruct.yamabuki.monster_wait == 0)
    {
        grYamabukiGateMakeMonster();
    }
}

// 0x8010AFF4
void grYamabukiGateUpdateOpen(void)
{
    if (gGRCommonStruct.yamabuki.monster_gobj == NULL)
    {
        grYamabukiGateSetClosedWait();
    }
    else if (gGRCommonStruct.yamabuki.gate_noentry == FALSE)
    {
        ITStruct *ip = itGetStruct(gGRCommonStruct.yamabuki.monster_gobj);
        f32 monster_x;
        f32 coll_width;

        if (ip == NULL)
        {
            return;
        }
#if defined(PORT) && defined(SSB64_NETMENU)
        /* Netplay rollback only: count down spawn egress grace before tracking monster X. */
        if (syNetplayRollbackSemanticsActive() != FALSE)
        {
            if (gGRCommonStruct.yamabuki.gate_wait != 0)
            {
                gGRCommonStruct.yamabuki.gate_wait--;
                gGRCommonStruct.yamabuki.gate_noentry = FALSE;
                grYamabukiGateSetPositionFar();
                return;
            }
        }

#endif
        monster_x = DObjGetStruct(gGRCommonStruct.yamabuki.monster_gobj)->translate.vec.f.x;
        coll_width = ip->coll_data.map_coll.width;
#if defined(PORT) && defined(SSB64_NETMENU)
        /* Netplay rollback only: shared-grid compare for cross-peer gate_pos agreement. */
        if (syNetplaySimQuantizeActive() != FALSE)
        {
            monster_x = grYamabukiGateQuantizeCompareF32(monster_x);
            coll_width = grYamabukiGateQuantizeCompareF32(coll_width);
        }

#endif
        gGRCommonStruct.yamabuki.gate_pos.x = monster_x - coll_width;
        gGRCommonStruct.yamabuki.gate_pos.y = gMPCollisionYakumonoDObjs->dobjs[3]->translate.vec.f.y;

#if defined(PORT) && defined(SSB64_NETMENU)
        /* Netplay rollback only: keep collision open until monster clears the doorway on resim. */
        if (syNetplayRollbackSemanticsActive() != FALSE)
        {
            if (gGRCommonStruct.yamabuki.gate_pos.x < 960.0F)
            {
                gGRCommonStruct.yamabuki.gate_noentry = FALSE;
                grYamabukiGateSetPositionFar();
                grYamabukiGateDiagLog("egress_hold");
                return;
            }
        }

#endif

        if (gGRCommonStruct.yamabuki.gate_pos.x < 960.0F)
        {
            gGRCommonStruct.yamabuki.gate_pos.x = 960.0F;

            gGRCommonStruct.yamabuki.gate_noentry = TRUE;
        }
        else if (gGRCommonStruct.yamabuki.gate_pos.x > 1600.0F)
        {
            gGRCommonStruct.yamabuki.gate_pos.x = 1600.0F;
        }
#if defined(PORT) && defined(SSB64_NETMENU)
        /* Netplay rollback only: quantize tracked gate_pos on the shared sim grid. */
        else if (syNetplaySimQuantizeActive() != FALSE)
        {
            gGRCommonStruct.yamabuki.gate_pos.x = grYamabukiGateQuantizeCompareF32(gGRCommonStruct.yamabuki.gate_pos.x);
        }

#endif
    }
}

// 0x8010B0AC
void grYamabukiGateClearMonsterGObj(void)
{
    gGRCommonStruct.yamabuki.monster_gobj = NULL;
}

// 0x8010B0B8
void grYamabukiGateSetClosedWait(void)
{
    gGRCommonStruct.yamabuki.gate_status = nGRYamabukiGateStatusWait;
    gGRCommonStruct.yamabuki.gate_wait = 1000;

    gGRCommonStruct.yamabuki.monster_wait = syUtilsRandIntRange(1000) + 1000;

    grYamabukiGateSetPositionNear();
#if defined(PORT) && defined(SSB64_NETMENU)
    /* Netplay rollback only: track gate_anim_phase for held-pose sync; vanilla uses AddAnimClose. */
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        grYamabukiGateBeginAnimClose();
    }
    else
    {
        grYamabukiGateAddAnimClose();
    }
#else
    grYamabukiGateAddAnimClose();

#endif
}

// 0x8010B108
void grYamabukiGateUpdateYakumonoPos(void)
{
    mpCollisionSetYakumonoPosID(3, &gGRCommonStruct.yamabuki.gate_pos);
}

#if defined(PORT) && defined(SSB64_NETMENU)
static void grYamabukiGateEnsureMapHead(void)
{
    if (gGRCommonStruct.yamabuki.map_head != NULL)
    {
        return;
    }
    gGRCommonStruct.yamabuki.map_head =
        (void *)((uintptr_t)PORT_RESOLVE(gMPCollisionGroundData->map_nodes) - (intptr_t)llGRYamabukiMapMapHead);
}

/*
 * syNetRbSnapResetParticlesForRollback() can hollow gate_gobj (gobj->obj == NULL) while the shell
 * survives on the ground link. Rebuild the DObj tree from map_head before anim re-seat / AddAnimOpen.
 */
static sb32 grYamabukiGateRepairDObjTreeIfHollow(void)
{
    GObj *gate_gobj;
    void *map_head;

    gate_gobj = gGRCommonStruct.yamabuki.gate_gobj;
    if (gate_gobj == NULL)
    {
        return FALSE;
    }
    if (DObjGetStruct(gate_gobj) != NULL)
    {
        return TRUE;
    }
    grYamabukiGateEnsureMapHead();
    map_head = gGRCommonStruct.yamabuki.map_head;
    if (map_head == NULL)
    {
        return FALSE;
    }
    gcRemoveDObjAll(gate_gobj);
    gcSetupCustomDObjs(
        gate_gobj,
        (DObjDesc *)((uintptr_t)map_head + (intptr_t)llGRYamabukiMapMapHead),
        NULL,
        nGCMatrixKindTraRotRpyR,
        nGCMatrixKindNull,
        nGCMatrixKindNull);
    return (DObjGetStruct(gate_gobj) != NULL) ? TRUE : FALSE;
}

/*
 * Rollback restore must recompute gate_pos from the live monster even when gate_noentry was saved
 * TRUE — forward UpdateOpen stops updating once latched, but a stale blob can leave collision at
 * 960 while status=Open and the rooftop Pokémon is still out.
 *
 * Mirrors MakeMonster: clear noentry, open collision to far, then track monster (never re-latch
 * noentry here — forward sim re-latches when the monster reaches the doorway).
 */
static void grYamabukiGateUpdateOpenForRestore(void)
{
    ITStruct *ip;
    f32 tracked_x;

    if (gGRCommonStruct.yamabuki.monster_gobj == NULL)
    {
        grYamabukiGateSetClosedWait();
        return;
    }
    ip = itGetStruct(gGRCommonStruct.yamabuki.monster_gobj);
    if (ip == NULL)
    {
        return;
    }
    gGRCommonStruct.yamabuki.gate_noentry = FALSE;
    grYamabukiGateSetPositionFar();
    if (gGRCommonStruct.yamabuki.gate_wait != 0)
    {
        return;
    }

    tracked_x = DObjGetStruct(gGRCommonStruct.yamabuki.monster_gobj)->translate.vec.f.x - ip->coll_data.map_coll.width;
#if defined(PORT) && defined(SSB64_NETMENU)
    if (syNetplaySimQuantizeActive() != FALSE)
    {
        tracked_x =
            grYamabukiGateQuantizeCompareF32(DObjGetStruct(gGRCommonStruct.yamabuki.monster_gobj)->translate.vec.f.x) -
            grYamabukiGateQuantizeCompareF32(ip->coll_data.map_coll.width);
    }

#endif
    gGRCommonStruct.yamabuki.gate_pos.y = gMPCollisionYakumonoDObjs->dobjs[3]->translate.vec.f.y;

    if (tracked_x < 960.0F)
    {
        grYamabukiGateSetPositionFar();
    }
    else
    {
        gGRCommonStruct.yamabuki.gate_pos.x = tracked_x;
        if (gGRCommonStruct.yamabuki.gate_pos.x > 1600.0F)
        {
            gGRCommonStruct.yamabuki.gate_pos.x = 1600.0F;
        }
#if defined(PORT) && defined(SSB64_NETMENU)
        else if (syNetplaySimQuantizeActive() != FALSE)
        {
            gGRCommonStruct.yamabuki.gate_pos.x = grYamabukiGateQuantizeCompareF32(gGRCommonStruct.yamabuki.gate_pos.x);
        }

#endif
    }
    gGRCommonStruct.yamabuki.gate_noentry = FALSE;
}

static sb32 grYamabukiGateMonsterIsLive(void)
{
    if (gGRCommonStruct.yamabuki.monster_gobj == NULL)
    {
        return FALSE;
    }
    return (itGetStruct(gGRCommonStruct.yamabuki.monster_gobj) != NULL) ? TRUE : FALSE;
}

static sb32 grYamabukiGateCollisionIsOpen(void)
{
    return (gGRCommonStruct.yamabuki.gate_pos.x >= 1280.0F) ? TRUE : FALSE;
}

static sb32 grYamabukiGateChildAnimIsActivelyPlaying(DObj *child)
{
    if (child == NULL)
    {
        return FALSE;
    }
    return (child->anim_wait != AOBJ_ANIM_NULL) ? TRUE : FALSE;
}

static sb32 grYamabukiGateMeshIsHeldOpen(DObj *child)
{
    if (child == NULL)
    {
        GObj *gate_gobj = gGRCommonStruct.yamabuki.gate_gobj;
        DObj *root;

        if (gate_gobj == NULL)
        {
            return FALSE;
        }
        root = DObjGetStruct(gate_gobj);
        child = (root != NULL) ? root->child : NULL;
        if (child == NULL)
        {
            return FALSE;
        }
    }
    if (grYamabukiGateChildAnimIsActivelyPlaying(child) != FALSE)
    {
        return FALSE;
    }
    return (child->anim_frame >= (GRYAMABUKI_GATE_OPEN_HELD_FRAME - 0.5F)) ? TRUE : FALSE;
}

static sb32 grYamabukiGateMeshIsHeldClosed(DObj *child)
{
    if (child == NULL)
    {
        GObj *gate_gobj = gGRCommonStruct.yamabuki.gate_gobj;
        DObj *root;

        if (gate_gobj == NULL)
        {
            return FALSE;
        }
        root = DObjGetStruct(gate_gobj);
        child = (root != NULL) ? root->child : NULL;
        if (child == NULL)
        {
            return FALSE;
        }
    }
    if (grYamabukiGateChildAnimIsActivelyPlaying(child) != FALSE)
    {
        return FALSE;
    }
    /* Held closed uses the open joint at frame 0; a completed close joint may still read frame 9. */
    if (child->anim_frame <= (GRYAMABUKI_GATE_CLOSED_HELD_FRAME + 0.5F))
    {
        return TRUE;
    }
    if (child->anim_frame >= (GRYAMABUKI_GATE_CLOSE_END_FRAME - 0.5F))
    {
        return TRUE;
    }
    return FALSE;
}

/*
 * NULL-wait held joints wrap 9->0 (open) or 0->9 (close) when gcPlayAnimAll runs; skip stepping while
 * the mesh is frozen in OpenHeld / Closed so presentation stays aligned with open collision.
 */
static sb32 grYamabukiGateShouldStepDoorAnim(void)
{
    switch (gGRCommonStruct.yamabuki.gate_anim_phase)
    {
    case nGRYamabukiGateAnimPhaseOpenHeld:
    case nGRYamabukiGateAnimPhaseClosed:
        return FALSE;

    default:
        return TRUE;
    }
}

static void grYamabukiGateBeginAnimOpen(void)
{
    grYamabukiGateAddAnimOpen();
    gGRCommonStruct.yamabuki.gate_anim_phase = nGRYamabukiGateAnimPhaseOpening;
}

static void grYamabukiGateBeginAnimClose(void)
{
    grYamabukiGateAddAnimClose();
    gGRCommonStruct.yamabuki.gate_anim_phase = nGRYamabukiGateAnimPhaseClosing;
}

static void grYamabukiGateReseatAnimCloseAtFrame(f32 frame)
{
    gcAddAnimJointAll(gGRCommonStruct.yamabuki.gate_gobj,
                      (AObjEvent32 **)((uintptr_t)gGRCommonStruct.yamabuki.map_head +
                                       (intptr_t)llGRYamabukiMapGateCloseAnimJoint),
                      frame);
    gcPlayAnimAll(gGRCommonStruct.yamabuki.gate_gobj);
}

static void grYamabukiGateReapplyChildHeldOpenPose(DObj *child)
{
    if (child == NULL)
    {
        return;
    }
    gcApplyDObjAnimJointPoseAtFrame(child, GRYAMABUKI_GATE_OPEN_HELD_FRAME, TRUE);
    sGRYamabukiGateDiagLastChildAnimFrame = GRYAMABUKI_GATE_OPEN_HELD_FRAME;
}

static void grYamabukiGateReapplyChildHeldClosedPose(DObj *child)
{
    GObj *gate_gobj;
    DObj *root;

    gate_gobj = gGRCommonStruct.yamabuki.gate_gobj;
    if ((gate_gobj == NULL) || (child == NULL))
    {
        return;
    }
    grYamabukiGateReseatAnimOpenAtFrame(GRYAMABUKI_GATE_CLOSED_HELD_FRAME);
    root = DObjGetStruct(gate_gobj);
    child = (root != NULL) ? root->child : NULL;
    if (child != NULL)
    {
        gcApplyDObjAnimJointPoseAtFrame(child, GRYAMABUKI_GATE_CLOSED_HELD_FRAME, TRUE);
        sGRYamabukiGateDiagLastChildAnimFrame = GRYAMABUKI_GATE_CLOSED_HELD_FRAME;
    }
}

/*
 * Seat the open joint at its fully-open frame and freeze the child cursor there.
 * The open anim wraps frame 9 -> 0 (closed mesh) when it completes; while GateStatusOpen
 * and the rooftop Pokémon is still alive we must hold frame 9 so synctest restore (which
 * faithfully restores anim_wait=AOBJ_ANIM_NULL at frame 0) does not leave a closed door.
 */
static void grYamabukiGateHoldOpenVisualAtEndFrame(void)
{
    GObj *gate_gobj;
    DObj *root;
    DObj *child;

    gate_gobj = gGRCommonStruct.yamabuki.gate_gobj;
    if (gate_gobj == NULL)
    {
        return;
    }
    grYamabukiGateReseatAnimOpenAtFrame(GRYAMABUKI_GATE_OPEN_HELD_FRAME);
    root = DObjGetStruct(gate_gobj);
    child = (root != NULL) ? root->child : NULL;
    if (child != NULL)
    {
        child->anim_frame = GRYAMABUKI_GATE_OPEN_HELD_FRAME;
        child->anim_wait = AOBJ_ANIM_NULL;
        sGRYamabukiGateDiagLastChildAnimFrame = GRYAMABUKI_GATE_OPEN_HELD_FRAME;
        sGRYamabukiGateHeldOpenRefTz = child->translate.vec.f.z;
    }
    if (gate_gobj->anim_frame != GRYAMABUKI_GATE_OPEN_HELD_FRAME)
    {
        gate_gobj->anim_frame = GRYAMABUKI_GATE_OPEN_HELD_FRAME;
    }
    gGRCommonStruct.yamabuki.gate_anim_phase = nGRYamabukiGateAnimPhaseOpenHeld;
}

static void grYamabukiGateHoldClosedVisualAtEndFrame(void)
{
    GObj *gate_gobj;
    DObj *root;
    DObj *child;

    gate_gobj = gGRCommonStruct.yamabuki.gate_gobj;
    if (gate_gobj == NULL)
    {
        return;
    }
    grYamabukiGateReseatAnimOpenAtFrame(GRYAMABUKI_GATE_CLOSED_HELD_FRAME);
    root = DObjGetStruct(gate_gobj);
    child = (root != NULL) ? root->child : NULL;
    if (child != NULL)
    {
        child->anim_frame = GRYAMABUKI_GATE_CLOSED_HELD_FRAME;
        child->anim_wait = AOBJ_ANIM_NULL;
        sGRYamabukiGateDiagLastChildAnimFrame = GRYAMABUKI_GATE_CLOSED_HELD_FRAME;
        sGRYamabukiGateHeldClosedRefTz = child->translate.vec.f.z;
    }
    if (gate_gobj->anim_frame != GRYAMABUKI_GATE_CLOSED_HELD_FRAME)
    {
        gate_gobj->anim_frame = GRYAMABUKI_GATE_CLOSED_HELD_FRAME;
    }
    gGRCommonStruct.yamabuki.gate_anim_phase = nGRYamabukiGateAnimPhaseClosed;
}

/*
 * While collision is open (Wait+far or Open+live monster), keep the door mesh at the fully-open
 * pose whenever the open anim is not actively playing. Completed anim wraps to frame 0 (closed mesh)
 * even though yakumono id 3 stays at 1600+ — this matches the post-open_entry / post-spawn windows
 * where synctest restore otherwise leaves a closed mesh with open collision.
 */
static void grYamabukiGateSyncOpenPresentation(void)
{
    DObj *root;
    DObj *child;
    sb32 collision_open;

    if (gGRCommonStruct.yamabuki.gate_gobj == NULL)
    {
        return;
    }
    root = DObjGetStruct(gGRCommonStruct.yamabuki.gate_gobj);
    child = (root != NULL) ? root->child : NULL;
    if (child == NULL)
    {
        return;
    }
    collision_open = FALSE;
    if ((gGRCommonStruct.yamabuki.gate_status == nGRYamabukiGateStatusOpen) &&
        (grYamabukiGateMonsterIsLive() != FALSE))
    {
        collision_open = TRUE;
    }
    else if ((gGRCommonStruct.yamabuki.gate_status == nGRYamabukiGateStatusWait) &&
             (gGRCommonStruct.yamabuki.gate_pos.x >= 1280.0F))
    {
        collision_open = TRUE;
    }
    if (collision_open == FALSE)
    {
        return;
    }
    if (grYamabukiGateChildAnimIsActivelyPlaying(child) != FALSE)
    {
        return;
    }
    if (gGRCommonStruct.yamabuki.gate_anim_phase == nGRYamabukiGateAnimPhaseOpenHeld)
    {
        if (grYamabukiGateMeshIsHeldOpen(child) != FALSE)
        {
#ifdef PORT
            /* anim_frame says held-open; only trust it if the mesh translate is still at the captured
             * open pose. A rollback tree-rebuild resets translate while anim_frame stays 9 — detect that
             * drift and fall through to re-seat (THE static-closed-door fix). */
            if (grYamabukiGateHeldPoseTzMatches(child->translate.vec.f.z, sGRYamabukiGateHeldOpenRefTz) != FALSE)
            {
                return;
            }
            grYamabukiGateDiagLog("open_held_pose_drift");
#else
            return;
#endif
        }
        else
        {
            grYamabukiGateDiagLog("open_held_repair");
        }
    }
    /*
     * GateOpen uses TraI on the child joint — child_tx alone stays flat while the mesh moves.
     * Seat once on transition into OpenHeld; per-tick gcParse on a NULL-wait held joint corrupts
     * TraI translate (soak: child_tz runaway during egress_hold).
     */
    grYamabukiGateHoldOpenVisualAtEndFrame();
}

/*
 * While collision is closed (Wait+near), keep the door mesh at the fully-closed pose whenever the
 * close anim is not actively playing. Without this, a completed close anim wraps the close joint
 * 9->0 (open mesh on the close joint) while yakumono id 3 is already back at 960.
 */
static void grYamabukiGateSyncClosedPresentation(void)
{
    DObj *root;
    DObj *child;

    if (gGRCommonStruct.yamabuki.gate_gobj == NULL)
    {
        return;
    }
    if ((gGRCommonStruct.yamabuki.gate_status != nGRYamabukiGateStatusWait) ||
        (grYamabukiGateCollisionIsOpen() != FALSE))
    {
        return;
    }
    root = DObjGetStruct(gGRCommonStruct.yamabuki.gate_gobj);
    child = (root != NULL) ? root->child : NULL;
    if (child == NULL)
    {
        return;
    }
    if (grYamabukiGateChildAnimIsActivelyPlaying(child) != FALSE)
    {
        return;
    }
    if (gGRCommonStruct.yamabuki.gate_anim_phase == nGRYamabukiGateAnimPhaseClosed)
    {
        if (grYamabukiGateMeshIsHeldClosed(child) != FALSE)
        {
#ifdef PORT
            /* Mirror of the open-held drift guard: a rebuild can leave translate at the base/open pose
             * while anim_frame reads 0, which would otherwise render an open mesh over closed collision. */
            if (grYamabukiGateHeldPoseTzMatches(child->translate.vec.f.z, sGRYamabukiGateHeldClosedRefTz) != FALSE)
            {
                return;
            }
            grYamabukiGateDiagLog("closed_held_pose_drift");
#else
            return;
#endif
        }
        else
        {
            grYamabukiGateDiagLog("closed_held_repair");
        }
    }
    grYamabukiGateHoldClosedVisualAtEndFrame();
}

static void grYamabukiGateSyncAnimPhaseFromLive(void)
{
    DObj *root;
    DObj *child;

    if (gGRCommonStruct.yamabuki.gate_gobj == NULL)
    {
        return;
    }
    root = DObjGetStruct(gGRCommonStruct.yamabuki.gate_gobj);
    child = (root != NULL) ? root->child : NULL;
    if (child == NULL)
    {
        return;
    }
    if (grYamabukiGateCollisionIsOpen() != FALSE)
    {
        if (grYamabukiGateChildAnimIsActivelyPlaying(child) != FALSE)
        {
            gGRCommonStruct.yamabuki.gate_anim_phase = nGRYamabukiGateAnimPhaseOpening;
        }
        else if (child->anim_frame >= (GRYAMABUKI_GATE_OPEN_HELD_FRAME - 0.5F))
        {
            gGRCommonStruct.yamabuki.gate_anim_phase = nGRYamabukiGateAnimPhaseOpenHeld;
#ifdef PORT
            /* Anim has settled at the open frame; SyncOpenPresentation already ran this tick (ahead of us
             * in ProcUpdate / FinalizeAfterSnapshotRestore) and repaired any drift, so the live translate is
             * the authoritative held-open pose. Snapshot it as the drift reference for subsequent ticks. */
            sGRYamabukiGateHeldOpenRefTz = child->translate.vec.f.z;
#endif
        }
        else
        {
            gGRCommonStruct.yamabuki.gate_anim_phase = nGRYamabukiGateAnimPhaseOpening;
        }
    }
    else if (grYamabukiGateChildAnimIsActivelyPlaying(child) != FALSE)
    {
        gGRCommonStruct.yamabuki.gate_anim_phase = nGRYamabukiGateAnimPhaseClosing;
    }
    else
    {
        gGRCommonStruct.yamabuki.gate_anim_phase = nGRYamabukiGateAnimPhaseClosed;
#ifdef PORT
        sGRYamabukiGateHeldClosedRefTz = child->translate.vec.f.z;
#endif
    }
}

void grYamabukiGateFinalizeAfterSnapshotRestore(void)
{
    if ((gGRCommonStruct.yamabuki.gate_status == nGRYamabukiGateStatusOpen) &&
        (grYamabukiGateMonsterIsLive() != FALSE))
    {
        gGRCommonStruct.yamabuki.gate_noentry = FALSE;
    }
    else if ((gGRCommonStruct.yamabuki.gate_status == nGRYamabukiGateStatusWait) &&
             (gGRCommonStruct.yamabuki.gate_pos.x >= 1280.0F))
    {
        gGRCommonStruct.yamabuki.gate_noentry = FALSE;
    }
    grYamabukiGateSyncCollisionFromState();
    grYamabukiGateSyncOpenPresentation();
    grYamabukiGateSyncClosedPresentation();
    grYamabukiGateSyncAnimPhaseFromLive();
    grYamabukiGateDiagLog("restore_final");
}

#endif

/*
 * gate_pos (yakumono id 3) is collision; gate_gobj anim is presentation only.
 */
static void grYamabukiGateSyncCollisionFromState(void)
{
    switch (gGRCommonStruct.yamabuki.gate_status)
    {
    case nGRYamabukiGateStatusOpen:
        if ((gGRCommonStruct.yamabuki.monster_gobj != NULL) &&
            (itGetStruct(gGRCommonStruct.yamabuki.monster_gobj) != NULL))
        {
#ifdef PORT
#if defined(SSB64_NETMENU)
            grYamabukiGateUpdateOpenForRestore();
#else
            grYamabukiGateUpdateOpen();
#endif
#else
            grYamabukiGateUpdateOpen();
#endif
        }
        else
        {
            grYamabukiGateSetPositionFar();
        }
        break;

    case nGRYamabukiGateStatusWait:
        if (gGRCommonStruct.yamabuki.gate_pos.x >= 1280.0F)
        {
            grYamabukiGateSetPositionFar();
        }
        else
        {
            grYamabukiGateSetPositionNear();
        }
        break;

    default:
        grYamabukiGateSetPositionNear();
        break;
    }
    grYamabukiGateUpdateYakumonoPos();
}

static void grYamabukiGateOpenForSpawn(void)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    /* Netplay rollback only: re-seat held-open mesh after restore; vanilla uses AddAnimOpen. */
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        if (grYamabukiGateCollisionIsOpen() != FALSE)
        {
            if (grYamabukiGateMeshIsHeldOpen(NULL) != FALSE)
            {
                grYamabukiGateHoldOpenVisualAtEndFrame();
            }
            else
            {
                /* open_entry finished long ago but mesh wrapped to closed — replay the slide. */
                grYamabukiGateBeginAnimOpen();
            }
        }
        else
        {
            grYamabukiGateBeginAnimOpen();
        }
    }
    else
    {
        grYamabukiGateAddAnimOpen();
    }
#else
    grYamabukiGateAddAnimOpen();

#endif
    func_800269C0_275C0(nSYAudioFGMYamabukiGate);
    grYamabukiGateSetPositionFar();
    grYamabukiGateUpdateYakumonoPos();
#if defined(PORT) && defined(SSB64_NETMENU)
    grYamabukiGateDiagLog("spawn_open");
#endif
}

#if defined(PORT) && defined(SSB64_NETMENU)
/*
 * AddAnimOpen() hardcodes start frame 0.0F (correct for a live spawn that animates closed->open).
 * On rollback restore the door is mid-open: re-seat the SAME open joint at the snapshot's saved
 * anim_frame so the mesh holds its restored pose. Without this, per-frame synctest rollback re-seats
 * the open anim at frame 0 (the closed pose) every frame and the door never visually opens.
 */
static void grYamabukiGateReseatAnimOpenAtFrame(f32 frame)
{
    gcAddAnimJointAll(gGRCommonStruct.yamabuki.gate_gobj,
                      (AObjEvent32 **)((uintptr_t)gGRCommonStruct.yamabuki.map_head +
                                       (intptr_t)llGRYamabukiMapGateOpenAnimJoint),
                      frame);
    gcPlayAnimAll(gGRCommonStruct.yamabuki.gate_gobj);
}

void grYamabukiGateRestoreAfterRollback(f32 restore_anim_frame, f32 restore_anim_wait, u8 restore_anim_phase,
                                        sb32 has_restore_anim)
{
    GObj *gate_gobj;
    DObj *dobj;
    sb32 repaired;

    gate_gobj = gGRCommonStruct.yamabuki.gate_gobj;
    if (gate_gobj == NULL)
    {
        grYamabukiGateSyncCollisionFromState();
        grYamabukiGateDiagLog("restore_no_gobj");
        return;
    }
    repaired = grYamabukiGateRepairDObjTreeIfHollow();
    dobj = DObjGetStruct(gate_gobj);
    if (dobj == NULL)
    {
        grYamabukiGateSyncCollisionFromState();
        grYamabukiGateDiagLog(repaired != FALSE ? "restore_repair_fail" : "restore_no_dobj");
        return;
    }

    if (has_restore_anim != FALSE)
    {
        DObj *child;

        dobj->anim_frame = restore_anim_frame;
        dobj->anim_wait = restore_anim_wait;
        child = dobj->child;
        if (child != NULL)
        {
            child->anim_frame = restore_anim_frame;
            child->anim_wait = restore_anim_wait;
        }
        if (gate_gobj->anim_frame != restore_anim_frame)
        {
            gate_gobj->anim_frame = restore_anim_frame;
        }
        gGRCommonStruct.yamabuki.gate_anim_phase = restore_anim_phase;
    }

    switch (gGRCommonStruct.yamabuki.gate_status)
    {
    case nGRYamabukiGateStatusOpen:
        if ((gGRCommonStruct.yamabuki.monster_gobj != NULL) &&
            (itGetStruct(gGRCommonStruct.yamabuki.monster_gobj) != NULL))
        {
            gGRCommonStruct.yamabuki.gate_noentry = FALSE;
        }
        /* Anim re-seat deferred to syNetRbSnapApplyYamabukiGate + FinalizeAfterSnapshotRestore. */
        break;

    case nGRYamabukiGateStatusWait:
        if (gGRCommonStruct.yamabuki.gate_pos.x < 1280.0F)
        {
            switch (restore_anim_phase)
            {
            case nGRYamabukiGateAnimPhaseClosing:
                grYamabukiGateReseatAnimCloseAtFrame(restore_anim_frame);
                break;

            case nGRYamabukiGateAnimPhaseClosed:
                if ((has_restore_anim != FALSE) && (restore_anim_wait == AOBJ_ANIM_NULL) &&
                    (restore_anim_frame <= (GRYAMABUKI_GATE_CLOSED_HELD_FRAME + 0.5F)))
                {
                    grYamabukiGateHoldClosedVisualAtEndFrame();
                }
                else
                {
                    grYamabukiGateReseatAnimCloseAtFrame(restore_anim_frame);
                }
                break;

            default:
                grYamabukiGateReseatAnimCloseAtFrame(restore_anim_frame);
                break;
            }
        }
        /* Wait+far: collision only here; open mesh handled in FinalizeAfterSnapshotRestore. */
        break;

    default:
        if (has_restore_anim != FALSE)
        {
            grYamabukiGateReseatAnimCloseAtFrame(restore_anim_frame);
        }
        else
        {
            grYamabukiGateHoldClosedVisualAtEndFrame();
        }
        break;
    }

    grYamabukiGateSyncCollisionFromState();
    grYamabukiGateDiagLog("restore_done");
}

#endif

// 0x8010B130
void grYamabukiGateProcUpdate(GObj *ground_gobj)
{
    switch (gGRCommonStruct.yamabuki.gate_status)
    {
    case nGRYamabukiGateStatusSleep:
        grYamabukiGateUpdateSleep();
        break;

    case nGRYamabukiGateStatusWait:
        grYamabukiGateUpdateWait();
        grYamabukiGateUpdateYakumonoPos();
        break;

    case nGRYamabukiGateStatusOpen:
        grYamabukiGateUpdateOpen();
        grYamabukiGateUpdateYakumonoPos();
        break;
    }
#if defined(PORT) && defined(SSB64_NETMENU)
    if (gGRCommonStruct.yamabuki.gate_gobj != NULL)
    {
        /* Netplay rollback only: step door anim from priority-4 proc + held-pose presentation sync. */
        if (syNetplayRollbackSemanticsActive() != FALSE)
        {
            /*
             * Step the door open/close anim once per sim tick from this priority-4 ground proc (which re-runs
             * during rollback resim) instead of the dropped priority-5 gate-GObj process.
             */
            if (grYamabukiGateShouldStepDoorAnim() != FALSE)
            {
                gcPlayAnimAll(gGRCommonStruct.yamabuki.gate_gobj);
            }
            grYamabukiGateSyncOpenPresentation();
            grYamabukiGateSyncClosedPresentation();
            grYamabukiGateSyncAnimPhaseFromLive();
            grYamabukiGateDiagLogProcAnimOnChange();
        }
        else
        {
            /* Vanilla forward sim: substitute for the priority-5 gcPlayAnimAll gate GObj process. */
            gcPlayAnimAll(gGRCommonStruct.yamabuki.gate_gobj);
        }
    }

#endif
}

void grYamabukiMakeGate(void)
{
    GObj *gate_gobj;

    gGRCommonStruct.yamabuki.gate_gobj = gate_gobj = gcMakeGObjSPAfter(nGCCommonKindGround, NULL, nGCCommonLinkIDGround, GOBJ_PRIORITY_DEFAULT);

#ifdef PORT
#if defined(SSB64_NETMENU)
    /* DRAW-TIME pose probe wrapper (env SSB64_NETPLAY_YAMABUKI_GATE_DIAG). */
    gcAddGObjDisplay(gate_gobj, grYamabukiGateDrawWithDiag, 6, GOBJ_PRIORITY_DEFAULT, ~0);
#else
    gcAddGObjDisplay(gate_gobj, gcDrawDObjTreeDLLinksForGObj, 6, GOBJ_PRIORITY_DEFAULT, ~0);
#endif
#else
    gcAddGObjDisplay(gate_gobj, gcDrawDObjTreeDLLinksForGObj, 6, GOBJ_PRIORITY_DEFAULT, ~0);
#endif
    gcSetupCustomDObjs
    (
        gate_gobj, 
        (DObjDesc*) 
#ifdef PORT
        ((uintptr_t)gGRCommonStruct.yamabuki.map_head + (intptr_t)llGRYamabukiMapMapHead), 
#else
        ((uintptr_t)gGRCommonStruct.yamabuki.map_head + (intptr_t)&llGRYamabukiMapMapHead), 
#endif
        NULL, 
        nGCMatrixKindTraRotRpyR, 
        nGCMatrixKindNull, 
        nGCMatrixKindNull
    );
#ifdef PORT
    /*
     * PORT: do NOT register the door anim as a separate priority-5 GObj process on the gate GObj.
     * Under rollback netplay that process does not advance the door across the resim cycle (the
     * gate GObj's process list does not re-run in lockstep with the rolled-back/resimmed sim, while
     * the priority-4 ground proc — grYamabukiGateProcUpdate — does). Instead the door anim is stepped
     * once per sim tick at the end of grYamabukiGateProcUpdate so the open/close pose is a pure
     * function of the (snapshotted, hashed) gate state and reproduces deterministically on resim.
     */
#else
    gcAddGObjProcess(gate_gobj, gcPlayAnimAll, nGCProcessKindFunc, 5);
#endif
    grYamabukiGateAddAnimClose();
}

// 0x8010B250
void grYamabukiInitGroundVars(void)
{
	/* Union overlay: monster_gobj shares memory with other stages' cached GObj* slots. */
	gGRCommonStruct.yamabuki.monster_gobj = NULL;
#ifdef PORT
    gGRCommonStruct.yamabuki.map_head = (void*) ((uintptr_t)PORT_RESOLVE(gMPCollisionGroundData->map_nodes) - (intptr_t)llGRYamabukiMapMapHead);
#else
    gGRCommonStruct.yamabuki.map_head = (void*) ((uintptr_t)gMPCollisionGroundData->map_nodes - (intptr_t)&llGRYamabukiMapMapHead);
#endif

    mpCollisionSetYakumonoOnID(3);

    gGRCommonStruct.yamabuki.gate_wait = 1;
#ifdef PORT
    gGRCommonStruct.yamabuki.item_head = (void*) ((uintptr_t)gMPCollisionGroundData - (intptr_t)llGRYamabukiMapItemHead);
#else
    gGRCommonStruct.yamabuki.item_head = (void*) ((uintptr_t)gMPCollisionGroundData - (intptr_t)&llGRYamabukiMapItemHead);
#endif

    dGRYamabukiMonsterAttackKind = GRYAMABUKI_MONSTER_WEAPON_MAX;

    gGRCommonStruct.yamabuki.monster_id_prev = (nITKindGroundMonsterEnd - nITKindGroundMonsterStart) + 1;
    gGRCommonStruct.yamabuki.gate_pos.z = 0.0F;
#ifdef PORT
    gGRCommonStruct.yamabuki.gate_anim_phase = nGRYamabukiGateAnimPhaseClosed;
#endif

    grYamabukiGateSetPositionNear();
    grYamabukiMakeGate();
    grYamabukiGateUpdateYakumonoPos();

    gGRCommonStruct.yamabuki.gate_status = 0;
}

// 0x8010B2EC
GObj* grYamabukiMakeGround(void)
{
    GObj *ground_gobj = gcMakeGObjSPAfter(nGCCommonKindGround, NULL, nGCCommonLinkIDGround, GOBJ_PRIORITY_DEFAULT);

    gcAddGObjProcess(ground_gobj, grYamabukiGateProcUpdate, nGCProcessKindFunc, 4);
    grYamabukiInitGroundVars();

    return ground_gobj;
}
