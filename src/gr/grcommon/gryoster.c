#include <gr/ground.h>
#include <ft/fighter.h>
#include <reloc_data.h>
#ifdef PORT
#include <sys/objhelper.h>
extern void *func_800269C0_275C0(u16 id);
#endif
#if defined(PORT) && defined(SSB64_NETMENU)
#include <sys/netplay_sim_quantize.h>
extern void syNetSyncLogYosterCloudDiag(s32 cloud_id);
#endif

// // // // // // // // // // // //
//                               //
//       INITIALIZED DATA        //
//                               //
// // // // // // // // // // // //

// 0x8012EB20
#ifdef PORT
intptr_t dGRYosterCloudMatAnimJoints[/* */] = { llGRYosterMapCloudSolidMatAnimJoint, llGRYosterMapCloudEvaporateMatAnimJoint };
#else
intptr_t dGRYosterCloudMatAnimJoints[/* */] = { &llGRYosterMapCloudSolidMatAnimJoint, &llGRYosterMapCloudEvaporateMatAnimJoint };
#endif

// 0x8012EB28
u8 dGRYosterCloudLineIDs[/* */] = { 0x1, 0x2, 0x3 };

// // // // // // // // // // // //
//                               //
//          ENUMERATORS          //
//                               //
// // // // // // // // // // // //

enum grYosterCloudStatus
{
    nGRYosterCloudStatusSolid,
    nGRYosterCloudStatusEvaporate
};

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

// 0x80108550
LBGenerator* grYosterCloudVaporMakeEffect(Vec3f *pos)
{
    LBGenerator *gn = lbParticleMakeGenerator(gGRCommonStruct.yoster.particle_bank_id, 0);

    if (gn != NULL)
    {
        gn->pos.x = pos->x;
        gn->pos.y = pos->y;
        gn->pos.z = pos->z;
    }
    return gn;
}

// 0x801085A8
sb32 grYosterCheckFighterCloudStand(s32 cloud_id)
{
    GObj *fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
    s32 line_id = dGRYosterCloudLineIDs[cloud_id];

    while (fighter_gobj != NULL)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if (fp->ga == nMPKineticsGround)
        {
            if ((fp->coll_data.floor_line_id != -2) && (mpCollisionSetDObjNoID(fp->coll_data.floor_line_id) == line_id))
            {
                return TRUE;
            }
        }
        fighter_gobj = fighter_gobj->link_next;
    }
    return FALSE;
}

// 0x80108634
#ifdef PORT
/*
 * All three cloud GObjs share gobj->id == nGCCommonKindGround, so gcFindGObjByID() cannot tell
 * them apart on a rollback restore (it collapses clouds[0/1/2].gobj onto the first ground GObj).
 * Vanilla identifies clouds purely by array slot, so cache each slot's GObj at init. This table
 * is NOT rollback state, so a snapshot load can never corrupt it. grYosterInitAll refreshes it on
 * every stage load; the cloud GObjs are never destroyed/recreated mid-match, so it stays valid.
 */
static GObj *sGRYosterCloudGobjs[ARRAY_COUNT(gGRCommonStruct.yoster.clouds)];
static Vec3f sGRYosterCloudSpawnTranslate[ARRAY_COUNT(gGRCommonStruct.yoster.clouds)];
static u8 sGRYosterCloudReestablishFailed[ARRAY_COUNT(gGRCommonStruct.yoster.clouds)];
static u8 sGRYosterCloudReestablishedThisTick[ARRAY_COUNT(gGRCommonStruct.yoster.clouds)];
static void grYosterEnsureCloudDisplayDobjs(s32 cloud_id);
static void grYosterCloudPlayAnimAllProc(GObj *gobj);
static void grYosterApplyCloudRootTranslate(GRYosterCloud *cloud, s32 cloud_id);
static void grYosterSyncCloudYakumonoCollision(GRYosterCloud *cloud, s32 cloud_id);
static void grYosterAttachCloudDisplayFromCollChain(GRYosterCloud *cloud, DObj *coll_dobj, void *map_head);
static void grYosterRequeueCloudAnimAfterRepair(GRYosterCloud *cloud);
static sb32 grYosterReestablishCloudDobjTree(s32 cloud_id);
#endif

