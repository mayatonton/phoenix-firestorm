// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8
// FrameAtmosphere_Lighting UBO blueprint (= set=0 binding=2)
// Source: literal extract from class1/windlight/atmosphericsF.glsl:37 ifdef LL_VULKAN_GLSL block
//         (verified identical across 4 sample sites = atmosphericsF / atmosphericsHelpersV /
//          atmosphericsFuncs / atmosphericsHelpersF)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.1
//         set=0 帯 binding=2 / cadence=Frame

#version 450

layout(std140, set = 0, binding = 2) uniform FrameAtmosphere_Lighting
{
    vec3  sunlight_color;
    float scene_light_strength;
    vec3  moonlight_color;
    float haze_density;
    vec3  ambient_color;
    float density_multiplier;
    vec3  blue_horizon;
    float distance_multiplier;
    vec3  blue_density;
    float max_y;
    vec3  glow;
    float sky_sunlight_scale;
    float sky_ambient_scale;
    float sky_hdr_scale;
    int   classic_mode;
    int   cube_snapshot;
    float minimum_alpha;
    float max_cof;
    float _pad_atm0;
    float _pad_atm1;
};

void main() {}
