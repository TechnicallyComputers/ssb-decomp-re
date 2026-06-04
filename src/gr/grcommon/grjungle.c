#include <gr/ground.h>
#include <ft/fighter.h>
#include <reloc_data.h>
#ifdef PORT
#include <mp/map.h>
#include <sys/objanim.h>
#include <string.h>
#endif
#if defined(PORT) && defined(SSB64_NETMENU)
#include <sys/netplay_sim_quantize.h>
#endif

// // // // // // // // // // // //
//                               //
//       INITIALIZED DATA        //
//                               //
// // // // // // // // // // // //

// 0x8012EB50
DObjTransformTypes dGRJungleTaruCannTransformKinds[/* */] =
{
    { 0x28, nGCMatrixKindRotRpyR, 0x00 },
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0x00 }
};

#ifdef PORT
/*
 * Init-time barrel GObj cache (same pattern as Yoster cloud slots). The barrel shares
 * nGCCommonKindGround with other stages' ground GObjs; on DK Jungle it is the only one, but
 * resolving via gcFindGObjByID after rollback is still fragile. This pointer survives snapshot
 * loads because it is not part of rollback state.
 */
static GObj *sGRJungleTaruCannGobj;
static u8 sGRJungleTaruCannReestablishFailed;
static Vec3f sGRJungleTaruCannCachedTranslate;
static f32 sGRJungleTaruCannCachedRotateZ;

static void *grJungleEnsureMapHead(void);
static sb32 grJungleReestablishTaruCannDobjTree(const Vec3f *translate, f32 rotate_z);
static void grJungleRefreshTaruCannCachedPoseFromGObj(GObj *tarucann_gobj);
static void grJungleRepairTaruCannFromCachedPose(void);

#define GRJUNGLE_TARUCANN_DOBJ_ROOT_MOBA (1U << 0)
#define GRJUNGLE_TARUCANN_DOBJ_CHILD_MOBA (1U << 1)
#endif

// // // // // // // // // // // //
//                               //
//          ENUMERATORS          //
//                               //
// // // // // // // // // // // //

enum grJungleTaruCannStatus
{
    nGRJungleTaruCannStatusMove,
    nGRJungleTaruCannStatusRotate
};

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

#ifdef PORT
static void *grJungleEnsureMapHead(void)
{
    void *map_head = gGRCommonStruct.jungle.map_head;

    if (map_head != NULL)
    {
        return map_head;
    }
    if (gMPCollisionGroundData == NULL)
    {
        return NULL;
    }
    map_head =
        (void *)((uintptr_t)PORT_RESOLVE(gMPCollisionGroundData->map_nodes) - (intptr_t)llGRJungleMapMapHead);
    gGRCommonStruct.jungle.map_head = map_head;
    return map_head;
}

GObj *grJungleGetTaruCannGobj(void)
{
    return sGRJungleTaruCannGobj;
}

static void grJungleRefreshTaruCannCachedPoseFromGObj(GObj *tarucann_gobj)
{
    DObj *root;

    if (tarucann_gobj == NULL)
    {
        return;
    }
    root = DObjGetStruct(tarucann_gobj);
    if (root == NULL)
    {
        return;
    }
    sGRJungleTaruCannCachedTranslate = root->translate.vec.f;
    sGRJungleTaruCannCachedRotateZ = root->rotate.vec.f.z;
    /* Netplay rollback only: shared-grid pose cache for snapshot / repair paths. */
#if defined(PORT) && defined(SSB64_NETMENU)
    /* Netplay rollback only: shared-grid barrel cached pose. */
    if (syNetplaySimQuantizeActive() != FALSE)
    {
        syNetplayQuantizeVec3f(&sGRJungleTaruCannCachedTranslate);
        sGRJungleTaruCannCachedRotateZ = syNetplayQuantizeF32(sGRJungleTaruCannCachedRotateZ);
        syNetplayQuantizeDObjTranslate(root);
        if (root->child != NULL)
        {
            syNetplayQuantizeDObjTranslate(root->child);
        }
    }
#endif
}

static void grJungleRepairTaruCannFromCachedPose(void)
{
    grJungleRepairTaruCannPresentation(&sGRJungleTaruCannCachedTranslate, sGRJungleTaruCannCachedRotateZ);
}

