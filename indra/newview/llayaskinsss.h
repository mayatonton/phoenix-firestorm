/**
 * @file llayaskinsss.h
 * @brief AYAstorm r20 Phase B/E: per-attachment SSS whitelist matcher.
 *
 * The whitelist (AYAR20AvatarSkinSSSWhitelist cvar) is a newline-
 * separated list of **mesh asset UUIDs**. An attachment is an SSS
 * target iff its mesh asset UUID exactly matches one of the listed
 * entries. Empty / whitespace-only / malformed lines are silently
 * ignored, so manual paste-in is safe.
 *
 * Two ways to add a UUID to the list:
 *   - right-click an attachment in-world → "SSS に追加"
 *     (calls addMeshFromObject → addUUID → cvar setString)
 *   - paste a UUID directly into the Preferences > Graphics > SSS
 *     text editor (advanced users)
 *
 * Phase A (screen-space SSS blur pass) currently ignores the per-
 * object flag — it still blurs the entire framebuffer. Phase C will
 * hoist the flag into a GBuffer skin bit so the SSS pass can mask
 * per-pixel.
 */

#ifndef LL_LLAYASKINSSS_H
#define LL_LLAYASKINSSS_H

#include "llsingleton.h"
#include "lluuid.h"
#include <set>

class LLViewerObject;

namespace ayastorm
{

class SkinSSSMatcher : public LLSingleton<SkinSSSMatcher>
{
    LLSINGLETON(SkinSSSMatcher);
public:
    // True iff obj is a mesh-type sculpt whose mesh asset UUID is in
    // the loaded whitelist. False for null, non-mesh, and empty list.
    bool matches(LLViewerObject* obj) const;

    // Returns the mesh asset UUID of obj iff obj is a mesh-type
    // sculpt; LLUUID::null otherwise. Safe with null pointer.
    static LLUUID getMeshId(LLViewerObject* obj);

    // True iff mesh_id is currently in the loaded whitelist.
    bool isInWhitelist(const LLUUID& mesh_id) const;

    // Mutate the cvar to add / remove a UUID line. Re-parse and
    // re-evaluate all avatars is triggered via the cvar change signal.
    void addUUID(const LLUUID& mesh_id);
    void removeUUID(const LLUUID& mesh_id);

    // Re-parse the cvar and re-evaluate every attached object on
    // every avatar. Invoked from the cvar change signal.
    void reloadAndReevaluate();

private:
    void parseFromString(const std::string& raw);
    std::set<LLUUID> mUUIDs;
};

// Set mIsSSSTarget on the given attached object based on whitelist
// match. Walks the root's child list so linked prims inherit the
// flag. Safe to call with a null pointer.
void setSSSTargetForAttachment(LLViewerObject* root);

// Clear mIsSSSTarget on the given attached object and its children.
void clearSSSTargetForAttachment(LLViewerObject* root);

// Walk root + children, add every mesh-type child's mesh UUID to the
// whitelist. Returns the number of UUIDs newly added (already-listed
// UUIDs are silently skipped). Intended for multi-link mesh body
// attachments where Add-by-root only catches the root prim.
int addLinksetMeshIdsToWhitelist(LLViewerObject* root);

// Mirror of addLinksetMeshIdsToWhitelist: walk root + children, remove
// every found mesh UUID from the whitelist. Returns the number of
// UUIDs removed.
int removeLinksetMeshIdsFromWhitelist(LLViewerObject* root);

// True iff at least one mesh UUID in the linkset is NOT yet in the
// whitelist (= AddLinkset would do something).
bool linksetHasUnregisteredMesh(LLViewerObject* root);

// True iff at least one mesh UUID in the linkset IS in the whitelist
// (= RemoveLinkset would do something).
bool linksetHasRegisteredMesh(LLViewerObject* root);

} // namespace ayastorm

#endif // LL_LLAYASKINSSS_H
