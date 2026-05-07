// =====================================================================
//  aya_3dstream_setup.lsl
//
//  Owner-only touch UI for configuring AYAstorm 3D Stream tags on a
//  multi-prim linkset. Drop this single script into the ROOT prim only.
//
//  Touch ROOT prim  -> configure URL / Volume / range / ch
//  Touch CHILD prim -> configure that child's Volume / range / ch
//
//  Tag form written to each prim's Description (per AYAstorm spec):
//      [3dstream-stereo:{url:...}{ch:K}{range:N}{volume:V}
//                       {binaural:on|off}{venue:NAME}{wetgain:G}
//                       {upmix:on|off}]
//
//  - URL is shared by the linkset; only the speaker(s) carrying {url:}
//    cause AYAstorm to start/stop the stream. By convention this script
//    keeps the URL on the ROOT prim's tag.
//  - {ch:} controls how that prim consumes the stream. Channels:
//        L  R  M                       (stereo / r8)
//        FL FR C  LFE SL SR            (5.1 / r10)
//        None                          (no playback on this prim)
//  - Setting Ch=None on a speaker turns it into a source-only prim.
//    The script refuses to do this if it would leave the linkset with
//    zero speakers (which is the only real "format error" condition).
//  - r11 root-only fields:
//        {binaural:on|off}  lite-HRTF DSP toggle (default = on)
//        {venue:NAME}       convolution reverb (dry/room_*/hall_*/
//                           club/cathedral/outdoor); default = dry
//        {wetgain:G}        venue mix amount (default = 1.0)
//        {upmix:on|off}     2ch→5.1 upmix dispatch toggle (default = off)
//                           — opt-in. When on, a 2-ch source feeds DPL2-
//                           style matrix decode into FL/FR/C/LFE/SL/SR
//                           speakers in the linkset. 5.1 / 6-ch source
//                           always plays native (auto-bypass).
//
//  No URL retention: Stop drops {url:} entirely (no urlsave shadow),
//  Start prompts only when {url:} is absent. What the user wrote is
//  what the Description holds, so a stream that fails to play is a
//  visible signal that the URL or upstream server is the problem,
//  rather than being silently masked by an auto-restored prior URL.
//
//  Tag-only replacement: any text in the prim Description outside the
//  [3dstream-stereo:...] tag is preserved.
//
//  After saving, AYAstorm picks up the change at the next 30-second
//  Description poll (Stream3DPollInterval). The script reminds the user.
//
//  English-only UI by design.
// =====================================================================

// ---------- Constants ----------

string  TAG_PREFIX  = "[3dstream-stereo:";   // canonical
string  ALT_PREFIX  = "[ayastream-stereo:";  // legacy alias (read-only)
string  TAG_SUFFIX  = "]";

// Fixed dialog channel; listener stays open for the script's lifetime.
integer DIALOG_CHAN = -91234567;

// Mode tokens for gMode
string  M_ROOT         = "ROOT";
string  M_CHILD        = "CHILD";
string  M_CH           = "CH";
string  M_RANGE        = "RANGE";
string  M_VOLUME       = "VOLUME";
string  M_URL          = "URL";
string  M_RANGE_CUSTOM = "RANGE_CUSTOM";
string  M_VOL_CUSTOM   = "VOL_CUSTOM";
string  M_BINAURAL     = "BINAURAL";
string  M_VENUE        = "VENUE";
string  M_WETGAIN      = "WETGAIN";
string  M_WETGAIN_CUSTOM = "WETGAIN_CUSTOM";
string  M_UPMIX        = "UPMIX";
string  M_CONFIRM_NONE = "CONFIRM_NONE";
string  M_CONFIRM_RM   = "CONFIRM_RM";

// Channel button labels (must match spec exactly: L R M FL FR C LFE SL SR)
list CH_BUTTONS = [
    "L",   "R",   "M",
    "FL",  "FR",  "C",
    "LFE", "SL",  "SR",
    "None","Back"
];

// Range presets (meters). Custom button opens textbox.
list RANGE_BUTTONS = [
    "5", "10", "20",
    "30", "50", "80",
    "Custom", "Back"
];

// Volume presets (0.0 - 1.0). Custom button opens textbox.
list VOLUME_BUTTONS = [
    "0.25", "0.50", "0.75",
    "1.00", "1.50", "2.00",
    "Custom", "Back"
];

