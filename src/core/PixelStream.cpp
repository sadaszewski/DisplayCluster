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

#include "PixelStream.h"
#include "globals.h"
#include "ContentWindowManager.h"
#include "configuration/Configuration.h"
#include "Options.h"
#include "MPIChannel.h"
#include "MainWindow.h"
#include "GLWindow.h"
#include "log.h"

#include "PixelStreamSegmentRenderer.h"
#include "PixelStreamSegmentDecoder.h"

#include "PixelStreamSegmentParameters.h"

PixelStream::PixelStream(const QString &uri)
    : uri_(uri)
    , width_(0)
    , height_ (0)
    , buffersSwapped_(false)
{
}

void PixelStream::getDimensions(int &width, int &height) const
{
    width = width_;
    height = height_;
}

void PixelStream::preRenderUpdate(const QRectF& windowRect)
{
    if( isDecodingInProgress( ))
        return;

    // After swapping the buffers, wait until decoding has finished to update the renderers.
    if ( buffersSwapped_ )
    {
        adjustSegmentRendererCount(frontBuffer_.size());
        updateRenderers(frontBuffer_);
        recomputeDimensions(frontBuffer_);
        buffersSwapped_ = false;
    }

    // The window may have moved, so always check if some segments have become visible to upload them.
    updateVisibleTextures(windowRect);

    if ( !backBuffer_.empty( ))
    {
        swapBuffers();
        adjustFrameDecodersCount(frontBuffer_.size());
    }

    // The window may have moved, so always check if some segments have become visible to decode them.
    decodeVisibleTextures(windowRect);
}

void PixelStream::updateRenderers(const PixelStreamSegments& segments)
{
    assert(segmentRenderers_.size() == segments.size());

    for(size_t i=0; i<segments.size(); i++)
    {
        // The parameters always need to be up to date to determine visibility when rendering.
        segmentRenderers_[i]->setParameters(segments[i].parameters.x, segments[i].parameters.y,
                                            segments[i].parameters.width, segments[i].parameters.height);
        segmentRenderers_[i]->setTextureNeedsUpdate();
    }
}

void PixelStream::updateVisibleTextures(const QRectF& windowRect)
{
    for(size_t i=0; i<frontBuffer_.size(); i++)
    {
        if (segmentRenderers_[i]->textureNeedsUpdate() && !frontBuffer_[i].parameters.compressed &&
                isVisible(frontBuffer_[i], windowRect))
        {
            const QImage textureWrapper((const uchar*)frontBuffer_[i].imageData.constData(),
                                        frontBuffer_[i].parameters.width,
                                        frontBuffer_[i].parameters.height,
                                        QImage::Format_RGB32);

            segmentRenderers_[i]->updateTexture(textureWrapper);
        }
    }
}

void PixelStream::swapBuffers()
{
    assert(!backBuffer_.empty());

    frontBuffer_ = backBuffer_;
    backBuffer_.clear();

    buffersSwapped_ = true;
}

void PixelStream::recomputeDimensions(const PixelStreamSegments &segments)
{
    width_ = 0;
    height_ = 0;

    for(size_t i=0; i<segments.size(); i++)
    {
        const PixelStreamSegmentParameters& params = segments[i].parameters;
        width_ = std::max(width_, params.width+params.x);
        height_ = std::max(height_, params.height+params.y);
    }
}

void PixelStream::decodeVisibleTextures(const QRectF& windowRect)
{
    assert(frameDecoders_.size() == frontBuffer_.size());

    std::vector<PixelStreamSegmentDecoderPtr>::iterator frameDecoder_it = frameDecoders_.begin();
    PixelStreamSegments::iterator segment_it = frontBuffer_.begin();
    for ( ; segment_it != frontBuffer_.end(); ++segment_it, ++frameDecoder_it )
    {
        if ( segment_it->parameters.compressed && isVisible(*segment_it, windowRect) )
        {
            (*frameDecoder_it)->startDecoding(*segment_it);
        }
    }
}

void PixelStream::render(const QRectF&, const QRectF& windowRect)
{
    updateRenderedFrameIndex();

    const bool showSegmentBorders = g_configuration->getOptions()->getShowStreamingSegments();
    const bool showSegmentStatistics = g_configuration->getOptions()->getShowStreamingStatistics();

    glPushMatrix();
    glScalef(1.f/(float)width_, 1.f/(float)height_, 0.f);

    for(std::vector<PixelStreamSegmentRendererPtr>::iterator it=segmentRenderers_.begin(); it != segmentRenderers_.end(); ++it)
    {
        if (isVisible( (*it)->getRect(), windowRect ))
        {
            (*it)->render(showSegmentBorders, showSegmentStatistics);
        }
    }

    glPopMatrix();
}

void PixelStream::adjustFrameDecodersCount(const size_t count)
{
    // We need to insert NEW objects in the vector if it is smaller
    for (size_t i=frameDecoders_.size(); i<count; ++i)
        frameDecoders_.push_back( PixelStreamSegmentDecoderPtr(new PixelStreamSegmentDecoder()) );
    // Or resize it if it is bigger
    frameDecoders_.resize( count );
}

void PixelStream::adjustSegmentRendererCount(const size_t count)
{
    // Recreate the renderers if the number of segments has changed
    if (segmentRenderers_.size() != count)
    {
        segmentRenderers_.clear();
        for (size_t i=0; i<count; ++i)
            segmentRenderers_.push_back( PixelStreamSegmentRendererPtr(new PixelStreamSegmentRenderer(uri_)) );
    }
}

void PixelStream::insertNewFrame(const PixelStreamSegments &segments)
{
    backBuffer_ = segments;
}

bool PixelStream::isDecodingInProgress()
{
    // determine if threads are running on any processes for this PixelStream
    int localThreadsRunning = 0;

    std::vector<PixelStreamSegmentDecoderPtr>::const_iterator it;
    for (it = frameDecoders_.begin(); it != frameDecoders_.end(); ++it)
    {
        if ((*it)->isRunning())
            ++localThreadsRunning;
    }

    int globalThreadsRunning = g_mpiChannel->globalSum(localThreadsRunning);
    return globalThreadsRunning > 0;
}

bool PixelStream::isVisible(const QRect& segment, const QRectF& windowRect)
{
    // coordinates of segment in global tiled display space
    const double segmentX = windowRect.x() + (double)segment.x() / (double)width_ * windowRect.width();
    const double segmentY = windowRect.y() + (double)segment.y() / (double)height_ * windowRect.height();
    const double segmentW = (double)segment.width() / (double)width_ * windowRect.width();
    const double segmentH = (double)segment.height() / (double)height_ * windowRect.height();

    return g_mainWindow->isRegionVisible(QRectF(segmentX, segmentY, segmentW, segmentH));
}

bool PixelStream::isVisible(const dc::PixelStreamSegment& segment, const QRectF& windowRect)
{
    QRect segmentRegion(segment.parameters.x, segment.parameters.y,
                        segment.parameters.width, segment.parameters.height);
    return isVisible(segmentRegion, windowRect);
}

