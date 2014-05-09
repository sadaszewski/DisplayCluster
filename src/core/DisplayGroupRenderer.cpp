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

#include "DisplayGroupRenderer.h"

#include "DisplayGroupManager.h"
#include "ContentWindowManager.h"
#include "Marker.h"

DisplayGroupRenderer::DisplayGroupRenderer(DisplayGroupManagerPtr displayGroup,
                                           RenderContext& renderContext,
                                           FactoriesPtr factories)
    : factories_(factories)
    , displayGroup_(displayGroup)
    , markerRenderer_(renderContext)
{
}

void DisplayGroupRenderer::render()
{
    renderBackgroundContent(displayGroup_->getBackgroundContentWindow());
    renderContentWindows(displayGroup_->getContentWindowManagers());

    // Markers should be rendered last since they're blended
    renderMarkers(displayGroup_->getMarkers());
}

void DisplayGroupRenderer::setDisplayGroup(DisplayGroupManagerPtr displayGroup)
{
    displayGroup_ = displayGroup;
}

void DisplayGroupRenderer::renderBackgroundContent(ContentWindowManagerPtr backgroundContentWindow)
{
    // Render background content window
    if (backgroundContentWindow)
    {
        glPushMatrix();
        glTranslatef(0., 0., -1.f + std::numeric_limits<float>::epsilon());

        windowRenderer_.render(backgroundContentWindow, factories_);

        glPopMatrix();
    }
}

void DisplayGroupRenderer::renderContentWindows(ContentWindowManagerPtrs contentWindowManagers)
{
    const unsigned int windowCount = contentWindowManagers.size();
    unsigned int i = 0;
    for(ContentWindowManagerPtrs::iterator it = contentWindowManagers.begin(); it != contentWindowManagers.end(); ++it)
    {
        // It is currently not possible to cull windows that are invisible as this conflics
        // with the "garbage collection" mechanism for Contents. In fact, "stale" objects are objects
        // which have not been rendered for more than one frame (implicitly: objects without a window)
        // and those are destroyed by Factory::clearStaleObjects(). It is currently the only way to
        // remove a Content.
        //if ( isRegionVisible( (*it)->getCoordinates( )))
        {
            // the visible depths are in the range (-1,1); make the content window depths be in the range (-1,0)
            const float zCoordinate = -(float)(windowCount - i) / (float)(windowCount + 1);

            glPushMatrix();
            glTranslatef(0.f, 0.f, zCoordinate);

            windowRenderer_.render(*it, factories_);

            glPopMatrix();
        }

        ++i;
    }
}

void DisplayGroupRenderer::renderMarkers(const MarkerPtrs& markers)
{
    for(MarkerPtrs::const_iterator it = markers.begin(); it != markers.end(); ++it)
    {
        // only render recently active markers
        if((*it)->isActive())
            markerRenderer_.render(*it);
    }
}

