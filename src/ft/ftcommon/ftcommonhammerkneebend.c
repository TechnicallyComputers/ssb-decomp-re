#include <ft/fighter.h>

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

// 0x80147EC0
void ftCommonHammerKneeBendProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    ftStatusVarsHammer(fp)->kneebend_anim_frame++;

    if 
    (
        (ftStatusVarsHammer(fp)->input_source == FTCOMMON_KNEEBEND_INPUT_TYPE_BUTTON)     &&
        (ftStatusVarsHammer(fp)->kneebend_anim_frame <= FTCOMMON_KNEEBEND_SHORTHOP_FRAMES)&&
        (fp->input.pl.button_release & (R_CBUTTONS | L_CBUTTONS | D_CBUTTONS | U_CBUTTONS))
    )
    {
        ftStatusVarsHammer(fp)->is_shorthop = TRUE;
    }
    if (ftStatusVarsHammer(fp)->kneebend_anim_frame >= attr->kneebend_anim_length)
    {
        ftCommonHammerFallSetStatusJump(fighter_gobj);
    }
}

// 0x80147F54
void ftCommonHammerKneeBendProcInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (ftStatusVarsHammer(fp)->jump_force < fp->input.pl.stick_range.y)
    {
        ftStatusVarsHammer(fp)->jump_force = fp->input.pl.stick_range.y;
    }
}

// 0x80147F88
void ftCommonHammerKneeBendSetStatus(GObj *fighter_gobj, s32 input_source)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTCommonStatusHammerKneeBend, ftHammerGetAnimFrame(fighter_gobj), 1.0F, ftHammerGetStatUpdateFlags(fighter_gobj));
    ftHammerSetColAnim(fighter_gobj);

    ftStatusVarsHammer(fp)->jump_force = fp->input.pl.stick_range.y;
    ftStatusVarsHammer(fp)->kneebend_anim_frame = 0.0F;
    ftStatusVarsHammer(fp)->input_source = input_source;
    ftStatusVarsHammer(fp)->is_shorthop = FALSE;
}

// 0x8014800C
sb32 ftCommonHammerKneeBendCheckInterruptCommon(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 input_source = ftCommonKneeBendGetInputTypeCommon(fp);

    if (input_source != FTCOMMON_KNEEBEND_INPUT_TYPE_NONE)
    {
        ftCommonHammerKneeBendSetStatus(fighter_gobj, input_source);

        return TRUE;
    }
    else return FALSE;
}
