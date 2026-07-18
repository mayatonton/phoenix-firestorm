/**
 * @file lldrawpoolalpha.cpp
 * @brief LLDrawPoolAlpha class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "lldrawpoolalpha.h"

#include "llglheaders.h"
#include "llviewercontrol.h"
#include "llcriticaldamp.h"
#include "llfasttimer.h"
#include "llrender.h"

#include "llcubemap.h"
#include "llsky.h"
#include "lldrawable.h"
#include "llface.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"    // For debugging
#include "llviewerobjectlist.h" // For debugging
#include "llviewerwindow.h"
#include "pipeline.h"
#include "llpipelineframecontext.h"
#include "llviewershadermgr.h"
#include "llviewerregion.h"
#include "lldrawpoolwater.h"
#include "llspatialpartition.h"
#include "llvoavatar.h"
#include "gltfscenemanager.h"
#include "lltoolmgr.h"
#include "llvkloader.h"
#include "llvkcontract.h"
#include "llvkuboreg.h"
#include "llimagegl.h"
#include "llfetchedgltfmaterial.h"

#include "llenvironment.h"

#include <optional>

bool LLDrawPoolAlpha::sShowDebugAlpha = false;
bool LLDrawPoolAlpha::sShowDebugAlphaRigged = false;

#define current_shader (LLGLSLShader::sCurBoundShaderPtr)

LLVector4 LLDrawPoolAlpha::sWaterPlane;

// minimum alpha before discarding a fragment
static const F32 MINIMUM_ALPHA = 0.004f; // ~ 1/255

// minimum alpha before discarding a fragment when rendering impostors
static const F32 MINIMUM_IMPOSTOR_ALPHA = 0.1f;

LLDrawPoolAlpha::LLDrawPoolAlpha(U32 type) :
        LLRenderPass(type), target_shader(NULL),
        mColorSFactor(LLRender::BF_UNDEF), mColorDFactor(LLRender::BF_UNDEF),
        mAlphaSFactor(LLRender::BF_UNDEF), mAlphaDFactor(LLRender::BF_UNDEF)
{

}

LLDrawPoolAlpha::~LLDrawPoolAlpha()
{
}


void LLDrawPoolAlpha::prerender()
{
    mShaderLevel = LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_OBJECT);
}

S32 LLDrawPoolAlpha::getNumPostDeferredPasses()
{
    return 1;
}

// set some common parameters on the given shader to prepare for alpha rendering
static void prepare_alpha_shader(LLGLSLShader* shader, bool deferredEnvironment, F32 water_sign)
{
    // Does this deferred shader need environment uniforms set such as sun_dir, etc. ?
    // NOTE: We don't actually need a gbuffer since we are doing forward rendering (for transparency) post deferred rendering
    // TODO: bindDeferredShader() probably should have the updating of the environment uniforms factored out into updateShaderEnvironmentUniforms()
    // i.e. shaders\class1\deferred\alphaF.glsl
    if (deferredEnvironment)
    {
        shader->mCanBindFast = false;
    }

    shader->bind();


    F32 water_sign_pc = water_sign;
    if (LLPipelineFrameContext::getInstance().isHUDPass())
    { // for HUD attachments, only the pre-water pass is executed and we never want to clip anything
        water_sign_pc = 0.f;
    }

    if (LLVKLoader::isVulkanInitialized() && shader->mVkPipelineLayout != VK_NULL_HANDLE)
    {
        VkCommandBuffer cmd = LLVKLoader::getCurrentCommandBuffer();
        if (cmd != VK_NULL_HANDLE)
        {
            vkCmdPushConstants(cmd, shader->mVkPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                               LLVkUboReg::PC_OFF_WATER_SIGN, sizeof(F32), &water_sign_pc);

            const F32 aya_preview_neutral_atmos = 0.f;
            vkCmdPushConstants(cmd, shader->mVkPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                               LLVkUboReg::PC_OFF_PREVIEW_NEUTRAL_ATMOS, sizeof(F32), &aya_preview_neutral_atmos);
        }
    }

    if (LLPipelineFrameContext::getInstance().isImpostorPass())
    {
        shader->setMinimumAlpha(MINIMUM_IMPOSTOR_ALPHA);
    }
    else
    {
        shader->setMinimumAlpha(MINIMUM_ALPHA);
    }

    //also prepare rigged variant
    if (shader->mRiggedVariant && shader->mRiggedVariant != shader)
    {
        prepare_alpha_shader(shader->mRiggedVariant, deferredEnvironment, water_sign);
    }
}

extern bool gCubeSnapshot;

void LLDrawPoolAlpha::renderPostDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;

    if (LLPipeline::isWaterClip() && getType() == LLDrawPool::POOL_ALPHA_PRE_WATER)
    { // don't render alpha objects on the other side of the water plane if water is opaque
        return;
    }

    const U64 prep_t0 = LLVKLoader::perfLogEnabled() ? (U64)LLTimer::getTotalTime() : 0;

    F32 water_sign = 1.f;

    if (getType() == LLDrawPool::POOL_ALPHA_PRE_WATER)
    {
        water_sign = -1.f;
    }

    if (LLPipelineFrameContext::getInstance().isUnderWaterRendering())
    {
        water_sign *= -1.f;
    }

    // prepare shaders
    llassert(LLPipelineFrameContext::getInstance().isRenderingDeferred());

    emissive_shader = &gDeferredEmissiveProgram;
    prepare_alpha_shader(emissive_shader, false, water_sign);

    pbr_emissive_shader = &gPBRGlowProgram;
    prepare_alpha_shader(pbr_emissive_shader, false, water_sign);


    fullbright_shader   =
        (LLPipelineFrameContext::getInstance().isImpostorPass()) ? &gDeferredFullbrightAlphaMaskProgram :
        (LLPipelineFrameContext::getInstance().isHUDPass()) ? &gHUDFullbrightAlphaMaskAlphaProgram :
        &gDeferredFullbrightAlphaMaskAlphaProgram;
    prepare_alpha_shader(fullbright_shader, true, water_sign);

    simple_shader   =
        (LLPipelineFrameContext::getInstance().isImpostorPass()) ? &gDeferredAlphaImpostorProgram :
        (LLPipelineFrameContext::getInstance().isHUDPass()) ? &gHUDAlphaProgram :
        &gDeferredAlphaProgram;

    prepare_alpha_shader(simple_shader, true, water_sign); //prime simple shader (loads shadow relevant uniforms)

    LLGLSLShader* materialShader = gDeferredMaterialProgram;
    for (int i = 0; i < LLMaterial::SHADER_COUNT; ++i)
    {
        prepare_alpha_shader(&materialShader[i], true, water_sign);
    }

    pbr_shader =
        (LLPipelineFrameContext::getInstance().isHUDPass()) ? &gHUDPBRAlphaProgram :
        &gDeferredPBRAlphaProgram;

    prepare_alpha_shader(pbr_shader, true, water_sign);

    // explicitly unbind here so render loop doesn't make assumptions about the last shader
    // already being setup for rendering
    LLGLSLShader::unbind();

    if (prep_t0 != 0)
    {
        LLVKLoader::gVkPerf.alpha_us[11] += (U64)LLTimer::getTotalTime() - prep_t0;
    }

    const bool use_alpha_rt =
        !LLPipelineFrameContext::getInstance().isImpostorPass() && !LLPipelineFrameContext::getInstance().isHUDPass() &&
        !gCubeSnapshot &&
        getType() == LLDrawPool::POOL_ALPHA_POST_WATER &&
        LLPipelineFrameContext::getInstance().getActiveRT() == &gPipeline.mMainRT &&
        gPipeline.mAYAAlphaColor.isComplete();

    // <AYAstorm r30 P5 plate-clear unconditional 2026-05-23>
    // Clear mAYAAlphaColor every frame regardless of use_alpha_rt. When
    // inBuildMode() (= LMB on HUD) flips use_alpha_rt=false, this branch used
    // to skip the clear → previous frame's plate contents leaked into the
    // next frame's composite. The pre-tonemap composite step (renderFinalize)
    // reads mAYAAlphaColor unconditionally, so it must start clean every
    // frame.
    if (!LLPipelineFrameContext::getInstance().isImpostorPass() && !LLPipelineFrameContext::getInstance().isHUDPass() &&
        !gCubeSnapshot && getType() == LLDrawPool::POOL_ALPHA_POST_WATER &&
        LLPipelineFrameContext::getInstance().getActiveRT() == &gPipeline.mMainRT &&
        gPipeline.mAYAAlphaColor.isComplete())
    {
        LL_PROFILE_GPU_ZONE("aya alpha color clear");
        gPipeline.mAYAAlphaColor.bindTarget();
        {
            LLGLDepthTest depth_off(GL_FALSE, GL_FALSE);
            gGL.setClearColor(0.f, 0.f, 0.f, 0.f);
            gPipeline.mAYAAlphaColor.clear(GL_COLOR_BUFFER_BIT);
        }
        gPipeline.mAYAAlphaColor.flush();
    }
    // </AYAstorm r30 P5 plate-clear unconditional>

    if (use_alpha_rt)
    {
        LL_PROFILE_GPU_ZONE("aya alpha color redirect");
        // LLRenderTarget keeps an FBO bind stack: pushing mAYAAlphaColor on
        // top leaves mRT->screen underneath, and flush() at the end pops back
        // to it automatically — no manual screen.flush()/bindTarget() needed
        // (doing so trips the !isBoundInStack assertion on re-push).
        gPipeline.mAYAAlphaColor.bindTarget();
        mForwardToAlphaRT = true;
    }
    // </AYAstorm r30 P5 transparent-DoF C-(a)>

    // <AYAstorm r30 P5 二重アルファブロック対策> POST_WATER 内の forward
    // render を non-rigged → rigged の back-to-front 順に **常時 default 化**。
    // 元の rigged-first 順は `write_depth = rigged` (forwardRender 行 379)
    // で attachment alpha BLEND (hair / clothing) の z を共有 depth に
    // 書き込み、後続の non-rigged alpha (Rez Object 側の窓ガラス / lace /
    // 葉先) を GL_LEQUAL で reject → fragment 自体が走らず画素は opaque 段
    // の sky のまま残る = 「髪越しに窓ガラスが sky に抜ける」二重アルファ
    // ブロック regression。以前は use_alpha_rt 時のみ限定 swap だったが、
    // RenderDepthOfField = false (use_alpha_rt=false) の path で同症状が
    // 実機再現 (canary=12 緑で確認、Cinematic mode + DoF OFF) したため、
    // POST_WATER 全 path で swap を default 化。PRE_WATER は water fog
    // 計算 (write_depth が always true) のため rigged-first を維持。HUD は
    // forwardRender 1 回のみで対象外。
    if (!LLPipelineFrameContext::getInstance().isHUDPass() &&
        getType() == LLDrawPool::POOL_ALPHA_POST_WATER)
    {
        forwardRenderMerged();
    }
    else
    {
        // PRE_WATER / HUD の元順 — water fog 整合性のため touch しない。
        if (!LLPipelineFrameContext::getInstance().isHUDPass())
        {
            forwardRender(true);
        }
        forwardRender();
    }
    // </AYAstorm r30 P5 二重アルファブロック対策>

    // <AYAstorm r30 P5 transparent-DoF C-(a)> Pop alpha plate RT — flush()
    // auto-restores mRT->screen from the FBO stack.
    if (use_alpha_rt)
    {
        gPipeline.mAYAAlphaColor.flush();
        mForwardToAlphaRT = false;
    }
    // </AYAstorm r30 P5 transparent-DoF C-(a)>

    // <AYAstorm r30 P3 step 5> Volumetric Lighting also benefits from alpha
    // objects (foliage, fabric) being committed to the depth buffer so that
    // shadow sampling along the godray accumulates against them instead of
    // shooting through. Extend the existing DoF alpha-depth gate to also
    // fire when Cinematic + RenderVolumetricLighting is on. BD lineage:
    // lldrawpoolalpha.cpp:210-226 in BlackDragon 995a1354d8 (BD OR'd the two
    // pipeline statics directly).
    const bool volumetric_wants_alpha_depth =
        LLPipeline::isCinematicMode()
        && gSavedSettings.getBOOL("RenderVolumetricLighting");
    // </AYAstorm r30 P3 step 5>

    // final pass, render to depth for depth of field effects
    if (!LLPipelineFrameContext::getInstance().isImpostorPass() && (LLPipeline::RenderDepthOfField || volumetric_wants_alpha_depth) && !gCubeSnapshot && !LLPipelineFrameContext::getInstance().isHUDPass() && getType() == LLDrawPool::POOL_ALPHA_POST_WATER)
    {
        //update depth buffer sampler
        simple_shader = fullbright_shader = &gDeferredFullbrightAlphaMaskProgram;

        simple_shader->bind();
        // <FS:AYAstorm:r30-bd-port> Phase 6 step 2: Cinematic mode honors BD
        // RenderDepthOfFieldAlphas (default OFF). When OFF, alpha objects do not
        // contribute to depth-of-field depth (BD verbatim, lldrawpoolalpha.cpp:225).
        F32 dof_alpha_cutoff = 0.33f;
        if (LLPipeline::isCinematicMode())
        {
            static LLCachedControl<bool> render_dof_alphas(gSavedSettings, "RenderDepthOfFieldAlphas", false);
            dof_alpha_cutoff = render_dof_alphas ? 0.7f : 1.f;
        }
        simple_shader->setMinimumAlpha(dof_alpha_cutoff);
        // </FS:AYAstorm:r30-bd-port>

        // mask off color buffer writes as we're only writing to depth buffer
        gGL.setColorMask(false, false);

        // If the face is more than 90% transparent, then don't update the Depth buffer for Dof
        // We don't want the nearly invisible objects to cause of DoF effects
        renderAlpha(getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX | LLVertexBuffer::MAP_TANGENT | LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TEXCOORD2,
            true); // <--- discard mostly transparent faces

        gGL.setColorMask(true, false);
    }

    // <AYAstorm r30 P5 transparent-DoF L2-β> 2nd depth prepass at cutoff 0.5
    // into the alpha-aware depth RT (mAYAAlphaDepth). The RT was initialised
    // by pipeline.cpp before forward alpha to the opaque-only depth; here we
    // re-inject only those alpha BLEND fragments whose texture alpha ≥ 0.5
    // (window grilles, lace, foliage, etc.) so cofF treats them as subject.
    // Hair-style low-alpha BLEND fragments (alpha ≈ 0.2) are discarded so
    // the snapshotted background z survives and cofF blurs the background
    // visible through the hair. Distinct from the prepass above which still
    // writes deferredScreen.depth for atmospherics / HQ DoF gate.
    // <AYAstorm r30 P5 transparent-DoF C-(a)> When C-(a) is active the alpha
    // BLEND color is composited as a separate plate on top of the DoF result,
    // so cofF wants the opaque-only mAYAAlphaDepth snapshot. Re-injecting
    // grille z here would tell cofF "this pixel is subject" and prevent bg
    // blur behind grilles — exactly the regression C-(a) avoids. Skip the
    // re-injection when mAYAAlphaColor is alive.
    if (!LLPipelineFrameContext::getInstance().isImpostorPass() && LLPipeline::RenderDepthOfField &&
        !gCubeSnapshot && !LLPipelineFrameContext::getInstance().isHUDPass() &&
        getType() == LLDrawPool::POOL_ALPHA_POST_WATER &&
        LLPipelineFrameContext::getInstance().getActiveRT() == &gPipeline.mMainRT &&
        gPipeline.mAYAAlphaDepth.isComplete() &&
        !gPipeline.mAYAAlphaColor.isComplete())
    {
        LL_PROFILE_GPU_ZONE("aya alpha depth re-inject");
        gPipeline.mAYAAlphaDepth.bindTarget();

        simple_shader = fullbright_shader = &gDeferredFullbrightAlphaMaskProgram;
        simple_shader->bind();
        simple_shader->setMinimumAlpha(0.5f);

        gGL.setColorMask(false, false);
        renderAlpha(getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX | LLVertexBuffer::MAP_TANGENT | LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TEXCOORD2,
            true); // discard mostly transparent faces
        gGL.setColorMask(true, false);

        gPipeline.mAYAAlphaDepth.flush();
    }
    // </AYAstorm r30 P5 transparent-DoF L2-β>
}

void LLDrawPoolAlpha::forwardRender(bool rigged)
{
    gPipeline.enableLightsDynamic();

    LLGLSPipelineAlpha gls_pipeline_alpha;

    //enable writing to alpha for emissive effects
    gGL.setColorMask(true, true);

    bool write_depth = rigged ||
        LLDrawPoolWater::sSkipScreenCopy
        // we want depth written so that rendered alpha will
        // contribute to the alpha mask used for impostors
        || LLPipeline::sImpostorRenderAlphaDepthPass
        || getType() == LLDrawPoolAlpha::POOL_ALPHA_PRE_WATER; // needed for accurate water fog

    LLGLDepthTest depth(GL_TRUE, write_depth ? GL_TRUE : GL_FALSE);

    mColorSFactor = LLRender::BF_SOURCE_ALPHA;           // } regular alpha blend
    mColorDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA; // }
    // <AYAstorm r30 P5 transparent-DoF C-(a)> When redirected to
    // mAYAAlphaColor, switch alpha-channel factor to (ONE, 1-Sa) so the
    // separate plate accumulates premultiplied alpha = src.a (correct
    // coverage for "over" composite in dofCombineF). The default
    // (ZERO, 1-Sa) is "glow suppression" and only makes sense when alpha
    // is being written into mRT->screen.a (the scene buffer's glow channel).
    if (mForwardToAlphaRT)
    {
        mAlphaSFactor = LLRender::BF_ONE;
        mAlphaDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA;
    }
    else
    {
        mAlphaSFactor = LLRender::BF_ZERO;                         // } glow suppression
        mAlphaDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA;       // }
    }
    // </AYAstorm r30 P5 transparent-DoF C-(a)>
    gGL.blendFunc(mColorSFactor, mColorDFactor, mAlphaSFactor, mAlphaDFactor);

    if (rigged && mType == LLDrawPool::POOL_ALPHA_POST_WATER)
    { // draw GLTF scene to depth buffer before rigged alpha
        LL::GLTFSceneManager::instance().render(false, false);
        LL::GLTFSceneManager::instance().render(false, true);
        LL::GLTFSceneManager::instance().render(false, false, true);
        LL::GLTFSceneManager::instance().render(false, true, true);
    }

    // If the face is more than 90% transparent, then don't update the Depth buffer for Dof
    // We don't want the nearly invisible objects to cause of DoF effects
    renderAlpha(getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX | LLVertexBuffer::MAP_TANGENT | LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TEXCOORD2, false, rigged);

    gGL.setColorMask(true, false);

    if (!rigged && getType() == LLDrawPoolAlpha::POOL_ALPHA_POST_WATER)
    { //render "highlight alpha" on final non-rigged pass
        // NOTE -- hacky call here protected by !rigged instead of alongside "forwardRender"
        // so renderDebugAlpha is executed while gls_pipeline_alpha and depth GL state
        // variables above are still in scope
        renderDebugAlpha();
    }
}

void LLDrawPoolAlpha::forwardRenderMerged()
{
    gPipeline.enableLightsDynamic();

    LLGLSPipelineAlpha gls_pipeline_alpha;

    gGL.setColorMask(true, true);

    bool write_depth =
        LLDrawPoolWater::sSkipScreenCopy
        || LLPipeline::sImpostorRenderAlphaDepthPass
        || getType() == LLDrawPoolAlpha::POOL_ALPHA_PRE_WATER;

    LLGLDepthTest depth(GL_TRUE, write_depth ? GL_TRUE : GL_FALSE);

    mColorSFactor = LLRender::BF_SOURCE_ALPHA;
    mColorDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA;
    if (mForwardToAlphaRT)
    {
        mAlphaSFactor = LLRender::BF_ONE;
        mAlphaDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA;
    }
    else
    {
        mAlphaSFactor = LLRender::BF_ZERO;
        mAlphaDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA;
    }
    gGL.blendFunc(mColorSFactor, mColorDFactor, mAlphaSFactor, mAlphaDFactor);

    if (mType == LLDrawPool::POOL_ALPHA_POST_WATER)
    {
        LLGLDepthTest gltf_depth(GL_TRUE, GL_TRUE);
        LL::GLTFSceneManager::instance().render(false, false);
        LL::GLTFSceneManager::instance().render(false, true);
        LL::GLTFSceneManager::instance().render(false, false, true);
        LL::GLTFSceneManager::instance().render(false, true, true);
    }

    renderAlpha(getVertexDataMask() | LLVertexBuffer::MAP_TEXTURE_INDEX | LLVertexBuffer::MAP_TANGENT | LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TEXCOORD2, false, false, true);

    gGL.setColorMask(true, false);

    if (getType() == LLDrawPoolAlpha::POOL_ALPHA_POST_WATER)
    {
        renderDebugAlpha();
    }
}

void LLDrawPoolAlpha::renderDebugAlpha()
{
    if (sShowDebugAlpha && !gCubeSnapshot)
    {
        gHighlightProgram.bind();
        gGL.diffuseColor4f(1, 0, 0, 1);
        gGL.getTexUnit(0)->bindFast(LLViewerFetchedTexture::getSmokeImage());


        renderAlphaHighlight();

        pushUntexturedBatches(LLRenderPass::PASS_ALPHA_MASK);
        pushUntexturedBatches(LLRenderPass::PASS_ALPHA_INVISIBLE);

        // Material alpha mask
        gGL.diffuseColor4f(0, 0, 1, 1);
        pushUntexturedBatches(LLRenderPass::PASS_MATERIAL_ALPHA_MASK);
        pushUntexturedBatches(LLRenderPass::PASS_NORMMAP_MASK);
        pushUntexturedBatches(LLRenderPass::PASS_SPECMAP_MASK);
        pushUntexturedBatches(LLRenderPass::PASS_NORMSPEC_MASK);
        pushUntexturedBatches(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK);
        pushUntexturedBatches(LLRenderPass::PASS_GLTF_PBR_ALPHA_MASK);

        gGL.diffuseColor4f(0, 1, 0, 1);
        pushUntexturedBatches(LLRenderPass::PASS_INVISIBLE);
        // <FS:Beq> FIRE-32132 et al. Allow rigged mesh transparency highlights to be toggled
        if (sShowDebugAlphaRigged)
        {
        // </FS:Beq>
        gHighlightProgram.mRiggedVariant->bind();
        gGL.diffuseColor4f(0, 1, 0, 1);// <FS:Beq/> FIRE-32132 et al. (can plain PASS_ALPHA_MASK_RIGGED exist?) paint it green if so.
        pushRiggedBatches(LLRenderPass::PASS_ALPHA_MASK_RIGGED, false);
        pushRiggedBatches(LLRenderPass::PASS_ALPHA_INVISIBLE_RIGGED, false);

        // Material alpha mask
        gGL.diffuseColor4f(0, 1, 1, 1);// <FS:Beq/> FIRE-32132 et al. Allow rigged mesh transparency highlights to be toggled
        pushRiggedBatches(LLRenderPass::PASS_MATERIAL_ALPHA_MASK_RIGGED, false);
        pushRiggedBatches(LLRenderPass::PASS_NORMMAP_MASK_RIGGED, false);
        pushRiggedBatches(LLRenderPass::PASS_SPECMAP_MASK_RIGGED, false);
        pushRiggedBatches(LLRenderPass::PASS_NORMSPEC_MASK_RIGGED, false);
        pushRiggedBatches(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK_RIGGED, false);
        pushRiggedBatches(LLRenderPass::PASS_GLTF_PBR_ALPHA_MASK_RIGGED, false);

        gGL.diffuseColor4f(0, 1, 0, 1);
        pushRiggedBatches(LLRenderPass::PASS_INVISIBLE_RIGGED, false);
        // <FS:Beq> FIRE-32132 et al. Allow rigged mesh transparency highlights to be toggled
        }
        // </FS:Beq>
        LLGLSLShader::sCurBoundShaderPtr->unbind();
    }
}

void LLDrawPoolAlpha::renderAlphaHighlight()
{
    for (int pass = 0; pass < 2; ++pass)
    { //two passes, one rigged and one not
        const LLVOAvatar* lastAvatar = nullptr;
        U64 lastMeshId = 0;
        bool skipLastSkin = false;

        LLCullResult::sg_iterator begin = pass == 0 ? gPipeline.beginAlphaGroups() : gPipeline.beginRiggedAlphaGroups();
        LLCullResult::sg_iterator end = pass == 0 ? gPipeline.endAlphaGroups() : gPipeline.endRiggedAlphaGroups();

        for (LLCullResult::sg_iterator i = begin; i != end; ++i)
        {
            LLSpatialGroup* group = *i;
            if (group->getSpatialPartition()->mRenderByGroup &&
                !group->isDead())
            {
                LLSpatialGroup::drawmap_elem_t& draw_info = group->mDrawMap[LLRenderPass::PASS_ALPHA+pass]; // <-- hacky + pass to use PASS_ALPHA_RIGGED on second pass

                for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)
                {
                    LLDrawInfo& params = **k;

                    bool rigged = (params.mAvatar != nullptr);
                    gHighlightProgram.bind(rigged);

                    if (rigged)
                    {
                        if (!uploadMatrixPalette(params.mAvatar, params.mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
                        { // failed to upload matrix palette, skip rendering
                            continue;
                        }
                    }

                    // <FS:Beq> FIRE-32132 et al. Allow rigged mesh transparency highlights to be toggled
                    if (rigged && !sShowDebugAlphaRigged)
                    {
                        // if we don't want to show rigged alpha highlights then skip
                        continue;
                    }
                    else if (rigged && sShowDebugAlphaRigged)
                    {
                        // if we do and this is rigged then use a different colour
                        gGL.diffuseColor4f(1, 0.5, 0, 1);
                    }
                    else // NB dangling else to drop through to "normal behaviour"
                    // </FS:Beq>
                    gGL.diffuseColor4f(1, 0, 0, 1);
                    LLRenderPass::applyModelMatrix(params);
                    params.mVertexBuffer->setBuffer();
                    params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
                }
            }
        }
    }

    // make sure static version of highlight shader is bound before returning
    gHighlightProgram.bind();
}

inline bool IsFullbright(LLDrawInfo& params)
{
    return params.mFullbright;
}

inline bool IsMaterial(LLDrawInfo& params)
{
    return params.mMaterial != nullptr;
}

inline bool IsEmissive(LLDrawInfo& params)
{
    return params.mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_EMISSIVE);
}

inline void Draw(LLDrawInfo* draw, U32 mask)
{
    LLVKContract::DrawScope vkc_scope(draw, "alpha");
    draw->mVertexBuffer->setBuffer();
    LLRenderPass::applyModelMatrix(*draw);
    draw->mVertexBuffer->drawRange(LLRender::TRIANGLES, draw->mStart, draw->mEnd, draw->mCount, draw->mOffset);
}

bool LLDrawPoolAlpha::TexSetup(LLDrawInfo* draw, bool use_material)
{
    bool tex_setup = false;

    if (draw->mGLTFMaterial)
    {
        if (draw->mTextureMatrix)
        {
            tex_setup = true;
            gGL.getTexUnit(0)->activate();
            gGL.matrixMode(LLRender::MM_TEXTURE);
            gGL.loadMatrix((GLfloat*)draw->mTextureMatrix->mMatrix);
            gPipeline.mTextureMatrixOps++;
        }
    }
    else
    {
        if (!LLPipelineFrameContext::getInstance().isHUDPass() && use_material && current_shader)
        {
            if (draw->mNormalMap)
            {
                current_shader->bindTexture(LLShaderMgr::BUMP_MAP, draw->mNormalMap);
            }

            if (draw->mSpecularMap)
            {
                current_shader->bindTexture(LLShaderMgr::SPECULAR_MAP, draw->mSpecularMap);
            }
        }
        else if (current_shader == simple_shader || current_shader == simple_shader->mRiggedVariant)
        {
            current_shader->bindTexture(LLShaderMgr::BUMP_MAP, LLViewerFetchedTexture::sFlatNormalImagep);
            current_shader->bindTexture(LLShaderMgr::SPECULAR_MAP, LLViewerFetchedTexture::sWhiteImagep);
        }
        if (draw->mTextureList.size() > 1)
        {
            for (U32 i = 0; i < draw->mTextureList.size(); ++i)
            {
                if (draw->mTextureList[i].notNull())
                {
                    gGL.getTexUnit(i)->bindFast(draw->mTextureList[i]);
                }
            }
        }
        else
        { //not batching textures or batch has only 1 texture -- might need a texture matrix
            if (draw->mTexture.notNull())
            {
                if (use_material)
                {
                    current_shader->bindTexture(LLShaderMgr::DIFFUSE_MAP, draw->mTexture);
                }
                else
                {
                    gGL.getTexUnit(0)->bindFast(draw->mTexture);
                }

                if (draw->mTextureMatrix)
                {
                    tex_setup = true;
                    gGL.getTexUnit(0)->activate();
                    gGL.matrixMode(LLRender::MM_TEXTURE);
                    gGL.loadMatrix((GLfloat*)draw->mTextureMatrix->mMatrix);
                    gPipeline.mTextureMatrixOps++;
                }
            }
            else
            {
                gGL.getTexUnit(0)->unbindFast(LLTexUnit::TT_TEXTURE);
            }
        }
    }

    return tex_setup;
}

void LLDrawPoolAlpha::RestoreTexSetup(bool tex_setup)
{
    if (tex_setup)
    {
        gGL.getTexUnit(0)->activate();
        gGL.matrixMode(LLRender::MM_TEXTURE);
        gGL.loadIdentity();
        gGL.matrixMode(LLRender::MM_MODELVIEW);
    }
}

void LLDrawPoolAlpha::drawEmissive(LLDrawInfo* draw)
{
    LLVKContract::DrawScope vkc_scope(draw, "alphaEmi");
    draw->mVertexBuffer->setBuffer();
    draw->mVertexBuffer->drawRange(LLRender::TRIANGLES, draw->mStart, draw->mEnd, draw->mCount, draw->mOffset);
}


void LLDrawPoolAlpha::renderEmissives(std::vector<LLDrawInfo*>& emissives)
{
    emissive_shader->bind();

    for (LLDrawInfo* draw : emissives)
    {
        bool tex_setup = TexSetup(draw, false);
        drawEmissive(draw);
        RestoreTexSetup(tex_setup);
    }
}

void LLDrawPoolAlpha::renderPbrEmissives(std::vector<LLDrawInfo*>& emissives)
{
    pbr_emissive_shader->bind();

    for (LLDrawInfo* draw : emissives)
    {
        llassert(draw->mGLTFMaterial);
        LLGLDisable cull_face(draw->mGLTFMaterial->mDoubleSided ? GL_CULL_FACE : 0);
        draw->mGLTFMaterial->bind(draw->mTexture);
        LLVKContract::DrawScope vkc_scope(draw, "alphaPbrEmi");
        draw->mVertexBuffer->setBuffer();
        draw->mVertexBuffer->drawRange(LLRender::TRIANGLES, draw->mStart, draw->mEnd, draw->mCount, draw->mOffset);
    }
}

void LLDrawPoolAlpha::renderRiggedEmissives(std::vector<LLDrawInfo*>& emissives)
{
    LLGLDepthTest depth(GL_TRUE, GL_FALSE); //disable depth writes since "emissive" is additive so sorting doesn't matter
    LLGLSLShader* shader = emissive_shader->mRiggedVariant;
    shader->bind();

    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin = false;

    for (LLDrawInfo* draw : emissives)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("Emissives");

        if (uploadMatrixPalette(draw->mAvatar, draw->mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
        {
            bool tex_setup = TexSetup(draw, false);
            drawEmissive(draw);
            RestoreTexSetup(tex_setup);
        }
    }
}

void LLDrawPoolAlpha::renderRiggedPbrEmissives(std::vector<LLDrawInfo*>& emissives)
{
    LLGLDepthTest depth(GL_TRUE, GL_FALSE); //disable depth writes since "emissive" is additive so sorting doesn't matter
    pbr_emissive_shader->bind(true);

    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin = false;

    for (LLDrawInfo* draw : emissives)
    {
        if (!uploadMatrixPalette(draw->mAvatar, draw->mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
        { // failed to upload matrix palette, skip rendering
            continue;
        }

        LLGLDisable cull_face(draw->mGLTFMaterial->mDoubleSided ? GL_CULL_FACE : 0);
        draw->mGLTFMaterial->bind(draw->mTexture);
        LLVKContract::DrawScope vkc_scope(draw, "alphaPbrEmi");
        draw->mVertexBuffer->setBuffer();
        draw->mVertexBuffer->drawRange(LLRender::TRIANGLES, draw->mStart, draw->mEnd, draw->mCount, draw->mOffset);
    }
}

namespace
{

struct AlphaRunSpan
{
    U32 mFirst = 0;
    U32 mCount = 0;
    LLDrawInfo* mRep = nullptr;
};

struct AlphaRun
{
    std::vector<VkDrawIndexedIndirectCommand> mCmds;
    std::vector<AlphaRunSpan> mSpans;
    const LLMatrix4* mModelMatrix = nullptr;
    LLGLSLShader* mShader = nullptr;

    bool empty() const { return mCmds.empty(); }

    void reset()
    {
        mCmds.clear();
        mSpans.clear();
        mModelMatrix = nullptr;
        mShader = nullptr;
    }
};

constexpr size_t ALPHA_RUN_MAX_CMDS = 4096;

bool alphaRunCandidate(const LLDrawInfo& params)
{
    if (!LLVKLoader::isIndirectDrawEnabled())
    {
        return false;
    }
    if (params.mAvatar.notNull()
        || params.mMaterial != nullptr
        || params.mGLTFMaterial.notNull()
        || params.mTextureMatrix != nullptr)
    {
        return false;
    }
    if (params.mBlendFuncSrc != LLRender::BF_SOURCE_ALPHA
        || params.mBlendFuncDst != LLRender::BF_ONE_MINUS_SOURCE_ALPHA)
    {
        return false;
    }
    LLVertexBuffer* vb = params.mVertexBuffer.get();
    if (vb == nullptr || params.mCount == 0
        || vb->getVkVertexSlice().buffer == VK_NULL_HANDLE
        || vb->getVkIndexSlice().buffer == VK_NULL_HANDLE)
    {
        return false;
    }
    return true;
}

void writeAlphaPerProgramUBO(bool reset_minimum_alpha)
{
    if (LLVKLoader::isVulkanInitialized() && current_shader
        && current_shader->mWritePerProgramUBOMinimumAlpha
        && current_shader->mVkPerProgramUBO != VK_NULL_HANDLE
        && current_shader->mVkPerProgramUBOMapped != nullptr
        && (current_shader->mVkPerProgramUBOSize == LLVKLoader::ALPHAF_UBO_SIZE_SHADOW
            || current_shader->mVkPerProgramUBOSize == LLVKLoader::ALPHAF_UBO_SIZE_NO_SHADOW))
    {
        const bool no_shadow =
            (current_shader->mVkPerProgramUBOSize == LLVKLoader::ALPHAF_UBO_SIZE_NO_SHADOW);
        const F32 cur_min_alpha = reset_minimum_alpha ? 0.f : MINIMUM_ALPHA;

        current_shader->rotatePerProgramUBOSlot();
        char* base = (char*)current_shader->mVkActivePerProgramUBOMapped;

        memcpy(base + 0, &cur_min_alpha, sizeof(F32));

        const F32 near_clip_v = LLViewerCamera::getInstance()->getNear() * 2.f;
        memcpy(base + LLVKLoader::ALPHAF_UBO_OFFSET_NEAR_CLIP, &near_clip_v, sizeof(F32));

        if (no_shadow)
        {
            F32 sun_moon[8] = {
                gPipeline.mTransformedSunDir.mV[0],  gPipeline.mTransformedSunDir.mV[1],  gPipeline.mTransformedSunDir.mV[2],  0.f,
                gPipeline.mTransformedMoonDir.mV[0], gPipeline.mTransformedMoonDir.mV[1], gPipeline.mTransformedMoonDir.mV[2], 0.f,
            };
            memcpy(base + LLVKLoader::ALPHAF_UBO_OFFSET_SUN_MOON, sun_moon, sizeof(sun_moon));
        }

        U32 lights_offset =
            no_shadow ? LLVKLoader::ALPHAF_UBO_OFFSET_LIGHTS_NO_SHADOW
                      : LLVKLoader::ALPHAF_UBO_OFFSET_LIGHTS_SHADOW;
        F32 lp[LL_NUM_LIGHT_UNITS * 4];
        F32 ld[LL_NUM_LIGHT_UNITS * 3];
        F32 la[LL_NUM_LIGHT_UNITS * 4];
        F32 ldi[LL_NUM_LIGHT_UNITS * 3];
        gGL.getLightArrayData(lp, ld, la, ldi);

        memcpy(base + lights_offset, lp, sizeof(F32) * LL_NUM_LIGHT_UNITS * 4);
        lights_offset += 128;
        for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; i++)
        {
            F32 v[4] = { ld[i*3+0], ld[i*3+1], ld[i*3+2], 0.f };
            memcpy(base + lights_offset + i * 16, v, 16);
        }
        lights_offset += 128;
        memcpy(base + lights_offset, la, sizeof(F32) * LL_NUM_LIGHT_UNITS * 4);
        lights_offset += 128;
        for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; i++)
        {
            F32 v[4] = { ldi[i*3+0], ldi[i*3+1], ldi[i*3+2], 0.f };
            memcpy(base + lights_offset + i * 16, v, 16);
        }
    }
}

void appendAlphaRunCmd(AlphaRun& run, LLDrawInfo& params)
{
    LLVertexBuffer* vb = params.mVertexBuffer.get();
    const LLVKLoader::MegaSliceV& vs = vb->getVkVertexSlice();
    const LLVKLoader::MegaSliceI& is = vb->getVkIndexSlice();
    AlphaRunSpan* span = run.mSpans.empty() ? nullptr : &run.mSpans.back();
    LLVertexBuffer* rep_vb = span ? span->mRep->mVertexBuffer.get() : nullptr;
    if (rep_vb == nullptr
        || rep_vb->getVkVertexSlice().buffer != vs.buffer
        || rep_vb->getVkIndexSlice().buffer != is.buffer
        || rep_vb->getIndicesType() != vb->getIndicesType())
    {
        AlphaRunSpan fresh;
        fresh.mFirst = (U32)run.mCmds.size();
        fresh.mCount = 0;
        fresh.mRep   = &params;
        run.mSpans.push_back(fresh);
        span = &run.mSpans.back();
    }
    ++span->mCount;
    VkDrawIndexedIndirectCommand dc;
    dc.indexCount    = params.mCount;
    dc.instanceCount = 1;
    dc.firstIndex    = is.offset / vb->getIndicesStride() + params.mOffset;
    dc.vertexOffset  = (S32)vs.first;
    dc.firstInstance = LLVKLoader::getCurrentDrawDataID();
    run.mCmds.push_back(dc);
}

void flushAlphaRun(AlphaRun& run)
{
    if (run.empty())
    {
        run.reset();
        return;
    }
    const U64 flu_t0 = LLVKLoader::perfLogEnabled() ? (U64)LLTimer::getTotalTime() : 0;
    LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
    llassert(shader == run.mShader);
    if (shader != nullptr)
    {
        gGL.syncMatrices();
        LLVKContract::DrawScope vkc_scope(nullptr, "alphaRun");
        VkDescriptorSet set_to_bind = LLGLSLShader::vkResolvePerCallSetForDraw();
        VkCommandBuffer cmd = LLVKLoader::getCurrentCommandBuffer();
        if (set_to_bind == VK_NULL_HANDLE)
        {
            LLVKContract::drawSkipped(LLVKContract::C_UNKNOWN, shader->mName);
        }
        else if (cmd == VK_NULL_HANDLE)
        {
            LLVKContract::drawSkipped(LLVKContract::C_CMD_NULL, shader->mName);
        }
        if (set_to_bind != VK_NULL_HANDLE && cmd != VK_NULL_HANDLE)
        {
            VkPipeline pipeline = shader->getOrCreateVkPipelineForBoundRT(LLRender::TRIANGLES);
            if (pipeline == VK_NULL_HANDLE)
            {
                LLVKContract::drawSkipped(LLVKContract::C_PIPELINE_NULL, shader->mName);
            }
            else
            {
                if (!LLVKLoader::isInRenderPassScope())
                {
                    LLRenderTarget* bound_rt = LLRenderTarget::getCurrentBoundTarget();
                    if (bound_rt == nullptr)
                    {
                        LLVKLoader::beginSwapchainRendering();
                    }
                    else
                    {
                        bound_rt->resumeVkDynamicRendering();
                    }
                }
                LLVKLoader::bindGraphicsPipelineOnce(cmd, pipeline);
                {
                    const bool vk_screen_space_copy = LLGLSLShader::vkUsePositiveViewport(
                        LLRenderTarget::getCurrentBoundTarget() != nullptr,
                        LLGLSLShader::vkCaptureRegimeActive());
                    LLVKLoader::setupViewportAndScissor(cmd, vk_screen_space_copy);
                }
                LLVKLoader::bindDrawDescriptorSetsOnce(cmd,
                                                       shader->mVkPipelineLayout,
                                                       LLVKLoader::getCurrentPerFrameDescriptorSet(),
                                                       set_to_bind,
                                                       shader->mVkSet1DynamicCount,
                                                       LLGLSLShader::sCurPerCallVkDynamicOffsets);
                LLVKLoader::pushModelviewOnce(cmd,
                                              shader->mVkPipelineLayout,
                                              LLVKLoader::getCurrentModelviewMatrix());
                VkBuffer     ring_buf    = VK_NULL_HANDLE;
                VkDeviceSize ring_offset = 0;
                void*        ring_mapped = nullptr;
                if (LLVKLoader::indirectRingAlloc((U32)run.mCmds.size(), ring_buf, ring_offset, ring_mapped))
                {
                    std::memcpy(ring_mapped, run.mCmds.data(),
                                run.mCmds.size() * sizeof(VkDrawIndexedIndirectCommand));
                    for (const AlphaRunSpan& span : run.mSpans)
                    {
                        span.mRep->mVertexBuffer->setBuffer();
                        vkCmdDrawIndexedIndirect(cmd, ring_buf,
                                                 ring_offset + (VkDeviceSize)span.mFirst * sizeof(VkDrawIndexedIndirectCommand),
                                                 span.mCount,
                                                 sizeof(VkDrawIndexedIndirectCommand));
                        ++LLVKLoader::gVkPerf.mdi_call;
                    }
                }
                else
                {
                    for (const AlphaRunSpan& span : run.mSpans)
                    {
                        span.mRep->mVertexBuffer->setBuffer();
                        for (U32 c = span.mFirst; c < span.mFirst + span.mCount; ++c)
                        {
                            const VkDrawIndexedIndirectCommand& dc = run.mCmds[c];
                            vkCmdDrawIndexed(cmd, dc.indexCount, 1, dc.firstIndex, dc.vertexOffset, dc.firstInstance);
                        }
                    }
                }
                ++LLVKLoader::gVkPerf.alp_run;
            }
        }
    }
    if (flu_t0 != 0)
    {
        LLVKLoader::gVkPerf.alpha_us[9] += (U64)LLTimer::getTotalTime() - flu_t0;
    }
    run.reset();
}

}

void LLDrawPoolAlpha::renderAlpha(U32 mask, bool depth_only, bool rigged, bool unified)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    bool initialized_lighting = false;
    bool light_enabled = true;

    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    const LLGLSLShader* lastAvatarShader = nullptr;
    bool skipLastSkin = false;

    std::vector<std::pair<LLSpatialGroup*, bool>> render_groups;

    if (unified)
    {
        LLCullResult::sg_iterator ai = gPipeline.beginAlphaGroups();
        LLCullResult::sg_iterator ae = gPipeline.endAlphaGroups();
        LLCullResult::sg_iterator ri = gPipeline.beginRiggedAlphaGroups();
        LLCullResult::sg_iterator re = gPipeline.endRiggedAlphaGroups();
        while (ai != ae || ri != re)
        {
            if (ri == re || (ai != ae && (*ai)->mDepth >= (*ri)->mDepth))
            {
                render_groups.emplace_back(*ai, false);
                ++ai;
            }
            else
            {
                render_groups.emplace_back(*ri, true);
                ++ri;
            }
        }
    }
    else if (rigged)
    {
        for (LLCullResult::sg_iterator i = gPipeline.beginRiggedAlphaGroups(); i != gPipeline.endRiggedAlphaGroups(); ++i)
        {
            render_groups.emplace_back(*i, true);
        }
    }
    else
    {
        for (LLCullResult::sg_iterator i = gPipeline.beginAlphaGroups(); i != gPipeline.endAlphaGroups(); ++i)
        {
            render_groups.emplace_back(*i, false);
        }
    }

    bool unified_base_write_depth =
        LLDrawPoolWater::sSkipScreenCopy
        || LLPipeline::sImpostorRenderAlphaDepthPass
        || getType() == LLDrawPoolAlpha::POOL_ALPHA_PRE_WATER;

    LLEnvironment& env = LLEnvironment::instance();
    F32 water_height = env.getWaterHeight();

    bool above_water = getType() == LLDrawPool::POOL_ALPHA_POST_WATER;
    if (LLPipelineFrameContext::getInstance().isUnderWaterRendering())
    {
        above_water = !above_water;
    }


    static AlphaRun run;

    const bool alp_perf = LLVKLoader::perfLogEnabled();
    U64 alp_us[11] = {};
    auto alp_now = [&]() -> U64 { return alp_perf ? (U64)LLTimer::getTotalTime() : 0; };

    for (const std::pair<LLSpatialGroup*, bool>& render_entry : render_groups)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("renderAlpha - group");
        LLSpatialGroup* group = render_entry.first;
        const bool group_rigged = render_entry.second;
        std::optional<LLGLDepthTest> unified_depth;
        if (unified)
        {
            unified_depth.emplace(GL_TRUE, (group_rigged || unified_base_write_depth) ? GL_TRUE : GL_FALSE);
        }
        llassert(group);
        llassert(group->getSpatialPartition());

        if (group->getSpatialPartition()->mRenderByGroup &&
            !group->isDead())
        {

            LLSpatialBridge* bridge = group->getSpatialPartition()->asBridge();
            const LLVector4a* ext = bridge ? bridge->getSpatialExtents() : group->getExtents();

            if (!LLPipelineFrameContext::getInstance().isHUDPass())
            {
                if (above_water)
                { // reject any spatial groups that have no part above water
                    if (ext[1].getF32ptr()[2] < water_height)
                    {
                        continue;
                    }
                }
                else
                { // reject any spatial groups that he no part below water
                    if (ext[0].getF32ptr()[2] > water_height)
                    {
                        continue;
                    }
                }
            }

            static std::vector<LLDrawInfo*> emissives;
            static std::vector<LLDrawInfo*> rigged_emissives;
            static std::vector<LLDrawInfo*> pbr_emissives;
            static std::vector<LLDrawInfo*> pbr_rigged_emissives;

            emissives.resize(0);
            rigged_emissives.resize(0);
            pbr_emissives.resize(0);
            pbr_rigged_emissives.resize(0);

            bool is_particle_or_hud_particle = group->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_PARTICLE
                                                      || group->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_HUD_PARTICLE;

            if (depth_only && is_particle_or_hud_particle)
            {
                continue;
            }

            // <FS:LO> Dont suspend partical processing while particles are hidden, just skip over drawing them
            if(!(gPipeline.sRenderParticles) && (
                                                 group->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_PARTICLE ||
                                                 group->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_HUD_PARTICLE))
            {
                continue;
            }
            // </FS:LO>

            bool disable_cull = is_particle_or_hud_particle;
            LLGLDisable cull(disable_cull ? GL_CULL_FACE : 0);

            LLSpatialGroup::drawmap_elem_t& draw_info = group_rigged ? group->mDrawMap[LLRenderPass::PASS_ALPHA_RIGGED] : group->mDrawMap[LLRenderPass::PASS_ALPHA];

            for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)
            {
                LLDrawInfo& params = **k;
                if ((bool)params.mAvatar != group_rigged)
                {
                    continue;
                }

                LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("ra - push batch");

                const bool run_candidate = alphaRunCandidate(params);
                if (!run.empty() && (!run_candidate || params.mModelMatrix != run.mModelMatrix))
                {
                    flushAlphaRun(run);
                }

                U64 alp_t = alp_now();

                LLRenderPass::applyModelMatrix(params);

                { U64 t2 = alp_now(); alp_us[0] += t2 - alp_t; alp_t = t2; }

                LLMaterial* mat = NULL;
                LLGLTFMaterial *gltf_mat = params.mGLTFMaterial;

                LLGLDisable cull_face(gltf_mat && gltf_mat->mDoubleSided ? GL_CULL_FACE : 0);

                if (gltf_mat && gltf_mat->mAlphaMode == LLGLTFMaterial::ALPHA_MODE_BLEND)
                {
                    target_shader = pbr_shader;
                    if (params.mAvatar != nullptr)
                    {
                        target_shader = target_shader->mRiggedVariant;
                    }

                    // shader must be bound before LLGLTFMaterial::bind
                    if (current_shader != target_shader)
                    {
                        gPipeline.bindDeferredShaderFast(*target_shader);
                    }

                    params.mGLTFMaterial->bind(params.mTexture);

                    if (LLVKLoader::isVulkanInitialized()
                        && target_shader == &gHUDPBRAlphaProgram
                        && target_shader->mVkPerProgramUBO != VK_NULL_HANDLE
                        && target_shader->mVkPerProgramUBOMapped != nullptr)
                    {
                        target_shader->rotatePerProgramUBOSlot();
                        char* base = (char*)target_shader->mVkActivePerProgramUBOMapped;
                        LLGLTFMaterial* gm = params.mGLTFMaterial;
                        F32 emissive_packed[4] = {
                            gm->mEmissiveColor.mV[0], gm->mEmissiveColor.mV[1], gm->mEmissiveColor.mV[2], 0.f,
                        };
                        memcpy(base + 0, emissive_packed, 16);
                    }

                    if (LLVKLoader::isVulkanInitialized()
                        && target_shader->mVkPerProgramUBO != VK_NULL_HANDLE
                        && target_shader->mVkPerProgramUBOMapped != nullptr
                        && target_shader->mVkPerProgramUBOSize >= 32)
                    {
                        target_shader->rotatePerProgramUBOSlot();
                        char* base = (char*)target_shader->mVkActivePerProgramUBOMapped;
                        LLGLTFMaterial* gm = params.mGLTFMaterial;

                        F32 mr_packed[8] = {
                            gm->mMetallicFactor, gm->mRoughnessFactor, 0.f, 0.f,
                            gm->mEmissiveColor.mV[0], gm->mEmissiveColor.mV[1], gm->mEmissiveColor.mV[2], 0.f,
                        };
                        memcpy(base + 0, mr_packed, 32);

                        bool has_shadow = target_shader->mFeatures.hasShadows;
                        U32 offset = 32;

                        if (!has_shadow)
                        {
                            F32 sun_moon[8] = {
                                gPipeline.mTransformedSunDir.mV[0],  gPipeline.mTransformedSunDir.mV[1],  gPipeline.mTransformedSunDir.mV[2],  0.f,
                                gPipeline.mTransformedMoonDir.mV[0], gPipeline.mTransformedMoonDir.mV[1], gPipeline.mTransformedMoonDir.mV[2], 0.f,
                            };
                            memcpy(base + offset, sun_moon, 32);
                            offset += 32;
                        }

                        F32 lp[LL_NUM_LIGHT_UNITS * 4];
                        F32 ld[LL_NUM_LIGHT_UNITS * 3];
                        F32 la[LL_NUM_LIGHT_UNITS * 4];
                        F32 ldi[LL_NUM_LIGHT_UNITS * 3];
                        F32 lda[LL_NUM_LIGHT_UNITS * 2];
                        gGL.getLightArrayData(lp, ld, la, ldi);
                        gGL.getLightDeferredAttenuationData(lda);

                        memcpy(base + offset, lp, sizeof(F32) * LL_NUM_LIGHT_UNITS * 4);
                        offset += 128;

                        for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; i++)
                        {
                            F32 v[4] = { ld[i*3+0], ld[i*3+1], ld[i*3+2], 0.f };
                            memcpy(base + offset + i * 16, v, 16);
                        }
                        offset += 128;

                        memcpy(base + offset, la, sizeof(F32) * LL_NUM_LIGHT_UNITS * 4);
                        offset += 128;

                        for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; i++)
                        {
                            F32 v[4] = { ldi[i*3+0], ldi[i*3+1], ldi[i*3+2], 0.f };
                            memcpy(base + offset + i * 16, v, 16);
                        }
                        offset += 128;

                        for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; i++)
                        {
                            F32 v[4] = { lda[i*2+0], lda[i*2+1], 0.f, 0.f };
                            memcpy(base + offset + i * 16, v, 16);
                        }

                    }
                }
                else
                {
                    mat = LLPipelineFrameContext::getInstance().isHUDPass() ? nullptr : params.mMaterial;

                    if (params.mFullbright)
                    {
                        // Turn off lighting if it hasn't already been so.
                        if (light_enabled || !initialized_lighting)
                        {
                            initialized_lighting = true;
                            target_shader = fullbright_shader;

                            light_enabled = false;
                        }
                    }
                    // Turn on lighting if it isn't already.
                    else if (!light_enabled || !initialized_lighting)
                    {
                        initialized_lighting = true;
                        target_shader = simple_shader;
                        light_enabled = true;
                    }

                    if (LLPipelineFrameContext::getInstance().isHUDPass())
                    {
                        target_shader = fullbright_shader;
                    }
                    else if (mat)
                    {
                        U32 mask = params.mShaderMask;

                        llassert(mask < LLMaterial::SHADER_COUNT);
                        target_shader = &(gDeferredMaterialProgram[mask]);
                    }
                    else if (!params.mFullbright)
                    {
                        target_shader = simple_shader;
                    }
                    else
                    {
                        target_shader = fullbright_shader;
                    }

                    if (params.mAvatar != nullptr)
                    {
                        llassert(target_shader->mRiggedVariant != nullptr);
                        target_shader = target_shader->mRiggedVariant;
                    }

                    if (current_shader != target_shader)
                    {// If we need shaders, and we're not ALREADY using the proper shader, then bind it
                    // (this way we won't rebind shaders unnecessarily).
                        flushAlphaRun(run);
                        alp_t = alp_now();
                        gPipeline.bindDeferredShaderFast(*target_shader);

                        if (params.mFullbright)
                        { // make sure the bind the exposure map for fullbright shaders so they can cancel out exposure
                            S32 channel = target_shader->enableTexture(LLShaderMgr::EXPOSURE_MAP);
                            if (channel > -1)
                            {
                                gGL.getTexUnit(channel)->bind(&gPipeline.mExposureMap);
                            }
                        }
                    }

                    LLVector4 spec_color(1, 1, 1, 1);
                    F32 env_intensity = 0.0f;
                    F32 brightness = 1.0f;

                    // We have a material.  Supply the appropriate data here.
                    if (mat)
                    {
                        spec_color = params.mSpecColor;
                        env_intensity = params.mEnvIntensity;
                        brightness = params.mFullbright ? 1.f : 0.f;
                    }

                    if (current_shader)
                    {
                        if (LLVKLoader::isVulkanInitialized())
                        {
                            const ptrdiff_t shader_index = current_shader - gDeferredMaterialProgram;
                            if (shader_index >= 0 && shader_index < (ptrdiff_t)(LLMaterial::SHADER_COUNT * 2))
                            {
                                const U32 layout_index = (U32)(shader_index % LLMaterial::SHADER_COUNT);
                                const U32 alpha_mode = (U32)(layout_index & 0x3);
                                if (alpha_mode == 1)
                                {
                                    writeMaterialFAllUBO(*current_shader, layout_index,
                                                         brightness, env_intensity,
                                                         spec_color.mV,
                                                         params.mAlphaMaskCutoff,
                                                         params.mIsSSSTarget ? 1.f : 0.f);
                                }
                                else
                                {
                                    writeMaterialFPerDrawUBO(*current_shader, layout_index,
                                                              brightness, env_intensity,
                                                              spec_color.mV,
                                                              params.mAlphaMaskCutoff,
                                                              params.mIsSSSTarget ? 1.f : 0.f);
                                }
                            }
                        }
                    }
                }

                { U64 t2 = alp_now(); alp_us[1] += t2 - alp_t; alp_t = t2; }

                if (params.mAvatar && !uploadMatrixPalette(params.mAvatar, params.mSkinInfo, lastAvatar, lastMeshId, lastAvatarShader, skipLastSkin))
                {
                    alp_us[2] += alp_now() - alp_t;
                    continue;
                }

                { U64 t2 = alp_now(); alp_us[2] += t2 - alp_t; alp_t = t2; }

                bool tex_setup = TexSetup(&params, (mat != nullptr));

                { U64 t2 = alp_now(); alp_us[3] += t2 - alp_t; alp_t = t2; }

                {
                    gGL.blendFunc((LLRender::eBlendFactor) params.mBlendFuncSrc, (LLRender::eBlendFactor) params.mBlendFuncDst, mAlphaSFactor, mAlphaDFactor);

                    bool reset_minimum_alpha = false;
                    if (!LLPipelineFrameContext::getInstance().isImpostorPass() &&
                        params.mBlendFuncDst != LLRender::BF_SOURCE_ALPHA &&
                        params.mBlendFuncSrc != LLRender::BF_SOURCE_ALPHA)
                    { // this draw call has a custom blend function that may require rendering of "invisible" fragments
                        current_shader->setMinimumAlpha(0.f);
                        reset_minimum_alpha = true;
                    }

                    const bool collapse_now = run_candidate
                        && current_shader != nullptr
                        && current_shader->mVkUsesBindlessHeap;

                    { U64 t2 = alp_now(); alp_us[4] += t2 - alp_t; alp_t = t2; }

                    if (collapse_now)
                    {
                        if (run.empty())
                        {
                            run.mShader = current_shader;
                            run.mModelMatrix = params.mModelMatrix;
                            writeAlphaPerProgramUBO(reset_minimum_alpha);
                        }
                        { U64 t2 = alp_now(); alp_us[5] += t2 - alp_t; alp_t = t2; }
                        LLRenderPass::buildAndOverrideScenePerDrawSet(&params, true);
                        { U64 t2 = alp_now(); alp_us[6] += t2 - alp_t; alp_t = t2; }
                        appendAlphaRunCmd(run, params);
                        ++LLVKLoader::gVkPerf.alp_col;
                        if (run.mCmds.size() >= ALPHA_RUN_MAX_CMDS)
                        {
                            flushAlphaRun(run);
                        }
                        alp_us[8] += alp_now() - alp_t;
                    }
                    else
                    {
                        flushAlphaRun(run);
                        alp_t = alp_now();
                        params.mVertexBuffer->setBuffer();

                        { U64 t2 = alp_now(); alp_us[7] += t2 - alp_t; alp_t = t2; }

                        writeAlphaPerProgramUBO(reset_minimum_alpha);

                        { U64 t2 = alp_now(); alp_us[5] += t2 - alp_t; alp_t = t2; }

                        LLVKContract::DrawScope vkc_scope(&params, "alpha");
                        LLRenderPass::buildAndOverrideScenePerDrawSet(&params, true);

                        { U64 t2 = alp_now(); alp_us[6] += t2 - alp_t; alp_t = t2; }

                        params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
                        ++LLVKLoader::gVkPerf.alp_inl;
                        alp_us[7] += alp_now() - alp_t;
                    }

                    if (reset_minimum_alpha)
                    {
                        current_shader->setMinimumAlpha(MINIMUM_ALPHA);
                    }
                }

                // If this alpha mesh has glow, then draw it a second time to add the destination-alpha (=glow).  Interleaving these state-changing calls is expensive, but glow must be drawn Z-sorted with alpha.
                if (getType() != LLDrawPool::POOL_ALPHA_PRE_WATER &&
                    params.mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_EMISSIVE))
                {
                    if (params.mAvatar != nullptr)
                    {
                        if (params.mGLTFMaterial.isNull())
                        {
                            rigged_emissives.push_back(&params);
                        }
                        else
                        {
                            pbr_rigged_emissives.push_back(&params);
                        }
                    }
                    else
                    {
                        if (params.mGLTFMaterial.isNull())
                        {
                            emissives.push_back(&params);
                        }
                        else
                        {
                            pbr_emissives.push_back(&params);
                        }
                    }
                }

                if (tex_setup)
                {
                    gGL.getTexUnit(0)->activate();
                    gGL.matrixMode(LLRender::MM_TEXTURE);
                    gGL.loadIdentity();
                    gGL.matrixMode(LLRender::MM_MODELVIEW);
                }
            }

            flushAlphaRun(run);

            // render emissive faces into alpha channel for bloom effects
            if (!depth_only)
            {
                U64 emi_t = alp_now();

                gPipeline.enableLightsDynamic();

                // <AYAstorm r30 P5 fix glow-lost-in-plate 2026-05-23>
                // Emissive pass uses (BF_ZERO, BF_ONE) for color (no-op) and
                // (BF_ONE, BF_ONE) for alpha (additive glow). When the BLEND
                // pass above was redirected to mAYAAlphaColor (plate), the
                // currently-bound FBO is the plate — leaving it bound here
                // makes emissive ADD glow into plate.a, which is the plate's
                // coverage channel used by dofCombineF's over-composite. That
                // both corrupts coverage AND prevents glow from ever reaching
                // mRT->screen.a (the scene glow channel combineGlow reads).
                // Net effect: alpha BLEND material loses its glow entirely.
                //
                // Fix: temporarily pop the plate so the emissive pass targets
                // mRT->screen, where glow accumulation belongs. flush() pops
                // the plate off LLRenderTarget's FBO stack, leaving the
                // pre-pushed screen on top; bindTarget() re-pushes the plate
                // after the emissive pass so subsequent BLEND in the second
                // forwardRender() call continues to write to the plate.
                const bool emissive_to_screen = mForwardToAlphaRT;
                if (emissive_to_screen)
                {
                    gPipeline.mAYAAlphaColor.flush();
                }
                // </AYAstorm r30 P5 fix glow-lost-in-plate>

                // install glow-accumulating blend mode
                // don't touch color, add to alpha (glow)
                gGL.blendFunc(LLRender::BF_ZERO, LLRender::BF_ONE, LLRender::BF_ONE, LLRender::BF_ONE);

                bool rebind = false;
                LLGLSLShader* lastShader = current_shader;
                if (!emissives.empty())
                {
                    light_enabled = true;
                    renderEmissives(emissives);
                    rebind = true;
                }

                if (!pbr_emissives.empty())
                {
                    light_enabled = true;
                    renderPbrEmissives(pbr_emissives);
                    rebind = true;
                }

                if (!rigged_emissives.empty())
                {
                    light_enabled = true;
                    renderRiggedEmissives(rigged_emissives);
                    rebind = true;
                }

                if (!pbr_rigged_emissives.empty())
                {
                    light_enabled = true;
                    renderRiggedPbrEmissives(pbr_rigged_emissives);
                    rebind = true;
                }

                // restore our alpha blend mode
                gGL.blendFunc(mColorSFactor, mColorDFactor, mAlphaSFactor, mAlphaDFactor);

                if (lastShader && rebind)
                {
                    lastShader->bind();
                }

                // <AYAstorm r30 P5 fix glow-lost-in-plate 2026-05-23>
                // Re-push plate so subsequent BLEND (second forwardRender call
                // or other consumers) continues writing into the plate.
                if (emissive_to_screen)
                {
                    gPipeline.mAYAAlphaColor.bindTarget();
                }
                // </AYAstorm r30 P5 fix glow-lost-in-plate>

                alp_us[10] += alp_now() - emi_t;
            }
        }
    }

    gGL.setSceneBlendType(LLRender::BT_ALPHA);

    LLVertexBuffer::unbind();

    if (alp_perf)
    {
        for (U32 i = 0; i < 11; ++i)
        {
            if (alp_us[i] != 0)
            {
                LLVKLoader::gVkPerf.alpha_us[i] += alp_us[i];
            }
        }
    }

    if (!light_enabled)
    {
        gPipeline.enableLightsDynamic();
    }
}

// <AYAstorm r30 P2> Motion blur / velocity pass (BD lineage, NiranV Dean,
// 995a1354d8). LGPL-2.1-only. Only POST_WATER emits velocity to avoid
// double-stamping mLastModelMatrix (BD comment). Alpha shares the
// gVelocityAlphaProgram with PBR alpha-mask.

S32 LLDrawPoolAlpha::getNumMotionBlurPasses()
{
    if (getType() == LLDrawPool::POOL_ALPHA_PRE_WATER)
        return 0;
    return 1;
}

void LLDrawPoolAlpha::beginMotionBlurPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED;
    gVelocityAlphaProgram.bind();
}

void LLDrawPoolAlpha::endMotionBlurPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED;
    gVelocityAlphaProgram.unbind();
}

void LLDrawPoolAlpha::renderMotionBlur(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED;
    LLGLEnable cull(GL_CULL_FACE);
    pushVelocityBatchesTextured(LLRenderPass::PASS_ALPHA);

    gVelocityAlphaProgram.bind(true);
    pushRiggedVelocityBatchesTextured(LLRenderPass::PASS_ALPHA_RIGGED);
}
// </AYAstorm r30 P2>