static void grYosterRecoverCloudAltitudeIfNeeded(GRYosterCloud *cloud, s32 cloud_id);
void grYosterRebindCloudDobjs(s32 cloud_id)
{
    GRYosterCloud *cloud;
    DObj *coll_dobj;
    s32 j;

    if ((cloud_id < 0) || (cloud_id >= ARRAY_COUNT(gGRCommonStruct.yoster.clouds)))
    {
        return;
    }
    cloud = &gGRCommonStruct.yoster.clouds[cloud_id];
    if (cloud->gobj == NULL)
    {
        for (j = 0; j < ARRAY_COUNT(cloud->dobj); j++)
        {
            cloud->dobj[j] = NULL;
        }
        return;
    }
    coll_dobj = DObjGetStruct(cloud->gobj);
#ifdef PORT
    /*
     * Netplay snapshot loads (e.g. the LOAD_HASH_DRIFT resim at tick 509) tear the cloud
     * GObj's DObj payload down to NULL while the GObj itself survives. Rebind alone cannot
     * recover from a null root, so re-establish the DObj tree once per hollow spell. The
     * spawn-anchored translate (grYosterApplyCloudRootTranslate) keeps collision X/Z correct;
     * the snapshot restore must NOT write its (possibly zeroed) saved translate onto the root.
     */
    if (coll_dobj == NULL)
    {
        if (grYosterReestablishCloudDobjTree(cloud_id) != FALSE)
        {
            coll_dobj = DObjGetStruct(cloud->gobj);
        }
    }
    else
    {
        sGRYosterCloudReestablishFailed[cloud_id] = 0U;
    }
#endif
    if (coll_dobj == NULL)
    {
        for (j = 0; j < ARRAY_COUNT(cloud->dobj); j++)
        {
            cloud->dobj[j] = NULL;
        }
        return;
    }
    coll_dobj = coll_dobj->child;
    for (j = 0; j < ARRAY_COUNT(cloud->dobj); j++)
    {
        if (coll_dobj == NULL)
        {
            cloud->dobj[j] = NULL;
            continue;
        }
        cloud->dobj[j] = coll_dobj->child;
        coll_dobj = coll_dobj->sib_next;
    }
#ifdef PORT
    grYosterEnsureCloudDisplayDobjs(cloud_id);
#endif
}

#ifdef PORT
static void grYosterApplyCloudRootTranslate(GRYosterCloud *cloud, s32 cloud_id)
{
    DObj *root;

    if ((cloud == NULL) || (cloud->gobj == NULL))
    {
        return;
    }
    if ((cloud_id < 0) || (cloud_id >= (s32)ARRAY_COUNT(sGRYosterCloudSpawnTranslate)))
    {
        return;
    }
    root = DObjGetStruct(cloud->gobj);
    if (root == NULL)
    {
        return;
    }
    grYosterRecoverCloudAltitudeIfNeeded(cloud, cloud_id);
    root->translate.vec.f.x = sGRYosterCloudSpawnTranslate[cloud_id].x;
    root->translate.vec.f.z = sGRYosterCloudSpawnTranslate[cloud_id].z;
    root->translate.vec.f.y = cloud->altitude - cloud->pressure;
}

GObj *grYosterGetCloudGobj(s32 cloud_id)
{
    if ((cloud_id < 0) || (cloud_id >= (s32)ARRAY_COUNT(sGRYosterCloudGobjs)))
    {
        return NULL;
    }
    return sGRYosterCloudGobjs[cloud_id];
}

void grYosterGetCloudSpawnTranslate(s32 cloud_id, Vec3f *out)
{
    if ((out == NULL) || (cloud_id < 0) || (cloud_id >= (s32)ARRAY_COUNT(sGRYosterCloudSpawnTranslate)))
    {
        return;
    }
    *out = sGRYosterCloudSpawnTranslate[cloud_id];
}

