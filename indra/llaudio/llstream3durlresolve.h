/**
 * @file llstream3durlresolve.h
 * @brief AYAstorm r11 P10 / r13 C — viewer-side URL pre-resolve for Stream3D sources.
 *
 * Resolves HTTPS→HTTP cross-protocol redirects (typical of Cloudflare/CDN
 * fronted Shoutcast/Icecast streams) before handing the URL to FMOD's
 * netstream, which does not follow such redirects on its own. See
 * `doc/spec_binaural_venue_reverb.md` §4.7.
 *
 * r13 C: API is fully asynchronous. submit() enqueues a resolve request
 * onto a dedicated worker thread and returns immediately. The caller
 * polls the result on its own update tick. This eliminates the multi-
 * second main-thread block that the synchronous r11 P10 path produced
 * when many tagged streams started at once on login (the OS unresponsive
 * dialog trigger). r13 A+B (drain rate-limit / shorter timeouts) remain
 * in place as a safety net but no longer carry the worst case.
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

#ifndef LL_STREAM3DURLRESOLVE_H
#define LL_STREAM3DURLRESOLVE_H

#include <cstdint>
#include <string>

namespace LLStream3DUrlResolve
{
    using RequestId = std::uint64_t;
    constexpr RequestId kInvalidRequestId = 0;

    // r13 C: named ResolveStatus rather than Status to avoid colliding
    // with Xlib's `#define Status int` which leaks into every TU that
    // pulls in the GLX headers via llglheaders.h. Pre-existing trap on
    // Linux Firestorm builds; an unscoped `Status` here triggered an
    // "expected identifier before 'int'" macro-substitution error.
    enum class ResolveStatus
    {
        Pending, // worker has not produced a result yet
        Done,    // resolve succeeded; out_url holds the post-redirect URL
        Failed,  // resolve gave up (timeout / no redirect / bad scheme);
                 // out_url holds the original URL so the caller can use
                 // it unconditionally as the FMOD createStream URL.
        Unknown, // request id is not (or no longer) tracked — caller
                 // should treat as terminal and fall back to its own URL
    };

    // Submit `url` for background resolution. Returns a positive request
    // id; caller polls() on subsequent ticks until Done/Failed. Returns
    // kInvalidRequestId only if the worker thread could not be created
    // (e.g. resource exhaustion at startup); the caller should fall back
    // to the synchronous open path with the raw URL.
    //
    // The worker is lazily started on the first submit and lives until
    // shutdown(). Submitting an empty string returns kInvalidRequestId.
    //
    // Protocol allowlist is HTTP + HTTPS only — file:// / ftp:// / etc.
    // are blocked at the curl layer. Authentication / cookies are never
    // sent.
    RequestId submit(const std::string& url);

    // Non-blocking poll. Returns Pending while the worker is still
    // resolving; on Done/Failed/Unknown, fills out_url and consumes the
    // tracked entry (subsequent poll() calls with the same id return
    // Unknown). Safe to call from any thread but expected from the main
    // thread driving LLPositionalStreamMulti::update().
    ResolveStatus poll(RequestId id, std::string& out_url);

    // Mark a pending request as cancelled. The worker may still finish
    // the in-flight curl call (curl_easy is not interruptible from
    // outside), but the result is dropped before being written to the
    // tracked entry. Safe to call multiple times; safe with stale ids.
    void cancel(RequestId id);

    // Drain the queue and join the worker thread. Called once at viewer
    // shutdown so the curl handle is released cleanly. Idempotent —
    // subsequent submit() calls are no-ops returning kInvalidRequestId.
    void shutdown();
}

#endif // LL_STREAM3DURLRESOLVE_H
