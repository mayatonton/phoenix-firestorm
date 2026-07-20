/**
 * @file llreloadqueue.h
 * @brief Single owner for deferred runtime shader / GL-buffer reloads
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLRELOADQUEUE_H
#define LL_LLRELOADQUEUE_H

#include "stdtypes.h"

class LLReloadQueue
{
public:
    enum ReloadKind : U32
    {
        RK_Shaders   = 1u << 0,
        RK_GLBuffers = 1u << 1,
    };

    static void request(U32 kinds);
    static void drain();

private:
    static U32 sPending;
};

#endif // LL_LLRELOADQUEUE_H
