/*********************************************************************/
/* Copyright (c) 2014, EPFL/Blue Brain Project                       */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#ifndef DISPLAYGROUPRENDERER_H
#define DISPLAYGROUPRENDERER_H

#include "types.h"

#include "ContentWindowRenderer.h"
#include "MarkerRenderer.h"
#include "Renderable.h"

#if ENABLE_SKELETON_SUPPORT
#include "SkeletonRenderer.h"
#endif

/**
 * Renders a DisplayGroup.
 */
class DisplayGroupRenderer : public Renderable
{
public:
    /** Constructor */
    DisplayGroupRenderer(FactoriesPtr factories);

    /**
     * Render the associated DisplayGroup.
     * @see setDisplayGroup()
     */
    void render() override;

    /**
     * Set the DisplayGroup to render, replacing the previous one.
     * @see render()
     */
    void setDisplayGroup(DisplayGroupManagerPtr displayGroup);

private:
    FactoriesPtr factories_;
    DisplayGroupManagerPtr displayGroup_;
    ContentWindowRenderer windowRenderer_;
    MarkerRenderer markerRenderer_;
#if ENABLE_SKELETON_SUPPORT
    SkeletonRenderer skeletonRenderer_;
#endif

    void renderBackgroundContent(ContentWindowManagerPtr backgroundContentWindow);
    void renderContentWindows(ContentWindowManagerPtrs contentWindowManagers);
    void renderMarkers(const MarkerPtrs& markers);
};

#endif // DISPLAYGROUPRENDERER_H
