/**
 * @file llviewertexturelist.cpp
 * @brief Object for managing the list of images within a region
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include <sys/stat.h>

#include "llviewertexturelist.h"

#include <algorithm>
#include <set>
#include <vector>

#include "llagent.h"
#include "llface.h"
#include "llgl.h" // fot gathering stats from GL
#include "llimagegl.h"
#include "llimagebmp.h"
#include "llimagej2c.h"
#include "llimagetga.h"
#include "llimagejpeg.h"
#include "llimagepng.h"
#include "llimageworker.h"
#include "llmemory.h"

#include "llsdserialize.h"
#include "llsys.h"
#include "llfilesystem.h"
#include "llxmltree.h"
#include "message.h"

#include "lldrawpoolbump.h" // to init bumpmap images
#include "lltexturecache.h"
#include "lltexturefetch.h"
#include "llviewercontrol.h"
#include "llviewercamera.h"
#include "llviewerobject.h"
#include "llviewertexture.h"
#include "llvoavatar.h"
#include "llviewermedia.h"
#include "llviewernetwork.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "pipeline.h"
#include "llappviewer.h"
#include "llxuiparser.h"
#include "lltracerecording.h"
#include "llviewerdisplay.h"
#include "llviewerwindow.h"
#include "llprogressview.h"
#include "llstartup.h"

////////////////////////////////////////////////////////////////////////////

void (*LLViewerTextureList::sUUIDCallback)(void **, const LLUUID&) = NULL;

S32 LLViewerTextureList::sNumImages = 0;

// <FS:Ansariel> Fast cache stats
U32 LLViewerTextureList::sNumFastCacheReads = 0;

LLViewerTextureList gTextureList;

extern LLGLSLShader gCopyProgram;

namespace
{
enum AYAQueuedTextureBucket
{
    AYA_QUEUE_BUCKET_NEAR = 0,
    AYA_QUEUE_BUCKET_MID,
    AYA_QUEUE_BUCKET_FAR,
    AYA_QUEUE_BUCKET_OFFSCREEN,
    AYA_QUEUE_BUCKET_UNREFERENCED,
    AYA_QUEUE_BUCKET_PROTECTED,
    AYA_QUEUE_BUCKET_COUNT
};

S32 aya_texture_create_priority_score(LLViewerFetchedTexture* imagep, AYAQueuedTextureBucket* bucketp = nullptr);

bool aya_texture_management_enabled()
{
    static LLCachedControl<bool> texture_management_enabled(gSavedSettings, "AYALoadingTextureManagementEnabled", false);
    return texture_management_enabled() &&
        LLStartUp::getStartupState() >= STATE_STARTED;
}

bool aya_texture_protected_fttype(LLViewerFetchedTexture* imagep)
{
    if (!imagep)
    {
        return false;
    }

    const FTType ft_type = imagep->getFTType();
    return ft_type == FTT_LOCAL_FILE ||
        ft_type == FTT_MAP_TILE ||
        ft_type == FTT_SERVER_BAKE ||
        ft_type == FTT_HOST_BAKE;
}

bool aya_texture_protected_boost(LLViewerFetchedTexture* imagep)
{
    if (!imagep)
    {
        return false;
    }

    const S32 boost = imagep->getBoostLevel();
    return boost == LLGLTexture::BOOST_AVATAR_SELF ||
        boost == LLGLTexture::BOOST_AVATAR_BAKED ||
        boost == LLGLTexture::BOOST_AVATAR_BAKED_SELF ||
        boost >= LLGLTexture::BOOST_HIGH;
}

bool aya_texture_has_volume_ref(LLViewerFetchedTexture* imagep)
{
    if (!imagep)
    {
        return false;
    }

    for (U32 channel = 0; channel < LLRender::NUM_VOLUME_TEXTURE_CHANNELS; ++channel)
    {
        if (imagep->getNumVolumes(channel) > 0)
        {
            return true;
        }
    }

    return false;
}

bool aya_texture_has_protected_work(LLViewerFetchedTexture* imagep)
{
    return imagep &&
        (aya_texture_protected_fttype(imagep) ||
         aya_texture_protected_boost(imagep) ||
         imagep->hasCallbacks() ||
         imagep->needsToSaveRawImage() ||
         imagep->forSculpt());
}

bool aya_texture_has_strength_sweep_blocker(LLViewerFetchedTexture* imagep)
{
    return imagep &&
        (imagep->hasCallbacks() ||
         imagep->needsToSaveRawImage() ||
         imagep->forSculpt());
}

bool aya_texture_strength_sweep_protected_fttype(LLViewerFetchedTexture* imagep)
{
    if (!imagep)
    {
        return false;
    }

    const FTType ft_type = imagep->getFTType();
    return ft_type == FTT_LOCAL_FILE ||
        ft_type == FTT_MAP_TILE ||
        ft_type == FTT_SERVER_BAKE ||
        ft_type == FTT_HOST_BAKE;
}

bool aya_object_is_avatar_related(LLViewerObject* objp)
{
    return objp && (objp->isAvatar() || objp->getAvatar());
}

bool aya_object_is_protected_texture_owner(LLViewerObject* objp)
{
    if (!objp)
    {
        return false;
    }

    if (objp->isHUDAttachment())
    {
        return true;
    }

    LLVOAvatar* avatarp = objp->asAvatar();
    if (!avatarp)
    {
        avatarp = objp->getAvatar();
    }

    return avatarp && avatarp->isSelf();
}

LLVOAvatar* aya_object_avatar_owner(LLViewerObject* objp)
{
    if (!objp)
    {
        return nullptr;
    }

    LLVOAvatar* avatarp = objp->asAvatar();
    if (!avatarp)
    {
        avatarp = objp->getAvatar();
    }
    return avatarp;
}

bool aya_texture_has_self_or_hud_face_owner(LLViewerFetchedTexture* imagep)
{
    if (!imagep)
    {
        return false;
    }

    for (U32 channel = 0; channel < LLRender::NUM_TEXTURE_CHANNELS; ++channel)
    {
        const LLViewerTexture::ll_face_list_t* faces = imagep->getFaceList(channel);
        const U32 face_count = llmin((U32)imagep->getNumFaces(channel), (U32)(faces ? faces->size() : 0));
        if (!faces || face_count == 0)
        {
            continue;
        }

        for (U32 face_index = 0; face_index < face_count; ++face_index)
        {
            LLFace* facep = (*faces)[face_index];
            LLViewerObject* objp = facep ? facep->getViewerObject() : nullptr;
            if (aya_object_is_protected_texture_owner(objp))
            {
                return true;
            }
        }
    }

    return false;
}

bool aya_texture_has_unloaded_avatar_face_owner(
    LLViewerFetchedTexture* imagep,
    F32 near_distance_m,
    bool require_visible_or_near)
{
    if (!imagep)
    {
        return false;
    }

    const LLVector3 camera_origin = LLViewerCamera::getInstance()->getOrigin();
    for (U32 channel = 0; channel < LLRender::NUM_TEXTURE_CHANNELS; ++channel)
    {
        const LLViewerTexture::ll_face_list_t* faces = imagep->getFaceList(channel);
        const U32 face_count = llmin((U32)imagep->getNumFaces(channel), (U32)(faces ? faces->size() : 0));
        if (!faces || face_count == 0)
        {
            continue;
        }

        for (U32 face_index = 0; face_index < face_count; ++face_index)
        {
            LLFace* facep = (*faces)[face_index];
            LLViewerObject* objp = facep ? facep->getViewerObject() : nullptr;
            LLVOAvatar* avatarp = aya_object_avatar_owner(objp);
            if (!avatarp || avatarp->isSelf())
            {
                continue;
            }

            const F32 distance = facep ? (facep->getPositionAgent() - camera_origin).magVec() : F32_MAX;
            const bool visible_or_near = !require_visible_or_near || facep->mInFrustum || distance <= near_distance_m;
            if (visible_or_near &&
                (!avatarp->isFullyLoaded() || avatarp->getRezzedStatus() < 3))
            {
                return true;
            }
        }
    }

    return false;
}

bool aya_texture_is_avatar_callback_priority(LLViewerFetchedTexture* imagep)
{
    if (!imagep)
    {
        return false;
    }

    if (imagep->getBoostLevel() == LLGLTexture::BOOST_AVATAR_SELF ||
        imagep->getBoostLevel() == LLGLTexture::BOOST_AVATAR_BAKED_SELF ||
        aya_texture_has_self_or_hud_face_owner(imagep))
    {
        return true;
    }

    const bool avatar_baked_callback =
        imagep->hasCallbacks() &&
        (imagep->getFTType() == FTT_SERVER_BAKE ||
            imagep->getFTType() == FTT_HOST_BAKE ||
            imagep->getBoostLevel() == LLGLTexture::BOOST_AVATAR_BAKED);
    if (avatar_baked_callback)
    {
        return true;
    }

    static LLCachedControl<F32> near_distance_m(gSavedSettings, "AYATextureDistanceCapNearMeters", 10.f);
    const bool avatar_or_bake_texture =
        imagep->getFTType() == FTT_SERVER_BAKE ||
        imagep->getFTType() == FTT_HOST_BAKE ||
        imagep->getBoostLevel() == LLGLTexture::BOOST_AVATAR ||
        imagep->getBoostLevel() == LLGLTexture::BOOST_AVATAR_BAKED;
    return avatar_or_bake_texture &&
        aya_texture_has_unloaded_avatar_face_owner(imagep, near_distance_m(), true);
}

bool aya_texture_is_avatar_related_create_candidate(LLViewerFetchedTexture* imagep)
{
    if (!imagep)
    {
        return false;
    }

    const S32 boost = imagep->getBoostLevel();
    if (boost == LLGLTexture::BOOST_AVATAR ||
        boost == LLGLTexture::BOOST_AVATAR_BAKED ||
        boost == LLGLTexture::BOOST_AVATAR_SELF ||
        boost == LLGLTexture::BOOST_AVATAR_BAKED_SELF ||
        imagep->getFTType() == FTT_SERVER_BAKE ||
        imagep->getFTType() == FTT_HOST_BAKE)
    {
        return true;
    }

    for (U32 channel = 0; channel < LLRender::NUM_TEXTURE_CHANNELS; ++channel)
    {
        const LLViewerTexture::ll_face_list_t* faces = imagep->getFaceList(channel);
        const U32 face_count = llmin((U32)imagep->getNumFaces(channel), (U32)(faces ? faces->size() : 0));
        if (!faces || face_count == 0)
        {
            continue;
        }

        for (U32 face_index = 0; face_index < face_count; ++face_index)
        {
            LLFace* facep = (*faces)[face_index];
            LLViewerObject* objp = facep ? facep->getViewerObject() : nullptr;
            if (aya_object_is_avatar_related(objp))
            {
                return true;
            }
        }
    }

    return false;
}

U32 aya_shift_texture_cap(U32 cap_resolution, S32 shift, U32 floor_resolution)
{
    U32 shifted = llmax((U32)1, cap_resolution);
    for (S32 i = 0; i < shift; ++i)
    {
        shifted = llmax((U32)1, shifted / 2);
    }
    return (U32)llclamp((S32)shifted, (S32)floor_resolution, LLGLTexture::MAX_IMAGE_SIZE_DEFAULT);
}

S32 aya_texture_downscale_strength_level(F32 strength)
{
    static constexpr S32 AYA_TEXTURE_DOWNSCALE_MAX_LEVEL = 5;
    return llclamp((S32)llroundf(strength), 0, AYA_TEXTURE_DOWNSCALE_MAX_LEVEL);
}

U32 aya_apply_texture_downscale_strength(U32 cap_resolution, S32 strength_level)
{
    static constexpr S32 AYA_TEXTURE_DOWNSCALE_MAX_LEVEL = 5;
    static constexpr U32 AYA_TEXTURE_CAP_MIN_REQUEST_PX = 16;
    const S32 level = llclamp(strength_level, 0, AYA_TEXTURE_DOWNSCALE_MAX_LEVEL);
    U32 cap = (U32)llclamp((S32)cap_resolution, (S32)AYA_TEXTURE_CAP_MIN_REQUEST_PX, LLGLTexture::MAX_IMAGE_SIZE_DEFAULT);
    if (level <= 0)
    {
        return cap;
    }
    if (level >= AYA_TEXTURE_DOWNSCALE_MAX_LEVEL)
    {
        return AYA_TEXTURE_CAP_MIN_REQUEST_PX;
    }

    for (S32 i = 0; i < level; ++i)
    {
        cap = llmax(AYA_TEXTURE_CAP_MIN_REQUEST_PX, cap / 2);
    }
    return cap;
}

U32 aya_effective_texture_cap_resolution(LLViewerFetchedTexture* imagep, U32 requested_cap_resolution)
{
    static constexpr U32 AYA_TEXTURE_CAP_MIN_REQUEST_PX = 16;
    const U32 requested_cap = (U32)llclamp((S32)requested_cap_resolution, (S32)AYA_TEXTURE_CAP_MIN_REQUEST_PX, LLGLTexture::MAX_IMAGE_SIZE_DEFAULT);
    if (!imagep || imagep->getFullWidth() <= 0 || imagep->getFullHeight() <= 0)
    {
        return requested_cap;
    }

    const S32 max_discard = llclamp(imagep->getMaxDiscardLevel(), 0, MAX_DISCARD_LEVEL);
    S32 effective_discard = 0;
    for (S32 discard = 0; discard <= max_discard; ++discard)
    {
        const U32 width = llmax(1, imagep->getFullWidth() >> discard);
        const U32 height = llmax(1, imagep->getFullHeight() >> discard);
        effective_discard = discard;
        if (llmax(width, height) <= requested_cap)
        {
            break;
        }
    }

    const U32 effective_width = llmax(1, imagep->getFullWidth() >> effective_discard);
    const U32 effective_height = llmax(1, imagep->getFullHeight() >> effective_discard);
    return llmax(effective_width, effective_height);
}

enum AYAPendingCreateRelease
{
    AYA_PENDING_CREATE_KEEP = 0,
    AYA_PENDING_CREATE_RELEASE_INACTIVE,
    AYA_PENDING_CREATE_RELEASE_DISTANCE
};

enum AYAMemorySweepBucket
{
    AYA_MEMORY_SWEEP_KEEP = 0,
    AYA_MEMORY_SWEEP_UNREFERENCED,
    AYA_MEMORY_SWEEP_OFFSCREEN,
    AYA_MEMORY_SWEEP_FAR,
    AYA_MEMORY_SWEEP_CAP_CANDIDATE
};

bool aya_texture_can_release_pending_create(
    LLViewerFetchedTexture* imagep,
    F32 min_unreferenced_seconds,
    bool allow_distance_defer,
    AYAPendingCreateRelease* release_reasonp = nullptr,
    AYAQueuedTextureBucket* bucketp = nullptr)
{
    if (release_reasonp)
    {
        *release_reasonp = AYA_PENDING_CREATE_KEEP;
    }
    if (bucketp)
    {
        *bucketp = AYA_QUEUE_BUCKET_PROTECTED;
    }

    if (!imagep ||
        imagep->isDeleted() ||
        !imagep->isInImageList() ||
        !imagep->mCreatePending ||
        imagep->getType() != LLViewerTexture::LOD_TEXTURE ||
        imagep->getBoostLevel() != LLGLTexture::BOOST_NONE ||
        aya_texture_has_protected_work(imagep) ||
        imagep->isFetching() ||
        imagep->isInFastCacheList())
    {
        return false;
    }

    static LLCachedControl<F32> near_distance_m(gSavedSettings, "AYATextureDistanceCapNearMeters", 10.f);
    if (aya_texture_has_unloaded_avatar_face_owner(imagep, near_distance_m(), true))
    {
        return false;
    }

    const bool has_faces = imagep->getTotalNumFaces() > 0;
    const bool has_volume_refs = aya_texture_has_volume_ref(imagep);
    const bool old_enough =
        imagep->getLastReferencedTimer()->getElapsedTimeF32() >= min_unreferenced_seconds;

    if (!has_faces && !has_volume_refs && old_enough)
    {
        if (release_reasonp)
        {
            *release_reasonp = AYA_PENDING_CREATE_RELEASE_INACTIVE;
        }
        if (bucketp)
        {
            *bucketp = AYA_QUEUE_BUCKET_UNREFERENCED;
        }
        return true;
    }

    if (!imagep->hasGLTexture() ||
        imagep->getDiscardLevel() < 0)
    {
        return false;
    }

    if (!allow_distance_defer)
    {
        return false;
    }

    AYAQueuedTextureBucket bucket = AYA_QUEUE_BUCKET_PROTECTED;
    aya_texture_create_priority_score(imagep, &bucket);
    if (bucketp)
    {
        *bucketp = bucket;
    }

    if (bucket == AYA_QUEUE_BUCKET_FAR ||
        bucket == AYA_QUEUE_BUCKET_OFFSCREEN ||
        bucket == AYA_QUEUE_BUCKET_UNREFERENCED)
    {
        if (release_reasonp)
        {
            *release_reasonp = AYA_PENDING_CREATE_RELEASE_DISTANCE;
        }
        return true;
    }

    return false;
}

bool aya_texture_can_enter_teleport_sweep(LLViewerFetchedTexture* imagep)
{
    return imagep &&
        !imagep->isDeleted() &&
        imagep->isInImageList() &&
        imagep->getType() == LLViewerTexture::LOD_TEXTURE &&
        imagep->getBoostLevel() == LLGLTexture::BOOST_NONE &&
        !aya_texture_has_protected_work(imagep) &&
        imagep->getTotalNumFaces() == 0 &&
        !aya_texture_has_volume_ref(imagep) &&
        !imagep->isFetching() &&
        !imagep->isInFastCacheList() &&
        imagep->hasGLTexture();
}

bool aya_texture_can_enter_strength_sweep(LLViewerFetchedTexture* imagep)
{
    const S32 boost = imagep ? imagep->getBoostLevel() : LLGLTexture::BOOST_NONE;
    static LLCachedControl<F32> near_distance_m(gSavedSettings, "AYATextureDistanceCapNearMeters", 10.f);
    return imagep &&
        !imagep->isDeleted() &&
        imagep->isInImageList() &&
        imagep->getType() == LLViewerTexture::LOD_TEXTURE &&
        !imagep->mCreatePending &&
        !imagep->mDownScalePending &&
        !aya_texture_strength_sweep_protected_fttype(imagep) &&
        !imagep->hasCallbacks() &&
        boost < LLGLTexture::BOOST_HIGH &&
        boost != LLGLTexture::BOOST_AVATAR_SELF &&
        boost != LLGLTexture::BOOST_AVATAR_BAKED &&
        boost != LLGLTexture::BOOST_AVATAR_BAKED_SELF &&
        boost != LLGLTexture::BOOST_HUD &&
        boost != LLGLTexture::BOOST_UI &&
        boost != LLGLTexture::BOOST_ICON &&
        boost != LLGLTexture::BOOST_THUMBNAIL &&
        boost != LLGLTexture::BOOST_PREVIEW &&
        boost != LLGLTexture::BOOST_MAP &&
        boost != LLGLTexture::BOOST_MAP_VISIBLE &&
        boost != LLGLTexture::BOOST_SCULPTED &&
        boost != LLGLTexture::BOOST_SELECTED &&
        boost != LLGLTexture::BOOST_SUPER_HIGH &&
        !aya_texture_has_strength_sweep_blocker(imagep) &&
        !aya_texture_has_self_or_hud_face_owner(imagep) &&
        !aya_texture_has_unloaded_avatar_face_owner(imagep, near_distance_m(), true) &&
        !imagep->isFetching() &&
        !imagep->isInFastCacheList() &&
        imagep->hasGLTexture();
}

bool aya_texture_strength_sweep_should_retry_later(LLViewerFetchedTexture* imagep)
{
    if (!imagep ||
        imagep->isDeleted() ||
        !imagep->isInImageList() ||
        imagep->getType() != LLViewerTexture::LOD_TEXTURE)
    {
        return false;
    }

    const S32 boost = imagep->getBoostLevel();
    static LLCachedControl<F32> near_distance_m(gSavedSettings, "AYATextureDistanceCapNearMeters", 10.f);
    if (aya_texture_strength_sweep_protected_fttype(imagep) ||
        boost >= LLGLTexture::BOOST_HIGH ||
        boost == LLGLTexture::BOOST_AVATAR_SELF ||
        boost == LLGLTexture::BOOST_AVATAR_BAKED ||
        boost == LLGLTexture::BOOST_AVATAR_BAKED_SELF ||
        boost == LLGLTexture::BOOST_HUD ||
        boost == LLGLTexture::BOOST_UI ||
        boost == LLGLTexture::BOOST_ICON ||
        boost == LLGLTexture::BOOST_THUMBNAIL ||
        boost == LLGLTexture::BOOST_PREVIEW ||
        boost == LLGLTexture::BOOST_MAP ||
        boost == LLGLTexture::BOOST_MAP_VISIBLE ||
        boost == LLGLTexture::BOOST_SCULPTED ||
        boost == LLGLTexture::BOOST_SELECTED ||
        boost == LLGLTexture::BOOST_SUPER_HIGH ||
        imagep->needsToSaveRawImage() ||
        imagep->forSculpt() ||
        aya_texture_has_self_or_hud_face_owner(imagep) ||
        aya_texture_has_unloaded_avatar_face_owner(imagep, near_distance_m(), true))
    {
        return false;
    }

    return imagep->mCreatePending ||
        imagep->mDownScalePending ||
        imagep->hasCallbacks() ||
        imagep->isFetching() ||
        imagep->isInFastCacheList() ||
        !imagep->hasGLTexture();
}

S32 aya_texture_create_priority_score(LLViewerFetchedTexture* imagep, AYAQueuedTextureBucket* bucketp)
{
    if (bucketp)
    {
        *bucketp = AYA_QUEUE_BUCKET_UNREFERENCED;
    }

    if (!imagep || imagep->isDeleted() || !imagep->isInImageList())
    {
        return S32_MIN / 2;
    }

    S32 score = 0;
    static LLCachedControl<F32> near_distance_m(gSavedSettings, "AYATextureDistanceCapNearMeters", 10.f);
    if (aya_texture_protected_fttype(imagep) ||
        imagep->needsToSaveRawImage() ||
        imagep->forSculpt())
    {
        score += 60000;
        if (bucketp)
        {
            *bucketp = AYA_QUEUE_BUCKET_PROTECTED;
        }
    }
    else if (imagep->hasCallbacks())
    {
        score += 40000;
        if (bucketp)
        {
            *bucketp = AYA_QUEUE_BUCKET_PROTECTED;
        }
    }
    else if (aya_texture_protected_boost(imagep))
    {
        score += 5000;
    }

    if (imagep->isFetching() || imagep->isInFastCacheList())
    {
        score += 5000;
    }

    if (imagep->getTotalNumFaces() == 0)
    {
        if (aya_texture_has_volume_ref(imagep))
        {
            score += 12000;
            if (bucketp && *bucketp != AYA_QUEUE_BUCKET_PROTECTED)
            {
                *bucketp = AYA_QUEUE_BUCKET_MID;
            }
        }
        else if (!aya_texture_has_protected_work(imagep))
        {
            score -= 50000;
            if (bucketp)
            {
                *bucketp = AYA_QUEUE_BUCKET_UNREFERENCED;
            }
        }
        return score;
    }

    const LLVector3 camera_pos = LLViewerCamera::getInstance()->getOrigin();
    bool saw_face = false;
    bool saw_in_frustum = false;
    bool saw_avatar_related = false;
    bool saw_protected_owner = false;
    F32 best_pixel_area = 0.f;
    F32 best_distance = F32_MAX;
    U32 checked_faces = 0;
    const U32 max_faces_to_check = 64;
    static LLCachedControl<F32> mid_distance_m(gSavedSettings, "AYATextureDistanceCapMidMeters", 20.f);
    static LLCachedControl<F32> far_distance_m(gSavedSettings, "AYATextureDistanceCapFarMeters", 50.f);

    for (U32 channel = 0; channel < LLRender::NUM_TEXTURE_CHANNELS && checked_faces < max_faces_to_check; ++channel)
    {
        const LLViewerTexture::ll_face_list_t* faces = imagep->getFaceList(channel);
        const U32 num_faces = llmin((U32)imagep->getNumFaces(channel), (U32)(faces ? faces->size() : 0));
        if (!faces || num_faces == 0)
        {
            continue;
        }

        for (U32 face_index = 0; face_index < num_faces && checked_faces < max_faces_to_check; ++face_index)
        {
            LLFace* facep = (*faces)[face_index];
            LLViewerObject* objp = facep ? facep->getViewerObject() : nullptr;
            if (!facep || !objp)
            {
                continue;
            }

            ++checked_faces;
            saw_face = true;
            saw_in_frustum |= facep->mInFrustum;
            saw_avatar_related |= aya_object_is_avatar_related(objp);
            saw_protected_owner |= aya_object_is_protected_texture_owner(objp);
            best_pixel_area = llmax(best_pixel_area, facep->getPixelArea());
            best_distance = llmin(best_distance, (facep->getPositionAgent() - camera_pos).magVec());
        }
    }

    if (!saw_face)
    {
        if (aya_texture_has_volume_ref(imagep))
        {
            score += 12000;
            if (bucketp && *bucketp != AYA_QUEUE_BUCKET_PROTECTED)
            {
                *bucketp = AYA_QUEUE_BUCKET_MID;
            }
        }
        else if (!aya_texture_has_protected_work(imagep))
        {
            score -= 50000;
            if (bucketp)
            {
                *bucketp = AYA_QUEUE_BUCKET_UNREFERENCED;
            }
        }
        return score;
    }

    if (saw_protected_owner)
    {
        score += 60000;
        if (bucketp)
        {
            *bucketp = AYA_QUEUE_BUCKET_PROTECTED;
        }
    }

    if (saw_avatar_related && saw_in_frustum && best_distance <= near_distance_m())
    {
        score += 60000;
        if (bucketp)
        {
            *bucketp = AYA_QUEUE_BUCKET_PROTECTED;
        }
    }
    else if (saw_avatar_related)
    {
        score += 12000;
    }

    if (saw_in_frustum)
    {
        score += 20000;
    }
    else
    {
        score -= 8000;
    }

    if (best_distance <= near_distance_m())
    {
        score += 25000;
        if (bucketp && *bucketp != AYA_QUEUE_BUCKET_PROTECTED)
        {
            *bucketp = AYA_QUEUE_BUCKET_NEAR;
        }
    }
    else if (best_distance <= mid_distance_m())
    {
        score += 15000;
        if (bucketp && *bucketp != AYA_QUEUE_BUCKET_PROTECTED)
        {
            *bucketp = AYA_QUEUE_BUCKET_MID;
        }
    }
    else if (best_distance <= far_distance_m())
    {
        score += 5000;
        if (bucketp && *bucketp != AYA_QUEUE_BUCKET_PROTECTED)
        {
            *bucketp = AYA_QUEUE_BUCKET_FAR;
        }
    }
    else
    {
        score -= 5000;
        if (bucketp && *bucketp != AYA_QUEUE_BUCKET_PROTECTED)
        {
            *bucketp = AYA_QUEUE_BUCKET_FAR;
        }
    }

    if (!saw_in_frustum && bucketp && *bucketp != AYA_QUEUE_BUCKET_PROTECTED)
    {
        *bucketp = AYA_QUEUE_BUCKET_OFFSCREEN;
    }

    if (best_pixel_area >= 65536.f)
    {
        score += 12000;
    }
    else if (best_pixel_area >= 16384.f)
    {
        score += 6000;
    }
    else if (best_pixel_area <= 1024.f)
    {
        score -= 2000;
    }

    return score;
}

AYAMemorySweepBucket aya_texture_memory_sweep_bucket(
    LLViewerFetchedTexture* imagep,
    F32 near_distance_m,
    F32 far_distance_m,
    F32 min_unreferenced_seconds)
{
    if (!imagep ||
        imagep->isDeleted() ||
        !imagep->isInImageList() ||
        imagep->getType() != LLViewerTexture::LOD_TEXTURE ||
        imagep->mCreatePending ||
        imagep->mDownScalePending ||
        aya_texture_has_protected_work(imagep) ||
        imagep->isFetching() ||
        imagep->isInFastCacheList())
    {
        return AYA_MEMORY_SWEEP_KEEP;
    }

    const bool has_faces = imagep->getTotalNumFaces() > 0;
    const bool has_volume_refs = aya_texture_has_volume_ref(imagep);
    const bool old_enough =
        imagep->getLastReferencedTimer()->getElapsedTimeF32() >= llmax(0.f, min_unreferenced_seconds);

    if (!has_faces && !has_volume_refs && old_enough)
    {
        return AYA_MEMORY_SWEEP_UNREFERENCED;
    }

    bool saw_sweepable_face = false;
    bool saw_protected_owner = false;
    bool any_in_frustum = false;
    bool any_near = false;

    for (U32 channel = 0; channel < LLRender::NUM_TEXTURE_CHANNELS; ++channel)
    {
        const LLViewerTexture::ll_face_list_t* faces = imagep->getFaceList(channel);
        const U32 face_count = llmin((U32)imagep->getNumFaces(channel), (U32)(faces ? faces->size() : 0));
        if (!faces || face_count == 0)
        {
            continue;
        }

        for (U32 face_index = 0; face_index < face_count; ++face_index)
        {
            LLFace* facep = (*faces)[face_index];
            if (!facep || !facep->getViewerObject())
            {
                continue;
            }

            LLViewerObject* objp = facep->getViewerObject();
            const F32 distance = (facep->getPositionAgent() - LLViewerCamera::getInstance()->getOrigin()).magVec();
            if (aya_object_is_protected_texture_owner(objp) ||
                (aya_object_is_avatar_related(objp) && distance <= near_distance_m))
            {
                saw_protected_owner = true;
                continue;
            }

            saw_sweepable_face = true;
            any_in_frustum |= facep->mInFrustum;

            if (distance <= far_distance_m)
            {
                any_near = true;
            }
        }
    }

    if (saw_protected_owner || !saw_sweepable_face)
    {
        return AYA_MEMORY_SWEEP_KEEP;
    }

    if (!any_in_frustum)
    {
        return AYA_MEMORY_SWEEP_OFFSCREEN;
    }

    if (!any_near)
    {
        return AYA_MEMORY_SWEEP_FAR;
    }

    return AYA_MEMORY_SWEEP_KEEP;
}
}

ETexListType get_element_type(S32 priority)
{
    return (priority == LLViewerFetchedTexture::BOOST_ICON || priority == LLViewerFetchedTexture::BOOST_THUMBNAIL) ? TEX_LIST_SCALE : TEX_LIST_STANDARD;
}

///////////////////////////////////////////////////////////////////////////////

LLTextureKey::LLTextureKey()
: textureId(LLUUID::null),
textureType(TEX_LIST_STANDARD)
{
}

LLTextureKey::LLTextureKey(LLUUID id, ETexListType tex_type)
: textureId(id), textureType(tex_type)
{
}

///////////////////////////////////////////////////////////////////////////////

LLViewerTextureList::LLViewerTextureList()
    : mForceResetTextureStats(false),
    mInitialized(false),
    mAYATeleportTextureGeneration(0),
    mAYATeleportSweepActive(false)
{
}

void LLViewerTextureList::init()
{
    mInitialized = true ;
    sNumImages = 0;
    doPreloadImages();
}


void LLViewerTextureList::doPreloadImages()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    LL_DEBUGS("ViewerImages") << "Preloading images..." << LL_ENDL;

    llassert_always(mInitialized) ;
    llassert_always(mImageList.empty()) ;
    llassert_always(mUUIDMap.empty()) ;

    // Set the "missing asset" image
    LLViewerFetchedTexture::sMissingAssetImagep = LLViewerTextureManager::getFetchedTextureFromFile("missing_asset.tga", FTT_LOCAL_FILE, MIPMAP_NO, LLViewerFetchedTexture::BOOST_UI);

    // Set the "white" image
    LLViewerFetchedTexture::sWhiteImagep = LLViewerTextureManager::getFetchedTextureFromFile("white.tga", FTT_LOCAL_FILE, MIPMAP_NO, LLViewerFetchedTexture::BOOST_UI);
    LLTexUnit::sWhiteTexture = LLViewerFetchedTexture::sWhiteImagep->getTexName();
    LLUIImageList* image_list = LLUIImageList::getInstance();

    // Set default particle texture
    LLViewerFetchedTexture::sDefaultParticleImagep = LLViewerTextureManager::getFetchedTextureFromFile("pixiesmall.j2c");

    // Set the default flat normal map
    // BLANK_OBJECT_NORMAL has a version on dataserver, but it has compression artifacts
    LLViewerFetchedTexture::sFlatNormalImagep =
        LLViewerTextureManager::getFetchedTextureFromFile("flatnormal.tga",
                                                          FTT_LOCAL_FILE,
                                                          MIPMAP_NO,
                                                          LLViewerFetchedTexture::BOOST_BUMP,
                                                          LLViewerTexture::FETCHED_TEXTURE,
                                                          0,
                                                          0,
                                                          BLANK_OBJECT_NORMAL);

    // PBR: irradiance
    LLViewerFetchedTexture::sDefaultIrradiancePBRp = LLViewerTextureManager::getFetchedTextureFromFile("default_irradiance.png", FTT_LOCAL_FILE, MIPMAP_YES, LLViewerFetchedTexture::BOOST_UI);

    image_list->initFromFile();

    // turn off clamping and bilinear filtering for uv picking images
    //LLViewerFetchedTexture* uv_test = preloadUIImage("uv_test1.tga", LLUUID::null, false);
    //uv_test->setClamp(false, false);
    //uv_test->setMipFilterNearest(true, true);
    //uv_test = preloadUIImage("uv_test2.tga", LLUUID::null, false);
    //uv_test->setClamp(false, false);
    //uv_test->setMipFilterNearest(true, true);

    LLViewerFetchedTexture* image = LLViewerTextureManager::getFetchedTextureFromFile("silhouette.j2c", FTT_LOCAL_FILE, MIPMAP_YES, LLViewerFetchedTexture::BOOST_UI);
    if (image)
    {
        image->setAddressMode(LLTexUnit::TAM_WRAP);
        mImagePreloads.insert(image);
    }
    image = LLViewerTextureManager::getFetchedTextureFromFile("world/NoEntryLines.png", FTT_LOCAL_FILE, MIPMAP_YES, LLViewerFetchedTexture::BOOST_UI);
    if (image)
    {
        image->setAddressMode(LLTexUnit::TAM_WRAP);
        mImagePreloads.insert(image);
    }
    image = LLViewerTextureManager::getFetchedTextureFromFile("world/NoEntryPassLines.png", FTT_LOCAL_FILE, MIPMAP_YES, LLViewerFetchedTexture::BOOST_UI);
    if (image)
    {
        image->setAddressMode(LLTexUnit::TAM_WRAP);
        mImagePreloads.insert(image);
    }
    image = LLViewerTextureManager::getFetchedTextureFromFile("transparent.j2c", FTT_LOCAL_FILE, MIPMAP_YES, LLViewerFetchedTexture::BOOST_UI, LLViewerTexture::FETCHED_TEXTURE,
        0, 0, IMG_TRANSPARENT);
    if (image)
    {
        image->setAddressMode(LLTexUnit::TAM_WRAP);
        mImagePreloads.insert(image);
    }
    image = LLViewerTextureManager::getFetchedTextureFromFile("alpha_gradient.tga", FTT_LOCAL_FILE, MIPMAP_YES, LLViewerFetchedTexture::BOOST_UI, LLViewerTexture::FETCHED_TEXTURE,
        GL_ALPHA8, GL_ALPHA, IMG_ALPHA_GRAD);
    if (image)
    {
        image->setAddressMode(LLTexUnit::TAM_CLAMP);
        mImagePreloads.insert(image);
    }
    image = LLViewerTextureManager::getFetchedTextureFromFile("alpha_gradient_2d.j2c", FTT_LOCAL_FILE, MIPMAP_YES, LLViewerFetchedTexture::BOOST_UI, LLViewerTexture::FETCHED_TEXTURE,
        GL_ALPHA8, GL_ALPHA, IMG_ALPHA_GRAD_2D);
    if (image)
    {
        image->setAddressMode(LLTexUnit::TAM_CLAMP);
        mImagePreloads.insert(image);
    }
}

static std::string get_texture_list_name()
{
    // <FS:Ansariel> OpenSim compatibility
    //if (LLGridManager::getInstance()->isInProductionGrid())
    if (LLGridManager::getInstance()->isInSLMain())
    // </FS:Ansariel>
    {
        return gDirUtilp->getExpandedFilename(LL_PATH_CACHE,
            "texture_list_" + gSavedSettings.getString("LoginLocation") + "." + gDirUtilp->getUserName() + ".xml");
    }
    else
    {
        const std::string& grid_id_str = LLGridManager::getInstance()->getGridId();
        const std::string& grid_id_lower = utf8str_tolower(grid_id_str);
        return gDirUtilp->getExpandedFilename(LL_PATH_CACHE,
            "texture_list_" + gSavedSettings.getString("LoginLocation") + "." + gDirUtilp->getUserName() + "." + grid_id_lower + ".xml");
    }
}

void LLViewerTextureList::doPrefetchImages()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    // todo: do not load without getViewerAssetUrl()
    // either fail login without caps or provide this
    // in some other way, textures won't load otherwise
    LLViewerFetchedTexture *imagep = findImage(DEFAULT_WATER_NORMAL, TEX_LIST_STANDARD);
    if (!imagep)
    {
        // add it to mImagePreloads only once
        imagep = LLViewerTextureManager::getFetchedTexture(DEFAULT_WATER_NORMAL, FTT_DEFAULT, MIPMAP_YES, LLViewerFetchedTexture::BOOST_UI);
        if (imagep)
        {
            imagep->setAddressMode(LLTexUnit::TAM_WRAP);
            mImagePreloads.insert(imagep);
        }
    }

    LLViewerTextureManager::getFetchedTexture(IMG_SHOT);
    LLViewerTextureManager::getFetchedTexture(IMG_SMOKE_POOF);
    LLViewerFetchedTexture::sSmokeImagep = LLViewerTextureManager::getFetchedTexture(IMG_SMOKE, FTT_DEFAULT, true, LLGLTexture::BOOST_UI);
    LLViewerFetchedTexture::sSmokeImagep->setNoDelete();

    LLStandardBumpmap::addstandard();

    if (LLAppViewer::instance()->getPurgeCache())
    {
        // cache was purged, no point
        return;
    }

    // Pre-fetch textures from last logout
    LLSD imagelist;
    std::string filename = get_texture_list_name();
    llifstream file;
    file.open(filename.c_str());
    if (file.is_open())
    {
        if ( ! LLSDSerialize::fromXML(imagelist, file) )
        {
            file.close();
            LL_WARNS() << "XML parse error reading texture list '" << filename << "'" << LL_ENDL;
            LL_WARNS() << "Removing invalid texture list '" << filename << "'" << LL_ENDL;
            LLFile::remove(filename);
            return;
        }
        file.close();
    }
    S32 texture_count = 0;
    for (LLSD::array_iterator iter = imagelist.beginArray();
         iter != imagelist.endArray(); ++iter)
    {
        LLSD imagesd = *iter;
        LLUUID uuid = imagesd["uuid"];
        S32 pixel_area = imagesd["area"];
        S32 texture_type = imagesd["type"];

        if((LLViewerTexture::FETCHED_TEXTURE == texture_type || LLViewerTexture::LOD_TEXTURE == texture_type) && !LLViewerTexture::isInvisiprim(uuid))
        {
            LLViewerFetchedTexture* image = LLViewerTextureManager::getFetchedTexture(uuid, FTT_DEFAULT, MIPMAP_TRUE, LLGLTexture::BOOST_NONE, texture_type);
            if (image)
            {
                texture_count += 1;
                image->addTextureStats((F32)pixel_area);
            }
        }
    }
    LL_DEBUGS() << "fetched " << texture_count << " images from " << filename << LL_ENDL;
}

///////////////////////////////////////////////////////////////////////////////

LLViewerTextureList::~LLViewerTextureList()
{
}

void LLViewerTextureList::shutdown()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    LL_WARNS() << "Shutdown called" << LL_ENDL;
    // clear out preloads
    mImagePreloads.clear();

    // Write out list of currently loaded textures for precaching on startup
    typedef std::set<std::pair<S32,LLViewerFetchedTexture*> > image_area_list_t;
    image_area_list_t image_area_list;
    for (image_list_t::iterator iter = mImageList.begin();
         iter != mImageList.end(); ++iter)
    {
        LLViewerFetchedTexture* image = *iter;
        if (!image->hasGLTexture() ||
            !image->getUseDiscard() ||
            image->needsAux() ||
            !image->getTargetHost().isInvalid() ||
            !image->getUrl().empty() ||
            image->isInvisiprim()
            )
        {
            continue; // avoid UI, baked, and other special images
        }
        if(!image->getBoundRecently())
        {
            continue ;
        }
        S32 desired = image->getDesiredDiscardLevel();
        if (desired >= 0 && desired < MAX_DISCARD_LEVEL)
        {
            S32 pixel_area = image->getWidth(desired) * image->getHeight(desired);
            image_area_list.insert(std::make_pair(pixel_area, image));
        }
    }

    LLSD imagelist;
    const S32 max_count = 1000;
    S32 count = 0;
    S32 image_type ;
    for (image_area_list_t::reverse_iterator riter = image_area_list.rbegin();
         riter != image_area_list.rend(); ++riter)
    {
        LLViewerFetchedTexture* image = riter->second;
        image_type = (S32)image->getType() ;
        imagelist[count]["area"] = riter->first;
        imagelist[count]["uuid"] = image->getID();
        imagelist[count]["type"] = image_type;
        if (++count >= max_count)
            break;
    }

    if (count > 0 && !gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "").empty())
    {
        std::string filename = get_texture_list_name();
        llofstream file;
        file.open(filename.c_str());
        LL_DEBUGS() << "saving " << imagelist.size() << " image list entries" << LL_ENDL;
        LLSDSerialize::toPrettyXML(imagelist, file);
    }

    //
    // Clean up "loaded" callbacks.
    //
    mCallbackList.clear();

    // Flush all of the references
    while (!mCreateTextureList.empty())
    {
        LLPointer<LLViewerFetchedTexture> imagep = mCreateTextureList.front();
        if (imagep.notNull())
        {
            imagep->mCreatePending = false;
        }
        mCreateTextureList.pop();
    }
    while (!mDownScaleQueue.empty())
    {
        LLPointer<LLViewerFetchedTexture> imagep = mDownScaleQueue.front();
        if (imagep.notNull())
        {
            imagep->mDownScalePending = false;
        }
        mDownScaleQueue.pop();
    }
    while (!mAYATeleportSweepQueue.empty())
    {
        mAYATeleportSweepQueue.pop();
    }
    mAYATeleportSweepActive = false;
    mFastCacheList.clear();

    mUUIDMap.clear();

    mImageList.clear();

    mInitialized = false ; //prevent loading textures again.
}

void LLViewerTextureList::dump()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    LL_INFOS() << "LLViewerTextureList::dump()" << LL_ENDL;
    for (image_list_t::iterator it = mImageList.begin(); it != mImageList.end(); ++it)
    {
        LLViewerFetchedTexture* image = *it;

        LL_INFOS() << "priority " << image->getMaxVirtualSize()
        << " boost " << image->getBoostLevel()
        << " size " << image->getWidth() << "x" << image->getHeight()
        << " discard " << image->getDiscardLevel()
        << " desired " << image->getDesiredDiscardLevel()
        << " http://asset.siva.lindenlab.com/" << image->getID() << ".texture"
        << LL_ENDL;
    }
}

void LLViewerTextureList::destroyGL()
{
    LLImageGL::destroyGL();
}

/* Vertical tab container button image IDs
 Seem to not decode when running app in debug.

 const LLUUID BAD_IMG_ONE("1097dcb3-aef9-8152-f471-431d840ea89e");
 const LLUUID BAD_IMG_TWO("bea77041-5835-1661-f298-47e2d32b7a70");
 */

