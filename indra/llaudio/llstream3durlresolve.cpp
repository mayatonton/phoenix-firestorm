/**
 * @file llstream3durlresolve.cpp
 * @brief AYAstorm r11 P10 — viewer-side URL pre-resolve implementation.
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
        // r13: tightened from 3000/2000 ms. Combined with the per-frame
        // drain rate-limit in LLPositionalStreamMgr::update(), this caps
        // the worst-case single-frame stall to ~1.5 s — short enough to
        // stay under the OS unresponsive-window threshold even at login
        // when several https:// streams resolve back-to-back. A slow CDN
        // that exceeds the budget falls through to the raw URL (FMOD
        // attempts the HTTPS connect itself, which usually still works).
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 1500L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 1000L);
        // Block redirects to file:// / ftp:// / etc. — only HTTP/HTTPS
        // are valid Stream3D URLs (spec §4.7.6). Setopt the source-side
        // protocols too so curl_easy_setopt rejects a non-HTTP `in`
        // before we waste any network round-trips.
#ifdef CURLPROTO_HTTP
        curl_easy_setopt(curl, CURLOPT_PROTOCOLS, (long)(CURLPROTO_HTTP | CURLPROTO_HTTPS));
        curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, (long)(CURLPROTO_HTTP | CURLPROTO_HTTPS));
#endif
        // Discard sink in case the server ignores our HEAD/Range hint
        // and responds with a body anyway.
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discardWriteCallback);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        // Keep error reporting silent at the curl layer; we surface
        // failure modes via the LL_DEBUGS log line below.
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    }

    // Try one resolve pass with the given easy session, writing the
    // effective URL into out_effective. Returns the curl result code.
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
}

namespace LLStream3DUrlResolve
{
    bool resolveStreamUrl(const std::string& in, std::string& out)
    {
        out = in;
        if (in.empty())
        {
            return false;
        }

        CURL* curl = curl_easy_init();
        if (!curl)
        {
            LL_DEBUGS("Stream3DUrlResolve") << "curl_easy_init failed; falling back to raw URL"
                                             << LL_ENDL;
            return false;
        }

        std::string effective = in;

        // Attempt 1: HEAD. The cheapest probe for redirect-only servers.
        applyCommonOptions(curl, in);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        CURLcode rc = performAndCaptureEffective(curl, effective);

        // Attempt 2: ranged GET (1 byte). Some Shoutcast/Icecast servers
        // refuse HEAD with 405 — fall through to a minimal GET so we can
        // still observe the redirect chain via Location headers.
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
                // HEAD succeeded but no redirect was followed. Don't waste
                // a second round trip — the URL is already canonical.
            }
            else if (rc != CURLE_OK || head_unsupported)
            {
                // Reset only the bits we change; the common options stay
                // valid because we kept the same easy session.
                curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
                curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
                curl_easy_setopt(curl, CURLOPT_RANGE, "0-0");
                effective = in;
                rc = performAndCaptureEffective(curl, effective);
                // Disarm RANGE before close so a stale string ptr doesn't
                // outlive the local cleanup path.
                curl_easy_setopt(curl, CURLOPT_RANGE, (char*)nullptr);
            }
        }

        curl_easy_cleanup(curl);

        if (rc != CURLE_OK)
        {
            LL_DEBUGS("Stream3DUrlResolve") << "resolve failed for '" << in
                                             << "' (curl=" << rc
                                             << " " << curl_easy_strerror(rc)
                                             << "); falling back to raw URL"
                                             << LL_ENDL;
            out = in;
            return false;
        }

        if (effective.empty() || effective == in)
        {
            // No redirect chain — caller uses the original URL.
            out = in;
            return false;
        }

        out = effective;
        return true;
    }
}
