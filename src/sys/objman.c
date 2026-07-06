#include <sys/obj.h>

#include <sys/objtypes.h>
#include <sys/debug.h>
#include <sys/taskman.h>
#include <sys/objdisplay.h>
#include <sys/rdp.h>
#include <stddef.h>

extern void port_log(const char *fmt, ...);

#if defined(PORT) && defined(SSB64_NETMENU)
#include <sys/netplay_resim_replay_hang_diag.h>
#endif

#ifdef PORT
/* PORT diag: log GObj allocations for the kinds known to leak stale
 * DObj.dl_link across scene boundaries (Ground=1010, Effect=1011).
 * Resolves to the caller of the wrapper (gcMakeGObjSPAfter / SPBefore /
 * After / Before), which is the user-level allocation site
 * (efManagerMakeEffect, grCastleSetup, etc.) — addr2line on it pins the
 * scene/effect that allocated this GObj. Cross-reference with the
 * stale-dl_link bail log to identify which scene's GObj is surviving
 * the scene-arena recycle. */
#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_ReturnAddress)
#define PORT_CALLER_RA() _ReturnAddress()
#else
#define PORT_CALLER_RA() __builtin_return_address(0)
#endif
#if defined(SSB64_NETMENU)
#define PORT_LOG_GOBJ_ALLOC(gobj, _id, _link)                                  \
	do                                                                         \
	{                                                                          \
		if ((gobj) != NULL && ((_id) == 1010 || (_id) == 1011))                \
		{                                                                      \
			port_log("SSB64: gobj_alloc gobj=%p id=%u link=%u caller=%p "      \
			         "frame=%u\n",                                             \
			         (void *) (gobj), (unsigned) (_id), (unsigned) (_link),    \
			         PORT_CALLER_RA(),                                         \
			         (unsigned) dSYTaskmanFrameCount);                         \
		}                                                                      \
	} while (0)
#else
#define PORT_LOG_GOBJ_ALLOC(gobj, _id, _link) ((void) 0)
#endif
#else
#define PORT_LOG_GOBJ_ALLOC(gobj, _id, _link) ((void) 0)
#endif

/* Issue #128 follow-on (item-side variant): a stale GObj* from BSS-stored
 * handles is being injected into gGCCommonDLLinks[] *after* gcSetupObjman
 * cleared the array. Catching the injection (here) names the caller — the
 * deferred discovery in gcCaptureTaggedGObjs only sees the consequence.
 * Same gate as libultraship/src/fast/interpreter.cpp:50-56. */
#ifdef PORT
#if defined(__SANITIZE_ADDRESS__)
#define PORT_DIAG_HAVE_ASAN 1
#elif defined(__has_feature)
#  if __has_feature(address_sanitizer)
#    define PORT_DIAG_HAVE_ASAN 1
#  endif
#endif
#ifdef PORT_DIAG_HAVE_ASAN
#include <sanitizer/asan_interface.h>
#endif
#endif

// // // // // // // // // // // //
//                               //
//   GLOBAL / STATIC VARIABLES   //
//                               //
// // // // // // // // // // // //

GObjThread *sGCThreadHead;
u32 sGCThreadsActive;
u32 sGCStacksActiveNum;
u32 sGCThreadStackSize;
u32 sUnkUnusedSetup;
GObjThreadStack *sGCThreadStackHead;

void (*sGCProcessFunction)(GObjProcess*);
GObjProcess *sGCProcessHead;
GObjProcess *sGCProcessQueue[6];
u32 sGCProcessesActive;

#if defined(PORT) && defined(SSB64_NETMENU)
#define GCPORT_PROCESS_QUEUE_REPAIR_LIMIT 2048U

static void gcPortClearGObjProcessPriorityLinks(GObjProcess *gobjproc)
{
	if (gobjproc == NULL)
	{
		return;
	}
	gobjproc->priority_next = NULL;
	gobjproc->priority_prev = NULL;
}

static sb32 gcPortGObjProcessIsOnFreeList(GObjProcess *gobjproc)
{
	GObjProcess *cur;
	u32 guard;

	if (gobjproc == NULL)
	{
		return FALSE;
	}
	for (cur = sGCProcessHead, guard = 0U; (cur != NULL) && (guard < GCPORT_PROCESS_QUEUE_REPAIR_LIMIT);
	     cur = cur->link_next, guard++)
	{
		if (cur == gobjproc)
		{
			return TRUE;
		}
	}
	if (guard >= GCPORT_PROCESS_QUEUE_REPAIR_LIMIT)
	{
		port_log("SSB64: gobjproc_free_list_scan_capped proc=%p head=%p\n",
		         (void *)gobjproc,
		         (void *)sGCProcessHead);
	}
	return FALSE;
}

static sb32 gcPortTryUnlinkGObjProcessFromPriorityQueue(GObjProcess *gobjproc, const char *site)
{
	GObjProcess *prev;
	GObjProcess *cur;
	u32 guard;
	s32 priority;
	s32 queue_idx;

	if (gobjproc == NULL)
	{
		return FALSE;
	}
	priority = gobjproc->priority;
	if ((priority < 0) || (priority >= (s32)ARRAY_COUNT(sGCProcessQueue)))
	{
		if ((gobjproc->priority_next != NULL) || (gobjproc->priority_prev != NULL))
		{
			port_log("SSB64: gobjproc_priority_invalid site=%s proc=%p priority=%d next=%p prev=%p\n",
			         (site != NULL) ? site : "?",
			         (void *)gobjproc,
			         (int)priority,
			         (void *)gobjproc->priority_next,
			         (void *)gobjproc->priority_prev);
		}
	}
	for (queue_idx = 0; queue_idx < (s32)ARRAY_COUNT(sGCProcessQueue); queue_idx++)
	{
		prev = NULL;
		cur = sGCProcessQueue[queue_idx];
		for (guard = 0U; (cur != NULL) && (guard < GCPORT_PROCESS_QUEUE_REPAIR_LIMIT);
		     guard++, prev = cur, cur = cur->priority_next)
		{
			if (cur == gobjproc)
			{
				if (prev != NULL)
				{
					prev->priority_next = cur->priority_next;
				}
				else
				{
					sGCProcessQueue[queue_idx] = cur->priority_next;
				}
				if (cur->priority_next != NULL)
				{
					cur->priority_next->priority_prev = prev;
				}
				if ((cur->priority_prev != prev) || (queue_idx != priority))
				{
					GObj *parent = cur->parent_gobj;

					port_log("SSB64: gobjproc_priority_unlink_repaired site=%s proc=%p parent_id=%u priority=%d queue=%d stale_prev=%p scan_prev=%p next=%p\n",
					         (site != NULL) ? site : "?",
					         (void *)cur,
					         (parent != NULL) ? parent->id : 0U,
					         (int)priority,
					         (int)queue_idx,
					         (void *)cur->priority_prev,
					         (void *)prev,
					         (void *)cur->priority_next);
				}
				gcPortClearGObjProcessPriorityLinks(cur);
				return TRUE;
			}
		}
		if (guard >= GCPORT_PROCESS_QUEUE_REPAIR_LIMIT)
		{
			port_log("SSB64: gobjproc_priority_unlink_scan_capped site=%s proc=%p priority=%d queue=%d head=%p\n",
			         (site != NULL) ? site : "?",
			         (void *)gobjproc,
			         (int)priority,
			         (int)queue_idx,
			         (void *)sGCProcessQueue[queue_idx]);
		}
	}
	if ((gobjproc->priority_next != NULL) || (gobjproc->priority_prev != NULL))
	{
		GObj *parent = gobjproc->parent_gobj;

		port_log("SSB64: gobjproc_priority_stale_links_cleared site=%s proc=%p parent_id=%u priority=%d next=%p prev=%p\n",
		         (site != NULL) ? site : "?",
		         (void *)gobjproc,
		         (parent != NULL) ? parent->id : 0U,
		         (int)priority,
		         (void *)gobjproc->priority_next,
		         (void *)gobjproc->priority_prev);
		gcPortClearGObjProcessPriorityLinks(gobjproc);
	}
	return FALSE;
}
#endif

GObj *gGCCommonLinks[GC_COMMON_MAX_LINKS];
s32 D_80046774_40794;
GObj *sGCCommonLinks[GC_COMMON_MAX_LINKS];
GObj *sGCCommonHead;
GObj *gGCCommonDLLinks[GC_COMMON_MAX_DLLINKS];
GObj *sGCCommonDLLinks[GC_COMMON_MAX_DLLINKS];
s32 sGCCommonsActiveNum;
u16 sGCCommonSize;
s16 sGCCommonsMaxNum;

XObj *sGCMatrixHead;
u32 sGCMatrixesActiveNum;

void (*sGCDrawFuncEject)(DObjVec*);

AObj *sGCAnimHead;
u32 sGCAnimsActiveNum;

MObj *sGCMaterialHead;
u32 sGCMaterialsActive;

DObj *sGCDrawHead;
u32 sGCDrawsActiveNum;
u16 sGCDrawSize;

SObj *sGCSpriteHead;
u32 sGCSpritesActiveNum;
u16 sGCSpriteSize;

CObj *sGCCameraHead;
u32 sGCCamerasActiveNum;
u16 sGCCameraSize;

GObj *gGCCurrentCommon;
GObj *gGCCurrentCamera; // Is this exclusively a camera GObj?
GObj *gGCCurrentDisplay;

GObjProcess *gGCCurrentProcess;
u32 sGCRunStatus;
OSMesg sGCMesgs[1];
OSMesgQueue gGCMesgQueue;

GCGfxLink gGCFrameQueueGfxLinks[64];
u8 D_80046F88[24];

// // // // // // // // // // // //
//                               //
//       INITIALIZED DATA        //
//                               //
// // // // // // // // // // // //

// 0x80038B70
OSId dGCProcessThreadID = 10000000;

// 0x80038B74
s32 dGCCurrentStatus = nGCStatusSystem;

// 0x8003B878
GCPersp dGCPerspDefault = { NULL, 0, 30.0F, 4.0F / 3.0F, 100.0F, 12800.0F, 1.0F };

// 0x8003B984
GCOrtho dGCOrthoDefault = { NULL, -160.0F, 160.0F, -120.0F, 120.0F, 100.0F, 12800.0F, 1.0F };

// 0x8003B8B4
CObjVec dGCCObjVecDefault = { NULL, { 0.0F, 0.0F, 1500.0F }, { 0.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } };

// 0x8003B8DC
GCTranslate dGCTranslateDefault = { NULL, { 0.0F, 0.0F, 0.0F } };

// 0x8003B8EC
GCRotate dGCRotateDefaultAXYZ = { NULL, 0.0F, { 0.0F, 0.0F, 1.0F } };

// 0x8003B900
GCRotate dGCRotateDefaultRpy = { NULL, 0.0F, { 0.0F, 0.0F, 0.0F } };

// 0x8003B914
GCScale dGCScaleDefault = { NULL, { 1.0F, 1.0F, 1.0F } };

// // // // // // // // // // // //
//                               //
//           FUNCTIONS           //
//                               //
// // // // // // // // // // // //

// 0x800073E0
GObjThread* gcGetGObjThread(void)
{
	GObjThread* gobjthread;

	if (sGCThreadHead == NULL)
	{
		sGCThreadHead = syTaskmanMalloc(sizeof(GObjThread), 0x8);
		sGCThreadHead->next = NULL;
	}

	if (sGCThreadHead == NULL)
	{
		syDebugPrintf("om : couldn't get GObjThread\n");
		while (TRUE);
	}

	gobjthread = sGCThreadHead;
	sGCThreadHead = sGCThreadHead->next;
	sGCThreadsActive++;

	return gobjthread;
}

// 0x8000745C
void gcSetGObjThreadPrevAlloc(GObjThread *gobjthread)
{
	gobjthread->next = sGCThreadHead;
	sGCThreadHead = gobjthread;
	sGCThreadsActive--;
}

// 0x80007488
GObjStack* gcGetGObjStackOfSize(size_t size)
{
	GObjThreadStack *curr, *prev;
	GObjStack *ret;

	curr = sGCThreadStackHead;
	prev = NULL;

	while (curr != NULL)
	{
		if (curr->size == size)
		{
			break;
		}
		prev = curr;
		curr = curr->next;
	}
	if (curr == NULL)
	{
		curr = syTaskmanMalloc(sizeof(GObjThreadStack), 0x4);
		curr->next = NULL;
		curr->stack = NULL;
		curr->size = size;

		if (prev != NULL)
		{
			prev->next = curr;
		}
		else sGCThreadStackHead = curr;
	}

	if (curr->stack != NULL)
	{
		ret = curr->stack;
		curr->stack = curr->stack->next;
	}
	else
	{
		ret = syTaskmanMalloc(size + offsetof(GObjStack, stack), 0x8);
		ret->stack_size = size;
	}
	ret->next = NULL;
	sGCStacksActiveNum++;

	return ret;
}

// 0x80007564
GObjStack* gcGetDefaultGObjStack(void)
{
	return gcGetGObjStackOfSize(sGCThreadStackSize);
}

