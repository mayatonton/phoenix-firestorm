/**
 * @file llayaupdatechecker.cpp
 * @brief AYAstorm GitHub release notification checker.
 */

#include "llviewerprecompiledheaders.h"

#include "llayaupdatechecker.h"

#include "llcorehttputil.h"
#include "llcoros.h"
#include "llhttpconstants.h"
#include "llstring.h"
#include "llregex.h"
#include "llversioninfo.h"
#include "llviewercontrol.h"
#include "llweb.h"

#include <boost/json.hpp>
#include <boost/lexical_cast.hpp>
#include <ctime>

namespace
{
const char* LOG_TAG = "AYAUpdate";

struct ParsedClassicTag
{
    S32 major = 0;
    S32 minor = 0;
    S32 patch = 0;
    S32 ayastorm_r = 0;
    S32 r_patch = 0;
    S32 bugfix = 0;
};

std::string jsonString(const boost::json::object& obj, const char* key)
{
    const boost::json::value* value = obj.if_contains(key);
    if (!value || !value->is_string())
    {
        return {};
    }

    const boost::json::string& str = value->as_string();
    return std::string(str.data(), str.size());
}

bool jsonBool(const boost::json::object& obj, const char* key, bool fallback = false)
{
    const boost::json::value* value = obj.if_contains(key);
    return value && value->is_bool() ? value->as_bool() : fallback;
}

bool parseClassicTag(const std::string& tag, ParsedClassicTag& parsed)
{
    static const boost::regex tag_re(
        "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)-ayastorm-r([0-9]+)(?:(?:\\.([0-9]+))|(?:\\.(x)))?(?:-bugfix-([0-9]+))?(?:\\+([A-Za-z0-9._-]+))?$",
        boost::regex::perl | boost::regex::icase);

    boost::smatch match;
    if (!boost::regex_match(tag, match, tag_re))
    {
        return false;
    }

    parsed.major = boost::lexical_cast<S32>(match[1].str());
    parsed.minor = boost::lexical_cast<S32>(match[2].str());
    parsed.patch = boost::lexical_cast<S32>(match[3].str());
    parsed.ayastorm_r = boost::lexical_cast<S32>(match[4].str());
    parsed.r_patch = match[5].matched ? boost::lexical_cast<S32>(match[5].str()) : 0;
    if (match[6].matched)
    {
        // Existing r10.x tags represent the patched r10 line. Sort them after plain r10.
        parsed.r_patch = 1000;
    }
    parsed.bugfix = match[7].matched ? boost::lexical_cast<S32>(match[7].str()) : 0;
    return true;
}

bool isNewerClassicTag(const ParsedClassicTag& remote, const ParsedClassicTag& local)
{
    if (remote.major != local.major) return remote.major > local.major;
    if (remote.minor != local.minor) return remote.minor > local.minor;
    if (remote.patch != local.patch) return remote.patch > local.patch;
    if (remote.ayastorm_r != local.ayastorm_r) return remote.ayastorm_r > local.ayastorm_r;
    if (remote.r_patch != local.r_patch) return remote.r_patch > local.r_patch;
    return remote.bugfix > local.bugfix;
}

std::string getConfiguredLocalTag()
{
    const std::string override_tag = gSavedSettings.getString("AYAUpdateNotifyLocalTagOverride");
    if (!override_tag.empty())
    {
        return override_tag;
    }

    return LLVersionInfo::instance().getAYAstormReleaseTag();
}

std::string getConfiguredLocalFamily()
{
    std::string family = LLVersionInfo::instance().getAYAstormReleaseFamily();
    LLStringUtil::toLower(family);
    return family.empty() ? "classic" : family;
}

bool shouldCheckNow()
{
    if (!gSavedSettings.getBOOL("AYAUpdateNotifyEnabled"))
    {
        return false;
    }

    const S32 interval_hours = llmax(0, gSavedSettings.getS32("AYAUpdateNotifyCheckIntervalHours"));
    if (interval_hours == 0)
    {
        return true;
    }

    const S32 last_checked = gSavedSettings.getS32("AYAUpdateNotifyLastChecked");
    const S32 now = static_cast<S32>(std::time(nullptr));
    return last_checked <= 0 || now - last_checked >= interval_hours * 60 * 60;
}

bool isFamilyAccepted(const std::string& family, const std::string& tag)
{
    if (family != "classic")
    {
        return false;
    }

    std::string lowered_tag = tag;
    LLStringUtil::toLower(lowered_tag);

    static const boost::regex foreign_family_re("(^|[-+.])(vk|vulkan|metal|mtl)([-+.]|$)");
    if (boost::regex_search(lowered_tag, foreign_family_re))
    {
        return false;
    }

    ParsedClassicTag ignored;
    return parseClassicTag(tag, ignored);
}

LLAyastormUpdateChecker::CheckResult makeResult(LLAyastormUpdateChecker::CheckStatus status,
                                                const std::string& message,
                                                const LLAyastormUpdateChecker::UpdateInfo& update = LLAyastormUpdateChecker::UpdateInfo())
{
    LLAyastormUpdateChecker::CheckResult result;
    result.status = status;
    result.message = message;
    result.update = update;
    return result;
}
}