// r11: lite-HRTF on/off. Default = remove tag (viewer default ON).
list BINAURAL_BUTTONS = [
    "On", "Off",
    "Default", "Back"
];

// r11: venue convolution reverb (9 IR slots from spec §4.5).
list VENUE_BUTTONS = [
    "dry",        "room_small", "room_medium",
    "hall_small", "hall_medium","hall_large",
    "club",       "cathedral",  "outdoor",
    "Default",    "Back"
];

// r11: wet gain presets (0.0 - 4.0). Custom button opens textbox.
list WETGAIN_BUTTONS = [
    "0.5",  "1.0", "1.5",
    "2.0",  "Custom",
    "Default", "Back"
];

// r12: 2ch→5.1 upmix dispatch on/off. Default = remove tag (viewer
// default OFF, opt-in). Same shape as BINAURAL_BUTTONS.
list UPMIX_BUTTONS = [
    "On", "Off",
    "Default", "Back"
];

// ---------- Globals ----------

key      gUser;       // toucher (always owner)
integer  gLink;       // link number being edited (1 = root, >=2 = child)
string   gMode;       // current dialog mode
string   gPending;    // pending value across confirm dialogs (e.g. ch name)

// ---------- Tag parsing helpers ----------

// Returns offset of TAG_PREFIX or ALT_PREFIX in s, or -1.
// Out-parameter prefix length must be inferred by the caller.
integer findTagStart(string s)
{
    integer i = llSubStringIndex(s, TAG_PREFIX);
    if (i != -1) return i;
    return llSubStringIndex(s, ALT_PREFIX);
}

integer findTagPrefixLen(string s, integer start)
{
    if (start < 0) return 0;
    string head = llGetSubString(s, start, start + llStringLength(TAG_PREFIX) - 1);
    if (head == TAG_PREFIX) return llStringLength(TAG_PREFIX);
    return llStringLength(ALT_PREFIX);
}

// Find matching closing bracket for the tag starting at 'start'. Returns -1 on failure.
integer findTagEnd(string s, integer start)
{
    integer i = llSubStringIndex(llGetSubString(s, start, -1), TAG_SUFFIX);
    if (i == -1) return -1;
    return start + i;
}

// Read all {key:value} fields inside the tag body.
// Returns strided list [key0, val0, key1, val1, ...].
list parseFields(string body)
{
    list out = [];
    integer n = llStringLength(body);
    integer i = 0;
    while (i < n)
    {
        integer ob = llSubStringIndex(llGetSubString(body, i, -1), "{");
        if (ob == -1) return out;
        ob = i + ob;
        integer cb = llSubStringIndex(llGetSubString(body, ob, -1), "}");
        if (cb == -1) return out;
        cb = ob + cb;
        string inner = llGetSubString(body, ob + 1, cb - 1);
        integer colon = llSubStringIndex(inner, ":");
        if (colon != -1)
        {
            string k = llStringTrim(llGetSubString(inner, 0, colon - 1), STRING_TRIM);
            string v = llStringTrim(llGetSubString(inner, colon + 1, -1), STRING_TRIM);
            out += [k, v];
        }
        i = cb + 1;
    }
    return out;
}

// Search only even-indexed positions so a value that happens to equal
// the searched key cannot produce a false hit.
integer findFieldIndex(list fields, string fkey)
{
    integer n = llGetListLength(fields);
    integer i;
    for (i = 0; i < n; i += 2)
    {
        if (llList2String(fields, i) == fkey) return i;
    }
    return -1;
}

string getField(list fields, string fkey)
{
    integer i = findFieldIndex(fields, fkey);
    if (i == -1) return "";
    return llList2String(fields, i + 1);
}

list setField(list fields, string fkey, string val)
{
    integer i = findFieldIndex(fields, fkey);
    if (i == -1) return fields + [fkey, val];
    return llListReplaceList(fields, [val], i + 1, i + 1);
}

list dropField(list fields, string fkey)
{
    integer i = findFieldIndex(fields, fkey);
    if (i == -1) return fields;
    return llDeleteSubList(fields, i, i + 1);
}