void grYosterAnchorCloudRootTranslate(s32 cloud_id)
{
    GRYosterCloud *cloud;

    if ((cloud_id < 0) || (cloud_id >= ARRAY_COUNT(gGRCommonStruct.yoster.clouds)))
    {
        return;
    }
    cloud = &gGRCommonStruct.yoster.clouds[cloud_id];
    grYosterApplyCloudRootTranslate(cloud, cloud_id);
    grYosterSyncCloudYakumonoCollision(cloud, cloud_id);
}

static void grYosterCloudPlayAnimAllProc(GObj *gobj)
{
    s32 i;

    gcPlayAnimAll(gobj);
    for (i = 0; i < ARRAY_COUNT(gGRCommonStruct.yoster.clouds); i++)
    {
        GRYosterCloud *cloud = &gGRCommonStruct.yoster.clouds[i];

        if (cloud->gobj == gobj)
        {
            grYosterApplyCloudRootTranslate(cloud, i);
            return;
        }
    }
}

static void grYosterRequeueCloudAnimAfterRepair(GRYosterCloud *cloud)
{
    if (cloud == NULL)
    {
        return;
    }
    if ((cloud->status == nGRYosterCloudStatusSolid) && (cloud->anim_id == -1))
    {
        cloud->anim_id = nGRYosterCloudStatusSolid;
    }
    else if ((cloud->status == nGRYosterCloudStatusEvaporate) && (cloud->anim_id == -1))
    {
        cloud->anim_id = nGRYosterCloudStatusEvaporate;
    }
}

static void grYosterSyncCloudYakumonoCollision(GRYosterCloud *cloud, s32 cloud_id)
{
    DObj *root;
    Vec3f yak_pos;

    if ((cloud == NULL) || (cloud->gobj == NULL))
    {
        return;
    }
    root = DObjGetStruct(cloud->gobj);
    if (root == NULL)
    {
        return;
    }
    yak_pos = root->translate.vec.f;
    if (cloud->is_cloud_line_active != FALSE)
    {
        mpCollisionSetYakumonoOnID(dGRYosterCloudLineIDs[cloud_id]);
        mpCollisionSetYakumonoPosID(dGRYosterCloudLineIDs[cloud_id], &yak_pos);
    }
    else
    {
        mpCollisionSetYakumonoOffID(dGRYosterCloudLineIDs[cloud_id]);
    }
}

static void grYosterAttachCloudDisplayFromCollChain(GRYosterCloud *cloud, DObj *coll_dobj, void *map_head)
{
    s32 j;
    DObj *cloud_dobj;

    for (j = 0; j < ARRAY_COUNT(cloud->dobj); j++)
    {
        cloud->dobj[j] = NULL;
        if (coll_dobj == NULL)
        {
            continue;
        }
        cloud_dobj = gcAddChildForDObj(coll_dobj, (void *)((uintptr_t)map_head + (intptr_t)llGRYosterMapCloudDisplayList));
        cloud->dobj[j] = cloud_dobj;
        gcAddXObjForDObjFixed(cloud_dobj, nGCMatrixKindTra, 0);
        gcAddXObjForDObjFixed(cloud_dobj, nGCMatrixKind48, 0);
        lbCommonAddMObjForTreeDObjs(cloud_dobj, (MObjSub ***)((uintptr_t)map_head + (intptr_t)llGRYosterMap_4B8_MObjSub));
        coll_dobj = coll_dobj->sib_next;
    }
}