// 0x80007588
void gcEjectGObjStack(GObjStack *gobjstack)
{
	GObjThreadStack *gobjthreadstack = sGCThreadStackHead;

	while (gobjthreadstack != NULL)
	{
		if (gobjthreadstack->size == gobjstack->stack_size)
		{
			break;
		}
		gobjthreadstack = gobjthreadstack->next;
	}
	if (gobjthreadstack == NULL)
	{
		syDebugPrintf("om : Illegal GObjThreadStack Link\n");
		while (TRUE);
	}
	gobjstack->next = gobjthreadstack->stack;
	gobjthreadstack->stack = gobjstack;
	sGCStacksActiveNum--;
}

// 0x80007604
GObjProcess* gcGetGObjProcess(void)
{
	GObjProcess *gobjproc;

	if (sGCProcessHead == NULL)
	{
		sGCProcessHead = syTaskmanMalloc(sizeof(GObjProcess), 4);
		sGCProcessHead->link_next = NULL;
#if defined(PORT) && defined(SSB64_NETMENU)
		sGCProcessHead->priority_next = NULL;
		sGCProcessHead->priority_prev = NULL;
		sGCProcessHead->priority = 0;
#endif
	}

	if (sGCProcessHead == NULL)
	{
		syDebugPrintf("om : couldn't get GObjProcess\n");
		while (TRUE);
	}

	gobjproc = sGCProcessHead;
	sGCProcessHead = sGCProcessHead->link_next;
#if defined(PORT) && defined(SSB64_NETMENU)
	(void)gcPortTryUnlinkGObjProcessFromPriorityQueue(gobjproc, "alloc");
	gcPortClearGObjProcessPriorityLinks(gobjproc);
#endif
	sGCProcessesActive++;

	return gobjproc;
}

// 0x80007680
void gcLinkGObjProcess(GObjProcess *gobjproc)
{
	GObj *parent_gobj = gobjproc->parent_gobj;
	s32 link_id = gobjproc->parent_gobj->link_id;
	GObj *prev_gobj = gobjproc->parent_gobj;

#if defined(PORT) && defined(SSB64_NETMENU)
	/*
	 * Netplay effect/rebirth repair can expose stale process free-list nodes after a same-frame
	 * eject/re-mint. Never insert a process whose old priority links still point into a live queue.
	 */
	(void)gcPortTryUnlinkGObjProcessFromPriorityQueue(gobjproc, "link_pre");
#endif
	while (TRUE)
	{
		while (prev_gobj != NULL)
		{
			GObjProcess *prev_gobjproc = prev_gobj->gobjproc_tail;

			while (prev_gobjproc != NULL)
			{
				if (prev_gobjproc->priority == gobjproc->priority)
				{
					gobjproc->priority_next = prev_gobjproc->priority_next;
					prev_gobjproc->priority_next = gobjproc;
					gobjproc->priority_prev = prev_gobjproc;

					goto loop_break;
				}
				prev_gobjproc = prev_gobjproc->link_prev;
			}
			prev_gobj = prev_gobj->link_prev;
		}
		if (link_id != 0)
		{
			prev_gobj = sGCCommonLinks[--link_id];
		}
		else
		{
			gobjproc->priority_next = sGCProcessQueue[gobjproc->priority];
			sGCProcessQueue[gobjproc->priority] = gobjproc;
			gobjproc->priority_prev = NULL;
			break;
		}
	}
loop_break:
	if (gobjproc->priority_next != NULL)
	{
		gobjproc->priority_next->priority_prev = gobjproc;
	}
	if (parent_gobj->gobjproc_tail != NULL)
	{
		parent_gobj->gobjproc_tail->link_next = gobjproc;
	}
	else parent_gobj->gobjproc_head = gobjproc;

	gobjproc->link_prev = parent_gobj->gobjproc_tail;
	gobjproc->link_next = NULL;
	parent_gobj->gobjproc_tail = gobjproc;
}

// 0x80007758
void gcSetGObjProcessPrevAlloc(GObjProcess *gobjproc)
{
#if defined(PORT) && defined(SSB64_NETMENU)
	/*
	 * The free-list link reuses link_next; priority_next/prev must not keep stale queue pointers.
	 * A later allocation can otherwise be inserted a second time and form sGCProcessQueue cycles.
	 */
	(void)gcPortTryUnlinkGObjProcessFromPriorityQueue(gobjproc, "free");
	if (gcPortGObjProcessIsOnFreeList(gobjproc) != FALSE)
	{
		port_log("SSB64: gobjproc_free_reject reason=double_free proc=%p parent=%p priority=%d\n",
		         (void *)gobjproc,
		         (void *)gobjproc->parent_gobj,
		         (int)gobjproc->priority);
		gcPortClearGObjProcessPriorityLinks(gobjproc);
		return;
	}
#endif
	gobjproc->link_next = sGCProcessHead;
#if defined(PORT) && defined(SSB64_NETMENU)
	gcPortClearGObjProcessPriorityLinks(gobjproc);
#endif
	sGCProcessHead = gobjproc;
	sGCProcessesActive--;
}

// 0x80007784
void func_80007784(GObjProcess *gobjproc)
{
#if defined(PORT) && defined(SSB64_NETMENU)
	/*
	 * Unlink by scanning from the queue head instead of trusting priority_prev/next. Those fields are
	 * stale after double-end/free-list corruption, and blind splicing is what creates priority cycles.
	 */
	(void)gcPortTryUnlinkGObjProcessFromPriorityQueue(gobjproc, "unlink");
	return;
#else
	if (gobjproc->priority_prev != NULL)
	{
		gobjproc->priority_prev->priority_next = gobjproc->priority_next;
	}
	else sGCProcessQueue[gobjproc->priority] = gobjproc->priority_next;

	if (gobjproc->priority_next != NULL)
	{
		gobjproc->priority_next->priority_prev = gobjproc->priority_prev;
	}
#endif
}

// 0x800077D0
void func_800077D0(GObjProcess *gobjproc)
{
	GObj *gobj = gobjproc->parent_gobj;

	func_80007784(gobjproc);

	if (gobjproc->link_prev != NULL)
	{
		gobjproc->link_prev->link_next = gobjproc->link_next;
	}
	else gobj->gobjproc_head = gobjproc->link_next;

	if (gobjproc->link_next != NULL)
	{
		gobjproc->link_next->link_prev = gobjproc->link_prev;
	}
	else gobj->gobjproc_tail = gobjproc->link_prev;
}

// 0x80007840
GObjProcess* gcGetCurrentGObjProcess(void)
{
	return gGCCurrentProcess;
}

// 0x8000784C
u64* gcGetGObjProcessThreadStack(GObjProcess *gobjproc)
{
	if (gobjproc == NULL)
	{
		gobjproc = gGCCurrentProcess;
	}
	if ((gobjproc != NULL) && (gobjproc->kind == nGCProcessKindThread))
	{
		return gobjproc->exec.gobjthread->stack;
	}
	else return NULL;
}

// 0x800007884
size_t gcGetGObjProcessThreadStackSize(GObjProcess *gobjproc)
{
	if (gobjproc == NULL)
	{
		gobjproc = gGCCurrentProcess;
	}
	if ((gobjproc != NULL) && (gobjproc->kind == nGCProcessKindThread))
	{
		return gobjproc->exec.gobjthread->stack_size;
	}
	else return 0;
}

// 0x800078BC
void gcSetGObjProcessFunction(void (*proc)(GObjProcess*))
{
	sGCProcessFunction = proc;
}

// 0x800078C8
s32 gcGetGObjsActiveNum(void)
{
	GObj *gobj = sGCCommonHead;
	s32 i = 0;

	while (gobj != NULL)
	{
		i++;
		gobj = gobj->link_next;
	}
	return i + sGCCommonsActiveNum;
}

// 0x800078FC
GObj* gcGetGObjSetNextAlloc(void)
{
	GObj *gobj;

	if ((sGCCommonsMaxNum == -1) || (sGCCommonsActiveNum < sGCCommonsMaxNum))
	{
		gobj = sGCCommonHead;

		if (gobj == NULL)
		{
			sGCCommonHead = syTaskmanMalloc(sGCCommonSize, 0x8);
			if (sGCCommonHead == NULL)
			{
				syDebugPrintf("om : couldn't get GObj (size=%d)\n", sGCCommonSize);
				while (TRUE);
			}
			sGCCommonHead->link_next = NULL;
			gobj = sGCCommonHead;
		}
	}
	else return NULL;

	if (gobj == NULL)
	{
		return NULL;
	}
	sGCCommonHead = gobj->link_next;
	sGCCommonsActiveNum++;

	return gobj;
}

// 0x800079A8
void gcSetGObjPrevAlloc(GObj *gobj)
{
	gobj->link_next = sGCCommonHead;
	sGCCommonHead = gobj;
	sGCCommonsActiveNum--;
}

// 0x800079D4
void gcLinkGObjAfter(GObj *this_gobj, GObj *link_gobj)
{
	this_gobj->link_prev = link_gobj;

	if (link_gobj != NULL)
	{
		this_gobj->link_next = link_gobj->link_next;
		link_gobj->link_next = this_gobj;
	}
	else
	{
		this_gobj->link_next = gGCCommonLinks[this_gobj->link_id];
		gGCCommonLinks[this_gobj->link_id] = this_gobj;
	}
	if (this_gobj->link_next != NULL)
		this_gobj->link_next->link_prev = this_gobj;
	else
		sGCCommonLinks[this_gobj->link_id] = this_gobj;
}

// 0x80007A3C
void gcLinkGObjSPAfter(GObj *this_gobj)
{
	GObj *current_gobj = sGCCommonLinks[this_gobj->link_id];

	while ((current_gobj != NULL) && (current_gobj->link_priority < this_gobj->link_priority))
	{
		current_gobj = current_gobj->link_prev;
	}
	gcLinkGObjAfter(this_gobj, current_gobj);
}

// 0x80007AA8
void gcLinkGObjSPBefore(GObj *this_gobj)
{
	GObj *current_gobj = gGCCommonLinks[this_gobj->link_id];
	GObj *found_gobj;

	while ((current_gobj != NULL) && (this_gobj->link_priority < current_gobj->link_priority))
	{
		current_gobj = current_gobj->link_next;
	}
	if (current_gobj != NULL)
	{
		found_gobj = current_gobj->link_prev;
	}
	else found_gobj = sGCCommonLinks[this_gobj->link_id];

	gcLinkGObjAfter(this_gobj, found_gobj);
}

// 0x80007B30
void gcRemoveGObjFromLinkedList(GObj *this_gobj)
{
	if (this_gobj->link_prev != NULL)
	{
		this_gobj->link_prev->link_next = this_gobj->link_next;
	}
	else gGCCommonLinks[this_gobj->link_id] = this_gobj->link_next;

	if (this_gobj->link_next != NULL)
	{
		this_gobj->link_next->link_prev = this_gobj->link_prev;
	}
	else sGCCommonLinks[this_gobj->link_id] = this_gobj->link_prev;
}

// 0x80007B98
void gcAppendGObjToDLLinkedList(GObj *this_gobj, GObj *dl_link_gobj)
{
#ifdef PORT_DIAG_HAVE_ASAN
	/* Catch the stale-GObj injection at write-time, not at first-draw read.
	 * If this_gobj points into freed/poisoned heap, the next deref below
	 * (this_gobj->dl_link_prev = ...) will trip ASan with a use-after-free
	 * report whose stack trace names the caller — the BSS holder of the
	 * stale pointer. The describe_address dump above the report names the
	 * original allocation + free sites. */
	if (__asan_region_is_poisoned((void *)this_gobj, sizeof(GObj)) != NULL) {
#if defined(SSB64_NETMENU)
		port_log("SSB64: gcAppendGObjToDLLinkedList: POISONED this_gobj=%p "
		         "(caller injected stale GObj* from freed prior-scene heap). "
		         "ASan should halt on the next deref.\n",
		         (void *)this_gobj);
#endif
		__asan_describe_address((void *)this_gobj);
	}
#endif
	this_gobj->dl_link_prev = dl_link_gobj;

	if (dl_link_gobj != NULL)
	{
		this_gobj->dl_link_next = dl_link_gobj->dl_link_next;
		dl_link_gobj->dl_link_next = this_gobj;
	}
	else
	{
		this_gobj->dl_link_next = gGCCommonDLLinks[this_gobj->dl_link_id];
		gGCCommonDLLinks[this_gobj->dl_link_id] = this_gobj;
	}

	if (this_gobj->dl_link_next != NULL)
	{
		this_gobj->dl_link_next->dl_link_prev = this_gobj;
	}
	else sGCCommonDLLinks[this_gobj->dl_link_id] = this_gobj;
}

// 0x80007C00
void gcDLLinkGObjTail(GObj *this_gobj)
{
	GObj *current_gobj = sGCCommonDLLinks[this_gobj->dl_link_id];

	while ((current_gobj != NULL) && (current_gobj->dl_link_priority < this_gobj->dl_link_priority))
	{
		current_gobj = current_gobj->dl_link_prev;
	}
	gcAppendGObjToDLLinkedList(this_gobj, current_gobj);
}

