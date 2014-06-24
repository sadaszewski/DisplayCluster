/*********************************************************************/
/* Copyright (c) 2013, EPFL/Blue Brain Project                       */
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

#include "PixelStreamDispatcher.h"
#include "PixelStreamWindowManager.h"
#include "PixelStreamFrame.h"

#define DISPATCH_FREQUENCY 100

#define STREAM_WINDOW_DEFAULT_SIZE 100

PixelStreamDispatcher::PixelStreamDispatcher(PixelStreamWindowManager& windowManager)
    : windowManager_(windowManager)
{
#ifdef USE_TIMER
    connect(&sendTimer_, SIGNAL(timeout()), this, SLOT(dispatchFrames()));
    sendTimer_.start(1000/DISPATCH_FREQUENCY);
#else
    lastFrameSent_ = boost::posix_time::microsec_clock::universal_time();
    // Not using a queued connection here causes the rendering to lag behind and the main UI to freeze..
    connect(this, SIGNAL(dispatchFramesSignal()), this, SLOT(dispatchFrames()), Qt::QueuedConnection);
#endif

    // Connect with the DisplayGroupManager
    connect(this, SIGNAL(openPixelStream(QString, QSize)), &windowManager, SLOT(openPixelStreamWindow(QString, QSize)));
    connect(this, SIGNAL(deletePixelStream(QString)), &windowManager, SLOT(closePixelStreamWindow(QString)));
    connect(&windowManager, SIGNAL(pixelStreamWindowClosed(QString)), this, SLOT(deleteStream(QString)));
}

void PixelStreamDispatcher::addSource(const QString uri, const size_t sourceIndex)
{
    streamBuffers_[uri].addSource(sourceIndex);
}

void PixelStreamDispatcher::removeSource(const QString uri, const size_t sourceIndex)
{
    if(!streamBuffers_.count(uri))
        return;

    streamBuffers_[uri].removeSource(sourceIndex);

    if (streamBuffers_[uri].getSourceCount() == 0)
    {
        deleteStream(uri);
    }
}

void PixelStreamDispatcher::processSegment(const QString uri, const size_t sourceIndex, dc::PixelStreamSegment segment)
{
    if (streamBuffers_.count(uri))
        streamBuffers_[uri].insertSegment(segment, sourceIndex);
}

void PixelStreamDispatcher::processFrameFinished(const QString uri, const size_t sourceIndex)
{
    if (!streamBuffers_.count(uri))
        return;

    streamBuffers_[uri].finishFrameForSource(sourceIndex);

    // When the first frame is complete, notify that the stream is now open
    if (streamBuffers_[uri].isFirstFrame() && streamBuffers_[uri].hasFrameComplete())
    {
        QSize size = streamBuffers_[uri].getFrameSize();
        emit openPixelStream(uri, size);
    }

#ifdef USE_TIMER
#else
    const boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
    if ((now - lastFrameSent_).total_milliseconds() > 1000/DISPATCH_FREQUENCY)
    {
        lastFrameSent_ = now;
        //dispatchFrames(); // See comment above about direct Signal connection..
        emit dispatchFramesSignal();
    }
#endif
}

void PixelStreamDispatcher::deleteStream(const QString uri)
{
    if (streamBuffers_.count(uri))
    {
        streamBuffers_.erase(uri);
        emit deletePixelStream(uri);
    }
}

void PixelStreamDispatcher::dispatchFrames()
{
    for (StreamBuffers::iterator it = streamBuffers_.begin(); it != streamBuffers_.end(); ++it)
    {
        PixelStreamFramePtr frame(new PixelStreamFrame);
        frame->uri = it->first;

        // Only dispatch the lastest frame
        while (it->second.hasFrameComplete())
        {
            frame->segments = it->second.getFrame();
        }
        if (!frame->segments.empty())
        {
            QSize size = it->second.computeFrameDimensions(frame->segments);
            windowManager_.updateDimension(frame->uri, size);

            emit sendFrame(frame);
        }
    }
}