static sb32 grYosterReestablishCloudDobjTree(s32 cloud_id)
{
    GRYosterCloud *cloud;
    GObj *map_gobj;
    void *map_head;
    DObj *root;

    if ((cloud_id < 0) || (cloud_id >= (s32)ARRAY_COUNT(gGRCommonStruct.yoster.clouds)))
    {
        return FALSE;
    }
    if (sGRYosterCloudReestablishFailed[cloud_id] != 0U)
    {
        return FALSE;
    }
    cloud = &gGRCommonStruct.yoster.clouds[cloud_id];
    map_gobj = cloud->gobj;
    map_head = gGRCommonStruct.yoster.map_head;
    if ((map_gobj == NULL) || (map_head == NULL))
    {
        return FALSE;
    }
    gcRemoveDObjAll(map_gobj);
    gcSetupCustomDObjs
    (
        map_gobj,
        (DObjDesc *)((intptr_t)llGRYosterMapMapHead + (uintptr_t)map_head),
        NULL,
        nGCMatrixKindTra,
        nGCMatrixKindNull,
        nGCMatrixKindNull
    );
    gcAddAnimJointAll(map_gobj, (AObjEvent32 **)((uintptr_t)map_head + (intptr_t)llGRYosterMap_1E0_AnimJoint), 0);
    root = DObjGetStruct(map_gobj);
    if (root == NULL)
    {
        sGRYosterCloudReestablishFailed[cloud_id] = 1U;
        return FALSE;
    }
    grYosterAttachCloudDisplayFromCollChain(cloud, root->child, map_head);
    gcPlayAnimAll(map_gobj);
    grYosterApplyCloudRootTranslate(cloud, cloud_id);
    grYosterSyncCloudYakumonoCollision(cloud, cloud_id);
    grYosterRequeueCloudAnimAfterRepair(cloud);
    sGRYosterCloudReestablishFailed[cloud_id] = 0U;
    sGRYosterCloudReestablishedThisTick[cloud_id] = 1U;
    return (cloud->dobj[0] != NULL) ? TRUE : FALSE;
}

sb32 grYosterCloudReestablishedThisTick(s32 cloud_id)
{
    if ((cloud_id < 0) || (cloud_id >= (s32)ARRAY_COUNT(sGRYosterCloudReestablishedThisTick)))
    {
        return FALSE;
    }
    return (sGRYosterCloudReestablishedThisTick[cloud_id] != 0U) ? TRUE : FALSE;
}

void grYosterRepairCloudPresentation(s32 cloud_id)
{
    if ((cloud_id < 0) || (cloud_id >= ARRAY_COUNT(gGRCommonStruct.yoster.clouds)))
    {
        return;
    }
    grYosterRebindCloudDobjs(cloud_id);
    grYosterAnchorCloudRootTranslate(cloud_id);
}

static void grYosterEnsureCloudDisplayDobjs(s32 cloud_id)
{
    GRYosterCloud *cloud;
    DObj *coll_dobj;
    void *map_head;
    sb32 repaired;
    s32 j;

    if ((cloud_id < 0) || (cloud_id >= ARRAY_COUNT(gGRCommonStruct.yoster.clouds)))
    {
        return;
    }
    cloud = &gGRCommonStruct.yoster.clouds[cloud_id];
    map_head = gGRCommonStruct.yoster.map_head;
    if ((cloud->gobj == NULL) || (map_head == NULL))
    {
        return;
    }
    coll_dobj = DObjGetStruct(cloud->gobj);
    if (coll_dobj == NULL)
    {
        return;
    }
    if (coll_dobj->child == NULL)
    {
        return;
    }
    repaired = FALSE;
    coll_dobj = coll_dobj->child;
    for (j = 0; j < ARRAY_COUNT(cloud->dobj); j++)
    {
        DObj *cloud_dobj;

        if (coll_dobj == NULL)
        {
            cloud->dobj[j] = NULL;
            continue;
        }
        cloud_dobj = coll_dobj->child;
        if (cloud_dobj == NULL)
        {
            cloud_dobj = gcAddChildForDObj(coll_dobj, (void *)((uintptr_t)map_head + (intptr_t)llGRYosterMapCloudDisplayList));
            gcAddXObjForDObjFixed(cloud_dobj, nGCMatrixKindTra, 0);
            gcAddXObjForDObjFixed(cloud_dobj, nGCMatrixKind48, 0);
            lbCommonAddMObjForTreeDObjs(cloud_dobj, (MObjSub ***)((uintptr_t)map_head + (intptr_t)llGRYosterMap_4B8_MObjSub));
            repaired = TRUE;
        }
        else if (cloud_dobj->mobj == NULL)
        {
            lbCommonAddMObjForTreeDObjs(cloud_dobj, (MObjSub ***)((uintptr_t)map_head + (intptr_t)llGRYosterMap_4B8_MObjSub));
        }
        cloud->dobj[j] = cloud_dobj;
        coll_dobj = coll_dobj->sib_next;
    }
    if (repaired != FALSE)
    {
        grYosterRequeueCloudAnimAfterRepair(cloud);
    }
}
#endif

