#include <ft/fighter.h>

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

// 0x80148050
void ftCommonHammerLandingProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftStatusVarsHammer(fp)->landing_anim_frame++;

    if (ftStatusVarsHammer(fp)->landing_anim_frame <= 4.0F)
    {
        ftHammerSetStatusHammerWait(fighter_gobj);
    }
}

// 0x801480A4
void ftCommonHammerLandingSetStatus(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, nFTCommonStatusHammerLanding, ftHammerGetAnimFrame(fighter_gobj), 1.0F, ftHammerGetStatUpdateFlags(fighter_gobj));
    ftHammerSetColAnim(fighter_gobj);

    ftStatusVarsHammer(fp)->landing_anim_frame = 0.0F;
}