///////////////////////////////////////////////////////////////////////////////

LLViewerFetchedTexture* LLViewerTextureList::getImageFromFile(const std::string& filename,
                                                   FTType f_type,
                                                   bool usemipmaps,
                                                   LLViewerTexture::EBoostLevel boost_priority,
                                                   S8 texture_type,
                                                   LLGLint internal_format,
                                                   LLGLenum primary_format,
                                                   const LLUUID& force_id)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    LL_PROFILE_ZONE_TEXT(filename.c_str(), filename.size());
    if(!mInitialized)
    {
        return NULL ;
    }

    std::string full_path = gDirUtilp->findSkinnedFilename("textures", filename);
    if (full_path.empty())
    {
        LL_WARNS() << "Failed to find local image file: " << filename << LL_ENDL;
        LLViewerTexture::EBoostLevel priority = LLGLTexture::BOOST_UI;
        return LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT, FTT_DEFAULT, true, priority);
    }

    std::string url = "file://" + full_path;

    return getImageFromUrl(url, f_type, usemipmaps, boost_priority, texture_type, internal_format, primary_format, force_id);
}

LLViewerFetchedTexture* LLViewerTextureList::getImageFromUrl(const std::string& url,
                                                   FTType f_type,
                                                   bool usemipmaps,
                                                   LLViewerTexture::EBoostLevel boost_priority,
                                                   S8 texture_type,
                                                   LLGLint internal_format,
                                                   LLGLenum primary_format,
                                                   const LLUUID& force_id)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if(!mInitialized)
    {
        return NULL ;
    }

    // generate UUID based on hash of filename
    LLUUID new_id;
    if (force_id.notNull())
    {
        new_id = force_id;
    }
    else
    {
        new_id.generate(url);
    }

    LLPointer<LLViewerFetchedTexture> imagep = findImage(new_id, get_element_type(boost_priority));

    if (!imagep.isNull())
    {
        LLViewerFetchedTexture *texture = imagep.get();
        if (texture->getUrl().empty())
        {
            LL_WARNS() << "Requested texture " << new_id << " already exists but does not have a URL" << LL_ENDL;
        }
        else if (texture->getUrl() != url)
        {
            // This is not an error as long as the images really match -
            // e.g. could be two avatars wearing the same outfit.
            LL_DEBUGS("Avatar") << "Requested texture " << new_id
                                << " already exists with a different url, requested: " << url
                                << " current: " << texture->getUrl() << LL_ENDL;
        }

    }
    if (imagep.isNull())
    {
        switch(texture_type)
        {
        case LLViewerTexture::FETCHED_TEXTURE:
            imagep = new LLViewerFetchedTexture(url, f_type, new_id, usemipmaps);
            break ;
        case LLViewerTexture::LOD_TEXTURE:
            imagep = new LLViewerLODTexture(url, f_type, new_id, usemipmaps);
            break ;
        default:
            LL_ERRS() << "Invalid texture type " << texture_type << LL_ENDL ;
        }

        if (internal_format && primary_format)
        {
            imagep->setExplicitFormat(internal_format, primary_format);
        }

        addImage(imagep, get_element_type(boost_priority));

        if (boost_priority != 0)
        {
            if (boost_priority == LLViewerFetchedTexture::BOOST_UI)
            {
                imagep->dontDiscard();
            }
            if (boost_priority == LLViewerFetchedTexture::BOOST_ICON
                || boost_priority == LLViewerFetchedTexture::BOOST_THUMBNAIL)
            {
                // Agent and group Icons are downloadable content, nothing manages
                // icon deletion yet, so they should not persist
                imagep->dontDiscard();
                imagep->forceActive();
            }
            imagep->setBoostLevel(boost_priority);
        }
    }

    imagep->setGLTextureCreated(true);

    return imagep;
}