// 0x80007C6C
void gcDLLinkGObjHead(GObj *this_gobj)
{
	GObj *current_gobj = gGCCommonDLLinks[this_gobj->dl_link_id];
	GObj *found_gobj;

	while ((current_gobj != NULL) && (this_gobj->dl_link_priority < current_gobj->dl_link_priority))
	{
		current_gobj = current_gobj->dl_link_next;
	}
	if (current_gobj != NULL)
	{
		found_gobj = current_gobj->dl_link_prev;
	}
	else found_gobj = sGCCommonDLLinks[this_gobj->dl_link_id];

	gcAppendGObjToDLLinkedList(this_gobj, found_gobj);
}

// 0x80007CF4
void gcRemoveGObjFromDLLinkedList(GObj *this_gobj)
{
	if (this_gobj->dl_link_prev != NULL)
	{
		this_gobj->dl_link_prev->dl_link_next = this_gobj->dl_link_next;
	}
	else gGCCommonDLLinks[this_gobj->dl_link_id] = this_gobj->dl_link_next;

	if (this_gobj->dl_link_next != NULL)
	{
		this_gobj->dl_link_next->dl_link_prev = this_gobj->dl_link_prev;
	}
	else sGCCommonDLLinks[this_gobj->dl_link_id] = this_gobj->dl_link_prev;
}

// 0x80007D5C
XObj* gcGetXObjSetNextAlloc(void)
{
	XObj *xobj;

	if (sGCMatrixHead == NULL)
	{
		sGCMatrixHead = syTaskmanMalloc(sizeof(XObj), 0x8);
		sGCMatrixHead->next = NULL;
	}
	if (sGCMatrixHead == NULL)
	{
		syDebugPrintf("om : couldn't get OMMtx\n");
		while (TRUE);
	}
	xobj = sGCMatrixHead;
	sGCMatrixHead = sGCMatrixHead->next;
	sGCMatrixesActiveNum++;

	return xobj;
}

// 0x80007DD8
void gcSetXObjPrevAlloc(XObj *xobj)
{
	xobj->next = sGCMatrixHead;
	sGCMatrixHead = xobj;
	sGCMatrixesActiveNum--;
}

// 0x80007E04
AObj* gcGetAObjSetNextAlloc(void)
{
	AObj *aobj;

	if (sGCAnimHead == NULL)
	{
		sGCAnimHead = syTaskmanMalloc(sizeof(AObj), 0x4);

		sGCAnimHead->next = NULL;
	}
	if (sGCAnimHead == NULL)
	{
		syDebugPrintf("om : couldn't get AObj\n");
		while (TRUE);
	}
	aobj = sGCAnimHead;
	sGCAnimHead = sGCAnimHead->next;
	sGCAnimsActiveNum++;

	return aobj;
}

// 0x80007E80
void gcAppendAObjToDObj(DObj *dobj, AObj *aobj)
{
	aobj->next = dobj->aobj;
	dobj->aobj = aobj;
}

// 0x80007E90
void gcAppendAObjToMObj(MObj *mobj, AObj *aobj)
{
	aobj->next = mobj->aobj;
	mobj->aobj = aobj;
}

// 0x80007EA0
void gcAppendAObjToCamera(CObj *cobj, AObj *aobj)
{
	aobj->next = cobj->aobj;
	cobj->aobj = aobj;
}

// 0x80007EB0
void gcSetAObjPrevAlloc(AObj *aobj)
{
	aobj->next = sGCAnimHead;
	sGCAnimsActiveNum--;
	sGCAnimHead = aobj;
}

// 0x80007EDC
MObj* gcGetMObjSetNextAlloc(void)
{
	MObj *mobj;

	if (sGCMaterialHead == NULL)
	{
		sGCMaterialHead = syTaskmanMalloc(sizeof(MObj), 0x4);
		sGCMaterialHead->next = NULL;
	}

	if (sGCMaterialHead == NULL)
	{
		syDebugPrintf("om : couldn't get MObj\n");
		while (TRUE);
	}

	mobj = sGCMaterialHead;
	sGCMaterialHead = sGCMaterialHead->next;
	sGCMaterialsActive++;

	return mobj;
}

// 0x80007F58
void gcSetMObjPrevAlloc(MObj *mobj)
{
	mobj->next = sGCMaterialHead;
	sGCMaterialsActive--;
	sGCMaterialHead = mobj;
}

// 0x80007F84
DObj* gcGetDObjSetNextAlloc(void)
{
	DObj *dobj;

	if (sGCDrawHead == NULL)
	{
		sGCDrawHead = syTaskmanMalloc(sGCDrawSize, 0x8);

		sGCDrawHead->alloc_free = NULL;
	}
	if (sGCDrawHead == NULL)
	{
		syDebugPrintf("om : couldn't get DObj\n");
		while (TRUE);
	}
	dobj = sGCDrawHead;
	sGCDrawHead = sGCDrawHead->alloc_free;
	sGCDrawsActiveNum++;

	return dobj;
}

// 0x80008004
void gcSetDObjPrevAlloc(DObj *dobj)
{
	dobj->alloc_free = sGCDrawHead;
	sGCDrawsActiveNum--;
	sGCDrawHead = dobj;
}

// 0x80008030
SObj* gcGetSObjSetNextAlloc(void)
{
	SObj *sobj;

	if (sGCSpriteHead == NULL)
	{
		sGCSpriteHead = syTaskmanMalloc(sGCSpriteSize, 0x8);
		sGCSpriteHead->alloc_free = NULL;
	}
	if (sGCSpriteHead == NULL)
	{
		syDebugPrintf("om : couldn't get SObj\n");
		while (TRUE);
	}
	sobj = sGCSpriteHead;
	sGCSpriteHead = sGCSpriteHead->alloc_free;
	sGCSpritesActiveNum++;

	return sobj;
}

// 0x800080B0
void gcSetSObjPrevAlloc(SObj *sobj)
{
	sobj->alloc_free = sGCSpriteHead;
	sGCSpritesActiveNum--;
	sGCSpriteHead = sobj;
}

// 0x800080DC
CObj *gcGetCObjSetNextAlloc(void)
{
	CObj *cobj;

	if (sGCCameraHead == NULL)
	{
		sGCCameraHead = syTaskmanMalloc(sGCCameraSize, 0x8);
		sGCCameraHead->next = NULL;
	}

	if (sGCCameraHead == NULL)
	{
		syDebugPrintf("om : couldn't get Camera\n");
		while (TRUE);
	}

	cobj = sGCCameraHead;
	sGCCameraHead = sGCCameraHead->next;
	sGCCamerasActiveNum++;

	return cobj;
}

// 0x8000815C
void gcSetCObjPrevAlloc(CObj *cobj)
{
	cobj->next = sGCCameraHead;
	sGCCamerasActiveNum--;
	sGCCameraHead = cobj;
}

// 0x80008188
GObjProcess* gcAddGObjProcess(GObj *gobj, void (*proc)(GObj*), u8 kind, u32 priority)
{
	GObjStack *gobjstack;
	GObjThread *gobjthread;
	GObjProcess *gobjproc;

	if (gobj == NULL)
	{
		gobj = gGCCurrentCommon;
	}
	gobjproc = gcGetGObjProcess();

	if (priority >= 6)
	{
		syDebugPrintf("om : GObjProcess's priority is bad value\n");
		while (TRUE);
	}
	gobjproc->priority = priority;
	gobjproc->kind = kind;
	gobjproc->is_paused = FALSE;
	gobjproc->parent_gobj = gobj;
	gobjproc->func_id = proc;

	switch (kind)
	{
	case nGCProcessKindThread:
		gobjthread = gcGetGObjThread();
		gobjproc->exec.gobjthread = gobjthread;

		gobjstack = gcGetDefaultGObjStack();
		gobjthread->stack = gobjstack->stack;
		gobjthread->stack_size = sGCThreadStackSize;

		osCreateThread
		(
			&gobjthread->thread,
			dGCProcessThreadID++,
			(void (*)(void*)) proc,
			gobj,
			&gobjthread->stack[sGCThreadStackSize / sizeof(u64)],
			51
		);
		gobjthread->stack[7] = 0xFEDCBA98;

		if (dGCProcessThreadID >= 20000000)
		{
			dGCProcessThreadID = 10000000;
		}
		break;

	case nGCProcessKindFunc:
		gobjproc->exec.func = proc;
		break;
	
	default:
		syDebugPrintf("om : GObjProcess's kind is bad value\n");
		while (TRUE);
	}
	gcLinkGObjProcess(gobjproc);

	return gobjproc;
}

// 0x80008304
GObjProcess* unref_80008304(GObj *gobj, void (*proc)(GObj*), u32 pri, s32 thread_id, u32 stack_size)
{
	GObjProcess *gobjproc;	// s0
	GObjThread* gobjthread; // v1 / sp28
	GObjStack *gobjstack;
	OSId tid;

	if (gobj == NULL)
	{
		gobj = gGCCurrentCommon;
	}
	gobjproc = gcGetGObjProcess();

	if (pri >= 6)
	{
		syDebugPrintf("om : GObjProcess's priority is bad value\n");
		while (TRUE);
	}

	gobjproc->priority = pri;
	gobjproc->is_paused = FALSE;
	gobjproc->parent_gobj = gobj;
	gobjproc->func_id = proc;

	gobjproc->exec.gobjthread = gobjthread = gcGetGObjThread();
	gobjproc->kind = nGCProcessKindThread;

	gobjstack = (stack_size == 0) ? gcGetDefaultGObjStack() : gcGetGObjStackOfSize(stack_size);
	gobjthread->stack = gobjstack->stack;
	gobjthread->stack_size = (stack_size == 0) ? sGCThreadStackSize : stack_size;
	tid = (thread_id != -1) ? thread_id : dGCProcessThreadID++;

	osCreateThread
	(
		&gobjthread->thread,
		tid,
		(void (*)(void*)) proc,
		gobj,
		&gobjthread->stack[gobjthread->stack_size / sizeof(u64)],
		51
	);
	gobjthread->stack[7] = 0xFEDCBA98;

	if (dGCProcessThreadID >= 20000000)
	{
		dGCProcessThreadID = 10000000;
	}
	gcLinkGObjProcess(gobjproc);

	return gobjproc;
}

// 0x8000848C
void gcEndGObjProcess(GObjProcess *gobjproc)
{
	GObjStack *gobjstack;

	if ((gobjproc == NULL) || (gobjproc == gGCCurrentProcess))
	{
		sGCRunStatus = 1;

		if (gGCCurrentProcess->kind == nGCProcessKindThread)
		{
			gcSleepCurrentGObjThread(1);
		}
		return;
	}

	if (sGCProcessFunction != NULL)
	{
		sGCProcessFunction(gobjproc);
	}
	switch (gobjproc->kind)
	{
	case nGCProcessKindThread:
		osDestroyThread(&gobjproc->exec.gobjthread->thread);
		// cast from stack pointer back to stack node
		gobjstack = (GObjStack*) ((uintptr_t)(gobjproc->exec.gobjthread->stack) - offsetof(GObjStack, stack));
		gcEjectGObjStack(gobjstack);
		gcSetGObjThreadPrevAlloc(gobjproc->exec.gobjthread);
		break;

	case nGCProcessKindFunc: break;
	}

	func_800077D0(gobjproc);
	gcSetGObjProcessPrevAlloc(gobjproc);
}