LLAyastormUpdateChecker::LLAyastormUpdateChecker()
{
}

LLAyastormUpdateChecker::~LLAyastormUpdateChecker()
{
}

boost::signals2::connection LLAyastormUpdateChecker::addUpdateCallback(const update_signal_t::slot_type& cb)
{
    return mUpdateSignal.connect(cb);
}

boost::signals2::connection LLAyastormUpdateChecker::addCheckCallback(const check_signal_t::slot_type& cb)
{
    return mCheckSignal.connect(cb);
}

void LLAyastormUpdateChecker::start()
{
    if (mStarted || mSessionSuppressed)
    {
        return;
    }

    mStarted = true;
    launchCheck(false);
}

void LLAyastormUpdateChecker::checkNow()
{
    launchCheck(true);
}

void LLAyastormUpdateChecker::launchCheck(bool manual)
{
    if (mChecking)
    {
        if (manual)
        {
            publishCheckResult(makeResult(CheckStatus::IN_PROGRESS, "AYAstorm update check is already running."));
        }
        return;
    }

    mChecking = true;
    LLCoros::instance().launch("LLAyastormUpdateChecker::checkCoro",
        boost::bind(&LLAyastormUpdateChecker::checkCoro, this, manual));
}

bool LLAyastormUpdateChecker::hasUpdate() const
{
    return mUpdateInfo.available && !mSessionSuppressed;
}

const LLAyastormUpdateChecker::UpdateInfo& LLAyastormUpdateChecker::getUpdateInfo() const
{
    return mUpdateInfo;
}

void LLAyastormUpdateChecker::openReleasePage()
{
    if (mUpdateInfo.release_url.empty())
    {
        return;
    }

    gSavedSettings.setString("AYAUpdateNotifyLastNotifiedTag", mUpdateInfo.latest_tag);
    LLWeb::loadURLExternal(mUpdateInfo.release_url);
    suppressForSession();
}

void LLAyastormUpdateChecker::suppressForSession()
{
    mSessionSuppressed = true;
}

void LLAyastormUpdateChecker::skipCurrentVersion()
{
    if (!mUpdateInfo.latest_tag.empty())
    {
        gSavedSettings.setString("AYAUpdateNotifySkippedTag", mUpdateInfo.latest_tag);
    }
    suppressForSession();
}

void LLAyastormUpdateChecker::publish(const UpdateInfo& info)
{
    if (!info.available || mSessionSuppressed)
    {
        return;
    }

    mUpdateInfo = info;
    mUpdateSignal(mUpdateInfo);
}

void LLAyastormUpdateChecker::publishCheckResult(const CheckResult& result)
{
    mCheckSignal(result);
}

