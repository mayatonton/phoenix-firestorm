/***********************************
 * exoPostBaseV.glsl
 * Provides screen coordinates for post processing effects.
 * This is basically a more minimal reimplementation of postDeferredV.glsl.
 * Copyright Geenz Spad, 2012
 ***********************************/

in vec3 position;
in vec2 texcoord0;

out vec2 vary_fragcoord;

#ifdef LL_VULKAN_GLSL
layout(set=0, binding=0, std140) uniform FrameViewProj {
    mat4 modelview_projection_matrix;
    mat4 modelview_matrix;
    mat4 projection_matrix;
    mat4 inv_proj;
    mat4 proj_mat;
    mat4 last_modelview_matrix;
    mat3 env_mat;
    mat3 normal_matrix;
    vec2 screen_res;
};
#else
uniform vec2 screen_res;
#endif

void main()
{
    vec4 pos = vec4(position.xyz, 1.0);
    gl_Position = pos;

    vary_fragcoord.xy = (pos.xy * 0.5 + 0.5);
}
