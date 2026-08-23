/*
 * FTStatusVars overlay accessors — Milestone 1 (Approach C scaffolding).
 * Returns the same union member pointers as direct status_vars.common.* access.
 * PORT + SSB64_NETMENU: ftStatusVarsNoteAccess() feeds netplay_statusvars_witness when env-gated.
 *
 * Include from ftcommon.h (enum). Inline accessors require FTStruct complete —
 * fttypes.h re-includes this header after FTStruct is defined.
 *
 * No outer include guard: ftcommon.h and fttypes.h both include this file.
 */

#ifndef _FTSTATUSVARS_ENUM_DONE_
#define _FTSTATUSVARS_ENUM_DONE_

typedef enum FTStatusVarsOverlay
{
    nFTStatusVarsOverlayNone = -1,
    nFTStatusVarsOverlayDead,
    nFTStatusVarsOverlayRebirth,
    nFTStatusVarsOverlaySleep,
    nFTStatusVarsOverlayEntry,
    nFTStatusVarsOverlayTurn,
    nFTStatusVarsOverlayKneeBend,
    nFTStatusVarsOverlayJumpAerial,
    nFTStatusVarsOverlayDamage,
    nFTStatusVarsOverlaySquat,
    nFTStatusVarsOverlayDokan,
    nFTStatusVarsOverlayLanding,
    nFTStatusVarsOverlayFallSpecial,
    nFTStatusVarsOverlayTwister,
    nFTStatusVarsOverlayTaruCann,
    nFTStatusVarsOverlayDownWait,
    nFTStatusVarsOverlayDownBounce,
    nFTStatusVarsOverlayRebound,
    nFTStatusVarsOverlayCliffWait,
    nFTStatusVarsOverlayCliffMotion,
    nFTStatusVarsOverlayLift,
    nFTStatusVarsOverlayItemThrow,
    nFTStatusVarsOverlayItemSwing,
    nFTStatusVarsOverlayFireFlower,
    nFTStatusVarsOverlayHammer,
    nFTStatusVarsOverlayGuard,
    nFTStatusVarsOverlayEscape,
    nFTStatusVarsOverlayCatchMain,
    nFTStatusVarsOverlayCatchWait,
    nFTStatusVarsOverlayCapture,
    nFTStatusVarsOverlayThrown,
    nFTStatusVarsOverlayCaptureKirby,
    nFTStatusVarsOverlayCaptureYoshi,
    nFTStatusVarsOverlayCaptureCaptain,
    nFTStatusVarsOverlayThrowF,
    nFTStatusVarsOverlayThrowFF,
    nFTStatusVarsOverlayThrowFDamage,
    nFTStatusVarsOverlayAttack1,
    nFTStatusVarsOverlayAttack100,
    nFTStatusVarsOverlayAttackLw3,
    nFTStatusVarsOverlayAttack4,
    nFTStatusVarsOverlayAttackAir,
    nFTStatusVarsOverlayFoxSpecialHi,
    nFTStatusVarsOverlayCount

} FTStatusVarsOverlay;

#endif /* _FTSTATUSVARS_ENUM_DONE_ */

#ifdef _FTSTRUCT_DEFINED_

#ifndef _FTSTATUSVARS_INLINE_DONE_
#define _FTSTATUSVARS_INLINE_DONE_

#if defined(PORT) && defined(SSB64_NETMENU)
void syNetplayStatusVarsWitnessNoteAccess(const FTStruct *fp, FTStatusVarsOverlay overlay);
/* Same, plus the accessor's caller address so a stomp names its writer. */
void syNetplayStatusVarsWitnessNoteAccessFrom(const FTStruct *fp, FTStatusVarsOverlay overlay,
                                             const void *caller);
void *syNetplayStatusVarsBankAuthoritySlot(FTStruct *fp, FTStatusVarsOverlay overlay, void *union_member);
void syNetplayStatusVarsWitnessEnterDamageInit(void);
void syNetplayStatusVarsWitnessLeaveDamageInit(void);
void syNetplayStatusVarsWitnessProbeJumpAerialEntry(const FTStruct *fp);
void syNetplayStatusVarsWitnessProbeAirVelTransN(const FTStruct *fp, const DObj *transn_joint,
                                                 const DObj *topn_joint, f32 out_drift, f32 out_vy,
                                                 f32 out_vz, f32 cos_v, f32 sin_v);
