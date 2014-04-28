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

#include "DisplayGroupManager.h"

#include "ContentWindowManager.h"
#include "Content.h"

#include "globals.h"
#include "MPIChannel.h"

DisplayGroupManager::DisplayGroupManager()
{
    // register types for use in signals/slots
    qRegisterMetaType<Event>("Event");
    qRegisterMetaType<ContentWindowManagerPtr>("ContentWindowManagerPtr");
    qRegisterMetaType<PixelStreamSegment>("PixelStreamSegment");

#if ENABLE_SKELETON_SUPPORT
    qRegisterMetaType<std::vector< boost::shared_ptr<SkeletonState> > >("std::vector< boost::shared_ptr<SkeletonState> >");
#endif
}

DisplayGroupManager::~DisplayGroupManager()
{
}

MarkerPtr DisplayGroupManager::getNewMarker()
{
    QMutexLocker locker(&markersMutex_);

    MarkerPtr marker(new Marker());
    markers_.push_back(marker);

    // the marker needs to be owned by the main thread for queued connections to work properly
    marker->moveToThread(QApplication::instance()->thread());

    // make marker trigger sendDisplayGroup() when it is updated
    connect(marker.get(), SIGNAL(positionChanged()), this, SLOT(sendDisplayGroup()), Qt::QueuedConnection);

    return marker;
}

MarkerPtrs DisplayGroupManager::getMarkers() const
{
    QMutexLocker locker(&markersMutex_);
    return markers_;
}

void DisplayGroupManager::deleteMarkers()
{
    QMutexLocker locker(&markersMutex_);

    if( markers_.empty( ))
        return;

    markers_[0]->releaseTexture();
    markers_.clear();
}

#if ENABLE_SKELETON_SUPPORT
std::vector<boost::shared_ptr<SkeletonState> > DisplayGroupManager::getSkeletons()
{
    return skeletons_;
}
#endif

void DisplayGroupManager::addContentWindowManager(ContentWindowManagerPtr contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::addContentWindowManager(contentWindowManager, source);

    if(source != this)
    {
        contentWindowManager->setDisplayGroupManager(shared_from_this());
        watchChanges(contentWindowManager);

        emit modified(shared_from_this());

        if (contentWindowManager->getContent()->getType() != CONTENT_TYPE_PIXEL_STREAM)
        {
            // TODO initialize all content dimensions on creation so we can
            // remove this procedure (DISCL-21)
            // make sure we have its dimensions so we can constrain its aspect ratio
            g_mpiChannel->sendContentsDimensionsRequest(getContentWindowManagers());
        }
    }
}

void DisplayGroupManager::watchChanges(ContentWindowManagerPtr contentWindow)
{
    // Don't call sendDisplayGroup() on movedToFront() or destroyed() since it happens already
    connect(contentWindow.get(), SIGNAL(contentDimensionsChanged(int, int, ContentWindowInterface *)),
            this, SLOT(sendDisplayGroup()));
    connect(contentWindow.get(), SIGNAL(coordinatesChanged(QRectF, ContentWindowInterface *)),
            this, SLOT(sendDisplayGroup()));
    connect(contentWindow.get(), SIGNAL(positionChanged(double, double, ContentWindowInterface *)),
            this, SLOT(sendDisplayGroup()));
    connect(contentWindow.get(), SIGNAL(sizeChanged(double, double, ContentWindowInterface *)),
            this, SLOT(sendDisplayGroup()));
    connect(contentWindow.get(), SIGNAL(centerChanged(double, double, ContentWindowInterface *)),
            this, SLOT(sendDisplayGroup()));
    connect(contentWindow.get(), SIGNAL(zoomChanged(double, ContentWindowInterface *)),
            this, SLOT(sendDisplayGroup()));
    connect(contentWindow.get(), SIGNAL(windowStateChanged(ContentWindowInterface::WindowState, ContentWindowInterface *)),
            this, SLOT(sendDisplayGroup()));
    connect(contentWindow.get(), SIGNAL(contentModified()),
            this, SLOT(sendDisplayGroup()));
}

void DisplayGroupManager::removeContentWindowManager(ContentWindowManagerPtr contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::removeContentWindowManager(contentWindowManager, source);

    if(source != this)
    {
        // disconnect any existing connections with the window
        disconnect(contentWindowManager.get(), 0, this, 0);

        // set null display group in content window manager object
        contentWindowManager->setDisplayGroupManager(DisplayGroupManagerPtr());

        emit modified(shared_from_this());
    }
}

void DisplayGroupManager::moveContentWindowManagerToFront(ContentWindowManagerPtr contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::moveContentWindowManagerToFront(contentWindowManager, source);

    if(source != this)
    {
        emit modified(shared_from_this());
    }
}

void DisplayGroupManager::setBackgroundContent(ContentPtr content)
{
    if (content)
    {
        backgroundContent_ = ContentWindowManagerPtr(new ContentWindowManager(content));
        // set display group in content window manager object
        backgroundContent_->setDisplayGroupManager(shared_from_this());
        backgroundContent_->adjustSize( SIZE_FULLSCREEN );
        watchChanges(backgroundContent_);
    }
    else
    {
        backgroundContent_ = ContentWindowManagerPtr();
    }

    emit modified(shared_from_this());
}

ContentWindowManagerPtr DisplayGroupManager::getBackgroundContentWindowManager() const
{
    return backgroundContent_;
}

bool DisplayGroupManager::isEmpty() const
{
    return contentWindowManagers_.empty();
}

ContentWindowManagerPtr DisplayGroupManager::getActiveWindow() const
{
    if (isEmpty())
        return ContentWindowManagerPtr();

    return contentWindowManagers_.back();
}

QColor DisplayGroupManager::getBackgroundColor() const
{
    return backgroundColor_;
}

void DisplayGroupManager::setBackgroundColor(QColor color)
{
    if(color == backgroundColor_)
        return;
    backgroundColor_ = color;

    emit modified(shared_from_this());
}

void DisplayGroupManager::advanceContents()
{
    // note that if we have multiple ContentWindowManagers corresponding to a single Content object,
    // we will call advance() multiple times per frame on that Content object...
    for(unsigned int i=0; i<contentWindowManagers_.size(); i++)
    {
        contentWindowManagers_[i]->getContent()->advance(contentWindowManagers_[i]);
    }
    if (backgroundContent_)
    {
        backgroundContent_->getContent()->advance(backgroundContent_);
    }
}

void DisplayGroupManager::sendDisplayGroup()
{
    emit modified(shared_from_this());
}

#if ENABLE_SKELETON_SUPPORT
void DisplayGroupManager::setSkeletons(std::vector< boost::shared_ptr<SkeletonState> > skeletons)
{
    skeletons_ = skeletons;

    emit modified(shared_from_this());
}
#endif
