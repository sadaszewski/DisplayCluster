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

#include "ContentWindowGraphicsItem.h"
#include "configuration/Configuration.h"
#include "Content.h"
#include "ContentWindowManager.h"
#include "DisplayGroupManager.h"
#include "DisplayGroupGraphicsView.h"
#include "globals.h"
#include "Options.h"
#include "ContentInteractionDelegate.h"
#include "gestures/DoubleTapGestureRecognizer.h"
#include "gestures/PanGestureRecognizer.h"
#include "gestures/PinchGestureRecognizer.h"

qreal ContentWindowGraphicsItem::zCounter_ = 0;

ContentWindowGraphicsItem::ContentWindowGraphicsItem(ContentWindowManagerPtr contentWindowManager)
    : ContentWindowInterface(contentWindowManager)
    , resizing_(false)
    , moving_(false)
{
    // graphics items are movable
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);

    // new items at the front
    // we assume that interface items will be constructed in depth order so this produces the correct result...
    setZToFront();

    grabGesture( DoubleTapGestureRecognizer::type( ));
    grabGesture( PanGestureRecognizer::type( ));
    grabGesture( PinchGestureRecognizer::type( ));
    grabGesture( Qt::SwipeGesture );
    grabGesture( Qt::TapAndHoldGesture );
    grabGesture( Qt::TapGesture );
}

ContentWindowGraphicsItem::~ContentWindowGraphicsItem()
{
}

QRectF ContentWindowGraphicsItem::boundingRect() const
{
    return coordinates_;
}

void ContentWindowGraphicsItem::paint(QPainter * painter, const QStyleOptionGraphicsItem* , QWidget* )
{
    if( !getContentWindowManager( ))
        return;

    drawFrame_( painter );

    drawCloseButton_( painter );

    drawResizeIndicator_( painter );

    drawFullscreenButton_( painter );

    drawMovieControls_( painter );

    drawTextLabel_( painter );
}

void ContentWindowGraphicsItem::adjustSize( const SizeState state,
                                            ContentWindowInterface * source )
{
    if(source != this)
        prepareGeometryChange();

    ContentWindowInterface::adjustSize( state, source );
}

void ContentWindowGraphicsItem::setCoordinates(QRectF coordinates, ContentWindowInterface * source)
{
    if(source != this)
        prepareGeometryChange();

    ContentWindowInterface::setCoordinates(coordinates, source);
}

void ContentWindowGraphicsItem::setPosition(double x_, double y_, ContentWindowInterface * source)
{
    if(source != this)
        prepareGeometryChange();

    ContentWindowInterface::setPosition(x_, y_, source);
}

void ContentWindowGraphicsItem::setSize(double w, double h, ContentWindowInterface * source)
{
    if(source != this)
        prepareGeometryChange();

    ContentWindowInterface::setSize(w, h, source);
}

void ContentWindowGraphicsItem::setCenter(double centerX, double centerY, ContentWindowInterface * source)
{
    if(source != this)
        prepareGeometryChange();

    ContentWindowInterface::setCenter(centerX, centerY, source);
}

void ContentWindowGraphicsItem::setZoom(double zoom, ContentWindowInterface * source)
{
    if(source != this)
        prepareGeometryChange();

    ContentWindowInterface::setZoom(zoom, source);
}

void ContentWindowGraphicsItem::setWindowState(ContentWindowInterface::WindowState windowState, ContentWindowInterface * source)
{
    if(source != this)
        prepareGeometryChange();

    ContentWindowInterface::setWindowState(windowState, source);
}

void ContentWindowGraphicsItem::setEvent(Event event, ContentWindowInterface * source)
{
    if(source != this)
        prepareGeometryChange();

    ContentWindowInterface::setEvent(event, source);
}

void ContentWindowGraphicsItem::setZToFront()
{
    zCounter_ = zCounter_ + 1;
    setZValue(zCounter_);
}

bool ContentWindowGraphicsItem::sceneEvent( QEvent* event )
{
    switch( event->type( ))
    {
    case QEvent::Gesture:
        getContentWindowManager()->getInteractionDelegate().gestureEvent( static_cast< QGestureEvent* >( event ));
        return true;
    case QEvent::KeyPress:
        // Override default behaviour to process TAB key events
        keyPressEvent(static_cast<QKeyEvent *>(event));
        return true;
    default:
        return QGraphicsObject::sceneEvent( event );
    }
}


void ContentWindowGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    // handle mouse movements differently depending on selected mode of item
    if(!selected())
    {
        if(event->buttons().testFlag(Qt::LeftButton))
        {
            if(resizing_)
            {
                QRectF r = boundingRect();
                QPointF eventPos = event->pos();

                r.setBottomRight(eventPos);

                QRectF sceneRect = mapRectToScene(r);

                double w = sceneRect.width();
                double h = sceneRect.height();

                setSize(w, h);
            }
            else
            {
                QPointF delta = event->pos() - event->lastPos();

                double new_x = coordinates_.x() + delta.x();
                double new_y = coordinates_.y() + delta.y();

                setPosition(new_x, new_y);
            }
        }
    }
    else
    {
        ContentWindowManagerPtr contentWindow = getContentWindowManager();
        if(contentWindow)
        {
            // Zoom or forward event depending on type
            contentWindow->getInteractionDelegate().mouseMoveEvent(event);

            // force a redraw to update window info label
            update();
        }
    }
}

void ContentWindowGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    // on Mac we've seen that mouse events can go to the wrong graphics item
    // this is due to the bug: https://bugreports.qt.nokia.com/browse/QTBUG-20493
    // here we ignore the event if it shouldn't have been sent to us, which ensures
    // it will go to the correct item...
    if(boundingRect().contains(event->pos()) == false)
    {
        event->ignore();
        return;
    }

    // button dimensions
    float buttonWidth, buttonHeight;
    getButtonDimensions(buttonWidth, buttonHeight);

    // item rectangle and event position
    QRectF r = boundingRect();
    QPointF eventPos = event->pos();

    // check to see if user clicked on the close button
    if(fabs((r.x()+r.width()) - eventPos.x()) <= buttonWidth &&
       fabs(r.y() - eventPos.y()) <= buttonHeight)
    {
        close();

        return;
    }

    // move to the front of the GUI display
    moveToFront();

    ContentWindowManagerPtr contentWindow = getContentWindowManager();
    if (!contentWindow)
        return;

    if (selected())
    {
        contentWindow->getInteractionDelegate().mousePressEvent(event);
        return;
    }

    contentWindow->getContent()->blockAdvance( true );

    // check to see if user clicked on the resize button
    if(fabs((r.x()+r.width()) - eventPos.x()) <= buttonWidth &&
       fabs((r.y()+r.height()) - eventPos.y()) <= buttonHeight)
    {
        resizing_ = true;
    }
    // check to see if user clicked on the fullscreen button
    else if(fabs(r.x() - eventPos.x()) <= buttonWidth &&
            fabs((r.y()+r.height()) - eventPos.y()) <= buttonHeight)
    {
        toggleFullscreen();
    }
    else if(fabs(((r.x()+r.width())/2) - eventPos.x() - buttonWidth) <= buttonWidth &&
            fabs((r.y()+r.height()) - eventPos.y()) <= buttonHeight &&
            g_configuration->getOptions()->getShowMovieControls( ))
    {
        contentWindow->setControlState( ControlState(contentWindow->getControlState() ^ STATE_PAUSED) );
    }
    else if(fabs(((r.x()+r.width())/2) - eventPos.x()) <= buttonWidth &&
            fabs((r.y()+r.height()) - eventPos.y()) <= buttonHeight &&
            g_configuration->getOptions()->getShowMovieControls( ))
    {
        contentWindow->setControlState( ControlState(contentWindow->getControlState() ^ STATE_LOOP) );
    }
    else
        moving_ = true;

    QGraphicsItem::mousePressEvent(event);
}

void ContentWindowGraphicsItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)
{
    // on Mac we've seen that mouse events can go to the wrong graphics item
    // this is due to the bug: https://bugreports.qt.nokia.com/browse/QTBUG-20493
    // here we ignore the event if it shouldn't have been sent to us, which ensures
    // it will go to the correct item...
    if(boundingRect().contains(event->pos()) == false)
    {
        event->ignore();
        return;
    }

    toggleWindowState();

    QGraphicsItem::mouseDoubleClickEvent(event);
}

void ContentWindowGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    resizing_ = false;
    moving_ = false;

    ContentWindowManagerPtr contentWindow = getContentWindowManager();

    if( contentWindow )
    {
        contentWindow->getContent()->blockAdvance( false );

        if (selected())
        {
            contentWindow->getInteractionDelegate().mouseReleaseEvent(event);
        }
    }

    QGraphicsItem::mouseReleaseEvent(event);
}

