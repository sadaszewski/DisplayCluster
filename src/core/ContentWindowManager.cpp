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

#include "ContentWindowManager.h"
#include "Content.h"
#include "DisplayGroupManager.h"
#include "globals.h"
#include "Options.h"
#include "MPIChannel.h"
#include "ContentInteractionDelegate.h"
#include "configuration/Configuration.h"
#include "GLWindow.h"
#include "config.h"
#include "log.h"

// Specialized delegate implementations
#include "PixelStreamInteractionDelegate.h"
#include "ZoomInteractionDelegate.h"
#if ENABLE_PDF_SUPPORT
#  include "PDFInteractionDelegate.h"
#endif

IMPLEMENT_SERIALIZE_FOR_XML(ContentWindowManager)

ContentWindowManager::ContentWindowManager()
    : interactionDelegate_( 0 )
{
}

ContentWindowManager::ContentWindowManager(ContentPtr content)
    : interactionDelegate_( 0 )
{
    setContent(content);

    adjustSize( SIZE_1TO1 );
}

ContentWindowManager::~ContentWindowManager()
{
    delete interactionDelegate_;
}

void ContentWindowManager::setContent(ContentPtr content)
{
    if(content_)
    {
        content_->disconnect(this, SLOT(setContentDimensions(int, int)));
        content_->disconnect(this, SIGNAL(modified()));
    }

    // set content object
    content_ = content;

    // content dimensions
    if(content_)
    {
        content_->getDimensions(contentWidth_, contentHeight_);

        // receive updates to content dimensions
        connect(content.get(), SIGNAL(dimensionsChanged(int, int)),
                this, SLOT(setContentDimensions(int, int)));

        // Notify DisplayGroup that the content was modified
        connect(content.get(), SIGNAL(modified()),
                this, SIGNAL(contentModified()));
    }
    else
        contentWidth_ = contentHeight_ = 0;

    createInteractionDelegate();
}

ContentPtr ContentWindowManager::getContent()
{
    return content_;
}

void ContentWindowManager::createInteractionDelegate()
{
    if (!g_mpiChannel || g_mpiChannel->getRank() != 0)
        return;

    delete interactionDelegate_;
    interactionDelegate_ = 0;

    if(!getContent())
        return;

    if (getContent()->getType() == CONTENT_TYPE_PIXEL_STREAM)
    {
        interactionDelegate_ = new PixelStreamInteractionDelegate(*this);
    }
#if ENABLE_PDF_SUPPORT
    else if (getContent()->getType() == CONTENT_TYPE_PDF)
    {
        interactionDelegate_ = new PDFInteractionDelegate(*this);
    }
#endif
    else
    {
        interactionDelegate_ = new ZoomInteractionDelegate(*this);
    }
}

DisplayGroupManagerPtr ContentWindowManager::getDisplayGroupManager()
{
    return displayGroupManager_.lock();
}

void ContentWindowManager::setDisplayGroupManager(DisplayGroupManagerPtr displayGroupManager)
{
    displayGroupManager_ = displayGroupManager;
}

ContentInteractionDelegate& ContentWindowManager::getInteractionDelegate()
{
    return *interactionDelegate_;
}

void ContentWindowManager::moveToFront(ContentWindowInterface * source)
{
    ContentWindowInterface::moveToFront(source);

    if(source != this)
    {
        DisplayGroupManagerPtr dgm = getDisplayGroupManager();
        if (dgm)
            dgm->moveContentWindowManagerToFront(shared_from_this());
        else
            put_flog(LOG_DEBUG, "The DisplayGroupMangerPtr is invalid");
    }
}

void ContentWindowManager::close(ContentWindowInterface * source)
{
    ContentWindowInterface::close(source);

    if(source != this)
    {
        getDisplayGroupManager()->removeContentWindowManager(shared_from_this());
    }
}

QPointF ContentWindowManager::getWindowCenterPosition() const
{
    return QPointF(coordinates_.x() + 0.5 * coordinates_.width(), coordinates_.y() + 0.5 * coordinates_.height());
}

void ContentWindowManager::centerPositionAround(const QPointF& position, const bool constrainToWindowBorders)
{
    if(position.isNull())
        return;

    double newX = position.x() - 0.5 * coordinates_.width();
    double newY = position.y() - 0.5 * coordinates_.height();

    if (constrainToWindowBorders)
    {
        if (newX + coordinates_.width() > 1.0)
            newX = 1.0-coordinates_.width();
        if (newY + coordinates_.height() > 1.0)
            newY = 1.0-coordinates_.height();

        newX = std::max(0.0, newX);
        newY = std::max(0.0, newY);
    }

    setPosition(newX, newY);
}

