#include <ft/fighter.h>

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

// 0x8014D850
void ftDonkeyThrowFKneeBendProcUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    ftStatusVarsThrowF(fp)->kneebend_anim_frame++;

    if
    (
        (ftStatusVarsThrowF(fp)->input_source == FTCOMMON_KNEEBEND_INPUT_TYPE_BUTTON)     &&
        (ftStatusVarsThrowF(fp)->kneebend_anim_frame <= FTCOMMON_KNEEBEND_SHORTHOP_FRAMES)&&
        (fp->input.pl.button_release & (R_CBUTTONS | L_CBUTTONS | D_CBUTTONS | U_CBUTTONS))
    )
    {
        ftStatusVarsThrowF(fp)->is_shorthop = TRUE;
    }
    if (attr->kneebend_anim_length <= ftStatusVarsThrowF(fp)->kneebend_anim_frame)
    {
        ftDonkeyThrowFJumpSetStatus(fighter_gobj);
    }
}

// 0x8014D8E4
void ftDonkeyThrowFKneeBendProcInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (ftCommonHeavyThrowCheckInterruptCommon(fighter_gobj) == FALSE)
    {
        if (ftDonkeyThrowFFCheckInterruptThrowFCommon(fighter_gobj) == FALSE)
        {
            if (ftStatusVarsThrowF(fp)->jump_force < fp->input.pl.stick_range.y)
            {
                ftStatusVarsThrowF(fp)->jump_force = fp->input.pl.stick_range.y;
            }
        }
    }
}

// 0x8014D950
void ftDonkeyThrowFKneeBendSetStatus(GObj *fighter_gobj, s32 input_source)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, nFTDonkeyStatusThrowFKneeBend, 0.0F, 0.0F, FTSTATUS_PRESERVE_NONE);

    ftStatusVarsThrowF(fp)->jump_force = fp->input.pl.stick_range.y;
    ftStatusVarsThrowF(fp)->kneebend_anim_frame = 0.0F;
    ftStatusVarsThrowF(fp)->input_source = input_source;
    ftStatusVarsThrowF(fp)->is_shorthop = FALSE;
}

// 0x8014D9B8
sb32 ftDonkeyThrowFKneeBendCheckInterruptThrowFCommon(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 input_source = ftCommonKneeBendGetInputTypeCommon(fp);

    if (input_source != FTCOMMON_KNEEBEND_INPUT_TYPE_NONE)
    {
        ftDonkeyThrowFKneeBendSetStatus(fighter_gobj, input_source);

        return TRUE;
    }
    else return FALSE;
}
