// =====================================================================
//  aya_3dstream_setup.lsl — AYAstorm 3D Stream tag editor (r12.1 slim)
//
//  Drop into the ROOT prim of a multi-prim linkset. Touch a prim to
//  configure its [3dstream-stereo:{...}] tag. Owner-only.
//
//  Tag form (canonical short keys emitted by this script):
//      [3dstream-stereo:{url:..}{ch:K}{range:N}{volume:V}
//                       {bin:on|off}{v:NAME}{wg:G}{upmix:on|off}{lg:G}]
//
//  See doc/spec_dist_stereo*.md for full field semantics. r12.1 added
//  {lg:G} (LFE gain multiplier, [0.0-3.0]; 1.0=passthrough, 2.0=+6dB)
//  and a setup-helper hover-text overlay (Float Text on/off).
//
//  This script lives close to the LSL Mono 64 KB script-memory cap.
//  Body text in dialogs is intentionally minimal, M_* mode codes are
//  integers (not strings), and there are no global button-list arrays
//  — all literals are inlined into their show*() function so they
//  exist only on the (transient) call stack, not in the global heap.
// =====================================================================

string  TAG_PREFIX  = "[3dstream-stereo:";
string  ALT_PREFIX  = "[ayastream-stereo:";
string  TAG_SUFFIX  = "]";

integer DIALOG_CHAN = -91234567;

// Mode codes — integer, not string. Each `string M_X = "NAME"` would
// add ~one heap object per global; 19 of them measurably push toward
// Stack-Heap Collision on a script of this size.
integer M_ROOT           = 1;
integer M_CHILD          = 2;
integer M_CH             = 3;
integer M_RANGE          = 4;
integer M_VOLUME         = 5;
integer M_URL            = 6;
integer M_RANGE_CUSTOM   = 7;
integer M_VOL_CUSTOM     = 8;
integer M_BINAURAL       = 9;
integer M_VENUE          = 10;
integer M_WETGAIN        = 11;
integer M_WETGAIN_CUSTOM = 12;
integer M_UPMIX          = 13;
integer M_LFEGAIN        = 14;
integer M_LFEGAIN_CUSTOM = 15;
integer M_MORE           = 16;
integer M_FLOAT_TEXT     = 17;
integer M_CONFIRM        = 18;

key      gUser;
integer  gLink;
integer  gMode;
integer  gFloatText;

// ---------- Tag parsing ----------

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

integer findTagEnd(string s, integer start)
{
    integer i = llSubStringIndex(llGetSubString(s, start, -1), TAG_SUFFIX);
    if (i == -1) return -1;
    return start + i;
}

string normalizeKey(string k)
{
    if (k == "bin") return "binaural";
    if (k == "v")   return "venue";
    if (k == "wg")  return "wetgain";
    if (k == "lg")  return "lfegain";
    return k;
}

string normalizeVenueValue(string v)
{
    if (v == "d")  return "dry";
    if (v == "rs") return "room_small";
    if (v == "rm") return "room_medium";
    if (v == "hs") return "hall_small";
    if (v == "hm") return "hall_medium";
    if (v == "hl") return "hall_large";
    if (v == "cl") return "club";
    if (v == "ct") return "cathedral";
    if (v == "od") return "outdoor";
    return v;
}

string emitShortKey(string k)
{
    if (k == "binaural") return "bin";
    if (k == "venue")    return "v";
    if (k == "wetgain")  return "wg";
    if (k == "lfegain")  return "lg";
    return k;
}