void ContentWindowGraphicsItem::wheelEvent(QGraphicsSceneWheelEvent * event)
{
    // on Mac we've seen that mouse events can go to the wrong graphics item
    // this is due to the bug: https://bugreports.qt.nokia.com/browse/QTBUG-20493
    // here we ignore the event if it shouldn't have been sent to us, which ensures
    // it will go to the correct item...
    if(boundingRect().contains(event->pos()) == false)
    {
        event->ignore();
        return;
    }

    ContentWindowManagerPtr contentWindow = getContentWindowManager();

    if( contentWindow )
    {
        // handle wheel movements differently depending on state of item window
        if (selected())
        {
            contentWindow->getInteractionDelegate().wheelEvent(event);
        }
        else
        {
            // scale size based on wheel delta
            // typical delta value is 120, so scale based on that
            double factor = 1. + (double)event->delta() / (10. * 120.);

            scaleSize(factor);
        }
    }
}

void ContentWindowGraphicsItem::keyPressEvent(QKeyEvent *event)
{
    if (selected())
    {
        ContentWindowManagerPtr contentWindow = getContentWindowManager();

        if( contentWindow )
        {
            contentWindow->getInteractionDelegate().keyPressEvent(event);
        }
    }
}

void ContentWindowGraphicsItem::keyReleaseEvent(QKeyEvent *event)
{
    if (selected())
    {
        ContentWindowManagerPtr contentWindow = getContentWindowManager();

        if( contentWindow )
        {
            contentWindow->getInteractionDelegate().keyReleaseEvent(event);
        }
    }
}

void ContentWindowGraphicsItem::drawCloseButton_( QPainter* painter )
{
    float buttonWidth, buttonHeight;
    getButtonDimensions(buttonWidth, buttonHeight);

    QRectF closeRect(coordinates_.x() + coordinates_.width() - buttonWidth, coordinates_.y(), buttonWidth, buttonHeight);
    QPen pen;
    pen.setColor(QColor(255,0,0));
    painter->setPen(pen);
    painter->drawRect(closeRect);
    painter->drawLine(QPointF(coordinates_.x() + coordinates_.width() - buttonWidth, coordinates_.y()),
                      QPointF(coordinates_.x() + coordinates_.width(), coordinates_.y() + buttonHeight));
    painter->drawLine(QPointF(coordinates_.x() + coordinates_.width(), coordinates_.y()),
                      QPointF(coordinates_.x() + coordinates_.width() - buttonWidth, coordinates_.y() + buttonHeight));
}

void ContentWindowGraphicsItem::drawResizeIndicator_( QPainter* painter )
{
    float buttonWidth, buttonHeight;
    getButtonDimensions(buttonWidth, buttonHeight);

    QRectF resizeRect(coordinates_.x() + coordinates_.width() - buttonWidth, coordinates_.y() + coordinates_.height() - buttonHeight, buttonWidth, buttonHeight);
    QPen pen;
    pen.setColor(QColor(128,128,128));
    painter->setPen(pen);
    painter->drawRect(resizeRect);
    painter->drawLine(QPointF(coordinates_.x() + coordinates_.width(), coordinates_.y() + coordinates_.height() - buttonHeight),
                      QPointF(coordinates_.x() + coordinates_.width() - buttonWidth, coordinates_.y() + coordinates_.height()));
}

void ContentWindowGraphicsItem::drawFullscreenButton_( QPainter* painter )
{
    float buttonWidth, buttonHeight;
    getButtonDimensions(buttonWidth, buttonHeight);

    QRectF fullscreenRect(coordinates_.x(), coordinates_.y() + coordinates_.height() - buttonHeight, buttonWidth, buttonHeight);
    QPen pen;
    pen.setColor(QColor(128,128,128));
    painter->setPen(pen);
    painter->drawRect(fullscreenRect);
}

void ContentWindowGraphicsItem::drawMovieControls_( QPainter* painter )
{
    ContentWindowManagerPtr contentWindowManager = getContentWindowManager();

    float buttonWidth, buttonHeight;
    getButtonDimensions(buttonWidth, buttonHeight);

    QPen pen;

    if( contentWindowManager->getContent()->getType() == CONTENT_TYPE_MOVIE &&
        g_configuration->getOptions()->getShowMovieControls( ))
    {
        // play/pause
        QRectF playPauseRect(coordinates_.x() + coordinates_.width()/2 - buttonWidth, coordinates_.y() + coordinates_.height() - buttonHeight,
                              buttonWidth, buttonHeight);
        pen.setColor(QColor(contentWindowManager->getControlState() & STATE_PAUSED ? 128 :200,0,0));
        painter->setPen(pen);
        painter->fillRect(playPauseRect, pen.color());

        // loop
        QRectF loopRect(coordinates_.x() + coordinates_.width()/2, coordinates_.y() + coordinates_.height() - buttonHeight,
                        buttonWidth, buttonHeight);
        pen.setColor(QColor(0,contentWindowManager->getControlState() & STATE_LOOP ? 200 :128,0));
        painter->setPen(pen);
        painter->fillRect(loopRect, pen.color());
    }
}