LLImageRaw* LLViewerTextureList::getRawImageFromMemory(const U8* data, U32 size, std::string_view mimetype)
{
    LLPointer<LLImageFormatted> image = LLImageFormatted::loadFromMemory(data, size, mimetype);

    if (image)
    {
        LLImageRaw* raw_image = new LLImageRaw();
        image->decode(raw_image, 0.f);
        return raw_image;
    }
    else
    {
        return nullptr;
    }
}

LLViewerFetchedTexture* LLViewerTextureList::getImageFromMemory(const U8* data, U32 size, std::string_view mimetype)
{
    LLPointer<LLImageRaw> raw_image = getRawImageFromMemory(data, size, mimetype);
    if (raw_image.notNull())
    {
        LLViewerFetchedTexture* imagep = new LLViewerFetchedTexture(raw_image, FTT_LOCAL_FILE, true);
        addImage(imagep, TEX_LIST_STANDARD);

        imagep->dontDiscard();
        imagep->setBoostLevel(LLViewerFetchedTexture::BOOST_PREVIEW);
        return imagep;
    }
    else
    {
        return nullptr;
    }
}

LLViewerFetchedTexture* LLViewerTextureList::getImage(const LLUUID &image_id,
                                                   FTType f_type,
                                                   bool usemipmaps,
                                                   LLViewerTexture::EBoostLevel boost_priority,
                                                   S8 texture_type,
                                                   LLGLint internal_format,
                                                   LLGLenum primary_format,
                                                   LLHost request_from_host)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if(!mInitialized)
    {
        return NULL ;
    }

    // Return the image with ID image_id
    // If the image is not found, creates new image and
    // enqueues a request for transmission

    if (image_id.isNull())
    {
        return (LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT, FTT_DEFAULT, true, LLGLTexture::BOOST_UI));
    }

    LLPointer<LLViewerFetchedTexture> imagep = findImage(image_id, get_element_type(boost_priority));
    if (!imagep.isNull())
    {
        LLViewerFetchedTexture *texture = imagep.get();
        if (request_from_host.isOk() &&
            !texture->getTargetHost().isOk())
        {
            LL_WARNS() << "Requested texture " << image_id << " already exists but does not have a host" << LL_ENDL;
        }
        else if (request_from_host.isOk() &&
                 texture->getTargetHost().isOk() &&
                 request_from_host != texture->getTargetHost())
        {
            LL_WARNS() << "Requested texture " << image_id << " already exists with a different target host, requested: "
                    << request_from_host << " current: " << texture->getTargetHost() << LL_ENDL;
        }
        if (f_type != FTT_DEFAULT && imagep->getFTType() != f_type)
        {
            LL_WARNS() << "FTType mismatch: requested " << f_type << " image has " << imagep->getFTType() << LL_ENDL;
        }

    }
    if (imagep.isNull())
    {
        imagep = createImage(image_id, f_type, usemipmaps, boost_priority, texture_type, internal_format, primary_format, request_from_host) ;
    }

    imagep->setGLTextureCreated(true);

    return imagep;
}

//when this function is called, there is no such texture in the gTextureList with image_id.
LLViewerFetchedTexture* LLViewerTextureList::createImage(const LLUUID &image_id,
                                                   FTType f_type,
                                                   bool usemipmaps,
                                                   LLViewerTexture::EBoostLevel boost_priority,
                                                   S8 texture_type,
                                                   LLGLint internal_format,
                                                   LLGLenum primary_format,
                                                   LLHost request_from_host)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    static LLCachedControl<bool> fast_cache_fetching_enabled(gSavedSettings, "FastCacheFetchEnabled", true); // <FS:Ansariel> Keep Fast Cache option

    LLPointer<LLViewerFetchedTexture> imagep ;
    switch(texture_type)
    {
    case LLViewerTexture::FETCHED_TEXTURE:
        imagep = new LLViewerFetchedTexture(image_id, f_type, request_from_host, usemipmaps);
        break ;
    case LLViewerTexture::LOD_TEXTURE:
        imagep = new LLViewerLODTexture(image_id, f_type, request_from_host, usemipmaps);
        break ;
    default:
        LL_ERRS() << "Invalid texture type " << texture_type << LL_ENDL ;
    }

    if (internal_format && primary_format)
    {
        imagep->setExplicitFormat(internal_format, primary_format);
    }

    // <FS:minerjr> [FIRE-35428] - Mega prim issue - fix compressed sculpted textures
    // Sculpted textures use the RGBA data for coodinates, so any compression can cause artifacts.
    if (boost_priority == LLViewerFetchedTexture::BOOST_SCULPTED)
    {
        // Disable the compression of BOOST_SCULPTED textures
        if (imagep->getGLTexture())
        {
            imagep->getGLTexture()->setAllowCompression(false);
        }
    }
    // </FS:minerjr> [FIRE-35428]

    addImage(imagep, get_element_type(boost_priority));

    if (boost_priority != 0)
    {
        if (boost_priority == LLViewerFetchedTexture::BOOST_UI)
        {
            imagep->dontDiscard();
        }
        if (boost_priority == LLViewerFetchedTexture::BOOST_ICON
            || boost_priority == LLViewerFetchedTexture::BOOST_THUMBNAIL)
        {
            // Agent and group Icons are downloadable content, nothing manages
            // icon deletion yet, so they should not persist.
            imagep->dontDiscard();
            imagep->forceActive();
        }
        imagep->setBoostLevel(boost_priority);
    }
    else
    {
        //by default, the texture can not be removed from memory even if it is not used.
        //here turn this off
        //if this texture should be set to NO_DELETE, call setNoDelete() afterwards.
        imagep->forceActive() ;
    }

    // <FS:Ansariel> Keep Fast Cache option
    // <FS:minerjr> [FIRE-35428] - Mega prim issue - fix compressed sculpted textures
    //if(fast_cache_fetching_enabled)
    // If the texture is Sculpted, don't allow it to be added to fast cache as it can affect the texture.
    if(fast_cache_fetching_enabled && boost_priority != LLViewerFetchedTexture::BOOST_SCULPTED)
    // </FS:minerjr> [FIRE-35428]
    {
        mFastCacheList.insert(imagep);
        imagep->setInFastCacheList(true);
    }
    // </FS:Ansariel>
    return imagep ;
}

void LLViewerTextureList::findTexturesByID(const LLUUID &image_id, std::vector<LLViewerFetchedTexture*> &output)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    LLTextureKey search_key(image_id, TEX_LIST_STANDARD);
    uuid_map_t::iterator iter = mUUIDMap.lower_bound(search_key);
    while (iter != mUUIDMap.end() && iter->first.textureId == image_id)
    {
        output.push_back(iter->second);
        iter++;
    }
}

LLViewerFetchedTexture *LLViewerTextureList::findImage(const LLTextureKey &search_key)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    uuid_map_t::iterator iter = mUUIDMap.find(search_key);
    if (iter == mUUIDMap.end())
        return NULL;
    return iter->second;
}

LLViewerFetchedTexture *LLViewerTextureList::findImage(const LLUUID &image_id, ETexListType tex_type)
{
    return findImage(LLTextureKey(image_id, tex_type));
}

void LLViewerTextureList::addImageToList(LLViewerFetchedTexture *image)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    assert_main_thread();
    llassert_always(mInitialized) ;
    llassert(image);
    if (image->isInImageList())
    {   // Flag is already set?
        LL_WARNS() << "LLViewerTextureList::addImageToList - image " << image->getID()  << " already in list" << LL_ENDL;
    }
    else
    {
        if (!(mImageList.insert(image)).second)
        {
            LL_WARNS() << "Error happens when insert image " << image->getID()  << " into mImageList!" << LL_ENDL ;
        }
        image->setInImageList(true);
    }
}

void LLViewerTextureList::removeImageFromList(LLViewerFetchedTexture *image)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    assert_main_thread();
    llassert_always(mInitialized) ;
    llassert(image);

    size_t count = 0;
    if (image->isInImageList())
    {
        image->setInImageList(false);
        count = mImageList.erase(image) ;
        if(count != 1)
        {
            LL_INFOS() << "Image  " << image->getID()
                << " had mInImageList set but mImageList.erase() returned " << count
                << LL_ENDL;
        }
    }
    else
    {   // Something is wrong, image is expected in list or callers should check first
        LL_INFOS() << "Calling removeImageFromList() for " << image->getID()
            << " but doesn't have mInImageList set"
            << " ref count is " << image->getNumRefs()
            << LL_ENDL;
        uuid_map_t::iterator iter = mUUIDMap.find(LLTextureKey(image->getID(), (ETexListType)image->getTextureListType()));
        if(iter == mUUIDMap.end())
        {
            LL_INFOS() << "Image  " << image->getID() << " is also not in mUUIDMap!" << LL_ENDL ;
        }
        else if (iter->second != image)
        {
            LL_INFOS() << "Image  " << image->getID() << " was in mUUIDMap but with different pointer" << LL_ENDL ;
    }
        else
    {
            LL_INFOS() << "Image  " << image->getID() << " was in mUUIDMap with same pointer" << LL_ENDL ;
        }
        count = mImageList.erase(image) ;
        llassert(count != 0);
        if(count != 0)
        {   // it was in the list already?
            LL_WARNS() << "Image  " << image->getID()
                << " had mInImageList false but mImageList.erase() returned " << count
                << LL_ENDL;
        }
    }
}

void LLViewerTextureList::addImage(LLViewerFetchedTexture *new_image, ETexListType tex_type)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if (!new_image)
    {
        return;
    }
    //llassert(new_image);
    LLUUID image_id = new_image->getID();
    LLTextureKey key(image_id, tex_type);

    LLViewerFetchedTexture *image = findImage(key);
    if (image)
    {
        LL_INFOS() << "Image with ID " << image_id << " already in list" << LL_ENDL;
    }
    sNumImages++;

    addImageToList(new_image);
    mUUIDMap[key] = new_image;
    new_image->setTextureListType(tex_type);
}


void LLViewerTextureList::deleteImage(LLViewerFetchedTexture *image)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if( image)
    {
        if (image->hasCallbacks())
        {
            mCallbackList.erase(image);
        }
        LLTextureKey key(image->getID(), (ETexListType)image->getTextureListType());
        llverify(mUUIDMap.erase(key) == 1);
        sNumImages--;
        removeImageFromList(image);
    }
}

///////////////////////////////////////////////////////////////////////////////


void LLViewerTextureList::updateImages(F32 max_time)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    static bool cleared = false;
    if(gTeleportDisplay)
    {
        if(!cleared)
        {
            startAYATeleportTextureSweep();
            clearFetchingRequests();
            gPipeline.clearRebuildGroups();
            cleared = true;
            return;
        }
        // ARRIVING is a delay to let things decode, cache and process,
        // so process textures like normal despite gTeleportDisplay
        if (gAgent.getTeleportState() != LLAgent::TELEPORT_ARRIVING)
        {
            return;
        }
    }
    else
    {
        cleared = false;
    }

    LLAppViewer::getTextureFetch()->setTextureBandwidth((F32)LLTrace::get_frame_recording().getPeriodMeanPerSec(LLStatViewer::TEXTURE_NETWORK_DATA_RECEIVED).value());

    {
        using namespace LLStatViewer;
        sample(NUM_IMAGES, sNumImages);
        sample(NUM_RAW_IMAGES, LLImageRaw::sRawImageCount);
        sample(FORMATTED_MEM, F64Bytes(LLImageFormatted::sGlobalFormattedMemory));
    }

    // make sure each call below gets at least its "fair share" of time
    F32 min_time = max_time * 0.33f;
    F32 remaining_time = max_time;

    //loading from fast cache
    remaining_time -= updateImagesLoadingFastCache(remaining_time);
    remaining_time = llmax(remaining_time, min_time);

    //dispatch to texture fetch threads
    remaining_time -= updateImagesFetchTextures(remaining_time);
    remaining_time = llmax(remaining_time, min_time);

    processAYATeleportTextureSweep();

    //handle results from decode threads
    updateImagesCreateTextures(remaining_time);

    static LLCachedControl<U32> max_callbacks_per_frame(gSavedSettings, "RenderTextureMaxCallbacksPerFrame", 16);
    static LLCachedControl<U32> guard_max_callbacks_per_frame(gSavedSettings, "RenderTextureGuardMaxCallbacksPerFrame", 3);
    static LLCachedControl<F32> callback_time_ms(gSavedSettings, "RenderTextureCallbackTimeSliceMs", 2.f);
    static LLCachedControl<F32> guard_callback_time_ms(gSavedSettings, "RenderTextureGuardCallbackTimeSliceMs", 0.75f);
    static LLCachedControl<U32> callback_backlog_threshold(gSavedSettings, "AYALoadingDynamicResolutionTextureCallbackBacklogThreshold", 24);
    const bool loading_texture_management = aya_texture_management_enabled();
    const bool callback_guard_budget_active =
        loading_texture_management &&
        callback_backlog_threshold() > 0 &&
        mCallbackList.size() >= callback_backlog_threshold();
    const U32 effective_max_callbacks = callback_guard_budget_active ? guard_max_callbacks_per_frame() : max_callbacks_per_frame();
    const F32 effective_callback_budget = llmax(0.001f, (callback_guard_budget_active ? guard_callback_time_ms() : callback_time_ms()) * 0.001f);

    LLTimer callback_timer;
    U32 callback_count = 0;
    bool didone = false;
    std::set<LLViewerFetchedTexture*> priority_callbacks_processed;
    if (loading_texture_management && !mCallbackList.empty())
    {
        std::vector<LLPointer<LLViewerFetchedTexture> > priority_callbacks;
        priority_callbacks.reserve(llmin((U32)mCallbackList.size(), (U32)32));
        for (image_list_t::iterator iter = mCallbackList.begin();
             iter != mCallbackList.end(); ++iter)
        {
            LLViewerFetchedTexture* image = *iter;
            if (aya_texture_is_avatar_callback_priority(image))
            {
                priority_callbacks.push_back(image);
                if (priority_callbacks.size() >= 32)
                {
                    break;
                }
            }
        }

        const U32 priority_callback_limit = effective_max_callbacks;
        const F32 priority_callback_budget = effective_callback_budget;
        U32 priority_callback_count = 0;
        for (std::vector<LLPointer<LLViewerFetchedTexture> >::iterator iter = priority_callbacks.begin();
             iter != priority_callbacks.end(); ++iter)
        {
            if (priority_callback_count >= priority_callback_limit ||
                (priority_callback_count > 0 && callback_timer.getElapsedTimeF32() > priority_callback_budget))
            {
                break;
            }

            LLViewerFetchedTexture* image = *iter;
            if (!image)
            {
                continue;
            }

            const bool callback_ran = image->doLoadedCallbacks();
            didone = callback_ran || didone;
            priority_callbacks_processed.insert(image);
            ++priority_callback_count;
            if (callback_ran)
            {
                ++callback_count;
            }
        }
    }

    for (image_list_t::iterator iter = mCallbackList.begin();
        iter != mCallbackList.end(); )
    {
        if (callback_count >= effective_max_callbacks ||
            (callback_count > 0 && callback_timer.getElapsedTimeF32() > effective_callback_budget))
        {
            break;
        }

        //trigger loaded callbacks on local textures immediately
        LLViewerFetchedTexture* image = *iter++;
        if (priority_callbacks_processed.find(image) != priority_callbacks_processed.end())
        {
            continue;
        }
        if (!image->getUrl().empty())
        {
            // Do stuff to handle callbacks, update priorities, etc.
            didone = image->doLoadedCallbacks();
            ++callback_count;
        }
        else if (!didone)
        {
            // Do stuff to handle callbacks, update priorities, etc.
            didone = image->doLoadedCallbacks();
            ++callback_count;
        }

        if (callback_timer.getElapsedTimeF32() > effective_callback_budget)
        {
            break;
        }
    }

    updateImagesUpdateStats();
}