// 0x8000855C
XObj* gcAddXObjForDObjVar(DObj *dobj, u8 kind, u8 arg2, s32 xobj_id)
{
	uintptr_t csr;
	GCTranslate* translate;
	GCRotate* rotate;
	GCScale* scale;
	XObj *xobj;
	s32 i;

	if (dobj->xobjs_num == ARRAY_COUNT(dobj->xobjs))
	{
		syDebugPrintf("om : couldn\'t add OMMtx for DObj\n");
		while (TRUE);
	}
	if (dobj->vec != NULL)
	{
		csr = (uintptr_t)dobj->vec->data;

		for (i = 0; i < ARRAY_COUNT(dobj->vec->kinds); i++)
		{
			switch (dobj->vec->kinds[i])
			{
			case nGCDrawVectorKindNone: break;

			case nGCDrawVectorKindTranslate:
				translate = (GCTranslate*)csr;
				csr += sizeof(GCTranslate);
				break;

			case nGCDrawVectorKindRotate:
				rotate = (GCRotate*)csr;
				csr += sizeof(GCRotate);
				break;

			case nGCDrawVectorKindScale:
				scale = (GCScale*)csr;
				csr += sizeof(GCScale);
				break;
			}
		}
	}
	for (i = dobj->xobjs_num; i > xobj_id; i--)
	{
		dobj->xobjs[i] = dobj->xobjs[i - 1];
	}
	dobj->xobjs_num++;

	dobj->xobjs[xobj_id] = xobj = gcGetXObjSetNextAlloc();
	xobj->kind = kind;

	switch (kind)
	{
	case nGCMatrixKindTra:
	case 34:
	case 36:
	case 38:
	case 40:
	case 55:
		dobj->translate = dGCTranslateDefault;
		dobj->translate.xobj = xobj;
		break;

	case nGCMatrixKindRotD:
	case nGCMatrixKindRotR:
		dobj->rotate = dGCRotateDefaultAXYZ;
		dobj->rotate.xobj = xobj;
		break;

	case nGCMatrixKindTraRotD:
	case nGCMatrixKindTraRotR:
		dobj->translate = dGCTranslateDefault;
		dobj->rotate = dGCRotateDefaultAXYZ;
		dobj->translate.xobj = xobj;
		dobj->rotate.xobj = xobj;
		break;

	case nGCMatrixKindRotRpyD:
	case nGCMatrixKindRotRpyR:
	case nGCMatrixKindRotPyrR:
		dobj->rotate = dGCRotateDefaultRpy;
		dobj->rotate.xobj = xobj;
		break;

	case nGCMatrixKindTraRotRpyD:
	case nGCMatrixKindTraRotRpyR:
	case nGCMatrixKindTraRotPyrR:
	case 51:
	case 52:
		dobj->translate = dGCTranslateDefault;
		dobj->rotate = dGCRotateDefaultRpy;
		dobj->translate.xobj = xobj;
		dobj->rotate.xobj = xobj;
		break;

	case nGCMatrixKindTraRotRSca:
		dobj->translate = dGCTranslateDefault;
		dobj->rotate = dGCRotateDefaultAXYZ;
		dobj->scale = dGCScaleDefault;
		dobj->translate.xobj = xobj;
		dobj->rotate.xobj = xobj;
		dobj->scale.xobj = xobj;
		break;

	case nGCMatrixKindTraRotRpyRSca:
	case nGCMatrixKindTraRotPyrRSca:
	case 54:
		dobj->translate = dGCTranslateDefault;
		dobj->rotate = dGCRotateDefaultRpy;
		dobj->scale = dGCScaleDefault;
		dobj->translate.xobj = xobj;
		dobj->rotate.xobj = xobj;
		dobj->scale.xobj = xobj;
		break;

	case nGCMatrixKindSca:
	case 43:
	case 44:
	case 47:
	case 48:
	case 49:
	case 50:
	case 53:
		dobj->scale = dGCScaleDefault;
		dobj->scale.xobj = xobj;
		break;

	case 45:
	case 46:
		dobj->rotate = dGCRotateDefaultAXYZ;
		dobj->scale = dGCScaleDefault;
		dobj->rotate.xobj = xobj;
		dobj->scale.xobj = xobj;
		break;

	case nGCMatrixKindVecTra:
		*translate = dGCTranslateDefault;
		translate->xobj = xobj;
		break;

	case nGCMatrixKindVecRotR:
		*rotate = dGCRotateDefaultAXYZ;
		rotate->xobj = xobj;
		break;

	case nGCMatrixKindVecRotRpyR:
		*rotate = dGCRotateDefaultRpy;
		rotate->xobj = xobj;
		break;

	case nGCMatrixKindVecSca:
		*scale = dGCScaleDefault;
		scale->xobj = xobj;
		break;

	case nGCMatrixKindVecTraRotR:
		*translate = dGCTranslateDefault;
		*rotate = dGCRotateDefaultAXYZ;

		translate->xobj = rotate->xobj = xobj;
		break;

	case nGCMatrixKindVecTraRotRSca:
		*translate = dGCTranslateDefault;
		*rotate = dGCRotateDefaultAXYZ;
		*scale = dGCScaleDefault;

		translate->xobj = rotate->xobj = scale->xobj = xobj;
		break;

	case nGCMatrixKindVecTraRotRpyR:
		*translate = dGCTranslateDefault;
		*rotate = dGCRotateDefaultRpy;

		translate->xobj = rotate->xobj = xobj;
		break;

	case nGCMatrixKindVecTraRotRpyRSca:
		*translate = dGCTranslateDefault;
		*rotate = dGCRotateDefaultRpy;
		*scale = dGCScaleDefault;

		translate->xobj = rotate->xobj = scale->xobj = xobj;
		break;

	case 1:
	case 17: break;
	}
	xobj->unk05 = arg2;

	return xobj;
}

// 0x80008CC0
// This actually returns gcAddXObjForDObjVar, see https://decomp.me/scratch/nIQ4X
XObj* gcAddXObjForDObjFixed(DObj *dobj, u8 kind, u8 arg2)
{
	return gcAddXObjForDObjVar(dobj, kind, arg2, dobj->xobjs_num);
}

// 0x80008CF0
XObj* gcAddXObjForCamera(CObj *cobj, u8 kind, u8 arg2)
{
	XObj *xobj;

	if (cobj->xobjs_num == ARRAY_COUNT(cobj->xobjs))
	{
		syDebugPrintf("om : couldn't add OMMtx for Camera\n");
		while (TRUE);
	}
	xobj = gcGetXObjSetNextAlloc();

	cobj->xobjs[cobj->xobjs_num] = xobj;
	cobj->xobjs_num++;

	xobj->kind = kind;

	switch (kind)
	{
	case nGCMatrixKindPerspFastF:
	case nGCMatrixKindPerspF:
		cobj->projection.persp = dGCPerspDefault;
		cobj->projection.persp.xobj = xobj;
		break;

	case nGCMatrixKindOrtho:
		cobj->projection.ortho = dGCOrthoDefault;
		cobj->projection.ortho.xobj = xobj;
		break;

	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
	case 17:
		cobj->vec = dGCCObjVecDefault;
		cobj->vec.xobj = xobj;
		break;

	case 1:
	case 2: break;
	}
	xobj->unk05 = arg2;

	return xobj;
}

// 0x80008E78
AObj* gcAddAObjForDObj(DObj *dobj, u8 track)
{
	AObj *aobj = gcGetAObjSetNextAlloc();

	aobj->track = track;
	aobj->kind = nGCAnimKindNone;
	aobj->interpolate = NULL;
	aobj->rate_target = 0.0F;
	aobj->rate_base = 0.0F;
	aobj->value_target = 0.0F;
	aobj->value_base = 0.0F;
	aobj->length = 0.0F;
	aobj->length_invert = 1.0F;

	gcAppendAObjToDObj(dobj, aobj);

	return aobj;
}

// 0x80008EE4
void gcRemoveAObjFromDObj(DObj *dobj)
{
	AObj *current_aobj;
	AObj *next_aobj;

	current_aobj = dobj->aobj;

	while (current_aobj != NULL)
	{
		next_aobj = current_aobj->next;
		gcSetAObjPrevAlloc(current_aobj);
		current_aobj = next_aobj;
	}
	dobj->aobj = NULL;
	dobj->anim_wait = AOBJ_ANIM_NULL;
}

// 0x80008F44
AObj* gcAddAObjForMObj(MObj *mobj, u8 track)
{
	AObj *aobj = gcGetAObjSetNextAlloc();

	aobj->track = track;
	aobj->kind = nGCAnimKindNone;
	aobj->interpolate = NULL;
	aobj->rate_target = 0.0F;
	aobj->rate_base = 0.0F;
	aobj->value_target = 0.0F;
	aobj->value_base = 0.0F;
	aobj->length = 0.0F;
	aobj->length_invert = 1.0F;

	gcAppendAObjToMObj(mobj, aobj);

	return aobj;
}

// 0x80008FB0
// free struct AObj list at unk90
void gcRemoveAObjFromMObj(MObj *mobj)
{
	AObj *current_aobj;
	AObj *next_aobj;

	current_aobj = mobj->aobj;

	while (current_aobj != NULL)
	{
		next_aobj = current_aobj->next;
		gcSetAObjPrevAlloc(current_aobj);
		current_aobj = next_aobj;
	}
	mobj->aobj = NULL;
	mobj->anim_wait = AOBJ_ANIM_NULL;
}

// 0x80009010
AObj* gcAddAObjForCamera(CObj *cobj, u8 track)
{
	AObj *aobj = gcGetAObjSetNextAlloc();

	aobj->track = track;
	aobj->kind = nGCAnimKindNone;
	aobj->interpolate = NULL;
	aobj->rate_target = 0.0F;
	aobj->rate_base = 0.0F;
	aobj->value_target = 0.0F;
	aobj->value_base = 0.0F;
	aobj->length = 0.0F;
	aobj->length_invert = 1.0F;

	gcAppendAObjToCamera(cobj, aobj);

	return aobj;
}

// 0x8000907C
void gcRemoveAObjFromCamera(CObj *cobj)
{
	AObj *current_aobj;
	AObj *next_aobj;

	current_aobj = cobj->aobj;

	while (current_aobj != NULL)
	{
		next_aobj = current_aobj->next;
		gcSetAObjPrevAlloc(current_aobj);
		current_aobj = next_aobj;
	}
	cobj->aobj = NULL;
	cobj->anim_wait = AOBJ_ANIM_NULL;
}

// 0x800090DC
MObj* gcAddMObjForDObj(DObj *dobj, MObjSub* mobjsub)
{
	MObj *mobj = gcGetMObjSetNextAlloc();

	if (dobj->mobj != NULL)
	{
		MObj *current_mobj = dobj->mobj->next;
		MObj *prior_mobj = dobj->mobj;

		while (current_mobj != NULL)
		{
			prior_mobj = current_mobj;
			current_mobj = current_mobj->next;
		}
		prior_mobj->next = mobj;
	}
	else dobj->mobj = mobj;

	mobj->next = NULL;
	mobj->lfrac = mobjsub->prim_l / 255.0F;
	mobj->sub = *mobjsub;

	mobj->sub.unk24 = mobjsub->trau;
	mobj->sub.unk28 = mobjsub->scau;
	mobj->texture_id_curr = 0;
	mobj->texture_id_next = 0;
	mobj->palette_id = 0;
	mobj->aobj = NULL;
	mobj->matanim_joint.event32 = NULL;
	mobj->anim_wait = AOBJ_ANIM_NULL;
	mobj->anim_speed = 1.0F;
	mobj->anim_frame = 0.0F;

	return mobj;
}

// 0x800091F4
void gcRemoveMObjAll(DObj *dobj)
{
	MObj *current_mobj;
	MObj *next_mobj;
	AObj *current_aobj;
	AObj *next_aobj;

	current_mobj = dobj->mobj;

	while (current_mobj != NULL)
	{
		current_aobj = current_mobj->aobj;

		while (current_aobj != NULL)
		{
			next_aobj = current_aobj->next;
			gcSetAObjPrevAlloc(current_aobj);
			current_aobj = next_aobj;
		}
		next_mobj = current_mobj->next;
		gcSetMObjPrevAlloc(current_mobj);
		current_mobj = next_mobj;
	}
	dobj->mobj = NULL;
}

// 0x8000926C
void gcInitDObj(DObj *dobj)
{
	s32 i;

	dobj->vec = NULL;
	dobj->flags = DOBJ_FLAG_NONE;
	dobj->is_anim_root = FALSE;
	dobj->xobjs_num = 0;

	for (i = 0; i < ARRAY_COUNT(dobj->xobjs); i++)
	{
		dobj->xobjs[i] = NULL;
	}
	dobj->aobj = NULL;
	dobj->anim_joint.event32 = NULL;
	dobj->anim_wait = AOBJ_ANIM_NULL;
	dobj->anim_speed = 1.0F;
	dobj->anim_frame = 0.0F;
	dobj->mobj = NULL;
	dobj->user_data.p = NULL;
}

// 0x800092D0
DObj* gcAddDObjForGObj(GObj *gobj, void *dvar)
{
	DObj *new_dobj, *current_dobj;

	if (gobj == NULL)
	{
		gobj = gGCCurrentCommon;
	}
	new_dobj = gcGetDObjSetNextAlloc();

	if (gobj->obj != NULL)
	{
		current_dobj = DObjGetStruct(gobj);

		while (current_dobj->sib_next != NULL)
		{
			current_dobj = current_dobj->sib_next;
		}
		current_dobj->sib_next = new_dobj;
		new_dobj->sib_prev = current_dobj;
	}
	else
	{
		gobj->obj_kind = nGCCommonAppendDObj;
		gobj->obj = new_dobj;
		new_dobj->sib_prev = NULL;
	}
	new_dobj->parent_gobj = gobj;
	new_dobj->parent = DOBJ_PARENT_NULL;
	new_dobj->sib_next = NULL;
	new_dobj->child = NULL;
	new_dobj->dv = dvar;

	gcInitDObj(new_dobj);

	return new_dobj;
}

// 0x80009380
DObj* gcAddSiblingForDObj(DObj *dobj, void *dvar)
{
	DObj *new_dobj = gcGetDObjSetNextAlloc();

	if (dobj->sib_next != NULL)
	{
		dobj->sib_next->sib_prev = new_dobj;
	}
	new_dobj->sib_prev = dobj;
	new_dobj->sib_next = dobj->sib_next;
	dobj->sib_next = new_dobj;
	new_dobj->parent_gobj = dobj->parent_gobj;
	new_dobj->parent = dobj->parent;

	new_dobj->child = NULL;
	new_dobj->dv = dvar;

	gcInitDObj(new_dobj);

	return new_dobj;
}

