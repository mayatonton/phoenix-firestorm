/***********************************
 * exoPostBaseV.glsl
 * Provides screen coordinates for post processing effects.
 * This is basically a more minimal reimplementation of postDeferredV.glsl.
 * Copyright Geenz Spad, 2012
 ***********************************/

#ifdef LL_VULKAN_GLSL
layout(location = 0) in vec3 position;

layout(location = 0) out vec2 vary_fragcoord;
#else
in vec3 position;

out vec2 vary_fragcoord;

uniform vec2 screen_res;
#endif

void main()
{
    vec4 pos = vec4(position.xyz, 1.0);
    gl_Position = pos;

    vary_fragcoord.xy = (pos.xy * 0.5 + 0.5);
}
