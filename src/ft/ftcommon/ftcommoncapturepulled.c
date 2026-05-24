#include <ft/fighter.h>
#include <ft/ftparam.h>
#include <it/item.h>

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

// 0x8014A5F0
void ftCommonCapturePulledRotateScale(GObj *fighter_gobj, Vec3f *this_pos, Vec3f *rotate)
{
    FTStruct *this_fp = ftGetStruct(fighter_gobj);
#ifdef PORT
    FTStruct *capture_fp;
    DObj *attach_dobj;
    /* Rollback / synctest can transiently break the capture coupling: GObj
     * present but capture_gobj cleared. Fall back to the victim's own pos so
     * callers don't sample uninitialised stack memory. */
    if (this_fp->capture_gobj == NULL)
    {
        if (this_pos != NULL)
        {
            *this_pos = DObjGetStruct(fighter_gobj)->translate.vec.f;
        }
        return;
    }
    capture_fp = ftGetStruct(this_fp->capture_gobj);
    attach_dobj = capture_fp->joints[capture_fp->attr->joint_itemheavy_id];
    if (attach_dobj == NULL)
    {
        if (this_pos != NULL)
        {
            *this_pos = DObjGetStruct(fighter_gobj)->translate.vec.f;
        }
        return;
    }
    /* Defensive: clear FTParts transform-cache short-circuit flags along the
     * TopN -> root spine so the func_ovl2_800EDBA4 walk-up below can't return
     * a frame-stale matrix in obscure draw-thread timing. Empirically the
     * actual bug behind the reported "inverted victim" was elsewhere (see
     * docs/bugs/grab_pose_eulerextract_gimbal_2026-05-23.md) and this is a
     * no-op for the offline case, but the invalidate is cheap, harmless, and
     * still useful as belt-and-suspenders for future save-state / rollback
     * paths that may revive coupling with a stale grabber cache. */
    ftParamInvalidateFighterRootChain(this_fp->capture_gobj);
#else
    FTStruct *capture_fp = ftGetStruct(this_fp->capture_gobj);
#endif
#if defined(REGION_US)
    DObj *joint = DObjGetStruct(fighter_gobj)->child;
    Mtx44f mtx;

#ifdef PORT
    func_ovl0_800C9A38(mtx, attach_dobj);
#else
    func_ovl0_800C9A38(mtx, capture_fp->joints[capture_fp->attr->joint_itemheavy_id]);
#endif
    func_ovl2_800EDA0C(mtx, rotate);

    this_pos->x = (-joint->translate.vec.f.x * DObjGetStruct(fighter_gobj)->scale.vec.f.x);
    this_pos->y = (-joint->translate.vec.f.y * DObjGetStruct(fighter_gobj)->scale.vec.f.y);
    this_pos->z = (-joint->translate.vec.f.z * DObjGetStruct(fighter_gobj)->scale.vec.f.z);

    gmCollisionGetWorldPosition(mtx, this_pos);
#else
    FTParts *ftparts;
    DObj *joint = DObjGetStruct(fighter_gobj)->child;

#ifdef PORT
    func_ovl2_800EDBA4(attach_dobj);
    ftparts = attach_dobj->user_data.p;
#else
    func_ovl2_800EDBA4(capture_fp->joints[capture_fp->attr->joint_itemheavy_id]);
    ftparts = capture_fp->joints[capture_fp->attr->joint_itemheavy_id]->user_data.p;
#endif
    func_ovl2_800EDA0C(ftparts->mtx_translate, rotate);

    this_pos->x = (-joint->translate.vec.f.x * DObjGetStruct(fighter_gobj)->scale.vec.f.x);
    this_pos->y = (-joint->translate.vec.f.y * DObjGetStruct(fighter_gobj)->scale.vec.f.y);
    this_pos->z = (-joint->translate.vec.f.z * DObjGetStruct(fighter_gobj)->scale.vec.f.z);

    gmCollisionGetWorldPosition(ftparts->mtx_translate, this_pos);
#endif
}

