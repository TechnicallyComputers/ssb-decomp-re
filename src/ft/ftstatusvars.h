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
    nFTStatusVarsOverlayCount

} FTStatusVarsOverlay;

#endif /* _FTSTATUSVARS_ENUM_DONE_ */

#ifdef _FTSTRUCT_DEFINED_

#ifndef _FTSTATUSVARS_INLINE_DONE_
#define _FTSTATUSVARS_INLINE_DONE_

#if defined(PORT) && defined(SSB64_NETMENU)
void syNetplayStatusVarsWitnessNoteAccess(const FTStruct *fp, FTStatusVarsOverlay overlay);
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
    syNetplayStatusVarsWitnessNoteAccess(fp, overlay);
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
    return &((FTStruct *)(void *)fp)->status_vars.common.dead;
}

static inline ftCommonRebirthStatusVars *ftStatusVarsRebirth(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayRebirth);
    return &((FTStruct *)(void *)fp)->status_vars.common.rebirth;
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
    return &((FTStruct *)(void *)fp)->status_vars.common.turn;
}

static inline ftCommonKneeBendStatusVars *ftStatusVarsKneeBend(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayKneeBend);
    return &((FTStruct *)(void *)fp)->status_vars.common.kneebend;
}

static inline ftCommonJumpAerialStatusVars *ftStatusVarsJumpAerial(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayJumpAerial);
    return &((FTStruct *)(void *)fp)->status_vars.common.jumpaerial;
}

static inline ftCommonDamageStatusVars *ftStatusVarsDamage(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayDamage);
    return &((FTStruct *)(void *)fp)->status_vars.common.damage;
}

static inline ftCommonSquatStatusVars *ftStatusVarsSquat(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlaySquat);
    return &((FTStruct *)(void *)fp)->status_vars.common.squat;
}

static inline ftCommonDokanStatusVars *ftStatusVarsDokan(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayDokan);
    return &((FTStruct *)(void *)fp)->status_vars.common.dokan;
}

static inline ftCommonLandingStatusVars *ftStatusVarsLanding(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayLanding);
    return &((FTStruct *)(void *)fp)->status_vars.common.landing;
}

static inline ftCommonFallSpecialStatusVars *ftStatusVarsFallSpecial(const FTStruct *fp)
{
    ftStatusVarsNoteAccess(fp, nFTStatusVarsOverlayFallSpecial);
    return &((FTStruct *)(void *)fp)->status_vars.common.fallspecial;
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
    return &((FTStruct *)(void *)fp)->status_vars.common.catchwait;
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

#endif /* _FTSTATUSVARS_INLINE_DONE_ */
#endif /* _FTSTRUCT_DEFINED_ */