static sb32 grJungleReestablishTaruCannDobjTree(const Vec3f *translate, f32 rotate_z)
{
    GObj *tarucann_gobj;
    void *map_head;
    DObj *root;
    Vec3f saved_translate = {0.0F, 0.0F, 0.0F};
    f32 saved_rotate_z = 0.0F;

    if (sGRJungleTaruCannReestablishFailed != 0U)
    {
        return FALSE;
    }
    tarucann_gobj = grJungleGetTaruCannGobj();
    if (tarucann_gobj == NULL)
    {
        return FALSE;
    }
    map_head = grJungleEnsureMapHead();
    if (map_head == NULL)
    {
        return FALSE;
    }
    root = DObjGetStruct(tarucann_gobj);
    if (root != NULL)
    {
        saved_translate = root->translate.vec.f;
        saved_rotate_z = root->rotate.vec.f.z;
    }
    if (translate != NULL)
    {
        saved_translate = *translate;
        saved_rotate_z = rotate_z;
    }
    gcRemoveDObjAll(tarucann_gobj);
    grModelSetupGroundDObjs(tarucann_gobj,
                            (DObjDesc *)((intptr_t)llGRJungleMapMapHead + (uintptr_t)map_head),
                            NULL,
                            dGRJungleTaruCannTransformKinds);
    gcAddAnimJointAll(tarucann_gobj,
                      (AObjEvent32 **)((uintptr_t)map_head + (intptr_t)llGRJungleMapTaruCannDefaultAnimJoint),
                      0.0F);
    gcPlayAnimAll(tarucann_gobj);
    root = DObjGetStruct(tarucann_gobj);
    if ((root == NULL) || (root->child == NULL))
    {
        sGRJungleTaruCannReestablishFailed = 1U;
        return FALSE;
    }
    root->translate.vec.f = saved_translate;
    root->rotate.vec.f.z = saved_rotate_z;
    sGRJungleTaruCannReestablishFailed = 0U;
    grJungleRefreshTaruCannCachedPoseFromGObj(tarucann_gobj);
    return TRUE;
}

void grJungleRepairTaruCannPresentation(const Vec3f *translate, f32 rotate_z)
{
    GObj *tarucann_gobj;
    DObj *root;
    const Vec3f *reest_translate;
    f32 reest_rotate_z;
    sb32 reestablished;

    tarucann_gobj = grJungleGetTaruCannGobj();
    if (tarucann_gobj == NULL)
    {
        return;
    }
    gGRCommonStruct.jungle.tarucann_gobj = tarucann_gobj;
    if (grJungleEnsureMapHead() == NULL)
    {
        return;
    }
    root = DObjGetStruct(tarucann_gobj);
    reestablished = FALSE;
    if ((root == NULL) || (root->child == NULL))
    {
        reest_translate = translate;
        reest_rotate_z = rotate_z;
        if (reest_translate == NULL)
        {
            reest_translate = &sGRJungleTaruCannCachedTranslate;
            reest_rotate_z = sGRJungleTaruCannCachedRotateZ;
        }
        reestablished = grJungleReestablishTaruCannDobjTree(reest_translate, reest_rotate_z);
        root = DObjGetStruct(tarucann_gobj);
    }
    else
    {
        sGRJungleTaruCannReestablishFailed = 0U;
    }
    /*
     * Phase 5b: only write snapshotted root translate when the DObj tree was rebuilt. For an intact
     * tree, translate is animation-driven (Move slide) and forcing snapshot X pins the barrel; rotate.z
     * is still restored manually during the Rotate phase.
     */
    if ((root != NULL) && (translate != NULL))
    {
        if (reestablished != FALSE)
        {
            root->translate.vec.f = *translate;
            root->rotate.vec.f.z = rotate_z;
        }
        else if (gGRCommonStruct.jungle.tarucann_status == nGRJungleTaruCannStatusRotate)
        {
            root->rotate.vec.f.z = rotate_z;
        }
    }
    grJungleRefreshTaruCannCachedPoseFromGObj(tarucann_gobj);
}

sb32 grJungleEnsureTaruCannCoupling(GObj *tarucann_gobj)
{
    DObj *root;

    if (tarucann_gobj == NULL)
    {
        return FALSE;
    }
    root = DObjGetStruct(tarucann_gobj);
    if ((root == NULL) || (root->child == NULL))
    {
#if defined(SSB64_NETMENU)
        /* Netplay rollback only: rebuild hollow barrel tree after particle reset / restore. */
        if (syNetplayRollbackSemanticsActive() != FALSE)
        {
            grJungleRepairTaruCannFromCachedPose();
            root = DObjGetStruct(tarucann_gobj);
        }
        else
#endif
        {
            return FALSE;
        }
    }
    if (root == NULL)
    {
        return FALSE;
    }
#if defined(SSB64_NETMENU)
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        grJungleRefreshTaruCannCachedPoseFromGObj(tarucann_gobj);
    }
#endif
    return TRUE;
}

