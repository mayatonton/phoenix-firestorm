/**
 * @file llpipelinecapture.cpp
 * @brief Rendering pipeline: impostor/snapshot/capture (pure move from pipeline.cpp).
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llviewerprecompiledheaders.h"
#include "pipeline.h"
#include "llvkbucket.h"
#include <unordered_map>
#include <optional>
#include "llimagepng.h"
#include "llaudioengine.h" // For debugging.
#include "llocclusiongeometrymgr.h" // r13: OBB occlusion debug overlay.
#include "llerror.h"
#include "llfile.h"
#include "llviewercontrol.h"
#include "llfasttimer.h"
#include "llfontgl.h"
#include "llfontvertexbuffer.h"
#include "llnamevalue.h"
#include "llpointer.h"
#include "llprimitive.h"
#include "llvolume.h"
#include "material_codes.h"
#include "v3color.h"
#include "llui.h"
#include "llglheaders.h"
#include "llrender.h"
#include "llvkloader.h"
#include "llvkcontract.h"
#include <fstream>
#include "llvkuboreg.h"
#include "llstartup.h"
#include "llwindow.h"   // swapBuffers()
#include "llagent.h"
#include "llagentcamera.h"
#include "llappviewer.h"
#include "lltexturecache.h"
#include "lltexturefetch.h"
#include "llimageworker.h"
#include "lldrawable.h"
#include "lldrawpoolalpha.h"
#include "lldrawpoolavatar.h"
#include "lldrawpoolbump.h"
#include "lldrawpoolwlsky.h"
#include "lldrawpooltree.h"
#include "lldrawpoolwater.h"
#include "llface.h"
#include "llfeaturemanager.h"
#include "llfloatertelehub.h"
#include "llfloaterreg.h"
#include "llhudmanager.h"
#include "llhudnametag.h"
#include "llhudtext.h"
#include "lllightconstants.h"
#include "llmeshrepository.h"
#include "llvolumemgr.h"
#include "llpipelineframecontext.h"
#include "llpipelinelistener.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llsky.h"
#include "lltracker.h"
#include "lltool.h"
#include "lltoolmgr.h"
#include "llviewercamera.h"
#include "llviewermediafocus.h"
#include "llviewertexturelist.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llparcel.h" // <FS:AYA> [ParcelHide-Tag] for LLParcel::getDesc()
#include "llviewerregion.h" // for audio debugging.
#include "llviewerwindow.h" // For getSpinAxis
#include "llvoavatarself.h"
#include "llviewerjointattachment.h"
#include "llvocache.h"
#include "llvosky.h"
#include "llvowlsky.h"
#include "llvotree.h"
#include "llvovolume.h"
#include "llvosurfacepatch.h"
#include "llvowater.h"
#include "llvotree.h"
#include "llvopartgroup.h"
#include "llworld.h"
#include "llcubemap.h"
#include "llviewershadermgr.h"
#include "llreloadqueue.h"
#include "llviewerstats.h"
#include "llviewerjoystick.h"
#include "llviewerdisplay.h"
#include "llspatialpartition.h"
#include "llmutelist.h"
#include "lltoolpie.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llpathinglib.h"
#include "llfloaterpathfindingconsole.h"
#include "llfloaterpathfindingcharacters.h"
#include "llfloatertools.h"
#include "llfloatersnapshot.h" // <FS:Beq/> for snapshotFrame
#include "llfloaterflickr.h" // <FS:Beq/> for snapshotFrame
#include "fsfloaterprimfeed.h" // <FS:Beq/> for snapshotFrame
#include "llsnapshotlivepreview.h" // <FS:Beq/> for snapshotFrame
#include "llpathfindingpathtool.h"
#include "llscenemonitor.h"
#include "llprogressview.h"
#include "llcleanup.h"
#include "gltfscenemanager.h"
#include "llvisualeffect.h"
#include "rlvactions.h"
#include "rlvlocks.h"
#include "llenvironment.h"
#include "llsettingsvo.h"
// PCH 経由で Xlib (X11/Xlib.h:84) の `#define None 0L` が流入し、enum class member
// 等の `None` トークンを数値リテラル `0L` に置換してしまう。pipeline.cpp は X11
// API を直接呼ばないのでファイル冒頭で undef して局所的に無効化する。
// memory: project_linux_xlib_status_define_trap.md
#undef None

#include "llerror.h"
#include "llpipelineinternal.h"

namespace
{
    LLFrameTimer sFSSelfRiggedPickerArmTimer;
    F32 sFSSelfRiggedPickerArmSeconds = 0.f;
    U32 sFSSelfRiggedPickerArmGeneration = 0;
    U32 sFSSelfRiggedPickerRenderGeneration = 0;

    enum class FSRiggedPickerObjectIDBufferOwner
    {
        None,
        Self,
        Other
    };

    FSRiggedPickerObjectIDBufferOwner sFSRiggedPickerObjectIDBufferOwner =
        FSRiggedPickerObjectIDBufferOwner::None;

    LLFrameTimer sFSOtherRiggedPickerArmTimer;
    F32 sFSOtherRiggedPickerArmSeconds = 0.f;
    U32 sFSOtherRiggedPickerArmGeneration = 0;
    U32 sFSOtherRiggedPickerRenderGeneration = 0;
    LLPointer<LLVOAvatar> sFSOtherRiggedPickerAvatar;
    LLUUID sFSOtherRiggedPickerAvatarID;

    // All PASS_*_RIGGED types in the LL render map. The visible deferred opaque
    // pass dispatches rigged geometry through these via
    // pushRiggedBatches (see lldrawpool.cpp:410, 466). Iterating the same set
    // gives pixel-perfect agreement with what the user actually sees.
    const U32 kFSRiggedPasses[] = {
        LLRenderPass::PASS_SIMPLE_RIGGED,
        LLRenderPass::PASS_FULLBRIGHT_RIGGED,
        LLRenderPass::PASS_INVISIBLE_RIGGED,
        LLRenderPass::PASS_INVISI_SHINY_RIGGED,
        LLRenderPass::PASS_FULLBRIGHT_SHINY_RIGGED,
        LLRenderPass::PASS_SHINY_RIGGED,
        LLRenderPass::PASS_BUMP_RIGGED,
        LLRenderPass::PASS_POST_BUMP_RIGGED,
        LLRenderPass::PASS_MATERIAL_RIGGED,
        LLRenderPass::PASS_MATERIAL_ALPHA_RIGGED,
        LLRenderPass::PASS_MATERIAL_ALPHA_MASK_RIGGED,
        LLRenderPass::PASS_MATERIAL_ALPHA_EMISSIVE_RIGGED,
        LLRenderPass::PASS_SPECMAP_RIGGED,
        LLRenderPass::PASS_SPECMAP_BLEND_RIGGED,
        LLRenderPass::PASS_SPECMAP_MASK_RIGGED,
        LLRenderPass::PASS_SPECMAP_EMISSIVE_RIGGED,
        LLRenderPass::PASS_NORMMAP_RIGGED,
        LLRenderPass::PASS_NORMMAP_BLEND_RIGGED,
        LLRenderPass::PASS_NORMMAP_MASK_RIGGED,
        LLRenderPass::PASS_NORMMAP_EMISSIVE_RIGGED,
        LLRenderPass::PASS_NORMSPEC_RIGGED,
        LLRenderPass::PASS_NORMSPEC_BLEND_RIGGED,
        LLRenderPass::PASS_NORMSPEC_MASK_RIGGED,
        LLRenderPass::PASS_NORMSPEC_EMISSIVE_RIGGED,
        LLRenderPass::PASS_GLOW_RIGGED,
        LLRenderPass::PASS_GLTF_GLOW_RIGGED,
        LLRenderPass::PASS_ALPHA_RIGGED,
        LLRenderPass::PASS_ALPHA_MASK_RIGGED,
        LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK_RIGGED,
        LLRenderPass::PASS_ALPHA_INVISIBLE_RIGGED,
        LLRenderPass::PASS_GLTF_PBR_RIGGED,
        LLRenderPass::PASS_GLTF_PBR_ALPHA_MASK_RIGGED,
    };
}

// <FS:Beq> Rework Snapshot Guide Rendering
void LLPipeline::renderSnapshotGuidesOverlay()
{
    if (!mSnapshotGuideState.active || !mSnapshotGuideState.show_guides)
    {
        mSnapshotGuideState.active = false;
        return;
    }

    if (!gViewerWindow || !gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
    {
        mSnapshotGuideState.active = false;
        return;
    }

    LLRect view_rect = gViewerWindow->getWorldViewRectRaw();
    const F32 width = (F32)view_rect.getWidth();
    const F32 height = (F32)view_rect.getHeight();
    if (width <= 0.f || height <= 0.f)
    {
        mSnapshotGuideState.active = false;
        return;
    }

    const F32 left_norm = llmin(mSnapshotGuideState.left, mSnapshotGuideState.right);
    const F32 right_norm = llmax(mSnapshotGuideState.left, mSnapshotGuideState.right);
    const F32 bottom_norm = llmin(mSnapshotGuideState.bottom, mSnapshotGuideState.top);
    const F32 top_norm = llmax(mSnapshotGuideState.bottom, mSnapshotGuideState.top);

    const F32 left_px = left_norm * width;
    const F32 right_px = right_norm * width;
    const F32 bottom_px = bottom_norm * height;
    const F32 top_px = top_norm * height;

    const F32 frame_width = right_px - left_px;
    const F32 frame_height = top_px - bottom_px;
    if (frame_width <= 0.f || frame_height <= 0.f)
    {
        mSnapshotGuideState.active = false;
        return;
    }

    const F32 alpha = llclamp(mSnapshotGuideState.visibility, 0.f, 1.f);
    if (alpha <= 0.f)
    {
        mSnapshotGuideState.active = false;
        return;
    }

    LLGLDisable depth(GL_DEPTH_TEST);
    LLGLDisable cull(GL_CULL_FACE);
    LLGLDisable stencil(GL_STENCIL_TEST);
    LLGLEnable blend(GL_BLEND);
    gGL.setSceneBlendType(LLRender::BT_ALPHA);

    LLGLSLShader* ui_shader = &gUIProgram;
    ui_shader->bind();

    if (!LLViewerFetchedTexture::sWhiteImagep.isNull())
    {
        gGL.getTexUnit(0)->bind(LLViewerFetchedTexture::sWhiteImagep);
    }

    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.pushMatrix();
    gGL.loadIdentity();
    gGL.ortho(0.f, width, 0.f, height, -1.f, 1.f);

    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.pushMatrix();
    gGL.loadIdentity();
    gGLLastMatrix = nullptr;

    const LLColor4 line_color(mSnapshotGuideState.color, alpha);
    gGL.color4fv(line_color.mV);

    const F32 thickness = llmax(mSnapshotGuideState.thickness, 0.f);
    const F32 half_thickness = thickness * 0.5f;
    auto draw_filled_rect = [&](F32 l, F32 b, F32 r, F32 t)
    {
        const S32 left_i = ll_round(l);
        const S32 right_i = ll_round(r);
        const S32 top_i = ll_round(t);
        const S32 bottom_i = ll_round(b);
        gl_rect_2d(left_i, top_i, right_i, bottom_i, line_color, true);
    };

    auto draw_vertical_norm = [&](F32 norm)
    {
        const F32 x = left_px + frame_width * norm;
        draw_filled_rect(x - half_thickness, bottom_px, x + half_thickness, top_px);
    };

    auto draw_horizontal_norm = [&](F32 norm)
    {
        const F32 y = bottom_px + frame_height * norm;
        draw_filled_rect(left_px, y - half_thickness, right_px, y + half_thickness);
    };

    switch (mSnapshotGuideState.style)
    {
        case SnapshotGuideState::Style::RuleOfThirds:
        {
            constexpr std::array<F32, 2> offsets = { 1.f / 3.f, 2.f / 3.f };
            for (F32 offset : offsets)
            {
                draw_vertical_norm(offset);
                draw_horizontal_norm(offset);
            }
            break;
        }
        case SnapshotGuideState::Style::GoldenRatio:
        {
            constexpr F32                               phi         = 1.61803398875f;
            const SnapshotGuideState::GoldenOrientation orientation = mSnapshotGuideState.golden_orientation;

            const F32 scale = llmin(frame_width / phi, frame_height);
            if (scale <= 0.f)
            {
                break;
            }

            const F32 golden_width  = phi * scale;
            const F32 golden_height = scale;
            const F32 pad_x         = frame_width - golden_width;
            const F32 pad_y         = frame_height - golden_height;

            F32 anchor_x = left_px;
            F32 anchor_y = bottom_px;
            switch (orientation)
            {
                case SnapshotGuideState::GoldenOrientation::TopLeft:
                    anchor_y += pad_y;
                    break;
                case SnapshotGuideState::GoldenOrientation::TopRight:
                    anchor_x += pad_x;
                    anchor_y += pad_y;
                    break;
                case SnapshotGuideState::GoldenOrientation::BottomRight:
                    anchor_x += pad_x;
                    break;
                case SnapshotGuideState::GoldenOrientation::BottomLeft:
                default:
                    break;
            }

            auto map_point = [&](F32 local_x, F32 local_y) -> LLVector2
            {
                F32 x = local_x;
                F32 y = local_y;

                if (orientation == SnapshotGuideState::GoldenOrientation::TopLeft ||
                    orientation == SnapshotGuideState::GoldenOrientation::BottomLeft)
                {
                    x = golden_width - local_x;
                }

                if (orientation == SnapshotGuideState::GoldenOrientation::BottomLeft ||
                    orientation == SnapshotGuideState::GoldenOrientation::BottomRight)
                {
                    y = golden_height - local_y;
                }

                return LLVector2(anchor_x + x, anchor_y + y);
            };

            std::vector<std::pair<LLVector2, LLVector2>> line_segments;
            line_segments.reserve(24);

            auto add_line = [&](F32 x0, F32 y0, F32 x1, F32 y1)
            {
                line_segments.emplace_back(map_point(x0, y0), map_point(x1, y1));
            };

            // Outline of the fitted golden rectangle.
            add_line(0.f, 0.f, golden_width, 0.f);
            add_line(0.f, golden_height, golden_width, golden_height);
            add_line(0.f, 0.f, 0.f, golden_height);
            add_line(golden_width, 0.f, golden_width, golden_height);

            // Generate subdivision lines while we walk the squares.
            F32 x0 = 0.f;
            F32 y0 = 0.f;
            F32 x1 = golden_width;
            F32 y1 = golden_height;

            for (U32 step = 0; step < 12; ++step)
            {
                const F32 width  = x1 - x0;
                const F32 height = y1 - y0;
                if (width <= 1.f || height <= 1.f)
                {
                    break;
                }

                switch (step % 4)
                {
                    case 0:
                        x0 += height;
                        add_line(x0, y0, x0, y1);
                        break;
                    case 1:
                        y0 += width;
                        add_line(x0, y0, x1, y0);
                        break;
                    case 2:
                        x1 -= height;
                        add_line(x1, y0, x1, y1);
                        break;
                    default:
                        y1 -= width;
                        add_line(x0, y1, x1, y1);
                        break;
                }
            }

            auto draw_golden_spiral = [&](U32 max_depth)
            {
                gGL.begin(LLRender::LINE_STRIP);

                F32 spiral_x0 = 0.f;
                F32 spiral_y0 = 0.f;
                F32 spiral_x1 = golden_width;
                F32 spiral_y1 = golden_height;

                for (U32 step = 0; step < max_depth; ++step)
                {
                    const F32 width  = spiral_x1 - spiral_x0;
                    const F32 height = spiral_y1 - spiral_y0;
                    if (width <= 1.f || height <= 1.f)
                    {
                        break;
                    }

                    F32 size        = 0.f;
                    F32 cx          = 0.f;
                    F32 cy          = 0.f;
                    F32 start_angle = 0.f;
                    F32 end_angle   = 0.f;

                    switch (step % 4)
                    {
                        case 0: // left square
                            size        = height;
                            cx          = spiral_x0 + size;
                            cy          = spiral_y0 + size;
                            start_angle = F_PI;
                            end_angle   = 1.5f * F_PI;
                            spiral_x0 += size;
                            break;
                        case 1: // bottom square
                            size        = width;
                            cx          = spiral_x0;
                            cy          = spiral_y0 + size;
                            start_angle = 1.5f * F_PI;
                            end_angle   = 2.f * F_PI;
                            spiral_y0 += size;
                            break;
                        case 2: // right square
                            size        = height;
                            cx          = spiral_x1 - size;
                            cy          = spiral_y0;
                            start_angle = 0.f;
                            end_angle   = F_PI_BY_TWO;
                            spiral_x1 -= size;
                            break;
                        case 3: // top square
                        default:
                            size        = width;
                            cx          = spiral_x0 + size;
                            cy          = spiral_y1 - size;
                            start_angle = F_PI_BY_TWO;
                            end_angle   = F_PI;
                            spiral_y1 -= size;
                            break;
                    }

                    if (size <= 0.f)
                    {
                        break;
                    }

                    const S32 segments = llclamp((S32)(size / 4.f), 12, 64);
                    for (S32 i = 0; i <= segments; ++i)
                    {
                        const F32 t       = start_angle + (end_angle - start_angle) * (F32)i / (F32)segments;
                        const F32 local_x = cx + cosf(t) * size;
                        const F32 local_y = cy + sinf(t) * size;
                        LLVector2 mapped  = map_point(local_x, local_y);
                        gGL.vertex2f(mapped.mV[0], mapped.mV[1]);
                    }
                }

                gGL.end();
            };

            gGL.flush();
            const F32 line_width = llmax(thickness, 1.f);
            gGL.setLineWidth(line_width);
            draw_golden_spiral(12);
            gGL.setLineWidth(1.f);

            if (!line_segments.empty())
            {
                gGL.flush();
                gGL.setLineWidth(line_width);
                gGL.begin(LLRender::LINES);
                for (const auto& segment : line_segments)
                {
                    gGL.vertex2f(segment.first.mV[VX], segment.first.mV[VY]);
                    gGL.vertex2f(segment.second.mV[VX], segment.second.mV[VY]);
                }
                gGL.end();
                gGL.setLineWidth(1.f);
            }
            break;
        }
        case SnapshotGuideState::Style::Diagonal:
        {
            const F32 line_width = llmax(thickness, 1.f);
            gGL.flush();
            gGL.setLineWidth(line_width);
            gGL.begin(LLRender::LINES);
            gGL.vertex2f(left_px, bottom_px);
            gGL.vertex2f(right_px, top_px);
            gGL.vertex2f(left_px, top_px);
            gGL.vertex2f(right_px, bottom_px);
            gGL.end();
            gGL.setLineWidth(1.f);
            break;
        }
    }

    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.popMatrix();
    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.popMatrix();
    gGLLastMatrix = nullptr;

    ui_shader->unbind();

    mSnapshotGuideState.active = false;
}

// </FS:Beq>

// <FS:Beq> Render Snapshot frame oerlay
bool LLPipeline::renderSnapshotFrame(LLRenderTarget* src, LLRenderTarget* dst)
{
    static LLCachedControl<bool> show_frame(gSavedSettings, "FSSnapshotShowCaptureFrame", false);
    static LLCachedControl<bool> show_guides(gSavedSettings, "FSSnapshotShowGuides", false);

    mSnapshotGuideState.active = false;
    mSnapshotGuideState.show_guides = false;

    float left   = 0.f;
    float top    = 0.f;
    float right  = 1.f;
    float bottom = 1.f;        

    // TODO - add debug settings to control the appearance of the snapshot frameand guides
    static LLCachedControl<LLColor3> border_color(gSavedSettings, "FSSnapshotFrameBorderColor", LLColor3(1.f, 0.f, 0.f));    
    static LLCachedControl<LLColor3> guide_color(gSavedSettings, "FSSnapshotFrameGuideColor", LLColor3(1.f, 1.f, 0.f));    
    static LLCachedControl<F32> border_thickness(gSavedSettings, "FSSnapshotFrameBorderWidth", 2.0f);    
    static LLCachedControl<F32> guide_thickness(gSavedSettings, "FSSnapshotFrameGuideWidth", 2.0f);    
    static LLCachedControl<F32> guide_visibility(gSavedSettings, "FSSnapshotGuideVisibility", 0.5f);
    static LLCachedControl<std::string> guide_style_setting(gSavedSettings, "FSSnapshotGuideStyle", std::string("rule_of_thirds"));

    SnapshotGuideState::Style guide_style = SnapshotGuideState::Style::RuleOfThirds;
    SnapshotGuideState::GoldenOrientation golden_orientation = SnapshotGuideState::GoldenOrientation::TopLeft;
    const std::string style_value = guide_style_setting();
    if (style_value == "golden_ratio" || style_value == "golden_ratio_top_left")
    {
        guide_style = SnapshotGuideState::Style::GoldenRatio;
        golden_orientation = SnapshotGuideState::GoldenOrientation::TopLeft;
    }
    else if (style_value == "golden_ratio_top_right")
    {
        guide_style = SnapshotGuideState::Style::GoldenRatio;
        golden_orientation = SnapshotGuideState::GoldenOrientation::TopRight;
    }
    else if (style_value == "golden_ratio_bottom_left")
    {
        guide_style = SnapshotGuideState::Style::GoldenRatio;
        golden_orientation = SnapshotGuideState::GoldenOrientation::BottomLeft;
    }
    else if (style_value == "golden_ratio_bottom_right")
    {
        guide_style = SnapshotGuideState::Style::GoldenRatio;
        golden_orientation = SnapshotGuideState::GoldenOrientation::BottomRight;
    }
    else if (style_value == "diagonal")
    {
        guide_style = SnapshotGuideState::Style::Diagonal;
    }
    else
    {
        guide_style = SnapshotGuideState::Style::RuleOfThirds;
    }
    const F32 guide_visibility_value = show_guides ? (F32)guide_visibility : 0.f;
    const bool simple_snapshot_visible = LLFloaterReg::instanceVisible("simple_snapshot");
    const bool flickr_snapshot_visible = LLFloaterReg::instanceVisible("flickr");
    const bool primfeed_snapshot_visible = LLFloaterReg::instanceVisible("primfeed"); // <FS:Beq/> Primfeed integration
    const bool snapshot_visible = LLFloaterReg::instanceVisible("snapshot");
    const bool any_snapshot_visible = simple_snapshot_visible || flickr_snapshot_visible || primfeed_snapshot_visible || snapshot_visible; // <FS:Beq/> Primfeed integration
    if (!show_frame || !any_snapshot_visible || !gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
    {
        return false;
    }
    LLSnapshotLivePreview * previewView = nullptr;
    if (snapshot_visible)
    {
        auto * floater =dynamic_cast<LLFloaterSnapshotBase*>(LLFloaterReg::findInstance("snapshot"));
        previewView = floater->impl->getPreviewView();
    }
    // Note: simple_snapshot not supported as there can be more than one active and more complex selection is required
    if (flickr_snapshot_visible && !previewView)
    {
        auto * floater = dynamic_cast<LLFloaterFlickr*>(LLFloaterReg::findInstance("flickr"));
        previewView = floater->getPreviewView();
    }
     // <FS:Beq> Primfeed integration
    if (primfeed_snapshot_visible && !previewView)
    {
        auto * floater = dynamic_cast<FSFloaterPrimfeed*>(LLFloaterReg::findInstance("primfeed"));
        previewView = floater->getPreviewView();
    }
    // </FS:Beq>
    if(!previewView)
    {
        return false;
    }
    
    static LLCachedControl<bool> keep_aspect(gSavedSettings, "KeepAspectForSnapshot", false);
    
    S32 snapshot_width;
    S32 snapshot_height;
    previewView->getSize(snapshot_width, snapshot_height);
    F32 screen_aspect = float(gViewerWindow->getWindowWidthRaw()) / float(gViewerWindow->getWindowHeightRaw());
    F32 snapshot_aspect = float(snapshot_width) / float(snapshot_height);

    if (keep_aspect || (std::fabs(screen_aspect - snapshot_aspect) < 1e-6f) )
    {
        top    = 0.0f;
        left   = 0.0f;
        bottom = 1.0f;
        right  = 1.0f;
    }

    float w = screen_aspect;
    float h = 1.0;
    if (snapshot_aspect > screen_aspect)
    {
        float frame_width = w;
        float frame_height = frame_width / snapshot_aspect;
        // Centre this box in [0..1]x[0..1]
        float y_offset = 0.5f * (h - frame_height);
        left   = 0.f;
        top    = y_offset / h;
        right  = 1.f;
        bottom = (y_offset + frame_height) / h;        
    }
    else
    {
        float frame_height = h;
        float frame_width = h * snapshot_aspect;
        // Centre this box in [0..1]x[0..1]
        float x_offset = 0.5f * (w - frame_width);
        left   = x_offset / w;
        top    = 0.f;
        right  = (x_offset + frame_width) / w;
        bottom = 1.f;        

    }
    LL_PROFILE_GPU_ZONE("Snapshot Frame");
    {
    LLRTScope rts(*dst, false, "snapshot_frame");
    if (rts)
    {
    LLGLSLShader *shader = &gPostSnapshotFrameProgram;

    // bind the program and output to screentriangle VBO
    shader->bind();

    S32 channel = shader->enableTexture(LLShaderMgr::DEFERRED_DIFFUSE, src->getUsage());
    if (channel > -1)
    {
        src->bindTexture(0, channel, LLTexUnit::TFO_POINT);
    }
    else
    {
        LL_ERRS("snapshot_frame") << "Failed to bind diffuse texture" << LL_ENDL;
    }

    // Assuming frame_rect is a static or accessible variable containing the frame dimensions

    if (LLVKLoader::isVulkanInitialized() && shader->mVkPerProgramUBO != VK_NULL_HANDLE
        && shader->mVkPerProgramUBOMapped != nullptr)
    {
        LLVKLoader::PostSnapshotFrame_PerProgramBind ubo_data = {};
        ubo_data.screen_res[0]          = (F32)dst->getWidth();
        ubo_data.screen_res[1]          = (F32)dst->getHeight();
        ubo_data.frame_rect[0]          = (F32)left;
        ubo_data.frame_rect[1]          = (F32)top;
        ubo_data.frame_rect[2]          = (F32)right;
        ubo_data.frame_rect[3]          = (F32)bottom;
        ubo_data.border_color[0]        = border_color().mV[0];
        ubo_data.border_color[1]        = border_color().mV[1];
        ubo_data.border_color[2]        = border_color().mV[2];
        ubo_data.border_thickness       = (F32)border_thickness;
        memcpy(shader->vkPerProgramBaseWritePtr(), &ubo_data, sizeof(ubo_data));
    }

    // Guides are rendered in a later UI pass; no additional uniforms required here.

    mScreenTriangleVB->setBuffer();
    mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

    shader->disableTexture(LLShaderMgr::DEFERRED_DIFFUSE, src->getUsage());
    shader->unbind();
    }
    }

    if (show_frame && show_guides && gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
    {
        mSnapshotGuideState.active = true;
        mSnapshotGuideState.show_guides = true;
        mSnapshotGuideState.left = left;
        mSnapshotGuideState.right = right;
        mSnapshotGuideState.bottom = bottom;
        mSnapshotGuideState.top = top;
        mSnapshotGuideState.color = guide_color();
        mSnapshotGuideState.thickness = guide_thickness();
        mSnapshotGuideState.visibility = llclamp(guide_visibility_value, 0.f, 1.f);
        mSnapshotGuideState.style = guide_style;
        mSnapshotGuideState.golden_orientation = golden_orientation;
    }

    return true;
}

bool LLPipeline::renderRiggedObjectIDBufferForAvatar(LLVOAvatar* target_avatar,
                                                     U32 max_draw_calls,
                                                     U32 max_triangles)
{
    if (!target_avatar || target_avatar->isDead()) return false;
    if (!mObjectIDBuffer.isComplete()) return false;

    gGL.flush();

    GLboolean previous_color_mask[4] = {
        (GLboolean)(gGL.getColorMaskR() ? GL_TRUE : GL_FALSE),
        (GLboolean)(gGL.getColorMaskG() ? GL_TRUE : GL_FALSE),
        (GLboolean)(gGL.getColorMaskB() ? GL_TRUE : GL_FALSE),
        (GLboolean)(gGL.getColorMaskA() ? GL_TRUE : GL_FALSE) };
    GLfloat previous_clear_color[4] = { 0.f, 0.f, 0.f, 0.f };
    GLint previous_cull_face_mode = LLGLState::sCullFaceMode;
    const F32* cur_cc = gGL.getClearColor();
    previous_clear_color[0] = cur_cc[0];
    previous_clear_color[1] = cur_cc[1];
    previous_clear_color[2] = cur_cc[2];
    previous_clear_color[3] = cur_cc[3];

    bool over_budget = false;
    bool admitted = false;
    {
    LLRTScope rts(mObjectIDBuffer, false, "rigged_objid");
    if (rts)
    {
    admitted = true;
    // gbuffer3 has no alpha in default LL config (project memory
    // reference_gbuffer3_storage); make sure all four channels are writable so
    // the top 8 bits of each packed ID survive the write. Go through gGL so
    // LLRender's cached mask stays in sync with the actual GL state.
    gGL.setColorMask(false, false, false, false);
    gGL.setColorMask(true, true, true, true);
    gGL.setClearColor(0.f, 0.f, 0.f, 0.f);
    LLRenderTarget::clearBoundTarget(GL_COLOR_BUFFER_BIT);

    // Depth shared with deferredScreen — test only, no write.
    LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_LEQUAL);
    LLGLDisable   blend(GL_BLEND);
    // Cull pinned to BACK to match deferred opaque (back-facing collar
    // interiors must not write IDs at chin pixels).
    LLGLEnable    cull (GL_CULL_FACE);
    LLGLState::setCullFaceMode(GL_BACK);

    gFSObjectIDShader.bind();

    // uploadMatrixPalette caches the last (avatar, mesh) pair to skip redundant
    // GPU uploads for back-to-back DrawInfos with the same skin.
    const LLVOAvatar* lastAvatar = nullptr;
    U64  lastMeshId   = 0;
    bool skipLastSkin = false;
    U32 draw_calls = 0;
    U32 triangles = 0;

    for (U32 pass_type : kFSRiggedPasses)
    {
        LLCullResult::drawinfo_iterator begin = beginRenderMap(pass_type);
        LLCullResult::drawinfo_iterator end   = endRenderMap(pass_type);
        for (LLCullResult::drawinfo_iterator i = begin; i != end; )
        {
            LLDrawInfo* info = *i;
            LLCullResult::increment_iterator(i, end);
            if (!info || !info->mVertexBuffer || info->mCount == 0) continue;
            if (info->mAvatar.get() != target_avatar) continue;
            const LLMeshSkinInfo* skin = info->mSkinInfo.get();
            if (!skin || skin->mHash == 0) continue;
            U32 id = info->mFSPickerLocalID;
            if (id == 0)
            {
                // DrawInfo wasn't stamped with a LocalID at construction
                // time (non-prim source, or stale batch from a removed
                // attachment). Skip — its pixels stay 0 in the buffer and
                // resolve as "no rigged attachment here".
                continue;
            }

            const U32 next_draw_calls = draw_calls + 1;
            const U32 next_triangles = triangles + (info->mCount / 3);
            if ((max_draw_calls > 0 && next_draw_calls > max_draw_calls) ||
                (max_triangles > 0 && next_triangles > max_triangles))
            {
                over_budget = true;
                break;
            }

            F32 r = ((id >>  0) & 0xff) / 255.f;
            F32 g = ((id >>  8) & 0xff) / 255.f;
            F32 b = ((id >> 16) & 0xff) / 255.f;
            F32 a = ((id >> 24) & 0xff) / 255.f;
            if (LLVKLoader::isVulkanInitialized()
                && gFSObjectIDShader.mVkPerProgramUBOMapped != nullptr
                && gFSObjectIDShader.mVkPerProgramUBOSize >= sizeof(LLVKLoader::FsObjectIDF_PerProgramBind))
            {
                LLVKLoader::FsObjectIDF_PerProgramBind ubo_data{};
                ubo_data.object_id_packed[0] = r;
                ubo_data.object_id_packed[1] = g;
                ubo_data.object_id_packed[2] = b;
                ubo_data.object_id_packed[3] = a;
                gFSObjectIDShader.rotatePerProgramUBOSlot();
                std::memcpy(gFSObjectIDShader.vkPerProgramActiveWritePtr(), &ubo_data, sizeof(ubo_data));
            }

            if (!LLRenderPass::uploadMatrixPalette(target_avatar, skin,
                                                   lastAvatar, lastMeshId, skipLastSkin))
            {
                continue;
            }

            const U32 draw_id = LLRenderPass::establishPerDrawId(info, &gFSObjectIDShader);
            info->mVertexBuffer->setBuffer();
            info->mVertexBuffer->drawRange(LLRender::TRIANGLES,
                                           info->mStart, info->mEnd,
                                           info->mCount, info->mOffset, draw_id);
            draw_calls = next_draw_calls;
            triangles = next_triangles;
        }

        if (over_budget)
        {
            break;
        }
    }

    gFSObjectIDShader.unbind();

    if (over_budget)
    {
        gGL.setClearColor(0.f, 0.f, 0.f, 0.f);
        LLRenderTarget::clearBoundTarget(GL_COLOR_BUFFER_BIT);
    }

    }
    }

    gGL.setColorMask(previous_color_mask[0] == GL_TRUE,
                     previous_color_mask[1] == GL_TRUE,
                     previous_color_mask[2] == GL_TRUE,
                     previous_color_mask[3] == GL_TRUE);
    gGL.setClearColor(previous_clear_color[0], previous_clear_color[1], previous_clear_color[2], previous_clear_color[3]);
    LLGLState::setCullFaceMode(previous_cull_face_mode);

    return admitted && !over_budget;
}

void LLPipeline::armSelfRiggedObjectIDBuffer(F32 seconds)
{
    if (seconds <= 0.f)
    {
        return;
    }

    const bool was_armed = isSelfRiggedObjectIDBufferArmed();
    sFSSelfRiggedPickerArmSeconds = seconds;
    sFSSelfRiggedPickerArmTimer.reset();

    if (!was_armed)
    {
        ++sFSSelfRiggedPickerArmGeneration;
    }
}

bool LLPipeline::isSelfRiggedObjectIDBufferArmed() const
{
    return sFSSelfRiggedPickerArmSeconds > 0.f &&
           sFSSelfRiggedPickerArmTimer.getElapsedTimeF32() <= sFSSelfRiggedPickerArmSeconds;
}

bool LLPipeline::isSelfRiggedObjectIDBufferReady() const
{
    return isSelfRiggedObjectIDBufferArmed() &&
           sFSRiggedPickerObjectIDBufferOwner == FSRiggedPickerObjectIDBufferOwner::Self &&
           sFSSelfRiggedPickerRenderGeneration == sFSSelfRiggedPickerArmGeneration;
}

void LLPipeline::armOtherRiggedObjectIDBuffer(LLVOAvatar* avatar, F32 seconds)
{
    if (seconds <= 0.f || !avatar || avatar->isDead())
    {
        return;
    }
    if (isAgentAvatarValid() && avatar == gAgentAvatarp.get())
    {
        return;
    }
    if (avatar->isImpostor())
    {
        return;
    }

    const bool was_armed = isOtherRiggedObjectIDBufferArmed();
    const bool target_changed = (sFSOtherRiggedPickerAvatarID != avatar->getID());
    sFSOtherRiggedPickerAvatar = avatar;
    sFSOtherRiggedPickerAvatarID = avatar->getID();
    sFSOtherRiggedPickerArmSeconds = seconds;
    sFSOtherRiggedPickerArmTimer.reset();

    if (!was_armed || target_changed)
    {
        ++sFSOtherRiggedPickerArmGeneration;
    }
}

bool LLPipeline::isOtherRiggedObjectIDBufferArmed() const
{
    if (sFSOtherRiggedPickerArmSeconds <= 0.f ||
        sFSOtherRiggedPickerArmTimer.getElapsedTimeF32() > sFSOtherRiggedPickerArmSeconds)
    {
        return false;
    }
    return sFSOtherRiggedPickerAvatar.notNull() &&
           !sFSOtherRiggedPickerAvatar->isDead() &&
           sFSOtherRiggedPickerAvatarID.notNull();
}

bool LLPipeline::isOtherRiggedObjectIDBufferReady(const LLUUID& avatar_id) const
{
    return avatar_id.notNull() &&
           isOtherRiggedObjectIDBufferArmed() &&
           sFSOtherRiggedPickerAvatarID == avatar_id &&
           sFSRiggedPickerObjectIDBufferOwner == FSRiggedPickerObjectIDBufferOwner::Other &&
           sFSOtherRiggedPickerRenderGeneration == sFSOtherRiggedPickerArmGeneration;
}

void LLPipeline::clearOtherRiggedObjectIDBuffer()
{
    if (!isOtherRiggedObjectIDBufferArmed() &&
        sFSOtherRiggedPickerArmSeconds <= 0.f &&
        sFSOtherRiggedPickerAvatar.isNull() &&
        sFSOtherRiggedPickerAvatarID.isNull())
    {
        return;
    }

    sFSOtherRiggedPickerArmSeconds = 0.f;
    sFSOtherRiggedPickerAvatar = nullptr;
    sFSOtherRiggedPickerAvatarID.setNull();
    ++sFSOtherRiggedPickerArmGeneration;
    if (sFSRiggedPickerObjectIDBufferOwner == FSRiggedPickerObjectIDBufferOwner::Other)
    {
        sFSRiggedPickerObjectIDBufferOwner = FSRiggedPickerObjectIDBufferOwner::None;
    }
}

void LLPipeline::renderSelfRiggedObjectIDBuffer()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    LL_PROFILE_GPU_ZONE("renderSelfRiggedObjectIDBuffer");

    static LLCachedControl<bool> gpu_enable(gSavedSettings, "FSSelfRiggedPickerGPU", false);
    if (!gpu_enable) return;
    if (!isAgentAvatarValid()) return;

    if (renderRiggedObjectIDBufferForAvatar(gAgentAvatarp.get(), 0, 0))
    {
        sFSRiggedPickerObjectIDBufferOwner = FSRiggedPickerObjectIDBufferOwner::Self;
        sFSSelfRiggedPickerRenderGeneration = sFSSelfRiggedPickerArmGeneration;
    }
    else
    {
        sFSRiggedPickerObjectIDBufferOwner = FSRiggedPickerObjectIDBufferOwner::None;
        sFSSelfRiggedPickerRenderGeneration = 0;
    }
}

void LLPipeline::renderOtherRiggedObjectIDBuffer()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    LL_PROFILE_GPU_ZONE("renderOtherRiggedObjectIDBuffer");

    static LLCachedControl<bool> enable(gSavedSettings, "FSOtherRiggedPickerEnable", false);
    static LLCachedControl<bool> gpu_enable(gSavedSettings, "FSOtherRiggedPickerGPU", true);
    // AYA P0 fixup: MaxDrawCalls / MaxTriangles were cvars; hardcoded now.
    static constexpr U32 kMaxDrawCalls = 512;
    static constexpr U32 kMaxTriangles = 1200000;
    if (!enable || !gpu_enable) return;
    if (gAgentCamera.getCameraMode() == CAMERA_MODE_MOUSELOOK ||
        gAgentCamera.cameraCustomizeAvatar())
    {
        clearOtherRiggedObjectIDBuffer();
        return;
    }
    if (!isOtherRiggedObjectIDBufferArmed()) return;
    LLVOAvatar* target_avatar = sFSOtherRiggedPickerAvatar.get();
    if (!target_avatar || target_avatar->isDead() || target_avatar->isImpostor())
    {
        return;
    }

    if (renderRiggedObjectIDBufferForAvatar(target_avatar, kMaxDrawCalls, kMaxTriangles))
    {
        sFSRiggedPickerObjectIDBufferOwner = FSRiggedPickerObjectIDBufferOwner::Other;
        sFSOtherRiggedPickerRenderGeneration = sFSOtherRiggedPickerArmGeneration;
    }
    else
    {
        sFSRiggedPickerObjectIDBufferOwner = FSRiggedPickerObjectIDBufferOwner::None;
        sFSOtherRiggedPickerRenderGeneration = 0;
    }
}

bool LLPipeline::snapshotSceneDepthToWaterDis()
{
    LLRTDetour det(gPipeline.mWaterDis, false, "atm_depthcopy");
    if (det)
    {
        // copy depth buffer for use in haze shader (use water displacement map as temp storage)
        LLGLDepthTest depth(GL_TRUE, GL_TRUE, GL_ALWAYS);

        LLRenderTarget& src = getFrameRT()->screen;
        LLRenderTarget& depth_src = getFrameRT()->deferredScreen;

        gCopyDepthProgram.bind();

        S32 diff_map = gCopyDepthProgram.getTextureChannel(LLShaderMgr::DIFFUSE_MAP);
        S32 depth_map = gCopyDepthProgram.getTextureChannel(LLShaderMgr::DEFERRED_DEPTH);

        gGL.getTexUnit(diff_map)->bind(&src);
        gGL.getTexUnit(depth_map)->bind(&depth_src, true);

        if (LLVKLoader::isVulkanInitialized())
        {
            src.bindForShaderRead();
            depth_src.bindForShaderRead(0, true);
        }

        gGL.setColorMask(false, false);
        gPipeline.mScreenTriangleVB->setBuffer();
        gPipeline.mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

    }
    det.resume();
    return (bool)det;
}

void LLPipeline::doWaterExclusionMask()
{
    {
        LLRTScope rts(mWaterExclusionMask, false, "water_exclusion_mask");
        if (rts)
        {
            gGL.setClearColor(1, 1, 1, 1);
            mWaterExclusionMask.clear();
            mWaterExclusionPool->render(buildRecordPassContext());
        }
    }
    gGL.setClearColor(0, 0, 0, 0);
}

void LLPipeline::generateImpostor(LLVOAvatar* avatar, bool preview_avatar, bool for_profile, LLViewerObject* specific_attachment)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    LL_PROFILE_GPU_ZONE("generateImpostor");

    static LLCullResult result;
    result.clear();
    grabReferences(result);

    if (!avatar || avatar->isDead() || !avatar->mDrawable)
    {
        LL_WARNS_ONCE("AvatarRenderPipeline") << "Avatar is " << (avatar ? "not drawable" : "null") << LL_ENDL;
        return;
    }
    LL_DEBUGS_ONCE("AvatarRenderPipeline") << "Avatar " << avatar->getID() << " is drawable" << LL_ENDL;

    assertInitialized();

    // previews can't be muted or impostered
    bool visually_muted = !for_profile && !preview_avatar && avatar->isVisuallyMuted();
    LL_DEBUGS_ONCE("AvatarRenderPipeline") << "Avatar " << avatar->getID()
                              << " is " << ( visually_muted ? "" : "not ") << "visually muted"
                              << LL_ENDL;
    bool too_complex = !for_profile && !preview_avatar && avatar->isTooComplex();
    LL_DEBUGS_ONCE("AvatarRenderPipeline") << "Avatar " << avatar->getID()
                              << " is " << ( too_complex ? "" : "not ") << "too complex"
                              << LL_ENDL;

    pushRenderTypeMask();

    if (visually_muted || too_complex)
    {
        // only show jelly doll geometry
        andRenderTypeMask(LLPipeline::RENDER_TYPE_AVATAR,
                            LLPipeline::RENDER_TYPE_CONTROL_AV,
                            END_RENDER_TYPES);
    }
    else
    {
        //hide world geometry
        clearRenderTypeMask(
            RENDER_TYPE_SKY,
            RENDER_TYPE_WL_SKY,
            RENDER_TYPE_TERRAIN,
            RENDER_TYPE_GRASS,
            RENDER_TYPE_CONTROL_AV, // Animesh
            RENDER_TYPE_TREE,
            RENDER_TYPE_VOIDWATER,
            RENDER_TYPE_WATER,
            RENDER_TYPE_ALPHA_PRE_WATER,
            RENDER_TYPE_PASS_GRASS,
            RENDER_TYPE_HUD,
            RENDER_TYPE_PARTICLES,
            RENDER_TYPE_CLOUDS,
            RENDER_TYPE_HUD_PARTICLES,
            END_RENDER_TYPES
         );
    }

    if (specific_attachment && specific_attachment->isHUDAttachment())
    { //enable HUD rendering
        setRenderTypeMask(RENDER_TYPE_HUD, END_RENDER_TYPES);
    }

    S32 occlusion = sUseOcclusion;
    sUseOcclusion = 0;

    LLPipelineFrameContext::getInstance().setReflectionPass(!isFrameRenderingDeferred());

    LLPipelineFrameContext::getInstance().setShadowPass(true);
    LLPipelineFrameContext::getInstance().setImpostorPass(true);

    LLViewerCamera* viewer_camera = LLViewerCamera::getInstance();

    {
        markVisible(avatar->mDrawable, *viewer_camera);

        if (preview_avatar)
        {
            // Only show rigged attachments for preview
            // For the sake of performance and so that static
            // objects won't obstruct previewing changes
            LLVOAvatar::attachment_map_t::iterator iter;
            for (iter = avatar->mAttachmentPoints.begin();
                iter != avatar->mAttachmentPoints.end();
                ++iter)
            {
                LLViewerJointAttachment *attachment = iter->second;
                for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
                    attachment_iter != attachment->mAttachedObjects.end();
                    ++attachment_iter)
                {
                    LLViewerObject* attached_object = attachment_iter->get();
                    // <FS:Ansariel> FIRE-31966: Some mesh bodies/objects don't show in shape editor previews -> show everything but animesh
                    //if (attached_object)
                    //{
                    //    if (attached_object->isRiggedMesh())
                    //    {
                    //        markVisible(attached_object->mDrawable->getSpatialBridge(), *viewer_camera);
                    //    }
                    //    else
                    //    {
                    //        // sometimes object is a linkset and rigged mesh is a child
                    //        LLViewerObject::const_child_list_t& child_list = attached_object->getChildren();
                    //        for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
                    //            iter != child_list.end(); iter++)
                    //        {
                    //            LLViewerObject* child = *iter;
                    //            if (child->isRiggedMesh())
                    //            {
                    //                markVisible(attached_object->mDrawable->getSpatialBridge(), *viewer_camera);
                    //                break;
                    //            }
                    //        }
                    //    }
                    //}
                    if (attached_object && !attached_object->getControlAvatar())
                    {
                        markVisible(attached_object->mDrawable->getSpatialBridge(), *viewer_camera);
                    }
                    // </FS:Ansariel>
                }
            }
        }
        else
        {
            if (specific_attachment)
            {
                markVisible(specific_attachment->mDrawable->getSpatialBridge(), *viewer_camera);
            }
            else
            {
                LLVOAvatar::attachment_map_t::iterator iter;
                LLVOAvatar::attachment_map_t::iterator begin = avatar->mAttachmentPoints.begin();
                LLVOAvatar::attachment_map_t::iterator end = avatar->mAttachmentPoints.end();

                for (iter = begin;
                    iter != end;
                    ++iter)
                {
                    LLViewerJointAttachment* attachment = iter->second;
                    for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
                        attachment_iter != attachment->mAttachedObjects.end();
                        ++attachment_iter)
                    {
                        LLViewerObject* attached_object = attachment_iter->get();
                        if (attached_object)
                        {
                            markVisible(attached_object->mDrawable->getSpatialBridge(), *viewer_camera);
                        }
                    }
                }
            }
        }
    }

    stateSort(*LLViewerCamera::getInstance(), result);

    LLCamera camera = *viewer_camera;
    LLVector2 tdim;
    U32 resY = 0;
    U32 resX = 0;
    std::optional<LLRTScope> rts;

    if (!preview_avatar)
    {
        const LLVector4a* ext = avatar->mDrawable->getSpatialExtents();
        LLVector3 pos(avatar->getRenderPosition()+avatar->getImpostorOffset());

        camera.lookAt(viewer_camera->getOrigin(), pos, viewer_camera->getUpAxis());

        LLVector4a half_height;
        half_height.setSub(ext[1], ext[0]);
        half_height.mul(0.5f);

        LLVector4a left;
        left.load3(camera.getLeftAxis().mV);
        left.mul(left);
        llassert(left.dot3(left).getF32() > F_APPROXIMATELY_ZERO);
        left.normalize3fast();

        LLVector4a up;
        up.load3(camera.getUpAxis().mV);
        up.mul(up);
        llassert(up.dot3(up).getF32() > F_APPROXIMATELY_ZERO);
        up.normalize3fast();

        tdim.mV[0] = fabsf(half_height.dot3(left).getF32());
        tdim.mV[1] = fabsf(half_height.dot3(up).getF32());

        gGL.matrixMode(LLRender::MM_PROJECTION);
        gGL.pushMatrix();

        F32 distance = (pos-camera.getOrigin()).length();
        F32 fov = atanf(tdim.mV[1]/distance)*2.f*RAD_TO_DEG;
        F32 aspect = tdim.mV[0]/tdim.mV[1];
        glm::mat4 persp = glm::perspective(glm::radians(fov), aspect, 1.f, 256.f);
        set_current_projection(persp);
        gGL.loadMatrix(glm::value_ptr(persp));

        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.pushMatrix();

        F32 ogl_mat[16];
        camera.getOpenGLTransform(ogl_mat);
        glm::mat4 mat = glm::make_mat4((GLfloat*) OGL_TO_CFR_ROTATION) * glm::make_mat4(ogl_mat);

        gGL.loadMatrix(glm::value_ptr(mat));
        set_current_modelview(mat);

        gGL.setClearColor(0.0f,0.0f,0.0f,0.0f);
        gGL.setColorMask(true, true);

        // get the number of pixels per angle
        F32 pa = gViewerWindow->getWindowHeightRaw() / (RAD_TO_DEG * viewer_camera->getView());

        //get resolution based on angle width and height of impostor (double desired resolution to prevent aliasing)
        resY = llmin(nhpo2((U32) (fov*pa)), (U32) 512);
        resX = llmin(nhpo2((U32) (atanf(tdim.mV[0]/distance)*2.f*RAD_TO_DEG*pa)), (U32) 512);

        if (!for_profile)
        {
            if (!avatar->mImpostor.isComplete())
            {
                avatar->mImpostor.allocate(resX, resY, GL_RGBA, true);

                if (isFrameRenderingDeferred())
                {
                    addDeferredAttachments(avatar->mImpostor, true);
                }

                gGL.getTexUnit(0)->bind(&avatar->mImpostor);
                gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
                gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
            }
            else if (resX != avatar->mImpostor.getWidth() || resY != avatar->mImpostor.getHeight())
            {
                avatar->mImpostor.resize(resX, resY);
            }

            rts.emplace(avatar->mImpostor, false, "impostor");
        }
    }

    F32 old_alpha = LLPipelineFrameContext::getInstance().getAvatarMinimumAlpha();

    if (visually_muted || too_complex)
    { //disable alpha masking for muted avatars (get whole skin silhouette)
        LLPipelineFrameContext::getInstance().setAvatarMinimumAlpha(0.f);
    }

    if (preview_avatar || for_profile)
    {
        // previews and profiles don't care about imposters
        renderGeomDeferred(camera);
        renderGeomPostDeferred(camera);
    }
    else if (rts && *rts)
    {
        avatar->mImpostor.clear();
        renderGeomDeferred(camera);

        renderGeomPostDeferred(camera);

        // Shameless hack time: render it all again,
        // this time writing the depth
        // values we need to generate the alpha mask below
        // while preserving the alpha-sorted color rendering
        // from the previous pass
        //
        sImpostorRenderAlphaDepthPass = true;
        // depth-only here...
        //
        gGL.setColorMask(false,false);
        renderGeomPostDeferred(camera);

        sImpostorRenderAlphaDepthPass = false;

    }

    LLPipelineFrameContext::getInstance().setAvatarMinimumAlpha(old_alpha);

    if (!for_profile && (preview_avatar || (rts && *rts)))
    { //create alpha mask based on depth buffer (grey out if muted)
        LLGLDisable blend(GL_BLEND);

        if (visually_muted || too_complex)
        {
            gGL.setColorMask(true, true);
        }
        else
        {
            gGL.setColorMask(false, true);
        }

        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

        LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_GREATER);

        gGL.flush();

        gGL.pushMatrix();
        gGL.loadIdentity();
        gGL.matrixMode(LLRender::MM_PROJECTION);
        gGL.pushMatrix();
        gGL.loadIdentity();

        static const F32 clip_plane = 0.99999f;

        gDebugProgram.bind();

        if (visually_muted)
        {   // Visually muted avatar
            LLColor4 muted_color(avatar->getMutedAVColor());
            LL_DEBUGS_ONCE("AvatarRenderPipeline") << "Avatar " << avatar->getID() << " MUTED set solid color " << muted_color << LL_ENDL;
            gGL.diffuseColor4fv( muted_color.mV );
        }
        else if (!preview_avatar)
        { //grey muted avatar
            LL_DEBUGS_ONCE("AvatarRenderPipeline") << "Avatar " << avatar->getID() << " MUTED set grey" << LL_ENDL;
            gGL.diffuseColor4fv(LLColor4::pink.mV );
        }

        gGL.begin(LLRender::TRIANGLES);
        {
            gGL.vertex3f(-1.f, -1.f, clip_plane);
            gGL.vertex3f(1.f, -1.f, clip_plane);
            gGL.vertex3f(1.f, 1.f, clip_plane);

            gGL.vertex3f(-1.f, -1.f, clip_plane);
            gGL.vertex3f(1.f, 1.f, clip_plane);
            gGL.vertex3f(-1.f, 1.f, clip_plane);
        }
        gGL.end();
        gGL.flush();

        gDebugProgram.unbind();

        gGL.popMatrix();
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.popMatrix();
    }

    if (rts && *rts)
    {
        rts.reset();
        avatar->setImpostorDim(tdim);
    }

    sUseOcclusion = occlusion;
    LLPipelineFrameContext::getInstance().setReflectionPass(false);
    LLPipelineFrameContext::getInstance().setImpostorPass(false);
    LLPipelineFrameContext::getInstance().setShadowPass(false);
    popRenderTypeMask();

    if (!preview_avatar)
    {
        gGL.matrixMode(LLRender::MM_PROJECTION);
        gGL.popMatrix();
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.popMatrix();
    }

    if (!preview_avatar && !for_profile)
    {
        avatar->mNeedsImpostorUpdate = false;
        avatar->cacheImpostorValues();
        avatar->mLastImpostorUpdateFrameTime = gFrameTimeSeconds;
    }

    LLVertexBuffer::unbind();
}