void LLViewerTextureList::startAYATeleportTextureSweep()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    static LLCachedControl<bool> sweep_enabled(gSavedSettings, "AYATeleportTextureSweepEnabled", true);
    static LLCachedControl<U32> max_queued(gSavedSettings, "AYATeleportTextureSweepMaxQueuedTextures", 2048);
    static LLCachedControl<bool> sweep_log(gSavedSettings, "AYATeleportTextureSweepLog", true);

    if (mAYATeleportSweepActive)
    {
        if (sweep_log())
        {
            LL_INFOS("AYATextureCap")
                << "teleport_texture_sweep event=skip"
                << " reason=active"
                << " generation=" << mAYATeleportTextureGeneration
                << " remaining=" << mAYATeleportSweepQueue.size()
                << LL_ENDL;
        }
        return;
    }

    if (!aya_texture_management_enabled() || !sweep_enabled())
    {
        return;
    }

    while (!mAYATeleportSweepQueue.empty())
    {
        mAYATeleportSweepQueue.pop();
    }

    ++mAYATeleportTextureGeneration;
    if (mAYATeleportTextureGeneration == 0)
    {
        ++mAYATeleportTextureGeneration;
    }

    const U32 queue_limit = llclamp((S32)max_queued(), 0, 16384);
    U32 queued = 0;
    U32 skipped_active = 0;
    U32 skipped_special = 0;
    U32 skipped_cap = 0;
    for (image_list_t::iterator iter = mImageList.begin();
         iter != mImageList.end(); ++iter)
    {
        LLViewerFetchedTexture* imagep = *iter;
        if (!imagep ||
            imagep->getType() != LLViewerTexture::LOD_TEXTURE ||
            imagep->getBoostLevel() != LLGLTexture::BOOST_NONE)
        {
            ++skipped_special;
            continue;
        }

        if (imagep->getTotalNumFaces() > 0 || aya_texture_has_volume_ref(imagep))
        {
            ++skipped_active;
            continue;
        }

        if (!aya_texture_can_enter_teleport_sweep(imagep))
        {
            ++skipped_special;
            continue;
        }

        if (queued >= queue_limit)
        {
            ++skipped_cap;
            break;
        }

        imagep->mAYATeleportSeenGeneration = mAYATeleportTextureGeneration;
        imagep->mAYATeleportUsedGeneration = 0;
        mAYATeleportSweepQueue.push(imagep);
        ++queued;
    }

    mAYATeleportSweepActive = queued > 0;
    mAYATeleportSweepTimer.reset();
    mAYATeleportSweepLogTimer.reset();

    if (sweep_log())
    {
        LL_INFOS("AYATextureCap")
            << "teleport_texture_sweep event=start"
            << " generation=" << mAYATeleportTextureGeneration
            << " queued=" << queued
            << " skipped_active=" << skipped_active
            << " skipped_special=" << skipped_special
            << " skipped_cap=" << skipped_cap
            << LL_ENDL;
    }
}

void LLViewerTextureList::processAYATeleportTextureSweep()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    static LLCachedControl<bool> sweep_enabled(gSavedSettings, "AYATeleportTextureSweepEnabled", true);
    static LLCachedControl<F32> grace_seconds(gSavedSettings, "AYATeleportTextureSweepGraceSeconds", 20.f);
    static LLCachedControl<U32> max_per_frame(gSavedSettings, "AYATeleportTextureSweepMaxTexturesPerFrame", 128);
    static LLCachedControl<bool> destroy_gl_enabled(gSavedSettings, "AYATeleportTextureSweepDestroyGLEnabled", false);
    static LLCachedControl<bool> sweep_log(gSavedSettings, "AYATeleportTextureSweepLog", true);

    if (!mAYATeleportSweepActive)
    {
        return;
    }

    if (!aya_texture_management_enabled() || !sweep_enabled())
    {
        while (!mAYATeleportSweepQueue.empty())
        {
            mAYATeleportSweepQueue.pop();
        }
        mAYATeleportSweepActive = false;
        if (sweep_log())
        {
            LL_INFOS("AYATextureCap")
                << "teleport_texture_sweep event=cancel"
                << " reason=disabled"
                << " texture_management_enabled=" << (aya_texture_management_enabled() ? "true" : "false")
                << " sweep_enabled=" << (sweep_enabled() ? "true" : "false")
                << LL_ENDL;
        }
        return;
    }

    if (mAYATeleportSweepTimer.getElapsedTimeF32() < llmax(0.f, grace_seconds()))
    {
        return;
    }

    const U32 budget = llclamp((S32)max_per_frame(), 1, 4096);
    U32 scanned = 0;
    U32 scheduled = 0;
    U32 kept_used = 0;
    U32 kept_active = 0;
    U32 kept_special = 0;
    U32 skipped_no_gl = 0;
    U32 destroyed_gl = 0;
    U32 destroyed_raw = 0;

    while (!mAYATeleportSweepQueue.empty() && scanned < budget)
    {
        LLPointer<LLViewerFetchedTexture> imagep = mAYATeleportSweepQueue.front();
        mAYATeleportSweepQueue.pop();
        ++scanned;

        if (imagep.isNull() || imagep->isDeleted() || !imagep->isInImageList())
        {
            continue;
        }

        if (imagep->mAYATeleportSeenGeneration != mAYATeleportTextureGeneration)
        {
            continue;
        }

        if (imagep->mAYATeleportUsedGeneration == mAYATeleportTextureGeneration)
        {
            ++kept_used;
            continue;
        }

        if (imagep->getTotalNumFaces() > 0 || aya_texture_has_volume_ref(imagep))
        {
            ++kept_active;
            continue;
        }

        if (imagep->getType() != LLViewerTexture::LOD_TEXTURE ||
            imagep->getBoostLevel() != LLGLTexture::BOOST_NONE ||
            imagep->hasCallbacks() ||
            imagep->needsToSaveRawImage() ||
            imagep->isFetching() ||
            imagep->isInFastCacheList())
        {
            ++kept_special;
            continue;
        }

        if (imagep->hasSavedRawImage())
        {
            imagep->destroySavedRawImage();
            ++destroyed_raw;
        }

        if (destroy_gl_enabled() && imagep->hasGLTexture())
        {
            imagep->destroyGLTexture();
            imagep->mMaxVirtualSize = 0.f;
            ++destroyed_gl;
            continue;
        }

        if (!imagep->hasGLTexture() ||
            imagep->getDiscardLevel() < 0 ||
            imagep->getDiscardLevel() >= MAX_DISCARD_LEVEL)
        {
            ++skipped_no_gl;
            continue;
        }

        const bool was_pending = imagep->mDownScalePending;
        imagep->mMaxVirtualSize = 0.f;
        imagep->processTextureStats();
        if (!was_pending && imagep->mDownScalePending)
        {
            ++scheduled;
        }
    }

    if (sweep_log() &&
        (scheduled > 0 || mAYATeleportSweepLogTimer.getElapsedTimeF32() >= 10.f))
    {
        LL_INFOS("AYATextureCap")
            << "teleport_texture_sweep event=progress"
            << " generation=" << mAYATeleportTextureGeneration
            << " scanned=" << scanned
            << " scheduled=" << scheduled
            << " kept_used=" << kept_used
            << " kept_active=" << kept_active
            << " kept_special=" << kept_special
            << " skipped_no_gl=" << skipped_no_gl
            << " destroyed_gl=" << destroyed_gl
            << " destroyed_raw=" << destroyed_raw
            << " remaining=" << mAYATeleportSweepQueue.size()
            << " downscale_queue=" << mDownScaleQueue.size()
            << LL_ENDL;
        mAYATeleportSweepLogTimer.reset();
    }

    if (mAYATeleportSweepQueue.empty())
    {
        mAYATeleportSweepActive = false;
        if (sweep_log())
        {
            LL_INFOS("AYATextureCap")
                << "teleport_texture_sweep event=complete"
                << " generation=" << mAYATeleportTextureGeneration
                << " downscale_queue=" << mDownScaleQueue.size()
                << LL_ENDL;
        }
    }
}

void LLViewerTextureList::clearFetchingRequests()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if (LLAppViewer::getTextureFetch()->getNumRequests() == 0)
    {
        return;
    }

    LLAppViewer::getTextureFetch()->deleteAllRequests();

    for (image_list_t::iterator iter = mImageList.begin();
         iter != mImageList.end(); ++iter)
    {
        LLViewerFetchedTexture* imagep = *iter;
        imagep->forceToDeleteRequest() ;
    }
}

extern bool gCubeSnapshot;

void LLViewerTextureList::updateImageDecodePriority(LLViewerFetchedTexture* imagep, bool flush_images, bool flush_saved_raw_image)
{
    llassert(!gCubeSnapshot);

    constexpr F32 BIAS_TRS_OUT_OF_SCREEN = 1.5f;
    constexpr F32 BIAS_TRS_ON_SCREEN = 1.f;

    if (imagep->getBoostLevel() < LLViewerFetchedTexture::BOOST_HIGH)  // don't bother checking face list for boosted textures
    {
        static LLCachedControl<F32> texture_scale_min(gSavedSettings, "TextureScaleMinAreaFactor", 0.0095f);
        static LLCachedControl<F32> texture_scale_max(gSavedSettings, "TextureScaleMaxAreaFactor", 25.f);
        static LLCachedControl<bool> distance_cap_enabled(gSavedSettings, "AYATextureDistanceCapEnabled", true);
        static LLCachedControl<F32> near_distance_m(gSavedSettings, "AYATextureDistanceCapNearMeters", 10.f);
        static LLCachedControl<F32> mid_distance_m(gSavedSettings, "AYATextureDistanceCapMidMeters", 20.f);
        static LLCachedControl<F32> far_distance_m(gSavedSettings, "AYATextureDistanceCapFarMeters", 50.f);
        static LLCachedControl<U32> near_resolution_px(gSavedSettings, "AYATextureDistanceCapNearResolution", 2048);
        static LLCachedControl<U32> mid_resolution_px(gSavedSettings, "AYATextureDistanceCapMidResolution", 1024);
        static LLCachedControl<U32> far_resolution_px(gSavedSettings, "AYATextureDistanceCapFarResolution", 512);
        static LLCachedControl<U32> distant_resolution_px(gSavedSettings, "AYATextureDistanceCapDistantResolution", 256);
        static LLCachedControl<bool> aux_cap_enabled(gSavedSettings, "AYATextureDistanceCapAuxEnabled", true);
        static LLCachedControl<U32> aux_near_resolution_px(gSavedSettings, "AYATextureDistanceCapAuxNearResolution", 1024);
        static LLCachedControl<U32> aux_mid_resolution_px(gSavedSettings, "AYATextureDistanceCapAuxMidResolution", 512);
        static LLCachedControl<U32> aux_far_resolution_px(gSavedSettings, "AYATextureDistanceCapAuxFarResolution", 256);
        static LLCachedControl<U32> aux_distant_resolution_px(gSavedSettings, "AYATextureDistanceCapAuxDistantResolution", 128);
        static LLCachedControl<bool> offscreen_cap_enabled(gSavedSettings, "AYATextureOffscreenCapEnabled", true);
        static LLCachedControl<U32> offscreen_resolution_px(gSavedSettings, "AYATextureOffscreenCapResolution", 256);
        static LLCachedControl<U32> aux_offscreen_resolution_px(gSavedSettings, "AYATextureOffscreenCapAuxResolution", 128);
        static LLCachedControl<F32> texture_downscale_strength(gSavedSettings, "AYALoadingTextureDownscaleStrength", 0.f);
        static LLCachedControl<bool> distance_cap_log(gSavedSettings, "AYATextureDistanceCapLog", true);
        static LLTimer distance_cap_log_timer;
        static U32 s_cap_faces = 0;
        static U32 s_cap_clamped_faces = 0;
        static U32 s_cap_offscreen_faces = 0;
        static U32 s_cap_aux_faces = 0;
        static U32 s_cap_near_faces = 0;
        static U32 s_cap_mid_faces = 0;
        static U32 s_cap_far_faces = 0;
        static U32 s_cap_distant_faces = 0;
        static U32 s_cap_protected_owner_faces = 0;
        static U32 s_cap_hud_owner_faces = 0;

        F32 max_vsize = 0.f;
        bool on_screen = false;
        bool has_protected_owner = false;
        bool has_hud_owner = false;

        U32 face_count = 0;
        U32 max_faces_to_check = 1024;
        const bool loading_texture_management = aya_texture_management_enabled();
        const S32 downscale_strength_level = aya_texture_downscale_strength_level(texture_downscale_strength());
        const bool texture_downscale_active = loading_texture_management && downscale_strength_level > 0;
        const bool distance_cap_active = texture_downscale_active && distance_cap_enabled();
        const bool offscreen_cap_active = texture_downscale_active && offscreen_cap_enabled();
        const bool texture_owner_protection_active = texture_downscale_active;
        const bool cap_stats_log_active = texture_downscale_active && distance_cap_log();

        // get adjusted bias based on image resolution
        LLImageGL* img = imagep->getGLTexture();
        F32 max_discard = F32(img ? img->getMaxDiscardLevel() : MAX_DISCARD_LEVEL);
        F32 bias = llclamp(max_discard - 2.f, 1.f, LLViewerTexture::sDesiredDiscardBias);

        // convert bias into a vsize scaler
        bias = (F32) llroundf(powf(4, bias - 1.f));

        LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
        for (U32 i = 0; i < LLRender::NUM_TEXTURE_CHANNELS; ++i)
        {
            const bool auxiliary_channel = i != LLRender::DIFFUSE_MAP && i != LLRender::BASECOLOR_MAP;
            face_count += imagep->getNumFaces(i);
            S32 faces_to_check = (face_count > max_faces_to_check) ? 0 : imagep->getNumFaces(i);

            for (S32 fi = 0; fi < faces_to_check; ++fi)
            {
                LLFace* face = (*(imagep->getFaceList(i)))[fi];

                if (face && face->getViewerObject())
                {
                    F32 radius;
                    F32 cos_angle_to_view_dir;

                    if ((gFrameCount - face->mLastTextureUpdate) > 10)
                    { // only call calcPixelArea at most once every 10 frames for a given face
                        // this helps eliminate redundant calls to calcPixelArea for faces that have multiple textures
                        // assigned to them, such as is the case with GLTF materials or Blinn-Phong materials
                        face->mInFrustum = face->calcPixelArea(cos_angle_to_view_dir, radius);
                        face->mLastTextureUpdate = gFrameCount;
                    }

                    F32 vsize = face->getPixelArea();

                    on_screen |= face->mInFrustum;

                    // Scale desired texture resolution higher or lower depending on texture scale
                    //
                    // Minimum usage examples: a 1024x1024 texture with aplhabet (texture atlas),
                    // runing string shows one letter at a time. If texture has ten 100px symbols
                    // per side, minimal scale is (100/1024)^2 = 0.0095
                    //
                    // Maximum usage examples: huge chunk of terrain repeats texture
                    // TODO: make this work with the GLTF texture transforms
                    S32 te_offset = face->getTEOffset();  // offset is -1 if not inited
                    LLViewerObject* objp = face->getViewerObject();
                    const LLTextureEntry* te = (te_offset < 0 || te_offset >= objp->getNumTEs()) ? nullptr : objp->getTE(te_offset);
                    F32 min_scale = te ? llmin(fabsf(te->getScaleS()), fabsf(te->getScaleT())) : 1.f;
                    min_scale = llclamp(min_scale * min_scale, texture_scale_min(), texture_scale_max());
                    vsize /= min_scale;
                    const F32 distance = texture_downscale_active ?
                        (face->getPositionAgent() - LLViewerCamera::getInstance()->getOrigin()).magVec() :
                        F32_MAX;
                    LLVOAvatar* avatar_owner = texture_owner_protection_active ? aya_object_avatar_owner(objp) : nullptr;
                    const bool avatar_loading_owner =
                        texture_owner_protection_active &&
                        avatar_owner &&
                        !avatar_owner->isSelf() &&
                        (!avatar_owner->isFullyLoaded() || avatar_owner->getRezzedStatus() < 3) &&
                        (face->mInFrustum || distance <= near_distance_m());
                    const bool protected_owner =
                        texture_owner_protection_active &&
                        (aya_object_is_protected_texture_owner(objp) || avatar_loading_owner);
                    const bool hud_owner = texture_owner_protection_active && objp->isHUDAttachment();
                    has_protected_owner |= protected_owner;
                    has_hud_owner |= hud_owner;
                    if (cap_stats_log_active && protected_owner)
                    {
                        ++s_cap_protected_owner_faces;
                        if (hud_owner)
                        {
                            ++s_cap_hud_owner_faces;
                        }
                    }

                    if (hud_owner)
                    {
                        vsize = llmax(vsize, MAX_IMAGE_AREA);
                    }

                    // apply bias to offscreen faces all the time, but only to onscreen faces when bias is large
                    // use mImportanceToCamera to make bias switch a bit more gradual
                    if ((!face->mInFrustum && !protected_owner) ||
                        (!protected_owner && LLViewerTexture::sDesiredDiscardBias > 1.9f + face->mImportanceToCamera / 2.f))
                    {
                        vsize /= bias;
                    }

                    // boost resolution of textures that are important to the camera
                    if (face->mInFrustum)
                    {
                        static LLCachedControl<F32> texture_camera_boost(gSavedSettings, "TextureCameraBoost", 8.f);
                        vsize *= llmax(face->mImportanceToCamera*texture_camera_boost, 1.f);
                    }

                    if (distance_cap_active && !protected_owner)
                    {
                        U32 cap_resolution = distant_resolution_px();
                        U32* cap_bucket = &s_cap_distant_faces;
                        if (distance <= near_distance_m())
                        {
                            cap_resolution = near_resolution_px();
                            cap_bucket = &s_cap_near_faces;
                        }
                        else if (distance <= mid_distance_m())
                        {
                            cap_resolution = mid_resolution_px();
                            cap_bucket = &s_cap_mid_faces;
                        }
                        else if (distance <= far_distance_m())
                        {
                            cap_resolution = far_resolution_px();
                            cap_bucket = &s_cap_far_faces;
                        }

                        if (aux_cap_enabled() && auxiliary_channel)
                        {
                            if (distance <= near_distance_m())
                            {
                                cap_resolution = aux_near_resolution_px();
                            }
                            else if (distance <= mid_distance_m())
                            {
                                cap_resolution = aux_mid_resolution_px();
                            }
                            else if (distance <= far_distance_m())
                            {
                                cap_resolution = aux_far_resolution_px();
                            }
                            else
                            {
                                cap_resolution = aux_distant_resolution_px();
                            }
                        }

                        cap_resolution = aya_shift_texture_cap(
                            cap_resolution,
                            0,
                            1);
                        cap_resolution = aya_apply_texture_downscale_strength(cap_resolution, downscale_strength_level);
                        cap_resolution = aya_effective_texture_cap_resolution(imagep, cap_resolution);
                        const F32 cap = (F32)llclamp((S32)cap_resolution, 1, LLGLTexture::MAX_IMAGE_SIZE_DEFAULT);
                        const F32 cap_area = cap * cap;
                        if (cap_stats_log_active)
                        {
                            ++s_cap_faces;
                            ++(*cap_bucket);
                            if (auxiliary_channel)
                            {
                                ++s_cap_aux_faces;
                            }
                            if (vsize > cap_area)
                            {
                                ++s_cap_clamped_faces;
                            }
                        }
                        vsize = llmin(vsize, cap_area);
                    }

                    if (offscreen_cap_active && !protected_owner && !face->mInFrustum)
                    {
                        const U32 base_cap_resolution = auxiliary_channel ? aux_offscreen_resolution_px() : offscreen_resolution_px();
                        const U32 cap_resolution = aya_shift_texture_cap(
                            base_cap_resolution,
                            1,
                            1);
                        const U32 strengthened_cap_resolution = aya_apply_texture_downscale_strength(cap_resolution, downscale_strength_level);
                        const U32 effective_cap_resolution = aya_effective_texture_cap_resolution(imagep, strengthened_cap_resolution);
                        const F32 cap = (F32)llclamp((S32)effective_cap_resolution, 1, LLGLTexture::MAX_IMAGE_SIZE_DEFAULT);
                        const F32 cap_area = cap * cap;
                        if (cap_stats_log_active)
                        {
                            ++s_cap_faces;
                            ++s_cap_offscreen_faces;
                            if (auxiliary_channel)
                            {
                                ++s_cap_aux_faces;
                            }
                            if (vsize > cap_area)
                            {
                                ++s_cap_clamped_faces;
                            }
                        }
                        vsize = llmin(vsize, cap_area);
                    }

                    max_vsize = llmax(max_vsize, vsize);

                    // addTextureStats limits size to sMaxVirtualSize
                    if (max_vsize >= LLViewerFetchedTexture::sMaxVirtualSize
                        && (on_screen || LLViewerTexture::sDesiredDiscardBias <= BIAS_TRS_ON_SCREEN))
                    {
                        break;
                    }
                }
            }

            if (max_vsize >= LLViewerFetchedTexture::sMaxVirtualSize
                && (on_screen || LLViewerTexture::sDesiredDiscardBias <= BIAS_TRS_ON_SCREEN))
            {
                break;
            }
        }

        if (face_count > max_faces_to_check)
        { // this texture is used in so many places we should just boost it and not bother checking its vsize
            // this is especially important because the above is not time sliced and can hit multiple ms for a single texture
            max_vsize = MAX_IMAGE_AREA;
        }

        if (has_protected_owner)
        {
            on_screen = true;
        }
        if (has_hud_owner)
        {
            max_vsize = llmax(max_vsize, MAX_IMAGE_AREA);
        }

        if (mAYATeleportSweepActive &&
            face_count > 0 &&
            imagep->mAYATeleportSeenGeneration == mAYATeleportTextureGeneration)
        {
            imagep->mAYATeleportUsedGeneration = mAYATeleportTextureGeneration;
        }

        if (imagep->getType() == LLViewerTexture::LOD_TEXTURE && imagep->getBoostLevel() == LLViewerTexture::BOOST_NONE)
        { // conditionally reset max virtual size for unboosted LOD_TEXTURES
          // this is an alternative to decaying mMaxVirtualSize over time
          // that keeps textures from continously downrezzing and uprezzing in the background

            if (LLViewerTexture::sDesiredDiscardBias > BIAS_TRS_OUT_OF_SCREEN ||
                (!on_screen && LLViewerTexture::sDesiredDiscardBias > BIAS_TRS_ON_SCREEN))
            {
                imagep->mMaxVirtualSize = 0.f;
            }
        }

        imagep->addTextureStats(max_vsize);

        if (cap_stats_log_active && distance_cap_log_timer.getElapsedTimeF32() >= 10.f)
        {
            LL_INFOS("AYATextureCap")
                << "distance_cap_stats faces=" << s_cap_faces
                << " clamped_faces=" << s_cap_clamped_faces
                << " offscreen_faces=" << s_cap_offscreen_faces
                << " aux_faces=" << s_cap_aux_faces
                << " near=" << s_cap_near_faces
                << " mid=" << s_cap_mid_faces
                << " far=" << s_cap_far_faces
                << " distant=" << s_cap_distant_faces
                << " protected_owner_faces=" << s_cap_protected_owner_faces
                << " hud_owner_faces=" << s_cap_hud_owner_faces
                << " texture_management_active=" << (loading_texture_management ? "true" : "false")
                << " downscale_strength_level=" << downscale_strength_level
                << " fps=" << gFPSClamped
                << " downscale_queue=" << mDownScaleQueue.size()
                << LL_ENDL;
            s_cap_faces = 0;
            s_cap_clamped_faces = 0;
            s_cap_offscreen_faces = 0;
            s_cap_aux_faces = 0;
            s_cap_near_faces = 0;
            s_cap_mid_faces = 0;
            s_cap_far_faces = 0;
            s_cap_distant_faces = 0;
            s_cap_protected_owner_faces = 0;
            s_cap_hud_owner_faces = 0;
            distance_cap_log_timer.reset();
        }
    }

#if 0
    imagep->setDebugText(llformat("%d/%d - %d/%d -- %d/%d",
        (S32)sqrtf(max_vsize),
        (S32)sqrtf(imagep->mMaxVirtualSize),
        imagep->getDiscardLevel(),
        imagep->getDesiredDiscardLevel(),
        imagep->getWidth(),
        imagep->getFullWidth()));
#endif

    // make sure to addTextureStats for any spotlights that are using this texture
    for (S32 vi = 0; vi < imagep->getNumVolumes(LLRender::LIGHT_TEX); ++vi)
    {
        LLVOVolume* volume = (*imagep->getVolumeList(LLRender::LIGHT_TEX))[vi];
        volume->updateSpotLightPriority();
    }

    F32 max_inactive_time = 20.f; // inactive time before deleting saved raw image
    S32 min_refs = 3; // 1 for mImageList, 1 for mUUIDMap, and 1 for "entries" in updateImagesFetchTextures

    F32 lazy_flush_timeout = 30.f; // delete unused images after 30 seconds

    //
    // Flush formatted images using a lazy flush
    //
    S32 num_refs = imagep->getNumRefs();
    if (num_refs <= min_refs && flush_images)
    {
        if (imagep->getLastReferencedTimer()->getElapsedTimeF32() > lazy_flush_timeout)
        {
            // Remove the unused image from the image list
            deleteImage(imagep);
            return;
        }
    }
    else
    {
        // still referenced outside of image list, reset timer
        imagep->getLastReferencedTimer()->reset();

        if (flush_saved_raw_image &&
            imagep->hasSavedRawImage())
        {
            if (imagep->getElapsedLastReferencedSavedRawImageTime() > max_inactive_time)
            {
                imagep->destroySavedRawImage();
            }
        }

        if (imagep->isDeleted())
        {
            return;
        }
    }

    if (!imagep->isInImageList())
    {
        return;
    }
    if (imagep->isInFastCacheList())
    {
        return; //wait for loading from the fast cache.
    }

    imagep->processTextureStats();
}

