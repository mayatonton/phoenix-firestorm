/**
 * @file class1/deferred/skinSSSV.glsl
 *
 * AYAstorm r20 P0a prototype: avatar skin SSS (subsurface scattering)
 * vertex shader. Fullscreen quad pass-through with screen UV passed
 * to fragment.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * $/LicenseInfo$
 */

#ifdef LL_VULKAN_GLSL
layout(location=0) in vec3 position;
#else
in vec3 position;
#endif

#ifdef LL_VULKAN_GLSL
layout(location=1) out vec2 vary_fragcoord;
#else
out vec2 vary_fragcoord;
#endif

void main()
{
    vec4 pos = vec4(position.xyz, 1.0);
    gl_Position = pos;
    vary_fragcoord = pos.xy * 0.5 + 0.5;
}
