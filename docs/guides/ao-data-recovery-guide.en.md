# AO Data Recovery Guide (AYAstorm r31.2)

This guide is for users who experienced "I deleted an AO set in a Firestorm-family viewer (upstream Firestorm / older AYAstorm builds / other FS-derived viewers), and the AO was also gone in other viewers."

## Things to know first

### Deleted AO data cannot be brought back

Unfortunately, once a viewer has deleted AO data (animations inside a set, configuration notecards), there is **no way to recover the original data**. It is completely gone from the Second Life servers. Asking Linden Lab will not help either.

However, **you can get your AO working again**. The "What to do 2" section below explains how.

### Why did this happen?

In Second Life's Inventory, the storage location for AO sets is shared across all Firestorm-family viewers. Upstream Firestorm, AYAstorm, and other FS-derived viewers all store AO data in the same place on your SL account.

This was designed so that "the same AO works no matter which viewer you log in with," but the side effect was that "deleting an AO in one viewer also deletes it from every other viewer."

AYAstorm r31.2 fixes this so that "delete" no longer destroys the data.

---

## What to do 1: Install AYAstorm r31.2 or later (prevent recurrence)

1. Download the r31.2 (or later) installer from the AYAstorm web site
2. Install → launch → log in with your SL account
3. From now on, "Delete" in the AO window **keeps the data intact** — it just hides the set

That alone prevents the problem from happening again.

---

## What to do 2: Get AO working again (recovery procedure)

The original AO sets you deleted cannot be restored, but you can get AO functionality working again. Use one of the two methods below.

### A. If you have a notecard backup of your AO

In SL, there is a long-standing habit of backing up AO configuration to notecards. If you have one:

1. Open the Inventory window and locate your AO configuration notecard (usually under `Notecards` or wherever you saved it)
2. Open the AO window in AYAstorm (Avatar menu → Animation Overrider)
3. Click the "+" button to create a new set
4. Drag-and-drop the notecard onto the AO window to import
5. Select the imported set and verify it works

### B. If you don't have a backup (most users)

Get a fresh AO from the SL Marketplace. Free options work fine:

- **Vista Free AO** — long-standing free AO
- **Animare Free AO** — also free
- **ZHAO-II** — the originator of the notecard format
- Search keywords: "AO HUD", "Animation Override", "AO Free"

Once you have one:

1. Purchase from Marketplace (even free items go into your inventory via the "Buy" button)
2. Rez the notecard included with the AO product (drop it on the ground) or pull it out of inventory
3. Open the AO window in AYAstorm → "+" → drag-and-drop the notecard
4. Verify

Your AO will be working again.

### Note: if you believe you lost a Linden-distributed preset AO

If you believe you lost an AO preset originally distributed by Linden Lab (e.g. a Library-based AO) and **specifically want that preset itself restored**, the viewer side cannot recover it — please contact Linden Lab Support.

If your goal is simply to "get AO functionality working again," option B (re-import a free AO) above is sufficient.

---

## What to do 3: Manage hidden sets (new in AYAstorm r31.2)

In AYAstorm r31.2, "Delete" is replaced by "Hide." To inspect and restore hidden sets:

1. Open the AO window
2. Click the **"Manage hidden sets"** button (near the bottom)
3. The list of hidden sets appears
4. To restore one: select the set → "Restore selected"
5. To restore all: "Restore all"

The data is never touched, so these sets remain visible and usable from other viewers (including upstream Firestorm).

---

## A note about using upstream Firestorm

The upstream Firestorm viewer (as of this writing, unpatched) still destroys AO data when you press "Delete" on an AO set.

**If you accidentally deleted an AO in upstream Firestorm**:
→ Use "What to do 2" above to bring AO functionality back.

**Recommended workflow**:
- Do all AO editing / deletion in AYAstorm r31.2 or later
- In upstream Firestorm, just **use** your AO sets — don't delete them
- Sets you "Hide" in AYAstorm are still visible and usable from upstream Firestorm

---

## If you are still stuck

- AYAstorm issue tracker: [GitHub Issues](https://github.com/mayatonton/phoenix-firestorm/issues)
- SL support: Linden Lab cannot recover AO data since it no longer exists on the server

---

For technical details, see [docs/specs/ayastorm-r31-2-ao-bridge-recovery.md](../specs/ayastorm-r31-2-ao-bridge-recovery.md).