// 0x800093F4
DObj* gcAddChildForDObj(DObj *dobj, void *dvar)
{
	DObj *new_dobj = gcGetDObjSetNextAlloc();
	DObj *current_dobj;

	if (dobj->child != NULL)
	{
		current_dobj = dobj->child;

		while (current_dobj->sib_next != NULL)
		{
			current_dobj = current_dobj->sib_next;
		}
		current_dobj->sib_next = new_dobj;
		new_dobj->sib_prev = current_dobj;
	}
	else
	{
		dobj->child = new_dobj;
		new_dobj->sib_prev = NULL;
	}
	new_dobj->parent_gobj = dobj->parent_gobj;
	new_dobj->parent = dobj;
	new_dobj->child = NULL;
	new_dobj->sib_next = NULL;
	new_dobj->dv = dvar;

	gcInitDObj(new_dobj);

	return new_dobj;
}

// 0x8000948C
// drop_dobj, cleanup_dobj, gcSetDObjPrevAlloc?
void gcEjectDObj(DObj *dobj)
{
	s32 i;
	AObj *current_aobj, *next_aobj;
	MObj *current_mobj, *next_mobj;

	while (dobj->child != NULL)
	{
		gcEjectDObj(dobj->child);
	}
	if (dobj->parent == (DObj*)1)
	{
		if (dobj == DObjGetStruct(dobj->parent_gobj))
		{
			dobj->parent_gobj->obj = dobj->sib_next;

			if (DObjGetStruct(dobj->parent_gobj) == NULL)
			{
				dobj->parent_gobj->obj_kind = nGCCommonAppendNone;
			}
		}
	}
	else if (dobj == dobj->parent->child)
	{
		dobj->parent->child = dobj->sib_next;
	}
	if (dobj->sib_prev != NULL)
	{
		dobj->sib_prev->sib_next = dobj->sib_next;
	}
	if (dobj->sib_next != NULL)
	{
		dobj->sib_next->sib_prev = dobj->sib_prev;
	}
	for (i = 0; i < ARRAY_COUNT(dobj->xobjs); i++)
	{
		if (dobj->xobjs[i] != NULL)
		{
			gcSetXObjPrevAlloc(dobj->xobjs[i]);
		}
	}
	if ((dobj->vec != NULL) && (sGCDrawFuncEject != NULL))
	{
		sGCDrawFuncEject(dobj->vec);
	}
	current_aobj = dobj->aobj;

	while (current_aobj != NULL)
	{
		next_aobj = current_aobj->next;
		gcSetAObjPrevAlloc(current_aobj);
		current_aobj = next_aobj;
	}
	current_mobj = dobj->mobj;

	while (current_mobj != NULL)
	{
		current_aobj = current_mobj->aobj;

		while (current_aobj != NULL)
		{
			next_aobj = current_aobj->next;
			gcSetAObjPrevAlloc(current_aobj);
			current_aobj = next_aobj;
		}
		next_mobj = current_mobj->next;
		gcSetMObjPrevAlloc(current_mobj);
		current_mobj = next_mobj;
	}

#ifdef PORT
	/* Clear display union so recycled DObjs never keep a dangling dl_link/dv. */
	dobj->dv = NULL;
#endif

	gcSetDObjPrevAlloc(dobj);
}

// 0x80009614
SObj* gcAddSObjForGObj(GObj *gobj, Sprite *sprite)
{
	SObj *new_sobj;

	if (gobj == NULL)
	{
		gobj = gGCCurrentCommon;
	}
	new_sobj = gcGetSObjSetNextAlloc();

	if (SObjGetStruct(gobj) != NULL)
	{
		SObj *tail_sobj = SObjGetStruct(gobj);

		while (tail_sobj->next != NULL)
		{
			tail_sobj = tail_sobj->next;
		}
		tail_sobj->next = new_sobj;
		new_sobj->prev = tail_sobj;
	}
	else
	{
		gobj->obj_kind = nGCCommonAppendSObj;
		gobj->obj = new_sobj;
		new_sobj->prev = NULL;
	}
	new_sobj->parent_gobj = gobj;
	new_sobj->next = NULL;

	if (sprite != NULL)
	{
		new_sobj->sprite = *sprite;
	}
#ifdef PORT
	else
	{
		/* gcGetSObjSetNextAlloc pulls from the recycled-SObj freelist
		 * with no zero-init, so the embedded Sprite carries whatever the
		 * previous occupant wrote (including `nbitmaps` and `bitmap`
		 * token). Zero it on the NULL-sprite path (e.g. ifcommon.c:1003)
		 * so the next caller's `attr = SP_HIDDEN` is the only field
		 * driving the renderer rather than leaking stale data. */
		bzero(&new_sobj->sprite, sizeof(new_sobj->sprite));
	}
#endif
	new_sobj->user_data.p = NULL;

	return new_sobj;
}

// 0x800096EC
void gcEjectSObj(SObj *sobj)
{
	if (sobj == SObjGetStruct(sobj->parent_gobj))
	{
		sobj->parent_gobj->obj = sobj->next;

		if (SObjGetStruct(sobj->parent_gobj) == NULL)
		{
			sobj->parent_gobj->obj_kind = nGCCommonAppendNone;
		}
	}
	if (sobj->prev != NULL)
	{
		sobj->prev->next = sobj->next;
	}
	if (sobj->next != NULL)
	{
		sobj->next->prev = sobj->prev;
	}
	gcSetSObjPrevAlloc(sobj);
}

CObj* gcAddCameraForGObj(GObj *gobj)
{
	s32 i;
	CObj *new_cobj;

	if (gobj == NULL)
	{
		gobj = gGCCurrentCommon;
	}
	gobj->obj_kind = nGCCommonAppendCamera;

	new_cobj = gcGetCObjSetNextAlloc();
	gobj->obj = new_cobj;
	new_cobj->parent_gobj = gobj;

	syRdpSetDefaultViewport(&new_cobj->viewport);

	new_cobj->xobjs_num = 0;

	for (i = 0; i < ARRAY_COUNT(new_cobj->xobjs); i++)
	{
		new_cobj->xobjs[i] = NULL;
	}
	new_cobj->flags = COBJ_FLAG_NONE;
	new_cobj->color = GPACK_RGBA8888(0x00, 0x00, 0x00, 0x00);
	new_cobj->func_camera = NULL;
	new_cobj->unk_camera_0x8C = 0;

	new_cobj->aobj = NULL;
	new_cobj->camanim_joint.event32 = NULL;

	new_cobj->anim_wait = AOBJ_ANIM_NULL;
	new_cobj->anim_speed = 1.0F;
	new_cobj->anim_frame = 0.0F;

	return new_cobj;
}

// 0x80009810
void gcEjectCamera(CObj *cobj)
{
	GObj *gobj;
	s32 i;
	AObj *current_aobj;
	AObj *next_aobj;

	gobj = cobj->parent_gobj;
	gobj->obj_kind = nGCCommonAppendNone;
	gobj->obj = NULL;

	for (i = 0; i < ARRAY_COUNT(cobj->xobjs); i++)
	{
		if (cobj->xobjs[i] != NULL)
			gcSetXObjPrevAlloc(cobj->xobjs[i]);
	}
	current_aobj = cobj->aobj;

	while (current_aobj != NULL)
	{
		next_aobj = current_aobj->next;
		gcSetAObjPrevAlloc(current_aobj);
		current_aobj = next_aobj;
	}
	gcSetCObjPrevAlloc(cobj);
}

// 0x800098A4
GObj* gcInitGObjCommon(u32 id, void (*func_run)(GObj*), u8 link, u32 priority)
{
	GObj *new_gobj;

	if (link >= ARRAY_COUNT(gGCCommonLinks))
	{
		syDebugPrintf("omGAddCommon() : link num over : link = %d : id = %d\n", link, id);
		while (TRUE);
	}

	new_gobj = gcGetGObjSetNextAlloc();

	if (new_gobj == NULL)
		return NULL;

	new_gobj->id = id;
	new_gobj->link_id = link;
	new_gobj->link_priority = priority;
	new_gobj->func_run = func_run;
	new_gobj->gobjproc_head = NULL;
	new_gobj->gobjproc_tail = NULL;
	new_gobj->gobjscripts_num = 0;
	new_gobj->flags = GOBJ_FLAG_NONE;

	new_gobj->obj_kind = nGCCommonAppendNone;
	new_gobj->obj = NULL;

	new_gobj->dl_link_id = ARRAY_COUNT(gGCCommonDLLinks);
	new_gobj->anim_frame = 0.0F;
	new_gobj->func_anim = NULL;
	new_gobj->user_data.p = NULL;
	new_gobj->proc_display = NULL;
	new_gobj->camera_mask = 0;
	new_gobj->camera_tag = ~0;

	return new_gobj;
}

// 0x80009968
GObj* gcMakeGObjSPAfter(u32 id, void (*func_run)(GObj*), u8 link, u32 priority)
{
	GObj *new_gobj = gcInitGObjCommon(id, func_run, link, priority);

	if (new_gobj == NULL)
	{
		return NULL;
	}
	gcLinkGObjSPAfter(new_gobj);

	PORT_LOG_GOBJ_ALLOC(new_gobj, id, link);
	return new_gobj;
}

// 0x800099A8
GObj* gcMakeGObjSPBefore(u32 id, void (*func_run)(GObj*), u8 link, u32 priority)
{
	GObj *new_gobj = gcInitGObjCommon(id, func_run, link, priority);

	if (new_gobj == NULL)
	{
		return NULL;
	}
	gcLinkGObjSPBefore(new_gobj);

	PORT_LOG_GOBJ_ALLOC(new_gobj, id, link);
	return new_gobj;
}

// 0x800099E8
GObj* gcMakeGObjAfter(u32 id, void (*func_run)(GObj*), GObj *link_gobj)
{
	GObj *new_gobj = gcInitGObjCommon(id, func_run, link_gobj->link_id, link_gobj->link_priority);

	if (new_gobj == NULL)
	{
		return NULL;
	}
	gcLinkGObjAfter(new_gobj, link_gobj);

	PORT_LOG_GOBJ_ALLOC(new_gobj, id, link_gobj->link_id);
	return new_gobj;
}

// 0x80009A34
GObj* gcMakeGObjBefore(u32 id, void (*func_run)(GObj*), GObj *link_gobj)
{
	GObj *new_gobj = gcInitGObjCommon(id, func_run, link_gobj->link_id, link_gobj->link_priority);

	if (new_gobj == NULL)
	{
		return NULL;
	}
	gcLinkGObjAfter(new_gobj, link_gobj->link_prev);

	PORT_LOG_GOBJ_ALLOC(new_gobj, id, link_gobj->link_id);
	return new_gobj;
}

// 0x80009A84
/* PORT: 0xFE is a sentinel written into obj_kind after a successful eject.
 * Decomp sets obj_kind to {0,1,2,3} (None/DObj/SObj/CObj); gcInitGObjCommon
 * resets it to 0 on every re-alloc (see line ~1719), so a live gobj can
 * never legitimately observe 0xFE. Detecting it tells us the caller is
 * double-ejecting a freed gobj — which corrupts the free list and
 * surfaces later as a zombie pointer deref. */
#define GOBJ_PORT_EJECTED_SENTINEL 0xFE
#if defined(PORT) && defined(SSB64_NETMENU)
#include <sys/objman_gcport.h>
#include <sys/netinput.h>
#include <sys/netpeer.h>

extern void efManagerNetplayTeardownParticleCouplingBeforeForwardEject(GObj *effect_gobj);

static sb32 gcPortGObjEjectTraceEnabled(void);
#endif
void gcEjectGObj(GObj *gobj)
{
	if ((gobj == NULL) || (gobj == gGCCurrentCommon))
	{
		sGCRunStatus = 2;
		return;
	}

	/* PORT: guard against double-eject. If we see the sentinel, the gobj
	 * is already on the free list. Walking the list again would remove a
	 * different gobj from its linked list (stale link_prev/link_next) and
	 * push this gobj onto the free list a second time. Log loudly and
	 * bail so the game stays alive long enough to diagnose the caller. */
	if (gobj->obj_kind == GOBJ_PORT_EJECTED_SENTINEL) {
#if defined(SSB64_NETMENU)
		port_log("SSB64: gcEjectGObj DOUBLE-EJECT DETECTED gobj=%p id=%u "
		         "link_id=%u dl_link_id=%u link_prev=%p link_next=%p — bailing\n",
		         (void*)gobj, gobj->id,
		         (unsigned)gobj->link_id, (unsigned)gobj->dl_link_id,
		         (void*)gobj->link_prev, (void*)gobj->link_next);
#endif
		return;
	}

#if defined(PORT) && defined(SSB64_NETMENU)
	if ((gobj->link_id == nGCCommonLinkIDEffect) || (gobj->link_id == nGCCommonLinkIDSpecialEffect))
	{
		efManagerNetplayTeardownParticleCouplingBeforeForwardEject(gobj);
	}
#endif

	/* PORT crash-diag: log eject so we can correlate with a later crash. */
#if defined(PORT) && defined(SSB64_NETMENU)
	if (gcPortGObjEjectTraceEnabled() != FALSE)
	{
		port_log("SSB64: gcEjectGObj ENTER sim_tick=%u gobj=%p id=%u kind=%u link_id=%u dl_link_id=%u "
		         "gpr_head=%p obj=%p link_next=%p link_prev=%p\n",
		         (unsigned int)syNetInputGetTick(), (void *)gobj, gobj->id, (unsigned)gobj->obj_kind,
		         (unsigned)gobj->link_id, (unsigned)gobj->dl_link_id, (void *)gobj->gobjproc_head, gobj->obj,
		         (void *)gobj->link_next, (void *)gobj->link_prev);
		gcPortRecordGObjEject(gobj);
	}
#endif

	gcEndProcessAll(gobj);

	switch (gobj->obj_kind)
	{
	case nGCCommonAppendDObj: gcRemoveDObjAll(gobj); break;

	case nGCCommonAppendSObj: gcRemoveSObjAll(gobj); break;

	case nGCCommonAppendCamera: gcEjectCamera(CObjGetStruct(gobj)); break;
	}

	if (gobj->dl_link_id != ARRAY_COUNT(gGCCommonDLLinks))
		gcRemoveGObjFromDLLinkedList(gobj);

	gcRemoveGObjFromLinkedList(gobj);
	gcSetGObjPrevAlloc(gobj);

	/* PORT: stamp sentinel AFTER the gobj is safely on the free list so
	 * any subsequent eject attempt is detected above. */
	gobj->obj_kind = GOBJ_PORT_EJECTED_SENTINEL;

#if defined(PORT) && defined(SSB64_NETMENU)
	if (gcPortGObjEjectTraceEnabled() != FALSE)
	{
		port_log("SSB64: gcEjectGObj EXIT gobj=%p\n", (void*)gobj);
	}
#endif
}

