#ifndef _GRYAMABUKI_H_
#define _GRYAMABUKI_H_

#include <ssb_types.h>
#include <sys/objdef.h>
#include <gr/grdef.h>

typedef enum GRYamabukiGateStatus
{
    nGRYamabukiGateStatusSleep,
    nGRYamabukiGateStatusWait, /* gates opening + lights on still counts as Wait */
    nGRYamabukiGateStatusOpen  /* fully open when a Pokémon appears */
} GRYamabukiGateStatus;

#ifdef PORT
typedef enum GRYamabukiGateAnimPhase
{
    nGRYamabukiGateAnimPhaseClosed,
    nGRYamabukiGateAnimPhaseOpening,
    nGRYamabukiGateAnimPhaseOpenHeld,
    nGRYamabukiGateAnimPhaseClosing
} GRYamabukiGateAnimPhase;
#endif

extern void grYamabukiGateUpdateSleep(void);
extern sb32 grYamabukiGateCheckPlayersNear(void);
extern void grYamabukiGateMakeMonster(void);
extern void grYamabukiGateSetPositionFar(void);
extern void grYamabukiGateSetPositionNear(void);
extern void grYamabukiGateAddAnimOffset(intptr_t offset);
extern void grYamabukiGateAddAnimOpen(void);
extern void grYamabukiGateAddAnimClose(void);
extern void grYamabukiGateAddAnimOpenEntry(void);
extern void grYamabukiGateUpdateWait(void);
extern void grYamabukiGateUpdateOpen(void);
extern void grYamabukiGateClearMonsterGObj(void);
extern void grYamabukiGateSetClosedWait(void);
extern void grYamabukiGateUpdateYakumonoPos(void);
extern void grYamabukiGateProcUpdate(GObj *ground_gobj);
extern void grYamabukiMakeGate(void);
extern void grYamabukiInitGroundVars(void);
extern GObj* grYamabukiMakeGround(void);

#if defined(PORT) && defined(SSB64_NETMENU)
/* Re-sync tower-door yakumono + gate GObj anim after rollback snapshot apply. */
extern void grYamabukiGateRestoreAfterRollback(f32 restore_anim_frame, f32 restore_anim_wait, u8 restore_anim_phase,
                                               sb32 has_restore_anim);
/* Collision + open-mesh presentation after DObj anim blob apply (post particle-reset restore). */
extern void grYamabukiGateFinalizeAfterSnapshotRestore(void);
/*
 * Re-derive the live gate GObj from the ground link by its display proc. gate_gobj cannot be safely
 * resolved by gobj->id across rollback because every ground GObj shares id == nGCCommonKindGround.
 * Returns NULL if no gate GObj is currently on the ground link.
 */
extern GObj *grYamabukiGateResolveLiveGObj(void);
#endif

#endif
