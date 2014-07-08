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

#include "DisplayGroupGraphicsView.h"
#include "DisplayGroupGraphicsScene.h"
#include "ContentWindowGraphicsItem.h"

#include "globals.h"
#include "configuration/Configuration.h"

#include "gestures/PanGesture.h"
#include "gestures/PanGestureRecognizer.h"
#include "gestures/PinchGesture.h"
#include "gestures/PinchGestureRecognizer.h"

DisplayGroupGraphicsView::DisplayGroupGraphicsView(DisplayGroupManagerPtr displayGroup)
{
    // create and set scene for the view
    setScene(new DisplayGroupGraphicsScene(displayGroup));

    // force scene to be anchored at top left
    setAlignment(Qt::AlignLeft | Qt::AlignTop);

    // set attributes of the view
    setInteractive(true);
    setDragMode(QGraphicsView::RubberBandDrag);
    setAcceptDrops(true);

    grabGestures();
}

DisplayGroupGraphicsView::~DisplayGroupGraphicsView()
{
    delete scene();
}

void DisplayGroupGraphicsView::grabGestures()
{
    viewport()->grabGesture(Qt::TapGesture);
    viewport()->grabGesture(Qt::TapAndHoldGesture);
    viewport()->grabGesture(Qt::SwipeGesture);
}

bool DisplayGroupGraphicsView::viewportEvent( QEvent* evt )
{
    if( evt->type() == QEvent::Gesture )
    {
        QGestureEvent* gesture = static_cast< QGestureEvent* >( evt );
        gestureEvent( gesture );
        return QGraphicsView::viewportEvent( gesture );
    }
    return QGraphicsView::viewportEvent(evt);
}

void DisplayGroupGraphicsView::gestureEvent( QGestureEvent* evt )
{
    QGesture* gesture = 0;

    if( ( gesture = evt->gesture( Qt::SwipeGesture )))
    {
        evt->accept( Qt::SwipeGesture );
        swipe( static_cast< QSwipeGesture* >( gesture ));
    }
    else if( ( gesture = evt->gesture( PanGestureRecognizer::type( ))))
    {
        evt->accept( PanGestureRecognizer::type( ));
        pan( static_cast< PanGesture* >( gesture ));
    }
    else if( ( gesture = evt->gesture( PinchGestureRecognizer::type( ))))
    {
        evt->accept( PinchGestureRecognizer::type( ));
        pinch( static_cast< PinchGesture* >( gesture ));
    }
    else if( ( gesture = evt->gesture( Qt::TapGesture )))
    {
        evt->accept( Qt::TapGesture );
        tap( static_cast< QTapGesture* >( gesture ));
    }
    else if( ( gesture = evt->gesture( Qt::TapAndHoldGesture )))
    {
        evt->accept( Qt::TapAndHoldGesture );
        tapAndHold( static_cast< QTapAndHoldGesture* >( gesture ));
    }
}

void DisplayGroupGraphicsView::swipe( QSwipeGesture* )
{
    std::cout << "SWIPE VIEW" << std::endl;
}

void DisplayGroupGraphicsView::pan( PanGesture* )
{
}

void DisplayGroupGraphicsView::pinch( PinchGesture* )
{
}

void DisplayGroupGraphicsView::tap( QTapGesture* gesture )
{
    if( gesture->state() != Qt::GestureFinished )
        return;

    const QPointF position = getNormalizedPosition(gesture);

    if (isOnBackground(position))
        emit backgroundTap(position);
}

void DisplayGroupGraphicsView::tapAndHold( QTapAndHoldGesture* gesture )
{
    if( gesture->state() != Qt::GestureFinished )
        return;

    const QPointF position = getNormalizedPosition(gesture);

    if (isOnBackground(position))
        emit backgroundTapAndHold(position);
}

void DisplayGroupGraphicsView::resizeEvent(QResizeEvent * resizeEvt)
{
    // compute the scene rectangle to show such that the aspect ratio corresponds to the actual aspect ratio of the tiled display
    const float tiledDisplayAspect = g_configuration->getAspectRatio();
    const float windowAspect = (float)width() / (float)height();

    float sceneWidth, sceneHeight;

    if(tiledDisplayAspect >= windowAspect)
    {
        sceneWidth = 1.;
        sceneHeight = tiledDisplayAspect / windowAspect;
    }
    else // tiledDisplayAspect < windowAspect
    {
        sceneHeight = 1.;
        sceneWidth = windowAspect / tiledDisplayAspect;
    }

    // make sure we have a small buffer around the (0,0,1,1) scene rectangle
    float border = 0.05;

    sceneWidth = std::max(sceneWidth, (float)1.+border);
    sceneHeight = std::max(sceneHeight, (float)1.+border);

    setSceneRect(-(sceneWidth - 1.)/2., -(sceneHeight - 1.)/2., sceneWidth, sceneHeight);

    fitInView(sceneRect());

    QGraphicsView::resizeEvent(resizeEvt);
}

QPointF DisplayGroupGraphicsView::getNormalizedPosition( const QGesture* gesture ) const
{
    // Gesture::hotSpot() is the position (in pixels) in global SCREEN coordinates.
    // SCREEN is the Display where the Rank0 Qt Window lives.

    // Some gestures also have a position attribute which is inconsistent between
    // event types. For almost all gestures it is equal to the hotSpot attribute.
    // For the special case of the QTapGesture, it is actually the position in pixels
    // in DisplayWall coordinates...

    // The widgetPos is the position (in pixels) in the QGraphicsView.
    const QPoint widgetPos = mapFromGlobal( QPoint( gesture->hotSpot().x(),
                                                    gesture->hotSpot().y( )));
    // The returned value is the normalized position in the QGraphicsView.
    return mapToScene( widgetPos );
}

bool DisplayGroupGraphicsView::isOnBackground( const QPointF& position ) const
{
    const QGraphicsItem* item = scene()->itemAt( position );
    return dynamic_cast< const ContentWindowGraphicsItem* >( item ) == 0;
}