// 0x80009B48
void gcMoveGObjCommon(s32 sw, GObj *this_gobj, u8 link, u32 priority, GObj *other_gobj)
{
	GObjProcess *current_gobjproc;
	GObjProcess *orig_gobjproc;
	GObjProcess *next_gobjproc;

	if (link >= ARRAY_COUNT(gGCCommonLinks))
	{
		syDebugPrintf("omGMoveCommon() : link num over : link = %d : id = %d\n", link, this_gobj->id);

		while (TRUE);
	}

	if (this_gobj == NULL)
		this_gobj = gGCCurrentCommon;

	orig_gobjproc = this_gobj->gobjproc_head;

	this_gobj->gobjproc_head = NULL;
	this_gobj->gobjproc_tail = NULL;

	current_gobjproc = orig_gobjproc;

	while (current_gobjproc != NULL)
	{
		func_80007784(current_gobjproc);
		current_gobjproc = current_gobjproc->link_next;
	}
	gcRemoveGObjFromLinkedList(this_gobj);

	this_gobj->link_id = link;
	this_gobj->link_priority = priority;

	switch (sw)
	{
	case 0: gcLinkGObjSPAfter(this_gobj); break;

	case 1: gcLinkGObjSPBefore(this_gobj); break;

	case 2: gcLinkGObjAfter(this_gobj, other_gobj); break;

	case 3: gcLinkGObjAfter(this_gobj, other_gobj->link_prev); break;
	}

	current_gobjproc = orig_gobjproc;

	while (current_gobjproc != NULL)
	{
		next_gobjproc = current_gobjproc->link_next;
		gcLinkGObjProcess(current_gobjproc);
		current_gobjproc = next_gobjproc;
	}
}

// 0x80009C90
void func_80009C90(GObj *gobj, u8 link, u32 priority) { gcMoveGObjCommon(0, gobj, link, priority, NULL); }

// 0x80009CC8
void func_80009CC8(GObj *gobj, u8 link, u32 priority) { gcMoveGObjCommon(1, gobj, link, priority, NULL); }

// 0x80009D00
void unref_80009D00(GObj *this_gobj, GObj *other_gobj)
{
	gcMoveGObjCommon(2, this_gobj, other_gobj->link_id, other_gobj->link_priority, other_gobj);
}

// 0x80009D3C
void unref_80009D3C(GObj *this_gobj, GObj *other_gobj)
{
	gcMoveGObjCommon(3, this_gobj, other_gobj->link_id, other_gobj->link_priority, other_gobj);
}

// 0x80009D78
void gcLinkGObjDLCommon(GObj *gobj, void (*proc_display)(GObj*), u8 dl_link, u32 dl_order, u32 camera_tag)
{
	if (dl_link >= ARRAY_COUNT(gGCCommonDLLinks) - 1)
	{
		syDebugPrintf("omGLinkObjDLCommon() : dl_link num over : dl_link = %d : id = %d\n", dl_link, gobj->id);
		while (TRUE);
	}

	gobj->dl_link_id = dl_link;
	gobj->dl_link_priority = dl_order;
	gobj->proc_display = proc_display;
	gobj->camera_tag = camera_tag;
	gobj->frame_draw_last = dSYTaskmanFrameCount - 1;
}

// 0x80009DF4
void gcAddGObjDisplay(GObj *gobj, void (*proc_display)(GObj*), u8 dl_link, u32 priority, u32 camera_tag)
{
	if (gobj == NULL)
	{
		gobj = gGCCurrentCommon;
	}
	gcLinkGObjDLCommon(gobj, proc_display, dl_link, priority, camera_tag);
	gcDLLinkGObjTail(gobj);
}

// 0x80009E38
void unref_80009E38(GObj *gobj, void (*proc_display)(GObj*), u8 dl_link, u32 priority, u32 camera_tag)
{
	if (gobj == NULL)
		gobj = gGCCurrentCommon;

	gcLinkGObjDLCommon(gobj, proc_display, dl_link, priority, camera_tag);
	gcDLLinkGObjHead(gobj);
}

// 0x80009E7C
void unref_80009E7C(GObj *this_gobj, void (*proc_display)(GObj*), s32 arg2, GObj *other_gobj)
{
	if (this_gobj == NULL)
		this_gobj = gGCCurrentCommon;

	gcLinkGObjDLCommon(this_gobj, proc_display, other_gobj->dl_link_id, other_gobj->dl_link_priority, arg2);
	gcAppendGObjToDLLinkedList(this_gobj, other_gobj);
}

// 0x80009ED0
void unref_80009ED0(GObj *this_gobj, void (*proc_display)(GObj*), s32 arg2, GObj *other_gobj)
{
	if (this_gobj == NULL)
		this_gobj = gGCCurrentCommon;

	gcLinkGObjDLCommon(this_gobj, proc_display, other_gobj->dl_link_id, other_gobj->dl_link_priority, arg2);
	gcAppendGObjToDLLinkedList(this_gobj, other_gobj->link_prev);
}

// 0x80009F28
void func_80009F28(GObj *gobj, void (*proc_display)(GObj*), u32 priority, u64 arg3, u32 camera_tag)
{
	gobj->dl_link_id = ARRAY_COUNT(gGCCommonDLLinks) - 1;
	gobj->dl_link_priority = priority;
	gobj->proc_display = proc_display;
	gobj->camera_mask = arg3;
	gobj->camera_tag = camera_tag;
	gobj->buffer_mask = 0;
	gobj->frame_draw_last = dSYTaskmanFrameCount - 1;
}

// 0x80009F74
void func_80009F74(GObj *gobj, void (*proc_display)(GObj*), u32 priority, u64 arg3, u32 camera_tag)
{
	if (gobj == NULL)
	{
		gobj = gGCCurrentCommon;
	}
	func_80009F28(gobj, proc_display, priority, arg3, camera_tag);
	gcDLLinkGObjTail(gobj);
}

// 0x80009FC0
void unref_80009FC0(GObj *gobj, void (*proc_display)(GObj*), u32 priority, u64 arg3, u32 camera_tag)
{
	if (gobj == NULL)
	{
		gobj = gGCCurrentCommon;
	}
	func_80009F28(gobj, proc_display, priority, arg3, camera_tag);
	gcDLLinkGObjHead(gobj);
}

// 0x8000A00C
void unref_8000A00C(GObj *this_gobj, void (*proc_display)(GObj*), u64 arg2, s32 arg3, GObj *other_gobj)
{
	if (this_gobj == NULL)
	{
		this_gobj = gGCCurrentCommon;
	}
	func_80009F28(this_gobj, proc_display, other_gobj->dl_link_priority, arg2, arg3);
	gcAppendGObjToDLLinkedList(this_gobj, other_gobj);
}

// 0x8000A06C
void unref_8000A06C(GObj *this_gobj, void (*proc_display)(GObj*), u64 arg2, s32 arg3, GObj *other_gobj)
{
	if (this_gobj == NULL)
	{
		this_gobj = gGCCurrentCommon;
	}
	func_80009F28(this_gobj, proc_display, other_gobj->dl_link_priority, arg2, arg3);
	gcAppendGObjToDLLinkedList(this_gobj, other_gobj->link_prev);
}

// 0x8000A0D0
void gcMoveGObjDL(GObj *gobj, u8 dl_link, u32 priority)
{
	if (dl_link >= ARRAY_COUNT(gGCCommonDLLinks) - 1)
	{
		syDebugPrintf("omGMoveObjDL() : dl_link num over : dl_link = %d : id = %d\n", dl_link, gobj->id);
		while (TRUE);
	}
	gcRemoveGObjFromDLLinkedList(gobj);

	gobj->dl_link_id = dl_link;
	gobj->dl_link_priority = priority;

	gcDLLinkGObjTail(gobj);
}

// 0x8000A14C
void gcMoveGObjDLHead(GObj *gobj, u8 dl_link, u32 priority)
{
	if (dl_link >= ARRAY_COUNT(gGCCommonDLLinks) - 1)
	{
		syDebugPrintf("omGMoveObjDLHead() : dl_link num over : dl_link = %d : id = %d\n", dl_link, gobj->id);
		while (TRUE);
	}
	gcRemoveGObjFromDLLinkedList(gobj);
	
	gobj->dl_link_id = dl_link;
	gobj->dl_link_priority = priority;

	gcDLLinkGObjHead(gobj);
}

// 0x8000A1C8
void unref_8000A1C8(GObj *this_gobj, GObj *other_gobj)
{
	gcRemoveGObjFromDLLinkedList(this_gobj);
	this_gobj->dl_link_id = other_gobj->dl_link_id;
	this_gobj->dl_link_priority = other_gobj->dl_link_priority;
	gcAppendGObjToDLLinkedList(this_gobj, other_gobj);
}

// 0x8000A208
void unref_8000A208(GObj *this_gobj, GObj *other_gobj)
{
	gcRemoveGObjFromDLLinkedList(this_gobj);
	this_gobj->dl_link_id = other_gobj->dl_link_id;
	this_gobj->dl_link_priority = other_gobj->dl_link_priority;
	gcAppendGObjToDLLinkedList(this_gobj, other_gobj->dl_link_prev);
}

// 0x8000A24C
void func_8000A24C(GObj *gobj, u32 priority)
{
	gcRemoveGObjFromDLLinkedList(gobj);
	gobj->dl_link_priority = priority;
	gcDLLinkGObjTail(gobj);
}

// 0x8000A280
void unref_8000A280(GObj *gobj, u32 priority)
{
	gcRemoveGObjFromDLLinkedList(gobj);
	gobj->dl_link_priority = priority;
	gcDLLinkGObjHead(gobj);
}

// 0x8000A2B4
void func_8000A2B4(GObj *this_gobj, GObj *other_gobj)
{
	gcRemoveGObjFromDLLinkedList(this_gobj);
	this_gobj->dl_link_priority = other_gobj->dl_link_priority;
	gcAppendGObjToDLLinkedList(this_gobj, other_gobj);
}

// 0x8000A2EC
void unref_8000A2EC(GObj *this_gobj, GObj *other_gobj)
{
	gcRemoveGObjFromDLLinkedList(this_gobj);
	this_gobj->dl_link_priority = other_gobj->dl_link_priority;
	gcAppendGObjToDLLinkedList(this_gobj, other_gobj->link_prev);
}

// 0x8000A328
void gcSetMaxNumGObj(s32 num)
{
	sGCCommonsMaxNum = num;
}

// 0x8000A334
s16 gcGetMaxNumGObj(void)
{
	return sGCCommonsMaxNum;
}

// 0x8000A340
void gcDrawAll(void)
{
	s32 i;
	s32 frame_count;
	GObj *gobj;

	gGCCurrentCamera = NULL;
	gGCCurrentDisplay = NULL;

	for (i = 0, frame_count = dSYTaskmanFrameCount - 1; i < ARRAY_COUNT(gGCFrameQueueGfxLinks); i++)
	{
		gGCFrameQueueGfxLinks[i].frame = frame_count;
	}
	gobj = gGCCommonDLLinks[ARRAY_COUNT(gGCCommonDLLinks) - 1];

	while (gobj != NULL)
	{
		if (!(gobj->flags & GOBJ_FLAG_HIDDEN))
		{
			dGCCurrentStatus = nGCStatusCapturing;
			gGCCurrentCamera = gobj;

			gobj->proc_display(gobj);

			dGCCurrentStatus = nGCStatusSystem;
		}
		gobj = gobj->dl_link_next;
	}
}