string emitShortVenueValue(string v)
{
    if (v == "dry")         return "d";
    if (v == "room_small")  return "rs";
    if (v == "room_medium") return "rm";
    if (v == "hall_small")  return "hs";
    if (v == "hall_medium") return "hm";
    if (v == "hall_large")  return "hl";
    if (v == "club")        return "cl";
    if (v == "cathedral")   return "ct";
    if (v == "outdoor")     return "od";
    return v;
}

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
            k = normalizeKey(k);
            if (k == "venue") v = normalizeVenueValue(v);
            out += [k, v];
        }
        i = cb + 1;
    }
    return out;
}

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
    integer i;
    list order = ["url", "ch", "range", "volume",
                  "binaural", "venue", "wetgain", "upmix", "lfegain"];
    list seen  = [];
    for (i = 0; i < llGetListLength(order); ++i)
    {
        string k = llList2String(order, i);
        string v = getField(fields, k);
        if (v != "")
        {
            string ek = emitShortKey(k);
            string ev = v;
            if (k == "venue") ev = emitShortVenueValue(v);
            body += "{" + ek + ":" + ev + "}";
            seen += [k];
        }
    }
    integer n = llGetListLength(fields);
    for (i = 0; i < n; i += 2)
    {
        string k = llList2String(fields, i);
        if (llListFindList(seen, [k]) == -1)
        {
            string v = llList2String(fields, i + 1);
            string ek = emitShortKey(k);
            string ev = v;
            if (k == "venue") ev = emitShortVenueValue(v);
            body += "{" + ek + ":" + ev + "}";
        }
    }
    return body;
}

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
    if (end == -1) return newTag;
    string before = "";
    if (start > 0) before = llGetSubString(desc, 0, start - 1);
    string after = "";
    if (end < llStringLength(desc) - 1) after = llGetSubString(desc, end + 1, -1);
    if (newTag == "")
    {
        string joined = before + after;
        while (llSubStringIndex(joined, "  ") != -1)
        {
            integer dp = llSubStringIndex(joined, "  ");
            joined = llGetSubString(joined, 0, dp - 1) + " " + llGetSubString(joined, dp + 2, -1);
        }
        return llStringTrim(joined, STRING_TRIM);
    }
    return before + newTag + after;
}

list readFields(integer link)
{
    string desc = llList2String(llGetLinkPrimitiveParams(link, [PRIM_DESC]), 0);
    integer start = findTagStart(desc);
    if (start == -1) return [];
    integer plen = findTagPrefixLen(desc, start);
    integer end = findTagEnd(desc, start);
    if (end == -1) return [];
    return parseFields(llGetSubString(desc, start + plen, end - 1));
}

integer writeFields(integer link, list fields)
{
    string desc = llList2String(llGetLinkPrimitiveParams(link, [PRIM_DESC]), 0);
    string newTag = "";
    if (llGetListLength(fields) > 0)
    {
        newTag = TAG_PREFIX + buildTagBody(fields) + TAG_SUFFIX;
    }
    string newDesc = spliceTag(desc, newTag);
    if (llStringLength(newDesc) > 127) return 0;
    llSetLinkPrimitiveParamsFast(link, [PRIM_DESC, newDesc]);
    return 1;
}

integer countSpeakers(integer excludeLink)
{
    integer total = llGetNumberOfPrims();
    integer link;
    integer count = 0;
    for (link = 1; link <= total; ++link)
    {
        if (link == excludeLink) jump skip;
        string ch = getField(readFields(link), "ch");
        if (ch != "" && llToUpper(ch) != "NONE") ++count;
        @skip;
    }
    return count;
}

// One-line summary (replaces the older multi-line fmtCurrent — same
// info, ~5x less literal text in the binary).
string summary(integer link)
{
    list fields = readFields(link);
    if (llGetListLength(fields) == 0) return "(no tag)";
    string s = "";
    string url = getField(fields, "url");
    string ch  = getField(fields, "ch");
    string rng = getField(fields, "range");
    string vol = getField(fields, "volume");
    if (url != "") s += "url=" + url + " ";
    if (ch  != "") s += "ch=" + ch + " ";
    if (rng != "") s += "rng=" + rng + " ";
    if (vol != "") s += "vol=" + vol + " ";
    string bin = getField(fields, "binaural");
    string ven = getField(fields, "venue");
    string wet = getField(fields, "wetgain");
    string up  = getField(fields, "upmix");
    string lg  = getField(fields, "lfegain");
    if (bin != "") s += "bin=" + bin + " ";
    if (ven != "") s += "v=" + ven + " ";
    if (wet != "") s += "wg=" + wet + " ";
    if (up  != "") s += "up=" + up + " ";
    if (lg  != "") s += "lg=" + lg;
    return s;
}

string getCur(integer link, string fkey)
{
    string v = getField(readFields(link), fkey);
    if (v == "") return "(none)";
    return v;
}

refreshFloatText()
{
    integer total = llGetNumberOfPrims();
    integer link;
    for (link = 1; link <= total; ++link)
    {
        string txt = "";
        float alpha = 0.0;
        if (gFloatText)
        {
            list f = readFields(link);
            if (llGetListLength(f) > 0)
            {
                txt = "L" + (string)link;
                string ch  = getField(f, "ch");
                string vol = getField(f, "volume");
                string rng = getField(f, "range");
                if (ch  != "") txt += " ch=" + ch;
                if (vol != "") txt += " v=" + vol;
                if (rng != "") txt += " r=" + rng;
                alpha = 1.0;
            }
        }
        llSetLinkPrimitiveParamsFast(link, [PRIM_TEXT, txt, <1,1,1>, alpha]);
    }
}

