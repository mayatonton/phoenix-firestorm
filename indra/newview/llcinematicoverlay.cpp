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