void ContentWindowManager::render()
{
    bool showWindowBorders = true;
    bool showZoomContext = false;

    if(g_configuration)
    {
        showWindowBorders = g_configuration->getOptions()->getShowWindowBorders();
        showZoomContext = g_configuration->getOptions()->getShowZoomContext();
    }

    content_->render(shared_from_this(), showZoomContext);

    if(showWindowBorders || selected())
    {
        double horizontalBorder = 5. / (double)g_configuration->getTotalHeight(); // 5 pixels

        // enlarge the border if we're highlighted
        if(getHighlighted())
            horizontalBorder *= 4.;

        double verticalBorder = (double)g_configuration->getTotalHeight() /
                                (double)g_configuration->getTotalWidth() * horizontalBorder;

        glPushAttrib(GL_CURRENT_BIT);

        // color the border based on window state
        if(selected())
            glColor4f(1,0,0,1);
        else
            glColor4f(1,1,1,1);

        GLWindow::drawRectangle(coordinates_.x()-verticalBorder, coordinates_.y()-horizontalBorder,
                                coordinates_.width()+2.*verticalBorder, coordinates_.height()+2.*horizontalBorder);

        glPopAttrib();
    }

#if 0	// not needed for multitouch
    glPushAttrib(GL_CURRENT_BIT);

    // render buttons if any of the markers are over the window
    bool markerOverWindow = false;

    MarkerPtrs markers = getDisplayGroupManager()->getMarkers();

    for(unsigned int i=0; i<markers.size(); i++)
    {
        // don't consider inactive markers
        if(markers[i]->getActive() == false)
        {
            continue;
        }

        float markerX, markerY;
        markers[i]->getPosition(markerX, markerY);

        if(QRectF(x_, y_, w_, h_).contains(markerX, markerY) == true)
        {
            markerOverWindow = true;
            break;
        }
    }

    if(markerOverWindow == true)
    {
        // we need this to be slightly in front of the rest of the window
        glPushMatrix();
        glTranslatef(0,0,0.001);

        // button dimensions
        float buttonWidth, buttonHeight;
        getButtonDimensions(buttonWidth, buttonHeight);

        // draw close button
        QRectF closeRect(x_ + w_ - buttonWidth, y_, buttonWidth, buttonHeight);

        // semi-transparent background
        glColor4f(1,0,0,0.125);

        glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBegin(GL_QUADS);
        glVertex2f(closeRect.x(), closeRect.y());
        glVertex2f(closeRect.x()+closeRect.width(), closeRect.y());
        glVertex2f(closeRect.x()+closeRect.width(), closeRect.y()+closeRect.height());
        glVertex2f(closeRect.x(), closeRect.y()+closeRect.height());
        glEnd();

        glPopAttrib();

        glColor4f(1,0,0,1);

        glBegin(GL_LINE_LOOP);
        glVertex2f(closeRect.x(), closeRect.y());
        glVertex2f(closeRect.x()+closeRect.width(), closeRect.y());
        glVertex2f(closeRect.x()+closeRect.width(), closeRect.y()+closeRect.height());
        glVertex2f(closeRect.x(), closeRect.y()+closeRect.height());
        glEnd();

        glBegin(GL_LINES);
        glVertex2f(closeRect.x(), closeRect.y());
        glVertex2f(closeRect.x() + closeRect.width(), closeRect.y() + closeRect.height());
        glVertex2f(closeRect.x()+closeRect.width(), closeRect.y());
        glVertex2f(closeRect.x(), closeRect.y() + closeRect.height());
        glEnd();

        // resize indicator
        QRectF resizeRect(x_ + w_ - buttonWidth, y_ + h_ - buttonHeight, buttonWidth, buttonHeight);

        // semi-transparent background
        glColor4f(0.5,0.5,0.5,0.25);

        glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBegin(GL_QUADS);
        glVertex2f(resizeRect.x(), resizeRect.y());
        glVertex2f(resizeRect.x()+resizeRect.width(), resizeRect.y());
        glVertex2f(resizeRect.x()+resizeRect.width(), resizeRect.y()+resizeRect.height());
        glVertex2f(resizeRect.x(), resizeRect.y()+resizeRect.height());
        glEnd();

        glPopAttrib();

        glColor4f(0.5,0.5,0.5,1);

        glBegin(GL_LINE_LOOP);
        glVertex2f(resizeRect.x(), resizeRect.y());
        glVertex2f(resizeRect.x()+resizeRect.width(), resizeRect.y());
        glVertex2f(resizeRect.x()+resizeRect.width(), resizeRect.y()+resizeRect.height());
        glVertex2f(resizeRect.x(), resizeRect.y()+resizeRect.height());
        glEnd();

        glBegin(GL_LINES);
        glVertex2f(resizeRect.x()+resizeRect.width(), resizeRect.y());
        glVertex2f(resizeRect.x(), resizeRect.y() + resizeRect.height());
        glEnd();

        glPopMatrix();
    }

    glPopAttrib();
#endif
}
