// AYAstorm busy store bench: avatar crowd (OpenSim OSSL NPCs).
//
// The crowd of the wgpu-based viewer's busy store demo bench: 40 avatars
// standing on the plaza among the local lights, each wearing eight 7-prim
// linksets on bones all over the body. Placed in the "bench crowd controller" prim by
// scripts/bench/generate_busy_store_oar.py together with "bench wearable".
//
// NPCs have no inventory, so they cannot be given attachments from a prim's
// contents. They do rez the attachments their appearance lists by asset
// (AttachmentsModule.RezAttachments). The script therefore saves the owner's
// appearance once (OWNER_APPEARANCE: the owner is the estate owner, i.e. the
// bench avatar, and must be in the region with a finished bake), writes a
// copy that lists "bench wearable" at eight attachment points (APPEARANCE),
// and creates the crowd from that copy.
//
// Chat on channel 7 (owner only): "crowd" recreates the crowd, "clear"
// removes it, "appearance" saves the owner's current appearance again.
//
// Needs NPCs enabled with room for 40 per region ([NPC] MaxNumberNPCsPerScene
// of the bench guide) and
// osNpcCreate, osOwnerSaveAppearance, osGetNotecard and osMakeNotecard
// allowed for the estate owner.

integer CHANNEL = 7;
string OWNER_APPEARANCE = "bench owner appearance";
string APPEARANCE = "bench crowd appearance";
string WEARABLE = "bench wearable";
// head, chest, left/right shoulder, left/right hand, left/right foot
list POINTS = [ATTACH_HEAD, ATTACH_CHEST, ATTACH_LSHOULDER, ATTACH_RSHOULDER,
               ATTACH_LHAND, ATTACH_RHAND, ATTACH_LFOOT, ATTACH_RFOOT];
integer COLS = 8;
integer ROWS = 5;
// first row, 14 m ahead of the bench view (128, 96), facing it
vector ORIGIN = <122.4, 110.0, 23.0>;
float DX = 1.6;
float DY = 2.4;
float BAKE_WAIT = 30.0;  // seconds after the owner arrives
// One avatar at a time: each new NPC reads the appearance notecard and rezzes
// eight attachments from the asset store. Creating all 40 in the same instant
// crashed OpenSim's standalone SQLite store (SIGSEGV in sqlite3Prepare).
float SPAWN_INTERVAL = 0.5;

integer gOwnerSeenAt = 0;

clearCrowd()
{
    list npcs = osGetNPCList();
    integer i;
    for (i = 0; i < llGetListLength(npcs); i += 3)
        osNpcRemove(llList2Key(npcs, i));
}

makeCrowd()
{
    clearCrowd();
    rotation facing = llEuler2Rot(<0.0, 0.0, -PI_BY_TWO>);  // look along -Y
    integer n = COLS * ROWS;
    integer i;
    for (i = 0; i < n; ++i)
    {
        integer row = i / COLS;
        vector pos = ORIGIN + <(i % COLS) * DX + (row % 2) * DX / 2.0, row * DY, 0.0>;
        key npc = osNpcCreate("Crowd", (string)(i + 1), pos, APPEARANCE, OS_NPC_CREATOR_OWNED);
        osNpcSetRot(npc, facing);
        llSleep(SPAWN_INTERVAL);
    }
    llOwnerSay("bench crowd: " + (string)n + " avatars");
}

// The owner's appearance with the wearable added at every point of POINTS.
saveAppearance()
{
    if (llGetInventoryType(OWNER_APPEARANCE) == INVENTORY_NOTECARD)
        llRemoveInventory(OWNER_APPEARANCE);
    osOwnerSaveAppearance(OWNER_APPEARANCE);

    string asset = (string)llGetInventoryKey(WEARABLE);
    string entries = "";
    integer p;
    for (p = 0; p < llGetListLength(POINTS); ++p)
        entries += "<map><key>point</key><integer>" + (string)llList2Integer(POINTS, p)
            + "</integer><key>item</key><uuid>" + (string)NULL_KEY
            + "</uuid><key>asset</key><uuid>" + asset + "</uuid></map>";

    string text = osGetNotecard(OWNER_APPEARANCE);
    integer at = llSubStringIndex(text, "<key>attachments</key>");
    if (at < 0)
    {
        llOwnerSay("bench crowd: no attachments entry in the saved appearance");
        return;
    }
    string head = llGetSubString(text, 0, at + 21);
    string tail = llGetSubString(text, at + 22, -1);
    integer empty = llSubStringIndex(tail, "<array />");
    integer open = llSubStringIndex(tail, "<array>");
    if (empty >= 0 && (open < 0 || empty < open))
        tail = llGetSubString(tail, 0, empty - 1) + "<array>" + entries + "</array>"
            + llGetSubString(tail, empty + 9, -1);
    else
        tail = llGetSubString(tail, 0, open + 6) + entries + llGetSubString(tail, open + 7, -1);

    if (llGetInventoryType(APPEARANCE) == INVENTORY_NOTECARD)
        llRemoveInventory(APPEARANCE);
    osMakeNotecard(APPEARANCE, head + tail);
    llSleep(2.0);  // let the notecard asset be stored before the NPCs read it
    llOwnerSay("bench crowd: saved the owner's appearance with the wearables");
}

default
{
    state_entry()
    {
        llListen(CHANNEL, "", llGetOwner(), "");
        // let the region settle (with the notecard), or wait for the owner
        llSetTimerEvent(5.0);
    }

    on_rez(integer p) { llResetScript(); }

    // NPCs do not persist, but the script state does: after an OpenSim
    // restart the timer is already off and no crowd would be made.
    changed(integer change)
    {
        if (change & CHANGED_REGION_START) llResetScript();
    }

    timer()
    {
        if (llGetInventoryType(APPEARANCE) != INVENTORY_NOTECARD)
        {
            if (llGetAgentSize(llGetOwner()) == ZERO_VECTOR)
            {
                gOwnerSeenAt = 0;
                return;
            }
            if (gOwnerSeenAt == 0)
                gOwnerSeenAt = llGetUnixTime();
            if (llGetUnixTime() - gOwnerSeenAt < BAKE_WAIT)
                return;
            saveAppearance();
        }
        llSetTimerEvent(0.0);
        makeCrowd();
    }

    listen(integer channel, string name, key id, string msg)
    {
        if (msg == "crowd") makeCrowd();
        else if (msg == "clear") clearCrowd();
        else if (msg == "appearance") saveAppearance();
    }
}
