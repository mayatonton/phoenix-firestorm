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
#include "llmenugl.h"
#include "llmultifloater.h"
#include "llviewercontrol.h"
#include "llviewershadermgr.h"
#include "llviewerwindow.h"

namespace
{
    constexpr S32 AUX_ANCHOR_X = 100000;
    constexpr S32 AUX_ANCHOR_Y = 100000;

    class FSAuxGateRoot : public LLView
    {
    public:
        FSAuxGateRoot(const LLView::Params& p) : LLView(p) {}
    };

    LLHandle<LLFloater>    sAuxExtHandle;
    LLRect                 sAuxRegion;
    LLAuxWindowHandlesSDL  sAuxHandles;
    bool                   sAuxDone  = false;
    bool                   sAuxShown = false;
}

static bool auxModeOn()
{
    static const bool s_env_force = (getenv("AYASTORM_AUX_WINDOW") != nullptr);
    if (s_env_force)
    {
        return true;
    }
    static bool s_latched = false;
    static bool s_mode    = false;
    if (!s_latched && LLStartUp::getStartupState() == STATE_STARTED)
    {
        s_mode    = gSavedSettings.getBOOL("AYAMultiWindowMode");
        s_latched = true;
    }
    return s_latched && s_mode;
}

bool FSAuxWindow::pointInAuxRegion(S32 x, S32 y)
{
    return !sAuxRegion.isEmpty() && sAuxRegion.pointInRect(x, y);
}

LLFloater* FSAuxWindow::auxRegionFloater(S32 x, S32 y)
{
    if (sAuxRegion.isEmpty() || !sAuxRegion.pointInRect(x, y))
    {
        return nullptr;
    }
    return sAuxExtHandle.get();
}

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

static void auxExternalizeTarget()
{
    LLFloater* aux_chat = auxTargetFloater();
    if (!aux_chat || !aux_chat->getRect().isValid())
    {
        return;
    }
    if (!aux_chat->isAuxExternalized()
        || aux_chat->getRect().mLeft != AUX_ANCHOR_X
        || aux_chat->getRect().mBottom != AUX_ANCHOR_Y)
    {
        if (!aux_chat->isAuxExternalized())
        {
            aux_chat->setAuxExternalized(true, aux_chat->getRect());
        }
        aux_chat->setOrigin(AUX_ANCHOR_X, AUX_ANCHOR_Y);
        sAuxExtHandle = aux_chat->getHandle();
    }
}

void FSAuxWindow::preDisplay()
{
    if (!auxModeOn() || sAuxDone || LLApp::isExiting())
    {
        return;
    }
    auxExternalizeTarget();
}

