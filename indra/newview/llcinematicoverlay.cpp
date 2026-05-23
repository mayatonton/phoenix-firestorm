/**
 * @file llcinematicoverlay.cpp
 * @brief AYAstorm r30 BD full port Phase 5 R2: Cinematic mode BD-parity cvar overlay.
 *
 * See llcinematicoverlay.h and
 *   docs/specs/ayastorm-r30-p5-bd-ui-binding-audit-spec.md §3.4
 * for the architecture rationale.
 */
#include "llviewerprecompiledheaders.h"

#include "llcinematicoverlay.h"

#include "llcontrol.h"
#include "lldir.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llviewercontrol.h"

namespace
{
    const char OVERLAY_FILENAME[]  = "settings_cinematic_bd.xml";
    const char SENTINEL_CONTROL[]  = "AYACinematicOverlayApplied";
    const char MODE_CONTROL[]      = "AYAVisualRealismEnabled";

    bool loadOverlayLLSD(LLSD& out)
    {
        const std::string path =
            gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, OVERLAY_FILENAME);
        if (!gDirUtilp->fileExists(path))
        {
            LL_WARNS("CinematicOverlay")
                << "Overlay file not found: " << path << LL_ENDL;
            return false;
        }
        llifstream file(path.c_str());
        if (!file.is_open())
        {
            LL_WARNS("CinematicOverlay")
                << "Failed to open overlay file: " << path << LL_ENDL;
            return false;
        }
        const S32 rc = LLSDSerialize::fromXMLDocument(out, file);
        file.close();
        if (rc == LLSDParser::PARSE_FAILURE || !out.isMap())
        {
            LL_WARNS("CinematicOverlay")
                << "Overlay XML parse failed or root not a map: " << path << LL_ENDL;
            return false;
        }
        return true;
    }
}

void LLCinematicOverlay::applyCinematicOverlay()
{
    LLSD overlay;
    if (!loadOverlayLLSD(overlay))
    {
        return;
    }

    S32 applied = 0;
    S32 skipped = 0;
    for (LLSD::map_const_iterator it = overlay.beginMap();
         it != overlay.endMap(); ++it)
    {
        const std::string& key = it->first;
        const LLSD&        val = it->second;

        LLControlVariable* ctl = gSavedSettings.getControl(key);
        if (!ctl)
        {
            LL_WARNS("CinematicOverlay")
                << "Overlay references unregistered cvar, skipping: "
                << key << LL_ENDL;
            ++skipped;
            continue;
        }
        ctl->setValue(val);
        ++applied;
    }

    gSavedSettings.setBOOL(SENTINEL_CONTROL, true);

    LL_INFOS("CinematicOverlay")
        << "Applied Cinematic BD overlay: " << applied << " cvars set, "
        << skipped << " skipped." << LL_ENDL;
}

void LLCinematicOverlay::applyCinematicOverlayIfNeeded()
{
    const U32  mode    = gSavedSettings.getU32(MODE_CONTROL);
    const bool applied = gSavedSettings.getBOOL(SENTINEL_CONTROL);
    if (mode == 2 && !applied)
    {
        applyCinematicOverlay();
    }
}

void LLCinematicOverlay::clearOverlaySentinel()
{
    gSavedSettings.setBOOL(SENTINEL_CONTROL, false);
}

// <FS:AYAstorm> r20 SSS cvar consolidation migration.
// Pre-consolidation:
//   AYAR20AvatarSkinSSSEnabled         (default true)  controlled mode 1 SSS
//   AYAR20AvatarSkinSSSInCinematicEnabled (default false) controlled mode 2 SSS
// Post-consolidation:
//   AYAR20AvatarSkinSSSEnabled         (default false) controls both modes
//   AYAR20AvatarSkinSSSInCinematicEnabled — DEPRECATED (kept registered for
//                                            one-shot migration read only)
// Migration semantics (run once, gated by AYAR20SSSMigrationVersion):
//   new_enabled = old_enabled || old_in_cinematic
// This preserves intent for any user who explicitly opted in to SSS in either
// mode while letting fresh installs land on the new default OFF. Users who
// relied only on the old default true in mode 1 (= persisted store empty) will
// land on the new default false; that is the intentional break documented in
// the release notes.
void LLCinematicOverlay::applyR20SSSMigrationIfNeeded()
{
    static const char VERSION_CONTROL[]      = "AYAR20SSSMigrationVersion";
    static const char NEW_ENABLED_CONTROL[]  = "AYAR20AvatarSkinSSSEnabled";
    static const char OLD_IN_CINEMATIC_CTL[] = "AYAR20AvatarSkinSSSInCinematicEnabled";

    const S32 ver = gSavedSettings.getS32(VERSION_CONTROL);
    if (ver >= 1)
    {
        return;
    }

    const bool new_enabled     = gSavedSettings.getBOOL(NEW_ENABLED_CONTROL);
    const bool old_in_cinematic = gSavedSettings.getBOOL(OLD_IN_CINEMATIC_CTL);
    const bool merged          = new_enabled || old_in_cinematic;

    gSavedSettings.setBOOL(NEW_ENABLED_CONTROL, merged);
    gSavedSettings.setS32(VERSION_CONTROL, 1);

    LL_INFOS("CinematicOverlay")
        << "r20 SSS migration v0->v1: enabled=" << new_enabled
        << " in_cinematic=" << old_in_cinematic
        << " -> merged=" << merged << LL_ENDL;
}
// </FS:AYAstorm>

