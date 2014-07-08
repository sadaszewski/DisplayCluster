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

#ifndef CONTENT_WINDOW_MANAGER_H
#define CONTENT_WINDOW_MANAGER_H

#include "ContentWindowInterface.h"
#include "Content.h" // need pyContent for pyContentWindowManager

#include "serializationHelpers.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/serialization/weak_ptr.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>

class DisplayGroupManager;
class ContentInteractionDelegate;

/**
 * A window for placing a Content on the Wall.
 *
 * Can be serialized and distributed to the Wall applications.
 */
class ContentWindowManager : public ContentWindowInterface,
        public boost::enable_shared_from_this<ContentWindowManager>
{
    Q_OBJECT

public:
    /** No-argument constructor required for serialization. */
    ContentWindowManager();

    /**
     * Create a new window.
     * @param content The Content for this window.
     * @note Rank0 only.
     */
    ContentWindowManager(ContentPtr content);

    /** Destructor. */
    virtual ~ContentWindowManager();

    /** Get the content. */
    ContentPtr getContent() const;

    /** Set the content, replacing the existing one. @note Rank0 only. */
    void setContent(ContentPtr content);

    /** Get the parent DisplayGroup of this window. */
    DisplayGroupManagerPtr getDisplayGroupManager() const;

    /** Set a reference on the parent DisplayGroup of this window. */
    void setDisplayGroupManager(DisplayGroupManagerPtr displayGroupManager);

    /**
     * Get the interaction delegate.
     * @see createInteractionDelegate()
     * @note Rank0 only.
     */
    ContentInteractionDelegate& getInteractionDelegate() const;

    /**
     * Create a delegate to handle user interaction through dc::Events.
     * The type of delegate created depends on the ContentType.
     * @note Rank0 only.
     */
    void createInteractionDelegate();

    ///@{
    /** re-implemented ContentWindowInterface slots. */
    void moveToFront(ContentWindowInterface* source = 0) override;
    void close(ContentWindowInterface* source = 0) override;
    ///@}

    /** Get the position of the window center. */
    QPointF getWindowCenterPosition() const;

    /**
     * Move the window to a new position.
     * @param position The position for the window center
     * @param constrainToWindowBorders If true, the new position will be
     *        adjusted so that the window does not exceed the screen boundaries.
     */
    void centerPositionAround(const QPointF& position,
                              const bool constrainToWindowBorders);

signals:
    /** Emitted when the Content signals that it has been modified. */
    void contentModified();

protected:
    friend class boost::serialization::access;

    /** Serialize for sending to Wall applications. */
    template<class Archive>
    void serialize(Archive & ar, const unsigned int)
    {
        ar & content_;
        ar & displayGroupManager_;
        ar & contentWidth_;
        ar & contentHeight_;
        ar & coordinates_;
        ar & centerX_;
        ar & centerY_;
        ar & zoom_;
        ar & controlState_;
        ar & windowState_;
        ar & highlightedTimestamp_;
    }

    /** Serialize for saving to an xml file */
    template<class Archive>
    void serialize_for_xml(Archive & ar, const unsigned int)
    {
        ar & boost::serialization::make_nvp("content", content_);
        ar & boost::serialization::make_nvp("contentWidth", contentWidth_);
        ar & boost::serialization::make_nvp("contentHeight", contentHeight_);
        ar & boost::serialization::make_nvp("coordinates", coordinates_);
        ar & boost::serialization::make_nvp("coordinatesBackup", coordinatesBackup_);
        ar & boost::serialization::make_nvp("centerX", centerX_);
        ar & boost::serialization::make_nvp("centerY", centerY_);
        ar & boost::serialization::make_nvp("zoom", zoom_);
        ar & boost::serialization::make_nvp("controlState", controlState_);
        ar & boost::serialization::make_nvp("windowState", windowState_);
    }

private:
    ContentPtr content_;

    boost::weak_ptr<DisplayGroupManager> displayGroupManager_;

    // Rank0: Delegate to handle user inputs
    ContentInteractionDelegate* interactionDelegate_;
};

DECLARE_SERIALIZE_FOR_XML(ContentWindowManager)

// typedef needed for SIP
typedef ContentWindowManagerPtr pContentWindowManager;

class pyContentWindowManager
{
public:

    pyContentWindowManager(pyContent content)
    {
        ContentWindowManagerPtr contentWindow(new ContentWindowManager(content.get()));
        ptr_ = contentWindow;
    }

    pyContentWindowManager(ContentWindowManagerPtr contentWindow)
    {
        ptr_ = contentWindow;
    }

    ContentWindowManagerPtr get()
    {
        return ptr_;
    }

    pyContent getPyContent()
    {
        return pyContent(get()->getContent());
    }

private:

    ContentWindowManagerPtr ptr_;
};

#endif
