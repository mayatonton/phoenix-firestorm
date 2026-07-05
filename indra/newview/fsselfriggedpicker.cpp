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
#include "llvkloader.h"
#include "llviewerjointattachment.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "pipeline.h"

namespace
{
    // Recursive walk through an avatar's attachment tree, looking for the
    // child whose LocalID matches what the GPU object-ID buffer reported at
    // the mouse pixel. The caller scopes this to exactly one avatar so LocalID
    // values from unrelated scene objects cannot resolve here.
    LLViewerObject* findAttachmentByLocalID(LLViewerObject* obj, U32 target_id)
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
            if (LLViewerObject* hit = findAttachmentByLocalID(child, target_id))
            {
                return hit;
            }
        }
        return nullptr;
    }

    LLViewerObject* findAttachmentOnAvatarByLocalID(LLVOAvatar* avatar, U32 target_id)
    {
        if (!avatar || target_id == 0)
        {
            return nullptr;
        }

        for (const auto& it : avatar->mAttachmentPoints)
        {
            LLViewerJointAttachment* att = it.second;
            if (!att || att->getIsHUDAttachment())
            {
                continue;
            }
            for (LLViewerObject* root : att->mAttachedObjects)
            {
                if (LLViewerObject* hit = findAttachmentByLocalID(root, target_id))
                {
                    return hit;
                }
            }
        }
        return nullptr;
    }

    U32 readObjectIDBufferLocalID(S32 mouse_x, S32 mouse_y, bool& out_read)
    {
        out_read = false;

        // Coordinate conversion. mObjectIDBuffer is allocated at WorldViewRectRaw
        // dimensions (pipeline.cpp resizeScreenTexture), so its (0,0) corresponds
        // to the world view rect's bottom-left in raw pixels. LLCoordGL mouse
        // coords are window-relative scaled (logical) pixels.
        //   1) scaled -> raw via DisplayScale (mWindowRectRaw / mWindowRectScaled)
        //   2) subtract WorldViewRectRaw's mLeft/mBottom origin -> buffer-local
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
            return 0;
        }

        GLubyte rgba[4] = {0, 0, 0, 0};
        if (LLVKLoader::shouldUseVulkanRender())
        {
            if (!gPipeline.mObjectIDBuffer.hasVkImage(0) ||
                !LLVKLoader::readbackColorImageRegionVk(
                    gPipeline.mObjectIDBuffer.getVkImage(0),
                    gPipeline.mObjectIDBuffer.getVkTexLayout(0),
                    mx_buf, my_buf, 1, 1, 4, rgba))
            {
                return 0;
            }
        }
        else
        {
            gPipeline.mObjectIDBuffer.bindTarget();
            glReadPixels(mx_buf, my_buf, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
            gPipeline.mObjectIDBuffer.flush();
        }
        out_read = true;
        return ((U32)rgba[0])
             | ((U32)rgba[1] << 8)
             | ((U32)rgba[2] << 16)
             | ((U32)rgba[3] << 24);
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
    static LLCachedControl<bool> armed_mode(gSavedSettings, "FSSelfRiggedPickerArmedMode", true);
    if (!gpu_enable || !gPipeline.mObjectIDBuffer.isComplete())
    {
        return nullptr;
    }
    if (armed_mode && !gPipeline.isSelfRiggedObjectIDBufferReady())
    {
        return nullptr;
    }

    bool read = false;
    U32 local_id = readObjectIDBufferLocalID(mouse_x, mouse_y, read);
    if (!read)
    {
        return nullptr;
    }

    // GPU is the source of truth.
    out_gpu_authoritative = true;

    if (local_id != 0)
    {
        if (LLViewerObject* hit = findAttachmentOnAvatarByLocalID(gAgentAvatarp.get(), local_id))
        {
            return hit;
        }
        // id != 0 but no matching attachment: race (attachment removed
        // mid-frame, or stale value). Authoritative empty so caller can
        // force-to-body.
    }
    // id == 0: GPU says no self rigged attachment at this pixel — the truth
    // of the visible scene. Authoritative empty.
    return nullptr;
}

LLViewerObject* FSSelfRiggedPicker::findClosestAttachmentForAvatar(S32 mouse_x, S32 mouse_y,
                                                                   LLVOAvatar* avatar,
                                                                   bool& out_gpu_authoritative)
{
    out_gpu_authoritative = false;
    if (!avatar || avatar->isDead())
    {
        return nullptr;
    }
    if (isAgentAvatarValid() && avatar == gAgentAvatarp.get())
    {
        return nullptr;
    }

    static LLCachedControl<bool> enable(gSavedSettings, "FSOtherRiggedPickerEnable", false);
    static LLCachedControl<bool> gpu_enable(gSavedSettings, "FSOtherRiggedPickerGPU", true);
    if (!enable || !gpu_enable || !gPipeline.mObjectIDBuffer.isComplete())
    {
        return nullptr;
    }
    if (!gPipeline.isOtherRiggedObjectIDBufferReady(avatar->getID()))
    {
        return nullptr;
    }

    bool read = false;
    U32 local_id = readObjectIDBufferLocalID(mouse_x, mouse_y, read);
    if (!read)
    {
        return nullptr;
    }

    out_gpu_authoritative = true;
    if (local_id != 0)
    {
        return findAttachmentOnAvatarByLocalID(avatar, local_id);
    }
    return nullptr;
}
