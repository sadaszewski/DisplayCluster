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

#include "MainWindow.h"

#include "log.h"
#include "globals.h"
#include "Options.h"
#include "MPIChannel.h"
#include "configuration/WallConfiguration.h"

#include "DisplayGroupManager.h"
#include "GLWindow.h"

MainWindow::MainWindow(const WallConfiguration* configuration)
{
    setupWallOpenGLWindows(configuration);

    // setup connection so updateGLWindows() will be called continuously must be
    // queued so we return to the main event loop and avoid infinite recursion
    connect(this, SIGNAL(updateGLWindowsFinished()),
            this, SLOT(updateGLWindows()), Qt::QueuedConnection);

    // trigger the first update
    updateGLWindows();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupWallOpenGLWindows(const WallConfiguration* configuration)
{
    // if we have just one tile for this process, make the GL window the central widget
    // otherwise, create multiple windows
    if(configuration->getScreenCount() == 1)
    {
        move(configuration->getScreenPosition(0));
        resize(configuration->getScreenWidth(), configuration->getScreenHeight());

        GLWindowPtr glw(new GLWindow(0));
        glWindows_.push_back(glw);

        setCentralWidget(glw.get());

        if(configuration->getFullscreen())
            showFullScreen();
        else
            show();
    }
    else
    {
        for(int i=0; i<configuration->getScreenCount(); i++)
        {
            QRect windowRect = QRect(configuration->getScreenPosition(i), QSize(configuration->getScreenWidth(), configuration->getScreenHeight()));

            // setup shared OpenGL contexts
            GLWindow * shareWidget = NULL;

            if(i > 0)
                shareWidget = glWindows_[0].get();

            GLWindowPtr glw(new GLWindow(i, windowRect, shareWidget));
            glWindows_.push_back(glw);

            if(configuration->getFullscreen())
                glw->showFullScreen();
            else
                glw->show();
        }
    }
}

GLWindowPtr MainWindow::getGLWindow(const int index)
{
    return glWindows_[index];
}

GLWindowPtr MainWindow::getActiveGLWindow()
{
    return activeGLWindow_;
}

bool MainWindow::isRegionVisible(const QRectF& region) const
{
    for(unsigned int i=0; i<glWindows_.size(); i++)
    {
        if(glWindows_[i]->isRegionVisible(region))
            return true;
    }
    return false;
}

void MainWindow::updateGLWindows()
{
    // receive any waiting messages
    g_mpiChannel->receiveMessages(getGLWindow()->getPixelStreamFactory());

    // synchronize clock right after receiving messages to ensure we have an
    // accurate time for rendering, etc. below
    g_mpiChannel->synchronizeClock();

    if( g_configuration->getOptions()->getShowMouseCursor( ))
        unsetCursor();
    else
        setCursor( QCursor( Qt::BlankCursor ));

    // render all GLWindows
    for(size_t i=0; i<glWindows_.size(); i++)
    {
        activeGLWindow_ = glWindows_[i];
        glWindows_[i]->updateGL();
    }

    // all render processes render simultaneously
    g_mpiChannel->globalBarrier();
    swapBuffers();

    g_displayGroupManager->advanceContents();

    clearStaleFactoryObjects();
    glWindows_[0]->purgeTextures();

    ++g_frameCount;

    emit(updateGLWindowsFinished());
}

void MainWindow::swapBuffers()
{
    for(size_t i=0; i<glWindows_.size(); i++)
        glWindows_[i]->swapBuffers();
}

void MainWindow::clearStaleFactoryObjects()
{
    glWindows_[0]->getTextureFactory().clearStaleObjects();
    glWindows_[0]->getDynamicTextureFactory().clearStaleObjects();
    glWindows_[0]->getPDFFactory().clearStaleObjects();
    glWindows_[0]->getSVGFactory().clearStaleObjects();
    glWindows_[0]->getMovieFactory().clearStaleObjects();
    glWindows_[0]->getPixelStreamFactory().clearStaleObjects();
    glWindows_[0]->getPDFFactory().clearStaleObjects();
}

void MainWindow::finalize()
{
    for(size_t i=0; i<glWindows_.size(); i++)
        glWindows_[i]->finalize();
}
