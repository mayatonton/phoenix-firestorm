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