static f32 grYosterSampleYakumonoLineBaseY(s32 line_id)
{
    MPLineInfo *line_info;
    s32 i, j, k, l, index;
    f32 max_y;
    f32 y;

    if (gMPCollisionGeometry == NULL)
    {
        return 0.0F;
    }
#ifdef PORT
    line_info = (MPLineInfo *)PORT_RESOLVE(gMPCollisionGeometry->line_info);
#else
    line_info = gMPCollisionGeometry->line_info;
#endif
    if (line_info == NULL)
    {
        return 0.0F;
    }
    max_y = 0.0F;
    for (i = 0; i < gMPCollisionGeometry->yakumono_count; i++, line_info++)
    {
        if (line_info->yakumono_id != line_id)
        {
            continue;
        }
        for (j = 0; j < (s32)ARRAY_COUNT(line_info->line_data); j++)
        {
            MPLineData *line_data = &line_info->line_data[j];

            for (index = line_data->group_id, k = 0; k < line_data->line_count; k++, index++)
            {
                MPVertexLinks *vlinks = &gMPCollisionVertexLinks[index];

                for (l = vlinks->vertex1; l < (vlinks->vertex1 + vlinks->vertex2); l++)
                {
                    y = gMPCollisionVertexData->vpos[gMPCollisionVertexIDs->vertex_id[l]].pos.y;
                    if (y > max_y)
                    {
                        max_y = y;
                    }
                }
            }
        }
    }
    return max_y;
}

static void grYosterRecoverCloudAltitudeIfNeeded(GRYosterCloud *cloud, s32 cloud_id)
{
    f32 sample;
    s32 line_id;

    if (cloud->altitude != 0.0F)
    {
        return;
    }
#if defined(PORT) && defined(SSB64_NETMENU)
    /* Netplay rollback only: refill altitude zeroed by snapshot restore; stage init sets altitude offline. */
    if (syNetplayRollbackSemanticsActive() == FALSE)
    {
        return;
    }

#endif
    line_id = dGRYosterCloudLineIDs[cloud_id];
    if ((gMPCollisionYakumonoDObjs != NULL) && (gMPCollisionYakumonoDObjs->dobjs[line_id] != NULL))
    {
        sample = gMPCollisionYakumonoDObjs->dobjs[line_id]->translate.vec.f.y;
        if (sample != 0.0F)
        {
            cloud->altitude = sample;
            return;
        }
    }
    sample = grYosterSampleYakumonoLineBaseY(line_id);
    if (sample != 0.0F)
    {
        cloud->altitude = sample;
    }
}

static void grYosterTickCloudMatAnim(MObj *mobj)
{
    if ((mobj == NULL) || (mobj->anim_wait == AOBJ_ANIM_NULL))
    {
        return;
    }
    gcParseMObjMatAnimJoint(mobj);
    gcPlayMObjMatAnim(mobj);
}

sb32 grYosterCloudMatAnimIsIdle(MObj *mobj)
{
    if (mobj == NULL)
    {
        return FALSE;
    }
    if (mobj->anim_wait == AOBJ_ANIM_NULL)
    {
        return TRUE;
    }
    if (mobj->anim_wait == AOBJ_ANIM_END)
    {
        return TRUE;
    }
    return FALSE;
}

sb32 grYosterCloudPressureGateOpen(const GRYosterCloud *cloud, MObj *mobj)
{
    if (cloud == NULL)
    {
        return FALSE;
    }
    /* Mat-anim attach consumed; pressure lifecycle must not depend on MObj presence or anim_wait. */
    if (cloud->anim_id == -1)
    {
        return TRUE;
    }
    if (grYosterCloudMatAnimIsIdle(mobj) != FALSE)
    {
        return TRUE;
    }
    return FALSE;
}