// 0x8014A6B4
void ftCommonCapturePulledProcPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Vec3f pos;

    ftCommonCapturePulledRotateScale(fighter_gobj, &pos, &DObjGetStruct(fighter_gobj)->rotate.vec.f);

    DObjGetStruct(fighter_gobj)->translate.vec.f.x = pos.x;
    DObjGetStruct(fighter_gobj)->translate.vec.f.z = pos.z;

    if ((fp->status_id == nFTCommonStatusCapturePulled) && (fp->status_vars.common.capture.is_goto_pulled_wait != FALSE))
    {
        ftCommonCaptureWaitSetStatus(fighter_gobj);
    }
}

// 0x8014A72C
void ftCommonCapturePulledProcMap(GObj *fighter_gobj)
{
    FTStruct *this_fp = ftGetStruct(fighter_gobj);
    GObj *capture_gobj = this_fp->capture_gobj;
    FTStruct *capture_fp;
    Vec3f *this_pos = &DObjGetStruct(fighter_gobj)->translate.vec.f;

#ifdef PORT
    if (capture_gobj == NULL)
    {
        return;
    }
    capture_fp = ftGetStruct(capture_gobj);
    if (capture_fp == NULL)
    {
        return;
    }
#else
    capture_fp = ftGetStruct(capture_gobj);
#endif
    Vec3f capture_pos;
    f32 dist_y;

    if (mpCollisionGetFCCommonFloor(capture_fp->coll_data.floor_line_id, this_pos, &dist_y, &this_fp->coll_data.floor_flags, &this_fp->coll_data.floor_angle) != FALSE)
    {
        this_fp->coll_data.floor_line_id = capture_fp->coll_data.floor_line_id;

        if (dist_y >= 0.0F)
        {
            this_pos->y += dist_y;

            this_fp->ga = nMPKineticsGround;
            this_fp->jumps_used = 0;
        }
        else
        {
            this_pos->y += dist_y * 0.5F;

            this_fp->ga = nMPKineticsAir;
            this_fp->jumps_used = 1;
        }
    }
    else
    {
        if (capture_fp->lr == +1)
        {
            mpCollisionGetFloorEdgeR(capture_fp->coll_data.floor_line_id, &capture_pos);
        }
        else mpCollisionGetFloorEdgeL(capture_fp->coll_data.floor_line_id, &capture_pos);

        this_pos->y = this_pos->y + ((capture_pos.y - this_pos->y) * 0.5F);

        mpCommonSetFighterProjectFloor(fighter_gobj);

        this_fp->ga = nMPKineticsAir;
        this_fp->jumps_used = 1;
    }
}

// 0x8014A860
void ftCommonCapturePulledProcCapture(GObj *fighter_gobj, GObj *capture_gobj)
{
    FTStruct *this_fp = ftGetStruct(fighter_gobj);
    FTStruct *capture_fp;

    ftParamStopVoiceRunProcDamage(fighter_gobj);

    if ((this_fp->item_gobj != NULL) && (itGetStruct(this_fp->item_gobj)->weight == nITWeightHeavy))
    {
        ftSetupDropItem(this_fp);
    }
    if (this_fp->catch_gobj != NULL)
    {
        ftCommonThrownSetStatusDamageRelease(this_fp->catch_gobj);

        this_fp->catch_gobj = NULL;
    }
    else if (this_fp->capture_gobj != NULL)
    {
        ftCommonThrownDecideFighterLoseGrip(this_fp->capture_gobj, fighter_gobj);
    }
    this_fp->is_catch_or_capture = FALSE;

    this_fp->capture_gobj = capture_gobj;

    capture_fp = ftGetStruct(capture_gobj);

    this_fp->lr = -capture_fp->lr;

    ftMainSetStatus(fighter_gobj, nFTCommonStatusCapturePulled, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);

    this_fp->status_vars.common.capture.is_goto_pulled_wait = FALSE;

    ftParamSetCaptureImmuneMask(this_fp, FTCATCHKIND_MASK_ALL);
    ftParamMakeRumble(this_fp, 9, 0);
    ftPhysicsStopVelAll(fighter_gobj);
    ftCommonCapturePulledProcPhysics(fighter_gobj);
    ftCommonCapturePulledProcMap(fighter_gobj);
}