// ---------- Dialog plumbing ----------

clearMenu()
{
    gMode = 0;
    gUser = NULL_KEY;
    gLink = 0;
}

saveErr()
{
    llRegionSayTo(gUser, 0, "Save failed: Description over 127 bytes.");
}

saved()
{
    llRegionSayTo(gUser, 0, "Saved. AYAstorm picks up changes within 30s.");
}

// ---------- show*() ----------

showRoot()
{
    gMode = M_ROOT;
    llDialog(gUser,
        "ROOT prim\n" + summary(1)
        + "\nSpeakers: " + (string)countSpeakers(0),
        ["Start", "Stop", "URL",
         "Volume", "Range", "Ch",
         "Binaural", "Venue", "More...",
         "Remove Tag", "Close"],
        DIALOG_CHAN);
}

showMore()
{
    gMode = M_MORE;
    string ft = "off"; if (gFloatText) ft = "on";
    llDialog(gUser,
        "More\n" + summary(1) + "\nFloat Text: " + ft,
        ["WetGain", "Upmix", "LfeGain", "Float Text", "Back"],
        DIALOG_CHAN);
}

showFloatText()
{
    gMode = M_FLOAT_TEXT;
    string c = "off"; if (gFloatText) c = "on";
    llDialog(gUser, "Hover text: " + c, ["On", "Off", "Back"], DIALOG_CHAN);
}

showChild()
{
    gMode = M_CHILD;
    llDialog(gUser,
        "Child #" + (string)gLink + "\n" + summary(gLink),
        ["Volume", "Range", "Ch", "Remove Tag", "Close"],
        DIALOG_CHAN);
}

showCh()
{
    gMode = M_CH;
    llDialog(gUser, "Ch (link " + (string)gLink + "): " + getCur(gLink, "ch"),
        ["L", "R", "M",
         "FL", "FR", "C",
         "LFE", "SL", "SR",
         "None", "Back"],
        DIALOG_CHAN);
}

showRange()
{
    gMode = M_RANGE;
    llDialog(gUser, "Range m: " + getCur(gLink, "range"),
        ["5", "10", "20", "30", "50", "80", "Custom", "Back"],
        DIALOG_CHAN);
}

showVolume()
{
    gMode = M_VOLUME;
    llDialog(gUser, "Volume: " + getCur(gLink, "volume"),
        ["0.25", "0.50", "0.75", "1.00", "1.50", "2.00", "Custom", "Back"],
        DIALOG_CHAN);
}

showUrl()
{
    gMode = M_URL;
    llTextBox(gUser, "URL (http/https). Empty=cancel.\n" + getCur(1, "url"),
        DIALOG_CHAN);
}

showRangeCustom()
{
    gMode = M_RANGE_CUSTOM;
    llTextBox(gUser, "Range (meters). Empty=cancel.", DIALOG_CHAN);
}

showVolumeCustom()
{
    gMode = M_VOL_CUSTOM;
    llTextBox(gUser, "Volume 0.0-4.0. Empty=cancel.", DIALOG_CHAN);
}

showBinaural()
{
    gMode = M_BINAURAL;
    llDialog(gUser, "Binaural: " + getCur(1, "binaural"),
        ["On", "Off", "Default", "Back"], DIALOG_CHAN);
}

showVenue()
{
    gMode = M_VENUE;
    llDialog(gUser, "Venue: " + getCur(1, "venue"),
        ["dry", "room_small", "room_medium",
         "hall_small", "hall_medium", "hall_large",
         "club", "cathedral", "outdoor",
         "Default", "Back"],
        DIALOG_CHAN);
}

showWetGain()
{
    gMode = M_WETGAIN;
    llDialog(gUser, "Wetgain: " + getCur(1, "wetgain"),
        ["0.1", "0.2", "0.3", "0.4", "0.5", "Custom", "Default", "Back"],
        DIALOG_CHAN);
}

showWetGainCustom()
{
    gMode = M_WETGAIN_CUSTOM;
    llTextBox(gUser, "Wetgain 0.0-2.0 (musical range 0.1-0.5). Empty=cancel.", DIALOG_CHAN);
}

showLfeGain()
{
    gMode = M_LFEGAIN;
    llDialog(gUser, "LFE gain: " + getCur(1, "lfegain"),
        ["0.5", "1.0", "1.5", "2.0", "3.0", "Custom", "Default", "Back"],
        DIALOG_CHAN);
}