void grYosterUpdateCloudSolid(s32 cloud_id)
{
    Vec3f pos;
    DObj *dobj;
    GRYosterCloud *cloud;
    MObj *mobj;

    if ((cloud_id < 0) || (cloud_id >= ARRAY_COUNT(gGRCommonStruct.yoster.clouds)))
    {
        return;
    }
    cloud = &gGRCommonStruct.yoster.clouds[cloud_id];
    if (cloud->gobj == NULL)
    {
        return;
    }
    grYosterRecoverCloudAltitudeIfNeeded(cloud, cloud_id);
    mobj = ((cloud->dobj[0] != NULL) ? cloud->dobj[0]->mobj : NULL);
    grYosterTickCloudMatAnim(mobj);
    if (grYosterCloudPressureGateOpen(cloud, mobj) != FALSE)
    {
        if (cloud->is_cloud_line_active == FALSE)
        {
            mpCollisionSetYakumonoOnID(dGRYosterCloudLineIDs[cloud_id]);

            cloud->is_cloud_line_active = TRUE;
        }
        if (cloud->pressure_timer == 0)
        {
            cloud->status = nGRYosterCloudStatusEvaporate;
            cloud->anim_id = nGRYosterCloudStatusEvaporate;
            cloud->evaporate_wait = 180;

            dobj = DObjGetStruct(cloud->gobj);
            if (dobj == NULL)
            {
                return;
            }
            pos = dobj->translate.vec.f;

            pos.x += (-750.0F);
            pos.y += (-350.0F);

            grYosterCloudVaporMakeEffect(&pos);

            func_800269C0_275C0(nSYAudioFGMYosterCloudVapor);
        }
        else
        {
            if (grYosterCheckFighterCloudStand(cloud_id) != FALSE)
            {
                if (cloud->pressure_timer == -1)
                {
                    cloud->pressure_timer = 120;
                }
                cloud->pressure += 5.0F;

                if (cloud->pressure > 180.0F)
                {
                    cloud->pressure = 180.0F;
                }
            }
            else
            {
                cloud->pressure_timer = -1;
                cloud->pressure -= 5.0F;

                if (cloud->pressure < 0.0F)
                {
                    cloud->pressure = 0.0F;
                }
            }
            if (cloud->pressure_timer > 0)
            {
                cloud->pressure_timer--;
            }
        }
    }
    dobj = DObjGetStruct(cloud->gobj);
    if (dobj == NULL)
    {
        return;
    }
#ifdef PORT
    grYosterApplyCloudRootTranslate(cloud, cloud_id);
#else
    dobj->translate.vec.f.y = cloud->altitude - cloud->pressure;
#endif

    mpCollisionSetYakumonoPosID(dGRYosterCloudLineIDs[cloud_id], &dobj->translate.vec.f);
}

// 0x80108814
void grYosterUpdateCloudEvaporate(s32 cloud_id)
{
    GRYosterCloud *cloud;

    if ((cloud_id < 0) || (cloud_id >= ARRAY_COUNT(gGRCommonStruct.yoster.clouds)))
    {
        return;
    }
    cloud = &gGRCommonStruct.yoster.clouds[cloud_id];
    if (cloud->gobj == NULL)
    {
        return;
    }
    if (cloud->is_cloud_line_active != FALSE)
    {
        mpCollisionSetYakumonoOffID(dGRYosterCloudLineIDs[cloud_id]);

        cloud->is_cloud_line_active = FALSE;
    }
    if (cloud->evaporate_wait == 0)
    {
        cloud->status = nGRYosterCloudStatusSolid;
        cloud->anim_id = nGRYosterCloudStatusSolid;
        cloud->pressure_timer = -1;
        cloud->pressure = 0.0F;
    }
    else
    {
        cloud->evaporate_wait--;
    }
}

