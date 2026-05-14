/**
 * @file fsselfriggedpicker.cpp
 * @brief GPU picker for the agent's own rigged attachments.
 *
 *        Reads the byte quad at the mouse pixel from LLPipeline::mObjectIDBuffer
 *        (re-drawn each frame with each self rigged attachment's LocalID packed
 *        into RGBA8) and resolves it back to the owning LLViewerObject. The
 *        buffer is authoritative: id != 0 returns the matching attachment;
 *        id == 0 (or no match) returns null with out_gpu_authoritative=true so
 *        the caller can force-to-body for rigged self picks.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * AYAstorm Viewer Source Code
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "fsselfriggedpicker.h"

#include "llrendertarget.h"
#include "llviewercontrol.h"
#include "llviewerjointattachment.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "pipeline.h"

namespace
{
    // Recursive walk through gAgentAvatarp's attachment tree, looking for the
    // child whose LocalID matches what the GPU object-ID buffer reported at
    // the mouse pixel. Scoped to self attachments only — the upstream picker
    // handles the rest of the scene, and this also avoids LocalID collisions
    // with non-self objects.
    LLViewerObject* findSelfAttachmentByLocalID(LLViewerObject* obj, U32 target_id)
    {
        if (!obj)
        {
            return nullptr;
        }
        if (obj->getLocalID() == target_id)
        {
            return obj;
        }
        for (LLViewerObject* child : obj->getChildren())
        {
            if (LLViewerObject* hit = findSelfAttachmentByLocalID(child, target_id))
            {
                return hit;
            }
        }
        return nullptr;
    }
}

LLViewerObject* FSSelfRiggedPicker::findClosestAttachment(S32 mouse_x, S32 mouse_y,
                                                          bool& out_gpu_authoritative)
{
    out_gpu_authoritative = false;
    if (!isAgentAvatarValid())
    {
        return nullptr;
    }

    static LLCachedControl<bool> gpu_enable(gSavedSettings, "FSSelfRiggedPickerGPU", false);
    static LLCachedControl<bool> trace(gSavedSettings, "FSSelfRiggedPickerTrace", false);
    if (!gpu_enable || !gPipeline.mObjectIDBuffer.isComplete())
    {
        if (trace)
        {
            LL_INFOS("FSSelfRiggedPicker")
                << "readback skipped gpu_enable=" << (bool)gpu_enable
                << " buffer_complete=" << gPipeline.mObjectIDBuffer.isComplete()
                << LL_ENDL;
        }
        return nullptr;
    }

    // Coordinate conversion. mObjectIDBuffer is allocated at WorldViewRectRaw
    // dimensions (pipeline.cpp resizeScreenTexture), so its (0,0) corresponds
    // to the world view rect's bottom-left in raw pixels. LLCoordGL mouse
    // coords are window-relative scaled (logical) pixels.
    //   1) scaled → raw via DisplayScale (mWindowRectRaw / mWindowRectScaled)
    //   2) subtract WorldViewRectRaw's mLeft/mBottom origin → buffer-local
    // Missing either step (raw mismatch on HiDPI / UI-chrome offset) reads
    // the wrong pixel — typically id=0.
    const LLRect wv_raw = gViewerWindow->getWorldViewRectRaw();
    const F32 sx = (F32)gViewerWindow->getWindowWidthRaw()  / (F32)gViewerWindow->getWindowWidthScaled();
    const F32 sy = (F32)gViewerWindow->getWindowHeightRaw() / (F32)gViewerWindow->getWindowHeightScaled();
    const S32 mx_win_raw = (S32)llround((F32)mouse_x * sx);
    const S32 my_win_raw = (S32)llround((F32)mouse_y * sy);
    const S32 mx_buf = mx_win_raw - wv_raw.mLeft;
    const S32 my_buf = my_win_raw - wv_raw.mBottom;

    if (mx_buf < 0 || my_buf < 0 ||
        mx_buf >= (S32)gPipeline.mObjectIDBuffer.getWidth() ||
        my_buf >= (S32)gPipeline.mObjectIDBuffer.getHeight())
    {
        if (trace)
        {
            LL_INFOS("FSSelfRiggedPicker")
                << "readback skipped out_of_bounds"
                << " mouse=" << mouse_x << "," << mouse_y
                << " buffer_xy=" << mx_buf << "," << my_buf
                << " buffer=" << gPipeline.mObjectIDBuffer.getWidth()
                << "x" << gPipeline.mObjectIDBuffer.getHeight()
                << LL_ENDL;
        }
        return nullptr;
    }

    GLubyte rgba[4] = {0, 0, 0, 0};
    gPipeline.mObjectIDBuffer.bindTarget();
    glReadPixels(mx_buf, my_buf, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    gPipeline.mObjectIDBuffer.flush();
    U32 local_id = ((U32)rgba[0])
                 | ((U32)rgba[1] << 8)
                 | ((U32)rgba[2] << 16)
                 | ((U32)rgba[3] << 24);

    if (trace)
    {
        LL_INFOS("FSSelfRiggedPicker")
            << "readback"
            << " mouse=" << mouse_x << "," << mouse_y
            << " buffer_xy=" << mx_buf << "," << my_buf
            << " rgba=(" << (U32)rgba[0] << "," << (U32)rgba[1] << ","
            << (U32)rgba[2] << "," << (U32)rgba[3] << ")"
            << " local_id=" << local_id
            << LL_ENDL;
    }

    // GPU is the source of truth.
    out_gpu_authoritative = true;

    if (local_id != 0)
    {
        for (const auto& it : gAgentAvatarp->mAttachmentPoints)
        {
            LLViewerJointAttachment* att = it.second;
            if (!att || att->getIsHUDAttachment())
            {
                continue;
            }
            for (LLViewerObject* root : att->mAttachedObjects)
            {
                if (LLViewerObject* hit = findSelfAttachmentByLocalID(root, local_id))
                {
                    return hit;
                }
            }
        }
        // id != 0 but no matching attachment: race (attachment removed
        // mid-frame, or stale value). Authoritative empty so caller can
        // force-to-body.
    }
    // id == 0: GPU says no self rigged attachment at this pixel — the truth
    // of the visible scene. Authoritative empty.
    return nullptr;
}
