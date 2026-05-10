/**
 * @file llstream3durlresolve.cpp
 * @brief AYAstorm r11 P10 / r13 C — viewer-side URL pre-resolve implementation.
 *
 * r13 C: the public API is async. A single dedicated worker thread owns
 * the curl easy session and processes one request at a time off a FIFO
 * queue. The main thread submits and polls; nothing here ever blocks the
 * caller for longer than a mutex hold.
 *
 * Lifetime:
 *   - The worker is lazily constructed on the first submit() that
 *     succeeds in starting the thread (Meyers-style function-local
 *     static).
 *   - shutdown() flips a flag, drains the queue, and joins. After that
 *     submit() returns kInvalidRequestId so late callers fall back to
 *     the raw URL rather than racing with a destroyed worker.
 *
 * Cancellation:
 *   - curl_easy_perform is not interruptible from outside. cancel()
 *     therefore marks the result entry with a cancelled flag; the
 *     worker, on completing the in-flight request, sees the flag and
 *     drops the result instead of writing it. The caller's next poll()
 *     then returns Unknown.
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

#include "linden_common.h"

#include "llstream3durlresolve.h"

#include "llerror.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <curl/curl.h>

namespace
{
    // Discards any body bytes the server might send (we requested HEAD,
    // but a permissive server may still emit a body — silently drop it
    // rather than letting curl write to stdout).
    size_t discardWriteCallback(char* /*ptr*/, size_t size, size_t nmemb, void* /*userdata*/)
    {
        return size * nmemb;
    }

    // Apply the common easy-session options shared by HEAD and ranged-GET
    // attempts: redirect handling, scheme allowlist, timeout, no-body
    // sink. URL is set by the caller because each attempt may probe a
    // different opt mix.
    void applyCommonOptions(CURL* curl, const std::string& url)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3L);
        // r13 B: tightened from 3000/2000 ms. With r13 C the worker is
        // off the main thread so the wall budget no longer drives the
        // unresponsive-window threshold, but a tight cap still bounds
        // how long a stuck CDN can keep the queue from draining when
        // many tagged streams resolve back-to-back.
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 1500L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 1000L);
        // Block redirects to file:// / ftp:// / etc. — only HTTP/HTTPS
        // are valid Stream3D URLs (spec §4.7.6).
#ifdef CURLPROTO_HTTP
        curl_easy_setopt(curl, CURLOPT_PROTOCOLS, (long)(CURLPROTO_HTTP | CURLPROTO_HTTPS));
        curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, (long)(CURLPROTO_HTTP | CURLPROTO_HTTPS));