// <FS:AYAstorm> r30 release View Mode picker reshuffle migration.
// Pre-r30-release picker:  0=Firestorm View / 1=AYAstorm View / 2=Cinematic (preview)
// Post-r30-release picker: 0=Firestorm View / 2=AYAstorm View (legacy 1 removed)
// The previous Cinematic mode (value=2) is promoted to "AYAstorm View" and the
// previous AYAstorm View (value=1) disappears from the UI. Migration rewrites
// any persisted value 1 to 2 so returning users land on the new engine with
// their View Mode selection preserved.
void LLCinematicOverlay::applyAYAViewModeMigrationIfNeeded()
{
    static const char VERSION_CONTROL[] = "AYAViewModeMigrationVersion";
    static const char MODE_CONTROL[]    = "AYAVisualRealismEnabled";

    const S32 ver = gSavedSettings.getS32(VERSION_CONTROL);
    if (ver >= 1)
    {
        return;
    }

    const U32 mode = gSavedSettings.getU32(MODE_CONTROL);
    if (mode == 1)
    {
        gSavedSettings.setU32(MODE_CONTROL, 2);
        LL_INFOS("CinematicOverlay")
            << "View mode migration v0->v1: AYAVisualRealismEnabled "
            << "1 (legacy AYAstorm View) -> 2 (new AYAstorm View)" << LL_ENDL;
    }
    else
    {
        LL_INFOS("CinematicOverlay")
            << "View mode migration v0->v1: no rewrite needed "
            << "(AYAVisualRealismEnabled=" << mode << ")" << LL_ENDL;
    }
    gSavedSettings.setS32(VERSION_CONTROL, 1);
}
// </FS:AYAstorm>

// <FS:AYAstorm> r30 BD改善: r15 Godrays Cinematic opt-in default flip migration.
// Pre-flip (live A/B 期間):
//   AYAR15GodraysInCinematicEnabled default false → 既存ユーザーは false 持ち
// Post-flip (BD改善 phase 確定):
//   default true → 新規ユーザーは godrays 体験
// 既存ユーザーは Persist=1 で false が残るため新 default の恩恵を受けない。
// 本 migration で version 0 → 1 のとき強制 true 上書き。明示的に false に設定
// していたユーザーも一度上書きされるが、godrays 撮影体験を guarantee する方を優先。
// 再度 OFF にしたい場合は Debug Settings から手動で false に戻せる。
void LLCinematicOverlay::applyR15GodraysCinematicMigrationIfNeeded()
{
    static const char VERSION_CONTROL[] = "AYAR15GodraysCinematicMigrationVersion";
    static const char ENABLED_CONTROL[] = "AYAR15GodraysInCinematicEnabled";

    const S32 ver = gSavedSettings.getS32(VERSION_CONTROL);
    if (ver >= 1)
    {
        return;
    }

    const bool old_value = gSavedSettings.getBOOL(ENABLED_CONTROL);
    gSavedSettings.setBOOL(ENABLED_CONTROL, true);
    gSavedSettings.setS32(VERSION_CONTROL, 1);

    LL_INFOS("CinematicOverlay")
        << "r15 godrays Cinematic migration v0->v1: "
        << ENABLED_CONTROL << " " << (old_value ? "true" : "false") << " -> true" << LL_ENDL;
}
// </FS:AYAstorm>
