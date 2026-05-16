# AYAstorm r22 — Release Announcement

Short text intended to be pasted into the GitHub release page. **r22 is a chat-UX release** that splits the Nearby Chat / IM history into two tabs — "human avatar speech" and "System & Object notifications (LSL + system + TP / Region)".

> **Distribution**: r22 ships **bundled with the r23 release** (no standalone r22 tag). The r23 release page links back to this note and to the r22 spec.

Implementation details / known limits / configuration reference live in the permanent spec (`docs/ayastorm-r22-chat-tab-spec.md`). This note is link-only + diff highlights.

---

## AYAstorm r22 — Chat tab split (Human vs System & Object)

### Headline: keep conversations and notifications from clobbering each other

Through r21, Nearby Chat / IM history was a single widget where LSL warnings, ads, HUD notifications, TP arrivals, and region restart notices interleaved with human-to-human conversation and broke the flow. Hiding them outright would cause missed notifications, so r22 instead **splits them into two tabs** so neither category drowns the other out.

- `[Human]` tab: avatar speech (conversation) only
- `[System & Object]` tab: LSL `llSay` / `llRegionSay` / `IM_FROM_TASK` / system messages / TP / region notices — everything that isn't a conversation
- The inactive tab shows an **unread count badge `(N)`** (resets to 0 on tab switch, session-only)
- Your own Local Chat speech is recorded in the **Human tab**
- The input field is shared (sends to Local Chat regardless of which tab is visible)

Tabs apply to both Nearby Chat (all three FS V1 / V7 / LL styles) and IM (1:1). Group IM has no Object-speech path, so it uses the Human tab only.

Label order is `System & Object` (not `Object & System`).

Details → spec `docs/ayastorm-r22-chat-tab-spec.md`

### Friend online notifications also land in the Human tab (new cvar)

Friend online / offline notifications carry a "person I might want to talk to has arrived" signal, so they are **also routed to the Human tab** (in addition to System & Object). This means users keeping their eye on the Human tab don't miss when a friend logs in.

- Controlled by `FSFriendOnlineToHumanTab` (Boolean, default **ON**)
- Setting it to OFF stops the Human duplication; the notification still appears in System & Object
- This works independently of the existing SL cvar `OnlineOfflinetoNearbyChat` (default OFF, controls whether the notification reaches Nearby Chat at all) — with r22's tab split enabled, friend online/offline messages now reach Nearby Chat through this dedicated path

### History file format: single file, suffix marker for routing

`chat_*.txt` keeps the same one-file structure as r21, with a **suffix marker** appended to each line that tells the loader which tab (Human / System & Object) the line belongs to.

- Because the marker is at end-of-line, opening the same file in upstream Firestorm just shows **a trailing string** (parsing is unaffected)
- AYAstorm strips the marker before appending to the history widget, so it **never leaks to the user's screen**
- Legacy history (without markers) falls back to the Human tab
- Past friend online/offline notifications loaded at startup only appear in System & Object (the Human-duplicate flag is a live-routing decision and is not restored on history reload)

### Startup display color

Behaves identically to upstream Firestorm:

- History loaded at startup is rendered in **`ChatHistoryTextColorPersisted` (grey)** for both tabs
- Live receives during the session use `ChatHistoryTextColor` (normal color)
- Tab switching is **visible/invisible swap only** — widget contents are never touched, so grey-fading or color shuffling cannot occur

### Settings

Located in `Preferences → Chat → Chat Windows`:

| Key | Default | Purpose |
|---|---|---|
| `FSChatHumanObjectTabs` | `1` (ON) | Master switch for tab split. `0` reverts to the pre-r22 single-widget behaviour entirely (doubles as the escape hatch) |
| `FSFriendOnlineToHumanTab` | `1` (ON) | Also duplicate friend online/offline notifications into the Human tab. They always appear in the System & Object tab regardless |

In the UI, `FSFriendOnlineToHumanTab` is rendered as an indented child of the master switch and auto-greys out when the master is OFF.

> **A separate escape-hatch cvar is intentionally not provided.** Setting the master cvar to false already restores the pre-r22 behaviour, so an additional key would only add noise (per memory `feedback_prefer_defaults_over_config.md` — prefer one sensible default over many tuning knobs).

Changing `FSChatHumanObjectTabs` or `AYAChatWindowStyle` (V1 / V7 / LL switch) raises a **restart-required modal** and auto-closes any old-style IM container (M4-extra). `FSFriendOnlineToHumanTab` applies live without a restart.

### LL style notifications now consistent across all three styles (bundled in M8)

A latent upstream Firestorm bug surfaced during implementation and is fixed in the same release:

- The **TP arrival separator** (the divider around "Teleport from secondlife://... completed") now shows in Nearby Chat (System & Object tab) for **all three styles — FS V1 / V7 / LL**
- **TP completion notices / region simulator version mismatches / RLV system tips** and other messages routed through `ChatSystemMessageTip` now also reach LL style
- The `LLFloaterIMNearbyChat` dispatch that was commented out upstream in 2021 has been restored with `findTypedInstance` guards, running in parallel with the existing `FSFloaterNearbyChat` dispatch

### Unread badges now bump on TP arrival too

The unread badge introduced in M5 reused the `do_not_log` flag for suppression, which incorrectly silenced bumps for "no-history but fresh runtime" events like the TP separator. M8 introduces a dedicated `is_replay` flag for history-reload paths so the two concerns are properly separated.

### Known limitations

- **Unread badges are session-only**: They reset on restart. We deliberately do not persist the count to the history file (re-opening history surfaces the messages anyway, so a persisted count adds little value).
- **HUD allow list (route your own HUD output to Human) is out of scope for r22**: The "notifications from a HUD I attached should count as human" request is parked for r23+. Currently HUD `llSay` output is also routed to the System & Object tab.
- **Past friend online/offline notifications are not duplicated to Human on history reload**: The duplication is a live-routing decision; reloaded history only appears in System & Object.
- **`panel_nearby_chat.xml` is not touched**: The M3 structural survey found it dead / unreferenced, so the implementation only modifies `floater_fs_nearby_chat.xml` and `floater_im_session.xml` (via `tab_container`).

### Documentation

- r22 spec / GUI design / data layer / acceptance criteria / risk register: `docs/ayastorm-r22-chat-tab-spec.md`
- History file suffix marker format: spec §5 (`docs/ayastorm-r22-chat-tab-spec.md#5-データ層`)
- Routing table (which source type goes where): spec §4
- Friend online/offline exception path implementation: spec §4 "Friend online/offline exception"
- AYAChatWindowStyle (V1 / V7 / LL switching) restart-modal / floater auto-close: spec §6 (M4-extra)
