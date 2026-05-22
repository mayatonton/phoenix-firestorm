/**
 * @file llcinematicoverlay.h
 * @brief AYAstorm r30 BD full port Phase 5 R2: Cinematic mode BD-parity cvar overlay.
 *
 * Loads `app_settings/settings_cinematic_bd.xml` (flat LLSD map of
 * cvar -> BD canonical value) and forces those values into `gSavedSettings`
 * when AYAVisualRealismEnabled == 2 (Cinematic).
 *
 * Application policy (sentinel `AYACinematicOverlayApplied`):
 *   - Startup with mode 2 and sentinel == 0  -> apply overlay, set sentinel = 1
 *   - Mode switch 0/1 -> 2                   -> apply overlay, set sentinel = 1
 *   - Mode switch 2 -> 0/1                   -> reset sentinel = 0 (cvars untouched)
 *
 * User edits in mode 2 persist (the overlay is not re-applied until next entry
 * to mode 2 from a different mode, or until the user manually clears the
 * sentinel). This preserves per-session tunings while guaranteeing a known BD
 * baseline whenever Cinematic mode is freshly entered.
 *
 * Architecture rationale: see
 *   docs/specs/ayastorm-r30-p5-bd-ui-binding-audit-spec.md §3.4
 */
#ifndef LL_CINEMATIC_OVERLAY_H
#define LL_CINEMATIC_OVERLAY_H

namespace LLCinematicOverlay
{
    // Force-apply BD-parity values to all cvars listed in
    // settings_cinematic_bd.xml. Sets sentinel AYACinematicOverlayApplied = 1.
    void applyCinematicOverlay();

    // Apply only if AYAVisualRealismEnabled == 2 and the sentinel is unset.
    // Safe to call from initConfiguration and from the mode-switch handler.
    void applyCinematicOverlayIfNeeded();

    // Clear the sentinel so the next entry into mode 2 force-applies again.
    void clearOverlaySentinel();

    // <FS:AYAstorm> r20 SSS cvar consolidation migration. Runs once on
    // startup: when AYAR20SSSMigrationVersion < 1, OR-merges old
    // AYAR20AvatarSkinSSSInCinematicEnabled into AYAR20AvatarSkinSSSEnabled
    // (the new single-source-of-truth cvar) and bumps the version counter.
    // See settings.xml entry for AYAR20SSSMigrationVersion.
    void applyR20SSSMigrationIfNeeded();
    // </FS:AYAstorm>
}

#endif // LL_CINEMATIC_OVERLAY_H