// 0x80108890
void grYosterUpdateCloudAnim(s32 cloud_id)
{
    GRYosterCloud *cloud;
    s8 anim_id;
    sb32 anim_attached;

    if ((cloud_id < 0) || (cloud_id >= ARRAY_COUNT(gGRCommonStruct.yoster.clouds)))
    {
        return;
    }
    cloud = &gGRCommonStruct.yoster.clouds[cloud_id];
    anim_id = cloud->anim_id;
    if (anim_id != -1)
    {
        void *map_head = gGRCommonStruct.yoster.map_head;
        s32 i;

        if (map_head == NULL)
        {
            return;
        }
        anim_attached = FALSE;
        for (i = 0; i < ARRAY_COUNT(cloud->dobj); i++)
        {
            DObj *dobj = cloud->dobj[i];
            MObj *mobj;

            if (dobj == NULL)
            {
                continue;
            }
            lbCommonAddTreeDObjsAnimAll(dobj, NULL, (void *)((intptr_t)dGRYosterCloudMatAnimJoints[anim_id] + (uintptr_t)map_head), 0.0F);
            gcPlayDObjAnimJoint(dobj);
            for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
            {
                grYosterTickCloudMatAnim(mobj);
            }
            anim_attached = TRUE;
        }
        if (anim_attached != FALSE)
        {
            cloud->anim_id = -1;
        }
    }
}

// 0x80108960
void grYosterProcUpdate(GObj *ground_gobj)
{
    s32 i;

#ifdef PORT
    for (i = 0; i < (s32)ARRAY_COUNT(sGRYosterCloudReestablishedThisTick); i++)
    {
        sGRYosterCloudReestablishedThisTick[i] = 0U;
    }
#endif
    for (i = 0; i < ARRAY_COUNT(gGRCommonStruct.yoster.clouds); i++)
    {
        if (gGRCommonStruct.yoster.clouds[i].gobj == NULL)
        {
            continue;
        }
#ifdef PORT
        grYosterRebindCloudDobjs(i);
#endif
        switch (gGRCommonStruct.yoster.clouds[i].status)
        {
        case nGRYosterCloudStatusSolid:
            grYosterUpdateCloudSolid(i);
            break;

        case nGRYosterCloudStatusEvaporate:
            grYosterUpdateCloudEvaporate(i);
            break;
        }
		grYosterUpdateCloudAnim(i);
#if defined(PORT) && defined(SSB64_NETMENU)
		syNetSyncLogYosterCloudDiag(i);

#endif
	}
}