F32 LLViewerTextureList::updateImagesCreateTextures(F32 max_time)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if (gGLManager.mIsDisabled) return 0.0f;
    static LLCachedControl<U32> max_create_per_frame(gSavedSettings, "RenderTextureMaxCreatesPerFrame", 6);
    static LLCachedControl<U32> max_downscale_per_frame(gSavedSettings, "RenderTextureMaxDownscalesPerFrame", 1);
    static LLCachedControl<F32> max_create_time_ms(gSavedSettings, "RenderTextureCreateTimeSliceMs", 4.f);
    static LLCachedControl<F32> max_downscale_time_ms(gSavedSettings, "RenderTextureDownscaleTimeSliceMs", 2.f);
    static LLCachedControl<U32> guard_max_create_per_frame(gSavedSettings, "RenderTextureGuardMaxCreatesPerFrame", 2);
    static LLCachedControl<U32> guard_max_downscale_per_frame(gSavedSettings, "RenderTextureGuardMaxDownscalesPerFrame", 1);
    static LLCachedControl<F32> guard_max_create_time_ms(gSavedSettings, "RenderTextureGuardCreateTimeSliceMs", 1.5f);
    static LLCachedControl<F32> guard_max_downscale_time_ms(gSavedSettings, "RenderTextureGuardDownscaleTimeSliceMs", 1.f);
    static LLCachedControl<bool> distance_cap_log(gSavedSettings, "AYATextureDistanceCapLog", true);
    static LLCachedControl<bool> backpressure_enabled(gSavedSettings, "AYATextureCreateBackpressureEnabled", true);
    static LLCachedControl<U32> backpressure_queue_limit(gSavedSettings, "AYATextureCreateBackpressureQueueLimit", 4096);
    static LLCachedControl<U32> backpressure_hard_limit(gSavedSettings, "AYATextureCreateBackpressureHardLimit", 8192);
    static LLCachedControl<U32> backpressure_max_drops_per_frame(gSavedSettings, "AYATextureCreateBackpressureMaxDropsPerFrame", 512);
    static LLCachedControl<F32> backpressure_min_unreferenced_seconds(gSavedSettings, "AYATextureCreateBackpressureMinUnreferencedSeconds", 20.f);
    static LLCachedControl<bool> backpressure_defer_distance(gSavedSettings, "AYATextureCreateBackpressureDeferDistanceEnabled", true);
    static LLCachedControl<bool> backpressure_log(gSavedSettings, "AYATextureCreateBackpressureLog", true);
    static LLCachedControl<bool> priority_enabled(gSavedSettings, "AYATextureCreatePriorityEnabled", true);
    static LLCachedControl<U32> priority_min_queue(gSavedSettings, "AYATextureCreatePriorityMinQueue", 32);
    static LLCachedControl<U32> priority_scan_limit(gSavedSettings, "AYATextureCreatePriorityScanLimit", 2048);
    static LLCachedControl<F32> priority_interval_seconds(gSavedSettings, "AYATextureCreatePriorityIntervalSeconds", 0.25f);
    static LLCachedControl<bool> priority_camera_motion_boost(gSavedSettings, "AYATextureCreatePriorityCameraMotionBoostEnabled", true);
    static LLCachedControl<F32> priority_camera_move_meters(gSavedSettings, "AYATextureCreatePriorityCameraMoveMeters", 2.f);
    static LLCachedControl<F32> priority_camera_turn_dot(gSavedSettings, "AYATextureCreatePriorityCameraTurnDot", 0.985f);
    static LLCachedControl<U32> priority_camera_scan_multiplier(gSavedSettings, "AYATextureCreatePriorityCameraMotionScanMultiplier", 3);
    static LLCachedControl<bool> priority_log(gSavedSettings, "AYATextureCreatePriorityLog", false);
    static LLCachedControl<U32> scene_min_creates_per_frame(gSavedSettings, "AYATextureCreateSceneMinCreatesPerFrame", 1);
    static LLCachedControl<U32> scene_scan_ahead(gSavedSettings, "AYATextureCreateSceneScanAhead", 128);
    static LLTimer texture_work_log_timer;
    static LLTimer create_priority_timer;
    static U32 s_created_textures = 0;
    static U32 s_redundant_creates = 0;
    static U32 s_downscaled_textures = 0;
    static U32 s_backpressure_dropped_creates = 0;
    static U32 s_backpressure_scanned_creates = 0;
    static U32 s_priority_reorders = 0;
    static U32 s_priority_created[AYA_QUEUE_BUCKET_COUNT] = {};
    static U32 s_priority_queued[AYA_QUEUE_BUCKET_COUNT] = {};
    static U32 s_priority_middle_probe_offset = 0;
    static U32 s_max_create_queue = 0;
    static U32 s_max_downscale_queue = 0;
    static U32 s_scene_reserved_creates = 0;
    static U32 s_avatar_related_creates = 0;
    static U32 s_scene_promoted_creates = 0;
    static bool s_priority_camera_initialized = false;
    static LLVector3 s_priority_camera_pos;
    static LLVector3 s_priority_camera_at;

    const bool loading_texture_management = aya_texture_management_enabled();
    static LLCachedControl<U32> texture_create_backlog_threshold(gSavedSettings, "AYALoadingDynamicResolutionTextureCreateBacklogThreshold", 24);
    const bool create_guard_budget_active =
        loading_texture_management &&
        texture_create_backlog_threshold() > 0 &&
        mCreateTextureList.size() >= texture_create_backlog_threshold();
    const bool downscale_guard_budget_active =
        loading_texture_management &&
        texture_create_backlog_threshold() > 0 &&
        mDownScaleQueue.size() >= texture_create_backlog_threshold();
    const U32 effective_max_create_per_frame = create_guard_budget_active ? guard_max_create_per_frame() : max_create_per_frame();
    const U32 configured_max_downscale_per_frame = downscale_guard_budget_active ? guard_max_downscale_per_frame() : max_downscale_per_frame();
    const U32 effective_max_downscale_per_frame = configured_max_downscale_per_frame;
    const F32 effective_max_create_time_ms = create_guard_budget_active ? guard_max_create_time_ms() : max_create_time_ms();
    const F32 configured_max_downscale_time_ms = downscale_guard_budget_active ? guard_max_downscale_time_ms() : max_downscale_time_ms();
    const F32 effective_max_downscale_time_ms = llmax(configured_max_downscale_time_ms, 0.001f);

    //
    // Create GL textures for all textures that need them (images which have been
    // decoded, but haven't been pushed into GL).
    //

    LLTimer create_timer;
    const F32 create_budget = llmin(max_time, llmax(0.001f, effective_max_create_time_ms * 0.001f));
    U32 create_count = 0;
    U32 scene_create_count = 0;

    if (loading_texture_management && backpressure_enabled() && !mCreateTextureList.empty())
    {
        const U32 queue_limit = llmax((U32)1, backpressure_queue_limit());
        const U32 hard_limit = llmax(queue_limit, backpressure_hard_limit());
        const U32 queue_size = (U32)mCreateTextureList.size();
        if (queue_size > queue_limit)
        {
            const U32 base_drop_limit = llclamp((S32)backpressure_max_drops_per_frame(), 1, 8192);
            const U32 drop_limit = queue_size > hard_limit ? llmin(base_drop_limit * 4, (U32)8192) : base_drop_limit;
            const U32 scan_limit = llmin(queue_size, llmax(drop_limit * 4, drop_limit));
            const bool allow_distance_defer = backpressure_defer_distance() && queue_size > queue_limit;
            const U32 front_scan_limit = llmin(queue_size, llmax((U32)1, scan_limit / 4));
            const U32 tail_scan_limit = llmin(queue_size - front_scan_limit, scan_limit - front_scan_limit);
            const U32 middle_keep_count = queue_size - front_scan_limit - tail_scan_limit;
            U32 scanned = 0;
            U32 dropped = 0;
            U32 dropped_inactive = 0;
            U32 dropped_distance = 0;
            U32 kept = 0;
            U32 scanned_front = 0;
            U32 scanned_tail = 0;
            U32 dropped_buckets[AYA_QUEUE_BUCKET_COUNT] = {};
            std::vector<LLPointer<LLViewerFetchedTexture> > kept_front_window;
            std::vector<LLPointer<LLViewerFetchedTexture> > kept_tail_window;
            kept_front_window.reserve(front_scan_limit);
            kept_tail_window.reserve(tail_scan_limit);
            image_queue_t middle_queue;

            auto process_backpressure_candidate =
                [&](LLPointer<LLViewerFetchedTexture> imagep,
                    std::vector<LLPointer<LLViewerFetchedTexture> >& kept_window,
                    U32& scanned_window)
                {
                    if (imagep.isNull())
                    {
                        return;
                    }

                    if (dropped >= drop_limit || queue_size - dropped <= queue_limit)
                    {
                        kept_window.push_back(imagep);
                        ++kept;
                        return;
                    }

                    ++scanned;
                    ++scanned_window;

                    AYAPendingCreateRelease release_reason = AYA_PENDING_CREATE_KEEP;
                    AYAQueuedTextureBucket release_bucket = AYA_QUEUE_BUCKET_PROTECTED;
                    if (aya_texture_can_release_pending_create(
                            imagep.get(),
                            llmax(0.f, backpressure_min_unreferenced_seconds()),
                            allow_distance_defer,
                            &release_reason,
                            &release_bucket))
                    {
                        imagep->cancelCreateTexture();
                        ++dropped;
                        if (release_reason == AYA_PENDING_CREATE_RELEASE_DISTANCE)
                        {
                            ++dropped_distance;
                        }
                        else
                        {
                            ++dropped_inactive;
                        }
                        if (release_bucket >= 0 && release_bucket < AYA_QUEUE_BUCKET_COUNT)
                        {
                            ++dropped_buckets[release_bucket];
                        }
                        return;
                    }

                    kept_window.push_back(imagep);
                    ++kept;
                };

            for (U32 i = 0; !mCreateTextureList.empty() && i < front_scan_limit; ++i)
            {
                LLPointer<LLViewerFetchedTexture> imagep = mCreateTextureList.front();
                mCreateTextureList.pop();
                process_backpressure_candidate(imagep, kept_front_window, scanned_front);
            }

            for (U32 i = 0; !mCreateTextureList.empty() && i < middle_keep_count; ++i)
            {
                LLPointer<LLViewerFetchedTexture> imagep = mCreateTextureList.front();
                mCreateTextureList.pop();
                if (imagep.notNull())
                {
                    middle_queue.push(imagep);
                }
            }

            for (U32 i = 0; !mCreateTextureList.empty() && i < tail_scan_limit; ++i)
            {
                LLPointer<LLViewerFetchedTexture> imagep = mCreateTextureList.front();
                mCreateTextureList.pop();
                process_backpressure_candidate(imagep, kept_tail_window, scanned_tail);
            }

            image_queue_t rebuilt_queue;
            for (std::vector<LLPointer<LLViewerFetchedTexture> >::const_iterator iter = kept_front_window.begin();
                 iter != kept_front_window.end(); ++iter)
            {
                rebuilt_queue.push(*iter);
            }
            while (!middle_queue.empty())
            {
                rebuilt_queue.push(middle_queue.front());
                middle_queue.pop();
            }
            for (std::vector<LLPointer<LLViewerFetchedTexture> >::const_iterator iter = kept_tail_window.begin();
                 iter != kept_tail_window.end(); ++iter)
            {
                rebuilt_queue.push(*iter);
            }
            while (!mCreateTextureList.empty())
            {
                rebuilt_queue.push(mCreateTextureList.front());
                mCreateTextureList.pop();
            }
            mCreateTextureList.swap(rebuilt_queue);

            s_backpressure_dropped_creates += dropped;
            s_backpressure_scanned_creates += scanned;

            if (backpressure_log() && dropped > 0)
            {
                LL_INFOS("AYATextureCap")
                    << "create_backpressure event=drop_pending"
                    << " queue_before=" << queue_size
                    << " queue_after=" << mCreateTextureList.size()
                    << " scanned=" << scanned
                    << " scanned_front=" << scanned_front
                    << " scanned_tail=" << scanned_tail
                    << " dropped=" << dropped
                    << " dropped_inactive=" << dropped_inactive
                    << " dropped_distance=" << dropped_distance
                    << " dropped_far=" << dropped_buckets[AYA_QUEUE_BUCKET_FAR]
                    << " dropped_offscreen=" << dropped_buckets[AYA_QUEUE_BUCKET_OFFSCREEN]
                    << " dropped_unreferenced=" << dropped_buckets[AYA_QUEUE_BUCKET_UNREFERENCED]
                    << " allow_distance_defer=" << (allow_distance_defer ? "true" : "false")
                    << " kept=" << kept
                    << " fps=" << gFPSClamped
                    << LL_ENDL;
            }
        }
    }

    LLViewerCamera* priority_camera = nullptr;
    LLVector3 priority_camera_pos;
    LLVector3 priority_camera_at;
    bool priority_camera_motion = false;
    bool priority_camera_valid = false;
    const bool priority_reorder_candidate =
        loading_texture_management &&
        priority_enabled() &&
        mCreateTextureList.size() >= llmax((U32)1, priority_min_queue());
    if (priority_reorder_candidate)
    {
        priority_camera = LLViewerCamera::getInstance();
    }
    if (priority_camera && priority_camera_motion_boost())
    {
        priority_camera_pos = priority_camera->getOrigin();
        priority_camera_at = priority_camera->getAtAxis();
        priority_camera_valid = true;
        if (s_priority_camera_initialized)
        {
            const F32 move_meters = (priority_camera_pos - s_priority_camera_pos).magVec();
            const F32 turn_dot = priority_camera_at * s_priority_camera_at;
            priority_camera_motion =
                move_meters >= llmax(0.f, priority_camera_move_meters()) ||
                turn_dot <= llclamp(priority_camera_turn_dot(), -1.f, 1.f);
        }
        else
        {
            s_priority_camera_initialized = true;
            s_priority_camera_pos = priority_camera_pos;
            s_priority_camera_at = priority_camera_at;
        }
    }

    if (priority_reorder_candidate &&
        priority_camera &&
        (priority_camera_motion ||
         create_priority_timer.getElapsedTimeF32() >= llmax(0.05f, priority_interval_seconds())))
    {
        struct AYAQueuedCreateTexture
        {
            LLPointer<LLViewerFetchedTexture> mImage;
            S32 mScore = 0;
            U32 mSequence = 0;
            AYAQueuedTextureBucket mBucket = AYA_QUEUE_BUCKET_UNREFERENCED;
            U32 mSource = 0;
        };

        const U32 queue_size_before = (U32)mCreateTextureList.size();
        const U32 scan_multiplier = priority_camera_motion ?
            llclamp((S32)priority_camera_scan_multiplier(), 1, 8) :
            1;
        const U32 scan_limit = llclamp(
            (S32)(llmax((U32)1, priority_scan_limit()) * scan_multiplier),
            1,
            8192);
        const U32 scan_count = llmin(queue_size_before, scan_limit);
        const U32 front_scan_count = queue_size_before <= scan_count ? scan_count : llmax((U32)1, scan_count / 3);
        const U32 remaining_scan_count = scan_count - front_scan_count;
        const U32 tail_scan_count = queue_size_before <= scan_count ? 0 : (remaining_scan_count > 1 ? llmax((U32)1, remaining_scan_count / 2) : remaining_scan_count);
        const U32 available_middle_count = queue_size_before - front_scan_count - tail_scan_count;
        const U32 middle_scan_count = queue_size_before <= scan_count ? 0 : llmin(available_middle_count, scan_count - front_scan_count - tail_scan_count);
        const U32 middle_probe_span = available_middle_count > middle_scan_count ? available_middle_count - middle_scan_count + 1 : 1;
        const U32 middle_before_count = middle_scan_count > 0 ? s_priority_middle_probe_offset % middle_probe_span : available_middle_count;
        const U32 middle_after_count = available_middle_count - middle_before_count - middle_scan_count;
        enum
        {
            AYA_QUEUE_SOURCE_FRONT = 0,
            AYA_QUEUE_SOURCE_MIDDLE = 1,
            AYA_QUEUE_SOURCE_TAIL = 2
        };
        std::vector<AYAQueuedCreateTexture> queued;
        queued.reserve(scan_count);
        image_queue_t middle_before_queue;
        image_queue_t middle_after_queue;
        image_queue_t middle_deferred_queue;
        image_queue_t tail_deferred_queue;
        U32 sequence = 0;
        U32 bucket_counts[AYA_QUEUE_BUCKET_COUNT] = {};

        for (U32 i = 0; !mCreateTextureList.empty() && i < front_scan_count; ++i)
        {
            LLPointer<LLViewerFetchedTexture> imagep = mCreateTextureList.front();
            mCreateTextureList.pop();

            if (imagep.isNull() || !imagep->mCreatePending)
            {
                continue;
            }

            AYAQueuedCreateTexture item;
            item.mImage = imagep;
            item.mSequence = sequence++;
            item.mScore = aya_texture_create_priority_score(imagep.get(), &item.mBucket);
            item.mSource = AYA_QUEUE_SOURCE_FRONT;
            queued.push_back(item);
            ++bucket_counts[item.mBucket];
        }

        for (U32 i = 0; !mCreateTextureList.empty() && i < middle_before_count; ++i)
        {
            LLPointer<LLViewerFetchedTexture> imagep = mCreateTextureList.front();
            mCreateTextureList.pop();
            if (imagep.notNull() && imagep->mCreatePending)
            {
                middle_before_queue.push(imagep);
            }
        }

        for (U32 i = 0; !mCreateTextureList.empty() && i < middle_scan_count; ++i)
        {
            LLPointer<LLViewerFetchedTexture> imagep = mCreateTextureList.front();
            mCreateTextureList.pop();

            if (imagep.isNull() || !imagep->mCreatePending)
            {
                continue;
            }

            AYAQueuedCreateTexture item;
            item.mImage = imagep;
            item.mSequence = sequence++;
            item.mScore = aya_texture_create_priority_score(imagep.get(), &item.mBucket);
            item.mSource = AYA_QUEUE_SOURCE_MIDDLE;
            queued.push_back(item);
            ++bucket_counts[item.mBucket];
        }

        for (U32 i = 0; !mCreateTextureList.empty() && i < middle_after_count; ++i)
        {
            LLPointer<LLViewerFetchedTexture> imagep = mCreateTextureList.front();
            mCreateTextureList.pop();
            if (imagep.notNull() && imagep->mCreatePending)
            {
                middle_after_queue.push(imagep);
            }
        }

        for (U32 i = 0; !mCreateTextureList.empty() && i < tail_scan_count; ++i)
        {
            LLPointer<LLViewerFetchedTexture> imagep = mCreateTextureList.front();
            mCreateTextureList.pop();

            if (imagep.isNull() || !imagep->mCreatePending)
            {
                continue;
            }

            AYAQueuedCreateTexture item;
            item.mImage = imagep;
            item.mSequence = sequence++;
            item.mScore = aya_texture_create_priority_score(imagep.get(), &item.mBucket);
            item.mSource = AYA_QUEUE_SOURCE_TAIL;
            queued.push_back(item);
            ++bucket_counts[item.mBucket];
        }

        if (middle_scan_count > 0)
        {
            s_priority_middle_probe_offset = (middle_before_count + middle_scan_count) % middle_probe_span;
        }

        std::stable_sort(
            queued.begin(),
            queued.end(),
            [](const AYAQueuedCreateTexture& lhs, const AYAQueuedCreateTexture& rhs)
            {
                if (lhs.mScore != rhs.mScore)
                {
                    return lhs.mScore > rhs.mScore;
                }
                return lhs.mSequence < rhs.mSequence;
            });

        image_queue_t reordered_queue;
        for (std::vector<AYAQueuedCreateTexture>::const_iterator iter = queued.begin();
             iter != queued.end(); ++iter)
        {
            const bool promote_scanned =
                iter->mSource == AYA_QUEUE_SOURCE_FRONT ||
                iter->mBucket == AYA_QUEUE_BUCKET_NEAR ||
                iter->mBucket == AYA_QUEUE_BUCKET_MID ||
                iter->mBucket == AYA_QUEUE_BUCKET_PROTECTED;
            if (promote_scanned)
            {
                reordered_queue.push(iter->mImage);
            }
            else if (iter->mSource == AYA_QUEUE_SOURCE_MIDDLE)
            {
                middle_deferred_queue.push(iter->mImage);
            }
            else
            {
                tail_deferred_queue.push(iter->mImage);
            }
        }
        while (!middle_before_queue.empty())
        {
            reordered_queue.push(middle_before_queue.front());
            middle_before_queue.pop();
        }
        while (!middle_deferred_queue.empty())
        {
            reordered_queue.push(middle_deferred_queue.front());
            middle_deferred_queue.pop();
        }
        while (!middle_after_queue.empty())
        {
            reordered_queue.push(middle_after_queue.front());
            middle_after_queue.pop();
        }
        while (!tail_deferred_queue.empty())
        {
            reordered_queue.push(tail_deferred_queue.front());
            tail_deferred_queue.pop();
        }
        while (!mCreateTextureList.empty())
        {
            reordered_queue.push(mCreateTextureList.front());
            mCreateTextureList.pop();
        }
        mCreateTextureList.swap(reordered_queue);

        ++s_priority_reorders;
        for (U32 i = 0; i < AYA_QUEUE_BUCKET_COUNT; ++i)
        {
            s_priority_queued[i] += bucket_counts[i];
        }

        if (priority_log())
        {
            LL_INFOS("AYATextureQueue")
                    << "priority_stats event=reorder"
                    << " queue=" << queue_size_before
                    << " scanned=" << queued.size()
                    << " front_scanned=" << front_scan_count
                    << " middle_scanned=" << middle_scan_count
                    << " tail_scanned=" << tail_scan_count
                    << " camera_motion=" << (priority_camera_motion ? "true" : "false")
                    << " scan_limit=" << scan_limit
                    << " near=" << bucket_counts[AYA_QUEUE_BUCKET_NEAR]
                << " mid=" << bucket_counts[AYA_QUEUE_BUCKET_MID]
                << " far=" << bucket_counts[AYA_QUEUE_BUCKET_FAR]
                << " offscreen=" << bucket_counts[AYA_QUEUE_BUCKET_OFFSCREEN]
                << " unreferenced=" << bucket_counts[AYA_QUEUE_BUCKET_UNREFERENCED]
                << " protected=" << bucket_counts[AYA_QUEUE_BUCKET_PROTECTED]
                << LL_ENDL;
        }

        create_priority_timer.reset();
        if (priority_camera_valid)
        {
            s_priority_camera_initialized = true;
            s_priority_camera_pos = priority_camera_pos;
            s_priority_camera_at = priority_camera_at;
        }
    }

    auto pop_create_candidate =
        [&](AYAQueuedTextureBucket& create_bucket, bool& scene_candidate, bool& avatar_candidate)
            -> LLPointer<LLViewerFetchedTexture>
        {
            create_bucket = AYA_QUEUE_BUCKET_UNREFERENCED;
            scene_candidate = false;
            avatar_candidate = false;

            const U32 scene_min = loading_texture_management && effective_max_create_per_frame > 1 ?
                llmin(scene_min_creates_per_frame(), effective_max_create_per_frame - 1) :
                0;
            const U32 remaining_slots = effective_max_create_per_frame > create_count ?
                effective_max_create_per_frame - create_count :
                0;
            const U32 scene_needed = scene_min > scene_create_count ? scene_min - scene_create_count : 0;
            const bool must_find_scene = scene_needed > 0 && remaining_slots <= scene_needed;
            if (!must_find_scene)
            {
                LLPointer<LLViewerFetchedTexture> imagep = mCreateTextureList.front();
                mCreateTextureList.pop();
                if (imagep.notNull())
                {
                    aya_texture_create_priority_score(imagep.get(), &create_bucket);
                    avatar_candidate = aya_texture_is_avatar_related_create_candidate(imagep.get());
                    scene_candidate = !avatar_candidate && create_bucket != AYA_QUEUE_BUCKET_UNREFERENCED;
                }
                return imagep;
            }

            image_queue_t deferred_queue;
            LLPointer<LLViewerFetchedTexture> selected;
            AYAQueuedTextureBucket selected_bucket = AYA_QUEUE_BUCKET_UNREFERENCED;
            const U32 scan_limit = llmin((U32)mCreateTextureList.size(), llclamp((S32)scene_scan_ahead(), 1, 1024));
            for (U32 i = 0; !mCreateTextureList.empty() && i < scan_limit; ++i)
            {
                LLPointer<LLViewerFetchedTexture> imagep = mCreateTextureList.front();
                mCreateTextureList.pop();
                AYAQueuedTextureBucket bucket = AYA_QUEUE_BUCKET_UNREFERENCED;
                if (imagep.notNull())
                {
                    aya_texture_create_priority_score(imagep.get(), &bucket);
                }
                const bool is_avatar = imagep.notNull() && aya_texture_is_avatar_related_create_candidate(imagep.get());
                const bool is_scene = !is_avatar && bucket != AYA_QUEUE_BUCKET_UNREFERENCED;
                if (imagep.notNull() && imagep->mCreatePending && is_scene)
                {
                    selected = imagep;
                    selected_bucket = bucket;
                    scene_candidate = true;
                    avatar_candidate = false;
                    ++s_scene_promoted_creates;
                    break;
                }
                deferred_queue.push(imagep);
            }

            image_queue_t rebuilt_queue;
            while (!deferred_queue.empty())
            {
                rebuilt_queue.push(deferred_queue.front());
                deferred_queue.pop();
            }
            while (!mCreateTextureList.empty())
            {
                rebuilt_queue.push(mCreateTextureList.front());
                mCreateTextureList.pop();
            }
            mCreateTextureList.swap(rebuilt_queue);

            if (selected.notNull())
            {
                create_bucket = selected_bucket;
                return selected;
            }

            if (mCreateTextureList.empty())
            {
                return LLPointer<LLViewerFetchedTexture>();
            }

            LLPointer<LLViewerFetchedTexture> imagep = mCreateTextureList.front();
            mCreateTextureList.pop();
            if (imagep.notNull())
            {
                aya_texture_create_priority_score(imagep.get(), &create_bucket);
                avatar_candidate = aya_texture_is_avatar_related_create_candidate(imagep.get());
                scene_candidate = !avatar_candidate && create_bucket != AYA_QUEUE_BUCKET_UNREFERENCED;
            }
            return imagep;
        };

    while (!mCreateTextureList.empty())
    {
        s_max_create_queue = llmax(s_max_create_queue, (U32)mCreateTextureList.size());
        if (create_count >= effective_max_create_per_frame ||
            (create_count > 0 && create_timer.getElapsedTimeF32() > create_budget))
        {
            break;
        }

        bool scene_candidate = false;
        bool avatar_candidate = false;
        AYAQueuedTextureBucket create_bucket = AYA_QUEUE_BUCKET_UNREFERENCED;
        LLPointer<LLViewerFetchedTexture> imagep = pop_create_candidate(create_bucket, scene_candidate, avatar_candidate);
        if (imagep.isNull() || !imagep->mCreatePending)
        {
            continue;
        }
        llassert(imagep->mCreatePending);

        // desired discard may change while an image is being decoded. If the texture in VRAM is sufficient
        // for the current desired discard level, skip the texture creation.  This happens more often than it probably
        // should
        bool redundant_load = imagep->hasGLTexture() && imagep->getDiscardLevel() <= imagep->getDesiredDiscardLevel();

        if (!redundant_load)
        {
           imagep->createTexture();
           ++s_created_textures;
        }
        else
        {
            ++s_redundant_creates;
        }

        imagep->postCreateTexture();
        imagep->mCreatePending = false;
        ++s_priority_created[create_bucket];
        if (scene_candidate)
        {
            ++scene_create_count;
            ++s_scene_reserved_creates;
        }
        if (avatar_candidate)
        {
            ++s_avatar_related_creates;
        }

        if (imagep->hasGLTexture() && imagep->getDiscardLevel() < imagep->getDesiredDiscardLevel() &&
            imagep->getDesiredDiscardLevel() <= MAX_DISCARD_LEVEL &&
            !aya_texture_protected_fttype(imagep.get()) &&
            !aya_texture_protected_boost(imagep.get()) &&
            !imagep->hasCallbacks() &&
            !imagep->needsToSaveRawImage())
        {
            // NOTE: this may happen if the desired discard reduces while a decode is in progress and does not
            // necessarily indicate a problem, but if log occurrences excede that of dsiplay_stats: FPS,
            // something has probably gone wrong.
            LL_WARNS_ONCE("Texture") << "Texture will be downscaled immediately after loading." << LL_ENDL;
            imagep->scaleDown();
        }

        ++create_count;

        if (create_timer.getElapsedTimeF32() > create_budget)
        {
            break;
        }
    }

    U32 downscale_count = 0;
    if (!mDownScaleQueue.empty() && gPipeline.mDownResMap.isComplete())
    {
        LLGLDisable blend(GL_BLEND);
        gGL.setColorMask(true, true);

        // just in case we downres textures, bind downresmap and copy program
        gPipeline.mDownResMap.bindTarget();
        gCopyProgram.bind();
        gPipeline.mScreenTriangleVB->setBuffer();

        // give time to downscaling first -- if mDownScaleQueue is not empty, we're running out of memory and need
        // to free up memory by discarding off screen textures quickly

        // do at least 5 and make sure we don't get too far behind even if it violates
        // the time limit.  If we don't downscale quickly the viewer will hit swap and may
        // freeze.
        S32 min_count = llclamp((S32)effective_max_downscale_per_frame, 0, 16);
        const F32 downscale_budget = llmax(0.001f, effective_max_downscale_time_ms * 0.001f);

        create_timer.reset();
        while (!mDownScaleQueue.empty())
        {
            s_max_downscale_queue = llmax(s_max_downscale_queue, (U32)mDownScaleQueue.size());
            if (downscale_count >= effective_max_downscale_per_frame ||
                (downscale_count > 0 && create_timer.getElapsedTimeF32() > downscale_budget))
            {
                break;
            }

            LLPointer<LLViewerFetchedTexture> image = mDownScaleQueue.front();
            if (image.isNull())
            {
                mDownScaleQueue.pop();
                continue;
            }
            llassert(image->mDownScalePending);

            LLImageGL* img = image->getGLTexture();
            if (img &&
                img->getHasGLTexture() &&
                !aya_texture_protected_fttype(image.get()) &&
                !aya_texture_protected_boost(image.get()))
            {
                img->scaleDown(image->getDesiredDiscardLevel());
            }

            image->mDownScalePending = false;
            mDownScaleQueue.pop();
            ++downscale_count;
            ++s_downscaled_textures;

            if (create_timer.getElapsedTimeF32() > downscale_budget && --min_count <= 0)
            {
                break;
            }
        }

        gCopyProgram.unbind();
        gPipeline.mDownResMap.flush();
    }

    if (distance_cap_log() && texture_work_log_timer.getElapsedTimeF32() >= 10.f)
    {
        LL_INFOS("AYATextureCap")
            << "texture_work_stats created=" << s_created_textures
            << " redundant_creates=" << s_redundant_creates
            << " downscaled=" << s_downscaled_textures
            << " backpressure_dropped=" << s_backpressure_dropped_creates
            << " backpressure_scanned=" << s_backpressure_scanned_creates
            << " priority_reorders=" << s_priority_reorders
            << " created_near=" << s_priority_created[AYA_QUEUE_BUCKET_NEAR]
            << " created_mid=" << s_priority_created[AYA_QUEUE_BUCKET_MID]
            << " created_far=" << s_priority_created[AYA_QUEUE_BUCKET_FAR]
            << " created_offscreen=" << s_priority_created[AYA_QUEUE_BUCKET_OFFSCREEN]
            << " created_unreferenced=" << s_priority_created[AYA_QUEUE_BUCKET_UNREFERENCED]
            << " created_protected=" << s_priority_created[AYA_QUEUE_BUCKET_PROTECTED]
            << " created_scene_reserved=" << s_scene_reserved_creates
            << " created_avatar_related=" << s_avatar_related_creates
            << " scene_promoted=" << s_scene_promoted_creates
            << " queued_near=" << s_priority_queued[AYA_QUEUE_BUCKET_NEAR]
            << " queued_mid=" << s_priority_queued[AYA_QUEUE_BUCKET_MID]
            << " queued_far=" << s_priority_queued[AYA_QUEUE_BUCKET_FAR]
            << " queued_offscreen=" << s_priority_queued[AYA_QUEUE_BUCKET_OFFSCREEN]
            << " queued_unreferenced=" << s_priority_queued[AYA_QUEUE_BUCKET_UNREFERENCED]
            << " queued_protected=" << s_priority_queued[AYA_QUEUE_BUCKET_PROTECTED]
            << " create_queue_max=" << s_max_create_queue
            << " downscale_queue_max=" << s_max_downscale_queue
            << " create_queue_now=" << mCreateTextureList.size()
            << " downscale_queue_now=" << mDownScaleQueue.size()
            << " texture_management=" << (loading_texture_management ? "true" : "false")
            << " downscale_limit=" << effective_max_downscale_per_frame
            << " downscale_time_ms=" << effective_max_downscale_time_ms
            << " fps=" << gFPSClamped
            << LL_ENDL;
        s_created_textures = 0;
        s_redundant_creates = 0;
        s_downscaled_textures = 0;
        s_backpressure_dropped_creates = 0;
        s_backpressure_scanned_creates = 0;
        s_priority_reorders = 0;
        for (U32 i = 0; i < AYA_QUEUE_BUCKET_COUNT; ++i)
        {
            s_priority_created[i] = 0;
            s_priority_queued[i] = 0;
        }
        s_max_create_queue = 0;
        s_max_downscale_queue = 0;
        s_scene_reserved_creates = 0;
        s_avatar_related_creates = 0;
        s_scene_promoted_creates = 0;
        texture_work_log_timer.reset();
    }

    return create_timer.getElapsedTimeF32();
}

