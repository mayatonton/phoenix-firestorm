/**
 * @file llstream3durlresolve.h
 * @brief AYAstorm r11 P10 — viewer-side URL pre-resolve for Stream3D sources.
 *
 * Resolves HTTPS→HTTP cross-protocol redirects (typical of Cloudflare/CDN
 * fronted Shoutcast/Icecast streams) before handing the URL to FMOD's
 * netstream, which does not follow such redirects on its own. See
 * `doc/spec_binaural_venue_reverb.md` §4.7.
 *
 * The resolve is a single libcurl easy session — HEAD + FOLLOWLOCATION
 * (max 3 hops, 3 s timeout). HEAD-rejecting servers fall through to a
 * 1-byte ranged GET. Failure (timeout / network error / no redirect)
 * is non-fatal: callers fall back to the original URL transparently.
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

#include <string>

namespace LLStream3DUrlResolve
{
    // Resolve `in` through up to 3 HTTP/HTTPS redirects and write the
    // effective URL into `out`. Returns true if the resolution succeeded
    // AND the effective URL differs from `in`. Returns false on any
    // failure mode (curl error, timeout, scheme rejected, no redirect);
    // in the false case `out` is set to `in` so the caller can use it
    // unconditionally as the FMOD createStream URL.
    //
    // Protocol allowlist is HTTP + HTTPS only — file:// / ftp:// / etc.
    // are blocked to prevent a hostile redirect from landing FMOD on a
    // local resource. Authentication / cookies are never sent.
    //
    // Intended thread: the Stream3D decode thread (called once per
    // stream open, before FMOD::System::createStream). Blocks for up
    // to 3 seconds on the worst-case timeout — acceptable because the
    // decode thread is the right place to absorb open-time latency.
    bool resolveStreamUrl(const std::string& in, std::string& out);
}

#endif // LL_STREAM3DURLRESOLVE_H
