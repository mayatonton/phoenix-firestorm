/***********************************
 * exolineartoneF.glsl
 * Provides linear tone mapping functionality.
 * Copyright Geenz Spad, 2012
 ***********************************/
#extension GL_ARB_texture_rectangle : enable

/*[EXTRA_CODE_HERE]*/

#ifdef LL_VULKAN_GLSL
layout(location = 0) out vec4 frag_color;

layout(set = 1, binding = 1) uniform sampler2D diffuseRect;
layout(set = 1, binding = 0, std140) uniform PostVignette_PerProgramBind
{
#ifndef _AYA_UM_screen_res
#define _AYA_UM_screen_res 1
    vec2 screen_res;
#else
    vec2 _dup_PostVignette_screen_res;
#endif
#ifndef _AYA_UM__pad0
#define _AYA_UM__pad0 1
    vec2 _pad0;
#else
    vec2 _dup_PostVignette__pad0;
#endif
    vec3 vignette;
#ifndef _AYA_UM__pad1
#define _AYA_UM__pad1 1
    float _pad1;
#else
    float _dup_PostVignette__pad1;
#endif
};

layout(location = 0) in vec2 vary_fragcoord;
#else
out vec4 frag_color;

uniform sampler2D diffuseRect;
uniform vec2 screen_res;
uniform vec3 vignette;
in vec2 vary_fragcoord;
#endif


void main ()
{
    vec4 diff = texture(diffuseRect, vary_fragcoord.xy);
    vec2 tc = vary_fragcoord - 0.5f;
    float vignette_val = 1 - dot(tc, tc);
    diff.rgb *= clamp(pow(mix(1, vignette_val * vignette_val * vignette_val * vignette_val * vignette.z, vignette.x), vignette.y), 0, 1);
    frag_color = diff;
    // frag_color = vec4(0.0, 1.0, 0.0, 0.5);
}