sb32 grJungleTaruCannIsChildShootAnimActive(GObj *ground_gobj)
{
    void *map_head;
    DObj *root;
    DObj *child;
    AObjEvent32 *shoot_joint;

    if (ground_gobj == NULL)
    {
        return FALSE;
    }
    map_head = grJungleEnsureMapHead();
    if (map_head == NULL)
    {
        return FALSE;
    }
    root = DObjGetStruct(ground_gobj);
    if ((root == NULL) || (root->child == NULL))
    {
        return FALSE;
    }
    child = root->child;
    shoot_joint = (AObjEvent32 *)((uintptr_t)map_head + (intptr_t)llGRJungleMapTaruCannShootAnimJoint);
    return (child->anim_joint.event32 == shoot_joint) ? TRUE : FALSE;
}


#endif

// 0x80109CB0
void grJungleTaruCannAddAnimOffset(GObj *ground_gobj, intptr_t offset)
{
#ifdef PORT
    DObj *root;
    DObj *dobj;
    void *map_head;

    if (ground_gobj == NULL)
    {
        return;
    }
    if ((DObjGetStruct(ground_gobj) == NULL) || (DObjGetStruct(ground_gobj)->child == NULL))
    {
#if defined(SSB64_NETMENU)
        /* Netplay rollback only: repair hollow tree before seating child anim joint. */
        if (syNetplayRollbackSemanticsActive() != FALSE)
        {
            grJungleRepairTaruCannFromCachedPose();
        }
        else
#endif
        {
            return;
        }
    }
    root = DObjGetStruct(ground_gobj);
    if (root == NULL)
    {
        return;
    }
    dobj = root->child;
    if (dobj == NULL)
    {
        return;
    }
    map_head = grJungleEnsureMapHead();
    if (map_head == NULL)
    {
        return;
    }
    gcAddDObjAnimJoint(dobj, (AObjEvent32 *)((uintptr_t)map_head + (intptr_t)offset), 0.0F);
    gcParseDObjAnimJoint(dobj);
    gcPlayDObjAnimJoint(dobj);
#else
    DObj *dobj = DObjGetStruct(ground_gobj)->child;

    gcAddDObjAnimJoint(dobj, (AObjEvent32*) ((uintptr_t)gGRCommonStruct.jungle.map_head + (intptr_t)offset), 0.0F);
    gcParseDObjAnimJoint(dobj);
    gcPlayDObjAnimJoint(dobj);

#endif
}

// 0x80109CFC
void grJungleTaruCannAddAnimFill(GObj *ground_gobj)
{
#ifdef PORT
    grJungleTaruCannAddAnimOffset(ground_gobj, llGRJungleMapTaruCannFillAnimJoint);
#else
    grJungleTaruCannAddAnimOffset(ground_gobj, &llGRJungleMapTaruCannFillAnimJoint);
#endif
}

// 0x80109D20
void grJungleTaruCannAddAnimShoot(GObj *ground_gobj)
{
#ifdef PORT
    grJungleTaruCannAddAnimOffset(ground_gobj, llGRJungleMapTaruCannShootAnimJoint);
#else
    grJungleTaruCannAddAnimOffset(ground_gobj, &llGRJungleMapTaruCannShootAnimJoint);
#endif
}

// 0x80109D44
void grJungleTaruCannUpdateMove(GObj *ground_gobj)
{
    gGRCommonStruct.jungle.tarucann_wait--;

    if (gGRCommonStruct.jungle.tarucann_wait == 0)
    {
        gGRCommonStruct.jungle.tarucann_status = nGRJungleTaruCannStatusRotate;

        gGRCommonStruct.jungle.tarucann_rotate_step = ((syUtilsRandUShort() % 2) != 0) ? 0.07F : -0.07F;
#if defined(PORT) && defined(SSB64_NETMENU)
        /* Netplay rollback only: shared-grid rotate step for cross-peer agreement. */
        if (syNetplaySimQuantizeActive() != FALSE)
        {
            gGRCommonStruct.jungle.tarucann_rotate_step =
                syNetplayQuantizeF32(gGRCommonStruct.jungle.tarucann_rotate_step);
        }

#endif

        gGRCommonStruct.jungle.tarucann_wait = 90;
    }
}