showLfeGainCustom()
{
    gMode = M_LFEGAIN_CUSTOM;
    llTextBox(gUser, "LFE gain 0.0-3.0. Empty=cancel.", DIALOG_CHAN);
}

showUpmix()
{
    gMode = M_UPMIX;
    llDialog(gUser, "Upmix: " + getCur(1, "upmix"),
        ["On", "Off", "Default", "Back"], DIALOG_CHAN);
}

// ---------- Action handlers ----------

applyField(integer link, string fkey, string val)
{
    list fields = readFields(link);
    if (val == "") fields = dropField(fields, fkey);
    else           fields = setField(fields, fkey, val);
    if (writeFields(link, fields) == 0) { saveErr(); return; }
    saved();
    if (gFloatText) refreshFloatText();
}

backTo()
{
    if (gLink == 1) showRoot();
    else            showChild();
}

handleCh(string ch)
{
    if (ch == "Back") { backTo(); return; }
    if (ch == "None")
    {
        if (countSpeakers(gLink) == 0)
        {
            gMode = M_CONFIRM;
            llDialog(gUser, "Cannot: would leave 0 speakers.", ["Cancel"], DIALOG_CHAN);
            return;
        }
        list f = readFields(gLink);
        f = dropField(f, "ch");
        if (writeFields(gLink, f) == 0) { saveErr(); clearMenu(); return; }
        saved();
        clearMenu();
        return;
    }
    applyField(gLink, "ch", ch);
    clearMenu();
}

handleStop()
{
    list f = readFields(1);
    if (getField(f, "url") == "") { clearMenu(); return; }
    f = dropField(f, "url");
    if (writeFields(1, f) == 0) { saveErr(); clearMenu(); return; }
    saved();
    clearMenu();
}

handleStart()
{
    string cur = getField(readFields(1), "url");
    if (cur != "")
    {
        llRegionSayTo(gUser, 0, "Already configured (url=" + cur + "). Use URL or Stop.");
        clearMenu();
        return;
    }
    showUrl();
}

handleRange(string label)
{
    if (label == "Back")   { backTo(); return; }
    if (label == "Custom") { showRangeCustom(); return; }
    applyField(gLink, "range", label);
    clearMenu();
}

handleVolume(string label)
{
    if (label == "Back")   { backTo(); return; }
    if (label == "Custom") { showVolumeCustom(); return; }
    applyField(gLink, "volume", label);
    clearMenu();
}

handleRangeCustom(string text)
{
    text = llStringTrim(text, STRING_TRIM);
    if (text == "") { clearMenu(); return; }
    if ((float)text <= 0.0)
    {
        llRegionSayTo(gUser, 0, "Range must be > 0.");
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
        llRegionSayTo(gUser, 0, "Volume 0.0-4.0.");
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
        llRegionSayTo(gUser, 0, "URL needs http:// or https://");
        clearMenu();
        return;
    }
    applyField(1, "url", text);
    clearMenu();
}

// Drop a root-only field; takes the field key + which menu to return to
// on Default/Back. Replaces 5 nearly-identical handle*() bodies.
dropAndDone(string fkey)
{
    list f = readFields(1);
    f = dropField(f, fkey);
    if (writeFields(1, f) == 0) { saveErr(); clearMenu(); return; }
    saved();
    clearMenu();
}

handleBinaural(string label)
{
    if (label == "Back")    { showRoot(); return; }
    if (label == "Default") { dropAndDone("binaural"); return; }
    string val = "";
    if (label == "On")  val = "on";
    else if (label == "Off") val = "off";
    else { clearMenu(); return; }
    applyField(1, "binaural", val);
    clearMenu();
}

handleVenue(string label)
{
    if (label == "Back")    { showRoot(); return; }
    if (label == "Default") { dropAndDone("venue"); return; }
    applyField(1, "venue", label);
    clearMenu();
}

handleWetGain(string label)
{
    if (label == "Back")    { showMore(); return; }
    if (label == "Custom")  { showWetGainCustom(); return; }
    if (label == "Default") { dropAndDone("wetgain"); return; }
    applyField(1, "wetgain", label);
    clearMenu();
}

handleWetGainCustom(string text)
{
    text = llStringTrim(text, STRING_TRIM);
    if (text == "") { clearMenu(); return; }
    float v = (float)text;
    if (v < 0.0 || v > 2.0)
    {
        llRegionSayTo(gUser, 0, "Wetgain 0.0-2.0.");
        clearMenu();
        return;
    }
    applyField(1, "wetgain", text);
    clearMenu();
}

