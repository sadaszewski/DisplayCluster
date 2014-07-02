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

#include "FFMPEGMovie.h"

Movie::Movie(QString uri)
    : ffmpegMovie_(new FFMPEGMovie(uri))
    , uri_(uri)
    , paused_(false)
{}

Movie::~Movie()
{
    delete ffmpegMovie_;
}

void Movie::getDimensions(int &width, int &height) const
{
    width = ffmpegMovie_->getWidth();
    height = ffmpegMovie_->getHeight();
}

void Movie::nextFrame(const boost::posix_time::time_duration timeSinceLastFrame, const bool skipDecoding)
{
    if(paused_)
        return;

    ffmpegMovie_->update(timeSinceLastFrame, skipDecoding);

    if (ffmpegMovie_->isNewFrameAvailable())
        texture_.update(ffmpegMovie_->getData(), GL_RGBA);
}

bool Movie::generateTexture()
{
    QImage image(ffmpegMovie_->getWidth(), ffmpegMovie_->getHeight(), QImage::Format_RGB32);
    image.fill(0);

    return texture_.init(image);
}

void Movie::render(const QRectF& texCoords)
{
    if(!texture_.isValid() && !generateTexture())
        return;

    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);

    texture_.bind();

    quad_.setTexCoords(texCoords);
    quad_.render();

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