#endif

static inline void ftStatusVarsNoteAccess(const FTStruct *fp, FTStatusVarsOverlay overlay)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    /*
     * __builtin_return_address(0) is the accessor's caller — i.e. the code that touched this
     * overlay. Without it a stomp reports only which overlays collided, never who did it:
     * soak 2026-08-22 logged 16 guard/catchmain, 12 guard/catchwait and 10 guard/throwf stomps
     * landing exactly on grab ticks, with every gated snapshot path ruled out by inspection
     * (is_shield=0 and catch statuses sit outside [GuardStart, GuardEnd]).
     * GCC/Clang only; MSVC keeps the plain entry point.
     * See docs/bugs/netplay_grab_guard_overlay_stomp_2026-08-22.md.
     */
#if defined(__GNUC__)
    syNetplayStatusVarsWitnessNoteAccessFrom(fp, overlay, __builtin_return_address(0));
#else
    syNetplayStatusVarsWitnessNoteAccess(fp, overlay);
#endif
#else
    (void)fp;
    (void)overlay;
#endif
}

static inline void ftStatusVarsProbeJumpAerialEntry(const FTStruct *fp)
{
#if defined(PORT) && defined(SSB64_NETMENU)
    syNetplayStatusVarsWitnessProbeJumpAerialEntry(fp);
#else
    (void)fp;
#endif
}

static inline ftCommonDeadStatusVars *ftStatusVarsDead(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayDead);
#if defined(PORT) && defined(SSB64_NETMENU)
    /*
     * C2b migrated overlay: bank authority under rollback semantics. Mid-Dead loads were
     * restoring stale bank[Dead] while dead_gate_wait stayed live (soak seed 3685555679
     * FC@1389 dead_gate_wait live vs blob). SetWait writes bank then mirror; tagged capture
     * round-trips wait+pos. Offline modes keep the vanilla union.
     * See docs/bugs/netplay_dead_rebirth_damage_statusvars_bank_authority_2026-07-28.md.
     */
    return &((union FTStatusVars *)syNetplayStatusVarsBankAuthoritySlot(
                 (FTStruct *)(void *)fp, nFTStatusVarsOverlayDead,
                 &((FTStruct *)(void *)fp)->status_vars))
                ->common.dead;
#else
    return &((FTStruct *)(void *)fp)->status_vars.common.dead;
#endif
}

static inline ftCommonRebirthStatusVars *ftStatusVarsRebirth(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayRebirth);
#if defined(PORT) && defined(SSB64_NETMENU)
    /*
     * C2b migrated overlay: bank authority under rollback semantics. Rebirth SetStatus inits
     * every field (pos/halo_offset/waits/halo_number); mid-rebirth loads were projecting
     * stale bank zeros → topn_ty=0 FC (soak seed 3685555679 @1406). No sidecar — status-scoped.
     */
    return &((union FTStatusVars *)syNetplayStatusVarsBankAuthoritySlot(
                 (FTStruct *)(void *)fp, nFTStatusVarsOverlayRebirth,
                 &((FTStruct *)(void *)fp)->status_vars))
                ->common.rebirth;
#else
    return &((FTStruct *)(void *)fp)->status_vars.common.rebirth;
#endif
}

static inline ftCommonSleepStatusVars *ftStatusVarsSleep(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlaySleep);
    return &((FTStruct *)(void *)fp)->status_vars.common.sleep;
}

static inline ftCommonEntryStatusVars *ftStatusVarsEntry(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayEntry);
    return &((FTStruct *)(void *)fp)->status_vars.common.entry;
}

static inline ftCommonTurnStatusVars *ftStatusVarsTurn(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayTurn);
#if defined(PORT) && defined(SSB64_NETMENU)
    /*
     * C2b migrated overlay: under rollback semantics the sidecar bank slot is the forward-sim
     * authority, so Turn bytes (lr_dash dash-tap buffer read from Wait..Ottotto/Dash) survive
     * union aliasing and round-trip ring capture/apply exactly (blob turn_vars sidecar).
     * Offline modes fall through to the vanilla union inside AuthoritySlot.
     */
    return &((union FTStatusVars *)syNetplayStatusVarsBankAuthoritySlot(
                 (FTStruct *)(void *)fp, nFTStatusVarsOverlayTurn,
                 &((FTStruct *)(void *)fp)->status_vars))
                ->common.turn;
