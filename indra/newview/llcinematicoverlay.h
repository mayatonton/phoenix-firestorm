/**
 * @file llcinematicoverlay.h
 * @brief AYAstorm r30 BD full port Phase 5 R2: Cinematic mode BD-parity cvar overlay.
 *
 *
 * Architecture rationale: see
 *   docs/specs/ayastorm-r30-p5-bd-ui-binding-audit-spec.md §3.4
 */
#ifndef LL_CINEMATIC_OVERLAY_H
#define LL_CINEMATIC_OVERLAY_H

namespace LLCinematicOverlay
{
    void applyCinematicOverlay();

    void applyCinematicOverlayIfNeeded();

    void revertCinematicOverlay();

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

    // <FS:AYAstorm> RenderVolumetricLightingMultiplier VK default (4.0 -> 0.2)
    // one-shot force overwrite migration (no-op once AYAVolMulMigrationVersion >= 1).
    void applyVolMulMigrationIfNeeded();
    // </FS:AYAstorm>
}

#endif // LL_CINEMATIC_OVERLAY_H
