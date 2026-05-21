/**
 * @file bumpF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2007, Linden Research, Inc.
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

/*[EXTRA_CODE_HERE]*/

out vec4 frag_data[4];

uniform float minimum_alpha;
uniform sampler2D diffuseMap;
uniform sampler2D bumpMap;

in vec3 vary_mat0;
in vec3 vary_mat1;
in vec3 vary_mat2;

in vec4 vertex_color;
in vec2 vary_texcoord0;
in vec3 vary_position;

void mirrorClip(vec3 pos);
vec4 encodeNormal(vec3 n, float env, float gbuffer_flag);

// <AYAstorm canary: attachment magenta override>
uniform int aya_attachment_canary;
// </AYAstorm>

void main()
{
    mirrorClip(vary_position);

    // <AYAstorm canary: 1 = BoM MeshBody/MeshHead → magenta、2 = その他装着物 → blue>
    if (aya_attachment_canary != 0)
    {
        vec3 canary_rgb = (aya_attachment_canary == 1)
            ? vec3(1.0, 0.0, 1.0)                          // BoM body/head = magenta
            : ((aya_attachment_canary == 3)
                ? vec3(0.5, 0.5, 0.5)                      // プリム装着物 = gray
                : ((aya_attachment_canary == 4)
                    ? vec3(0.214, 0.051, 0.0)              // alpha BLEND 装着物 = brown (sRGB 0.5,0.25,0)
                    : ((aya_attachment_canary == 11)
                        ? vec3(0.0, 0.0, 0.0)              // 11 = Rez Object opaque 黒
                        : ((aya_attachment_canary == 12)
                            ? vec3(0.0, 1.0, 0.0)          // 12 = Rez Object alpha BLEND 緑
                            : vec3(0.0, 0.0, 1.0)))));     // mesh 装着物 = blue
        frag_data[0] = vec4(canary_rgb, 0.0);
        frag_data[1] = vec4(0.0);
        // bumpF stores tangent-space basis in vary_mat0/1/2; the world-space
        // surface normal is the z-row.
        vec3 fake_n = normalize(vec3(vary_mat0.z, vary_mat1.z, vary_mat2.z));
        // SKIP_ATMOS で softenLightF を bypass、canary 色を素通り
        frag_data[2] = encodeNormal(fake_n, 0.0, GBUFFER_FLAG_SKIP_ATMOS);
#if defined(HAS_EMISSIVE)
        frag_data[3] = vec4(canary_rgb, 0.0);
#endif
        return;
    }
    // </AYAstorm>

    vec4 col = texture(diffuseMap, vary_texcoord0.xy);

    if(col.a < minimum_alpha)
    {
        discard;
    }
    col *= vertex_color;

    vec3 norm = texture(bumpMap, vary_texcoord0.xy).rgb * 2.0 - 1.0;

    vec3 tnorm = vec3(dot(norm,vary_mat0),
            dot(norm,vary_mat1),
            dot(norm,vary_mat2));

    frag_data[0] = vec4(col.rgb, 0.0);
    frag_data[1] = vertex_color.aaaa; // spec
    //frag_data[1] = vec4(vec3(vertex_color.a), vertex_color.a+(1.0-vertex_color.a)*vertex_color.a); // spec - from former class3 - maybe better, but not so well tested
    vec3 nvn = normalize(tnorm);
    frag_data[2] = encodeNormal(nvn, vertex_color.a, GBUFFER_FLAG_HAS_ATMOS);

#if defined(HAS_EMISSIVE)
    frag_data[3] = vec4(0, 0, 0, 0);
#endif
}