#endif
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discardWriteCallback);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    }

    CURLcode performAndCaptureEffective(CURL* curl, std::string& out_effective)
    {
        const CURLcode rc = curl_easy_perform(curl);
        if (rc == CURLE_OK)
        {
            char* eff = nullptr;
            if (curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &eff) == CURLE_OK
                && eff != nullptr)
            {
                out_effective.assign(eff);
            }
        }
        return rc;
    }

    // Synchronous resolve helper, run on the worker thread. Reuses the
    // same HEAD-then-ranged-GET strategy as the original r11 P10 path.
    // On any failure the caller is expected to fall back to `in`; we
    // still write `in` into out_url so the LLPositionalStreamMulti code
    // path can use a single getter regardless of outcome.
    bool resolveSyncOnWorker(CURL* curl, const std::string& in, std::string& out_url)
    {
        out_url = in;

        std::string effective = in;
        applyCommonOptions(curl, in);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        CURLcode rc = performAndCaptureEffective(curl, effective);

        if (rc != CURLE_OK || effective == in)
        {
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            const bool head_unsupported = (rc == CURLE_HTTP_RETURNED_ERROR)
                || (http_code == 405)
                || (http_code == 400)
                || (http_code == 501);

            if (rc == CURLE_OK && effective == in)
            {
                // HEAD succeeded with no redirect — URL is canonical.
            }
            else if (rc != CURLE_OK || head_unsupported)
            {
                curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
                curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
                curl_easy_setopt(curl, CURLOPT_RANGE, "0-0");
                effective = in;
                rc = performAndCaptureEffective(curl, effective);
                curl_easy_setopt(curl, CURLOPT_RANGE, (char*)nullptr);
            }
        }

        // Reset for the next reuse so stale opts (NOBODY, HTTPGET) do
        // not leak across requests.
        curl_easy_reset(curl);

        if (rc != CURLE_OK)
        {
            LL_DEBUGS("Stream3DUrlResolve") << "resolve failed for '" << in
                                             << "' (curl=" << rc
                                             << " " << curl_easy_strerror(rc)
                                             << "); falling back to raw URL"
                                             << LL_ENDL;
            out_url = in;
            return false;
        }

        if (effective.empty() || effective == in)
        {
            out_url = in;
            return false;
        }

        out_url = effective;
        return true;
    }

    struct Request
    {
        LLStream3DUrlResolve::RequestId id;
        std::string url;
    };

    struct Result
    {
        LLStream3DUrlResolve::ResolveStatus status = LLStream3DUrlResolve::ResolveStatus::Pending;
        std::string url;
        bool cancelled = false;
    };

    class Worker
    {
    public:
        Worker()
        : mNextId(1)
        , mShutdown(false)
        , mStarted(false)
        {
        }

        ~Worker()
        {
            shutdown();
        }

        // Returns kInvalidRequestId if the worker thread cannot be
        // started or shutdown has already been called.
        LLStream3DUrlResolve::RequestId submit(const std::string& url)
        {
            if (url.empty())
            {
                return LLStream3DUrlResolve::kInvalidRequestId;
            }

            {
                std::lock_guard<std::mutex> lock(mMutex);
                if (mShutdown.load(std::memory_order_acquire))
                {
                    return LLStream3DUrlResolve::kInvalidRequestId;
                }
                if (!mStarted)
                {
                    try
                    {
                        mThread = std::thread(&Worker::loop, this);
                        mStarted = true;
                    }
                    catch (const std::system_error& e)
                    {
                        LL_WARNS("Stream3DUrlResolve")
                            << "failed to start resolve worker thread: "
                            << e.what() << LL_ENDL;
                        return LLStream3DUrlResolve::kInvalidRequestId;
                    }
                }

                const LLStream3DUrlResolve::RequestId id = mNextId++;
                if (mNextId == LLStream3DUrlResolve::kInvalidRequestId)
                {
                    // Wrap-around guard. We never realistically issue
                    // 2^64 requests, but stay correct if we do.
                    mNextId = 1;
                }
                mQueue.push_back({ id, url });
                Result r;
                r.status = LLStream3DUrlResolve::ResolveStatus::Pending;
                r.url = url;
                mResults.emplace(id, std::move(r));
                mCv.notify_one();
                return id;
            }
        }

        LLStream3DUrlResolve::ResolveStatus poll(LLStream3DUrlResolve::RequestId id,
                                          std::string& out_url)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            auto it = mResults.find(id);
            if (it == mResults.end())
            {
                out_url.clear();
                return LLStream3DUrlResolve::ResolveStatus::Unknown;
            }
            const LLStream3DUrlResolve::ResolveStatus s = it->second.status;
            if (s == LLStream3DUrlResolve::ResolveStatus::Pending)
            {
                // Don't expose the in-progress URL yet.
                out_url.clear();
                return s;
            }
            out_url = std::move(it->second.url);
            mResults.erase(it);
            return s;
        }

        void cancel(LLStream3DUrlResolve::RequestId id)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            auto it = mResults.find(id);
            if (it == mResults.end())
            {
                return;
            }
            // If the result is already Done/Failed, just drop it so the
            // caller's later poll() returns Unknown rather than a stale
            // resolved URL they no longer want.
            if (it->second.status != LLStream3DUrlResolve::ResolveStatus::Pending)
            {
                mResults.erase(it);
                return;
            }
            it->second.cancelled = true;
        }

        void shutdown()
        {
            std::thread t;
            {
                std::lock_guard<std::mutex> lock(mMutex);
                if (mShutdown.exchange(true, std::memory_order_acq_rel))
                {
                    return;
                }
                mQueue.clear();
                mCv.notify_all();
                if (mStarted)
                {
                    t = std::move(mThread);
                    mStarted = false;
                }
            }
            if (t.joinable())
            {
                t.join();
            }
        }

    private:
        void loop()
        {
            CURL* curl = curl_easy_init();
            if (!curl)
            {
                LL_WARNS("Stream3DUrlResolve")
                    << "curl_easy_init failed in resolve worker; "
                    << "all queued requests will fail back to raw URL"
                    << LL_ENDL;
            }

            for (;;)
            {
                Request req;
                {
                    std::unique_lock<std::mutex> lock(mMutex);
                    mCv.wait(lock, [this] {
                        return mShutdown.load(std::memory_order_acquire)
                               || !mQueue.empty();
                    });
                    if (mShutdown.load(std::memory_order_acquire))
                    {
                        break;
                    }
                    req = std::move(mQueue.front());
                    mQueue.pop_front();
                    auto it = mResults.find(req.id);
                    if (it != mResults.end() && it->second.cancelled)
                    {
                        // Caller already gave up — drop the entry and
                        // skip the network round-trip entirely.
                        mResults.erase(it);
                        continue;
                    }
                }

                std::string resolved = req.url;
                if (curl)
                {
                    resolveSyncOnWorker(curl, req.url, resolved);
                }

                {
                    std::lock_guard<std::mutex> lock(mMutex);
                    auto it = mResults.find(req.id);
                    if (it == mResults.end())
                    {
                        // shutdown() cleared us between dispatch and
                        // result write; nothing to do.
                        continue;
                    }
                    if (it->second.cancelled)
                    {
                        mResults.erase(it);
                        continue;
                    }
                    // We treat both "redirect followed" and "no
                    // redirect needed" as success — out_url holds the
                    // URL the caller should actually hand to FMOD. The
                    // Failed status is reserved for cases where curl
                    // itself errored, which resolveSyncOnWorker has
                    // already collapsed into out=in. Distinguishing
                    // those further isn't useful to the caller.
                    it->second.url = std::move(resolved);
                    it->second.status = LLStream3DUrlResolve::ResolveStatus::Done;
                }
            }

            if (curl)
            {
                curl_easy_cleanup(curl);
            }
        }

        std::mutex mMutex;
        std::condition_variable mCv;
        std::deque<Request> mQueue;
        std::unordered_map<LLStream3DUrlResolve::RequestId, Result> mResults;
        std::thread mThread;
        LLStream3DUrlResolve::RequestId mNextId;
        std::atomic<bool> mShutdown;
        bool mStarted;
    };

    Worker& worker()
    {
        static Worker w;
        return w;
    }
}

namespace LLStream3DUrlResolve
{
    RequestId submit(const std::string& url)
    {
        return worker().submit(url);
    }

    ResolveStatus poll(RequestId id, std::string& out_url)
    {
        return worker().poll(id, out_url);
    }

    void cancel(RequestId id)
    {
        worker().cancel(id);
    }

    void shutdown()
    {
        worker().shutdown();
    }
}
