/*********************************************************************/
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
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

#include "Texture.h"
#include "globals.h"
#include "log.h"
#include "RenderContext.h"
#include "GLWindow.h"

Texture::Texture(QString uri)
    : uri_( uri )
    , textureId_( 0 )
{
    const QImage image(uri_);
    if(image.isNull())
    {
        put_flog(LOG_ERROR, "error loading %s", uri_.toLocal8Bit().constData());
        return;
    }

    // save image dimensions
    imageWidth_ = image.width();
    imageHeight_ = image.height();
}

Texture::~Texture()
{
    if(textureId_)
        renderContext_->getGLWindow()->deleteTexture(textureId_);
}

void Texture::getDimensions(int &width, int &height) const
{
    width = imageWidth_;
    height = imageHeight_;
}

bool Texture::generateTexture()
{
    const QImage image(uri_);
    if(image.isNull())
    {
        put_flog(LOG_ERROR, "error loading %s", uri_.toLocal8Bit().constData());
        return false;
    }

    // generate new texture
    textureId_ = renderContext_->getGLWindow()->bindTexture(image, GL_TEXTURE_2D, GL_RGBA,
                                                          QGLContext::LinearFilteringBindOption |
                                                          QGLContext::MipmapBindOption);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return true;
}

void Texture::render(const QRectF& texCoords)
{
    if(!textureId_ && !generateTexture())
        return;

    // draw the texture
    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId_);

    glBegin(GL_QUADS);

    glTexCoord2f(texCoords.x(), texCoords.y());
    glVertex2f(0.,0.);

    glTexCoord2f(texCoords.x()+texCoords.width(), texCoords.y());
    glVertex2f(1.,0.);

    glTexCoord2f(texCoords.x()+texCoords.width(),texCoords.y()+texCoords.height());
    glVertex2f(1.,1.);

    glTexCoord2f(texCoords.x(),texCoords.y()+texCoords.height());
    glVertex2f(0.,1.);

    glEnd();

    glPopAttrib();
}
