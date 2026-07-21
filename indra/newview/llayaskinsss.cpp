/**
 * @file llayaskinsss.cpp
 * @brief AYAstorm r20 Phase B/E: per-attachment SSS whitelist matcher.
 *        See header for design notes.
 */

#include "llviewerprecompiledheaders.h"

#include "llayaskinsss.h"

#include "llcharacter.h"
#include "lldrawable.h"           // <FS:AYA r20 Phase C> REBUILD_GEOMETRY
#include "llspatialpartition.h"   // <FS:AYA r20 Phase C> LLSpatialGroup::GEOM_DIRTY
#include "pipeline.h"             // <FS:AYA r20 Phase C> gPipeline.markRebuild
#include "llvkloader.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"
#include "llviewerjointattachment.h"
#include "llvolume.h"        // LL_SCULPT_TYPE_MESH / LL_SCULPT_TYPE_MASK
#include "llvoavatar.h"

namespace
{
constexpr char kCvarName[] = "AYAR20AvatarSkinSSSWhitelist";

// Lowercase + trim in place.
void normalize(std::string& s)
{
    auto not_space = [](unsigned char c) { return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
    s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
}
}

namespace ayastorm
{

SkinSSSMatcher::SkinSSSMatcher()
{
    LLControlVariable* ctrl = gSavedSettings.getControl(kCvarName);
    if (ctrl)
    {
        parseFromString(ctrl->getValue().asString());
        ctrl->getSignal()->connect(
            [](LLControlVariable* /*c*/, const LLSD& /*new_val*/, const LLSD& /*old_val*/)
            {
                SkinSSSMatcher::instance().reloadAndReevaluate();
            });
    }
}

void SkinSSSMatcher::parseFromString(const std::string& raw)
{
    mUUIDs.clear();
    std::string line;
    line.reserve(64);
    auto flush = [&]()
    {
        normalize(line);
        if (!line.empty())
        {
            LLUUID id;
            if (LLUUID::parseUUID(line, &id) && id.notNull())
            {
                mUUIDs.insert(id);
            }
        }
        line.clear();
    };
    for (char c : raw)
    {
        if (c == '\n' || c == '\r')
        {
            flush();
        }
        else
        {
            line.push_back(c);
        }
    }
    flush();
}

// static
LLUUID SkinSSSMatcher::getMeshId(LLViewerObject* obj)
{
    if (!obj || !obj->isSculpted() || !obj->getVolume())
    {
        return LLUUID::null;
    }
    const LLVolumeParams& vp = obj->getVolume()->getParams();
    const U8 sculpt_type = vp.getSculptType();
    if ((sculpt_type & LL_SCULPT_TYPE_MASK) != LL_SCULPT_TYPE_MESH)
    {
        return LLUUID::null;
    }
    return vp.getSculptID();
}

bool SkinSSSMatcher::matches(LLViewerObject* obj) const
{
    if (mUUIDs.empty())
    {
        return false;
    }
    const LLUUID mesh_id = getMeshId(obj);
    if (mesh_id.isNull())
    {
        return false;
    }
    return mUUIDs.find(mesh_id) != mUUIDs.end();
}

bool SkinSSSMatcher::isInWhitelist(const LLUUID& mesh_id) const
{
    if (mesh_id.isNull()) return false;
    return mUUIDs.find(mesh_id) != mUUIDs.end();
}

void SkinSSSMatcher::addUUID(const LLUUID& mesh_id)
{
    if (mesh_id.isNull()) return;
    if (isInWhitelist(mesh_id)) return;
    LLControlVariable* ctrl = gSavedSettings.getControl(kCvarName);
    if (!ctrl) return;
    std::string text = ctrl->getValue().asString();
    if (!text.empty() && text.back() != '\n')
    {
        text.push_back('\n');
    }
    text.append(mesh_id.asString());
    text.push_back('\n');
    gSavedSettings.setString(kCvarName, text);  // fires signal → reloadAndReevaluate
}

void SkinSSSMatcher::removeUUID(const LLUUID& mesh_id)
{
    if (mesh_id.isNull()) return;
    LLControlVariable* ctrl = gSavedSettings.getControl(kCvarName);
    if (!ctrl) return;
    const std::string raw = ctrl->getValue().asString();
    const std::string target = mesh_id.asString();
    std::string out;
    out.reserve(raw.size());
    std::string line;
    line.reserve(64);
    auto flush_line_to_out = [&]()
    {
        std::string normalized = line;
        normalize(normalized);
        LLUUID id;
        // Drop only the matching UUID line; keep everything else
        // (including malformed lines and casing the user typed).
        if (!(LLUUID::parseUUID(normalized, &id) && id == mesh_id))
        {
            out.append(line);
            out.push_back('\n');
        }
        line.clear();
    };
    for (char c : raw)
    {
        if (c == '\n')
        {
            flush_line_to_out();
        }
        else if (c == '\r')
        {
            // Skip CR; treat CRLF and LF identically.
        }
        else
        {
            line.push_back(c);
        }
    }
    if (!line.empty())
    {
        flush_line_to_out();
    }
    gSavedSettings.setString(kCvarName, out);  // fires signal → reloadAndReevaluate
}

void SkinSSSMatcher::reloadAndReevaluate()
{
    LLControlVariable* ctrl = gSavedSettings.getControl(kCvarName);
    if (!ctrl)
    {
        return;
    }
    parseFromString(ctrl->getValue().asString());

    // Walk every avatar's attachments and re-evaluate. Editing the
    // whitelist is rare so the O(avatars * attachments) cost is fine
    // for a UI-driven event.
    for (LLCharacter* charp : LLCharacter::sInstances)
    {
        LLVOAvatar* avatar = dynamic_cast<LLVOAvatar*>(charp);
        if (!avatar) continue;
        for (const auto& pt_entry : avatar->mAttachmentPoints)
        {
            LLViewerJointAttachment* pt = pt_entry.second;
            if (!pt) continue;
            for (LLViewerObject* obj : pt->mAttachedObjects)
            {
                setSSSTargetForAttachment(obj);
            }
        }
    }
}

namespace
{
// <FS:AYA r20 Phase C> Phase C needs the per-object SSS flag to land in
// LLDrawInfo, which is populated during geometry rebuild. Flipping the
// bit alone is not enough — request a geometry rebuild so registerFace()
// re-reads isSSSTarget() and the new value reaches the GBuffer writers.
//
// markRebuild(drawable, REBUILD_GEOMETRY) by itself is not sufficient for
// rigged-mesh attachments: LLVolumeGeometryManager::rebuildGeom early-outs
// when the spatial group is not GEOM_DIRTY, leaving stale LLDrawInfo
// entries. Right-clicking happens to dirty the group via selection, which
// is why "Add" appeared to work but clearing the whitelist from prefs did
// not. Mirror the proven pattern from refreshOutsideParcelHiding /
// LLOctreeDirty (pipeline.cpp): set GEOM_DIRTY on the spatial group and
// enqueue it directly.
void apply_sss_flag(LLViewerObject* obj, bool match)
{
    if (!obj) return;
    const bool prev = obj->isSSSTarget();
    obj->setSSSTarget(match);
    if (prev != match && obj->mDrawable)
    {
        gPipeline.markRebuild(obj->mDrawable, LLDrawable::REBUILD_GEOMETRY);
        if (LLSpatialGroup* group = obj->mDrawable->getSpatialGroup())
        {
            group->setState(LLSpatialGroup::GEOM_DIRTY);
            ++LLVKLoader::gVkPerf.geo_dirty_site[7];
            gPipeline.markRebuild(group);
        }
    }
}
}

void setSSSTargetForAttachment(LLViewerObject* root)
{
    if (!root)
    {
        return;
    }
    // Each prim in a linkset has its own mesh UUID — evaluate per-prim
    // so a body root with a non-skin headpiece child stays correct.
    apply_sss_flag(root, SkinSSSMatcher::instance().matches(root));
    for (LLViewerObject* child : root->getChildren())
    {
        if (!child) continue;
        apply_sss_flag(child, SkinSSSMatcher::instance().matches(child));
    }
}

void clearSSSTargetForAttachment(LLViewerObject* root)
{
    if (!root)
    {
        return;
    }
    apply_sss_flag(root, false);
    for (LLViewerObject* child : root->getChildren())
    {
        apply_sss_flag(child, false);
    }
}

namespace
{
// Collect every distinct mesh-type UUID across root + children.
std::set<LLUUID> collectLinksetMeshIds(LLViewerObject* root)
{
    std::set<LLUUID> out;
    if (!root) return out;
    const LLUUID root_id = SkinSSSMatcher::getMeshId(root);
    if (root_id.notNull()) out.insert(root_id);
    for (LLViewerObject* child : root->getChildren())
    {
        const LLUUID id = SkinSSSMatcher::getMeshId(child);
        if (id.notNull()) out.insert(id);
    }
    return out;
}
}

int addLinksetMeshIdsToWhitelist(LLViewerObject* root)
{
    SkinSSSMatcher& m = SkinSSSMatcher::instance();
    int added = 0;
    for (const LLUUID& id : collectLinksetMeshIds(root))
    {
        if (!m.isInWhitelist(id))
        {
            m.addUUID(id);
            ++added;
        }
    }
    return added;
}

int removeLinksetMeshIdsFromWhitelist(LLViewerObject* root)
{
    SkinSSSMatcher& m = SkinSSSMatcher::instance();
    int removed = 0;
    for (const LLUUID& id : collectLinksetMeshIds(root))
    {
        if (m.isInWhitelist(id))
        {
            m.removeUUID(id);
            ++removed;
        }
    }
    return removed;
}

bool linksetHasUnregisteredMesh(LLViewerObject* root)
{
    const SkinSSSMatcher& m = SkinSSSMatcher::instance();
    for (const LLUUID& id : collectLinksetMeshIds(root))
    {
        if (!m.isInWhitelist(id)) return true;
    }
    return false;
}

bool linksetHasRegisteredMesh(LLViewerObject* root)
{
    const SkinSSSMatcher& m = SkinSSSMatcher::instance();
    for (const LLUUID& id : collectLinksetMeshIds(root))
    {
        if (m.isInWhitelist(id)) return true;
    }
    return false;
}

} // namespace ayastorm