#else
    return &((FTStruct *)(void *)fp)->status_vars.common.turn;
#endif
}

static inline ftCommonKneeBendStatusVars *ftStatusVarsKneeBend(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayKneeBend);
#if defined(PORT) && defined(SSB64_NETMENU)
    /*
     * C2b migrated overlay: bank authority under rollback semantics (same recipe as Turn).
     * KneeBend SetStatus inits every field; JumpSetStatus reads kneebend while still in
     * KneeBend status (before ftMainSetStatus) so no blob sidecar is required.
     * See docs/bugs/netplay_kneebend_jumpaerial_statusvars_bank_authority_2026-07-28.md.
     */
    return &((union FTStatusVars *)syNetplayStatusVarsBankAuthoritySlot(
                 (FTStruct *)(void *)fp, nFTStatusVarsOverlayKneeBend,
                 &((FTStruct *)(void *)fp)->status_vars))
                ->common.kneebend;
#else
    return &((FTStruct *)(void *)fp)->status_vars.common.kneebend;
#endif
}

static inline ftCommonJumpAerialStatusVars *ftStatusVarsJumpAerial(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayJumpAerial);
#if defined(PORT) && defined(SSB64_NETMENU)
    /*
     * C2b migrated overlay: bank authority under rollback semantics. Mid-JA ring loads were
     * projecting stale bank[JumpAerial] (zeros) over live ja_vel/drift — SoftLipPhase
     * compound=softlip_ja_vel_fork after light GGPO (soak seed 2157085813 @1424).
     * Ness/Yoshi SetStatus inits every overlay field used by ProcPhysics; tagged capture
     * round-trips without a sidecar. Offline modes keep the vanilla union.
     */
    return &((union FTStatusVars *)syNetplayStatusVarsBankAuthoritySlot(
                 (FTStruct *)(void *)fp, nFTStatusVarsOverlayJumpAerial,
                 &((FTStruct *)(void *)fp)->status_vars))
                ->common.jumpaerial;
#else
    return &((FTStruct *)(void *)fp)->status_vars.common.jumpaerial;
#endif
}

static inline ftCommonDamageStatusVars *ftStatusVarsDamage(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayDamage);
#if defined(PORT) && defined(SSB64_NETMENU)
    /*
     * C2b migrated overlay: bank authority under rollback semantics. Hitstun / kb_over folds
     * and DamageFly* into blastzone (soak seed 3685555679 FC@1266 DamageFlyRoll) round-trip
     * via tagged capture. Thrown pre-seeds script_id into bank[Damage] without stomping the
     * Thrown overlay — intentional bank isolation win. InitDamageVars sets hitstun/kb_over/
     * dust/coll_mask_curr; remaining fields retain prior bank episode (same risk class as
     * unscrubbed union leftovers). Offline modes keep the vanilla union.
     */
    return &((union FTStatusVars *)syNetplayStatusVarsBankAuthoritySlot(
                 (FTStruct *)(void *)fp, nFTStatusVarsOverlayDamage,
                 &((FTStruct *)(void *)fp)->status_vars))
                ->common.damage;
#else
    return &((FTStruct *)(void *)fp)->status_vars.common.damage;
#endif
}

static inline ftCommonSquatStatusVars *ftStatusVarsSquat(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlaySquat);
#if defined(PORT) && defined(SSB64_NETMENU)
    /*
     * C2b migrated overlay: bank authority under rollback semantics. Mid-Squat loads wiped
     * pass_wait → Pass vs SquatRv FC (soak seed 733611745 @569). SetStatus inits all fields;
     * blob squat_vars sidecar covers the interrupt window (same recipe as Turn). Offline modes
     * keep the vanilla union.
     * See docs/bugs/netplay_squat_landing_statusvars_bank_authority_2026-07-28.md.
     */
    return &((union FTStatusVars *)syNetplayStatusVarsBankAuthoritySlot(
                 (FTStruct *)(void *)fp, nFTStatusVarsOverlaySquat,
                 &((FTStruct *)(void *)fp)->status_vars))
                ->common.squat;
#else
    return &((FTStruct *)(void *)fp)->status_vars.common.squat;
#endif
}