F32 LLViewerTextureList::updateImagesLoadingFastCache(F32 max_time)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if (gGLManager.mIsDisabled) return 0.0f;
    if(mFastCacheList.empty())
    {
        return 0.f;
    }

    //
    // loading texture raw data from the fast cache directly.
    //

    LLTimer timer;
    image_list_t::iterator enditer = mFastCacheList.begin();
    for (image_list_t::iterator iter = mFastCacheList.begin();
         iter != mFastCacheList.end();)
    {
        image_list_t::iterator curiter = iter++;
        enditer = iter;
        LLViewerFetchedTexture *imagep = *curiter;
        imagep->loadFromFastCache();
        // <FS:Ansariel> Fast cache stats
        sNumFastCacheReads++;
        // </FS:Ansariel>
        if (timer.getElapsedTimeF32() > max_time)
            break;
    }
    mFastCacheList.erase(mFastCacheList.begin(), enditer);
    return timer.getElapsedTimeF32();
}

void LLViewerTextureList::forceImmediateUpdate(LLViewerFetchedTexture* imagep)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if(!imagep || gCubeSnapshot)
    {
        return ;
    }

    imagep->processTextureStats();

    return ;
}

F32 LLViewerTextureList::updateImagesFetchTextures(F32 max_time)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    typedef std::vector<LLPointer<LLViewerFetchedTexture> > entries_list_t;
    entries_list_t entries;
    static LLCachedControl<F32> texture_downscale_strength(gSavedSettings, "AYALoadingTextureDownscaleStrength", 0.f);
    static LLCachedControl<bool> distance_cap_enabled(gSavedSettings, "AYATextureDistanceCapEnabled", true);
    static LLCachedControl<bool> offscreen_cap_enabled(gSavedSettings, "AYATextureOffscreenCapEnabled", true);
    static LLCachedControl<bool> memory_sweep_enabled(gSavedSettings, "AYATextureMemorySweepEnabled", true);
    static LLCachedControl<U32> memory_sweep_scan_per_frame(gSavedSettings, "AYATextureMemorySweepScanPerFrame", 256);
    static LLCachedControl<U32> memory_sweep_destroy_gl_per_frame(gSavedSettings, "AYATextureMemorySweepDestroyGLPerFrame", 16);
    static LLCachedControl<U32> memory_sweep_destroy_raw_per_frame(gSavedSettings, "AYATextureMemorySweepDestroyRawPerFrame", 64);
    static LLCachedControl<U32> memory_sweep_scale_down_per_frame(gSavedSettings, "AYATextureMemorySweepScaleDownPerFrame", 32);
    static LLCachedControl<bool> memory_sweep_destroy_far_offscreen_gl(gSavedSettings, "AYATextureMemorySweepDestroyFarOffscreenGLEnabled", false);
    static LLCachedControl<F32> memory_sweep_destroy_far_offscreen_min_seconds(gSavedSettings, "AYATextureMemorySweepDestroyFarOffscreenMinSeconds", 12.f);
    static LLCachedControl<F32> memory_sweep_min_unreferenced_seconds(gSavedSettings, "AYATextureMemorySweepMinUnreferencedSeconds", 8.f);
    static LLCachedControl<U32> memory_sweep_min_physical_mb(gSavedSettings, "AYATextureMemorySweepMinPhysicalMB", 7168);
    static LLCachedControl<bool> memory_sweep_log(gSavedSettings, "AYATextureMemorySweepLog", true);
    static LLTextureKey s_memory_sweep_last_key;
    static LLTimer s_memory_sweep_log_timer;
    static U32 s_memory_sweep_scanned = 0;
    static U32 s_memory_sweep_destroyed_gl = 0;
    static U32 s_memory_sweep_destroyed_raw = 0;
    static U32 s_memory_sweep_scaled_down = 0;
    static U32 s_memory_sweep_skipped = 0;
    static U32 s_memory_sweep_unreferenced = 0;
    static U32 s_memory_sweep_offscreen = 0;
    static U32 s_memory_sweep_far = 0;
    static U32 s_memory_sweep_cap_candidates = 0;
    static S32 s_last_downscale_strength_level = -1;
    static bool s_last_texture_management_active = false;
    static bool s_force_strength_sweep = false;
    static U32 s_force_strength_sweep_remaining = 0;
    static U32 s_force_strength_sweep_retry_frames = 0;
    static std::set<LLTextureKey> s_force_strength_sweep_retry_keys;

    // update N textures at beginning of mImageList
    U32 update_count = 0;
    static const S32 MIN_UPDATE_COUNT = gSavedSettings.getS32("TextureFetchUpdateMinCount");       // default: 32

    // NOTE:  a texture may be deleted as a side effect of some of these updates
    // Deletion rules check ref count, so be careful not to hold any LLPointer references to the textures here other than the one in entries.

    //update MIN_UPDATE_COUNT or 5% of other textures, whichever is greater
    update_count = llmax((U32) MIN_UPDATE_COUNT, (U32) mUUIDMap.size()/20);
    if (LLViewerTexture::sDesiredDiscardBias > 1.f
        && LLViewerTexture::sBiasTexturesUpdated < (U32)mUUIDMap.size())
    {
        // We are over memory target. Bias affects discard rates, so update
        // existing textures agresively to free memory faster.
        update_count = (S32)(update_count * LLViewerTexture::sDesiredDiscardBias);

        // This isn't particularly precise and can overshoot, but it doesn't need
        // to be, just making sure it did a full circle and doesn't get stuck updating
        // at bias = 4 with 4 times the rate permanently.
        LLViewerTexture::sBiasTexturesUpdated += update_count;
    }
    update_count = llmin(update_count, (U32) mUUIDMap.size());

    { // copy entries out of UUID map to avoid iterator invalidation from deletion inside updateImageDecodeProiroty or updateFetch below
        LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("vtluift - copy");

        // copy entries out of UUID map for updating
        entries.reserve(update_count);
        uuid_map_t::iterator iter = mUUIDMap.upper_bound(mLastUpdateKey);
        while (update_count-- > 0)
        {
            if (iter == mUUIDMap.end())
            {
                iter = mUUIDMap.begin();
            }

            if (iter->second->getGLTexture())
            {
                entries.push_back(iter->second);
            }
            ++iter;
        }
    }

    LLTimer timer;

    for (auto& imagep : entries)
    {
        mLastUpdateKey = LLTextureKey(imagep->getID(), (ETexListType)imagep->getTextureListType());

        if (imagep->getNumRefs() > 1) // make sure this image hasn't been deleted before attempting to update (may happen as a side effect of some other image updating)
        {
            updateImageDecodePriority(imagep);
            imagep->updateFetch();
        }

        if (timer.getElapsedTimeF32() > max_time)
        {
            break;
        }
    }

    const U32 physical_mb = (U32)(LLMemory::getCurrentRSS() / (1024ULL * 1024ULL));
    const bool memory_pressure =
        memory_sweep_min_physical_mb() > 0 &&
        physical_mb >= memory_sweep_min_physical_mb();
    const S32 downscale_strength_level = aya_texture_downscale_strength_level(texture_downscale_strength());
    const bool texture_management_active =
        aya_texture_management_enabled() &&
        memory_sweep_enabled();
    if (!texture_management_active)
    {
        s_force_strength_sweep = false;
        s_force_strength_sweep_remaining = 0;
        s_force_strength_sweep_retry_frames = 0;
        s_force_strength_sweep_retry_keys.clear();
    }
    const bool texture_management_rising =
        texture_management_active && !s_last_texture_management_active;
    s_last_texture_management_active = texture_management_active;
    if (s_last_downscale_strength_level < 0)
    {
        if (texture_management_active &&
            downscale_strength_level > 0)
        {
            s_force_strength_sweep = true;
            s_force_strength_sweep_remaining = (U32)mUUIDMap.size();
            s_force_strength_sweep_retry_frames = 0;
            s_force_strength_sweep_retry_keys.clear();
            LL_INFOS("AYATextureCap")
                << "memory_sweep event=strength_initial"
                << " level=" << downscale_strength_level
                << " sweep_remaining=" << s_force_strength_sweep_remaining
                << LL_ENDL;
        }
        s_last_downscale_strength_level = downscale_strength_level;
    }
    else if (texture_management_rising &&
             downscale_strength_level > 0)
    {
        s_force_strength_sweep = true;
        s_force_strength_sweep_remaining = (U32)mUUIDMap.size();
        s_force_strength_sweep_retry_frames = 0;
        s_force_strength_sweep_retry_keys.clear();
        LL_INFOS("AYATextureCap")
            << "memory_sweep event=strength_enable"
            << " level=" << downscale_strength_level
            << " sweep_remaining=" << s_force_strength_sweep_remaining
            << LL_ENDL;
        s_last_downscale_strength_level = downscale_strength_level;
    }
    else if (texture_management_active &&
             downscale_strength_level > s_last_downscale_strength_level)
    {
        s_force_strength_sweep = true;
        s_force_strength_sweep_remaining = (U32)mUUIDMap.size();
        s_force_strength_sweep_retry_frames = 0;
        s_force_strength_sweep_retry_keys.clear();
        LL_INFOS("AYATextureCap")
            << "memory_sweep event=strength_increase"
            << " from=" << s_last_downscale_strength_level
            << " to=" << downscale_strength_level
            << " sweep_remaining=" << s_force_strength_sweep_remaining
            << LL_ENDL;
        s_last_downscale_strength_level = downscale_strength_level;
    }
    else if (downscale_strength_level != s_last_downscale_strength_level)
    {
        s_last_downscale_strength_level = downscale_strength_level;
    }
    const bool cap_kill_switch_active = distance_cap_enabled() || offscreen_cap_enabled();
    if (downscale_strength_level <= 0 ||
        !cap_kill_switch_active ||
        (s_force_strength_sweep_remaining == 0 && s_force_strength_sweep_retry_keys.empty()))
    {
        s_force_strength_sweep = false;
        s_force_strength_sweep_remaining = 0;
        s_force_strength_sweep_retry_frames = 0;
        s_force_strength_sweep_retry_keys.clear();
    }
    const bool strength_change_pressure =
        s_force_strength_sweep &&
        downscale_strength_level > 0 &&
        cap_kill_switch_active;
    const bool cap_enforcement_pressure =
        downscale_strength_level > 0 &&
        cap_kill_switch_active &&
        (memory_pressure || strength_change_pressure);
    const bool run_memory_sweep =
        aya_texture_management_enabled() &&
        memory_sweep_enabled() &&
        (memory_pressure || strength_change_pressure || cap_enforcement_pressure) &&
        !mUUIDMap.empty();

    if (run_memory_sweep)
    {
        const U32 force_multiplier = strength_change_pressure ? (U32)llclamp(downscale_strength_level, 1, 5) : 1;
        const U32 scan_budget = strength_change_pressure ?
            llclamp((S32)llmax(memory_sweep_scan_per_frame(), 512 * force_multiplier), 1, 8192) :
            llclamp((S32)memory_sweep_scan_per_frame(), 1, 8192);
        const U32 gl_destroy_budget = strength_change_pressure ?
            llclamp((S32)llmax(memory_sweep_destroy_gl_per_frame(), 16 * force_multiplier), 0, 1024) :
            llclamp((S32)memory_sweep_destroy_gl_per_frame(), 0, 1024);
        const U32 raw_destroy_budget = strength_change_pressure ?
            llclamp((S32)llmax(memory_sweep_destroy_raw_per_frame(), 128 * force_multiplier), 0, 4096) :
            llclamp((S32)memory_sweep_destroy_raw_per_frame(), 0, 4096);
        const U32 scale_down_budget = downscale_strength_level > 0 ?
            (strength_change_pressure ?
                llclamp((S32)memory_sweep_scale_down_per_frame(), 0, 4096) :
                llclamp((S32)memory_sweep_scale_down_per_frame(), 0, 4096)) :
            0;
        static LLCachedControl<F32> far_distance_m(gSavedSettings, "AYATextureDistanceCapFarMeters", 50.f);
        static LLCachedControl<F32> near_distance_m(gSavedSettings, "AYATextureDistanceCapNearMeters", 10.f);

        U32 scanned = 0;
        U32 primary_scanned = 0;
        U32 destroyed_gl = 0;
        U32 destroyed_raw = 0;
        U32 scaled_down = 0;
        U32 skipped = 0;
        U32 deferred_strength = 0;
        U32 bucket_counts[5] = {};
        uuid_map_t::iterator iter = mUUIDMap.upper_bound(s_memory_sweep_last_key);
        const U32 retry_budget = strength_change_pressure ?
            llmin((U32)s_force_strength_sweep_retry_keys.size(), llmax((U32)1, scan_budget / 4)) :
            0;
        while (scanned < scan_budget && !mUUIDMap.empty())
        {
            bool retry_entry = false;
            LLTextureKey image_key;
            LLPointer<LLViewerFetchedTexture> imagep;
            if (strength_change_pressure &&
                s_force_strength_sweep_remaining == 0 &&
                scanned < retry_budget &&
                !s_force_strength_sweep_retry_keys.empty())
            {
                std::set<LLTextureKey>::iterator retry_iter = s_force_strength_sweep_retry_keys.begin();
                image_key = *retry_iter;
                s_force_strength_sweep_retry_keys.erase(retry_iter);
                uuid_map_t::iterator found = mUUIDMap.find(image_key);
                if (found == mUUIDMap.end())
                {
                    ++scanned;
                    continue;
                }
                imagep = found->second;
                retry_entry = true;
            }
            else if (s_force_strength_sweep_remaining > 0 || !strength_change_pressure)
            {
                if (iter == mUUIDMap.end())
                {
                    iter = mUUIDMap.begin();
                }

                image_key = iter->first;
                imagep = iter->second;
                s_memory_sweep_last_key = image_key;
                ++iter;
                ++primary_scanned;
            }
            else
            {
                break;
            }

            ++scanned;

            AYAMemorySweepBucket bucket = aya_texture_memory_sweep_bucket(
                imagep,
                near_distance_m(),
                far_distance_m(),
                memory_sweep_min_unreferenced_seconds());
            if (bucket == AYA_MEMORY_SWEEP_KEEP)
            {
                if (cap_enforcement_pressure &&
                    aya_texture_can_enter_strength_sweep(imagep))
                {
                    bucket = AYA_MEMORY_SWEEP_CAP_CANDIDATE;
                }
                else
                {
                    if (strength_change_pressure &&
                        aya_texture_strength_sweep_should_retry_later(imagep))
                    {
                        ++deferred_strength;
                        if (s_force_strength_sweep_retry_keys.size() < 8192)
                        {
                            s_force_strength_sweep_retry_keys.insert(image_key);
                        }
                    }
                    ++skipped;
                    continue;
                }
            }

            ++bucket_counts[bucket];

            auto destroy_raw_if_budget_allows =
                [&]()
                {
                    bool destroyed_all_raw = true;
                    if (imagep->hasSavedRawImage() &&
                        destroyed_raw < raw_destroy_budget)
                    {
                        imagep->destroySavedRawImage();
                        ++destroyed_raw;
                    }
                    if (imagep->hasSavedRawImage())
                    {
                        destroyed_all_raw = false;
                    }
                    if (imagep->getRawImage() &&
                        destroyed_raw < raw_destroy_budget)
                    {
                        imagep->destroyRawImage();
                        ++destroyed_raw;
                    }
                    if (imagep->getRawImage())
                    {
                        destroyed_all_raw = false;
                    }
                    return destroyed_all_raw;
                };

            if (bucket != AYA_MEMORY_SWEEP_CAP_CANDIDATE)
            {
                destroy_raw_if_budget_allows();
            }

            if (bucket == AYA_MEMORY_SWEEP_UNREFERENCED &&
                imagep->hasGLTexture() &&
                destroyed_gl < gl_destroy_budget)
            {
                imagep->destroyGLTexture();
                imagep->mMaxVirtualSize = 0.f;
                ++destroyed_gl;
                continue;
            }

            const bool aggressive_destroy =
                memory_sweep_destroy_far_offscreen_gl() &&
                (bucket == AYA_MEMORY_SWEEP_OFFSCREEN || bucket == AYA_MEMORY_SWEEP_FAR) &&
                (memory_pressure ||
                 imagep->getLastReferencedTimer()->getElapsedTimeF32() >=
                     llmax(0.f, memory_sweep_destroy_far_offscreen_min_seconds()));
            if (aggressive_destroy &&
                imagep->hasGLTexture() &&
                destroyed_gl < gl_destroy_budget)
            {
                imagep->destroyGLTexture();
                imagep->mMaxVirtualSize = 0.f;
                ++destroyed_gl;
                continue;
            }

            if ((bucket == AYA_MEMORY_SWEEP_OFFSCREEN ||
                 bucket == AYA_MEMORY_SWEEP_FAR ||
                 bucket == AYA_MEMORY_SWEEP_CAP_CANDIDATE) &&
                imagep->hasGLTexture() &&
                imagep->getDiscardLevel() >= 0 &&
                imagep->getDiscardLevel() < MAX_DISCARD_LEVEL)
            {
                if (scaled_down >= scale_down_budget)
                {
                    if (strength_change_pressure &&
                        bucket == AYA_MEMORY_SWEEP_CAP_CANDIDATE &&
                        s_force_strength_sweep_retry_keys.size() < 8192)
                    {
                        s_force_strength_sweep_retry_keys.insert(image_key);
                        ++deferred_strength;
                    }
                    ++skipped;
                    continue;
                }
                imagep->mMaxVirtualSize = 0.f;
                if (bucket == AYA_MEMORY_SWEEP_CAP_CANDIDATE)
                {
                    updateImageDecodePriority(imagep, false, false);
                    if (imagep->isDeleted() || !imagep->isInImageList())
                    {
                        continue;
                    }
                }
                else
                {
                    imagep->processTextureStats();
                }
                if (imagep->scaleDown())
                {
                    ++scaled_down;
                }
            }
        }

        s_memory_sweep_scanned += scanned;
        s_memory_sweep_destroyed_gl += destroyed_gl;
        s_memory_sweep_destroyed_raw += destroyed_raw;
        s_memory_sweep_scaled_down += scaled_down;
        s_memory_sweep_skipped += skipped;
        s_memory_sweep_unreferenced += bucket_counts[AYA_MEMORY_SWEEP_UNREFERENCED];
        s_memory_sweep_offscreen += bucket_counts[AYA_MEMORY_SWEEP_OFFSCREEN];
        s_memory_sweep_far += bucket_counts[AYA_MEMORY_SWEEP_FAR];
        s_memory_sweep_cap_candidates += bucket_counts[AYA_MEMORY_SWEEP_CAP_CANDIDATE];
        if (strength_change_pressure)
        {
            const U32 consumed = primary_scanned >= s_force_strength_sweep_remaining ? s_force_strength_sweep_remaining : primary_scanned;
            s_force_strength_sweep_remaining -= consumed;
            if (!s_force_strength_sweep_retry_keys.empty() && s_force_strength_sweep_retry_frames < 60)
            {
                ++s_force_strength_sweep_retry_frames;
            }
            else if ((s_force_strength_sweep_remaining == 0 && s_force_strength_sweep_retry_keys.empty()) ||
                     s_force_strength_sweep_retry_frames >= 60)
            {
                s_force_strength_sweep = false;
                s_force_strength_sweep_remaining = 0;
                s_force_strength_sweep_retry_frames = 0;
                s_force_strength_sweep_retry_keys.clear();
            }
        }
    }

    if (memory_sweep_log() && s_memory_sweep_log_timer.getElapsedTimeF32() >= 10.f)
    {
        LL_INFOS("AYATextureCap")
            << "memory_sweep_stats"
            << " texture_management_enabled=" << (aya_texture_management_enabled() ? "true" : "false")
            << " enabled=" << (memory_sweep_enabled() ? "true" : "false")
            << " active=" << (run_memory_sweep ? "true" : "false")
            << " memory_pressure=" << (memory_pressure ? "true" : "false")
            << " strength_change_pressure=" << (strength_change_pressure ? "true" : "false")
            << " strength_sweep_remaining=" << s_force_strength_sweep_remaining
            << " strength_sweep_retry_frames=" << s_force_strength_sweep_retry_frames
            << " strength_sweep_retry_keys=" << s_force_strength_sweep_retry_keys.size()
            << " cap_enforcement_pressure=" << (cap_enforcement_pressure ? "true" : "false")
            << " physical_mb=" << physical_mb
            << " fps=" << gFPSClamped
            << " downscale_strength_level=" << downscale_strength_level
            << " scanned=" << s_memory_sweep_scanned
            << " destroyed_gl=" << s_memory_sweep_destroyed_gl
            << " destroyed_raw=" << s_memory_sweep_destroyed_raw
            << " scaled_down=" << s_memory_sweep_scaled_down
            << " skipped=" << s_memory_sweep_skipped
            << " unreferenced=" << s_memory_sweep_unreferenced
            << " offscreen=" << s_memory_sweep_offscreen
            << " far=" << s_memory_sweep_far
            << " cap_candidates=" << s_memory_sweep_cap_candidates
            << " image_count=" << mUUIDMap.size()
            << LL_ENDL;
        s_memory_sweep_scanned = 0;
        s_memory_sweep_destroyed_gl = 0;
        s_memory_sweep_destroyed_raw = 0;
        s_memory_sweep_scaled_down = 0;
        s_memory_sweep_skipped = 0;
        s_memory_sweep_unreferenced = 0;
        s_memory_sweep_offscreen = 0;
        s_memory_sweep_far = 0;
        s_memory_sweep_cap_candidates = 0;
        s_memory_sweep_log_timer.reset();
    }

    return timer.getElapsedTimeF32();
}