handleUpmix(string label)
{
    if (label == "Back")    { showMore(); return; }
    if (label == "Default") { dropAndDone("upmix"); return; }
    string val = "";
    if (label == "On")  val = "on";
    else if (label == "Off") val = "off";
    else { clearMenu(); return; }
    applyField(1, "upmix", val);
    clearMenu();
}

handleLfeGain(string label)
{
    if (label == "Back")    { showMore(); return; }
    if (label == "Custom")  { showLfeGainCustom(); return; }
    if (label == "Default") { dropAndDone("lfegain"); return; }
    applyField(1, "lfegain", label);
    clearMenu();
}

handleLfeGainCustom(string text)
{
    text = llStringTrim(text, STRING_TRIM);
    if (text == "") { clearMenu(); return; }
    float v = (float)text;
    if (v < 0.0 || v > 3.0)
    {
        llRegionSayTo(gUser, 0, "LFE gain 0.0-3.0.");
        clearMenu();
        return;
    }
    applyField(1, "lfegain", text);
    clearMenu();
}

handleFloatText(string label)
{
    if (label == "Back") { showMore(); return; }
    if (label == "On")  gFloatText = 1;
    else if (label == "Off") gFloatText = 0;
    else { clearMenu(); return; }
    refreshFloatText();
    clearMenu();
}

handleRemoveTag()
{
    list f = readFields(gLink);
    string ch = getField(f, "ch");
    integer isSpk = (ch != "" && llToUpper(ch) != "NONE");
    if (isSpk && countSpeakers(gLink) == 0)
    {
        gMode = M_CONFIRM;
        llDialog(gUser, "Cannot: this is the only speaker.", ["Cancel"], DIALOG_CHAN);
        return;
    }
    if (writeFields(gLink, []) == 0) { saveErr(); clearMenu(); return; }
    llRegionSayTo(gUser, 0, "Tag removed.");
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

    on_rez(integer p) { clearMenu(); }

    touch_start(integer n)
    {
        gUser = llDetectedKey(0);
        integer link = llDetectedLinkNumber(0);
        if (link == 0) link = 1;
        gLink = link;
        if (link == 1) showRoot();
        else           showChild();
    }

    listen(integer chan, string name, key id, string msg)
    {
        if (id != gUser) return;

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
            if (msg == "More...")    { showMore(); return; }
            if (msg == "Remove Tag") { handleRemoveTag(); return; }
            return;
        }
        if (gMode == M_MORE)
        {
            if (msg == "Back")       { showRoot(); return; }
            if (msg == "WetGain")    { showWetGain(); return; }
            if (msg == "Upmix")      { showUpmix(); return; }
            if (msg == "LfeGain")    { showLfeGain(); return; }
            if (msg == "Float Text") { showFloatText(); return; }
            return;
        }
        if (gMode == M_CHILD)
        {
            if (msg == "Close")      { clearMenu(); return; }
            if (msg == "Volume")     { showVolume(); return; }
            if (msg == "Range")      { showRange(); return; }
            if (msg == "Ch")         { showCh(); return; }
            if (msg == "Remove Tag") { handleRemoveTag(); return; }
            return;
        }
        if (gMode == M_CH)              { handleCh(msg); return; }
        if (gMode == M_RANGE)           { handleRange(msg); return; }
        if (gMode == M_VOLUME)          { handleVolume(msg); return; }
        if (gMode == M_RANGE_CUSTOM)    { handleRangeCustom(msg); return; }
        if (gMode == M_VOL_CUSTOM)      { handleVolumeCustom(msg); return; }
        if (gMode == M_URL)             { handleUrl(msg); return; }
        if (gMode == M_BINAURAL)        { handleBinaural(msg); return; }
        if (gMode == M_VENUE)           { handleVenue(msg); return; }
        if (gMode == M_WETGAIN)         { handleWetGain(msg); return; }
        if (gMode == M_WETGAIN_CUSTOM)  { handleWetGainCustom(msg); return; }
        if (gMode == M_UPMIX)           { handleUpmix(msg); return; }
        if (gMode == M_LFEGAIN)         { handleLfeGain(msg); return; }
        if (gMode == M_LFEGAIN_CUSTOM)  { handleLfeGainCustom(msg); return; }
        if (gMode == M_FLOAT_TEXT)      { handleFloatText(msg); return; }
        if (gMode == M_CONFIRM)         { clearMenu(); return; }
    }

    changed(integer change)
    {
        if (change & CHANGED_OWNER) llResetScript();
    }
}
