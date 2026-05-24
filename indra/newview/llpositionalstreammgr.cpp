/**
 * @file llpositionalstreammgr.cpp
 * @brief Manager for prim-bound 3D positional audio streams (AYAstorm).
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Phoenix Firestorm Project, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llpositionalstreammgr.h"

#include "llaudioengine.h"
#include "llaudioengine_fmodstudio.h"
#include "llfasttimer.h"
#include "llocclusiongeometrymgr.h"
#include "llpositionalstream.h"
#include "llpositionalstreammulti.h"
#include "llpositionalstreamstereo.h"
#include "llvenuereverbdsp.h"

#include "llviewercontrol.h"
#include "llpluginaudio.h"
#include "lltextureentry.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewermedia.h"
#include "llvovolume.h"

#include "llagent.h"
#include "llchat.h"
#include "llfloaterimnearbychat.h"
#include "llfloaterreg.h"
#include "llinstantmessage.h"
#include "llnotificationsutil.h"
#include "llselectmgr.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "message.h"

#include <cmath>

#include "llstring.h"
#include "lltimer.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <set>
#include <sstream>

namespace
{
    struct MediaFaceCandidate
    {
        LLUUID object_id;
        S32 link_number = -1;
        S32 face = -1;
        LLVOVolume* volume = nullptr;
    };

    S32 computeLinkNumber(LLViewerObject* root, LLViewerObject* object)
    {
        if (!root || !object)
        {
            return -1;
        }
        const LLViewerObject::child_list_t children = root->getChildren();
        if (object == root)
        {
            return children.empty() ? 0 : 1;
        }

        S32 link_number = 1;
        for (LLViewerObject::child_list_t::const_iterator iter = children.begin();
             iter != children.end();
             ++iter)
        {
            ++link_number;
            if (iter->get() == object)
            {
                return link_number;
            }
        }
        return -1;
    }

    LLVector3 toFloatVec(const LLVector3d& v)
    {
        LLVector3 out;
        out.setVec(v);
        return out;
    }

    LLViewerMediaImpl* findMediaFor3DSource(
        const LLPositionalStreamMgr::SourceBindingKey& source_key)
    {
        if (source_key.media_id.notNull())
        {
            if (LLViewerMediaImpl* media =
                    LLViewerMedia::getInstance()->getMediaImplFromTextureID(
                        source_key.media_id))
            {
                return media;
            }
        }

        if (source_key.media_object_id.notNull() && source_key.face >= 0)
        {
            if (LLViewerObject* object =
                    gObjectList.findObject(source_key.media_object_id))
            {
                if (!object->isDead())
                {
                    if (LLVOVolume* volume = dynamic_cast<LLVOVolume*>(object))
                    {
                        viewer_media_t media =
                            volume->getMediaImpl(static_cast<U8>(source_key.face));
                        if (media.notNull() && media->hasMedia())
                        {
                            return media.get();
                        }
                    }
                }
            }
        }

        return nullptr;
    }

    F32 effectiveDistributedStreamVolume(
        F32 master_volume,
        bool parcel_audible,
        bool media_source,
        bool use_media_volume,
        LLViewerMediaImpl* media)
    {
        if (!parcel_audible)
        {
            return 0.f;
        }
        if (!media_source)
        {
            return master_volume;
        }

        const F32 master = std::clamp(master_volume, 0.f, 1.f);
        if (!use_media_volume)
        {
            return master;
        }

        const F32 media_gain = media ? media->getStream3DAudioGain() : 0.f;
        return master * std::clamp(media_gain, 0.f, 1.f);
    }

    // r10 P5 (§4.4): label for one ChannelKind, used in the routing diagnostic
    // log. Matches the spec sample output exactly so a config check can grep
    // for "ch:FL × 2".
    const char* channelKindLabel(LLPositionalStreamMgr::ChannelKind ch)
    {
        using CK = LLPositionalStreamMgr::ChannelKind;
        switch (ch)
        {
        case CK::L:   return "L";
        case CK::R:   return "R";
        case CK::M:   return "M";
        case CK::FL:  return "FL";
        case CK::FR:  return "FR";
        case CK::C:   return "C";
        case CK::LFE: return "LFE";
        case CK::SL:  return "SL";
        case CK::SR:  return "SR";
        case CK::BL:  return "BL";
        case CK::BR:  return "BR";
        }
        return "?";
    }

    LLPositionalStreamMulti::Channel toMultiChannel(LLPositionalStreamMgr::ChannelKind ch)
    {
        using CK = LLPositionalStreamMgr::ChannelKind;
        using MC = LLPositionalStreamMulti::Channel;
        switch (ch)
        {
        case CK::L:   return MC::L;
        case CK::R:   return MC::R;
        case CK::M:   return MC::M;
        case CK::FL:  return MC::FL;
        case CK::FR:  return MC::FR;
        case CK::C:   return MC::C;
        case CK::LFE: return MC::LFE;
        case CK::SL:  return MC::SL;
        case CK::SR:  return MC::SR;
        case CK::BL:  return MC::BL;
        case CK::BR:  return MC::BR;
        }
        return MC::M;
    }

    bool tryParseFloat(const std::string& s, F32& out)
    {
        if (s.empty())
        {
            return false;
        }
        try
        {
            size_t consumed = 0;
            F32 val = std::stof(s, &consumed);
            if (consumed == 0)
            {
                return false;
            }
            out = val;
            return true;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    bool tryParseNonNegativeS32(const std::string& s, S32& out)
    {
        if (s.empty())
        {
            return false;
        }
        try
        {
            size_t consumed = 0;
            int val = std::stoi(s, &consumed, 10);
            if (consumed != s.size() || val < 0)
            {
                return false;
            }
            out = static_cast<S32>(val);
            return true;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    std::string toLowerAscii(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return s;
    }

    // Case-insensitive substring search (ASCII).
    size_t findCaseInsensitive(const std::string& haystack, const std::string& needle)
    {
        if (needle.empty() || haystack.size() < needle.size())
        {
            return std::string::npos;
        }
        const size_t end = haystack.size() - needle.size();
        for (size_t i = 0; i <= end; ++i)
        {
            bool match = true;
            for (size_t j = 0; j < needle.size(); ++j)
            {
                unsigned char a = static_cast<unsigned char>(haystack[i + j]);
                unsigned char b = static_cast<unsigned char>(needle[j]);
                if (std::tolower(a) != std::tolower(b))
                {
                    match = false;
                    break;
                }
            }
            if (match)
            {
                return i;
            }
        }
        return std::string::npos;
    }

    // Walks a body between [content_start, end) by directly scanning for
    // "{key:value}" blocks and invoking onPair(lowered_key, trimmed_val) for
    // each well-formed unit. Anything between blocks (comma, whitespace, or
    // nothing at all) is treated as a separator, so r5 unifies the previous
    // 3D Stream "comma required" and Parcel Hide "no separator" formats.
    template <typename F>
    void forEachKeyValue(const std::string& description,
                         size_t content_start, size_t end, F&& onPair)
    {
        size_t cursor = content_start;
        while (cursor < end)
        {
            size_t ob = description.find('{', cursor);
            if (ob == std::string::npos || ob >= end) break;
            size_t cb = description.find('}', ob + 1);
            if (cb == std::string::npos || cb > end) break;

            std::string inner = description.substr(ob + 1, cb - ob - 1);
            cursor = cb + 1;

            size_t colon = inner.find(':');
            if (colon == std::string::npos) continue;

            std::string key = inner.substr(0, colon);
            std::string val = inner.substr(colon + 1);
            LLStringUtil::trim(key);
            LLStringUtil::trim(val);
            key = toLowerAscii(key);

            onPair(key, val);
        }
    }

    // Locate the body of "[<prefix>...]" and return [content_start, end).
    // Returns false if prefix not found or no closing bracket.
    bool findTagBody(const std::string& description, const std::string& prefix,
                     size_t& out_content_start, size_t& out_end)
    {
        size_t begin = findCaseInsensitive(description, prefix);
        if (begin == std::string::npos) return false;
        size_t content_start = begin + prefix.size();
        size_t end = description.find(']', content_start);
        if (end == std::string::npos) return false;
        out_content_start = content_start;
        out_end = end;
        return true;
    }

    template <typename TagT>
    void resolveRolloffFromTag(const TagT& tag, F32& out_min, F32& out_max)
    {
        out_min = tag.min.value_or(gSavedSettings.getF32("Stream3DRolloffMin"));
        out_max = tag.max.value_or(gSavedSettings.getF32("Stream3DRolloffMax"));
    }

    // M8: emit a Firestorm toast that also auto-logs into Nearby Chat.
    // ChatSystemMessageTip is a built-in template (notifications.xml) that
    // marries `notifytip` (transient toast) with `log_to_chat="true"`.
    // Firestorm rewrote LLHandlerUtil::logToNearbyChat to deliver only into
    // FSFloaterNearbyChat, so AYAChatWindowStyle=2 (LL) users would otherwise
    // see only the toast. Forward the same line to LLFloaterIMNearbyChat in
    // that case so chat history matches the toast across both styles.
    void notifyStream3D(const std::string& message)
    {
        const std::string text = "3D Stream: " + message;

        LLSD args;
        args["MESSAGE"] = text;
        LLNotificationsUtil::add("ChatSystemMessageTip", args);

        if (ayastorm_is_ll_style())
        {
            if (LLFloaterIMNearbyChat* ll_chat =
                    LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat"))
            {
                LLChat chat_msg(text);
                chat_msg.mSourceType = CHAT_SOURCE_SYSTEM;
                chat_msg.mFromName = SYSTEM_FROM;
                chat_msg.mFromID = LLUUID::null;
                ll_chat->addMessage(chat_msg);
            }
        }
    }
}

LLPositionalStreamMgr& LLPositionalStreamMgr::instance()
{
    static LLPositionalStreamMgr sInstance;
    return sInstance;
}

LLPositionalStreamMgr::LLPositionalStreamMgr() = default;
LLPositionalStreamMgr::~LLPositionalStreamMgr() = default;

// static
std::optional<LLPositionalStreamMgr::TagData>
LLPositionalStreamMgr::parseTag(const std::string& description)
{
    // r5: new prefix is `[3dstream:`; the legacy `[ayastream:` is kept as a
    // permanent alias so already-placed prims keep working without re-edit.
    static const std::string kPrefix    = "[3dstream:";
    static const std::string kPrefixOld = "[ayastream:";

    size_t content_start = 0, end = 0;
    if (!findTagBody(description, kPrefix, content_start, end) &&
        !findTagBody(description, kPrefixOld, content_start, end)) return std::nullopt;

    TagData data;
    bool got_url = false;
    forEachKeyValue(description, content_start, end,
        [&](const std::string& key, const std::string& val)
        {
            if (key == "url")      { data.url = val; got_url = !val.empty(); }
            else if (key == "min") { F32 f; if (tryParseFloat(val, f)) data.min = f; }
            else if (key == "max") { F32 f; if (tryParseFloat(val, f)) data.max = f; }
        });

    if (!got_url) return std::nullopt;
    return data;
}

// r12 P9: short-name aliases for r11 reverb/binaural keys and venue values.
// Object Description has a hard 127-byte cap (SL protocol). Long forms like
// {binaural:on}{venue:hall_medium}{wetgain:1.5} eat 45 bytes alone; short
// forms compress to 22 (saves 23 bytes, enough headroom for typical URLs).
// Parser accepts both old and new; LSL writer emits short by default.
namespace
{
    // venue value alias table: short token → canonical long name.
    // Keep in sync with LLVenueReverbDsp::knownVenues() and the LSL setup
    // script's VENUE_BUTTONS list.
    static constexpr std::pair<std::string_view, std::string_view> kVenueAliases[] = {
        {"d",  "dry"},
        {"rs", "room_small"},
        {"rm", "room_medium"},
        {"hs", "hall_small"},
        {"hm", "hall_medium"},
        {"hl", "hall_large"},
        {"cl", "club"},
        {"ct", "cathedral"},
        {"od", "outdoor"},
    };

    std::string resolveVenueAlias(const std::string& val)
    {
        for (const auto& [short_form, long_form] : kVenueAliases)
        {
            if (val == short_form) return std::string(long_form);
        }
        return val; // not an alias → pass through as-is
    }
}

// static
std::optional<LLPositionalStreamMgr::ChannelKind>
LLPositionalStreamMgr::parseChannelKind(std::string_view s)
{
    // The {ch:...} alphabet. r8/r9 used L/R/M only; r10 added the 5.1
    // placement values. Every other consumer (evaluator, downmix table)
    // reads through the enum and never re-parses the source string.
    static constexpr std::pair<std::string_view, ChannelKind> kTokens[] = {
        {"L",   ChannelKind::L},
        {"R",   ChannelKind::R},
        {"M",   ChannelKind::M},
        {"FL",  ChannelKind::FL},
        {"FR",  ChannelKind::FR},
        {"C",   ChannelKind::C},
        {"LFE", ChannelKind::LFE},
        {"SL",  ChannelKind::SL},
        {"SR",  ChannelKind::SR},
        {"BL",  ChannelKind::BL},
        {"BR",  ChannelKind::BR},
    };

    auto upcase = [](char c) -> char
    {
        return (c >= 'a' && c <= 'z') ? char(c - 'a' + 'A') : c;
    };

    for (const auto& [token, kind] : kTokens)
    {
        if (s.size() != token.size()) continue;
        bool match = true;
        for (size_t i = 0; i < s.size(); ++i)
        {
            if (upcase(s[i]) != token[i]) { match = false; break; }
        }
        if (match) return kind;
    }
    return std::nullopt;
}

// static
LLPositionalStreamMgr::DistParseResult
LLPositionalStreamMgr::parseDistributedStereoTag(const std::string& description)
{
    // r5 で導入した [3dstream-stereo:...] と alias [ayastream-stereo:...] を
    // 同じ本体としてサポート (r8 で旧 {l:N}{r:N} 書式は廃止し、新書式のみ受ける)。
    // r31 では [3dstream:...] も distributed/linkset grammar として読む。
    // ただし [3dstream:{url:...}] 単独は evaluateLinkset() 側で既存 mono
    // fallback に戻すため、古い単一プリム配置は壊さない。
    static const std::string kPrefix    = "[3dstream-stereo:";
    static const std::string kPrefixOld = "[ayastream-stereo:";
    static const std::string kPrefixUnified = "[3dstream:";

    DistParseResult result;

    size_t content_start = 0, end = 0;
    bool unified_3dstream_prefix = false;
    if (!findTagBody(description, kPrefix, content_start, end) &&
        !findTagBody(description, kPrefixOld, content_start, end))
    {
        if (!findTagBody(description, kPrefixUnified, content_start, end))
        {
            return result; // no tag at all
        }
        unified_3dstream_prefix = true;
    }

    // Track which keys appeared (regardless of value validity) so a
    // {ch:X} with an invalid value still surfaces BadCh rather than
    // silently being treated as "no tag here".
    bool seen_ch_key = false;
    bool seen_url_key = false;
    bool seen_source_key = false;
    bool seen_media_source = false;

    DistStereoTagData data;
    data.unified_3dstream_prefix = unified_3dstream_prefix;
    F32 range_value = 0.f;
    bool range_valid = false;
    F32 volume_value = 0.f;
    bool volume_valid = false;

    auto setError = [&](DistParseError e, const std::string& v)
    {
        // First error wins — keeps the surfaced message stable when several
        // fields are bad (LSL editing typically fixes one at a time anyway).
        if (result.error == DistParseError::Ok)
        {
            result.error = e;
            result.bad_value = v;
        }
    };

    forEachKeyValue(description, content_start, end,
        [&](const std::string& key, const std::string& val)
        {
            if (key == "url")
            {
                seen_url_key = true;
                if (val.empty())
                {
                    setError(DistParseError::EmptyUrl, val);
                }
                else
                {
                    data.source_kind = DistSourceKind::Url;
                    data.url = val;
                }
            }
            else if (key == "source")
            {
                seen_source_key = true;
                std::string lowered = val;
                LLStringUtil::toLower(lowered);
                if (lowered == "media" || lowered == "media-stereo")
                {
                    seen_media_source = true;
                    data.source_kind = DistSourceKind::Media;
                    data.media_source_channels = 2;
                }
                else if (lowered == "media-5-1")
                {
                    seen_media_source = true;
                    data.source_kind = DistSourceKind::Media;
                    data.media_source_channels = 6;
                }
                else if (lowered == "media-7-1")
                {
                    seen_media_source = true;
                    data.source_kind = DistSourceKind::Media;
                    data.media_source_channels = 8;
                }
                else
                {
                    setError(DistParseError::BadSource, val);
                }
            }
            else if (key == "face")
            {
                S32 face = -1;
                if (tryParseNonNegativeS32(val, face))
                {
                    data.media_face = face;
                }
                else
                {
                    setError(DistParseError::BadFace, val);
                }
            }
            else if (key == "link")
            {
                S32 link = -1;
                if (tryParseNonNegativeS32(val, link))
                {
                    data.media_link = link;
                }
                else
                {
                    setError(DistParseError::BadLink, val);
                }
            }
            else if (key == "ch")
            {
                seen_ch_key = true;
                if (auto ck = parseChannelKind(val))
                {
                    data.ch = *ck;
                }
                else
                {
                    setError(DistParseError::BadCh, val);
                }
            }
            else if (key == "range")
            {
                F32 f;
                if (tryParseFloat(val, f) && f > 0.f)
                {
                    range_value = f;
                    range_valid = true;
                }
                else
                {
                    setError(DistParseError::BadRange, val);
                }
            }
            else if (key == "volume")
            {
                F32 f;
                if (tryParseFloat(val, f) && f >= 0.f && f <= 1.f)
                {
                    volume_value = f;
                    volume_valid = true;
                }
                else
                {
                    setError(DistParseError::BadVolume, val);
                }
            }
            else if (key == "binaural" || key == "bin")
            {
                // r11 P5: {binaural:on|off} (case-insensitive). Only
                // meaningful on the root prim (= same prim as {url}); we
                // still parse it on every prim so a malformed value
                // surfaces a chat error regardless of where the typo is.
                // r12 P9: short alias `bin` accepted for Desc 127-byte budget.
                std::string lowered = val;
                LLStringUtil::toLower(lowered);
                if (lowered == "on" || lowered == "true" || lowered == "1")
                {
                    data.binaural = true;
                }
                else if (lowered == "off" || lowered == "false" || lowered == "0")
                {
                    data.binaural = false;
                }
                else
                {
                    setError(DistParseError::BadBinaural, val);
                }
            }
            else if (key == "upmix")
            {
                // r12 P5: {upmix:on|off} (case-insensitive). Same shape
                // as binaural. Only meaningful on the root prim, but we
                // still parse on every prim so a typo surfaces a chat
                // error regardless of where it sits.
                std::string lowered = val;
                LLStringUtil::toLower(lowered);
                if (lowered == "on" || lowered == "true" || lowered == "1")
                {
                    data.upmix = true;
                }
                else if (lowered == "off" || lowered == "false" || lowered == "0")
                {
                    data.upmix = false;
                }
                else
                {
                    setError(DistParseError::BadUpmix, val);
                }
            }
            else if (key == "venue" || key == "v")
            {
                // r11 P8: {venue:NAME}. Validated against the bundled
                // catalog (LLVenueReverbDsp::knownVenues, includes "dry").
                // Spec §4.1 line 174 says unknown name is silent-ignore +
                // chat warn — i.e. NOT a tag-rejecting error like the
                // {binaural} branch above. So we capture the bad value
                // separately and let evaluateLinkset notify on its own
                // schedule, leaving data.venue at nullopt → effective
                // resolves to "dry".
                // r12 P9: short alias `v` accepted; venue value also
                // accepts 2-char alias (hm, ct, ...) per kVenueAliases.
                const std::string resolved = resolveVenueAlias(val);
                const auto& known = LLVenueReverbDsp::knownVenues();
                if (std::find(known.begin(), known.end(), resolved) != known.end())
                {
                    data.venue = resolved;
                }
                else
                {
                    data.bad_venue_value = val;
                }
            }
            else if (key == "wetgain" || key == "wg")
            {
                // r11 P9: {wetgain:N}. Spec §4.1 line 152 — F32 in
                // [0.0, 2.0], values outside the range are clamped (not
                // rejected). Non-numeric input is the only failure mode
                // surfaced as BadWetGain.
                // r12 P9: short alias `wg` accepted.
                F32 f;
                if (tryParseFloat(val, f))
                {
                    data.wetgain = std::clamp(f, 0.f, 2.f);
                }
                else
                {
                    setError(DistParseError::BadWetGain, val);
                }
            }
            else if (key == "lfegain" || key == "lg")
            {
                // r12.1: {lfegain:N}. F32 in [0.0, 3.0], values outside
                // the range are clamped (not rejected). Source-side /
                // root-only — child values silently ignored same as
                // {wetgain}/{venue}. Non-numeric input is BadLfeGain.
                F32 f;
                if (tryParseFloat(val, f))
                {
                    data.lfegain = std::clamp(f, 0.f, 3.f);
                }
                else
                {
                    setError(DistParseError::BadLfeGain, val);
                }
            }
            // Unknown keys (incl. removed-in-r8 {l}/{r}/{min}/{max}) are
            // silently ignored — the spec is permissive about extra fields.
        });

    // The tag is recognized when at least one of {url}, {source}, or {ch}
    // appeared.
    // A bracket body with neither is treated as "no tag here".
    const bool tag_recognized = seen_ch_key || seen_url_key || seen_source_key;
    if (!tag_recognized)
    {
        return result;
    }

    if (seen_url_key && seen_media_source)
    {
        setError(DistParseError::ConflictingSource, "url+source:media");
    }

    if (result.error != DistParseError::Ok)
    {
        return result; // data stays nullopt; caller can throttle/notify
    }

    // Fan {range} and {volume} into the role-specific slots. {range} on a
    // prim that is both source and speaker fills both range_default and
    // range_speaker from the same value (spec §4.3).
    if (range_valid)
    {
        if (data.source_kind.has_value()) data.range_default = range_value;
        if (data.ch.has_value())  data.range_speaker = range_value;
    }
    if (volume_valid && data.ch.has_value())
    {
        data.volume = volume_value;
    }

    result.data = data;
    return result;
}

void LLPositionalStreamMgr::onObjectPropertiesReceived(const LLUUID& id,
                                                       const std::string& description)
{
    if (id.isNull())
    {
        return;
    }

    // M8: kill switch + scan toggle gating. Cache the description anyway when
    // only DescriptionScan is off — re-enabling shouldn't have to wait for a
    // fresh polling cycle to know what every prim said.
    if (!gSavedSettings.getBOOL("Stream3DEnabled"))
    {
        return;
    }
    if (!gSavedSettings.getBOOL("Stream3DDescriptionScan"))
    {
        return;
    }

    // Always re-evaluate: if the description is unchanged the diff inside
    // evaluateBinding is cheap, and re-arrivals after object recreate
    // (e.g., teleport out and back) need to re-bind even when the text matches.
    auto& entry = mDescriptionCache[id];
    entry.description = description;
    const F64 reply_now = LLTimer::getElapsedSeconds();
    entry.last_polled  = reply_now;
    entry.last_replied = reply_now;
    entry.priority_retries = 0;

    // M3b debug: only log replies that look like our tag, to keep the noise
    // floor sane in busy regions.
    if (description.find("3dstream") != std::string::npos ||
        description.find("ayastream") != std::string::npos)
    {
        LL_INFOS("Stream3D") << "reply: " << id
                              << " desc=\"" << description << "\"" << LL_ENDL;
    }

    evaluateBinding(id);
}

void LLPositionalStreamMgr::notifyDistributedError(const LLUUID& prim_id,
                                                   DistErrorKind kind,
                                                   const std::string& detail)
{
    const F64 now = LLTimer::getElapsedSeconds();
    auto key = std::make_pair(prim_id, kind);
    auto it = mErrorThrottle.find(key);
    if (it != mErrorThrottle.end() && (now - it->second) < 30.0)
    {
        // spec §4.9: suppression logged but not surfaced to chat.
        LL_DEBUGS("Stream3D") << "[3dstream-stereo] notification suppressed: kind="
                               << static_cast<int>(kind) << " prim=" << prim_id
                               << " (next allowed in "
                               << (30.0 - (now - it->second)) << "s)" << LL_ENDL;
        return;
    }
    mErrorThrottle[key] = now;

    // spec §4.9 message templates. detail is interpolated where it carries
    // useful information (raw bad value, over-limit count, URL). Each kind
    // ends in a worked example so the user can copy-paste a fix.
    const std::string id_short = prim_id.asString().substr(0, 8);
    std::string msg;
    switch (kind)
    {
    case DistErrorKind::BadCh:
        msg = "タグ書式エラー (prim " + id_short + "): ch の値は L/R/M/FL/FR/C/LFE/SL/SR/BL/BR のいずれかである必要があります";
        if (!detail.empty()) msg += " (got '" + detail + "')";
        msg += "。例: [3dstream:{ch:L}{range:30}]";
        break;
    case DistErrorKind::BadRange:
        msg = "タグ書式エラー (prim " + id_short + "): range は正の数で指定してください";
        if (!detail.empty()) msg += " (got '" + detail + "')";
        msg += "。例: [3dstream:{ch:L}{range:30}]";
        break;
    case DistErrorKind::BadVolume:
        msg = "タグ書式エラー (prim " + id_short + "): volume は 0.0〜1.0 の範囲で指定してください";
        if (!detail.empty()) msg += " (got '" + detail + "')";
        msg += "。例: [3dstream:{ch:L}{volume:0.8}]";
        break;
    case DistErrorKind::EmptyUrl:
        msg = "タグ書式エラー (prim " + id_short + "): url が空です";
        msg += "。例: [3dstream:{url:http://example/stream.mp3}{range:30}]";
        break;
    case DistErrorKind::NoSpeakers:
        msg = "構造エラー (root " + id_short + "): 音源宣言が root にあるがスピーカー (ch) が見つかりません";
        msg += "。各スピーカープリムに [3dstream:{ch:L|R|M}] を記載してください";
        break;
    case DistErrorKind::SpeakerOverLimit:
        msg = "構造エラー (root " + id_short + "): スピーカー数が上限を超えています";
        if (!detail.empty()) msg += " (" + detail + ")";
        msg += "。Stream3DStereoMaxSpeakers の上限まで採用しました";
        break;
    case DistErrorKind::StreamStartFailed:
        msg = "再生エラー (root " + id_short + "): ストリームを開始できませんでした";
        if (!detail.empty()) msg += " (source='" + detail + "')";
        msg += "。音源の再生状態とネットワーク接続を確認してください";
        break;
    case DistErrorKind::UnsupportedSourceFormat:
        msg = "構造エラー (root " + id_short + "): 非対応のソース形式です";
        if (!detail.empty()) msg += " (" + detail + ")";
        msg += "。URL 音源は 1/2ch または 6ch、media 音源は source:media / media-5-1 / media-7-1 で指定してください";
        break;
    case DistErrorKind::BadBinaural:
        msg = "タグ書式エラー (prim " + id_short + "): binaural の値は on または off で指定してください";
        if (!detail.empty()) msg += " (got '" + detail + "')";
        msg += "。例: [3dstream:{url:http://example/stream.mp3}{binaural:off}]";
        break;
    case DistErrorKind::BadUpmix:
        msg = "タグ書式エラー (prim " + id_short + "): upmix の値は on または off で指定してください";
        if (!detail.empty()) msg += " (got '" + detail + "')";
        msg += "。例: [3dstream:{url:http://example/stream.mp3}{upmix:on}]";
        break;
    case DistErrorKind::BadVenue:
        msg = "タグ書式エラー (root " + id_short + "): venue の値が認識できません";
        if (!detail.empty()) msg += " (got '" + detail + "')";
        msg += "。許容値: dry / room_small / room_medium / hall_small / hall_medium / hall_large / club / cathedral / outdoor";
        break;
    case DistErrorKind::IRNotLoaded:
        msg = "再生エラー (root " + id_short + "): venue IR ファイルが読み込めませんでした";
        if (!detail.empty()) msg += " (venue='" + detail + "')";
        msg += "。dry にフォールバックします (app_settings/venue_ir/ の WAV を確認してください)";
        break;
    case DistErrorKind::BadWetGain:
        msg = "タグ書式エラー (prim " + id_short + "): wetgain の値は数値で指定してください";
        if (!detail.empty()) msg += " (got '" + detail + "')";
        msg += " (範囲外は 0.0〜2.0 にクランプされます)。例: [3dstream:{venue:hall_medium}{wetgain:1.0}]";
        break;
    case DistErrorKind::BadLfeGain:
        msg = "タグ書式エラー (prim " + id_short + "): lfegain の値は数値で指定してください";
        if (!detail.empty()) msg += " (got '" + detail + "')";
        msg += " (範囲外は 0.0〜3.0 にクランプされます)。例: [3dstream:{url:http://example/stream.mp3}{lfegain:2.0}]";
        break;
    case DistErrorKind::BadSource:
        msg = "タグ書式エラー (prim " + id_short + "): source の値は media / media-stereo / media-5-1 / media-7-1 のいずれかを指定してください";
        if (!detail.empty()) msg += " (got '" + detail + "')";
        msg += "。例: [3dstream:{source:media}{upmix:on}{ch:L}{range:30}]";
        break;
    case DistErrorKind::ConflictingSource:
        msg = "タグ書式エラー (prim " + id_short + "): url と source:media は同時に指定できません";
        msg += "。URL 音源なら {url:...}、prim media 音源なら {source:media} のどちらか一方を指定してください";
        break;
    case DistErrorKind::BadLink:
        msg = "タグ書式エラー (prim " + id_short + "): link は 0 以上の整数で指定してください";
        if (!detail.empty()) msg += " (got '" + detail + "')";
        msg += "。例: [3dstream:{source:media}{link:2}{face:0}{ch:L}{range:30}]";
        break;
    case DistErrorKind::BadFace:
        msg = "タグ書式エラー (prim " + id_short + "): face は 0 以上の整数で指定してください";
        if (!detail.empty()) msg += " (got '" + detail + "')";
        msg += "。例: [3dstream:{source:media}{link:2}{face:0}{ch:L}{range:30}]";
        break;
    case DistErrorKind::MediaFaceNotFound:
        msg = "構造エラー (root " + id_short + "): 指定された link / face に media が見つかりません";
        if (!detail.empty()) msg += " (" + detail + ")";
        msg += "。例: [3dstream:{source:media}{link:2}{face:0}{ch:L}{range:30}]";
        break;
    case DistErrorKind::MediaFaceAmbiguous:
        msg = "構造エラー (root " + id_short + "): linkset 内に media face が複数あるため source media を特定できません";
        if (!detail.empty()) msg += " (" + detail + ")";
        msg += "。例: [3dstream:{source:media}{link:2}{face:0}{ch:L}{range:30}]";
        break;
    case DistErrorKind::MediaSourceNotReady:
        msg = "再生待機 (root " + id_short + "): media source の音声がまだ準備できていません";
        if (!detail.empty()) msg += " (" + detail + ")";
        msg += "。linkset 内の対象 media を再生開始してから再評価されます";
        break;
    case DistErrorKind::MediaSourceInUse:
        msg = "構造エラー (root " + id_short + "): この media source は別の 3D Stream binding で使用中です";
        if (!detail.empty()) msg += " (" + detail + ")";
        msg += "。同じ media を複数の 3D Stream source として同時使用することはできません";
        break;
    }
    notifyStream3D(msg);
}

// static
bool LLPositionalStreamMgr::effectiveBinaural(std::optional<bool> tag_value)
{
    // r11 P5 / r31 policy: debug override (sentinel `-1` = follow tag)
    // wins over the publisher tag. Tag default is `off` when the publisher
    // omits the {binaural:...} key; binaural rendering is opt-in.
    const S32 dbg = gSavedSettings.getS32("Stream3DBinauralRender");
    if (dbg == 0) return false;        // force OFF
    if (dbg >= 1) return true;         // force ON
    return tag_value.value_or(false);  // sentinel -1 → follow tag
}

// static
bool LLPositionalStreamMgr::effectiveUpmix(std::optional<bool> tag_value)
{
    // r12 P5 / spec §6: same sentinel pattern as binaural, but tag default
    // is OFF (opt-in) — listeners hear the publisher's stereo placement
    // unchanged unless the publisher writes `{upmix:on}`.
    const S32 dbg = gSavedSettings.getS32("Stream3DUpmix");
    if (dbg == 0) return false;        // force OFF
    if (dbg >= 1) return true;         // force ON
    return tag_value.value_or(false);  // sentinel -1 → follow tag, default off
}

// static
std::string LLPositionalStreamMgr::effectiveVenue(const std::optional<std::string>& tag_value)
{
    // r11 P8 / spec §4.5: debug override wins when non-empty; otherwise
    // the publisher tag (or "dry" default) is used. The override string
    // is taken verbatim — invalid names will be rejected by setVenue()
    // and surface as IRNotLoaded chat warnings (no separate validation
    // here because both paths converge on the same failure handling).
    const std::string dbg = gSavedSettings.getString("Stream3DVenueOverride");
    if (!dbg.empty()) return dbg;
    return tag_value.value_or("dry");
}

void LLPositionalStreamMgr::applyVenueToBinding(DistributedStereoBinding& binding,
                                                const std::optional<std::string>& venue_tag)
{
    // r11 P8: refresh the binding's tag mirror first (cheap, no side
    // effects) so a fingerprint-match path can re-record the latest
    // tag value even if the resolved effective hasn't moved.
    binding.venue_tag = venue_tag;

    const std::string venue_effective = effectiveVenue(venue_tag);
    if (binding.venue_effective_applied == venue_effective)
    {
        return;
    }

    LLAudioEngine_FMODSTUDIO* engine = dynamic_cast<LLAudioEngine_FMODSTUDIO*>(gAudiop);
    LLVenueReverbDsp* dsp = engine ? engine->getVenueReverbDsp() : nullptr;
    if (!dsp)
    {
        // Engine not in FMOD mode (or DSP create failed at init): there's
        // no bus-level reverb to push to. Record the resolved name so we
        // don't loop on this branch every poll, and let the binding stay
        // silent on the wet path (which is exactly what no-DSP means).
        binding.venue_effective_applied = venue_effective;
        return;
    }

    if (dsp->setVenue(venue_effective))
    {
        binding.venue_effective_applied = venue_effective;
        return;
    }

    // setVenue() rejected the name. "dry" is documented to always
    // succeed, so a failure here means a non-"dry" name whose IR slot
    // was never primed (file missing / format reject / sample-rate
    // mismatch at engine init). Notify and fall back.
    notifyDistributedError(binding.root_id, DistErrorKind::IRNotLoaded, venue_effective);
    dsp->setVenue("dry");
    binding.venue_effective_applied = "dry";
}

// static
F32 LLPositionalStreamMgr::effectiveWetGain(std::optional<F32> tag_value)
{
    // r11 P9 / spec §4.5 line 385: debug override wins when ≥ 0.0;
    // sentinel `-1.0` (or any negative) means "follow tag". Tag default
    // is 0.2 when {wetgain:...} is omitted (r12.1: lowered from 1.0
    // after live-listening — bus-tail stereo IR convolution sounds
    // musically usable in roughly the 0.1–0.5 range; 1.0 was an
    // engineering "unity wet" default that smeared the dry mix).
    // Final value is clamped to [0.0, 2.0] — the same range parser
    // clamps to, repeated here in case the debug value is out of range.
    const F32 dbg = gSavedSettings.getF32("Stream3DVenueWetGain");
    const F32 raw = (dbg >= 0.f) ? dbg : tag_value.value_or(0.2f);
    return std::clamp(raw, 0.f, 2.f);
}

void LLPositionalStreamMgr::applyWetGainToBinding(DistributedStereoBinding& binding,
                                                  std::optional<F32> wetgain_tag)
{
    binding.wetgain_tag = wetgain_tag;

    const F32 wetgain_effective = effectiveWetGain(wetgain_tag);
    // NaN sentinel for "never pushed" — the != comparison below would
    // be true against any number, so first call always goes through.
    if (binding.wetgain_effective_applied == wetgain_effective)
    {
        return;
    }

    LLAudioEngine_FMODSTUDIO* engine = dynamic_cast<LLAudioEngine_FMODSTUDIO*>(gAudiop);
    LLVenueReverbDsp* dsp = engine ? engine->getVenueReverbDsp() : nullptr;
    if (dsp)
    {
        dsp->setWetGain(wetgain_effective);
    }
    // Even when the engine isn't in FMOD mode, record the resolved
    // value so we don't loop on this branch every poll cycle.
    binding.wetgain_effective_applied = wetgain_effective;
}

// static
F32 LLPositionalStreamMgr::effectiveLfeGain(std::optional<F32> tag_value)
{
    // r12.1: same precedence as effectiveWetGain — debug override wins
    // when >= 0.0; sentinel -1.0 (or any negative) means "follow tag".
    // Tag default is 1.0 (passthrough) when {lfegain:...} omitted.
    const F32 dbg = gSavedSettings.getF32("Stream3DLfeGain");
    const F32 raw = (dbg >= 0.f) ? dbg : tag_value.value_or(1.f);
    return std::clamp(raw, 0.f, 3.f);
}

void LLPositionalStreamMgr::applyLfeGainToBinding(DistributedStereoBinding& binding,
                                                  std::optional<F32> lfegain_tag)
{
    binding.lfegain_tag = lfegain_tag;

    const F32 lfegain_effective = effectiveLfeGain(lfegain_tag);
    if (binding.lfegain_effective_applied == lfegain_effective)
    {
        return;
    }

    if (binding.stream)
    {
        binding.stream->setLfeGain(lfegain_effective);
    }
    binding.lfegain_effective_applied = lfegain_effective;
}

void LLPositionalStreamMgr::evaluateBinding(const LLUUID& id)
{
    auto desc_it = mDescriptionCache.find(id);
    if (desc_it == mDescriptionCache.end())
    {
        return;
    }

    const std::string& desc = desc_it->second.description;
    auto dist = parseDistributedStereoTag(desc);
    auto mono_tag = parseTag(desc);

    if (mono_tag)
    {
        // r31: [3dstream:{url:...}] is still the old single-prim mono tag
        // unless the same tag also carries a speaker role, or the linkset has
        // [3dstream:{ch:...}] speakers. In those cases the unified tag is a
        // distributed/linkset declaration and must not start the mono path.
        if (dist.data && dist.data->unified_3dstream_prefix)
        {
            LLViewerObject* obj = gObjectList.findObject(id);
            LLViewerObject* root = obj && !obj->isDead() ? obj->getRootEdit() : nullptr;
            LLUUID root_id = root ? root->getID() : id;
            bool has_linkset_speaker = dist.data->ch.has_value();

            if (!has_linkset_speaker && root)
            {
                for (const auto& child : root->getChildren())
                {
                    if (!child || child->isDead())
                    {
                        continue;
                    }
                    auto child_desc_it = mDescriptionCache.find(child->getID());
                    if (child_desc_it == mDescriptionCache.end())
                    {
                        continue;
                    }
                    auto child_parse = parseDistributedStereoTag(child_desc_it->second.description);
                    if (child_parse.data &&
                        child_parse.data->ch.has_value())
                    {
                        has_linkset_speaker = true;
                        break;
                    }
                }
            }

            if (has_linkset_speaker || mDistributedBindings.find(root_id) != mDistributedBindings.end())
            {
                auto bind_it = mBindings.find(id);
                if (bind_it != mBindings.end())
                {
                    LL_INFOS("Stream3D") << "Removing positional binding for " << id
                                          << " (promoted to distributed 3dstream)"
                                          << LL_ENDL;
                    mBindings.erase(bind_it);
                }
                mPendingLinksetEval.insert(root_id);
                return;
            }

            if (dist.data->source_kind == DistSourceKind::Url)
            {
                mPendingLinksetEval.insert(root_id);
            }
        }

        evaluateMonoBinding(id, *mono_tag);
        // r8 F2-a: a prim that just became a mono source must drop out of any
        // distributed-stereo linkset it had been participating in. Re-evaluate
        // the previously-known root so the speaker slot is recomputed without
        // this prim. (mPrimToRoot lookup is the only side effect here; the
        // mono path itself is unchanged.)
        // r8 F8: defer to the per-frame drain in update() — see
        // mPendingLinksetEval comment for why.
        auto pr_it = mPrimToRoot.find(id);
        if (pr_it != mPrimToRoot.end())
        {
            mPendingLinksetEval.insert(pr_it->second);
        }
        return;
    }

    // No mono tag — drop any leftover mono binding before considering the
    // distributed-stereo path.
    auto bind_it = mBindings.find(id);
    if (bind_it != mBindings.end())
    {
        LL_INFOS("Stream3D") << "Removing positional binding for " << id
                              << " (tag gone)" << LL_ENDL;
        mBindings.erase(bind_it);
    }

    // r8 F2-a: distributed-stereo dispatch. {url}, {source}, or {ch}
    // triggers a linkset-level (re)evaluation rooted at this prim's
    // getRootEdit().
    if (dist.error != DistParseError::Ok)
    {
        LL_INFOS("Stream3D") << "[3dstream-stereo] parse error on " << id
                              << " (kind=" << static_cast<int>(dist.error)
                              << ", value='" << dist.bad_value << "')" << LL_ENDL;

        DistErrorKind k = DistErrorKind::BadCh;
        switch (dist.error)
        {
        case DistParseError::BadCh:       k = DistErrorKind::BadCh;       break;
        case DistParseError::BadRange:    k = DistErrorKind::BadRange;    break;
        case DistParseError::BadVolume:   k = DistErrorKind::BadVolume;   break;
        case DistParseError::EmptyUrl:    k = DistErrorKind::EmptyUrl;    break;
        case DistParseError::BadBinaural: k = DistErrorKind::BadBinaural; break;
        case DistParseError::BadUpmix:    k = DistErrorKind::BadUpmix;    break;
        case DistParseError::BadWetGain:  k = DistErrorKind::BadWetGain;  break;
        case DistParseError::BadLfeGain:  k = DistErrorKind::BadLfeGain;  break;
        case DistParseError::BadSource:   k = DistErrorKind::BadSource;   break;
        case DistParseError::ConflictingSource:
            k = DistErrorKind::ConflictingSource;
            break;
        case DistParseError::BadLink:     k = DistErrorKind::BadLink;     break;
        case DistParseError::BadFace:     k = DistErrorKind::BadFace;     break;
        case DistParseError::Ok:          break; // unreachable
        }
        notifyDistributedError(id, k, dist.bad_value);

        // Field is malformed → treat the slot as missing and re-evaluate the
        // owning linkset, if we knew about one. (Deferred — F8.)
        auto pr_it = mPrimToRoot.find(id);
        if (pr_it != mPrimToRoot.end())
        {
            mPendingLinksetEval.insert(pr_it->second);
        }
        return;
    }

    if (dist.data)
    {
        LLViewerObject* obj = gObjectList.findObject(id);
        if (!obj || obj->isDead())
        {
            return;
        }
        LLViewerObject* root = obj->getRootEdit();
        LLUUID root_id = root ? root->getID() : id;
        // r8 F8: defer to per-frame drain so a burst of replies for the same
        // linkset coalesces into a single rebuild.
        mPendingLinksetEval.insert(root_id);
        return;
    }

    // No tag of any kind. If this prim used to participate in a linkset
    // binding, that binding needs to be recomputed without it. (Deferred — F8.)
    auto pr_it = mPrimToRoot.find(id);
    if (pr_it != mPrimToRoot.end())
    {
        mPendingLinksetEval.insert(pr_it->second);
    }
}

void LLPositionalStreamMgr::evaluateLinkset(LLUUID root_id)
{
    LLViewerObject* root = gObjectList.findObject(root_id);
    if (!root || root->isDead() || root->isAvatar())
    {
        teardownDistributedBinding(root_id);
        return;
    }

    auto root_desc_it = mDescriptionCache.find(root_id);
    if (root_desc_it == mDescriptionCache.end()
        || root_desc_it->second.description.empty())
    {
        // Root description not yet known — keep any existing binding pending
        // and ask the poll loop to fetch the root proactively.
        enqueuePriorityPoll(root_id);
        return;
    }

    auto root_parse = parseDistributedStereoTag(root_desc_it->second.description);
    if (!root_parse.data || !root_parse.data->source_kind.has_value())
    {
        // r8 F2-a constraint: source declaration must live on the root prim.
        // A linkset without a root-level source cannot form a binding.
        teardownDistributedBinding(root_id);
        return;
    }

    const auto& root_data = *root_parse.data;
    SourceBindingKey source_key;
    std::string source_label;
    LLPluginAudioRingHeader* media_ring = nullptr;
    LLViewerMediaImpl* media_impl = nullptr;
    const bool source_is_media = (*root_data.source_kind == DistSourceKind::Media);
    bool media_source_uses_viewer_volume = true;

    if (source_is_media)
    {
        std::vector<MediaFaceCandidate> media_candidates;
        S32 media_face_count_in_linkset = 0;

        auto request_media_source_refresh = [&]()
        {
            enqueuePriorityPoll(root_id);
            for (const auto& child : root->getChildren())
            {
                if (!child || child->isDead())
                {
                    continue;
                }
                enqueuePriorityPoll(child->getID());
                requestChildDescViaSelect(child.get());
            }
        };

        auto collect_media_faces = [&](LLViewerObject* object)
        {
            if (!object || object->isDead())
            {
                return;
            }
            const S32 link_number = computeLinkNumber(root, object);
            LLVOVolume* volume = dynamic_cast<LLVOVolume*>(object);
            if (!volume)
            {
                return;
            }
            const S32 num_tes = volume->getNumTEs();
            for (S32 i = 0; i < num_tes; ++i)
            {
                const LLTextureEntry* te = volume->getTE(static_cast<U8>(i));
                if (te && te->hasMedia())
                {
                    ++media_face_count_in_linkset;
                    if (root_data.media_link && link_number != *root_data.media_link)
                    {
                        continue;
                    }
                    if (root_data.media_face && i != *root_data.media_face)
                    {
                        continue;
                    }
                    media_candidates.push_back({object->getID(), link_number, i, volume});
                }
            }
        };

        collect_media_faces(root);
        for (const auto& child : root->getChildren())
        {
            collect_media_faces(child.get());
        }

        if (media_candidates.empty())
        {
            if (root_data.media_face || root_data.media_link)
            {
                LL_DEBUGS("Stream3D") << "[3dstream-stereo] media face pending for root "
                                       << root_id
                                       << " link="
                                       << (root_data.media_link
                                               ? llformat("%d", *root_data.media_link)
                                               : std::string("any"))
                                       << " face="
                                       << (root_data.media_face
                                               ? llformat("%d", *root_data.media_face)
                                               : std::string("any"))
                                       << " media_faces_seen="
                                       << media_face_count_in_linkset
                                       << LL_ENDL;
                request_media_source_refresh();
                return;
            }

            notifyDistributedError(root_id, DistErrorKind::MediaFaceNotFound,
                                   root_data.media_face || root_data.media_link
                                       ? llformat("link=%s face=%s: no media face in linkset",
                                                  root_data.media_link
                                                      ? llformat("%d", *root_data.media_link).c_str()
                                                      : "any",
                                                  root_data.media_face
                                                      ? llformat("%d", *root_data.media_face).c_str()
                                                      : "any")
                                       : std::string("no media face in linkset"));
            teardownDistributedBinding(root_id);
            return;
        }
        if (media_candidates.size() > 1)
        {
            notifyDistributedError(root_id, DistErrorKind::MediaFaceAmbiguous,
                                   llformat("media_faces=%d; specify {link:N}{face:M}",
                                            static_cast<S32>(media_candidates.size())));
            teardownDistributedBinding(root_id);
            return;
        }

        const MediaFaceCandidate& media_source = media_candidates.front();
        media_source_uses_viewer_volume = (media_face_count_in_linkset <= 1);
        S32 media_face = media_source.face;
        viewer_media_t media = media_source.volume->getMediaImpl(static_cast<U8>(media_face));
        if (media.isNull() || !media->hasMedia())
        {
            LL_DEBUGS("Stream3D") << "[3dstream-stereo] media source not ready for root "
                                   << root_id << " media_prim=" << media_source.object_id
                                   << " face=" << media_face << LL_ENDL;
            request_media_source_refresh();
            return;
        }

        media_impl = media.get();
        media_ring = media_impl->getAudioRingForStream3D();
        if (!media_ring ||
            media_ring->mMagic != LL_PLUGIN_AUDIO_RING_MAGIC ||
            media_ring->mVersion != LL_PLUGIN_AUDIO_RING_VERSION)
        {
            LL_DEBUGS("Stream3D") << "[3dstream-stereo] media audio ring not ready for root "
                                   << root_id << " media_prim=" << media_source.object_id
                                   << " face=" << media_face << LL_ENDL;
            request_media_source_refresh();
            return;
        }

        source_key.kind = DistSourceKind::Media;
        source_key.media_object_id = media_source.object_id;
        source_key.media_id = media_impl->getMediaTextureID();
        source_key.face = media_face;
        source_key.media_source_channels = root_data.media_source_channels;
        source_label = "media:" + source_key.media_id.asString()
                       + ":prim=" + source_key.media_object_id.asString()
                       + ":link=" + llformat("%d", media_source.link_number)
                       + ":face=" + llformat("%d", media_face)
                       + ":logical_ch=" + llformat("%d", source_key.media_source_channels);

        for (const auto& [other_root_id, other_binding] : mDistributedBindings)
        {
            if (other_root_id == root_id)
            {
                continue;
            }
            if (other_binding.source_key.kind == DistSourceKind::Media &&
                ((source_key.media_id.notNull() &&
                  other_binding.source_key.media_id == source_key.media_id) ||
                 (other_binding.source_key.media_object_id == source_key.media_object_id &&
                  other_binding.source_key.face == source_key.face)))
            {
                notifyDistributedError(root_id, DistErrorKind::MediaSourceInUse,
                                       "other_root=" + other_root_id.asString().substr(0, 8));
                teardownDistributedBinding(root_id);
                return;
            }
        }
    }
    else if (root_data.url.has_value())
    {
        source_key.kind = DistSourceKind::Url;
        source_key.url = *root_data.url;
        source_label = *root_data.url;
    }
    else
    {
        teardownDistributedBinding(root_id);
        return;
    }

    const std::string url = source_label;

    // r9 P6.5: skip re-opening if this root's URL was already classified
    // FormatUnsupported. Without this gate, dead_roots teardown removes
    // the binding but the next desc poll re-enters here and rebuilds it,
    // looping FMOD open/close every Stream3DPollInterval. The user clears
    // the failure by editing the URL — a different URL falls through
    // (and we erase the cached entry).
    auto failed_it = mFormatFailedUrl.find(root_id);
    if (!source_is_media && failed_it != mFormatFailedUrl.end())
    {
        if (failed_it->second == url)
        {
            return;
        }
        mFormatFailedUrl.erase(failed_it);
    }

    const F32 fallback_range = gSavedSettings.getF32("Stream3DRolloffMax");
    const F32 range_default = root_data.range_default.value_or(fallback_range);
    // r11 P5: capture publisher's {binaural:on|off} tag and the resolved
    // effective value (= debug override × tag value) up front so the
    // fingerprint comparison below can detect either kind of change.
    const std::optional<bool> binaural_tag = root_data.binaural;
    const bool binaural_effective = effectiveBinaural(binaural_tag);
    // r12 P5: capture publisher's {upmix:on|off} tag and resolved
    // effective the same way binaural does. Tag default is OFF (opt-in)
    // so the legacy r5–r11 stereo placement is preserved when omitted.
    const std::optional<bool> upmix_tag = root_data.upmix;
    const bool upmix_effective = effectiveUpmix(upmix_tag);
    // r11 P8: capture the publisher's {venue:NAME} tag (parser-validated
    // against the catalog) and surface a parser-rejected value once via
    // chat. Children's bad_venue_value is intentionally not surfaced —
    // venue is a root-level concept (§4.5), so notifying on a child would
    // just be noise.
    const std::optional<std::string> venue_tag = root_data.venue;
    if (root_data.bad_venue_value)
    {
        notifyDistributedError(root_id, DistErrorKind::BadVenue, *root_data.bad_venue_value);
    }
    // r11 P9: wetgain mirrors venue's apply flow (engine-level DSP,
    // single-store atomic). No bad-value notification at this point —
    // BadWetGain is full-tag-rejecting at parse time, so we never reach
    // here with a malformed value (parse error returns nullopt data).
    const std::optional<F32> wetgain_tag = root_data.wetgain;

    // r12.1: lfegain mirrors wetgain's apply flow (single-store atomic),
    // but the target is per-stream (LLPositionalStreamMulti) instead of
    // a bus-level DSP. Same parse-time rejection means we never reach
    // here with a malformed value.
    const std::optional<F32> lfegain_tag = root_data.lfegain;

    std::vector<SpeakerSlot> speakers;
    auto collectSpeaker = [&](const LLUUID& prim_id, const DistStereoTagData& d)
    {
        if (!d.ch.has_value()) return;
        SpeakerSlot s;
        s.prim_id = prim_id;
        s.ch = *d.ch;
        s.range = d.range_speaker.value_or(range_default);
        s.volume = d.volume.value_or(1.f);
        speakers.push_back(s);
    };

    // Root may be both source and speaker.
    collectSpeaker(root_id, root_data);

    for (const auto& child : root->getChildren())
    {
        if (!child || child->isDead()) continue;
        const LLUUID& child_id = child->getID();
        auto cdesc_it = mDescriptionCache.find(child_id);
        if (cdesc_it == mDescriptionCache.end()
            || cdesc_it->second.description.empty())
        {
            // r8 F2-b: ask the poll loop to fetch this child ahead of the
            // round-robin scan so the binding completes promptly.
            enqueuePriorityPoll(child_id);
            // r8 F11: ObjectPropertiesFamily replies are filtered by the sim
            // for unselected child prims, so the priority poll alone never
            // resolves a child desc on a fresh login (root replies, children
            // stay silent). Trigger a one-shot ObjectSelect on the child to
            // force a full ObjectProperties reply (which carries Description
            // and feeds onObjectPropertiesReceived). The deselect drain in
            // update() releases the slot a moment later. We bypass LLSelectMgr
            // so the user's actual selection / edit-menu / selection beam
            // are untouched.
            requestChildDescViaSelect(child.get());
            continue;
        }
        auto cparse = parseDistributedStereoTag(cdesc_it->second.description);
        if (!cparse.data) continue;
        collectSpeaker(child_id, *cparse.data);
    }

    if (speakers.empty())
    {
        if (root_data.unified_3dstream_prefix &&
            root_data.source_kind == DistSourceKind::Url &&
            root_data.url.has_value())
        {
            // r31: [3dstream:{url:...}] without any {ch:...} speakers keeps
            // its long-standing mono behavior. This path also handles a
            // linkset that used to have unified speakers but no longer does:
            // tear down the distributed binding, then restore the mono one.
            teardownDistributedBinding(root_id);
            if (auto mono_tag = parseTag(root_desc_it->second.description))
            {
                evaluateMonoBinding(root_id, *mono_tag);
            }
            return;
        }

        LL_INFOS("Stream3D") << "[3dstream-stereo] structural error: root "
                              << root_id << " has no speakers" << LL_ENDL;
        notifyDistributedError(root_id, DistErrorKind::NoSpeakers, "");
        teardownDistributedBinding(root_id);
        return;
    }

    // r8 F5: per-binding speaker cap from settings.xml. Reads on every
    // evaluation so a runtime change (debug menu / login.xml override)
    // takes effect on the next poll cycle. Clamped to ≥ 1 so a hostile /
    // mistyped 0 setting doesn't silently disable distributed-stereo.
    const S32 kMaxSpeakers = std::max(1, gSavedSettings.getS32("Stream3DStereoMaxSpeakers"));
    S32 dropped = 0;
    if (static_cast<S32>(speakers.size()) > kMaxSpeakers)
    {
        dropped = static_cast<S32>(speakers.size()) - kMaxSpeakers;
        const S32 total = static_cast<S32>(speakers.size());
        speakers.resize(kMaxSpeakers);
        LL_INFOS("Stream3D") << "[3dstream-stereo] too many speakers on root "
                              << root_id << " — truncated to " << kMaxSpeakers
                              << " (dropped " << dropped << ")" << LL_ENDL;
        notifyDistributedError(root_id, DistErrorKind::SpeakerOverLimit,
                               llformat("declared=%d, used=%d", total, kMaxSpeakers));
    }

    // r8 F3-3: detect "no audible change" so we can keep the running stream
    // when an unrelated tag in the linkset is re-polled. Comparison covers
    // url + element-wise speaker tuple (prim, ch, range); volume is excluded
    // (r12.1: live-pushed via setSpeakerVolume per poll, no rebuild needed)
    // and position is propagated every tick by the update loop.
    auto old_it = mDistributedBindings.find(root_id);
    const bool was_present = (old_it != mDistributedBindings.end());
    bool fingerprint_match = false;
    if (was_present)
    {
        const auto& old_b = old_it->second;
        if (old_b.source_key == source_key
            && old_b.speakers.size() == speakers.size()
            && old_b.stream
            // r11 P5: a tag-only flip ({binaural:on}↔{binaural:off}) or a
            // debug-toggle change (Stream3DBinauralRender -1↔0↔1) must
            // rebuild the FMOD stream so makeChannelForBinding() runs the
            // gate again. Comparing the resolved effective is sufficient
            // because effectiveBinaural() folds both inputs into one bool.
            && old_b.binaural_effective_applied == binaural_effective
            // r12 P5: same fingerprint clause for {upmix:on|off}. Flipping
            // the tag (or Stream3DUpmix sentinel) must rebuild so
            // resolveReadOp re-emits OpKind::Upmix vs the r10 path.
            && old_b.upmix_effective_applied == upmix_effective)
        {
            fingerprint_match = true;
            for (size_t i = 0; i < speakers.size(); ++i)
            {
                const auto& a = old_b.speakers[i];
                const auto& c = speakers[i];
                if (a.prim_id != c.prim_id || a.ch != c.ch
                    || a.range != c.range)
                {
                    fingerprint_match = false;
                    break;
                }
            }
        }
    }

    if (fingerprint_match)
    {
        // Refresh metadata cheaply (range_default may have moved if the root
        // {range:} was re-typed to the same numeric value, etc.) but leave
        // the live stream untouched.
        old_it->second.range_default = range_default;
        old_it->second.dropped_speakers = dropped;
        // r12.1: mirror the latest per-speaker volume into the binding so
        // the per-poll push in update() sees the new value. Indexes line
        // up because the fingerprint compared prim_id / ch / range
        // element-wise above and matched.
        for (size_t i = 0; i < speakers.size(); ++i)
        {
            old_it->second.speakers[i].volume = speakers[i].volume;
        }
        // r11 P5: also refresh binaural_tag so a debug toggle later this
        // session that flips back to the sentinel still resolves correctly.
        old_it->second.binaural_tag = binaural_tag;
        // r12 P5: same refresh for the upmix tag mirror.
        old_it->second.upmix_tag = upmix_tag;
        // r11 P8: venue is engine-level (single bus DSP), so a tag-only
        // change doesn't rebuild the stream — just push the resolved name
        // to the DSP. applyVenueToBinding() is a no-op when the resolved
        // value matches what we already pushed.
        applyVenueToBinding(old_it->second, venue_tag);
        // r11 P9: same single-store atomic flow for wetgain.
        applyWetGainToBinding(old_it->second, wetgain_tag);
        // r12.1: same single-store atomic flow for lfegain (per-stream).
        applyLfeGainToBinding(old_it->second, lfegain_tag);
        if (source_is_media && media_impl)
        {
            media_impl->setStream3DAudioRedirected(true);
        }
        if (old_it->second.media_source_uses_viewer_volume != media_source_uses_viewer_volume)
        {
            old_it->second.media_source_uses_viewer_volume = media_source_uses_viewer_volume;
            old_it->second.last_pushed_volume = std::numeric_limits<F32>::quiet_NaN();
        }
        return;
    }

    // Structural change (or first build). Drop the previous mPrimToRoot
    // entries so the new speaker set is the only one indexed.
    if (was_present)
    {
        if (old_it->second.source_key.kind == DistSourceKind::Media)
        {
            if (LLViewerMediaImpl* old_media =
                    findMediaFor3DSource(old_it->second.source_key))
            {
                old_media->setStream3DAudioRedirected(false);
            }
        }
        for (const auto& s : old_it->second.speakers)
        {
            auto pr_it = mPrimToRoot.find(s.prim_id);
            if (pr_it != mPrimToRoot.end() && pr_it->second == root_id)
            {
                mPrimToRoot.erase(pr_it);
            }
        }
    }

    auto& binding = mDistributedBindings[root_id];
    if (auto mono_it = mBindings.find(root_id); mono_it != mBindings.end())
    {
        LL_INFOS("Stream3D") << "Removing positional binding for " << root_id
                              << " (distributed 3dstream active)" << LL_ENDL;
        mBindings.erase(mono_it);
    }

    binding.root_id = root_id;
    binding.source_key = source_key;
    binding.url = url;
    binding.range_default = range_default;
    binding.binaural_tag = binaural_tag;
    binding.binaural_effective_applied = binaural_effective;
    binding.upmix_tag = upmix_tag;
    binding.upmix_effective_applied = upmix_effective;
    binding.speakers = std::move(speakers);
    binding.dropped_speakers = dropped;
    binding.media_source_uses_viewer_volume = media_source_uses_viewer_volume;

    // r23: seed parcel-gate state for the distributed binding. Source
    // position = root prim position (per §3.2: single-point judgment, not
    // per-speaker). Reset last_pushed_volume too so the next update() pass
    // pushes the gated value even when the master volume is unchanged from
    // the previous binding instance — important for a teardown/rebuild
    // that lands on a different audible state.
    if (LLViewerObject* root_obj = gObjectList.findObject(root_id))
    {
        binding.is_attached = root_obj->isAttachment();
        binding.parcel_audible = LLViewerParcelMgr::getInstance()->canHearSound(
            root_obj->getPositionGlobal());
    }
    binding.last_pushed_volume = std::numeric_limits<F32>::quiet_NaN();
    // r11 P8: push venue selection before the stream comes up so the
    // first audio block out of process() already convolves through the
    // right slot (avoids a momentary "dry then wet" pop on first start).
    applyVenueToBinding(binding, venue_tag);
    // r11 P9: push wetgain in the same window for the same reason.
    applyWetGainToBinding(binding, wetgain_tag);
    // r12.1: lfegain pushes into the per-stream atomic; harmless if the
    // stream isn't constructed yet — the binding mirror will re-push on
    // the next evaluate cycle.
    applyLfeGainToBinding(binding, lfegain_tag);

    for (const auto& s : binding.speakers)
    {
        mPrimToRoot[s.prim_id] = root_id;
    }

    // (Re)build the FMOD stream. unique_ptr::reset() invokes the existing
    // stream's destructor, which joins the decode thread before releasing
    // FMOD resources (r7 M3 invariant carried from Stereo into Multi).
    binding.stream.reset();
    // Fresh stream object: any in-flight retry budget belongs to the old
    // stream. Keep notified_played sticky so unrelated tag edits don't spam
    // "now playing" again. (F7)
    binding.reconnect_attempts = 0;
    binding.next_retry_time = 0.0;
    auto stream = std::make_unique<LLPositionalStreamMulti>();
    stream->setVolume(effectiveDistributedStreamVolume(
        gSavedSettings.getF32("Stream3DVolumeMaster"),
        binding.parcel_audible,
        source_is_media,
        binding.media_source_uses_viewer_volume,
        media_impl));
    // r11 P5: publisher's lite-HRTF intent (× debug override) decided at
    // start time. Persists across the stream's reconnect cascade because
    // makeChannelForBinding() reads it on every channel bring-up.
    stream->setBinauralEnabled(binaural_effective);
    // r12 P4: publisher's {upmix} intent for resolveReadOp's 2ch dispatch
    // decision. binding.upmix_effective_applied stays false until the P5
    // tag parser flips it; the explicit setter call is here so P5 only
    // needs to update the value being read, not introduce new plumbing.
    stream->setUpmixEnabled(binding.upmix_effective_applied);
    // r12 P6: seed the live-tunable upmix knobs from settings before the
    // FMOD callbacks start running, so the very first chunk is computed
    // with the user's current values (not the helper's compile-time
    // defaults). The per-poll push in update() picks up subsequent
    // settings changes without rebuilding the stream.
    stream->setUpmixTuning(
        gSavedSettings.getF32("Stream3DUpmixLfeCutoff"),
        gSavedSettings.getF32("Stream3DUpmixCenterBleed"),
        gSavedSettings.getF32("Stream3DUpmixRearDelayMs"));
    // r12.1: seed the LFE gain from the binding's already-resolved
    // lfegain_tag (applyLfeGainToBinding ran before the stream existed,
    // so it only recorded the tag mirror; push the resolved value here
    // and re-record so subsequent applies are no-ops until tag changes).
    {
        const F32 lfe_gain = effectiveLfeGain(binding.lfegain_tag);
        stream->setLfeGain(lfe_gain);
        binding.lfegain_effective_applied = lfe_gain;
    }
    // r11 P10: viewer-side URL pre-resolve gate. Sentinel default -1 =
    // enabled (libcurl follows HTTPS→HTTP cross-protocol redirects before
    // FMOD::createStream sees the URL); 0 = disabled (FMOD-only, r10
    // behavior). Read once at start and pushed via setter so the resolve
    // path stays self-contained inside llaudio.
    stream->setUrlPreResolveEnabled(gSavedSettings.getS32("Stream3DUrlPreResolve") != 0);

    std::vector<LLPositionalStreamMulti::SpeakerConfig> configs;
    configs.reserve(binding.speakers.size());
    for (const auto& s : binding.speakers)
    {
        LLPositionalStreamMulti::SpeakerConfig c;
        c.ch = toMultiChannel(s.ch);
        c.range = s.range;
        c.volume = s.volume;
        // Initial position: best effort. Speaker prims that arrived in the
        // poll cache before their viewer object did get a zero vector here;
        // the next update tick replaces it with the real getPositionGlobal().
        if (LLViewerObject* obj = gObjectList.findObject(s.prim_id))
        {
            if (!obj->isDead())
            {
                c.position = toFloatVec(obj->getPositionGlobal());
            }
        }
        configs.push_back(c);
    }

    bool started = false;
    if (source_is_media)
    {
        if (media_impl)
        {
            media_impl->setStream3DAudioRedirected(true);
        }
        started = stream->startMedia(media_ring, url, configs, source_key.media_source_channels);
        if (!started && media_impl)
        {
            media_impl->setStream3DAudioRedirected(false);
        }
    }
    else
    {
        started = stream->start(url, configs);
    }

    if (!started)
    {
        LL_WARNS("Stream3D") << "[3dstream-stereo] stream start failed root="
                              << root_id << " url=" << url << LL_ENDL;
        notifyDistributedError(root_id, DistErrorKind::StreamStartFailed, url);
        // Keep the binding metadata so a subsequent re-evaluation (e.g.
        // network recovers) can retry without re-walking the linkset; the
        // missing stream is the signal that we owe a retry.
    }
    else
    {
        binding.stream = std::move(stream);
    }

    LL_INFOS("Stream3D") << "[3dstream-stereo] binding "
                          << (was_present ? "rebuilt" : "constructed")
                          << " root=" << root_id
                          << " url=" << url
                          << " speakers=" << binding.speakers.size()
                          << " (dropped=" << dropped << ")"
                          << " stream=" << (binding.stream ? "started" : "deferred")
                          << LL_ENDL;

    // r10 P5: structural change resets the diagnostic key so the next time
    // the rebuilt stream reaches Playing the diagnostic re-emits with the
    // new (url, speaker_set, observed_channel_count) tuple. r12 P4: same
    // treatment for the upmix auto-bypass notice — a rebuild is the only
    // time we want to re-announce the fall-through.
    binding.last_diagnostic_key.clear();
    binding.last_upmix_notice_key.clear();
}

void LLPositionalStreamMgr::emitRoutingDiagnostic(DistributedStereoBinding& b)
{
    // Caller is responsible for the "stream alive" gate, but be defensive:
    // a Failed binding sneaking through here would emit a 0ch line.
    if (!b.stream || !b.stream->isPlaying()) return;
    const int source_channels = b.stream->sourceChannels();
    if (source_channels <= 0) return;

    // §4.4.2 throttle key. The prim_set_signature is the sorted concatenation
    // of (uuid|ch_int) so reordering speakers in the description (without
    // changing membership) doesn't re-fire the diagnostic.
    std::vector<std::string> sig_entries;
    sig_entries.reserve(b.speakers.size());
    for (const auto& s : b.speakers)
    {
        sig_entries.push_back(s.prim_id.asString() + "|"
                              + std::to_string(static_cast<int>(s.ch)));
    }
    std::sort(sig_entries.begin(), sig_entries.end());
    std::string prim_signature;
    for (const auto& e : sig_entries) prim_signature += e + ";";

    std::string key = b.root_id.asString() + "#" + b.url + "#"
                      + std::to_string(source_channels) + "#" + prim_signature;
    if (b.last_diagnostic_key == key) return;
    b.last_diagnostic_key = key;

    // §4.4.3 (r10.x): setting gate. The key is updated above regardless so
    // that toggling the setting on doesn't dump stale notifications for a
    // binding that hasn't actually changed structure since last emit.
    if (!gSavedSettings.getBOOL("Stream3DRoutingDiagnostic")) return;

    // Per-ch speaker count. r10 receives any of L/R/M/FL/FR/C/LFE/SL/SR/BL/BR
    // (older r5–r9 viewers only emit L/R/M, which is still a valid subset).
    std::map<ChannelKind, int> ch_count;
    for (const auto& s : b.speakers) ++ch_count[s.ch];

    const bool has_lrm_bucket = (ch_count[ChannelKind::L]
                                 + ch_count[ChannelKind::R]
                                 + ch_count[ChannelKind::M]) > 0;

    // §4.4.1 fallback notifications — emit one chat line per case via
    // notifyStream3D (which prefixes "3D Stream: " automatically). Source
    // is normative, prim layout is dependent: walk source-side first to
    // catch missing dedicated prims, then prim-side for compat fallbacks.

    // (a) source-side: 6ch source with missing dedicated prim → folded into
    // the BS.775 downmix, or dropped entirely if no L/R/M prim exists.
    if (source_channels == 6 || source_channels == 8)
    {
        static constexpr ChannelKind kSrc8[] = {
            ChannelKind::FL, ChannelKind::FR, ChannelKind::C,
            ChannelKind::LFE, ChannelKind::SL, ChannelKind::SR,
            ChannelKind::BL, ChannelKind::BR,
        };
        for (auto sc : kSrc8)
        {
            if (source_channels == 6 &&
                (sc == ChannelKind::BL || sc == ChannelKind::BR))
            {
                continue;
            }
            if (ch_count[sc] > 0) continue;
            const char* name = channelKindLabel(sc);
            std::ostringstream line;
            if (has_lrm_bucket && source_channels == 6)
            {
                line << name << " content folded into BS.775 downmix"
                     << " (source is 6ch, no ch:" << name << " prim)";
            }
            else
            {
                line << name
                     << " content has no destination \xe2\x80\x94 dropped"
                     << " (source is " << source_channels << "ch, no ch:"
                     << name << " prim)";
            }
            notifyStream3D(line.str());
        }
    }

    // r12.1: when upmix is effective on a 2ch source, every 5.1 placement
    // prim (FL/FR/C/LFE/SL/SR) is fed by the DPL2 dispatch — the §4.2
    // compat-fallback messages below ("ch:LFE prim silent" etc.) describe
    // the r10 path that upmix replaces and would mislead the listener into
    // thinking those speakers were disabled. Skip the per-prim block and
    // emit a single positive notice instead. ch:L/R/M prims still get the
    // upmix dispatch too but those weren't warned in the r10 path either.
    if (b.upmix_effective_applied && source_channels == 2)
    {
        bool has_51_prim = false;
        for (auto pc : { ChannelKind::FL, ChannelKind::FR, ChannelKind::C,
                         ChannelKind::LFE, ChannelKind::SL, ChannelKind::SR,
                         ChannelKind::BL, ChannelKind::BR })
        {
            if (ch_count[pc] > 0) { has_51_prim = true; break; }
        }
        if (has_51_prim)
        {
            notifyStream3D("DPL2 upmix active — 5.1 placement fan-out "
                           "from 2ch source");
        }
        return;
    }

    // (b) prim-side: dedicated 5.1 prim (FL/FR/C/LFE/SL/SR) on 1ch or 2ch
    // source → compat fallback per §4.2 matrix. ch:L/R/M prims always have
    // a sensible mapping and are not warned (§4.4.1 row 5 "通知不要").
    static constexpr ChannelKind kPrimSurround[] = {
        ChannelKind::FL, ChannelKind::FR, ChannelKind::C,
        ChannelKind::LFE, ChannelKind::SL, ChannelKind::SR,
        ChannelKind::BL, ChannelKind::BR,
    };

    // r12: when upmix is engaged on a 2ch source, the 5.1 prims are not in
    // compat fallback — they receive their canonical band via the upmix DSP.
    // Replace the six per-prim "silent / playing X" lines with one summary
    // (mirrors emitUpmixAutoBypassNotice symmetry: one line per non-default
    // upmix outcome).
    if (b.upmix_effective_applied && source_channels == 2)
    {
        bool any_5_1 = false;
        for (auto pc : kPrimSurround) { if (ch_count[pc] > 0) { any_5_1 = true; break; } }
        if (any_5_1)
        {
            notifyStream3D("5.1 prims fed by stereo\xe2\x86\x92" "6ch upmix DSP");
        }
        return;
    }

    for (auto pc : kPrimSurround)
    {
        if (ch_count[pc] == 0) continue;
        if (source_channels == 6 &&
            pc != ChannelKind::BL && pc != ChannelKind::BR)
        {
            continue; // direct, no warn
        }
        if (source_channels == 8) continue; // direct, no warn

        const char* name = channelKindLabel(pc);
        const char* dest = nullptr;
        bool silent = false;
        switch (pc)
        {
            case ChannelKind::FL:
                dest = (source_channels == 2) ? "L" : "M";
                break;
            case ChannelKind::FR:
                dest = (source_channels == 2) ? "R" : "M";
                break;
            case ChannelKind::C:
                dest = "M";
                break;
            case ChannelKind::LFE:
            case ChannelKind::SL:
            case ChannelKind::SR:
            case ChannelKind::BL:
            case ChannelKind::BR:
                silent = true;
                break;
            default:
                break;
        }

        std::ostringstream line;
        if (silent)
        {
            line << "ch:" << name << " prim silent"
                 << " (source is " << source_channels << "ch)";
        }
        else
        {
            line << "ch:" << name << " prim playing " << dest
                 << " (source is " << source_channels << "ch)";
        }
        notifyStream3D(line.str());
    }
}

void LLPositionalStreamMgr::emitUpmixAutoBypassNotice(DistributedStereoBinding& b)
{
    // Cheap-path early returns first — this is called every update tick
    // for every binding that's reached Playing. Until P5 wires the tag
    // parser the first guard alone short-circuits all of P4.
    if (!b.upmix_effective_applied) return;
    if (!b.stream || !b.stream->isPlaying()) return;
    const int source_channels = b.stream->sourceChannels();
    if (source_channels < 6) return;  // 1ch / 2ch get the dispatch they asked for

    // Throttle key shape mirrors last_diagnostic_key (root#url#sch) plus
    // a fixed "upmix-auto-bypass" tag. We don't fold upmix_effective_applied
    // into the key because reaching this point already implies it's true;
    // a fall back to false will come with a structural rebuild that clears
    // the key in (re)buildDistributedBinding.
    std::string key = b.root_id.asString() + "#" + b.url + "#"
                      + std::to_string(source_channels) + "#upmix-auto-bypass";
    if (b.last_upmix_notice_key == key) return;
    b.last_upmix_notice_key = key;

    std::ostringstream line;
    line << "{upmix:on} requested but source is " << source_channels
         << "ch native — keeping r10 placement (auto-bypass)";
    notifyStream3D(line.str());
}

void LLPositionalStreamMgr::enqueuePriorityPoll(const LLUUID& id)
{
    // O(N) dedup; queue is small in practice (≤ ~16 per pending linkset).
    for (const auto& q : mPriorityPollQueue)
    {
        if (q == id) return;
    }
    mPriorityPollQueue.push_back(id);
}

void LLPositionalStreamMgr::bootstrapChildDescriptions(LLViewerObject* root_obj)
{
    // r13: see header. Mirror of the evaluateLinkset child-scan loop, but
    // exposed for occlude tag bootstrap (and any future sibling system that
    // needs a root's children's Description). The dedup in
    // requestChildDescViaSelect (mPendingChildDeselect.try_emplace) makes
    // repeat calls idempotent, so callers can fire on every root Desc update
    // without throttling themselves.
    if (!root_obj || root_obj->isDead()) return;
    for (const auto& child : root_obj->getChildren())
    {
        if (!child || child->isDead()) continue;
        requestChildDescViaSelect(child.get());
    }
}

void LLPositionalStreamMgr::onMediaSourceDestroying(LLViewerMediaImpl* media)
{
    if (!media)
    {
        return;
    }

    const LLUUID media_id = media->getMediaTextureID();
    for (auto& [root_id, b] : mDistributedBindings)
    {
        if (b.source_key.kind != DistSourceKind::Media || !b.stream)
        {
            continue;
        }

        const bool same_media_id =
            media_id.notNull() &&
            b.source_key.media_id.notNull() &&
            b.source_key.media_id == media_id;
        if (!same_media_id && findMediaFor3DSource(b.source_key) != media)
        {
            continue;
        }

        LL_INFOS("Stream3D") << "[3dstream-stereo] media source destroying for root "
                              << root_id
                              << "; detaching 3D audio ring before plugin teardown"
                              << LL_ENDL;
        b.stream->setMediaRingFor3DStream(nullptr);
        b.next_retry_time = 0.0;
        b.last_pushed_volume = std::numeric_limits<F32>::quiet_NaN();
        media->setStream3DAudioRedirected(false);
    }
}

void LLPositionalStreamMgr::requestChildDescViaSelect(LLViewerObject* child)
{
    // r8 F11: see header. We bypass LLSelectMgr deliberately — going through
    // selectObjectAndFamily would mutate gEditMenuHandler, raise selection
    // beams, and clobber the user's actual selection. The bare wire-level
    // ObjectSelect is enough to coax the sim into emitting full
    // ObjectProperties (which carries Description) for this child.
    if (!child || child->isDead()) return;

    LLViewerRegion* region = child->getRegion();
    if (!region) return;

    // If we already have a deselect pending for this prim, the previous select
    // is still in flight (or its reply hasn't been processed yet). Bumping the
    // due-time keeps the slot alive a bit longer instead of double-selecting.
    const F64 now = LLTimer::getElapsedSeconds();
    static constexpr F64 kSelectHoldSeconds = 1.0;
    auto [it, inserted] = mPendingChildDeselect.try_emplace(
        child->getID(), now + kSelectHoldSeconds);
    if (!inserted)
    {
        it->second = now + kSelectHoldSeconds;
        return;
    }

    LLMessageSystem* msg = gMessageSystem;
    msg->newMessageFast(_PREHASH_ObjectSelect);
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
    msg->nextBlockFast(_PREHASH_ObjectData);
    msg->addU32Fast(_PREHASH_ObjectLocalID, child->getLocalID());
    msg->sendReliable(region->getHost());
}

void LLPositionalStreamMgr::drainChildDeselects(F64 now)
{
    if (mPendingChildDeselect.empty()) return;

    for (auto it = mPendingChildDeselect.begin(); it != mPendingChildDeselect.end(); )
    {
        if (it->second > now)
        {
            ++it;
            continue;
        }

        LLViewerObject* obj = gObjectList.findObject(it->first);
        if (!obj || obj->isDead())
        {
            // Object gone — sim already cleared its half of the selection
            // when it emitted KillObject, so we just drop the entry.
            it = mPendingChildDeselect.erase(it);
            continue;
        }

        // r8 F11: user-collision guard. If LLSelectMgr has the prim in its
        // local selection (build tools open, edit-link picker, etc.), then
        // there's an authoritative ObjectSelect from the user side that
        // outlives our phantom one. Sending our deselect would clobber the
        // user's selection on the sim while their viewer still thinks it
        // owns the slot — so we drop the entry and let the user's lifecycle
        // manage deselect.
        if (obj->isSelected())
        {
            it = mPendingChildDeselect.erase(it);
            continue;
        }

        LLViewerRegion* region = obj->getRegion();
        if (region)
        {
            LLMessageSystem* msg = gMessageSystem;
            msg->newMessageFast(_PREHASH_ObjectDeselect);
            msg->nextBlockFast(_PREHASH_AgentData);
            msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
            msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
            msg->nextBlockFast(_PREHASH_ObjectData);
            msg->addU32Fast(_PREHASH_ObjectLocalID, obj->getLocalID());
            msg->sendReliable(region->getHost());
        }

        it = mPendingChildDeselect.erase(it);
    }
}

void LLPositionalStreamMgr::detectLinksetStructureChanges()
{
    if (mPrimToRoot.empty()) return;

    // Collect deltas first, then re-evaluate — calling evaluateLinkset while
    // iterating mPrimToRoot would mutate it under us.
    std::set<LLUUID> roots_to_reeval;
    for (const auto& [prim_id, registered_root] : mPrimToRoot)
    {
        LLViewerObject* obj = gObjectList.findObject(prim_id);
        if (!obj || obj->isDead())
        {
            // Prim gone (e.g. derez). Re-evaluating the registered root will
            // drop this slot and either rebuild or tear down the binding.
            roots_to_reeval.insert(registered_root);
            continue;
        }
        LLViewerObject* current_root_obj = obj->getRootEdit();
        const LLUUID current_root = current_root_obj
            ? current_root_obj->getID() : prim_id;
        if (current_root != registered_root)
        {
            // link / unlink moved this prim across linksets.
            roots_to_reeval.insert(registered_root);
            roots_to_reeval.insert(current_root);
        }
    }

    for (const auto& r : roots_to_reeval)
    {
        LL_INFOS("Stream3D") << "[3dstream-stereo] linkset structure change "
                              << "— re-evaluating root " << r << LL_ENDL;
        evaluateLinkset(r);
    }
}

void LLPositionalStreamMgr::teardownDistributedBinding(const LLUUID& root_id)
{
    auto it = mDistributedBindings.find(root_id);
    if (it == mDistributedBindings.end()) return;

    LL_INFOS("Stream3D") << "[3dstream-stereo] tearing down binding root="
                          << root_id
                          << " speakers=" << it->second.speakers.size()
                          << LL_ENDL;

    for (const auto& s : it->second.speakers)
    {
        auto pr_it = mPrimToRoot.find(s.prim_id);
        if (pr_it != mPrimToRoot.end() && pr_it->second == root_id)
        {
            mPrimToRoot.erase(pr_it);
        }
    }
    if (it->second.source_key.kind == DistSourceKind::Media)
    {
        if (LLViewerMediaImpl* media = findMediaFor3DSource(it->second.source_key))
        {
            media->setStream3DAudioRedirected(false);
        }
    }
    // r11 P8: revert the bus-level reverb to dry on teardown so a
    // single-publisher session leaves the reverb tail closed; otherwise
    // an unrelated future stream would inherit this binding's venue.
    // Last-writer-wins under multi-publisher load is acceptable for r11
    // (one DSP per bus) — spec §4.5 names this as a known limitation.
    // r11 P9: also drop wetgain back to 0.0 (silent wet path) for the
    // same reason — a future binding will push its own value before
    // its first audio block.
    if (LLAudioEngine_FMODSTUDIO* engine = dynamic_cast<LLAudioEngine_FMODSTUDIO*>(gAudiop))
    {
        if (LLVenueReverbDsp* dsp = engine->getVenueReverbDsp())
        {
            if (it->second.venue_effective_applied != "dry")
            {
                dsp->setVenue("dry");
            }
            dsp->setWetGain(0.f);
        }
    }
    mDistributedBindings.erase(it);
}

void LLPositionalStreamMgr::evaluateMonoBinding(const LLUUID& id, const TagData& tag)
{
    LLViewerObject* obj = gObjectList.findObject(id);
    if (!obj || obj->isDead())
    {
        return;
    }

    F32 want_min, want_max;
    resolveRolloffFromTag(tag, want_min, want_max);
    LLVector3 pos = toFloatVec(obj->getPositionGlobal());

    auto bind_it = mBindings.find(id);
    if (bind_it != mBindings.end())
    {
        Binding& b = bind_it->second;
        if (b.url == tag.url)
        {
            b.tag_min = tag.min;
            b.tag_max = tag.max;
            if (b.applied_min != want_min || b.applied_max != want_max)
            {
                b.stream->setRolloffDistances(want_min, want_max);
                b.applied_min = want_min;
                b.applied_max = want_max;
            }
            return;
        }
        LL_INFOS("Stream3D") << "Rebinding " << id << ": URL changed" << LL_ENDL;
        mBindings.erase(bind_it);
    }

    const S32 cap = gSavedSettings.getS32("Stream3DMaxConcurrent");
    if (cap > 0 && static_cast<S32>(mBindings.size()) >= cap)
    {
        LL_WARNS("Stream3D") << "Max concurrent streams (" << cap
                              << ") reached; not binding " << id << LL_ENDL;
        if (!mCapNotified)
        {
            mCapNotified = true;
            notifyStream3D(llformat(
                "max concurrent streams (%d) reached; new tagged prims will be ignored until one is released.",
                cap));
        }
        return;
    }

    auto stream = std::make_unique<LLPositionalStream>();
    stream->setRolloffDistances(want_min, want_max);
    stream->setVolume(gSavedSettings.getF32("Stream3DVolumeMaster"));
    if (!stream->start(tag.url, pos))
    {
        return;
    }

    Binding b;
    b.url = tag.url;
    b.tag_min = tag.min;
    b.tag_max = tag.max;
    b.applied_min = want_min;
    b.applied_max = want_max;
    b.stream = std::move(stream);
    // r23: seed parcel-gate state. is_attached pins the eval tier (per-frame
    // vs. signal-driven); parcel_audible is the initial gate so the first
    // per-poll push in update() converges immediately without a 1-frame leak.
    b.is_attached = obj->isAttachment();
    b.parcel_audible = LLViewerParcelMgr::getInstance()->canHearSound(obj->getPositionGlobal());
    mBindings.emplace(id, std::move(b));

    LL_INFOS("Stream3D") << "Bound positional stream to " << id
                          << " url=" << tag.url << LL_ENDL;
}

static LLTrace::BlockTimerStatHandle FTM_STREAM3D_MGR_UPDATE("Stream3D Mgr Update");

void LLPositionalStreamMgr::update()
{
    LL_RECORD_BLOCK_TIME(FTM_STREAM3D_MGR_UPDATE);

    // M8: master kill switch. Listener already tore down state when the
    // setting flipped to false, so just bail.
    if (!gSavedSettings.getBOOL("Stream3DEnabled"))
    {
        return;
    }

    // r23: late-bind the parcel-change callback. The static singleton is
    // built at process start when gAgent may not yet be alive, so we defer
    // the connect to here where update() can only run post-login. Once
    // connected, the early-return is a single bool load per tick.
    ensureParcelCallbackRegistered();

    // M8: re-arm the cap notification once we drop back under the cap, so a
    // future cap-hit triggers a fresh toast rather than being suppressed by
    // the stale flag.
    {
        const S32 cap = gSavedSettings.getS32("Stream3DMaxConcurrent");
        const S32 total = static_cast<S32>(mBindings.size());
        if (mCapNotified && (cap <= 0 || total < cap))
        {
            mCapNotified = false;
        }
    }

    const F64 now = LLTimer::getElapsedSeconds();
    pollObjectPropertiesFamily(now);

    // r8 F11: release child prims we briefly selected to force a full
    // ObjectProperties reply. Runs before the linkset-eval drain so the
    // ObjectDeselect goes out in the same frame the now-cached desc is
    // consumed — the sim never sees a long-held selection on our behalf.
    drainChildDeselects(now);

    // r8 F8: drain pending linkset re-evaluations. A selection-induced
    // ObjectProperties message can deliver 16 child descriptions back-to-
    // back; the previous "evaluate per reply" path rebuilt the FMOD multi-
    // stream once for every reply, blocking the main thread for several
    // seconds. Set semantics dedup the root ids so each affected linkset
    // rebuilds at most once.
    //
    // r13: rate-limit to one root per frame. evaluateLinkset() ultimately
    // calls stream->start() which still does a synchronous libcurl HEAD
    // pre-resolve for https:// URLs (~1.5s timeout post-r13 B). At login
    // ObjectProperties replies for every tagged prim arrive in the same
    // frame; draining all in one frame stacks N × 1.5s of main-thread
    // block and trips the OS unresponsive dialog. Spreading 1/frame caps
    // worst-case to a single resolve per frame at the cost of N extra
    // frames before all bindings are live (imperceptible at 60fps).
    if (!mPendingLinksetEval.empty())
    {
        auto first = mPendingLinksetEval.begin();
        const LLUUID root_id = *first;
        mPendingLinksetEval.erase(first);
        evaluateLinkset(root_id);
    }

    if (mDebugStream)
    {
        mDebugStream->update();
    }

    if (mDebugStereoStream)
    {
        mDebugStereoStream->update();
    }

    // M7: fixed 5s wait between reconnect attempts. Total worst-case downtime
    // before a binding is dropped == Stream3DReconnectAttempts * kRetryDelay.
    static constexpr F64 kRetryDelay = 5.0;
    const S32 max_attempts = gSavedSettings.getS32("Stream3DReconnectAttempts");

    for (auto it = mBindings.begin(); it != mBindings.end(); )
    {
        const LLUUID& id = it->first;
        Binding& b = it->second;

        LLViewerObject* obj = gObjectList.findObject(id);
        if (!obj || obj->isDead())
        {
            LL_INFOS("Stream3D") << "Object " << id
                                  << " gone; releasing positional stream" << LL_ENDL;
            it = mBindings.erase(it);
            continue;
        }

        if (b.stream->isFailed())
        {
            if (max_attempts <= 0)
            {
                LL_WARNS("Stream3D") << "Stream for " << id
                                      << " failed; auto-reconnect disabled, dropping binding"
                                      << LL_ENDL;
                it = mBindings.erase(it);
                continue;
            }
            if (b.reconnect_attempts >= max_attempts)
            {
                LL_WARNS("Stream3D") << "Stream for " << id << " exhausted "
                                      << max_attempts << " reconnect attempts; dropping binding"
                                      << LL_ENDL;
                notifyStream3D("stream gave up after "
                                + llformat("%d", max_attempts)
                                + " reconnect attempts: " + b.url);
                it = mBindings.erase(it);
                continue;
            }
            if (b.next_retry_time == 0.0)
            {
                b.next_retry_time = now + kRetryDelay;
                LL_INFOS("Stream3D") << "Stream for " << id
                                      << " failed; scheduling reconnect "
                                      << (b.reconnect_attempts + 1) << "/" << max_attempts
                                      << " in " << kRetryDelay << "s" << LL_ENDL;
            }
            else if (now >= b.next_retry_time)
            {
                ++b.reconnect_attempts;
                b.next_retry_time = 0.0;
                const LLVector3 pos = toFloatVec(obj->getPositionGlobal());
                b.stream->setRolloffDistances(b.applied_min, b.applied_max);
                b.stream->setVolume(gSavedSettings.getF32("Stream3DVolumeMaster"));
                // r23: reset the idempotent guard so the per-poll push next
                // frame applies the parcel gate to the fresh FMOD channel.
                b.last_pushed_volume = std::numeric_limits<F32>::quiet_NaN();
                LL_INFOS("Stream3D") << "Reconnect attempt " << b.reconnect_attempts
                                      << "/" << max_attempts << " for " << id
                                      << " url=" << b.url << LL_ENDL;
                b.stream->start(b.url, pos);
            }
            ++it;
            continue;
        }

        // Successful playback resets the retry counter so a later, independent
        // failure gets its own full budget rather than inheriting old strikes.
        if (b.reconnect_attempts > 0 && b.stream->isPlaying())
        {
            LL_INFOS("Stream3D") << "Reconnect succeeded for " << id
                                  << " after " << b.reconnect_attempts << " attempt(s)"
                                  << LL_ENDL;
            b.reconnect_attempts = 0;
            b.next_retry_time = 0.0;
        }

        // M8: notify exactly once per Binding, when it first reaches Playing.
        // Reconnect successes don't re-fire because Binding (and the flag)
        // survives across stream->start() retries.
        if (!b.notified_played && b.stream->isPlaying())
        {
            b.notified_played = true;
            notifyStream3D("now playing: " + b.url);
        }

        b.stream->setPosition(toFloatVec(obj->getPositionGlobal()));
        // r23: Tier 2 — attached source moves per-frame, so re-evaluate the
        // parcel gate every tick for this binding. Static-source (Tier 1)
        // bindings rely on the cached value last refreshed by the
        // parcel-change signal in onAgentParcelChanged().
        if (b.is_attached)
        {
            b.parcel_audible = computeParcelAudible(id, b.parcel_audible);
        }
        // r12.1: same per-poll master-volume push as the distributed
        // bindings loop below, so a Stream3DVolumeMaster edit propagates
        // without waiting for the next reconnect.
        // r23: gate volume by parcel_audible (0 when muted by SL parcel
        // SOUND_LOCAL rule), and skip the FMOD setVolume call when the
        // effective value hasn't changed since the last push.
        {
            const F32 master_vol = gSavedSettings.getF32("Stream3DVolumeMaster");
            const F32 effective_vol = b.parcel_audible ? master_vol : 0.f;
            if (std::isnan(b.last_pushed_volume) || b.last_pushed_volume != effective_vol)
            {
                b.stream->setVolume(effective_vol);
                b.last_pushed_volume = effective_vol;
            }
        }
        b.stream->update();
        ++it;
    }

    // r8 F3-3: distributed-stereo bindings. F7 mirrors the mono Bindings loop:
    // socket-level outages flip stream → State::Failed, this loop runs the
    // retry budget (Stream3DReconnectAttempts × 5s) and either reconnects or
    // drops the binding. F4 still owns parse-time error notifications inside
    // evaluateLinkset.
    constexpr F64 kRetryDelayDist = 5.0;
    const S32 max_attempts_dist = gSavedSettings.getS32("Stream3DReconnectAttempts");
    const F64 now_dist = LLTimer::getElapsedSeconds();

    // r13: refresh OBB occluders once per tick. Drops dead/de-tagged prims,
    // re-fetches transform, and re-parses {direct=N}{reverb=N} fields. Cost
    // is ~10us at the 64-prim cap; no-op when no occluders are registered.
    LLOcclusionGeometryMgr::instance().refreshOccluders();

    std::vector<LLUUID> dead_roots;
    for (auto& [root_id, b] : mDistributedBindings)
    {
        LLViewerObject* root = gObjectList.findObject(root_id);
        if (!root || root->isDead())
        {
            dead_roots.push_back(root_id);
            continue;
        }
        // r10.x: link/unlink demoted this prim — the binding's UUID is no
        // longer a root. The new root (already evaluated separately) owns
        // the speakers; the stale binding here would compete with it,
        // double-playing the same source out of two streams. Tear down on
        // demotion so mDistributedBindings only ever holds live root keys.
        if (root->getRootEdit() && root->getRootEdit()->getID() != root_id)
        {
            LL_INFOS("Stream3D") << "[3dstream-stereo] root " << root_id
                                  << " demoted to child of "
                                  << root->getRootEdit()->getID()
                                  << "; tearing down stale binding" << LL_ENDL;
            dead_roots.push_back(root_id);
            continue;
        }
        if (!b.stream)
        {
            // Stream couldn't be built yet (deferred above). Re-evaluating
            // here is not appropriate — that's the poll loop's job once the
            // descriptions / network resolve. Leave it for now.
            continue;
        }

        if (b.source_key.kind == DistSourceKind::Media)
        {
            LLPluginAudioRingHeader* ring = nullptr;
            if (LLViewerMediaImpl* media = findMediaFor3DSource(b.source_key))
            {
                media->setStream3DAudioRedirected(true);
                ring = media->getAudioRingForStream3D();
            }
            b.stream->setMediaRingFor3DStream(ring);
        }

        if (b.stream->isFailed())
        {
            // r9 P6: format mismatches (3/4/5/7/8ch source, or 6ch in a
            // codec we don't have a layout for) won't get better on retry.
            // Skip the reconnect budget, surface a clear notification once,
            // and drop the binding.
            if (b.stream->failReason() == LLPositionalStreamMulti::FailReason::FormatUnsupported)
            {
                LL_WARNS("Stream3D") << "[3dstream-stereo] stream for root "
                                      << root_id
                                      << " has unsupported source format ("
                                      << b.stream->failDetail()
                                      << "); not retrying" << LL_ENDL;
                notifyDistributedError(root_id,
                                       DistErrorKind::UnsupportedSourceFormat,
                                       b.stream->failDetail());
                // r9 P6.5: remember (root, url) so the next desc poll's
                // evaluateLinkset doesn't rebuild this binding and re-open
                // the stream just to fail again. Cleared on URL change.
                if (b.source_key.kind == DistSourceKind::Url)
                {
                    mFormatFailedUrl[root_id] = b.url;
                }
                dead_roots.push_back(root_id);
                continue;
            }
            if (max_attempts_dist <= 0)
            {
                LL_WARNS("Stream3D") << "[3dstream-stereo] stream for root "
                                      << root_id
                                      << " failed; auto-reconnect disabled, dropping binding"
                                      << LL_ENDL;
                dead_roots.push_back(root_id);
                continue;
            }
            if (b.reconnect_attempts >= max_attempts_dist)
            {
                LL_WARNS("Stream3D") << "[3dstream-stereo] stream for root "
                                      << root_id << " exhausted "
                                      << max_attempts_dist
                                      << " reconnect attempts; dropping binding"
                                      << LL_ENDL;
                notifyStream3D("stream gave up after "
                                + llformat("%d", max_attempts_dist)
                                + " reconnect attempts: " + b.url);
                dead_roots.push_back(root_id);
                continue;
            }
            if (b.next_retry_time == 0.0)
            {
                b.next_retry_time = now_dist + kRetryDelayDist;
                LL_INFOS("Stream3D") << "[3dstream-stereo] stream for root "
                                      << root_id << " failed; scheduling reconnect "
                                      << (b.reconnect_attempts + 1) << "/"
                                      << max_attempts_dist
                                      << " in " << kRetryDelayDist << "s" << LL_ENDL;
            }
            else if (now_dist >= b.next_retry_time)
            {
                ++b.reconnect_attempts;
                b.next_retry_time = 0.0;
                std::vector<LLPositionalStreamMulti::SpeakerConfig> configs;
                configs.reserve(b.speakers.size());
                for (const auto& s : b.speakers)
                {
                    LLPositionalStreamMulti::SpeakerConfig c;
                    c.ch = toMultiChannel(s.ch);
                    c.range = s.range;
                    c.volume = s.volume;
                    if (LLViewerObject* sp = gObjectList.findObject(s.prim_id))
                    {
                        if (!sp->isDead())
                        {
                            c.position = toFloatVec(sp->getPositionGlobal());
                        }
                    }
                    configs.push_back(c);
                }
                LLViewerMediaImpl* reconnect_media =
                    b.source_key.kind == DistSourceKind::Media
                        ? findMediaFor3DSource(b.source_key)
                        : nullptr;
                b.stream->setVolume(effectiveDistributedStreamVolume(
                    gSavedSettings.getF32("Stream3DVolumeMaster"),
                    b.parcel_audible,
                    b.source_key.kind == DistSourceKind::Media,
                    b.media_source_uses_viewer_volume,
                    reconnect_media));
                // r23: reset idempotent guard (see mono path); per-poll
                // push next frame applies the parcel gate to the new
                // FMOD channels.
                b.last_pushed_volume = std::numeric_limits<F32>::quiet_NaN();
                LL_INFOS("Stream3D") << "[3dstream-stereo] reconnect attempt "
                                      << b.reconnect_attempts << "/" << max_attempts_dist
                                      << " for root " << root_id
                                      << " url=" << b.url << LL_ENDL;
                if (b.source_key.kind == DistSourceKind::Media)
                {
                    LLPluginAudioRingHeader* ring = nullptr;
                    LLViewerMediaImpl* media = reconnect_media;
                    if (media)
                    {
                        media->setStream3DAudioRedirected(true);
                        ring = media->getAudioRingForStream3D();
                    }
                    if (!b.stream->startMedia(ring, b.url, configs,
                                              b.source_key.media_source_channels))
                    {
                        if (media)
                        {
                            media->setStream3DAudioRedirected(false);
                        }
                    }
                }
                else
                {
                    b.stream->start(b.url, configs);
                }
            }
            // Skip position pushes & update() while Failed.
            continue;
        }

        // Successful playback resets the retry counter so a later, independent
        // outage gets its own full budget rather than inheriting old strikes.
        if (b.reconnect_attempts > 0 && b.stream->isPlaying())
        {
            LL_INFOS("Stream3D") << "[3dstream-stereo] reconnect succeeded for root "
                                  << root_id << " after "
                                  << b.reconnect_attempts << " attempt(s)" << LL_ENDL;
            b.reconnect_attempts = 0;
            b.next_retry_time = 0.0;
        }

        // Notify exactly once per binding when it first reaches Playing.
        // Reconnect successes don't re-fire because the binding (and the
        // flag) survives across stream->start() retries.
        if (!b.notified_played && b.stream->isPlaying())
        {
            b.notified_played = true;
            notifyStream3D("now playing: " + b.url);
        }

        // r10 P5: emit (or skip via throttle key) the routing diagnostic
        // once per (root, url, observed_channel_count, speaker_set) tuple.
        // Cheap on the no-op path — single key compare.
        emitRoutingDiagnostic(b);

        // r12 P4: same idea for the upmix auto-bypass notice. Bails out
        // immediately when the binding never asked for upmix (the common
        // case), so the per-tick cost is one bool load. The first real
        // caller appears in P5 once the tag parser sets
        // upmix_effective_applied.
        emitUpmixAutoBypassNotice(b);

        for (size_t i = 0; i < b.speakers.size(); ++i)
        {
            LLViewerObject* sp = gObjectList.findObject(b.speakers[i].prim_id);
            if (sp && !sp->isDead())
            {
                b.stream->setSpeakerPosition(i, toFloatVec(sp->getPositionGlobal()));
            }
            // r12.1: push the per-speaker {volume:N} value every poll so a
            // tag edit propagates without rebuilding the FMOD stream.
            // setSpeakerVolume is idempotent (early-returns when unchanged)
            // so the steady-state cost is one float compare per speaker.
            b.stream->setSpeakerVolume(i, b.speakers[i].volume);
        }
        // r12 P6: push the live-tunable upmix knobs every poll so a debug-
        // settings edit picks up at the next FMOD chunk boundary without a
        // stream rebuild. setUpmixTuning is a lock-free atomic write
        // (cheap), and the values only matter when this binding actually
        // dispatches OpKind::Upmix — but unconditional push is simpler and
        // costs three settings reads + three atomic stores per binding.
        b.stream->setUpmixTuning(
            gSavedSettings.getF32("Stream3DUpmixLfeCutoff"),
            gSavedSettings.getF32("Stream3DUpmixCenterBleed"),
            gSavedSettings.getF32("Stream3DUpmixRearDelayMs"));
        // r12.1: same idea for LFE gain — applyLfeGainToBinding is the
        // event-driven path (Desc parse, new binding) and was missing the
        // per-poll edge case where AYA edits Stream3DLfeGain in debug
        // settings without touching the prim. Idempotent (early-returns
        // when effective value unchanged) so cost is one settings read +
        // a float compare on the no-op path.
        applyLfeGainToBinding(b, b.lfegain_tag);
        // r12.1: same per-poll trigger for the reverb knobs (venue +
        // wet gain) and master volume. apply{Venue,WetGain}ToBinding
        // both early-return when the effective value matches the
        // last-applied snapshot, so changing Stream3DVenueOverride /
        // Stream3DVenueWetGain in debug settings now propagates without
        // requiring a prim Desc edit. Volume push is unconditional but
        // setVolume is itself a single FMOD::Channel::setVolume call
        // that's a no-op when unchanged.
        applyVenueToBinding(b, b.venue_tag);
        applyWetGainToBinding(b, b.wetgain_tag);
        // r23: Tier 2 refresh for attached-source distributed bindings.
        // Source = root prim position; same evaluation as the mono path.
        if (b.is_attached)
        {
            b.parcel_audible = computeParcelAudible(root_id, b.parcel_audible);
        }
        // r23 + media source: parcel gate first; media routed through 3D
        // also keeps the existing media UI/global volume semantics as a
        // source gain before per-speaker volume is applied by llaudio.
        {
            const F32 master_vol = gSavedSettings.getF32("Stream3DVolumeMaster");
            const bool is_media_source = b.source_key.kind == DistSourceKind::Media;
            LLViewerMediaImpl* media = is_media_source
                ? findMediaFor3DSource(b.source_key)
                : nullptr;
            const F32 effective_vol = effectiveDistributedStreamVolume(
                master_vol,
                b.parcel_audible,
                is_media_source,
                b.media_source_uses_viewer_volume,
                media);
            if (std::isnan(b.last_pushed_volume) || b.last_pushed_volume != effective_vol)
            {
                b.stream->setVolume(effective_vol);
                b.last_pushed_volume = effective_vol;
            }
        }
        b.stream->update();

        // r13: per-frame OBB occlusion eval. The shipped libfmod
        // (2.03.07) returns FMOD_ERR_INTERNAL from System::createGeometry
        // even on a 1-poly probe, so we ray-cast on our side and feed the
        // result into Channel::set3DOcclusion. Tag-driven cap keeps the
        // cost negligible (<0.5 ms/sec at the spec §4.7 hard cap of 200
        // occluders × 6 speakers × 60 Hz). No-op when no occluders are
        // registered (the visitor still resets each channel to 0/0 once).
        if (gAudiop)
        {
            const LLVector3 lpos = gAudiop->getListenerPos();
            b.stream->forEachActiveSpeaker(
                [&lpos](FMOD::Channel* ch, FMOD::DSP* lpf, const LLVector3& spos)
                {
                    LLOcclusionGeometryMgr::instance().applyToChannel(ch, lpf, lpos, spos);
                });
        }
    }

    // r13 P8: same occlusion pass for llPlaySound / attached-sound channels.
    // The engine-side visitor filters to positional (= non-forced-priority)
    // channels with a live FMOD handle, so 2D UI / preview sounds pass
    // through untouched. We run this once per mgr update tick — same
    // cadence as the stream loop above — so cutoff smoothing and ramp share
    // a single dt source. mOccluders.empty() short-circuit inside
    // applyToChannel keeps the no-occluder case cheap (one ramp step per
    // active channel to drive trailing values back to zero).
    if (auto* fe = dynamic_cast<LLAudioEngine_FMODSTUDIO*>(gAudiop))
    {
        const LLVector3 lpos = gAudiop->getListenerPos();
        fe->forEachActive3DSfxChannel(
            [&lpos](FMOD::Channel* ch, FMOD::DSP* lpf, const LLVector3& spos)
            {
                LLOcclusionGeometryMgr::instance().applyToChannel(ch, lpf, lpos, spos);
            });
    }
    for (const auto& r : dead_roots)
    {
        LL_INFOS("Stream3D") << "[3dstream-stereo] root " << r
                              << " gone; tearing down binding" << LL_ENDL;
        teardownDistributedBinding(r);
    }
}

void LLPositionalStreamMgr::applyDefaultRolloff(F32 default_min, F32 default_max)
{
    if (mDebugStream)
    {
        mDebugStream->setRolloffDistances(default_min, default_max);
    }

    if (mDebugStereoStream)
    {
        mDebugStereoStream->setRolloffDistances(default_min, default_max);
    }

    for (auto& [id, b] : mBindings)
    {
        F32 want_min = b.tag_min.value_or(default_min);
        F32 want_max = b.tag_max.value_or(default_max);
        if (b.applied_min != want_min || b.applied_max != want_max)
        {
            b.stream->setRolloffDistances(want_min, want_max);
            b.applied_min = want_min;
            b.applied_max = want_max;
        }
    }
}

void LLPositionalStreamMgr::shutdownPrimBindings()
{
    if (!mBindings.empty())
    {
        LL_INFOS("Stream3D") << "Tearing down " << mBindings.size()
                              << " mono prim bindings" << LL_ENDL;
        // ~Binding's unique_ptr<> destructor invokes the stream's stop(),
        // which calls FMOD Channel::stop() synchronously. Output device
        // buffer drain (<50ms typical) is the only residual delay.
        mBindings.clear();
    }
    if (!mDistributedBindings.empty())
    {
        LL_INFOS("Stream3D") << "Tearing down " << mDistributedBindings.size()
                              << " distributed-stereo bindings" << LL_ENDL;
        for (const auto& [root_id, b] : mDistributedBindings)
        {
            if (b.source_key.kind == DistSourceKind::Media)
            {
                if (LLViewerMediaImpl* media = findMediaFor3DSource(b.source_key))
                {
                    media->setStream3DAudioRedirected(false);
                }
            }
        }
        mDistributedBindings.clear();
        mPrimToRoot.clear();
    }
    // r8 F2-b: any pending priority polls were tied to bindings that no
    // longer exist; drop them so we don't burn poll budget after teardown.
    mPriorityPollQueue.clear();
    // r8 F8: deferred re-evaluations referenced roots we just tore down.
    mPendingLinksetEval.clear();
    // r8 F11: drain pending deselects synchronously instead of just clearing
    // — otherwise a kill-switch toggle within the 1 s hold window would leave
    // phantom selections on the sim. Forcing now=+infinity makes drainChild
    // Deselects fire every entry; the user-collision guard inside still
    // protects build-tool selections.
    drainChildDeselects(std::numeric_limits<F64>::max());
}

void LLPositionalStreamMgr::shutdownAll()
{
    shutdownPrimBindings();
    // r8 F4: drop throttle history so a fresh session starts with no stale
    // suppression. mErrorThrottle is not load-bearing across sessions.
    mErrorThrottle.clear();
    // r9 P6.5: same reasoning — the format-failed URL cache should not
    // survive a Stream3D-disable toggle (or app exit).
    mFormatFailedUrl.clear();
    if (mDebugStream)
    {
        LL_INFOS("Stream3D") << "Tearing down debug mono stream" << LL_ENDL;
        mDebugStream->stop();
        mDebugStream.reset();
    }
    if (mDebugStereoStream)
    {
        LL_INFOS("Stream3D") << "Tearing down debug stereo stream" << LL_ENDL;
        mDebugStereoStream->stop();
        mDebugStereoStream.reset();
    }
}

void LLPositionalStreamMgr::forceRescan()
{
    // Two-step rescan:
    //
    // (1) Re-evaluate every cached prim *immediately* using the description
    //     we already hold. evaluateBinding() does a string parse + (re)bind
    //     against mDescriptionCache without any network roundtrip, so tagged
    //     prims rebind within this tick — no need to wait for the round-robin
    //     scan walk to circle back (which can take >1 minute in dense regions
    //     where ObjectPropertiesFamily is rate-limited to 10 req/s).
    //
    // (2) Zero last_polled so the periodic poll loop will eventually re-issue
    //     ObjectPropertiesFamily for everything (in case any descriptions
    //     changed while Stream3DEnabled was off). This is the slow safety
    //     net; (1) is what users feel.
    //
    // mPollCursor is left alone — the round-robin walk picks up where it was.
    int rebind_candidates = 0;
    for (auto& kv : mDescriptionCache)
    {
        if (!kv.second.description.empty())
        {
            ++rebind_candidates;
            evaluateBinding(kv.first);
        }
        kv.second.last_polled = 0.0;
        // r8 F8: also reset the priority retry counter so prims that the sim
        // previously refused to reply for get a fresh shot at the fast retry
        // path on the next poll tick.
        kv.second.priority_retries = 0;
    }
    LL_INFOS("Stream3D") << "Forced rescan: " << rebind_candidates
                          << " cached descriptions re-evaluated, last_polled cleared on "
                          << mDescriptionCache.size() << " entries" << LL_ENDL;
}

void LLPositionalStreamMgr::applyMasterVolume(F32 volume)
{
    if (mDebugStream)
    {
        mDebugStream->setVolume(volume);
    }
    if (mDebugStereoStream)
    {
        mDebugStereoStream->setVolume(volume);
    }
    // r23: gate by the cached parcel_audible for prim-bound streams. The
    // per-poll loop in update() also pushes effective volume every tick;
    // this path covers the Stream3DVolumeMaster signal-driven case so the
    // user-visible slider responds in the same frame as the setting change.
    for (auto& [id, b] : mBindings)
    {
        const F32 effective = b.parcel_audible ? volume : 0.f;
        b.stream->setVolume(effective);
        b.last_pushed_volume = effective;
    }
    for (auto& [root_id, b] : mDistributedBindings)
    {
        if (b.stream)
        {
            const bool is_media_source = b.source_key.kind == DistSourceKind::Media;
            LLViewerMediaImpl* media = is_media_source
                ? findMediaFor3DSource(b.source_key)
                : nullptr;
            const F32 effective = effectiveDistributedStreamVolume(
                volume,
                b.parcel_audible,
                is_media_source,
                b.media_source_uses_viewer_volume,
                media);
            b.stream->setVolume(effective);
            b.last_pushed_volume = effective;
        }
    }
}

// r23: lazy callback registration. Called on every update() tick; the
// `connected()` early-return makes the steady-state cost a single bool load.
void LLPositionalStreamMgr::ensureParcelCallbackRegistered()
{
    if (mParcelChangedConn.connected())
    {
        return;
    }
    mParcelChangedConn = gAgent.addParcelChangedCallback(
        boost::bind(&LLPositionalStreamMgr::onAgentParcelChanged, this));
}

// r23: Tier 1 refresh on agent parcel-change. Walks every binding (mono +
// distributed) and re-evaluates its cached parcel_audible. Tier 2 (attached)
// bindings will also be refreshed in update()'s per-frame pass, but we
// eagerly refresh here too so the very first frame after the signal already
// reflects the new gate state.
void LLPositionalStreamMgr::onAgentParcelChanged()
{
    for (auto& [id, b] : mBindings)
    {
        b.parcel_audible = computeParcelAudible(id, b.parcel_audible);
    }
    for (auto& [root_id, b] : mDistributedBindings)
    {
        b.parcel_audible = computeParcelAudible(root_id, b.parcel_audible);
    }
}

// r23: one canHearSound() probe at the source prim's world position. When
// the prim has gone away or its position can't be resolved, return the
// caller-supplied fallback (typically the previous cached value) so a
// transient miss doesn't flip a binding to muted.
bool LLPositionalStreamMgr::computeParcelAudible(const LLUUID& source_id, bool fallback) const
{
    LLViewerObject* obj = gObjectList.findObject(source_id);
    if (!obj || obj->isDead())
    {
        return fallback;
    }
    return LLViewerParcelMgr::getInstance()->canHearSound(obj->getPositionGlobal());
}

void LLPositionalStreamMgr::startDebug(const std::string& url, const LLVector3& world_pos)
{
    if (!mDebugStream)
    {
        mDebugStream = std::make_unique<LLPositionalStream>();
    }
    mDebugStream->setRolloffDistances(
        gSavedSettings.getF32("Stream3DRolloffMin"),
        gSavedSettings.getF32("Stream3DRolloffMax"));
    mDebugStream->start(url, world_pos);
}

void LLPositionalStreamMgr::stopDebug()
{
    if (mDebugStream)
    {
        mDebugStream->stop();
        mDebugStream.reset();
    }
}

void LLPositionalStreamMgr::startDebugStereo(const std::string& url,
                                             const LLVector3& l_pos,
                                             const LLVector3& r_pos)
{
    if (!mDebugStereoStream)
    {
        mDebugStereoStream = std::make_unique<LLPositionalStreamStereo>();
    }
    mDebugStereoStream->setRolloffDistances(
        gSavedSettings.getF32("Stream3DRolloffMin"),
        gSavedSettings.getF32("Stream3DRolloffMax"));
    mDebugStereoStream->start(url, l_pos, r_pos);
}

void LLPositionalStreamMgr::stopDebugStereo()
{
    if (mDebugStereoStream)
    {
        mDebugStereoStream->stop();
        mDebugStereoStream.reset();
    }
}

void LLPositionalStreamMgr::pollObjectPropertiesFamily(F64 now)
{
    // M8: Stream3DDescriptionScan also gates the active poll. (Enabled is
    // checked one level up in update(); reaching here implies Enabled=true.)
    if (!gSavedSettings.getBOOL("Stream3DDescriptionScan"))
    {
        return;
    }

    // Run the scan at ~2 Hz; with kBudgetPerScan = 5 that caps outgoing
    // requests at ~10 req/s sustained, regardless of frame rate.
    // 10 req/s × 30s poll_interval = ~300 prims per cycle. In denser regions
    // a given prim's effective re-poll interval will exceed Stream3DPollInterval.
    static constexpr F64 kScanInterval  = 0.5;
    static constexpr int kBudgetPerScan = 5;
    // r8 F8: priority queue retry cadence. Far below Stream3DPollInterval so
    // a freshly discovered linkset finishes binding within seconds even when
    // the first Family request was dropped, but capped to kPriorityRetryCap
    // attempts so prims the sim refuses to answer for don't burn the whole
    // priority budget every tick. After the cap, round-robin (poll_interval
    // cadence) takes over silently.
    static constexpr F64 kPriorityRetryWait = 3.0;
    static constexpr S32 kPriorityRetryCap  = 5;

    if (now - mLastPollScanTime < kScanInterval)
    {
        return;
    }
    mLastPollScanTime = now;

    const F32 poll_interval = gSavedSettings.getF32("Stream3DPollInterval");
    const F32 max_dist      = gSavedSettings.getF32("Stream3DMaxDistance");

    // Per-scan breakdown for support / tuning. Off by default; enable with
    //   --logdebug Stream3D
    LL_DEBUGS("Stream3D") << "poll diag: interval=" << poll_interval
                           << " max_dist=" << max_dist
                           << " num_objects=" << gObjectList.getNumObjects()
                           << LL_ENDL;

    if (poll_interval <= 0.f)
    {
        return;
    }

    if (max_dist <= 0.f)
    {
        return;
    }
    const F32 max_dist_sq = max_dist * max_dist;
    const LLVector3d agent_pos = gAgent.getPositionGlobal();

    // r8 F2-c: spot link/unlink/death once per scan tick. This is cheap and
    // independent of the request budget — it only reads gObjectList and may
    // call evaluateLinkset (which itself may enqueue priority polls picked
    // up by the drain below).
    detectLinksetStructureChanges();

    int budget = kBudgetPerScan;

    // r8 F2-b / F8: drain the priority queue first. These are prims that
    // evaluateLinkset wants polled now (root or speaker that we don't yet
    // have description for). Distance / avatar filtering still applies.
    //
    // Retry policy (F8): a prim stays in the queue across drains until either
    // its reply arrives (last_replied stamped) or kPriorityRetryCap attempts
    // were spent. The previous policy used the same 30 s round-robin throttle
    // here, so a child whose first Family request was dropped by the sim sat
    // mute for 30 s before round-robin retried — observable as the "16 spk
    // linkset only plays 1 speaker until the user touches it" symptom.
    //
    // Snapshot the queue size before draining so re-queued items don't get
    // re-processed in the same tick (same prim could otherwise be sent twice
    // back-to-back when budget is large).
    int n_priority = 0;
    size_t to_process = mPriorityPollQueue.size();
    while (budget > 0 && to_process > 0 && !mPriorityPollQueue.empty())
    {
        --to_process;
        const LLUUID id = mPriorityPollQueue.front();
        mPriorityPollQueue.pop_front();

        LLViewerObject* obj = gObjectList.findObject(id);
        if (!obj || obj->isDead()) continue;
        if (obj->isAvatar() || obj->isAttachment() || obj->isHUDAttachment()) continue;

        LLVector3d delta = obj->getPositionGlobal();
        delta -= agent_pos;
        if (static_cast<F32>(delta.magVecSquared()) > max_dist_sq) continue;

        auto& entry = mDescriptionCache[id];

        // Reply already arrived: round-robin owns this prim now.
        if (entry.last_replied > 0.0) continue;
        // Spent the priority budget without a reply: let round-robin retry
        // at its slower cadence. Don't re-queue.
        if (entry.priority_retries >= kPriorityRetryCap) continue;
        // Within retry-wait window since the last send: keep the slot but
        // wait for the next drain.
        if (entry.last_polled > 0.0 && (now - entry.last_polled) < kPriorityRetryWait)
        {
            mPriorityPollQueue.push_back(id);
            continue;
        }

        LLSelectMgr::getInstance()->requestObjectPropertiesFamily(obj);
        entry.last_polled = now;
        entry.priority_retries++;
        --budget;
        ++n_priority;

        // Re-queue so the next drain checks whether the reply arrived and,
        // if not, retries (subject to the cap above).
        mPriorityPollQueue.push_back(id);
    }

    const S32 num = gObjectList.getNumObjects();
    if (num == 0)
    {
        return;
    }

    int n_total = 0, n_filtered = 0, n_far = 0, n_recent = 0, n_sent = 0;
    int examined = 0;

    // Walk in round-robin order so dense regions don't starve prims past
    // the first slice each pass can reach before exhausting the budget.
    while (budget > 0 && examined < num)
    {
        if (mPollCursor >= num)
        {
            mPollCursor = 0;
        }
        const S32 i = mPollCursor++;
        ++examined;

        LLViewerObject* obj = gObjectList.getObject(i);
        if (!obj || obj->isDead())
        {
            continue;
        }
        ++n_total;

        // Avatars / attachments / HUDs don't carry [3dstream:...] tags.
        if (obj->isAvatar() || obj->isAttachment() || obj->isHUDAttachment())
        {
            ++n_filtered;
            continue;
        }

        LLVector3d delta = obj->getPositionGlobal();
        delta -= agent_pos;
        if (static_cast<F32>(delta.magVecSquared()) > max_dist_sq)
        {
            ++n_far;
            continue;
        }

        const LLUUID& id = obj->getID();
        // operator[] auto-creates an empty entry; harmless — last_polled stamps
        // it so we don't re-request before the interval elapses.
        auto& entry = mDescriptionCache[id];
        if (entry.last_polled > 0.0 && (now - entry.last_polled) < poll_interval)
        {
            ++n_recent;
            continue;
        }

        LLSelectMgr::getInstance()->requestObjectPropertiesFamily(obj);
        entry.last_polled = now;
        --budget;
        ++n_sent;
    }

    LL_DEBUGS("Stream3D") << "poll: scanned this pass total=" << n_total
                           << " filtered=" << n_filtered
                           << " far=" << n_far
                           << " recent=" << n_recent
                           << " sent=" << n_sent
                           << " priority_sent=" << n_priority
                           << " priority_queue=" << mPriorityPollQueue.size()
                           << " examined=" << examined
                           << " cursor=" << mPollCursor
                           << "/" << num
                           << LL_ENDL;
}