void LLViewerTextureList::updateImagesUpdateStats()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if (mForceResetTextureStats)
    {
        for (image_list_t::iterator iter = mImageList.begin();
             iter != mImageList.end(); )
        {
            LLViewerFetchedTexture* imagep = *iter++;
            imagep->resetTextureStats();
        }
        mForceResetTextureStats = false;
    }
}

void LLViewerTextureList::decodeAllImages(F32 max_time)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    LLTimer timer;

    //loading from fast cache
    max_time -= updateImagesLoadingFastCache(max_time);

    // Update texture stats and priorities
    std::vector<LLPointer<LLViewerFetchedTexture> > image_list;
    for (image_list_t::iterator iter = mImageList.begin();
         iter != mImageList.end(); )
    {
        LLViewerFetchedTexture* imagep = *iter++;
        image_list.push_back(imagep);
        imagep->setInImageList(false) ;
    }

    llassert_always(image_list.size() == mImageList.size()) ;
    mImageList.clear();
    for (std::vector<LLPointer<LLViewerFetchedTexture> >::iterator iter = image_list.begin();
         iter != image_list.end(); ++iter)
    {
        LLViewerFetchedTexture* imagep = *iter;
        imagep->processTextureStats();
        addImageToList(imagep);
    }
    image_list.clear();

    // Update fetch (decode)
    for (image_list_t::iterator iter = mImageList.begin();
         iter != mImageList.end(); )
    {
        LLViewerFetchedTexture* imagep = *iter++;
        imagep->updateFetch();
    }
    std::shared_ptr<LL::WorkQueue> main_queue = LLImageGLThread::sEnabledTextures ? LL::WorkQueue::getInstance("mainloop") : NULL;
    // Run threads
    size_t fetch_pending = 0;
    while (1)
    {
        LLAppViewer::instance()->getTextureCache()->update(1); // unpauses the texture cache thread
        LLAppViewer::instance()->getImageDecodeThread()->update(1); // unpauses the image thread
        fetch_pending = LLAppViewer::instance()->getTextureFetch()->update(1); // unpauses the texture fetch thread

        if (LLImageGLThread::sEnabledTextures)
        {
            main_queue->runFor(std::chrono::milliseconds(1));
            fetch_pending += main_queue->size();
        }

        if (fetch_pending == 0 || timer.getElapsedTimeF32() > max_time)
        {
            break;
        }
    }
    // Update fetch again
    for (image_list_t::iterator iter = mImageList.begin();
         iter != mImageList.end(); )
    {
        LLViewerFetchedTexture* imagep = *iter++;
        imagep->updateFetch();
    }
    max_time -= timer.getElapsedTimeF32();
    max_time = llmax(max_time, .001f);
    F32 create_time = updateImagesCreateTextures(max_time);

    LL_DEBUGS("ViewerImages") << "decodeAllImages() took " << timer.getElapsedTimeF32() << " seconds. "
    << " fetch_pending " << fetch_pending
    << " create_time " << create_time
    << LL_ENDL;
}

bool LLViewerTextureList::createUploadFile(LLPointer<LLImageRaw> raw_image,
                                           const std::string& out_filename,
                                           const S32 max_image_dimentions,
                                           const S32 min_image_dimentions)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    LLImageDataSharedLock lock(raw_image);

    // make a copy, since convertToUploadFile scales raw image
    LLPointer<LLImageRaw> scale_image = new LLImageRaw(
        raw_image->getData(),
        raw_image->getWidth(),
        raw_image->getHeight(),
        raw_image->getComponents());

    LLPointer<LLImageJ2C> compressedImage = LLViewerTextureList::convertToUploadFile(scale_image, max_image_dimentions);
    if (compressedImage->getWidth() < min_image_dimentions || compressedImage->getHeight() < min_image_dimentions)
    {
        std::string reason = llformat("Images below %d x %d pixels are not allowed. Actual size: %d x %dpx",
                                      min_image_dimentions,
                                      min_image_dimentions,
                                      compressedImage->getWidth(),
                                      compressedImage->getHeight());
        compressedImage->setLastError(reason);
        return false;
    }
    if (compressedImage.isNull())
    {
        compressedImage->setLastError("Couldn't convert the image to jpeg2000.");
        LL_INFOS() << "Couldn't convert to j2c, file : " << out_filename << LL_ENDL;
        return false;
    }
    if (!compressedImage->save(out_filename))
    {
        compressedImage->setLastError("Couldn't create the jpeg2000 image for upload.");
        LL_INFOS() << "Couldn't create output file : " << out_filename << LL_ENDL;
        return false;
    }
    return true;
}