void LLAyastormUpdateChecker::checkCoro(bool manual)
{
    const std::string local_tag = getConfiguredLocalTag();
    const std::string local_family = getConfiguredLocalFamily();
    UpdateInfo result_update;
    result_update.current_tag = local_tag;

    ParsedClassicTag local_parsed;
    if (local_tag.empty() || local_tag == "dev" || !parseClassicTag(local_tag, local_parsed))
    {
        if (!manual && !gSavedSettings.getBOOL("AYAUpdateNotifyForceDevBuild"))
        {
            LL_INFOS(LOG_TAG) << "Skipping update check for local tag '" << local_tag << "'" << LL_ENDL;
            mChecking = false;
            return;
        }
    }

    if (!manual && !shouldCheckNow())
    {
        LL_DEBUGS(LOG_TAG) << "Skipping update check; interval has not elapsed" << LL_ENDL;
        mChecking = false;
        return;
    }

    const std::string endpoint = gSavedSettings.getString("AYAUpdateNotifyEndpoint");
    if (endpoint.empty())
    {
        LL_WARNS(LOG_TAG) << "Update endpoint is empty" << LL_ENDL;
        if (manual)
        {
            publishCheckResult(makeResult(CheckStatus::FAILED, "AYAstorm update check failed: endpoint is empty.", result_update));
        }
        mChecking = false;
        return;
    }

    LL_INFOS(LOG_TAG) << "Checking GitHub releases: endpoint=" << endpoint
                      << " embedded_tag=" << LLVersionInfo::instance().getAYAstormReleaseTag()
                      << " effective_local_tag=" << local_tag
                      << " family=" << local_family << LL_ENDL;

    gSavedSettings.setS32("AYAUpdateNotifyLastChecked", static_cast<S32>(std::time(nullptr)));

    LLCore::HttpRequest::policy_t http_policy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t http_adapter =
        std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("AYAUpdateNotify", http_policy);
    LLCore::HttpRequest::ptr_t http_request = std::make_shared<LLCore::HttpRequest>();
    LLCore::HttpOptions::ptr_t http_options = std::make_shared<LLCore::HttpOptions>();
    LLCore::HttpHeaders::ptr_t http_headers = std::make_shared<LLCore::HttpHeaders>();

    http_options->setFollowRedirects(true);
    http_options->setTimeout(10);
    http_options->setTransferTimeout(15);

    http_headers->append(HTTP_OUT_HEADER_ACCEPT, "application/vnd.github+json");
    http_headers->append(HTTP_OUT_HEADER_USER_AGENT, "AYAstorm-Update-Checker");

    LLSD result = http_adapter->getRawAndSuspend(http_request, endpoint, http_options, http_headers);
    LLSD http_results = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(http_results);
    if (!status)
    {
        LL_WARNS(LOG_TAG) << "GitHub releases request failed: " << status.toString() << LL_ENDL;
        if (manual)
        {
            publishCheckResult(makeResult(CheckStatus::FAILED,
                "AYAstorm update check failed: " + status.toString(), result_update));
        }
        mChecking = false;
        return;
    }

    const LLSD::Binary& raw_body = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_RAW].asBinary();
    const std::string body(raw_body.begin(), raw_body.end());

    boost::system::error_code ec;
    boost::json::value root = boost::json::parse(body, ec);
    if (ec || !root.is_array())
    {
        LL_WARNS(LOG_TAG) << "Failed to parse GitHub releases JSON: " << ec.message() << LL_ENDL;
        if (manual)
        {
            publishCheckResult(makeResult(CheckStatus::FAILED,
                "AYAstorm update check failed: invalid release response.", result_update));
        }
        mChecking = false;
        return;
    }

    const bool include_prerelease = gSavedSettings.getBOOL("AYAUpdateNotifyIncludePrerelease");
    const std::string skipped_tag = manual ? std::string() : gSavedSettings.getString("AYAUpdateNotifySkippedTag");

    bool found = false;
    ParsedClassicTag best_parsed;
    UpdateInfo best;
    best.current_tag = local_tag;

    for (const boost::json::value& value : root.as_array())
    {
        if (!value.is_object())
        {
            continue;
        }

        const boost::json::object& release = value.as_object();
        if (jsonBool(release, "draft") || (!include_prerelease && jsonBool(release, "prerelease")))
        {
            continue;
        }

        const std::string remote_tag = jsonString(release, "tag_name");
        if (remote_tag.empty() || remote_tag == skipped_tag || !isFamilyAccepted(local_family, remote_tag))
        {
            continue;
        }

        ParsedClassicTag remote_parsed;
        if (!parseClassicTag(remote_tag, remote_parsed))
        {
            continue;
        }

        if (!found || isNewerClassicTag(remote_parsed, best_parsed))
        {
            found = true;
            best_parsed = remote_parsed;
            best.latest_tag = remote_tag;
            best.release_name = jsonString(release, "name");
            best.release_url = jsonString(release, "html_url");
        }
    }

    if (!found)
    {
        LL_INFOS(LOG_TAG) << "No matching AYAstorm release found for family " << local_family << LL_ENDL;
        if (manual)
        {
            publishCheckResult(makeResult(CheckStatus::FAILED,
                "No matching AYAstorm release was found for this viewer family.", result_update));
        }
        mChecking = false;
        return;
    }

    LL_INFOS(LOG_TAG) << "Latest matching AYAstorm release: " << best.latest_tag << LL_ENDL;

    if (!parseClassicTag(local_tag, local_parsed))
    {
        if (!manual && !gSavedSettings.getBOOL("AYAUpdateNotifyForceDevBuild"))
        {
            mChecking = false;
            return;
        }
        local_parsed = ParsedClassicTag();
    }

    if (!isNewerClassicTag(best_parsed, local_parsed))
    {
        LL_INFOS(LOG_TAG) << "AYAstorm is up to date: local=" << local_tag
                          << " remote=" << best.latest_tag << LL_ENDL;
        if (manual)
        {
            publishCheckResult(makeResult(CheckStatus::UP_TO_DATE, "AYAstorm is up to date.", best));
        }
        mChecking = false;
        return;
    }

    best.available = true;
    LL_INFOS(LOG_TAG) << "AYAstorm update available: local=" << local_tag
                      << " remote=" << best.latest_tag << LL_ENDL;
    mUpdateInfo = best;
    if (manual)
    {
        publishCheckResult(makeResult(CheckStatus::UPDATE_AVAILABLE, "AYAstorm update available.", best));
    }
    else
    {
        publish(best);
    }
    mChecking = false;
}