// 0x8000A40C
GObj* gcRunGObj(GObj *gobj)
{
	GObj *next_gobj;

	dGCCurrentStatus = nGCStatusRunning;
	gGCCurrentCommon = gobj;

#if defined(PORT) && defined(SSB64_NETMENU)
	syNetplayResimReplayHangDiagNoteGcRunGObj(gobj);
#endif

	gobj->func_run(gobj);

	next_gobj = gobj->link_next;

	gGCCurrentCommon = NULL;
	dGCCurrentStatus = nGCStatusSystem;

	switch (sGCRunStatus)
	{
	case nGCRunStatusDefault:
		break;

	case nGCRunStatusEject:
		sGCRunStatus = nGCRunStatusDefault;
		gcEjectGObj(gobj);
		break;

	default:
		sGCRunStatus = nGCRunStatusDefault;
		break;
	}
	return next_gobj;
}

// 0x8000A49C
GObjProcess* gcRunGObjProcess(GObjProcess *gobjproc)
{
	GObjProcess *next_gobjproc;

	dGCCurrentStatus = nGCStatusProcessing;
	gGCCurrentCommon = gobjproc->parent_gobj;
	gGCCurrentProcess = gobjproc;

#if defined(PORT) && defined(SSB64_NETMENU)
	syNetplayResimReplayHangDiagNoteGcRunGObjProcessBegin(gobjproc);
#endif

	switch (gobjproc->kind)
	{
	case nGCProcessKindThread:
		osStartThread(&gobjproc->exec.gobjthread->thread);
#ifdef PORT
		/* If the GObj thread's coroutine ran to completion (entry function
		 * returned) without yielding via gcSleepCurrentGObjThread, no
		 * message was sent to gGCMesgQueue — skip the blocking recv. */
		if (gobjproc->exec.gobjthread->thread.state == OS_STATE_STOPPED)
		{
			break;
		}
#endif
#if defined(PORT) && defined(SSB64_NETMENU)
		syNetplayResimReplayHangDiagNoteGcRunGObjProcessThreadRecvWait(gobjproc, gGCMesgQueue.validCount);
#endif
		osRecvMesg(&gGCMesgQueue, NULL, OS_MESG_BLOCK);
		break;

	case nGCProcessKindFunc:
#if defined(PORT) && defined(SSB64_NETMENU)
		if (gobjproc->parent_gobj == NULL)
		{
			GObjProcess *free_gobjproc;
			u32 free_guard;
			sb32 is_on_free_list;

			next_gobjproc = gobjproc->priority_next;
#if defined(SSB64_NETMENU)
			port_log("SSB64: gcRunGObjProcess ZOMBIE_FUNC_PROC proc=%p priority=%u kind=%u - unlinking\n",
			         (void *)gobjproc,
			         (unsigned int)gobjproc->priority,
			         (unsigned int)gobjproc->kind);
#endif
			is_on_free_list = FALSE;
			for (free_gobjproc = sGCProcessHead, free_guard = 0U;
			     (free_gobjproc != NULL) && (free_guard < 4096U);
			     free_gobjproc = free_gobjproc->link_next, free_guard++)
			{
				if (free_gobjproc == gobjproc)
				{
					is_on_free_list = TRUE;
					break;
				}
			}
			func_80007784(gobjproc);
			if (is_on_free_list == FALSE)
			{
				if (gobjproc->link_prev != NULL)
				{
					gobjproc->link_prev->link_next = gobjproc->link_next;
				}
				if (gobjproc->link_next != NULL)
				{
					gobjproc->link_next->link_prev = gobjproc->link_prev;
				}
				gobjproc->link_prev = NULL;
				gobjproc->link_next = NULL;
				if ((free_gobjproc != NULL) && (free_guard >= 4096U))
				{
					port_log("SSB64: gcRunGObjProcess ZOMBIE_FUNC_PROC proc=%p free-list walk capped - not pushing\n",
					         (void *)gobjproc);
				}
				else
				{
					gcSetGObjProcessPrevAlloc(gobjproc);
				}
			}
			else
			{
				port_log("SSB64: gcRunGObjProcess ZOMBIE_FUNC_PROC proc=%p already on free list - not pushing\n",
				         (void *)gobjproc);
			}
#if defined(PORT) && defined(SSB64_NETMENU)
			syNetplayResimReplayHangDiagNoteGcRunGObjProcessEnd();
#endif
			gGCCurrentCommon = NULL;
			gGCCurrentProcess = NULL;
			dGCCurrentStatus = nGCStatusSystem;
			sGCRunStatus = nGCRunStatusDefault;
			return next_gobjproc;
		}
#endif
		gobjproc->exec.func(gobjproc->parent_gobj);
		break;
	}
#if defined(PORT) && defined(SSB64_NETMENU)
	syNetplayResimReplayHangDiagNoteGcRunGObjProcessEnd();
#endif
	next_gobjproc = gobjproc->priority_next;

	gGCCurrentCommon = NULL;
	gGCCurrentProcess = NULL;
	dGCCurrentStatus = nGCStatusSystem;

	switch (sGCRunStatus)
	{
	case nGCRunStatusEject:
		sGCRunStatus = nGCRunStatusDefault;

		while ((next_gobjproc != NULL) && (next_gobjproc->parent_gobj == gobjproc->parent_gobj))
		{
			next_gobjproc = next_gobjproc->priority_next;
		}
		gcEjectGObj(gobjproc->parent_gobj);
		break;

	case nGCRunStatusEnd:
		sGCRunStatus = nGCRunStatusDefault;
		gcEndGObjProcess(gobjproc);
		break;

	case nGCRunStatusDefault:
		break;

	default:
		sGCRunStatus = nGCRunStatusDefault;
		break;
	}
	return next_gobjproc;
}

// 0x8000A5E4
void gcRunAll(void)
{
	s32 i;
	GObj *gobj;
	GObjProcess *gobjproc;

	sGCRunStatus = nGCRunStatusDefault;
	gGCCurrentCommon = NULL;
	gGCCurrentProcess = NULL;

	for (i = 0; i < ARRAY_COUNT(gGCCommonLinks); i++)
	{
		gobj = gGCCommonLinks[i];

		while (gobj != NULL)
		{
			if (!(gobj->flags & GOBJ_FLAG_NORUN) && (gobj->func_run != NULL))
			{
				gobj = gcRunGObj(gobj);
			}
			else gobj = gobj->link_next;
		}
	}
	for (i = ARRAY_COUNT(sGCProcessQueue) - 1; i >= 0; i--)
	{
		gobjproc = sGCProcessQueue[i];

		while (gobjproc != NULL)
		{
			if (gobjproc->is_paused == FALSE)
			{
				gobjproc = gcRunGObjProcess(gobjproc);
			}
			else gobjproc = gobjproc->priority_next;
		}
	}
}

// 0x8000A6E0
void gcSetupObjman(GCSetup *setup)
{
	s32 i;

	sGCThreadStackSize = setup->gobjthreadstack_size;
	sUnkUnusedSetup = setup->unk_gcsetup_0x14;

	if (setup->gobjthreads_num != 0)
	{
		GObjThread *current_gobjthread;
		sGCThreadHead = current_gobjthread = setup->gobjthreads;

		for (i = 0; i < setup->gobjthreads_num - 1; i++)
		{
			GObjThread *next_gobjthread = current_gobjthread + 1;

			current_gobjthread->next = next_gobjthread;
			current_gobjthread = next_gobjthread;
		}
		current_gobjthread->next = NULL;
	}
	else sGCThreadHead = NULL;

	if ((setup->gobjthreadstacks_num != 0) && (setup->gobjthreadstack_size != 0))
	{
		GObjStack *current_gobjstack;

		sGCThreadStackHead = syTaskmanMalloc(sizeof(GObjThreadStack), 0x4);
		sGCThreadStackHead->next = NULL;
		sGCThreadStackHead->size = sGCThreadStackSize;
		sGCThreadStackHead->stack = current_gobjstack = setup->gobjthreadstacks;

		for (i = 0; i < setup->gobjthreadstacks_num - 1; i++)
		{
			current_gobjstack->next = (GObjStack*) ((uintptr_t)current_gobjstack + sGCThreadStackSize + offsetof(GObjStack, stack));
			current_gobjstack->stack_size = sGCThreadStackSize;
			current_gobjstack = (GObjStack*) ((uintptr_t)current_gobjstack + sGCThreadStackSize + offsetof(GObjStack, stack));
		}
		current_gobjstack->stack_size = sGCThreadStackSize;
		current_gobjstack->next = NULL;
	}
	else sGCThreadStackHead = NULL;

	if (setup->gobjprocs_num != 0)
	{
		GObjProcess *current_gobjproc;
		sGCProcessHead = current_gobjproc = setup->gobjprocs;

		for (i = 0; i < setup->gobjprocs_num - 1; i++)
		{
			GObjProcess *next_gobjproc = current_gobjproc + 1;

			current_gobjproc->link_next = next_gobjproc;
			current_gobjproc = next_gobjproc;
		}
		current_gobjproc->link_next = NULL;
	}
	else sGCProcessHead = NULL;

	for (i = 0; i < ARRAY_COUNT(sGCProcessQueue); i++)
	{
		sGCProcessQueue[i] = NULL;
	}
	if (setup->gobjs_num != 0)
	{
		GObj *current_gobj;
		sGCCommonHead = current_gobj = setup->gobjs;

		for (i = 0; i < setup->gobjs_num - 1; i++)
		{
			current_gobj->link_next = (GObj*) ((uintptr_t)current_gobj + setup->gobj_size);
			current_gobj = current_gobj->link_next;
		}
		current_gobj->link_next = NULL;
	}
	else sGCCommonHead = NULL;

	sGCCommonSize = setup->gobj_size;
	sGCCommonsMaxNum = -1;
	sGCDrawFuncEject = setup->func_eject;

	if (setup->xobjs_num != 0)
	{
		XObj *current_xobj;
		sGCMatrixHead = current_xobj = setup->xobjs;

		for (i = 0; i < setup->xobjs_num - 1; i++)
		{
			XObj *next_xobj = current_xobj + 1;
			current_xobj->next = next_xobj;
			current_xobj = next_xobj;
		}
		current_xobj->next = NULL;
	}
	else sGCMatrixHead = NULL;

	if (setup->aobjs_num != 0)
	{
		AObj *current_aobj;
		sGCAnimHead = current_aobj = setup->aobjs;

		for (i = 0; i < setup->aobjs_num - 1; i++)
		{
			AObj *next_aobj = current_aobj + 1;
			current_aobj->next = next_aobj;
			current_aobj = next_aobj;
		}
		current_aobj->next = NULL;
	}
	else sGCAnimHead = NULL;

	if (setup->mobjs_num != 0)
	{
		MObj *current_mobj;
		sGCMaterialHead = current_mobj = setup->mobjs;

		for (i = 0; i < setup->mobjs_num - 1; i++)
		{
			MObj *mobj_next = current_mobj + 1;
			current_mobj->next = mobj_next;
			current_mobj = mobj_next;
		}
		current_mobj->next = NULL;
	}
	else sGCMaterialHead = NULL;

	if (setup->dobjs_num != 0)
	{
		DObj *current_dobj;
		sGCDrawHead = current_dobj = setup->dobjs;

		for (i = 0; i < setup->dobjs_num - 1; i++)
		{
			current_dobj->alloc_free = (DObj*) ((uintptr_t)current_dobj + setup->dobj_size);
			current_dobj = current_dobj->alloc_free;
		}
		current_dobj->alloc_free = NULL;
	}
	else sGCDrawHead = NULL;

	sGCDrawSize = setup->dobj_size;

	if (setup->sobjs_num != 0)
	{
		SObj *current_sobj;
		sGCSpriteHead = current_sobj = setup->sobjs;

		for (i = 0; i < setup->sobjs_num - 1; i++)
		{
			current_sobj->alloc_free = (SObj*) ((uintptr_t)current_sobj + setup->sobj_size);
			current_sobj = current_sobj->alloc_free;
		}
		current_sobj->alloc_free = NULL;
	}
	else sGCSpriteHead = NULL;

	sGCSpriteSize = setup->sobj_size;

	if (setup->cobjs_num != 0)
	{
		CObj *current_cobj;
		sGCCameraHead = current_cobj = setup->cameras;

		for (i = 0; i < setup->cobjs_num - 1; i++)
		{
			current_cobj->next = (CObj*) ((uintptr_t)current_cobj + setup->cobj_size);
			current_cobj = current_cobj->next;
		}
		current_cobj->next = NULL;
	}
	else sGCCameraHead = NULL;

	sGCCameraSize = setup->cobj_size;

	for (i = 0; i < (ARRAY_COUNT(sGCCommonLinks) + ARRAY_COUNT(gGCCommonLinks)) / 2; i++)
	{
		gGCCommonLinks[i] = sGCCommonLinks[i] = NULL;
	}
	for (i = 0; i < (ARRAY_COUNT(sGCCommonDLLinks) + ARRAY_COUNT(gGCCommonDLLinks)) / 2; i++)
	{
		gGCCommonDLLinks[i] = sGCCommonDLLinks[i] = NULL;
	}
	gcInitDLs();
	osCreateMesgQueue(&gGCMesgQueue, sGCMesgs, ARRAY_COUNT(sGCMesgs));

	sGCStacksActiveNum 	=
	sGCThreadsActive 	=
	sGCProcessesActive 	=
	sGCCommonsActiveNum =
	sGCMatrixesActiveNum=
	sGCAnimsActiveNum 	=
	sGCDrawsActiveNum 	=	
	sGCSpritesActiveNum =
	sGCCamerasActiveNum = 0;

	sGCProcessFunction = NULL;

	gcSetCameraMatrixMode(0);

	dGCCurrentStatus = nGCStatusSystem;
}