string buildTagBody(list fields)
{
    string body = "";
    integer n = llGetListLength(fields);
    integer i;
    // Preferred field order for readability.
    list order = ["url", "ch", "range", "volume",
                  "binaural", "venue", "wetgain", "upmix"];
    list seen  = [];
    for (i = 0; i < llGetListLength(order); ++i)
    {
        string k = llList2String(order, i);
        string v = getField(fields, k);
        if (v != "")
        {
            body += "{" + k + ":" + v + "}";
            seen += [k];
        }
    }
    for (i = 0; i < n; i += 2)
    {
        string k = llList2String(fields, i);
        if (llListFindList(seen, [k]) == -1)
        {
            string v = llList2String(fields, i + 1);
            body += "{" + k + ":" + v + "}";
        }
    }
    return body;
}

// Splice new tag (or empty string) into description, preserving surrounding text.
string spliceTag(string desc, string newTag)
{
    integer start = findTagStart(desc);
    if (start == -1)
    {
        if (newTag == "") return desc;
        if (desc == "") return newTag;
        return desc + " " + newTag;
    }
    integer end = findTagEnd(desc, start);
    if (end == -1)
    {
        // malformed; replace whole description with new tag
        return newTag;
    }
    string before = "";
    if (start > 0) before = llGetSubString(desc, 0, start - 1);
    string after = "";
    if (end < llStringLength(desc) - 1) after = llGetSubString(desc, end + 1, -1);

    if (newTag == "")
    {
        // remove surrounding single space if it exists, to avoid double space
        string joined = before + after;
        // collapse a run of spaces created by removal
        while (llSubStringIndex(joined, "  ") != -1)
        {
            integer dp = llSubStringIndex(joined, "  ");
            joined = llGetSubString(joined, 0, dp - 1) + " " + llGetSubString(joined, dp + 2, -1);
        }
        return llStringTrim(joined, STRING_TRIM);
    }
    return before + newTag + after;
}

// Read the current fields list for a given link number.
// Returns [] if no tag present.
list readFields(integer link)
{
    string desc = llList2String(
        llGetLinkPrimitiveParams(link, [PRIM_DESC]),
        0
    );
    integer start = findTagStart(desc);
    if (start == -1) return [];
    integer plen = findTagPrefixLen(desc, start);
    integer end = findTagEnd(desc, start);
    if (end == -1) return [];
    string body = llGetSubString(desc, start + plen, end - 1);
    return parseFields(body);
}

// Write the given fields list back to the link's description.
// If fields is empty, removes the tag entirely.
// Returns 1 on success, 0 if the resulting description would exceed 127 bytes.
integer writeFields(integer link, list fields)
{
    string desc = llList2String(
        llGetLinkPrimitiveParams(link, [PRIM_DESC]),
        0
    );
    string newTag = "";
    if (llGetListLength(fields) > 0)
    {
        newTag = TAG_PREFIX + buildTagBody(fields) + TAG_SUFFIX;
    }
    string newDesc = spliceTag(desc, newTag);
    if (llStringLength(newDesc) > 127)
    {
        return 0;
    }
    llSetLinkPrimitiveParamsFast(link, [PRIM_DESC, newDesc]);
    return 1;
}

// Count speakers (prims with {ch:} != "" and != "None") in linkset,
// optionally excluding one link number.
integer countSpeakers(integer excludeLink)
{
    integer total = llGetNumberOfPrims();
    integer link;
    integer count = 0;
    for (link = 1; link <= total; ++link)
    {
        if (link == excludeLink) jump skip;
        list fields = readFields(link);
        string ch = getField(fields, "ch");
        if (ch != "" && llToUpper(ch) != "NONE") ++count;
        @skip;
    }
    return count;
}

// Pretty-print current values for the given link, for menu body.
string fmtCurrent(integer link)
{
    list fields = readFields(link);
    if (llGetListLength(fields) == 0)
    {
        return "(no 3dstream tag set)";
    }
    string url    = getField(fields, "url");
    string ch     = getField(fields, "ch");
    string rng    = getField(fields, "range");
    string vol    = getField(fields, "volume");
    string bin    = getField(fields, "binaural");
    string ven    = getField(fields, "venue");
    string wet    = getField(fields, "wetgain");
    string up     = getField(fields, "upmix");
    string s = "";
    if (url != "") s += "url=" + url + "\n";
    if (ch  != "") s += "ch=" + ch + "  ";
    else           s += "ch=(none)  ";
    if (rng != "") s += "range=" + rng + "m  ";
    if (vol != "") s += "volume=" + vol;
    if (bin != "" || ven != "" || wet != "" || up != "") s += "\n";
    if (bin != "") s += "binaural=" + bin + "  ";
    if (ven != "") s += "venue=" + ven + "  ";
    if (wet != "") s += "wetgain=" + wet + "  ";
    if (up  != "") s += "upmix=" + up;
    return s;
}

