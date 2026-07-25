/**
* @file fsauxwindow.cpp
* @brief AYAstorm multiwindow aux window frame driver (create / close / UI subtree draw)
*
* $LicenseInfo:firstyear=2026&license=viewerlgpl$
* AYAstorm Viewer Source Code
* Copyright (C) 2025-2026 Ishikawa AYA (github: mayatonton, mayatonton1994@gmail.com)
*
* このファイルは Ishikawa AYA が新規に作成した独自著作物である。
* 著作権は Ishikawa AYA が保持し、パブリックドメインには置かない。
* All rights reserved by the author except as licensed below.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
* $/LicenseInfo$
*
* 光の国のひとたちと共にわたしはここにいる　彩
*/

#include "llviewerprecompiledheaders.h"

#include "fsauxwindow.h"

#if LL_SDL2

#include "llapp.h"
#include "llglstates.h"
#include "llrender.h"
#include "llrect.h"
#include "llstartup.h"
#include "llui.h"
#include "llview.h"
#include "llvkloader.h"
#include "llwindowsdl2.h"
#include "fsfloaternearbychat.h"
#include "llfloaterimnearbychat.h"
#include "llfloaterreg.h"
#include "llmultifloater.h"
#include "llviewercontrol.h"
#include "llviewershadermgr.h"
#include "llviewerwindow.h"

static LLFloater* auxTargetFloater()
{
    LLFloater* target = ayastorm_is_ll_style()
        ? static_cast<LLFloater*>(LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat"))
        : static_cast<LLFloater*>(FSFloaterNearbyChat::findInstance());
    if (target && target->getHost())
    {
        target = target->getHost();
    }
    return target;
}

void FSAuxWindow::frame()
{
    static const bool s_aux_window = (getenv("AYASTORM_AUX_WINDOW") != nullptr);
    if (!s_aux_window)
    {
        return;
    }

    static LLAuxWindowHandlesSDL s_aux_handles;
    static bool s_aux_done = false;
    static bool s_aux_shown = false;

    LLFloater* aux_chat = auxTargetFloater();
    const bool aux_rect_ok = aux_chat && aux_chat->getRect().isValid();
    const bool target_shown = aux_rect_ok && aux_chat->getVisible() && !aux_chat->isMinimized();

    if (!LLVKLoader::auxWindowActiveVk() && !s_aux_done && !LLApp::isExiting()
        && LLVKLoader::isVulkanInitialized()
        && LLStartUp::getStartupState() == STATE_STARTED
        && target_shown && gViewerWindow)
    {
        const LLVector2& ds = gViewerWindow->getDisplayScale();
        const S32 aux_w = ll_round(aux_chat->getRect().getWidth() * ds.mV[VX]);
        const S32 aux_h = ll_round((aux_chat->getRect().getHeight() - aux_chat->getHeaderHeight()) * ds.mV[VY]);
        const S32 saved_x = gSavedSettings.getS32("AYAAuxWindowPosX");
        const S32 saved_y = gSavedSettings.getS32("AYAAuxWindowPosY");
        if (!(llCreateAuxWindowSDL(aux_chat->getTitle().c_str(), aux_w, aux_h, s_aux_handles, saved_x, saved_y)
              && LLVKLoader::auxWindowInitVk(s_aux_handles.native_display,
                                             s_aux_handles.native_window)))
        {
            llDestroyAuxWindowSDL(s_aux_handles);
            s_aux_done = true;
        }
        else
        {
            s_aux_shown = true;
            if (gViewerWindow->getWindow())
            {
                gViewerWindow->getWindow()->bringToFront();
            }
        }
    }

    if (!LLVKLoader::auxWindowActiveVk())
    {
        return;
    }

    if (target_shown != s_aux_shown)
    {
        llSetAuxWindowVisibleSDL(s_aux_handles, target_shown);
        s_aux_shown = target_shown;
    }

    if (aux_chat)
    {
        static std::string s_aux_title;
        const std::string title = aux_chat->getTitle();
        if (title != s_aux_title)
        {
            llSetAuxWindowTitleSDL(s_aux_handles, title.c_str());
            s_aux_title = title;
        }
    }

    if (s_aux_shown)
    {
        int wx = 0, wy = 0;
        if (llGetAuxWindowPositionSDL(s_aux_handles, wx, wy))
        {
            if (wx != gSavedSettings.getS32("AYAAuxWindowPosX")
                || wy != gSavedSettings.getS32("AYAAuxWindowPosY"))
            {
                gSavedSettings.setS32("AYAAuxWindowPosX", wx);
                gSavedSettings.setS32("AYAAuxWindowPosY", wy);
            }
        }
    }

    if (llAuxWindowCloseRequestedSDL(s_aux_handles.sdl_window_id))
    {
        LLVKLoader::auxWindowShutdownVk();
        llDestroyAuxWindowSDL(s_aux_handles);
        s_aux_done = true;
    }
    else if (target_shown && LLVKLoader::auxWindowBeginUIFrameVk())
    {
        U32 aw_w = 0, aw_h = 0;
        LLVKLoader::auxWindowExtentVk(aw_w, aw_h);
        if (aux_rect_ok)
        {
            LLGLSUIDefault gls_ui;
            gGL.matrixMode(LLRender::MM_PROJECTION);
            gGL.pushMatrix();
            gGL.loadIdentity();
            gGL.ortho(0.0f, (F32)aw_w, 0.0f, (F32)aw_h, -1.0f, 1.0f);
            gGL.matrixMode(LLRender::MM_MODELVIEW);
            gGL.pushMatrix();
            gGL.loadIdentity();

            const LLRect aux_saved_dirty = LLView::sDirtyRect;
            LLView::sDirtyRect = aux_chat->calcScreenRect();

            gUIProgram.bind();
            gGL.color4f(1.f, 1.f, 1.f, 1.f);
            gGL.pushMatrix();
            LLUI::pushMatrix();
            LLUI::loadIdentity();
            const LLVector2 aux_ui_scale = LLUI::getScaleFactor();
            gGL.scaleUI(aux_ui_scale.mV[VX], aux_ui_scale.mV[VY], 1.f);
            aux_chat->draw();
            gGL.flush();
            LLUI::popMatrix();
            gGL.popMatrix();
            gUIProgram.unbind();

            LLView::sDirtyRect = aux_saved_dirty;

            gGL.matrixMode(LLRender::MM_MODELVIEW);
            gGL.popMatrix();
            gGL.matrixMode(LLRender::MM_PROJECTION);
            gGL.popMatrix();
            gGL.matrixMode(LLRender::MM_MODELVIEW);
        }
        LLVKLoader::auxWindowEndUIFrameVk();
    }
}

#else

void FSAuxWindow::frame()
{
}

#endif
