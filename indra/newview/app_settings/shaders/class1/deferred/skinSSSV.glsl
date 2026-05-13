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

in vec3 position;

out vec2 vary_fragcoord;

void main()
{
    vec4 pos = vec4(position.xyz, 1.0);
    gl_Position = pos;
    vary_fragcoord = pos.xy * 0.5 + 0.5;
}
