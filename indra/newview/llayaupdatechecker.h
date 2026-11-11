/**
 * @file llayaupdatechecker.h
 * @brief AYAstorm GitHub release notification checker.
 */

#ifndef LL_LLAYAUPDATECHECKER_H
#define LL_LLAYAUPDATECHECKER_H

#include "llsingleton.h"

#include <boost/signals2.hpp>
#include <string>

class LLAyastormUpdateChecker : public LLSingleton<LLAyastormUpdateChecker>
{
    LLSINGLETON(LLAyastormUpdateChecker);

public:
    struct UpdateInfo
    {
        bool available = false;
        std::string current_tag;
        std::string latest_tag;
        std::string release_name;
        std::string release_url;
    };

    enum class CheckStatus
    {
        UPDATE_AVAILABLE,
        UP_TO_DATE,
        DISABLED,
        IN_PROGRESS,
        FAILED
    };

    struct CheckResult
    {
        CheckStatus status = CheckStatus::FAILED;
        std::string message;
        UpdateInfo update;
    };

    using update_signal_t = boost::signals2::signal<void(const UpdateInfo&)>;
    using check_signal_t = boost::signals2::signal<void(const CheckResult&)>;

    ~LLAyastormUpdateChecker();

    boost::signals2::connection addUpdateCallback(const update_signal_t::slot_type& cb);
    boost::signals2::connection addCheckCallback(const check_signal_t::slot_type& cb);

    void start();
    void checkNow();
    void openReleasePage();
    void suppressForSession();
    void skipCurrentVersion();

    bool hasUpdate() const;
    const UpdateInfo& getUpdateInfo() const;

private:
    void launchCheck(bool manual);
    void checkCoro(bool manual);
    void publish(const UpdateInfo& info);
    void publishCheckResult(const CheckResult& result);

    bool mStarted = false;
    bool mChecking = false;
    bool mSessionSuppressed = false;
    UpdateInfo mUpdateInfo;
    update_signal_t mUpdateSignal;
    check_signal_t mCheckSignal;
};

#endif // LL_LLAYAUPDATECHECKER_H
