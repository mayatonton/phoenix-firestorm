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

    // <FS:AYAstorm> r30 release: View Mode picker reshuffle migration. Runs once
    // on startup: when AYAViewModeMigrationVersion < 1, rewrites persisted
    // AYAVisualRealismEnabled value 1 (legacy AYAstorm View) to 2 (the new
    // AYAstorm View, formerly Cinematic) and bumps the version counter. Lifts
    // users from the previous release into the new engine without touching
    // their picker selection. See settings.xml entry for
    // AYAViewModeMigrationVersion.
    void applyAYAViewModeMigrationIfNeeded();
    // </FS:AYAstorm>

    // <FS:AYAstorm> r30 BD改善: r15 Godrays Cinematic opt-in default flip.
    // Pre: AYAR15GodraysInCinematicEnabled default OFF (live A/B 期間)
    // Post: default ON (BD改善 phase で「Cinematic でも godrays 標準」と確定)
    // Runs once: when AYAR15GodraysCinematicMigrationVersion < 1, forces the
    // cvar to true regardless of persisted value, then bumps the version.
    // 既存ユーザーが live A/B 期間中 false 持ちで放置していたケースを救済する。
    void applyR15GodraysCinematicMigrationIfNeeded();
    // </FS:AYAstorm>

    // <FS:AYAstorm> r31.2 Cinematic overlay RenderGlowMinLuminance bugfix.
    // Pre: 0.0 (BD parity port から持ち込み、blank texture + 色付きプリムで
    //            意図しない bloom 発火が起きる)
    // Post: 0.5 (HDR linear 空間で中間 lit を切る現実的な threshold)
    // Runs once per Cinematic-mode startup: when AYAR31GlowMinLuminanceMigrationVersion < 1
    // AND AYAVisualRealismEnabled == 2, forces the cvar to 0.5 regardless of
    // persisted value, then bumps the version. Firestorm mode 起動時はスキップし
    // 次回 Cinematic 起動まで持ち越し (Firestorm mode のみ使うユーザーは LL default
    // 1.0 のままで影響を受けない)。
    void applyR31GlowMinLuminanceMigrationIfNeeded();
    // </FS:AYAstorm>
}

#endif // LL_CINEMATIC_OVERLAY_H