#if defined(PORT) && defined(SSB64_NETMENU)
#include <sys/objman_gcport.h>
#include <sys/netinput.h>
#include <sys/netpeer.h>
#include <ft/fighter.h>
#include <ft/ftdef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GCPORT_FNV_SEED 2166136261U
#define GCPORT_TRAVERSAL_LINK_LIMIT 2048U

typedef struct GCPortGObjEjectRecord
{
	u32 sim_tick;
	u32 gobj_id;
	u8 link_id;
	u8 obj_kind;
} GCPortGObjEjectRecord;

static GCPortGObjEjectRecord sGCPortGObjEjectRing[64];
static u32 sGCPortGObjEjectRingCap = GCPORT_GOBJ_EJECT_RING_DEFAULT;
static u32 sGCPortGObjEjectRingWrite;
static sb32 sGCPortGObjEjectTraceCache = -999;

static u32 gcPortFnvAccumulateU32(u32 hash, u32 value)
{
	hash ^= value;
	hash *= 16777619U;
	return hash;
}

static void gcPortSnprintGcRunAllTraversalCycleDiagAppend(char *buf, size_t bufsize, const char *fmt, ...)
{
	size_t len;
	va_list args;

	if ((buf == NULL) || (bufsize == 0U))
	{
		return;
	}
	len = strlen(buf);
	if (len >= (bufsize - 1U))
	{
		buf[bufsize - 1U] = '\0';
		return;
	}
	va_start(args, fmt);
	vsnprintf(buf + len, bufsize - len, fmt, args);
	va_end(args);
	buf[bufsize - 1U] = '\0';
}

static sb32 gcPortGObjEjectTraceEnabled(void)
{
	const char *e;

	if (sGCPortGObjEjectTraceCache != -999)
	{
		return (sGCPortGObjEjectTraceCache != 0) ? TRUE : FALSE;
	}
	e = getenv("SSB64_NETPLAY_GOBJ_EJECT_TRACE");
	sGCPortGObjEjectTraceCache = ((e != NULL) && (e[0] != '\0') && (strtol(e, NULL, 10) != 0L)) ? 1 : 0;
	if (sGCPortGObjEjectTraceCache != 0)
	{
		const char *ring_env = getenv("SSB64_NETPLAY_GOBJ_EJECT_RING");
		long ring_n;

		ring_n = ((ring_env != NULL) && (ring_env[0] != '\0')) ? strtol(ring_env, NULL, 10) : (long)GCPORT_GOBJ_EJECT_RING_DEFAULT;
		if (ring_n < 4L)
		{
			ring_n = 4L;
		}
		if (ring_n > (long)ARRAY_COUNT(sGCPortGObjEjectRing))
		{
			ring_n = (long)ARRAY_COUNT(sGCPortGObjEjectRing);
		}
		sGCPortGObjEjectRingCap = (u32)ring_n;
	}
	return (sGCPortGObjEjectTraceCache != 0) ? TRUE : FALSE;
}

void gcPortRecordGObjEject(const GObj *gobj)
{
	GCPortGObjEjectRecord *rec;

	if ((gobj == NULL) || (gcPortGObjEjectTraceEnabled() == FALSE) || (syNetPeerIsVSSessionActive() == FALSE))
	{
		return;
	}
	rec = &sGCPortGObjEjectRing[sGCPortGObjEjectRingWrite % sGCPortGObjEjectRingCap];
	rec->sim_tick = syNetInputGetTick();
	rec->gobj_id = gobj->id;
	rec->link_id = gobj->link_id;
	rec->obj_kind = gobj->obj_kind;
	sGCPortGObjEjectRingWrite++;
}

void gcPortDumpGObjEjectRing(const char *tag, u32 load_tick)
{
	u32 i;
	u32 count;
	u32 start;

	if (gcPortGObjEjectTraceEnabled() == FALSE)
	{
		return;
	}
	count = sGCPortGObjEjectRingWrite;
	if (count > sGCPortGObjEjectRingCap)
	{
		count = sGCPortGObjEjectRingCap;
	}
	start = (sGCPortGObjEjectRingWrite >= count) ? (sGCPortGObjEjectRingWrite - count) : 0U;
	port_log("SSB64: gcEjectGObj RING_DUMP tag=%s load_tick=%u entries=%u cap=%u\n", (tag != NULL) ? tag : "?",
		 load_tick, count, sGCPortGObjEjectRingCap);
	for (i = 0; i < count; i++)
	{
		const GCPortGObjEjectRecord *rec =
		    &sGCPortGObjEjectRing[(start + i) % sGCPortGObjEjectRingCap];

		port_log("SSB64: gcEjectGObj RING[%u] sim_tick=%u id=%u link_id=%u kind=%u\n", i, rec->sim_tick,
			 rec->gobj_id, (unsigned int)rec->link_id, (unsigned int)rec->obj_kind);
	}
}

void gcPortGcRunAllTraversalFingerprintEx(u32 *gch, u32 *ngobj, u32 *ngobj_run, u32 *nproc_run)
{
	s32 i;
	GObj *gobj;
	GObjProcess *gobjproc;
	u32 hash = GCPORT_FNV_SEED;
	u32 gobj_count = 0U;
	u32 gobj_run_count = 0U;
	u32 proc_run_count = 0U;

	for (i = 0; i < (s32)ARRAY_COUNT(gGCCommonLinks); i++)
	{
		u32 guard = 0U;

		for (gobj = gGCCommonLinks[i]; (gobj != NULL) && (guard < GCPORT_TRAVERSAL_LINK_LIMIT);
		     gobj = gobj->link_next, guard++)
		{
			u32 fold = GCPORT_FNV_SEED;
			s32 player = -1;

			gobj_count++;
			fold = gcPortFnvAccumulateU32(fold, (u32)i);
			fold = gcPortFnvAccumulateU32(fold, gobj->id);
			fold = gcPortFnvAccumulateU32(fold, (u32)gobj->obj_kind);
			fold = gcPortFnvAccumulateU32(fold, (u32)gobj->link_id);
			fold = gcPortFnvAccumulateU32(fold, (u32)(gobj->flags & GOBJ_FLAG_NORUN));
			if ((i == nGCCommonLinkIDFighter) && (gobj->obj != NULL))
			{
				FTStruct *fp = ftGetStruct(gobj);

				if (fp != NULL)
				{
					player = fp->player;
					fold = gcPortFnvAccumulateU32(fold, (u32)fp->player);
					fold = gcPortFnvAccumulateU32(fold, (u32)fp->fkind);
				}
			}
			hash ^= fold;
			hash = gcPortFnvAccumulateU32(hash, (u32)((player >= 0) ? (u32)player : 0xFFU));
			if (!(gobj->flags & GOBJ_FLAG_NORUN) && (gobj->func_run != NULL))
			{
				gobj_run_count++;
			}
		}
	}
	for (i = (s32)ARRAY_COUNT(sGCProcessQueue) - 1; i >= 0; i--)
	{
		u32 guard = 0U;

		for (gobjproc = sGCProcessQueue[i]; (gobjproc != NULL) && (guard < GCPORT_TRAVERSAL_LINK_LIMIT);
		     gobjproc = gobjproc->priority_next, guard++)
		{
			if (gobjproc->is_paused == FALSE)
			{
				proc_run_count++;
			}
		}
	}
	if (gch != NULL)
	{
		*gch = hash;
	}
	if (ngobj != NULL)
	{
		*ngobj = gobj_count;
	}
	if (ngobj_run != NULL)
	{
		*ngobj_run = gobj_run_count;
	}
	if (nproc_run != NULL)
	{
		*nproc_run = proc_run_count;
	}
}

void gcPortSnprintGcRunAllTraversalCycleDiag(char *buf, size_t bufsize)
{
	s32 i;

	if ((buf == NULL) || (bufsize == 0U))
	{
		return;
	}
	buf[0] = '\0';
	for (i = 0; i < (s32)ARRAY_COUNT(gGCCommonLinks); i++)
	{
		GObj *slow = gGCCommonLinks[i];
		GObj *fast = gGCCommonLinks[i];
		u32 steps = 0U;

		while ((fast != NULL) && (fast->link_next != NULL) && (steps < GCPORT_TRAVERSAL_LINK_LIMIT))
		{
			slow = slow->link_next;
			fast = fast->link_next->link_next;
			steps++;
			if (slow == fast)
			{
				gcPortSnprintGcRunAllTraversalCycleDiagAppend(
				    buf, bufsize, "common_cycle link=%d head=%p:g%u meet=%p:g%u steps=%u", (int)i,
				    (void *)gGCCommonLinks[i], (gGCCommonLinks[i] != NULL) ? gGCCommonLinks[i]->id : 0U,
				    (void *)slow, (slow != NULL) ? slow->id : 0U, steps);
				return;
			}
		}
		if (steps >= GCPORT_TRAVERSAL_LINK_LIMIT)
		{
			gcPortSnprintGcRunAllTraversalCycleDiagAppend(
			    buf, bufsize, "common_limit link=%d head=%p:g%u limit=%u", (int)i, (void *)gGCCommonLinks[i],
			    (gGCCommonLinks[i] != NULL) ? gGCCommonLinks[i]->id : 0U, GCPORT_TRAVERSAL_LINK_LIMIT);
			return;
		}
	}
	for (i = (s32)ARRAY_COUNT(sGCProcessQueue) - 1; i >= 0; i--)
	{
		GObjProcess *slow = sGCProcessQueue[i];
		GObjProcess *fast = sGCProcessQueue[i];
		u32 steps = 0U;

		while ((fast != NULL) && (fast->priority_next != NULL) && (steps < GCPORT_TRAVERSAL_LINK_LIMIT))
		{
			slow = slow->priority_next;
			fast = fast->priority_next->priority_next;
			steps++;
			if (slow == fast)
			{
				GObj *head_parent = (sGCProcessQueue[i] != NULL) ? sGCProcessQueue[i]->parent_gobj : NULL;
				GObj *meet_parent = (slow != NULL) ? slow->parent_gobj : NULL;

				gcPortSnprintGcRunAllTraversalCycleDiagAppend(
				    buf, bufsize, "proc_cycle pri=%d head=%p:p%u meet=%p:p%u steps=%u", (int)i,
				    (void *)sGCProcessQueue[i], (head_parent != NULL) ? head_parent->id : 0U, (void *)slow,
				    (meet_parent != NULL) ? meet_parent->id : 0U, steps);
				return;
			}
		}
		if (steps >= GCPORT_TRAVERSAL_LINK_LIMIT)
		{
			GObj *head_parent = (sGCProcessQueue[i] != NULL) ? sGCProcessQueue[i]->parent_gobj : NULL;

			gcPortSnprintGcRunAllTraversalCycleDiagAppend(
			    buf, bufsize, "proc_limit pri=%d head=%p:p%u limit=%u", (int)i, (void *)sGCProcessQueue[i],
			    (head_parent != NULL) ? head_parent->id : 0U, GCPORT_TRAVERSAL_LINK_LIMIT);
			return;
		}
	}
	gcPortSnprintGcRunAllTraversalCycleDiagAppend(buf, bufsize, "cycle=none");
}

u32 gcPortHashGcRunAllTraversalFingerprint(void)
{
	u32 gch;

	gcPortGcRunAllTraversalFingerprintEx(&gch, NULL, NULL, NULL);
	return gch;
}

void gcPortSnprintGcRunAllTraversalHeadPairs(char *buf, size_t bufsize, int max_pairs)
{
	s32 i;
	GObj *gobj;
	int pairs = 0;
	size_t pos = 0U;

	if ((buf == NULL) || (bufsize == 0U))
	{
		return;
	}
	buf[0] = '\0';
	if (max_pairs <= 0)
	{
		return;
	}
	for (i = 0; i < (s32)ARRAY_COUNT(gGCCommonLinks); i++)
	{
		for (gobj = gGCCommonLinks[i]; gobj != NULL; gobj = gobj->link_next)
		{
			int n;

			if (pairs >= max_pairs)
			{
				return;
			}
			n = snprintf(buf + pos, (pos < bufsize) ? (bufsize - pos) : 0U, "%sL%d:g%u", (pairs > 0) ? "," : "",
				     (int)i, gobj->id);
			if (n <= 0)
			{
				return;
			}
			pos += (size_t)n;
			if (pos >= bufsize)
			{
				buf[bufsize - 1U] = '\0';
				return;
			}
			pairs++;
		}
	}
}
#endif /* PORT && SSB64_NETMENU */