void ContentWindowGraphicsItem::drawTextLabel_( QPainter* painter )
{
    ContentWindowManagerPtr contentWindowManager = getContentWindowManager();

    float buttonWidth, buttonHeight;
    getButtonDimensions(buttonWidth, buttonHeight);

    const float fontSize = 24.;

    QFont font;
    font.setPixelSize(fontSize);
    painter->setFont(font);

    // color the text black
    QPen pen;
    pen.setColor(QColor(0,0,0));
    painter->setPen(pen);

    // scale the text size down to the height of the graphics view
    // and, calculate the bounding rectangle for the text based on this scale
    // the dimensions of the view need to be corrected for the tiled display aspect ratio
    // recall the tiled display UI is only part of the graphics view since we show it at the correct aspect ratio
    // TODO refactor this for clarity!
    float viewWidth = (float)scene()->views()[0]->width();
    float viewHeight = (float)scene()->views()[0]->height();

    const float tiledDisplayAspect = g_configuration->getAspectRatio();

    if(viewWidth / viewHeight > tiledDisplayAspect)
    {
        viewWidth = viewHeight * tiledDisplayAspect;
    }
    else if(viewWidth / viewHeight <= tiledDisplayAspect)
    {
        viewHeight = viewWidth / tiledDisplayAspect;
    }

    float verticalTextScale = 1. / viewHeight;
    float horizontalTextScale = viewHeight / viewWidth * verticalTextScale;

    painter->scale(horizontalTextScale, verticalTextScale);

    QRectF textBoundingRect = QRectF(coordinates_.x() / horizontalTextScale,
                                     coordinates_.y() / verticalTextScale,
                                     coordinates_.width() / horizontalTextScale,
                                     coordinates_.height() / verticalTextScale);

    // get the label and render it
    QString label(contentWindowManager->getContent()->getURI());
    QString labelSection = label.section("/", -1, -1).prepend(" ");
    painter->drawText(textBoundingRect, Qt::AlignLeft | Qt::AlignTop, labelSection);

    // draw window info at smaller scale
    verticalTextScale *= 0.5;
    horizontalTextScale *= 0.5;

    painter->scale(0.5, 0.5);

    textBoundingRect = QRectF((coordinates_.x()+buttonWidth) / horizontalTextScale,
                               coordinates_.y() / verticalTextScale,
                              (coordinates_.width()-buttonWidth) / horizontalTextScale,
                               coordinates_.height() / verticalTextScale);

    QString coordinatesLabel = QString(" (") + QString::number(coordinates_.x(), 'f', 2) + QString(" ,") +
                                               QString::number(coordinates_.y(), 'f', 2) + QString(", ") +
                                               QString::number(coordinates_.width(), 'f', 2) + QString(", ") +
                                               QString::number(coordinates_.height(), 'f', 2) + QString(")\n");
    QString zoomCenterLabel = QString(" zoom = ") + QString::number(zoom_, 'f', 2) + QString(" @ (") +
                              QString::number(centerX_, 'f', 2) + QString(", ") +
                              QString::number(centerY_, 'f', 2) + QString(")");
    QString interactionLabel = QString(" x: ") +
            QString::number(latestEvent_.mouseX, 'f', 2) +
            QString(" y: ") + QString::number(latestEvent_.mouseY, 'f', 2) +
            QString(" mouseLeft: ") + QString::number((int) latestEvent_.mouseLeft, 'b', 1) +
            QString(" mouseMiddle: ") + QString::number((int) latestEvent_.mouseMiddle, 'b', 1) +
            QString(" mouseRight: ") + QString::number((int) latestEvent_.mouseRight, 'b', 1);

    QString windowInfoLabel = coordinatesLabel + zoomCenterLabel + interactionLabel;
    painter->drawText(textBoundingRect, Qt::AlignLeft | Qt::AlignBottom, windowInfoLabel);
}

void ContentWindowGraphicsItem::drawFrame_(QPainter* painter)
{
    QPen pen;
    if(selected())
        pen.setColor(QColor(255,0,0));
    else
        pen.setColor(QColor(0,0,0));

    painter->setPen(pen);
    painter->setBrush( QBrush(QColor(0, 0, 0, 128)));
    painter->drawRect(boundingRect());
}
