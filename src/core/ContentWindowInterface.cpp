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

#include "ContentWindowInterface.h"
#include "ContentWindowManager.h"
#include "globals.h"
#include "configuration/Configuration.h"
#include "MPIChannel.h"
#include "RenderContext.h"
#include "EventReceiver.h"
#include "MPIChannel.h"

ContentWindowInterface::ContentWindowInterface()
    : uuid_(QUuid::createUuid())
    , contentWidth_(0)
    , contentHeight_(0)
    , centerX_(0.5)
    , centerY_(0.5)
    , zoom_(1)
    , windowState_( UNSELECTED )
    , sizeState_( SIZE_NORMALIZED )
    , controlState_( STATE_LOOP )
    , eventReceiversCount_( 0 )
{}

ContentWindowInterface::ContentWindowInterface(ContentWindowManagerPtr contentWindowManager)
    : uuid_(contentWindowManager ? contentWindowManager->getID() : QUuid::createUuid())
    , contentWindowManager_(contentWindowManager)
    , contentWidth_(0)
    , contentHeight_(0)
    , centerX_(0)
    , centerY_(0)
    , zoom_(0)
    , windowState_( UNSELECTED )
    , sizeState_( SIZE_NORMALIZED )
    , controlState_( STATE_PAUSED )
    , eventReceiversCount_( 0 )
{
    // copy all members from contentWindowManager
    if(contentWindowManager)
    {
        contentWidth_ = contentWindowManager->contentWidth_;
        contentHeight_ = contentWindowManager->contentHeight_;
        coordinates_ = contentWindowManager->coordinates_;
        centerX_ = contentWindowManager->centerX_;
        centerY_ = contentWindowManager->centerY_;
        zoom_ = contentWindowManager->zoom_;
        sizeState_ = contentWindowManager->sizeState_;
        controlState_ = contentWindowManager->controlState_;
        windowState_ = contentWindowManager->windowState_;
        latestEvent_ = contentWindowManager->latestEvent_;
    }

    // connect signals from this to slots on the ContentWindowManager
    // use queued connections for thread-safety
    connect(this, SIGNAL(contentDimensionsChanged(int, int, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setContentDimensions(int, int, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(coordinatesChanged(QRectF, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setCoordinates(QRectF, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(positionChanged(double, double, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setPosition(double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(sizeChanged(double, double, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setSize(double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(centerChanged(double, double, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setCenter(double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(zoomChanged(double, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setZoom(double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(windowStateChanged(ContentWindowInterface::WindowState, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setWindowState(ContentWindowInterface::WindowState, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(eventChanged(Event, ContentWindowInterface *)), contentWindowManager.get(), SLOT(setEvent(Event, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(highlighted(ContentWindowInterface *)), contentWindowManager.get(), SLOT(highlight(ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(movedToFront(ContentWindowInterface *)), contentWindowManager.get(), SLOT(moveToFront(ContentWindowInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(closed(ContentWindowInterface *)), contentWindowManager.get(), SLOT(close(ContentWindowInterface *)), Qt::QueuedConnection);

    // connect signals on the ContentWindowManager object to slots on this
    // use queued connections for thread-safety
    connect(contentWindowManager.get(), SIGNAL(contentDimensionsChanged(int, int, ContentWindowInterface *)), this, SLOT(setContentDimensions(int, int, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindowManager.get(), SIGNAL(coordinatesChanged(QRectF, ContentWindowInterface *)), this, SLOT(setCoordinates(QRectF, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindowManager.get(), SIGNAL(positionChanged(double, double, ContentWindowInterface *)), this, SLOT(setPosition(double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindowManager.get(), SIGNAL(sizeChanged(double, double, ContentWindowInterface *)), this, SLOT(setSize(double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindowManager.get(), SIGNAL(centerChanged(double, double, ContentWindowInterface *)), this, SLOT(setCenter(double, double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindowManager.get(), SIGNAL(zoomChanged(double, ContentWindowInterface *)), this, SLOT(setZoom(double, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindowManager.get(), SIGNAL(windowStateChanged(ContentWindowInterface::WindowState, ContentWindowInterface *)), this, SLOT(setWindowState(ContentWindowInterface::WindowState, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindowManager.get(), SIGNAL(eventChanged(Event, ContentWindowInterface *)), this, SLOT(setEvent(Event, ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindowManager.get(), SIGNAL(highlighted(ContentWindowInterface *)), this, SLOT(highlight(ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindowManager.get(), SIGNAL(movedToFront(ContentWindowInterface *)), this, SLOT(moveToFront(ContentWindowInterface *)), Qt::QueuedConnection);
    connect(contentWindowManager.get(), SIGNAL(closed(ContentWindowInterface *)), this, SLOT(close(ContentWindowInterface *)), Qt::QueuedConnection);

    // destruction
    connect(contentWindowManager.get(), SIGNAL(destroyed(QObject *)), this, SLOT(deleteLater()));
}

const QUuid& ContentWindowInterface::getID() const
{
    return uuid_;
}

ContentWindowManagerPtr ContentWindowInterface::getContentWindowManager()
{
    return contentWindowManager_.lock();
}

void ContentWindowInterface::getContentDimensions(int &contentWidth, int &contentHeight)
{
    contentWidth = contentWidth_;
    contentHeight = contentHeight_;
}

void ContentWindowInterface::getCoordinates(double &x, double &y, double &w, double &h)
{
    x = coordinates_.x();
    y = coordinates_.y();
    w = coordinates_.width();
    h = coordinates_.height();
}

QRectF ContentWindowInterface::getCoordinates() const
{
    return coordinates_;
}

void ContentWindowInterface::getPosition(double &x, double &y)
{
    x = coordinates_.x();
    y = coordinates_.y();
}

void ContentWindowInterface::getSize(double &w, double &h)
{
    w = coordinates_.width();
    h = coordinates_.height();
}

void ContentWindowInterface::getCenter(double &centerX, double &centerY)
{
    centerX = centerX_;
    centerY = centerY_;
}

double ContentWindowInterface::getZoom()
{
    return zoom_;
}

void ContentWindowInterface::toggleWindowState()
{
    setWindowState( windowState_ == UNSELECTED ? SELECTED : UNSELECTED );
}

void ContentWindowInterface::toggleFullscreen()
{
    adjustSize( getSizeState() == SIZE_FULLSCREEN ? SIZE_NORMALIZED : SIZE_FULLSCREEN );
}

ContentWindowInterface::WindowState ContentWindowInterface::getWindowState()
{
    return windowState_;
}

Event ContentWindowInterface::getEvent() const
{
    return latestEvent_;
}

bool ContentWindowInterface::registerEventReceiver(EventReceiver* receiver)
{
    const bool success = connect( this, SIGNAL(eventChanged( Event, ContentWindowInterface* )),
                                  receiver, SLOT(processEvent(Event)) );
    if (success)
        ++eventReceiversCount_;

    return success;
}

bool ContentWindowInterface::getHighlighted()
{
    const long dtMilliseconds = (g_mpiChannel->getTime() - highlightedTimestamp_).total_milliseconds();

    if(dtMilliseconds > HIGHLIGHT_TIMEOUT_MILLISECONDS || dtMilliseconds % (HIGHLIGHT_BLINK_INTERVAL*2) < HIGHLIGHT_BLINK_INTERVAL)
    {
        return false;
    }
    else
    {
        return true;
    }
}

SizeState ContentWindowInterface::getSizeState() const
{
    return sizeState_;
}

void ContentWindowInterface::getButtonDimensions(float &width, float &height)
{
    const float sceneHeightFraction = 0.125;
    const double screenAspect = g_configuration->getAspectRatio();

    width = sceneHeightFraction / screenAspect;
    height = sceneHeightFraction;

    // clamp to half rect dimensions
    if(width > 0.5 * coordinates_.width())
    {
        width = 0.49 * coordinates_.width();
    }

    if(height > 0.5 * coordinates_.height())
    {
        height = 0.49 * coordinates_.height();
    }
}

void ContentWindowInterface::fixAspectRatio(ContentWindowInterface * source)
{
    if(contentWidth_ == 0 && contentHeight_ == 0)
        return;

    double aspect = (double)contentWidth_ / (double)contentHeight_;
    const double screenAspect = g_configuration->getAspectRatio();

    aspect /= screenAspect;

    double w = coordinates_.width();
    double h = coordinates_.height();

    if(aspect > coordinates_.width() / coordinates_.height())
    {
        h = coordinates_.width() / aspect;
    }
    else if(aspect <= coordinates_.width() / coordinates_.height())
    {
        w = coordinates_.height() * aspect;
    }

    // we don't want to call setSize unless necessary, since it will emit a signal
    if(w != coordinates_.width() || h != coordinates_.height())
    {
        coordinates_.setWidth( w );
        coordinates_.setHeight( h );

        if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
            setSize(coordinates_.width(), coordinates_.height());
    }
}

void ContentWindowInterface::adjustSize( const SizeState state,
                                         ContentWindowInterface * source )
{
    sizeState_ = state;

    const double contentAR = contentHeight_ == 0 ? 16./9 :
                                 double(contentWidth_) / double(contentHeight_);
    const double wallAR = 1. / g_configuration->getAspectRatio();

    double height = contentHeight_ == 0
                            ? 1.
                            : double(contentHeight_) / double(g_configuration->getTotalHeight());
    double width = contentWidth_ == 0
                            ? wallAR * contentAR * height
                            : double(contentWidth_) / double(g_configuration->getTotalWidth());

    QRectF coordinates;

    switch( state )
    {
    case SIZE_FULLSCREEN:
        {
            coordinatesBackup_ = coordinates_;
            const double resize = std::min( 1. / height, 1. / width );
            width *= resize;
            height *= resize;

            // center on the wall
            coordinates.setRect( (1. - width) * .5 , (1. - height) * .5, width, height );
        } break;

    case SIZE_1TO1:
        height = std::min( height, 1. );
        width = wallAR * contentAR * height;
        if( width > 1. )
        {
            height /= width;
            width /= width;
        }

        // center on the wall
        coordinates.setRect( (1. - width) * .5 , (1. - height) * .5, width, height );
        break;

    case SIZE_NORMALIZED:
        coordinates = coordinatesBackup_;
        break;
    default:
        return;
    }

    setCoordinates( coordinates, source );
}

void ContentWindowInterface::setContentDimensions(int contentWidth, int contentHeight, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    contentWidth_ = contentWidth;
    contentHeight_ = contentHeight;

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(contentDimensionsChanged(contentWidth_, contentHeight_, source));
    }
}

void ContentWindowInterface::setCoordinates(QRectF coordinates, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    // don't allow negative width or height
    if( coordinates.isValid( ))
        coordinates_ = coordinates;
    else
        coordinates_.moveTo( coordinates.x(), coordinates.y( ));

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        fixAspectRatio(source);

        emit(coordinatesChanged(coordinates_, source));

        setEventToNewDimensions();
    }
}

void ContentWindowInterface::setPosition(double x, double y, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    coordinates_.moveTo( x, y );

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(positionChanged(coordinates_.x(), coordinates_.y(), source));
    }
}

void ContentWindowInterface::setSize(double w, double h, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    // don't allow negative width or height
    if(w > 0. && h > 0.)
    {
        coordinates_.setWidth( w );
        coordinates_.setHeight( h );
    }

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        fixAspectRatio(source);

        emit(sizeChanged(coordinates_.width(), coordinates_.height(), source));

        setEventToNewDimensions();
    }
}

void ContentWindowInterface::scaleSize(double factor, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    // don't allow negative factor
    if(factor < 0.)
    {
        return;
    }

    // calculate new coordinates
    coordinates_.setX( coordinates_.x() - (factor - 1.) * coordinates_.width() / 2. );
    coordinates_.setY( coordinates_.y() - (factor - 1.) * coordinates_.height() / 2. );
    coordinates_.setWidth( coordinates_.width() * factor );
    coordinates_.setHeight( coordinates_.height() * factor );

    setCoordinates(coordinates_);

    // we don't need to emit any signals since setCoordinates() takes care of this
}

void ContentWindowInterface::setCenter(double centerX, double centerY, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    // clamp center point such that view rectangle dimensions are constrained [0,1]
    float tX = centerX - 0.5 / zoom_;
    float tY = centerY - 0.5 / zoom_;
    float tW = 1./zoom_;
    float tH = 1./zoom_;

    // handle centerX, clamping it if necessary
    if(tX >= 0. && tX+tW <= 1.)
    {
        centerX_ = centerX;
    }
    else if(tX < 0.)
    {
        centerX_ = 0.5 / zoom_;
    }
    else if(tX+tW > 1.)
    {
        centerX_ = 1. - tW + 0.5 / zoom_;
    }

    // handle centerY, clamping it if necessary
    if(tY >= 0. && tY+tH <= 1.)
    {
        centerY_ = centerY;
    }
    else if(tY < 0.)
    {
        centerY_ = 0.5 / zoom_;
    }
    else if(tY+tH > 1.)
    {
        centerY_ = 1. - tH + 0.5 / zoom_;
    }

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(centerChanged(centerX_, centerY_, source));
    }
}

void ContentWindowInterface::setZoom(double zoom, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    // clamp zoom to be >= 1
    if(zoom < 1.)
    {
        zoom = 1.;
    }

    zoom_ = zoom;

    float tX = centerX_ - 0.5 / zoom;
    float tY = centerY_ - 0.5 / zoom;
    float tW = 1./zoom;
    float tH = 1./zoom;

    // see if we need to adjust the center point since the rectangle view bounds are outside [0,1]
    if(QRectF(0.,0.,1.,1.).contains(QRectF(tX,tY,tW,tH)) != true)
    {
        // handle centerX, clamping it if necessary
        if(tX < 0.)
        {
            centerX_ = 0.5 / zoom_;
        }
        else if(tX+tW > 1.)
        {
            centerX_ = 1. - tW + 0.5 / zoom_;
        }

        // handle centerY, clamping it if necessary
        if(tY < 0.)
        {
            centerY_ = 0.5 / zoom_;
        }
        else if(tY+tH > 1.)
        {
            centerY_ = 1. - tH + 0.5 / zoom_;
        }

        setCenter(centerX_, centerY_);
    }

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(zoomChanged(zoom_, source));
    }
}

void ContentWindowInterface::setWindowState(ContentWindowInterface::WindowState windowState, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    windowState_ = windowState;

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(windowStateChanged(windowState_, source));
    }
}

void ContentWindowInterface::setEvent(Event event_, ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    latestEvent_ = event_;

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(eventChanged(event_, source));
    }
}

void ContentWindowInterface::highlight(ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    // set highlighted timestamp
    highlightedTimestamp_ = g_mpiChannel->getTime();

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(highlighted(source));
    }
}

void ContentWindowInterface::moveToFront(ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(movedToFront(source));
    }
}

void ContentWindowInterface::close(ContentWindowInterface * source)
{
    if(source == this)
    {
        return;
    }

    if(source == NULL || dynamic_cast<ContentWindowManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(closed(source));
    }
}

void ContentWindowInterface::setEventToNewDimensions()
{
    Event state;
    state.type = Event::EVT_VIEW_SIZE_CHANGED;
    state.dx = coordinates_.width() * g_configuration->getTotalWidth();
    state.dy = coordinates_.height() * g_configuration->getTotalHeight();
    setEvent(state);
}