string linkLabel(integer link)
{
    if (link == LINK_ROOT || link == 1) return "ROOT prim";
    return "child prim #" + (string)link;
}

// ---------- Dialog plumbing ----------

clearMenu()
{
    gMode    = "";
    gUser    = NULL_KEY;
    gLink    = 0;
    gPending = "";
}

string getCurrentField(integer link, string fkey)
{
    list fields = readFields(link);
    string v = getField(fields, fkey);
    if (v == "") return "(none)";
    return v;
}

// ---------- Menu builders ----------

showRootMenu()
{
    gMode = M_ROOT;
    string body = "ROOT prim setup\n\n"
                + "Current:\n" + fmtCurrent(1)
                + "\n\nLinkset speakers: " + (string)countSpeakers(0)
                + "\n\nChoose a field to edit.";
    list buttons = ["Start", "Stop", "URL",
                    "Volume", "Range", "Ch",
                    "Binaural", "Venue", "WetGain",
                    "Upmix", "Remove Tag", "Close"];
    llDialog(gUser, body, buttons, DIALOG_CHAN);
}

showChildMenu()
{
    gMode = M_CHILD;
    string body = "Child prim setup (link #" + (string)gLink + ")\n\n"
                + "Current:\n" + fmtCurrent(gLink)
                + "\n\nChoose a field to edit.";
    list buttons = ["Volume", "Range", "Ch", "Remove Tag", "Close"];
    llDialog(gUser, body, buttons, DIALOG_CHAN);
}

showCh()
{
    gMode = M_CH;
    string body = "Select channel for " + linkLabel(gLink) + "\n\n"
                + "L/R/M: stereo or mid-mix\n"
                + "FL/FR/C/LFE/SL/SR: 5.1 placement\n"
                + "None: no playback on this prim (source-only)";
    llDialog(gUser, body, CH_BUTTONS, DIALOG_CHAN);
}

showRange()
{
    gMode = M_RANGE;
    string body = "Select range (meters) for " + linkLabel(gLink) + "\n"
                + "Current: " + getCurrentField(gLink, "range") + "m";
    llDialog(gUser, body, RANGE_BUTTONS, DIALOG_CHAN);
}

showVolume()
{
    gMode = M_VOLUME;
    string body = "Select volume for " + linkLabel(gLink) + "\n"
                + "Current: " + getCurrentField(gLink, "volume");
    llDialog(gUser, body, VOLUME_BUTTONS, DIALOG_CHAN);
}

showUrl()
{
    gMode = M_URL;
    string current = getCurrentField(1, "url");
    if (current == "") current = "(empty)";
    llTextBox(gUser,
        "Enter stream URL (http:// or https://). Empty input cancels.\n\nCurrent: " + current,
        DIALOG_CHAN);
}

showRangeCustom()
{
    gMode = M_RANGE_CUSTOM;
    llTextBox(gUser,
        "Enter custom range in meters (positive number). Empty input cancels.",
        DIALOG_CHAN);
}

showVolumeCustom()
{
    gMode = M_VOL_CUSTOM;
    llTextBox(gUser,
        "Enter custom volume (0.0 - 4.0). Empty input cancels.",
        DIALOG_CHAN);
}

// r11: lite-HRTF on/off (root prim only).
showBinaural()
{
    gMode = M_BINAURAL;
    string body = "Set binaural (lite-HRTF) for ROOT prim\n"
                + "Current: " + getCurrentField(1, "binaural") + "\n\n"
                + "On: force lite-HRTF on\n"
                + "Off: disable lite-HRTF (vanilla 3D positioning only)\n"
                + "Default: remove tag (viewer default = on)";
    llDialog(gUser, body, BINAURAL_BUTTONS, DIALOG_CHAN);
}

// r11: venue convolution reverb selection.
showVenue()
{
    gMode = M_VENUE;
    string body = "Select venue reverb for ROOT prim\n"
                + "Current: " + getCurrentField(1, "venue") + "\n\n"
                + "dry: no reverb\n"
                + "room_*: small/medium room\n"
                + "hall_*: concert hall (medium/large = CPU heavy)\n"
                + "club: nightclub\n"
                + "cathedral: long reverb (heaviest)\n"
                + "outdoor: open-air\n"
                + "Default: remove tag (= dry)";
    llDialog(gUser, body, VENUE_BUTTONS, DIALOG_CHAN);
}

