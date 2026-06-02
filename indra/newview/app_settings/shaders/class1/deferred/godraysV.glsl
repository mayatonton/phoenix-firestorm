/**
 * @file class1/deferred/godraysV.glsl
 *
 * AYAstorm r15: godrays (screen-space light shaft) vertex shader.
 * Fullscreen quad pass-through with screen UV passed to fragment.
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
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-22 Phase 2: bare vertex
// attribute `in vec3 position` に SPIR-V location 付与
// (multiPointLightV.glsl L26-30 と同範式統一)。Vulkan strict mode では
// 全 user input/output に location 必須。VBO TYPE_VERTEX=0 で attribute
// index 0 固定。影響: Godrays Shader 1 件 parse error 解消。
layout(location=0) in vec3 position;
#else
in vec3 position;
#endif

out vec2 vary_fragcoord;

void main()
{
    vec4 pos = vec4(position.xyz, 1.0);
    gl_Position = pos;
    vary_fragcoord = pos.xy * 0.5 + 0.5;
}
