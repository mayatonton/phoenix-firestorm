/***********************************
 * exolineartoneF.glsl
 * Provides linear tone mapping functionality.
 * Copyright Geenz Spad, 2012
 ***********************************/
#extension GL_ARB_texture_rectangle : enable

/*[EXTRA_CODE_HERE]*/

out vec4 frag_color;

#ifdef LL_VULKAN_GLSL
layout(set=1, binding=4) uniform sampler2D diffuseRect;
#else
uniform sampler2D diffuseRect;
#endif
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
uniform vec3 vignette;
in vec2 vary_fragcoord;


void main ()
{
    vec4 diff = texture(diffuseRect, vary_fragcoord.xy);
    vec2 tc = vary_fragcoord - 0.5f;
    float vignette_val = 1 - dot(tc, tc);
    diff.rgb *= clamp(pow(mix(1, vignette_val * vignette_val * vignette_val * vignette_val * vignette.z, vignette.x), vignette.y), 0, 1);
    frag_color = diff;
    // frag_color = vec4(0.0, 1.0, 0.0, 0.5);
}