// r11: wet gain (venue mix amount).
showWetGain()
{
    gMode = M_WETGAIN;
    string body = "Set wet gain for ROOT prim\n"
                + "Current: " + getCurrentField(1, "wetgain") + "\n\n"
                + "1.0 = unity, higher = more reverb mix";
    llDialog(gUser, body, WETGAIN_BUTTONS, DIALOG_CHAN);
}

showWetGainCustom()
{
    gMode = M_WETGAIN_CUSTOM;
    llTextBox(gUser,
        "Enter custom wetgain (0.0 - 4.0). Empty input cancels.",
        DIALOG_CHAN);
}

// r12: 2ch→5.1 upmix dispatch toggle (root prim only).
showUpmix()
{
    gMode = M_UPMIX;
    string body = "Set upmix (2ch -> 5.1) for ROOT prim\n"
                + "Current: " + getCurrentField(1, "upmix") + "\n\n"
                + "On: dispatch a 2-ch source through DPL2-style matrix\n"
                + "    decode into FL/FR/C/LFE/SL/SR speakers\n"
                + "Off: 2-ch source plays as 2-spk stereo (r11 behavior)\n"
                + "Default: remove tag (viewer default = off, opt-in)\n\n"
                + "Note: 5.1 / 6-ch source ignores this and plays native.";
    llDialog(gUser, body, UPMIX_BUTTONS, DIALOG_CHAN);
}

// ---------- Action handlers ----------

notifySaved()
{
    llRegionSayTo(gUser, 0,
        "Saved. AYAstorm re-reads tags every 30 seconds, "
        + "so playback may take up to half a minute to update.");
}

applyField(integer link, string fkey, string val)
{
    list fields = readFields(link);
    if (val == "") fields = dropField(fields, fkey);
    else           fields = setField(fields, fkey, val);
    if (writeFields(link, fields) == 0)
    {
        llRegionSayTo(gUser, 0,
            "Cannot save: resulting Description would exceed 127 bytes. "
            + "Try a shorter URL or remove other text from the Description.");
        return;
    }
    notifySaved();
}

handleChSelect(string ch)
{
    if (ch == "Back")
    {
        if (gLink == 1) showRootMenu();
        else            showChildMenu();
        return;
    }
    if (ch == "None")
    {
        // Safety: ensure at least one other speaker remains.
        if (countSpeakers(gLink) == 0)
        {
            gMode = M_CONFIRM_NONE;
            gPending = "None";
            llDialog(gUser,
                "WARNING: setting Ch=None on this prim would leave the "
              + "linkset with zero speakers, which AYAstorm treats as a "
              + "format error.\n\n"
              + "Add a speaker on another prim first, or cancel.",
                ["Cancel"], DIALOG_CHAN);
            return;
        }
        list fields = readFields(gLink);
        fields = dropField(fields, "ch");
        if (writeFields(gLink, fields) == 0)
        {
            llRegionSayTo(gUser, 0, "Save failed (description too long).");
            clearMenu();
            return;
        }
        notifySaved();
        clearMenu();
        return;
    }
    applyField(gLink, "ch", ch);
    clearMenu();
}

// Stop: drop {url:} entirely. No urlsave retention — what the publisher
// wrote is what stays in the Description (so a failed re-start is visible
// as silence, not masked by an auto-restored previous URL).
handleStop()
{
    list fields = readFields(1);
    string url = getField(fields, "url");
    if (url == "") { clearMenu(); return; }
    fields = dropField(fields, "url");
    if (writeFields(1, fields) == 0)
    {
        llRegionSayTo(gUser, 0, "Save failed (description too long).");
        clearMenu();
        return;
    }
    notifySaved();
    clearMenu();
}

// Start: if {url:} already present, do nothing (use URL to change it).
// Otherwise prompt the user for a URL. No restore-from-urlsave.
handleStart()
{
    list fields = readFields(1);
    string current = getField(fields, "url");
    if (current != "")
    {
        llRegionSayTo(gUser, 0,
            "Stream is already configured (url=" + current + ").\n"
          + "Use URL to change it, or Stop first.");
        clearMenu();
        return;
    }
    showUrl();
}

handleRange(string label)
{
    if (label == "Back")
    {
        if (gLink == 1) showRootMenu();
        else            showChildMenu();
        return;
    }
    if (label == "Custom")
    {
        showRangeCustom();
        return;
    }
    applyField(gLink, "range", label);
    clearMenu();
}

