/**
* @file fsauxwindow.h
* @brief AYAstorm multiwindow aux window frame driver
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

#ifndef FS_AUXWINDOW_H
#define FS_AUXWINDOW_H

#include "stdtypes.h"

class LLFloater;

namespace FSAuxWindow
{
    void preDisplay();
    void frame();
    bool pointInAuxRegion(S32 x, S32 y);
    LLFloater* auxRegionFloater(S32 x, S32 y);
}

#endif
