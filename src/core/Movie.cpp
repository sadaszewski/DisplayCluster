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

#include "Movie.h"
#include "globals.h"
#include "MPIChannel.h"
#include "RenderContext.h"
#include "GLWindow.h"
#include "FFMPEGMovie.h"
#include "log.h"

Movie::Movie(QString uri)
    : ffmpegMovie_(new FFMPEGMovie(uri))
    , uri_(uri)
    , textureId_(0)
    , paused_(false)
{}

Movie::~Movie()
{
    delete ffmpegMovie_;

    if(textureId_)
        renderContext_->getGLWindow()->deleteTexture(textureId_);
}

void Movie::getDimensions(int &width, int &height) const
{
    width = ffmpegMovie_->getWidth();
    height = ffmpegMovie_->getHeight();
}

void Movie::nextFrame(const boost::posix_time::time_duration timeSinceLastFrame, const bool skip)
{
    if( paused_ /*&& !skipped_frames_*/ )
        return;

    ffmpegMovie_->update(timeSinceLastFrame, skip);

    if (skip)
        return;

    updateTexture();
}

bool Movie::generateTexture()
{
    // create texture for movie
    QImage image(ffmpegMovie_->getWidth(), ffmpegMovie_->getHeight(), QImage::Format_RGB32);
    image.fill(0);

    textureId_ = renderContext_->getGLWindow()->bindTexture(image, GL_TEXTURE_2D, GL_RGBA, QGLContext::LinearFilteringBindOption);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return true;
}

void Movie::updateTexture()
{
    // put the RGB image to the already-created texture
    // glTexSubImage2D uses the existing texture and is more efficient than other means
    glBindTexture(GL_TEXTURE_2D, textureId_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ffmpegMovie_->getWidth(), ffmpegMovie_->getHeight(),
                    GL_RGBA, GL_UNSIGNED_BYTE, ffmpegMovie_->getData());
}

void Movie::render(const QRectF& texCoords)
{
    if(!textureId_ && !generateTexture())
        return;

    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId_);

    glBegin(GL_QUADS);

    glTexCoord2f(texCoords.x(), texCoords.y());
    glVertex2f(0., 0.);

    glTexCoord2f(texCoords.x() + texCoords.width(), texCoords.y());
    glVertex2f(1., 0.);

    glTexCoord2f(texCoords.x() + texCoords.width(), texCoords.y() + texCoords.height());
    glVertex2f(1., 1.);

    glTexCoord2f(texCoords.x(), texCoords.y() + texCoords.height());
    glVertex2f(0., 1.);

    glEnd();

    glPopAttrib();
}

void Movie::setPause(const bool pause)
{
    paused_ = pause;
}

void Movie::setLoop(const bool loop)
{
    ffmpegMovie_->setLoop(loop);
}