handleVolume(string label)
{
    if (label == "Back")
    {
        if (gLink == 1) showRootMenu();
        else            showChildMenu();
        return;
    }
    if (label == "Custom")
    {
        showVolumeCustom();
        return;
    }
    applyField(gLink, "volume", label);
    clearMenu();
}

handleRangeCustom(string text)
{
    text = llStringTrim(text, STRING_TRIM);
    if (text == "") { clearMenu(); return; }
    float v = (float)text;
    if (v <= 0.0)
    {
        llRegionSayTo(gUser, 0, "Range must be a positive number.");
        clearMenu();
        return;
    }
    applyField(gLink, "range", text);
    clearMenu();
}

handleVolumeCustom(string text)
{
    text = llStringTrim(text, STRING_TRIM);
    if (text == "") { clearMenu(); return; }
    float v = (float)text;
    if (v < 0.0 || v > 4.0)
    {
        llRegionSayTo(gUser, 0, "Volume must be between 0.0 and 4.0.");
        clearMenu();
        return;
    }
    applyField(gLink, "volume", text);
    clearMenu();
}

handleUrl(string text)
{
    text = llStringTrim(text, STRING_TRIM);
    if (text == "") { clearMenu(); return; }
    if (llSubStringIndex(text, "http://") != 0
     && llSubStringIndex(text, "https://") != 0)
    {
        llRegionSayTo(gUser, 0, "URL must start with http:// or https://");
        clearMenu();
        return;
    }
    applyField(1, "url", text);
    clearMenu();
}

// r11: handlers for binaural/venue/wetgain submenus. All three live on
// the ROOT prim only (spec §4.5/§4.6). "Default" drops the field so the
// viewer falls back to its default (binaural on, venue dry, wetgain 1.0).
handleBinaural(string label)
{
    if (label == "Back") { showRootMenu(); return; }
    if (label == "Default")
    {
        list fields = readFields(1);
        fields = dropField(fields, "binaural");
        if (writeFields(1, fields) == 0)
        {
            llRegionSayTo(gUser, 0, "Save failed (description too long).");
            clearMenu();
            return;
        }
        notifySaved();
        clearMenu();
        return;
    }
    string val = "";
    if (label == "On")  val = "on";
    else if (label == "Off") val = "off";
    else { clearMenu(); return; }
    applyField(1, "binaural", val);
    clearMenu();
}

handleVenue(string label)
{
    if (label == "Back") { showRootMenu(); return; }
    if (label == "Default")
    {
        list fields = readFields(1);
        fields = dropField(fields, "venue");
        if (writeFields(1, fields) == 0)
        {
            llRegionSayTo(gUser, 0, "Save failed (description too long).");
            clearMenu();
            return;
        }
        notifySaved();
        clearMenu();
        return;
    }
    applyField(1, "venue", label);
    clearMenu();
}

handleWetGain(string label)
{
    if (label == "Back") { showRootMenu(); return; }
    if (label == "Custom") { showWetGainCustom(); return; }
    if (label == "Default")
    {
        list fields = readFields(1);
        fields = dropField(fields, "wetgain");
        if (writeFields(1, fields) == 0)
        {
            llRegionSayTo(gUser, 0, "Save failed (description too long).");
            clearMenu();
            return;
        }
        notifySaved();
        clearMenu();
        return;
    }
    applyField(1, "wetgain", label);
    clearMenu();
}

handleWetGainCustom(string text)
{
    text = llStringTrim(text, STRING_TRIM);
    if (text == "") { clearMenu(); return; }
    float v = (float)text;
    if (v < 0.0 || v > 4.0)
    {
        llRegionSayTo(gUser, 0, "Wetgain must be between 0.0 and 4.0.");
        clearMenu();
        return;
    }
    applyField(1, "wetgain", text);
    clearMenu();
}

// r12: upmix on/off/default — same shape as binaural. Tag default in
// the viewer is OFF (opt-in), so "Default" simply drops the field and
// also lets the viewer-side debug Stream3DUpmix sentinel decide if the
// listener has overridden anything.
handleUpmix(string label)
{
    if (label == "Back") { showRootMenu(); return; }
    if (label == "Default")
    {
        list fields = readFields(1);
        fields = dropField(fields, "upmix");
        if (writeFields(1, fields) == 0)
        {
            llRegionSayTo(gUser, 0, "Save failed (description too long).");
            clearMenu();
            return;
        }
        notifySaved();
        clearMenu();
        return;
    }
    string val = "";
    if (label == "On")  val = "on";
    else if (label == "Off") val = "off";
    else { clearMenu(); return; }
    applyField(1, "upmix", val);
    clearMenu();
}

