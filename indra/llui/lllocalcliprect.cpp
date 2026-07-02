/**
* @file lllocalcliprect.cpp
*
* $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#include "linden_common.h"

#include "lllocalcliprect.h"

#include "llfontgl.h"
#include "llui.h"
#include "llvkloader.h"

/*static*/ std::stack<LLRect> LLScreenClipRect::sClipRectStack;


LLScreenClipRect::LLScreenClipRect(const LLRect& rect, bool enabled)
:   mScissorState(GL_SCISSOR_TEST),
    mEnabled(enabled)
{
    if (mEnabled)
    {
        pushClipRect(rect);
        mScissorState.setEnabled(!sClipRectStack.empty());
        updateScissorRegion();
    }
}

LLScreenClipRect::~LLScreenClipRect()
{
    if (mEnabled)
    {
        popClipRect();
        updateScissorRegion();
    }
}

//static
void LLScreenClipRect::pushClipRect(const LLRect& rect)
{
    LLRect combined_clip_rect = rect;
    if (!sClipRectStack.empty())
    {
        LLRect top = sClipRectStack.top();
        combined_clip_rect.intersectWith(top);

        if(combined_clip_rect.isEmpty())
        {
            // avoid artifacts where zero area rects show up as lines
            combined_clip_rect = LLRect::null;
        }
    }
    sClipRectStack.push(combined_clip_rect);
}

//static
void LLScreenClipRect::popClipRect()
{
    sClipRectStack.pop();
}

//static
void LLScreenClipRect::updateScissorRegion()
{
    if (sClipRectStack.empty())
    {
        // clip stack 空 = clip 無し = GL は GL_SCISSOR_TEST 無効 (= mScissorState LLGLEnable の
        //   RAII restore が担う)。Vulkan は dynamic state ゆえ scissor tracker を明示的に無効化
        //   (= 次 per-draw setupViewportAndScissor が full render viewport に戻す) = GL の
        //   persistent scissor 無効と 1:1。未配備時は per-draw full set のまま漏れ続ける defect。
        if (LLVKLoader::isVulkanInitialized())
        {
            LLVKLoader::disableScissor();
        }
        return;
    }

    // finish any deferred calls in the old clipping region
    gGL.flush();

    LLRect rect = sClipRectStack.top();
    stop_glerror();
    S32 x,y,w,h;
    x = llfloor(rect.mLeft * LLUI::getScaleFactor().mV[VX]);
    y = llfloor(rect.mBottom * LLUI::getScaleFactor().mV[VY]);
    w = llmax(0, llceil(rect.getWidth() * LLUI::getScaleFactor().mV[VX])) + 1;
    h = llmax(0, llceil(rect.getHeight() * LLUI::getScaleFactor().mV[VY])) + 1;
    // OpenGL/Vulkan dual path scissor dispatch。
    // = Vulkan VkRect2D extent は inclusive-exclusive 半開区間 semantics ゆえ +1 padding 不要。
    //   GL glScissor 互換の +1 boundary 補正 (= 上の w/h) は GL path のみ、Vulkan path は raw w/h 使用。
    if (LLVKLoader::isVulkanInitialized())
    {
        S32 w_vk = llmax(0, llceil(rect.getWidth() * LLUI::getScaleFactor().mV[VX]));
        S32 h_vk = llmax(0, llceil(rect.getHeight() * LLUI::getScaleFactor().mV[VY]));
        LLVKLoader::setScissor(x, y, w_vk, h_vk);
    }
    else
    {
        glScissor( x,y,w,h );
    }
    stop_glerror();
}

//---------------------------------------------------------------------------
// LLLocalClipRect
//---------------------------------------------------------------------------
LLLocalClipRect::LLLocalClipRect(const LLRect& rect, bool enabled /* = true */)
:   LLScreenClipRect(LLRect(rect.mLeft + LLFontGL::sCurOrigin.mX,
                    rect.mTop + LLFontGL::sCurOrigin.mY,
                    rect.mRight + LLFontGL::sCurOrigin.mX,
                    rect.mBottom + LLFontGL::sCurOrigin.mY), enabled)
{}

LLLocalClipRect::~LLLocalClipRect()
{}