// 0x80109DBC
void grJungleTaruCannUpdateRotate(GObj *ground_gobj)
{
    DObj *dobj = DObjGetStruct(ground_gobj);

#ifdef PORT
#if defined(SSB64_NETMENU)
    if (dobj == NULL)
    {
        /* Netplay rollback only: hollow barrel DObj after snapshot particle reset. */
        if (syNetplayRollbackSemanticsActive() != FALSE)
        {
            grJungleRepairTaruCannFromCachedPose();
            dobj = DObjGetStruct(ground_gobj);
        }
        if (dobj == NULL)
        {
            return;
        }
    }
#endif
#endif

    gGRCommonStruct.jungle.tarucann_wait--;

    if (gGRCommonStruct.jungle.tarucann_wait == 0)
    {
        gGRCommonStruct.jungle.tarucann_status = nGRJungleTaruCannStatusMove;

        gGRCommonStruct.jungle.tarucann_wait = syUtilsRandIntRange(180) + 180;

        dobj->rotate.vec.f.z = F_CST_DTOR32(0.0F);
    }
    else
    {
        dobj->rotate.vec.f.z += gGRCommonStruct.jungle.tarucann_rotate_step;
#if defined(PORT) && defined(SSB64_NETMENU)
        if (syNetplaySimQuantizeActive() != FALSE)
        {
            dobj->rotate.vec.f.z = syNetplayQuantizeF32(dobj->rotate.vec.f.z);
        }

#endif
    }
}

// 0x80109E34
void grJungleTaruCannProcUpdate(GObj *ground_gobj)
{
    switch (gGRCommonStruct.jungle.tarucann_status)
    {
    case nGRJungleTaruCannStatusMove:
        grJungleTaruCannUpdateMove(ground_gobj);
        break;

    case nGRJungleTaruCannStatusRotate:
        grJungleTaruCannUpdateRotate(ground_gobj);
        break;
    }
#if defined(PORT) && defined(SSB64_NETMENU)
    if (syNetplayRollbackSemanticsActive() != FALSE)
    {
        grJungleRefreshTaruCannCachedPoseFromGObj(ground_gobj);
    }

#endif
}

// 0x80109E84
void grJungleMakeTaruCann(void)
{
    void *map_head;
    GObj *tarucann_gobj;

#ifdef PORT
    map_head = (void*) ((uintptr_t)PORT_RESOLVE(gMPCollisionGroundData->map_nodes) - (intptr_t)llGRJungleMapMapHead);
#else
    map_head = (void*) ((uintptr_t)gMPCollisionGroundData->map_nodes - (intptr_t)&llGRJungleMapMapHead);
#endif
    gGRCommonStruct.jungle.map_head = map_head;

    gGRCommonStruct.jungle.tarucann_gobj = tarucann_gobj = gcMakeGObjSPAfter(nGCCommonKindGround, NULL, nGCCommonLinkIDGround, GOBJ_PRIORITY_DEFAULT);

#ifdef PORT
    sGRJungleTaruCannGobj = tarucann_gobj;
    sGRJungleTaruCannReestablishFailed = 0U;
#endif

    gcAddGObjDisplay(tarucann_gobj, gcDrawDObjTreeForGObj, 6, GOBJ_PRIORITY_DEFAULT, ~0);

#ifdef PORT
    grModelSetupGroundDObjs(tarucann_gobj, (DObjDesc*) ((intptr_t)llGRJungleMapMapHead + (uintptr_t)map_head), NULL, dGRJungleTaruCannTransformKinds);
#else
    grModelSetupGroundDObjs(tarucann_gobj, (DObjDesc*) ((intptr_t)&llGRJungleMapMapHead + (uintptr_t)map_head), NULL, dGRJungleTaruCannTransformKinds);
#endif
    gcAddGObjProcess(tarucann_gobj, gcPlayAnimAll, nGCProcessKindFunc, 5);

#ifdef PORT
    gcAddAnimJointAll(tarucann_gobj, (AObjEvent32 **)((uintptr_t)map_head + (intptr_t)llGRJungleMapTaruCannDefaultAnimJoint), 0.0F);
#else
    gcAddAnimJointAll(tarucann_gobj, ((uintptr_t)map_head + (intptr_t)&llGRJungleMapTaruCannDefaultAnimJoint), 0.0F);
#endif
    gcPlayAnimAll(tarucann_gobj);

    gcAddGObjProcess(tarucann_gobj, grJungleTaruCannProcUpdate, nGCProcessKindFunc, 4);
    ftMainCheckAddGroundObstacle(tarucann_gobj, grJungleTaruCannCheckGetDamageKind);

    gGRCommonStruct.jungle.tarucann_status = nGRJungleTaruCannStatusMove;
    gGRCommonStruct.jungle.tarucann_wait = syUtilsRandIntRange(180) + 180;
    gGRCommonStruct.jungle.tarucann_rotate_step = F_CST_DTOR32(0.0F);
#ifdef PORT
    grJungleRefreshTaruCannCachedPoseFromGObj(tarucann_gobj);
#endif
}

