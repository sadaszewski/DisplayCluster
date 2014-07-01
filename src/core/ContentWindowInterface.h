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

#ifndef CONTENT_WINDOW_INTERFACE_H
#define CONTENT_WINDOW_INTERFACE_H

#define HIGHLIGHT_TIMEOUT_MILLISECONDS 1500
#define HIGHLIGHT_BLINK_INTERVAL 250 // milliseconds

#include "Event.h"
#include "types.h"

#include <QObject>
#include <QUuid>
#include <QRectF>
#ifndef Q_MOC_RUN
// https://bugreports.qt.nokia.com/browse/QTBUG-22829: When Qt moc runs on CGAL
// files, do not process <boost/type_traits/has_operator.hpp>
#  include <boost/shared_ptr.hpp>
#  include <boost/weak_ptr.hpp>
#  include <boost/date_time/posix_time/posix_time.hpp>
#endif

class EventReceiver;

using dc::Event;

enum ControlState
{
    STATE_PAUSED = 1 << 0,
    STATE_LOOP   = 1 << 1
};

enum SizeState
{
    SIZE_1TO1,
    SIZE_FULLSCREEN,
    SIZE_NORMALIZED
};

class ContentWindowInterface : public QObject
{
    Q_OBJECT

    public:

        enum WindowState
        {
            UNSELECTED,   // the window is not selected and interaction modifies its position/size
            SELECTED      // the window is selected and interaction may be forwarded to the ContentInteractionDelegate
        };

        ContentWindowInterface();
        ContentWindowInterface(ContentWindowManagerPtr contentWindowManager);

        /** @return the unique identifier for this window. */
        const QUuid& getID() const;

        /** Get the ContentWindowManger associated to this object if it has one, otherwise returns 0. */
        ContentWindowManagerPtr getContentWindowManager();

        /** Get content dimensions in pixels. */
        void getContentDimensions(int &contentWidth, int &contentHeight);

        /** Get the normalized window coordiates. */
        void getCoordinates(double &x, double &y, double &w, double &h);

        /** Get the normalized window coordiates. */
        QRectF getCoordinates() const;

        /** Get the normalized position. */
        void getPosition(double &x, double &y);

        /** Get the normalized size. */
        void getSize(double &w, double &h);

        /** Get the normalized center position. */
        void getCenter(double &centerX, double &centerY);

        /** Get the zoom factor [1;inf]. */
        double getZoom();

        /** Is the window highlighted. */
        bool getHighlighted();

        /** Get the current size state. */
        SizeState getSizeState() const;

        /** Set the control state. */
        void setControlState( const ControlState state ) { controlState_ = state; }

        /** Get the control state. */
        ControlState getControlState() const { return controlState_; }

        /** Get the last event for this window. */
        Event getEvent() const;

        /** Toggle the window state. */
        void toggleWindowState();

        /** Toggle between fullscreen and 'normalized' by keeping the position
         *  and size after leaving fullscreen */
        void toggleFullscreen();

        /** Get the window state. */
        ContentWindowInterface::WindowState getWindowState();

        /** Get the dimensions for the window buttons in the ContentWindowGraphicsItem. */
        void getButtonDimensions(float &width, float &height);

        /** Set the aspect ratio policy based on the gloabl Options. */
        void fixAspectRatio(ContentWindowInterface * source = 0);

        /** Is the window selected. */
        bool selected() const { return windowState_ == SELECTED; }

        /** Register an object to receive this window's Events. */
        bool registerEventReceiver( EventReceiver* receiver );

        /** Does this window already have registered Event receiver(s) */
        bool hasEventReceivers() const { return eventReceiversCount_ > 0; }

    public slots:

        // these methods set the local copies of the state variables if source != this
        // they will emit signals if source == 0 or if this is a ContentWindowManager object
        // the source argument should not be provided by users -- only by these functions
        virtual void adjustSize( const SizeState state, ContentWindowInterface* source = 0 );
        virtual void setContentDimensions(int contentWidth, int contentHeight, ContentWindowInterface* source = 0);
        virtual void setCoordinates(QRectF coordinates, ContentWindowInterface* source = 0);
        virtual void setPosition(double x, double y, ContentWindowInterface* source = 0);
        virtual void setSize(double w, double h, ContentWindowInterface* source = 0);
        virtual void scaleSize(double factor, ContentWindowInterface* source = 0);
        virtual void setCenter(double centerX, double centerY, ContentWindowInterface* source = 0);
        virtual void setZoom(double zoom, ContentWindowInterface* source = 0);
        virtual void highlight(ContentWindowInterface* source = 0);
        virtual void setWindowState(ContentWindowInterface::WindowState windowState, ContentWindowInterface* source = 0);
        virtual void setEvent(Event event, ContentWindowInterface* source = 0);
        virtual void moveToFront(ContentWindowInterface* source = 0);
        virtual void close(ContentWindowInterface* source = 0);

    signals:

        // emitting these signals will trigger updates on the corresponding ContentWindowManager
        // as well as all other ContentWindowInterfaces to that ContentWindowManager
        void contentDimensionsChanged(int contentWidth, int contentHeight, ContentWindowInterface * source);
        void coordinatesChanged(QRectF coordinates, ContentWindowInterface * source);
        void positionChanged(double x, double y, ContentWindowInterface * source);
        void sizeChanged(double w, double h, ContentWindowInterface * source);
        void centerChanged(double centerX, double centerY, ContentWindowInterface * source);
        void zoomChanged(double zoom, ContentWindowInterface * source);
        void highlighted(ContentWindowInterface * source);
        void windowStateChanged(ContentWindowInterface::WindowState windowState, ContentWindowInterface * source);
        void eventChanged(Event event, ContentWindowInterface * source);
        void movedToFront(ContentWindowInterface * source);
        void closed(ContentWindowInterface * source);

    protected:

        void setEventToNewDimensions();

        const QUuid uuid_;

        // optional: reference to ContentWindowManager for non-ContentWindowManager objects
        boost::weak_ptr<ContentWindowManager> contentWindowManager_;

        // content dimensions in pixels
        // TODO remove those (DISCL-231)
        int contentWidth_;
        int contentHeight_;

        // normalized window coordinates
        QRectF coordinates_;
        QRectF coordinatesBackup_;

        // panning and zooming
        double centerX_;
        double centerY_;

        double zoom_;

        // window state
        ContentWindowInterface::WindowState windowState_;

        // Window interaction
        Event latestEvent_;

        SizeState sizeState_;

        ControlState controlState_;

        unsigned int eventReceiversCount_;

        // highlighted timestamp
        boost::posix_time::ptime highlightedTimestamp_;
};

#endif