static inline ftCommonDokanStatusVars *ftStatusVarsDokan(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayDokan);
    return &((FTStruct *)(void *)fp)->status_vars.common.dokan;
}

static inline ftCommonLandingStatusVars *ftStatusVarsLanding(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayLanding);
#if defined(PORT) && defined(SSB64_NETMENU)
    /*
     * C2b migrated overlay: bank authority under rollback semantics. Single-field overlay
     * (is_allow_interrupt) set on Landing SetStatus; tagged capture round-trips without a
     * sidecar. Offline modes keep the vanilla union.
     */
    return &((union FTStatusVars *)syNetplayStatusVarsBankAuthoritySlot(
                 (FTStruct *)(void *)fp, nFTStatusVarsOverlayLanding,
                 &((FTStruct *)(void *)fp)->status_vars))
                ->common.landing;
#else
    return &((FTStruct *)(void *)fp)->status_vars.common.landing;
#endif
}

static inline ftCommonFallSpecialStatusVars *ftStatusVarsFallSpecial(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayFallSpecial);
#if defined(PORT) && defined(SSB64_NETMENU)
    /*
     * C2b migrated overlay: bank authority under rollback semantics. FallSpecial SetStatus
     * inits every field; LandingFallSpecial shares this overlay (ownership table). No sidecar.
     * Offline modes keep the vanilla union.
     */
    return &((union FTStatusVars *)syNetplayStatusVarsBankAuthoritySlot(
                 (FTStruct *)(void *)fp, nFTStatusVarsOverlayFallSpecial,
                 &((FTStruct *)(void *)fp)->status_vars))
                ->common.fallspecial;
#else
    return &((FTStruct *)(void *)fp)->status_vars.common.fallspecial;
#endif
}

static inline ftCommonTwisterStatusVars *ftStatusVarsTwister(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayTwister);
    return &((FTStruct *)(void *)fp)->status_vars.common.twister;
}

static inline ftCommonTaruCannStatusVars *ftStatusVarsTaruCann(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayTaruCann);
    return &((FTStruct *)(void *)fp)->status_vars.common.tarucann;
}

static inline ftCommonDownWaITStatusVars *ftStatusVarsDownWait(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayDownWait);
    return &((FTStruct *)(void *)fp)->status_vars.common.downwait;
}

static inline ftCommonDownBounceStatusVars *ftStatusVarsDownBounce(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayDownBounce);
    return &((FTStruct *)(void *)fp)->status_vars.common.downbounce;
}

static inline ftCommonReboundStatusVars *ftStatusVarsRebound(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayRebound);
    return &((FTStruct *)(void *)fp)->status_vars.common.rebound;
}

static inline ftCommonCliffWaitStatusVars *ftStatusVarsCliffWait(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayCliffWait);
    return &((FTStruct *)(void *)fp)->status_vars.common.cliffwait;
}

static inline ftCommonCliffMotionStatusVars *ftStatusVarsCliffMotion(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayCliffMotion);
    return &((FTStruct *)(void *)fp)->status_vars.common.cliffmotion;
}

static inline ftCommonLiftStatusVars *ftStatusVarsLift(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayLift);
    return &((FTStruct *)(void *)fp)->status_vars.common.lift;
}

static inline ftCommonItemThrowStatusVars *ftStatusVarsItemThrow(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayItemThrow);
    return &((FTStruct *)(void *)fp)->status_vars.common.itemthrow;
}

static inline ftCommonItemSwingStatusVars *ftStatusVarsItemSwing(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayItemSwing);
    return &((FTStruct *)(void *)fp)->status_vars.common.itemswing;
}

static inline ftCommonFireFlowerStatusVars *ftStatusVarsFireFlower(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayFireFlower);
    return &((FTStruct *)(void *)fp)->status_vars.common.fireflower;
}

static inline ftCommonHammerStatusVars *ftStatusVarsHammer(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayHammer);
    return &((FTStruct *)(void *)fp)->status_vars.common.hammer;
}

static inline ftCommonGuardStatusVars *ftStatusVarsGuard(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayGuard);
    return &((FTStruct *)(void *)fp)->status_vars.common.guard;
}

static inline ftCommonEscapeStatusVars *ftStatusVarsEscape(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayEscape);
    return &((FTStruct *)(void *)fp)->status_vars.common.escape;
}