// 0x80109FB4
GObj* grJungleMakeGround(void)
{
    grJungleMakeTaruCann();

    return NULL;
}

// 0x80109FD8
sb32 grJungleTaruCannCheckGetDamageKind(GObj *ground_gobj, GObj *fighter_gobj, s32 *kind)
{
    FTStruct *this_fp = ftGetStruct(fighter_gobj);
    f32 dist_x;
    f32 dist_y;

    if ((this_fp->tarucann_wait == 0) && (this_fp->status_id != nFTCommonStatusTaruCann) && !(this_fp->capture_immune_mask & FTCATCHKIND_MASK_TARUCANN))
    {
        DObj *gr_dobj = DObjGetStruct(ground_gobj);
        DObj *ft_dobj = DObjGetStruct(fighter_gobj);

#if defined(PORT) && defined(SSB64_NETMENU)
        if (gr_dobj == NULL)
        {
            /* Netplay rollback only: repair before proximity test. */
            if (syNetplayRollbackSemanticsActive() != FALSE)
            {
                grJungleRepairTaruCannFromCachedPose();
                gr_dobj = DObjGetStruct(ground_gobj);
            }
            if (gr_dobj == NULL)
            {
                return FALSE;
            }
        }

#endif

        if (gr_dobj->translate.vec.f.x < ft_dobj->translate.vec.f.x)
        {
            dist_x = -(gr_dobj->translate.vec.f.x - ft_dobj->translate.vec.f.x);
        }
        else dist_x = gr_dobj->translate.vec.f.x - ft_dobj->translate.vec.f.x;

        if (gr_dobj->translate.vec.f.y < ft_dobj->translate.vec.f.y)
        {
            dist_y = -(gr_dobj->translate.vec.f.y - ft_dobj->translate.vec.f.y);
        }
        else dist_y = gr_dobj->translate.vec.f.y - ft_dobj->translate.vec.f.y;

        if ((dist_x < 280.0F) && (dist_y < 280.0F))
        {
            GObj *other_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];

            while (other_gobj != NULL)
            {
                if (other_gobj != fighter_gobj)
                {
                    FTStruct *other_fp = ftGetStruct(other_gobj);

                    if ((other_fp->status_id == nFTCommonStatusTaruCann) && (ground_gobj == ftStatusVarsTaruCann(other_fp)->tarucann_gobj))
                    {
                        return FALSE;
                    }
                }
                other_gobj = other_gobj->link_next;
            }
            *kind = nGMHitEnvironmentTaruCann;

            grJungleTaruCannAddAnimFill(ground_gobj);

            return TRUE;
        }
    }
    return FALSE;
}

// 0x8010A104
void grJungleTaruCannGetPosition(Vec3f *pos)
{
#ifdef PORT
    DObj *root;
    GObj *tarucann_gobj;

    if (pos == NULL)
    {
        return;
    }
    tarucann_gobj = gGRCommonStruct.jungle.tarucann_gobj;
    if (tarucann_gobj == NULL)
    {
        tarucann_gobj = grJungleGetTaruCannGobj();
    }
    if (tarucann_gobj == NULL)
    {
        *pos = sGRJungleTaruCannCachedTranslate;
        return;
    }
    root = DObjGetStruct(tarucann_gobj);
    if (root == NULL)
    {
        *pos = sGRJungleTaruCannCachedTranslate;
        return;
    }
    *pos = root->translate.vec.f;
    grJungleRefreshTaruCannCachedPoseFromGObj(tarucann_gobj);
#else
    *pos = DObjGetStruct(gGRCommonStruct.jungle.tarucann_gobj)->translate.vec.f;
#endif
}

// 0x8010A12C
f32 grJungleTaruCannGetRotate(void)
{
#ifdef PORT
    DObj *root;
    GObj *tarucann_gobj;

    tarucann_gobj = gGRCommonStruct.jungle.tarucann_gobj;
    if (tarucann_gobj == NULL)
    {
        tarucann_gobj = grJungleGetTaruCannGobj();
    }
    if (tarucann_gobj == NULL)
    {
        return sGRJungleTaruCannCachedRotateZ;
    }
    root = DObjGetStruct(tarucann_gobj);
    if (root == NULL)
    {
        return sGRJungleTaruCannCachedRotateZ;
    }
    grJungleRefreshTaruCannCachedPoseFromGObj(tarucann_gobj);
    return root->rotate.vec.f.z;
#else
    return DObjGetStruct(gGRCommonStruct.jungle.tarucann_gobj)->rotate.vec.f.z;
#endif
}
