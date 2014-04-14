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

#ifndef DISPLAY_GROUP_MANAGER_H
#define DISPLAY_GROUP_MANAGER_H

#include "DisplayGroupInterface.h"
#include "Marker.h"

#include "config.h"
#include "types.h"
#include "serializationHelpers.h"

#if ENABLE_SKELETON_SUPPORT
#include "SkeletonState.h"
#endif

#include <QtGui>
#include <vector>
#ifndef Q_MOC_RUN
// https://bugreports.qt.nokia.com/browse/QTBUG-22829: When Qt moc runs on CGAL
// files, do not process <boost/type_traits/has_operator.hpp>
#  include <boost/enable_shared_from_this.hpp>
#endif

class ContentWindowManager;
class EventReceiver;

class DisplayGroupManager : public DisplayGroupInterface,
        public boost::enable_shared_from_this<DisplayGroupManager>
{
    Q_OBJECT

public:

    DisplayGroupManager();
    ~DisplayGroupManager();

    MarkerPtr getNewMarker();
    MarkerPtrs getMarkers() const;
    void deleteMarkers();

#if ENABLE_SKELETON_SUPPORT
    std::vector< boost::shared_ptr<SkeletonState> > getSkeletons();
#endif

    // re-implemented DisplayGroupInterface slots
    void addContentWindowManager(ContentWindowManagerPtr contentWindowManager, DisplayGroupInterface * source=NULL);
    void removeContentWindowManager(ContentWindowManagerPtr contentWindowManager, DisplayGroupInterface * source=NULL);
    void moveContentWindowManagerToFront(ContentWindowManagerPtr contentWindowManager, DisplayGroupInterface * source=NULL);

    QColor getBackgroundColor() const;
    void setBackgroundColor(QColor color);

    bool setBackgroundContentFromUri(const QString& filename);
    void setBackgroundContentWindowManager(ContentWindowManagerPtr contentWindowManager);
    ContentWindowManagerPtr getBackgroundContentWindowManager() const;

    /**
     * Is the DisplayGroup empty.
     * @return true if the DisplayGroup has no ContentWindowManager, false otherwise.
     */
    bool isEmpty() const;

    /**
     * Get the active window.
     * @return A shared pointer to the active window. Can be empty if there is
     *         no Window available. @see isEmpty().
     */
    ContentWindowManagerPtr getActiveWindow() const;

signals:
    /** Emitted whenever the DisplayGroup is modified */
    void modified(DisplayGroupManagerPtr displayGroup);

public slots:
    /** Advance all contents */
    void advanceContents();

#if ENABLE_SKELETON_SUPPORT
    void setSkeletons(std::vector<boost::shared_ptr<SkeletonState> > skeletons);
#endif

private slots:
    void sendDisplayGroup();

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int)
    {
        QMutexLocker locker(&markersMutex_);
        ar & markers_;
        ar & contentWindowManagers_;
        ar & backgroundContent_;
        ar & backgroundColor_;
#if ENABLE_SKELETON_SUPPORT
        ar & skeletons_;
#endif
    }

    void watchChanges(ContentWindowManagerPtr contentWindow);

    ContentWindowManagerPtr backgroundContent_;
    QColor backgroundColor_;

    mutable QMutex markersMutex_;
    MarkerPtrs markers_;

#if ENABLE_SKELETON_SUPPORT
    std::vector<boost::shared_ptr<SkeletonState> > skeletons_;
#endif
};

#endif
