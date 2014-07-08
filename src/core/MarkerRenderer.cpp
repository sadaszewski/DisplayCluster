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

#include "MarkerRenderer.h"

#include "Marker.h"

#include "globals.h"
#include "configuration/Configuration.h"

#include "log.h"

#define MARKER_IMAGE_FILENAME ":/img/marker.png"

// this is a fraction of the tiled display width of 1
#define MARKER_WIDTH 0.005

MarkerRenderer::MarkerRenderer()
{
}

void MarkerRenderer::render(MarkerPtr marker)
{
    if (!texture_.isValid() && !generateTexture())
        return;

    float x, y;
    marker->getPosition(x, y);

    // marker height needs to be scaled by the tiled display aspect ratio
    const float tiledDisplayAspect = g_configuration->getAspectRatio();
    const float markerHeight = MARKER_WIDTH * tiledDisplayAspect;

    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPushMatrix();

    glTranslatef(x, y, 0);
    glScalef(MARKER_WIDTH, markerHeight, 1.f);
    glTranslatef(-0.5f*MARKER_WIDTH, -0.5f*markerHeight, 0); // Center unit quad

    texture_.bind();
    quad_.render();

    glPopMatrix();

    glPopAttrib();
}

bool MarkerRenderer::generateTexture()
{
    const QImage image(MARKER_IMAGE_FILENAME);

    if(image.isNull())
    {
        put_flog(LOG_ERROR, "error loading marker texture '%s'", MARKER_IMAGE_FILENAME);
        return false;
    }

    return texture_.init(image, GL_BGRA);
}
