#include <ft/fighter.h>
#if defined(PORT) && defined(SSB64_NETMENU)
#include <sys/netrollback.h>
#include <sys/netinput.h>
extern void port_log(const char *fmt, ...);
extern char *getenv(const char *name);
#endif

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

// 0x80145620
sb32 ftCommonCliffAttackCheckInterruptCommon(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

#if defined(PORT) && defined(SSB64_NETMENU)
    /* SSB64_NETMENU: stripped from offline builds. Runtime: diagnostics only, no behavior.
     *
     * Netplay diagnostic: this check reads input.pl.button_tap, a DERIVED edge latch that
     * syNetRbSnapshotApply deliberately does not restore ("ephemeral edges", re-derived by
     * ftMainProcessInput each tick). Soak 2026-08-31: p1's live pass took the ledge ATTACK
     * branch here at tick 4208 while its own resim of the same tick took CLIMB, with the
     * authoritative input history holding btn=0x0000 for the whole window on the owning
     * peer -- a tap the record does not contain. Log the latch at the decision so live and
     * resim passes can be compared directly. SSB64_NETPLAY_CLIFF_DIAG=1.
     */
    {
        static int s_diag = -1;

        if (s_diag < 0)
        {
            const char *e = getenv("SSB64_NETPLAY_CLIFF_DIAG");

            s_diag = ((e != NULL) && (e[0] != '\0') && (e[0] != '0')) ? 1 : 0;
        }
        if (s_diag != 0)
        {
            port_log("SSB64 CliffDiag: attack_check tick=%u player=%d status=%d tap=0x%04X hold=0x%04X "
                     "rel=0x%04X mask_ab=0x%04X fires=%d resim=%d\n",
                     (unsigned int)syNetInputGetTick(), (int)fp->player, (int)fp->status_id,
                     (unsigned int)fp->input.pl.button_tap, (unsigned int)fp->input.pl.button_hold,
                     (unsigned int)fp->input.pl.button_release,
                     (unsigned int)(fp->input.button_mask_a | fp->input.button_mask_b),
                     (int)((fp->input.pl.button_tap &
                            (fp->input.button_mask_a | fp->input.button_mask_b)) != 0),
                     (int)(syNetRollbackIsResimulating() != FALSE));
        }
    }
#endif
    if (fp->input.pl.button_tap & (fp->input.button_mask_a | fp->input.button_mask_b))
    {
        ftCommonCliffQuickOrSlowSetStatus(fighter_gobj, nFTCommonCliffKindAttackQuick);

        return TRUE;
    }
    else return FALSE;
}

// 0x8014566C
void ftCommonCliffAttackQuick1ProcUpdate(GObj *fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ftCommonCliffAttackQuick2SetStatus);
}

// 0x80145690
void ftCommonCliffAttackSlow1ProcUpdate(GObj *fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ftCommonCliffAttackSlow2SetStatus);
}

// 0x801456B4
void ftCommonCliffAttackQuick1SetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTCommonStatusCliffAttackQuick1, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);

    fp->is_cliff_hold = TRUE;

    fp->proc_damage = ftCommonCliffCommonProcDamage;
}

// 0x80145704
void ftCommonCliffAttackSlow1SetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTCommonStatusCliffAttackSlow1, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);

    fp->is_cliff_hold = TRUE;

    fp->proc_damage = ftCommonCliffCommonProcDamage;
}

// 0x80145754
void ftCommonCliffAttackQuick2SetStatus(GObj *fighter_gobj)
{
    ftCommonCliffCommon2UpdateCollData(fighter_gobj);
    ftMainSetStatus(fighter_gobj, nFTCommonStatusCliffAttackQuick2, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftCommonCliffCommon2InitStatusVars(fighter_gobj);
}

// 0x80145794
void ftCommonCliffAttackSlow2SetStatus(GObj *fighter_gobj)
{
    ftCommonCliffCommon2UpdateCollData(fighter_gobj);
    ftMainSetStatus(fighter_gobj, nFTCommonStatusCliffAttackSlow2, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftCommonCliffCommon2InitStatusVars(fighter_gobj);
}