static inline ftCommonCatchStatusVars *ftStatusVarsCatchMain(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayCatchMain);
    return &((FTStruct *)(void *)fp)->status_vars.common.catchmain;
}

static inline ftCommonCatchWaITStatusVars *ftStatusVarsCatchWait(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayCatchWait);
#if defined(PORT) && defined(SSB64_NETMENU)
    /*
     * C2b migrated overlay: bank authority under rollback semantics (same recipe as Turn).
     * Mid-CatchWait synctest/load projected stale bank[CatchWait] (zeros) over live
     * throw_wait → throw_wait==0 fired ThrowF while the peer stayed in CatchWait
     * (soak1 session 614503255 seed 1685497605 @1724). CatchWaitSetStatus inits
     * throw_wait; status-scoped — no blob sidecar. Offline modes keep the vanilla union.
     * See docs/bugs/netplay_catchwait_throw_wait_statusvars_bank_authority_2026-07-29.md.
     */
    return &((union FTStatusVars *)syNetplayStatusVarsBankAuthoritySlot(
                 (FTStruct *)(void *)fp, nFTStatusVarsOverlayCatchWait,
                 &((FTStruct *)(void *)fp)->status_vars))
                ->common.catchwait;
#else
    return &((FTStruct *)(void *)fp)->status_vars.common.catchwait;
#endif
}

static inline ftCommonCaptureStatusVars *ftStatusVarsCapture(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayCapture);
    return &((FTStruct *)(void *)fp)->status_vars.common.capture;
}

static inline ftCommonThrownStatusVars *ftStatusVarsThrown(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayThrown);
    return &((FTStruct *)(void *)fp)->status_vars.common.thrown;
}

static inline ftCommonCaptureKirbyStatusVars *ftStatusVarsCaptureKirby(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayCaptureKirby);
    return &((FTStruct *)(void *)fp)->status_vars.common.capturekirby;
}

static inline ftCommonCaptureYoshiStatusVars *ftStatusVarsCaptureYoshi(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayCaptureYoshi);
    return &((FTStruct *)(void *)fp)->status_vars.common.captureyoshi;
}

static inline ftCommonCaptureCaptainStatusVars *ftStatusVarsCaptureCaptain(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayCaptureCaptain);
    return &((FTStruct *)(void *)fp)->status_vars.common.capturecaptain;
}

static inline ftCommonThrowFStatusVars *ftStatusVarsThrowF(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayThrowF);
    return &((FTStruct *)(void *)fp)->status_vars.common.throwf;
}

static inline ftCommonThrowFFStatusVars *ftStatusVarsThrowFF(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayThrowFF);
    return &((FTStruct *)(void *)fp)->status_vars.common.throwff;
}

static inline ftCommonThrowFDamageStatusVars *ftStatusVarsThrowFDamage(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayThrowFDamage);
    return &((FTStruct *)(void *)fp)->status_vars.common.throwfdamage;
}

static inline ftCommonAttack1StatusVars *ftStatusVarsAttack1(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayAttack1);
    return &((FTStruct *)(void *)fp)->status_vars.common.attack1;
}

static inline ftCommonAttack100StatusVars *ftStatusVarsAttack100(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayAttack100);
    return &((FTStruct *)(void *)fp)->status_vars.common.attack100;
}

static inline ftCommonAttackLw3StatusVars *ftStatusVarsAttackLw3(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayAttackLw3);
    return &((FTStruct *)(void *)fp)->status_vars.common.attacklw3;
}

static inline ftCommonAttack4StatusVars *ftStatusVarsAttack4(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayAttack4);
    return &((FTStruct *)(void *)fp)->status_vars.common.attack4;
}

static inline ftCommonAttackAirStatusVars *ftStatusVarsAttackAir(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayAttackAir);
    return &((FTStruct *)(void *)fp)->status_vars.common.attackair;
}

static inline ftFoxSpecialHiStatusVars *ftStatusVarsFoxSpecialHi(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayFoxSpecialHi);
    return &((FTStruct *)(void *)fp)->status_vars.fox.specialhi;
}

#endif /* _FTSTATUSVARS_INLINE_DONE_ */
#endif /* _FTSTRUCT_DEFINED_ */