handleRemoveTag()
{
    // Removing a speaker leaves the linkset with no playback prim?
    list fields = readFields(gLink);
    string ch = getField(fields, "ch");
    integer isSpeaker = (ch != "" && llToUpper(ch) != "NONE");
    if (isSpeaker && countSpeakers(gLink) == 0)
    {
        gMode = M_CONFIRM_RM;
        llDialog(gUser,
            "WARNING: this prim is currently the only speaker in the "
          + "linkset. Removing its tag would leave AYAstorm with nothing "
          + "to play (format error).\n\n"
          + "Set up another speaker first, or cancel.",
            ["Cancel"], DIALOG_CHAN);
        return;
    }
    if (writeFields(gLink, []) == 0)
    {
        llRegionSayTo(gUser, 0, "Save failed (description too long).");
        clearMenu();
        return;
    }
    llRegionSayTo(gUser, 0,
        "Tag removed. AYAstorm will stop using this prim within ~30s.");
    clearMenu();
}

// ---------- State ----------

default
{
    state_entry()
    {
        clearMenu();
        llListen(DIALOG_CHAN, "", NULL_KEY, "");
    }

    on_rez(integer p)
    {
        clearMenu();
    }

    touch_start(integer n)
    {
        gUser = llDetectedKey(0);
        integer link = llDetectedLinkNumber(0);
        if (link == 0) link = 1;   // un-linked single prim
        gLink = link;

        if (link == 1) showRootMenu();
        else           showChildMenu();
    }

    listen(integer chan, string name, key id, string msg)
    {
        if (id != gUser) return;

        // ---- Root menu ----
        if (gMode == M_ROOT)
        {
            if (msg == "Close")      { clearMenu(); return; }
            if (msg == "Start")      { handleStart(); return; }
            if (msg == "Stop")       { handleStop(); return; }
            if (msg == "URL")        { showUrl(); return; }
            if (msg == "Volume")     { showVolume(); return; }
            if (msg == "Range")      { showRange(); return; }
            if (msg == "Ch")         { showCh(); return; }
            if (msg == "Binaural")   { showBinaural(); return; }
            if (msg == "Venue")      { showVenue(); return; }
            if (msg == "WetGain")    { showWetGain(); return; }
            if (msg == "Upmix")      { showUpmix(); return; }
            if (msg == "Remove Tag") { handleRemoveTag(); return; }
            return;
        }

        // ---- Child menu ----
        if (gMode == M_CHILD)
        {
            if (msg == "Close")      { clearMenu(); return; }
            if (msg == "Volume")     { showVolume(); return; }
            if (msg == "Range")      { showRange(); return; }
            if (msg == "Ch")         { showCh(); return; }
            if (msg == "Remove Tag") { handleRemoveTag(); return; }
            return;
        }

        // ---- Sub-menus ----
        if (gMode == M_CH)         { handleChSelect(msg);     return; }
        if (gMode == M_RANGE)      { handleRange(msg);        return; }
        if (gMode == M_VOLUME)     { handleVolume(msg);       return; }
        if (gMode == M_RANGE_CUSTOM) { handleRangeCustom(msg); return; }
        if (gMode == M_VOL_CUSTOM)   { handleVolumeCustom(msg); return; }
        if (gMode == M_URL)        { handleUrl(msg);          return; }
        if (gMode == M_BINAURAL)      { handleBinaural(msg);      return; }
        if (gMode == M_VENUE)         { handleVenue(msg);         return; }
        if (gMode == M_WETGAIN)       { handleWetGain(msg);       return; }
        if (gMode == M_WETGAIN_CUSTOM){ handleWetGainCustom(msg); return; }
        if (gMode == M_UPMIX)         { handleUpmix(msg);         return; }

        // ---- Confirm dialogs ----
        if (gMode == M_CONFIRM_NONE) { clearMenu(); return; }
        if (gMode == M_CONFIRM_RM)   { clearMenu(); return; }
    }

    changed(integer change)
    {
        if (change & CHANGED_OWNER) llResetScript();
    }
}