bool LLViewerTextureList::createUploadFile(const std::string& filename,
                                         const std::string& out_filename,
                                         const U8 codec,
                                         const S32 max_image_dimentions,
                                         const S32 min_image_dimentions,
                                         bool force_square)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    try
    {
    // Load the image
    LLPointer<LLImageFormatted> image = LLImageFormatted::createFromType(codec);
    if (image.isNull())
    {
        LL_WARNS() << "Couldn't open the image to be uploaded." << LL_ENDL;
        return false;
    }
    if (!image->load(filename))
    {
        image->setLastError("Couldn't load the image to be uploaded.");
        return false;
    }

    // calcDataSizeJ2C assumes maximum size is 2048 and for bigger images can
    // assign discard to bring imige to needed size, but upload does the scaling
    // as needed, so just reset discard.
    // Assume file is full and has 'discard' 0 data.
    // Todo: probably a better idea to have some setMaxDimentions in J2C
    // called when loading from a local file
    image->setDiscardLevel(0);

    // Decompress or expand it in a raw image structure
    LLPointer<LLImageRaw> raw_image = new LLImageRaw;
    if (!image->decode(raw_image, 0.0f))
    {
        image->setLastError("Couldn't decode the image to be uploaded.");
        return false;
    }
    // Check the image constraints
    if ((image->getComponents() != 3) && (image->getComponents() != 4))
    {
        image->setLastError("Image files with less than 3 or more than 4 components are not supported.");
        return false;
    }
    if (image->getWidth() < min_image_dimentions || image->getHeight() < min_image_dimentions)
    {
        std::string reason = llformat("Images below %d x %d pixels are not allowed. Actual size: %d x %dpx",
            min_image_dimentions,
            min_image_dimentions,
            image->getWidth(),
            image->getHeight());
        image->setLastError(reason);
        return false;
    }
    // Convert to j2c (JPEG2000) and save the file locally
    LLPointer<LLImageJ2C> compressedImage = convertToUploadFile(raw_image, max_image_dimentions, force_square);
    if (compressedImage.isNull())
    {
        image->setLastError("Couldn't convert the image to jpeg2000.");
        LL_INFOS() << "Couldn't convert to j2c, file : " << filename << LL_ENDL;
        return false;
    }
    if (!compressedImage->save(out_filename))
    {
        image->setLastError("Couldn't create the jpeg2000 image for upload.");
        LL_INFOS() << "Couldn't create output file : " << out_filename << LL_ENDL;
        return false;
    }
    // Test to see if the encode and save worked
    LLPointer<LLImageJ2C> integrity_test = new LLImageJ2C;
    if (!integrity_test->loadAndValidate( out_filename ))
    {
        image->setLastError("The created jpeg2000 image is corrupt.");
        LL_INFOS() << "Image file : " << out_filename << " is corrupt" << LL_ENDL;
        return false;
    }
    }
    catch (...)
    {
        LOG_UNHANDLED_EXCEPTION("");
        return false;
    }
    return true;
}

// note: modifies the argument raw_image!!!!
LLPointer<LLImageJ2C> LLViewerTextureList::convertToUploadFile(LLPointer<LLImageRaw> raw_image, const S32 max_image_dimentions, bool force_square, bool force_lossless)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    LLImageDataLock lock(raw_image);

    if (force_square)
    {
        S32 biggest_side = llmax(raw_image->getWidth(), raw_image->getHeight());
        S32 square_size = raw_image->biasedDimToPowerOfTwo(biggest_side, max_image_dimentions);

        raw_image->scale(square_size, square_size);
    }
    else
    {
        raw_image->biasedScaleToPowerOfTwo(max_image_dimentions);
    }
    LLPointer<LLImageJ2C> compressedImage = new LLImageJ2C();

    if (force_lossless ||
        (gSavedSettings.getBOOL("LosslessJ2CUpload") &&
            (raw_image->getWidth() * raw_image->getHeight() <= LL_IMAGE_REZ_LOSSLESS_CUTOFF * LL_IMAGE_REZ_LOSSLESS_CUTOFF)))
    {
        compressedImage->setReversible(true);
    }


    if (gSavedSettings.getBOOL("Jpeg2000AdvancedCompression"))
    {
        // This test option will create jpeg2000 images with precincts for each level, RPCL ordering
        // and PLT markers. The block size is also optionally modifiable.
        // Note: the images hence created are compatible with older versions of the viewer.
        // Read the blocks and precincts size settings
        S32 block_size = gSavedSettings.getS32("Jpeg2000BlocksSize");
        S32 precinct_size = gSavedSettings.getS32("Jpeg2000PrecinctsSize");
        LL_INFOS() << "Advanced JPEG2000 Compression: precinct = " << precinct_size << ", block = " << block_size << LL_ENDL;
        compressedImage->initEncode(*raw_image, block_size, precinct_size, 0);
    }

    if (!compressedImage->encode(raw_image, 0.0f))
    {
        LL_INFOS() << "convertToUploadFile : encode returns with error!!" << LL_ENDL;
        // Clear up the pointer so we don't leak that one
        compressedImage = NULL;
    }

    return compressedImage;
}

///////////////////////////////////////////////////////////////////////////////

// <FS:Ansariel> OpenSim compatibility
// static
void LLViewerTextureList::receiveImageHeader(LLMessageSystem *msg, void **user_data)
{
    static LLCachedControl<bool> log_texture_traffic(gSavedSettings,"LogTextureNetworkTraffic", false) ;

    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    // Receive image header, copy into image object and decompresses
    // if this is a one-packet image.

    LLUUID id;

    char ip_string[256];
    u32_to_ip_string(msg->getSenderIP(),ip_string);

    U32Bytes received_size ;
    if (msg->getReceiveCompressedSize())
    {
        received_size = (U32Bytes)msg->getReceiveCompressedSize() ;
    }
    else
    {
        received_size = (U32Bytes)msg->getReceiveSize() ;
    }
    add(LLStatViewer::TEXTURE_NETWORK_DATA_RECEIVED, received_size);
    add(LLStatViewer::TEXTURE_PACKETS, 1);

    U8 codec;
    U16 packets;
    U32 totalbytes;
    msg->getUUIDFast(_PREHASH_ImageID, _PREHASH_ID, id);
    msg->getU8Fast(_PREHASH_ImageID, _PREHASH_Codec, codec);
    msg->getU16Fast(_PREHASH_ImageID, _PREHASH_Packets, packets);
    msg->getU32Fast(_PREHASH_ImageID, _PREHASH_Size, totalbytes);

    S32 data_size = msg->getSizeFast(_PREHASH_ImageData, _PREHASH_Data);
    if (!data_size)
    {
        return;
    }
    if (data_size < 0)
    {
        // msg->getSizeFast() is probably trying to tell us there
        // was an error.
        LL_ERRS() << "image header chunk size was negative: "
        << data_size << LL_ENDL;
        return;
    }

    // this buffer gets saved off in the packet list
    U8 *data = new U8[data_size];
    msg->getBinaryDataFast(_PREHASH_ImageData, _PREHASH_Data, data, data_size);

    LLViewerFetchedTexture *image = LLViewerTextureManager::getFetchedTexture(id, FTT_DEFAULT, true, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
    if (!image)
    {
        delete [] data;
        return;
    }
    if(log_texture_traffic)
    {
        gTotalTextureBytesPerBoostLevel[image->getBoostLevel()] += received_size ;
    }

    //image->getLastPacketTimer()->reset();
    bool res = LLAppViewer::getTextureFetch()->receiveImageHeader(msg->getSender(), id, codec, packets, totalbytes, data_size, data);
    if (!res)
    {
        delete[] data;
    }
}

// static
void LLViewerTextureList::receiveImagePacket(LLMessageSystem *msg, void **user_data)
{
    static LLCachedControl<bool> log_texture_traffic(gSavedSettings,"LogTextureNetworkTraffic", false) ;

    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    // Receives image packet, copy into image object,
    // checks if all packets received, decompresses if so.

    LLUUID id;
    U16 packet_num;

    char ip_string[256];
    u32_to_ip_string(msg->getSenderIP(),ip_string);

    U32Bytes received_size ;
    if (msg->getReceiveCompressedSize())
    {
        received_size = (U32Bytes)msg->getReceiveCompressedSize() ;
    }
    else
    {
        received_size = (U32Bytes)msg->getReceiveSize() ;
    }

    add(LLStatViewer::TEXTURE_NETWORK_DATA_RECEIVED, F64Bytes(received_size));
    add(LLStatViewer::TEXTURE_PACKETS, 1);

    //llprintline("Start decode, image header...");
    msg->getUUIDFast(_PREHASH_ImageID, _PREHASH_ID, id);
    msg->getU16Fast(_PREHASH_ImageID, _PREHASH_Packet, packet_num);
    S32 data_size = msg->getSizeFast(_PREHASH_ImageData, _PREHASH_Data);

    if (!data_size)
    {
        return;
    }
    if (data_size < 0)
    {
        // msg->getSizeFast() is probably trying to tell us there
        // was an error.
        LL_ERRS() << "image data chunk size was negative: "
        << data_size << LL_ENDL;
        return;
    }
    if (data_size > MTUBYTES)
    {
        LL_ERRS() << "image data chunk too large: " << data_size << " bytes" << LL_ENDL;
        return;
    }
    U8 *data = new U8[data_size];
    msg->getBinaryDataFast(_PREHASH_ImageData, _PREHASH_Data, data, data_size);

    LLViewerFetchedTexture *image = LLViewerTextureManager::getFetchedTexture(id, FTT_DEFAULT, true, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
    if (!image)
    {
        delete [] data;
        return;
    }
    if(log_texture_traffic)
    {
        gTotalTextureBytesPerBoostLevel[image->getBoostLevel()] += received_size ;
    }

    //image->getLastPacketTimer()->reset();
    bool res = LLAppViewer::getTextureFetch()->receiveImagePacket(msg->getSender(), id, packet_num, data_size, data);
    if (!res)
    {
        delete[] data;
    }
}
// </FS:Ansariel>

// We've been that the asset server does not contain the requested image id.
// static
void LLViewerTextureList::processImageNotInDatabase(LLMessageSystem *msg,void **user_data)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    LLUUID image_id;
    msg->getUUIDFast(_PREHASH_ImageID, _PREHASH_ID, image_id);

    LLViewerFetchedTexture* image = gTextureList.findImage( image_id, TEX_LIST_STANDARD);
    if( image )
    {
        LL_WARNS() << "Image not in db" << LL_ENDL;
        image->setIsMissingAsset();
    }

    image = gTextureList.findImage(image_id, TEX_LIST_SCALE);
    if (image)
    {
        LL_WARNS() << "Icon not in db" << LL_ENDL;
        image->setIsMissingAsset();
    }
}


///////////////////////////////////////////////////////////////////////////////

// explicitly cleanup resources, as this is a singleton class with process
// lifetime so ability to perform std::map operations in destructor is not
// guaranteed.
void LLUIImageList::cleanUp()
{
    mUIImages.clear();
    mUITextureList.clear() ;
}

LLUIImagePtr LLUIImageList::getUIImageByID(const LLUUID& image_id, S32 priority)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    // use id as image name
    std::string image_name = image_id.asString();

    // look for existing image
    uuid_ui_image_map_t::iterator found_it = mUIImages.find(image_name);
    if (found_it != mUIImages.end())
    {
        return found_it->second;
    }

    const bool use_mips = false;
    const LLRect scale_rect = LLRect::null;
    const LLRect clip_rect = LLRect::null;
    return loadUIImageByID(image_id, use_mips, scale_rect, clip_rect, (LLViewerTexture::EBoostLevel)priority);
}

LLUIImagePtr LLUIImageList::getUIImage(const std::string& image_name, S32 priority)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    // look for existing image
    uuid_ui_image_map_t::iterator found_it = mUIImages.find(image_name);
    if (found_it != mUIImages.end())
    {
        return found_it->second;
    }

    const bool use_mips = false;
    const LLRect scale_rect = LLRect::null;
    const LLRect clip_rect = LLRect::null;
    return loadUIImageByName(image_name, image_name, use_mips, scale_rect, clip_rect, (LLViewerTexture::EBoostLevel)priority);
}

LLUIImagePtr LLUIImageList::loadUIImageByName(const std::string& name, const std::string& filename,
                                              bool use_mips, const LLRect& scale_rect, const LLRect& clip_rect, LLViewerTexture::EBoostLevel boost_priority,
                                              LLUIImage::EScaleStyle scale_style)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if (boost_priority == LLGLTexture::BOOST_NONE)
    {
        boost_priority = LLGLTexture::BOOST_UI;
    }
    LLViewerFetchedTexture* imagep = LLViewerTextureManager::getFetchedTextureFromFile(filename, FTT_LOCAL_FILE, MIPMAP_NO, boost_priority);
    return loadUIImage(imagep, name, use_mips, scale_rect, clip_rect, scale_style);
}

LLUIImagePtr LLUIImageList::loadUIImageByID(const LLUUID& id,
                                            bool use_mips, const LLRect& scale_rect, const LLRect& clip_rect, LLViewerTexture::EBoostLevel boost_priority,
                                            LLUIImage::EScaleStyle scale_style)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if (boost_priority == LLGLTexture::BOOST_NONE)
    {
        boost_priority = LLGLTexture::BOOST_UI;
    }
    LLViewerFetchedTexture* imagep = LLViewerTextureManager::getFetchedTexture(id, FTT_DEFAULT, MIPMAP_NO, boost_priority);
    return loadUIImage(imagep, id.asString(), use_mips, scale_rect, clip_rect, scale_style);
}

LLUIImagePtr LLUIImageList::loadUIImage(LLViewerFetchedTexture* imagep, const std::string& name, bool use_mips, const LLRect& scale_rect, const LLRect& clip_rect,
                                        LLUIImage::EScaleStyle scale_style)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if (!imagep) return NULL;

    imagep->setAddressMode(LLTexUnit::TAM_CLAMP);

    //don't compress UI images
    imagep->getGLTexture()->setAllowCompression(false);

    LLUIImagePtr new_imagep = new LLUIImage(name, imagep);
    new_imagep->setScaleStyle(scale_style);

    if (imagep->getBoostLevel() != LLGLTexture::BOOST_ICON
        && imagep->getBoostLevel() != LLGLTexture::BOOST_THUMBNAIL
        && imagep->getBoostLevel() != LLGLTexture::BOOST_PREVIEW)
    {
        // Don't add downloadable content into this list
        // all UI images are non-deletable and list does not support deletion
        imagep->setNoDelete();
        mUIImages.insert(std::make_pair(name, new_imagep));
        mUITextureList.push_back(imagep);
    }

    //Note:
    //Some other textures such as ICON also through this flow to be fetched.
    //But only UI textures need to set this callback.
    if(imagep->getBoostLevel() == LLGLTexture::BOOST_UI)
    {
        LLUIImageLoadData* datap = new LLUIImageLoadData;
        datap->mImageName = name;
        datap->mImageScaleRegion = scale_rect;
        datap->mImageClipRegion = clip_rect;

        imagep->setLoadedCallback(onUIImageLoaded, 0, false, false, datap, NULL);
    }
    return new_imagep;
}

LLUIImagePtr LLUIImageList::preloadUIImage(const std::string& name, const std::string& filename, bool use_mips, const LLRect& scale_rect, const LLRect& clip_rect, LLUIImage::EScaleStyle scale_style)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    // look for existing image
    uuid_ui_image_map_t::iterator found_it = mUIImages.find(name);
    if (found_it != mUIImages.end())
    {
        // image already loaded!
        LL_ERRS() << "UI Image " << name << " already loaded." << LL_ENDL;
    }

    return loadUIImageByName(name, filename, use_mips, scale_rect, clip_rect, LLGLTexture::BOOST_UI, scale_style);
}

//static
void LLUIImageList::onUIImageLoaded( bool success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* src_aux, S32 discard_level, bool final, void* user_data )
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if(!success || !user_data)
    {
        return;
    }

    LLUIImageLoadData* image_datap = (LLUIImageLoadData*)user_data;
    std::string ui_image_name = image_datap->mImageName;
    LLRect scale_rect = image_datap->mImageScaleRegion;
    LLRect clip_rect = image_datap->mImageClipRegion;
    if (final)
    {
        delete image_datap;
    }

    LLUIImageList* instance = getInstance();

    uuid_ui_image_map_t::iterator found_it = instance->mUIImages.find(ui_image_name);
    if (found_it != instance->mUIImages.end())
    {
        LLUIImagePtr imagep = found_it->second;

        // for images grabbed from local files, apply clipping rectangle to restore original dimensions
        // from power-of-2 gl image
        if (success && imagep.notNull() && src_vi && (src_vi->getUrl().compare(0, 7, "file://")==0))
        {
            F32 full_width = (F32)src_vi->getFullWidth();
            F32 full_height = (F32)src_vi->getFullHeight();
            F32 clip_x = (F32)src_vi->getOriginalWidth() / full_width;
            F32 clip_y = (F32)src_vi->getOriginalHeight() / full_height;
            if (clip_rect != LLRect::null)
            {
                imagep->setClipRegion(LLRectf(llclamp((F32)clip_rect.mLeft / full_width, 0.f, 1.f),
                                            llclamp((F32)clip_rect.mTop / full_height, 0.f, 1.f),
                                            llclamp((F32)clip_rect.mRight / full_width, 0.f, 1.f),
                                            llclamp((F32)clip_rect.mBottom / full_height, 0.f, 1.f)));
            }
            else
            {
                imagep->setClipRegion(LLRectf(0.f, clip_y, clip_x, 0.f));
            }
            if (scale_rect != LLRect::null)
            {
                imagep->setScaleRegion(
                    LLRectf(llclamp((F32)scale_rect.mLeft / (F32)imagep->getWidth(), 0.f, 1.f),
                        llclamp((F32)scale_rect.mTop / (F32)imagep->getHeight(), 0.f, 1.f),
                        llclamp((F32)scale_rect.mRight / (F32)imagep->getWidth(), 0.f, 1.f),
                        llclamp((F32)scale_rect.mBottom / (F32)imagep->getHeight(), 0.f, 1.f)));
            }

            imagep->onImageLoaded();
        }
    }
}

namespace LLInitParam
{
    template<>
    struct TypeValues<LLUIImage::EScaleStyle> : public TypeValuesHelper<LLUIImage::EScaleStyle>
    {
        static void declareValues()
        {
            declare("scale_inner",  LLUIImage::SCALE_INNER);
            declare("scale_outer",  LLUIImage::SCALE_OUTER);
        }
    };
}

struct UIImageDeclaration : public LLInitParam::Block<UIImageDeclaration>
{
    Mandatory<std::string>      name;
    Optional<std::string>       file_name;
    Optional<bool>              preload;
    Optional<LLRect>            scale;
    Optional<LLRect>            clip;
    Optional<bool>              use_mips;
    Optional<LLUIImage::EScaleStyle> scale_type;

    UIImageDeclaration()
    :   name("name"),
        file_name("file_name"),
        preload("preload", false),
        scale("scale"),
        clip("clip"),
        use_mips("use_mips", false),
        scale_type("scale_type", LLUIImage::SCALE_INNER)
    {}
};

struct UIImageDeclarations : public LLInitParam::Block<UIImageDeclarations>
{
    Mandatory<S32>  version;
    Multiple<UIImageDeclaration> textures;

    UIImageDeclarations()
    :   version("version"),
        textures("texture")
    {}
};

bool LLUIImageList::initFromFile()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    // Look for textures.xml in all the right places. Pass
    // constraint=LLDir::ALL_SKINS because we want to overlay textures.xml
    // from all the skins directories.
    std::vector<std::string> textures_paths =
        gDirUtilp->findSkinnedFilenames(LLDir::TEXTURES, "textures.xml", LLDir::ALL_SKINS);
    std::vector<std::string>::const_iterator pi(textures_paths.begin()), pend(textures_paths.end());
    if (pi == pend)
    {
        LL_WARNS() << "No textures.xml found in skins directories" << LL_ENDL;
        return false;
    }

    // The first (most generic) file gets special validations
    LLXMLNodePtr root;
    if (!LLXMLNode::parseFile(*pi, root, NULL))
    {
        LL_WARNS() << "Unable to parse UI image list file " << *pi << LL_ENDL;
        return false;
    }
    if (!root->hasAttribute("version"))
    {
        LL_WARNS() << "No valid version number in UI image list file " << *pi << LL_ENDL;
        return false;
    }

    UIImageDeclarations images;
    LLXUIParser parser;
    parser.readXUI(root, images, *pi);

    // add components defined in the rest of the skin paths
    while (++pi != pend)
    {
        LLXMLNodePtr update_root;
        if (LLXMLNode::parseFile(*pi, update_root, NULL))
        {
            parser.readXUI(update_root, images, *pi);
        }
    }

    if (!images.validateBlock()) return false;

    std::map<std::string, UIImageDeclaration> merged_declarations;
    for (LLInitParam::ParamIterator<UIImageDeclaration>::const_iterator image_it = images.textures.begin();
        image_it != images.textures.end();
        ++image_it)
    {
        merged_declarations[image_it->name].overwriteFrom(*image_it);
    }

    enum e_decode_pass
    {
        PASS_DECODE_NOW,
        PASS_DECODE_LATER,
        NUM_PASSES
    };

    for (S32 cur_pass = PASS_DECODE_NOW; cur_pass < NUM_PASSES; cur_pass++)
    {
        for (std::map<std::string, UIImageDeclaration>::const_iterator image_it = merged_declarations.begin();
            image_it != merged_declarations.end();
            ++image_it)
        {
            const UIImageDeclaration& image = image_it->second;
            std::string file_name = image.file_name.isProvided() ? image.file_name() : image.name();

            // load high priority textures on first pass (to kick off decode)
            enum e_decode_pass decode_pass = image.preload ? PASS_DECODE_NOW : PASS_DECODE_LATER;
            if (decode_pass != cur_pass)
            {
                continue;
            }
            preloadUIImage(image.name, file_name, image.use_mips, image.scale, image.clip, image.scale_type);
        }

        if (!gSavedSettings.getBOOL("NoPreload"))
        {
            if (cur_pass == PASS_DECODE_NOW)
            {
                // init fetching and decoding of preloaded images
                gTextureList.decodeAllImages(9.f);
            }
            else
            {
                // decodeAllImages needs two passes to refresh stats and priorities on second pass
                gTextureList.decodeAllImages(1.f);
            }
        }
    }
    return true;
}
