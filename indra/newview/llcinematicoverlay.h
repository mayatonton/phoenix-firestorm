/**
 * @file llcinematicoverlay.h
 * @brief AYAstorm r30 BD full port Phase 5 R2: Cinematic mode BD-parity cvar overlay.
 *
 * Loads `app_settings/settings_cinematic_bd.xml` (flat LLSD map of
 * cvar -> BD canonical value) and applies those values into `gSavedSettings`
 * as unsaved session values (LLControlVariable::setValue(v, false)) when
 * AYAVisualRealismEnabled == 2 (Cinematic).
 *
 * Application policy (session-only, no persistence):
 *   - Startup with mode 2       -> apply overlay as session values
 *   - Mode switch 0/1 -> 2      -> apply overlay as session values
 *   - Mode switch 2 -> 0/1      -> revert to user saved values
 *
 * Session values live at LLControlVariable::mValues[2+] and are structurally
 * excluded from getSaveValue()/saveToFile, so user settings can never be
 * contaminated regardless of crash timing. While the overlay is active, a
 * validate-signal guard on every overlay cvar rewrites any persistent
 * setValue attempt (floater sliders, D-buttons, quickprefs, command line)
 * into a session setValue, so mode-2 tunings are session-scoped by design.
 *
 * The legacy persistent sentinel `AYACinematicOverlayApplied` is retired; a
 * one-shot startup migration detects it and heals previously contaminated
 * user settings (evidence-logged resetToDefault of every overlay key).
 *
 * Architecture rationale: see
 *   docs/specs/ayastorm-r30-p5-bd-ui-binding-audit-spec.md §3.4
 */
#ifndef LL_CINEMATIC_OVERLAY_H
#define LL_CINEMATIC_OVERLAY_H

namespace LLCinematicOverlay
{
    // Apply BD-parity values from settings_cinematic_bd.xml as unsaved
    // session values and arm the session guard.
    void applyCinematicOverlay();

    // Runs the contamination-heal migration, then applies the overlay when
    // AYAVisualRealismEnabled == 2. Safe to call from initConfiguration.
    void applyCinematicOverlayIfNeeded();

    // Disarm the session guard and restore every overlay cvar to its user
    // saved value (normalizes the value stack to [default, saved]).
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
}

#endif // LL_CINEMATIC_OVERLAY_H