// 0x801089F4
void grYosterInitAll(void)
{
    DObj *cloud_dobj;
    GObj *map_gobj;
    DObj *coll_dobj;
    void *map_head;
    s32 i, j;

#ifdef PORT
    map_head = (void *)((uintptr_t)PORT_RESOLVE(gMPCollisionGroundData->map_nodes) - (intptr_t)llGRYosterMapMapHead);
#else
    map_head = (uintptr_t)gMPCollisionGroundData->map_nodes - (intptr_t)&llGRYosterMapMapHead;
#endif
    gGRCommonStruct.yoster.map_head = map_head;

    for (i = 0; i < ARRAY_COUNT(gGRCommonStruct.yoster.clouds); i++)
    {
        map_gobj = gcMakeGObjSPAfter(nGCCommonKindGround, NULL, nGCCommonLinkIDGround, GOBJ_PRIORITY_DEFAULT);

        gGRCommonStruct.yoster.clouds[i].gobj = map_gobj;
#ifdef PORT
        sGRYosterCloudGobjs[i] = map_gobj;
#endif

        gcAddGObjDisplay(map_gobj, gcDrawDObjTreeForGObj, 6, GOBJ_PRIORITY_DEFAULT, ~0);
        gcSetupCustomDObjs
        (
            map_gobj, 
#ifdef PORT
            (DObjDesc*) ((intptr_t)llGRYosterMapMapHead + (uintptr_t)map_head), 
#else
            (DObjDesc*) ((intptr_t)&llGRYosterMapMapHead + (uintptr_t)map_head), 
#endif
            NULL, 
            nGCMatrixKindTra,    // Make this nGCMatrixKindTraRotRpyRSca to see cloud scale animation
            nGCMatrixKindNull, 
            nGCMatrixKindNull
        );
#ifdef PORT
        gcAddGObjProcess(map_gobj, grYosterCloudPlayAnimAllProc, nGCProcessKindFunc, 5);
#else
        gcAddGObjProcess(map_gobj, gcPlayAnimAll, nGCProcessKindFunc, 5);
#endif

#ifdef PORT
        gcAddAnimJointAll(map_gobj, (AObjEvent32 **)((uintptr_t)map_head + (intptr_t)llGRYosterMap_1E0_AnimJoint), 0);
#else
        gcAddAnimJointAll(map_gobj, (uintptr_t)map_head + (intptr_t)&llGRYosterMap_1E0_AnimJoint, 0);
#endif

        coll_dobj = DObjGetStruct(map_gobj);
        coll_dobj->translate.vec.f = gMPCollisionYakumonoDObjs->dobjs[dGRYosterCloudLineIDs[i]]->translate.vec.f;

#ifdef PORT
        sGRYosterCloudSpawnTranslate[i] = coll_dobj->translate.vec.f;
#endif

        gGRCommonStruct.yoster.clouds[i].altitude = coll_dobj->translate.vec.f.y;
        if (gGRCommonStruct.yoster.clouds[i].altitude == 0.0F)
        {
            gGRCommonStruct.yoster.clouds[i].altitude =
                grYosterSampleYakumonoLineBaseY(dGRYosterCloudLineIDs[i]);
        }

        coll_dobj = coll_dobj->child;

#ifdef PORT
        grYosterAttachCloudDisplayFromCollChain(&gGRCommonStruct.yoster.clouds[i], coll_dobj, map_head);
#else
        for (j = 0; j < ARRAY_COUNT(gGRCommonStruct.yoster.clouds[i].dobj); j++, coll_dobj = coll_dobj->sib_next)
        {
            cloud_dobj = gcAddChildForDObj(coll_dobj, (uintptr_t)map_head + (intptr_t)&llGRYosterMapCloudDisplayList);
            gGRCommonStruct.yoster.clouds[i].dobj[j] = cloud_dobj;

            gcAddXObjForDObjFixed(cloud_dobj, nGCMatrixKindTra, 0);
            gcAddXObjForDObjFixed(cloud_dobj, nGCMatrixKind48, 0);
            lbCommonAddMObjForTreeDObjs(cloud_dobj, (uintptr_t)map_head + (intptr_t)&llGRYosterMap_4B8_MObjSub);
        }
#endif
        gcPlayAnimAll(map_gobj);

        gGRCommonStruct.yoster.clouds[i].status = nGRYosterCloudStatusSolid;
        gGRCommonStruct.yoster.clouds[i].anim_id = nGRYosterCloudStatusSolid;
        gGRCommonStruct.yoster.clouds[i].pressure_timer = -1;
        gGRCommonStruct.yoster.clouds[i].is_cloud_line_active = FALSE;
        gGRCommonStruct.yoster.clouds[i].pressure = 0.0F;

        mpCollisionSetYakumonoOnID(dGRYosterCloudLineIDs[i]);
    }
#ifdef PORT
    gGRCommonStruct.yoster.particle_bank_id = efParticleGetLoadBankID((uintptr_t)&lGRYosterParticleScriptBankLo, (uintptr_t)&lGRYosterParticleScriptBankHi, (uintptr_t)&lGRYosterParticleTextureBankLo, (uintptr_t)&lGRYosterParticleTextureBankHi);
#else
    gGRCommonStruct.yoster.particle_bank_id = efParticleGetLoadBankID(&lGRYosterParticleScriptBankLo, &lGRYosterParticleScriptBankHi, &lGRYosterParticleTextureBankLo, &lGRYosterParticleTextureBankHi);
#endif
}

// 0x80108C80
GObj* grYosterMakeGround(void)
{
    GObj *ground_gobj = gcMakeGObjSPAfter(nGCCommonKindGround, NULL, nGCCommonLinkIDGround, GOBJ_PRIORITY_DEFAULT);

    grYosterInitAll();
    gcAddGObjProcess(ground_gobj, grYosterProcUpdate, nGCProcessKindFunc, 4);

    return ground_gobj;
}