void FSAuxWindow::frame()
{
    if (!auxModeOn())
    {
        return;
    }

    LLAuxWindowHandlesSDL& s_aux_handles = sAuxHandles;
    bool& s_aux_done  = sAuxDone;
    bool& s_aux_shown = sAuxShown;

    static const bool s_query_registered = []() {
        LLMenuGL::sPopupConstraintQuery = []() -> LLRect {
            LLFloater* ext = sAuxExtHandle.get();
            if (ext && llAuxWindowHasFocusSDL(sAuxHandles))
            {
                return ext->calcScreenRect();
            }
            return LLRect();
        };
        return true;
    }();
    (void)s_query_registered;

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
        if (!(llCreateAuxWindowSDL(aux_chat->getTitle().c_str(), aux_w, aux_h, s_aux_handles, saved_x, saved_y, true)
              && LLVKLoader::auxWindowInitVk(s_aux_handles.native_display,
                                             s_aux_handles.native_window)))
        {
            llDestroyAuxWindowSDL(s_aux_handles);
            s_aux_done = true;
            if (LLFloater* ext = sAuxExtHandle.get())
            {
                const LLRect saved = ext->getAuxSavedRect();
                ext->setAuxExternalized(false);
                if (saved.isValid())
                {
                    ext->setOrigin(saved.mLeft, saved.mBottom);
                }
                sAuxExtHandle = LLHandle<LLFloater>();
            }
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

    const bool aux_active = LLVKLoader::auxWindowActiveVk();

    if (LLFloater* ext = sAuxExtHandle.get();
        ext && aux_active && ext != aux_chat)
    {
        const LLRect saved = ext->getAuxSavedRect();
        ext->setAuxExternalized(false);
        if (saved.isValid())
        {
            ext->setOrigin(saved.mLeft, saved.mBottom);
        }
        sAuxExtHandle = LLHandle<LLFloater>();
    }

    if (!aux_active)
    {
        sAuxRegion = LLRect();
        return;
    }

    auxExternalizeTarget();

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

    {
        int rw = 0, rh = 0;
        if (llAuxWindowTakeResizeSDL(s_aux_handles.sdl_window_id, rw, rh)
            && aux_rect_ok && gViewerWindow && rw > 0 && rh > 0)
        {
            const LLVector2& ds = gViewerWindow->getDisplayScale();
            const S32 fw = llmax(aux_chat->getMinWidth(), (S32)ll_round(rw / ds.mV[VX]));
            const S32 fh = llmax(aux_chat->getMinHeight(),
                                 (S32)ll_round(rh / ds.mV[VY]) + aux_chat->getHeaderHeight());
            LLRect r = aux_chat->getRect();
            if (fw != r.getWidth() || fh != r.getHeight())
            {
                r.mRight = r.mLeft + fw;
                r.mTop   = r.mBottom + fh;
                aux_chat->setShape(r, true);
                LLRect saved = aux_chat->getAuxSavedRect();
                if (saved.isValid())
                {
                    saved.mRight = saved.mLeft + fw;
                    saved.mTop   = saved.mBottom + fh;
                    aux_chat->setAuxExternalized(true, saved);
                }
            }
            LLVKLoader::auxWindowNotifyResizeVk();
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

    {
        int map_ox = 0, map_oy = 0;
        U32 map_w = 0, map_h = 0;
        bool map_on = false;
        if (target_shown && gViewerWindow && LLVKLoader::auxWindowExtentVk(map_w, map_h))
        {
            const LLVector2& ds = gViewerWindow->getDisplayScale();
            const LLRect sr = aux_chat->calcScreenRect();
            map_ox = ll_round(sr.mLeft * ds.mV[VX]);
            map_oy = ll_round(sr.mBottom * ds.mV[VY]);
            map_on = true;
        }
        llSetAuxWindowInputMapSDL(s_aux_handles, map_ox, map_oy, (int)map_h, map_on);
        sAuxRegion = map_on ? aux_chat->calcScreenRect() : LLRect();
    }

    if (llAuxWindowCloseRequestedSDL(s_aux_handles.sdl_window_id))
    {
        LLVKLoader::auxWindowShutdownVk();
        llDestroyAuxWindowSDL(s_aux_handles);
        s_aux_shown = false;
        sAuxRegion = LLRect();
        if (aux_chat)
        {
            aux_chat->closeFloater();
        }
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
            LLView::sDirtyRect = LLRect(0, AUX_ANCHOR_Y * 4, AUX_ANCHOR_X * 4, 0);

            static FSAuxGateRoot* s_gate_root = nullptr;
            if (s_gate_root == nullptr)
            {
                LLView::Params p;
                p.name = "aux_gate_root";
                p.rect = LLRect(0, AUX_ANCHOR_Y * 4, AUX_ANCHOR_X * 4, 0);
                s_gate_root = new FSAuxGateRoot(p);
            }
            LLView* aux_saved_root = LLUI::getInstance()->getRootView();
            LLUI::getInstance()->setRootView(s_gate_root);

            gUIProgram.bind();
            gGL.color4f(1.f, 1.f, 1.f, 1.f);
            gGL.pushMatrix();
            LLUI::pushMatrix();
            LLUI::loadIdentity();
            const LLVector2 aux_ui_scale = LLUI::getScaleFactor();
            gGL.scaleUI(aux_ui_scale.mV[VX], aux_ui_scale.mV[VY], 1.f);
            aux_chat->draw();
            if (LLMenuHolderGL* holder = LLMenuGL::sMenuContainer)
            {
                const LLRect fsr = aux_chat->calcScreenRect();
                LLMenuGL::sAuxDrawPass = true;
                for (LLView::child_list_const_reverse_iter_t it = holder->getChildList()->rbegin();
                     it != holder->getChildList()->rend(); ++it)
                {
                    LLView* menu = *it;
                    if (menu && menu->getVisible() && menu->getRect().isValid()
                        && LLMenuGL::isAuxOwnedTree(menu))
                    {
                        LLUI::pushMatrix();
                        LLUI::translate((F32)(menu->getRect().mLeft - fsr.mLeft),
                                        (F32)(menu->getRect().mBottom - fsr.mBottom));
                        menu->draw();
                        LLUI::popMatrix();
                    }
                }
                LLMenuGL::sAuxDrawPass = false;
            }
            gGL.flush();
            LLUI::popMatrix();
            gGL.popMatrix();
            gUIProgram.unbind();

            LLUI::getInstance()->setRootView(aux_saved_root);
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

void FSAuxWindow::preDisplay()
{
}

void FSAuxWindow::frame()
{
}

bool FSAuxWindow::pointInAuxRegion(S32, S32)
{
    return false;
}

LLFloater* FSAuxWindow::auxRegionFloater(S32, S32)
{
    return nullptr;
}

#endif
