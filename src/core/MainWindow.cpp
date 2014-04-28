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

#include "configuration/WallConfiguration.h"
#include "GLWindow.h"

MainWindow::MainWindow(const WallConfiguration* configuration)
{
    setupWallOpenGLWindows(configuration);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupWallOpenGLWindows(const WallConfiguration* configuration)
{
    for(int i=0; i<configuration->getScreenCount(); ++i)
    {
        QRect windowRect = QRect(configuration->getScreenPosition(i),
                                 QSize(configuration->getScreenWidth(),
                                       configuration->getScreenHeight()));

        // share OpenGL context from the first GLWindow
        GLWindow* shareWidget = (i==0) ? 0 : glWindows_[0].get();

        GLWindowPtr glw(new GLWindow(i, windowRect, shareWidget));
        glWindows_.push_back(glw);

        if(configuration->getFullscreen())
            glw->showFullScreen();
        else
            glw->show();
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

Factory<Texture> & MainWindow::getTextureFactory()
{
    return textureFactory_;
}

Factory<DynamicTexture> & MainWindow::getDynamicTextureFactory()
{
    return dynamicTextureFactory_;
}

Factory<PDF> & MainWindow::getPDFFactory()
{
    return pdfFactory_;
}

Factory<SVG> & MainWindow::getSVGFactory()
{
    return svgFactory_;
}

Factory<Movie> & MainWindow::getMovieFactory()
{
    return movieFactory_;
}

Factory<PixelStream> & MainWindow::getPixelStreamFactory()
{
    return pixelStreamFactory_;
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

void MainWindow::updateGLWindows(DisplayGroupManagerPtr displayGroup)
{
    for(size_t i=0; i<glWindows_.size(); i++)
    {
        activeGLWindow_ = glWindows_[i];
        glWindows_[i]->setDisplayGroup(displayGroup);
        glWindows_[i]->updateGL();
    }
}

void MainWindow::swapBuffers()
{
    for(size_t i=0; i<glWindows_.size(); i++)
        glWindows_[i]->swapBuffers();
}

void MainWindow::clearStaleFactoryObjects()
{
    textureFactory_.clearStaleObjects();
    dynamicTextureFactory_.clearStaleObjects();
    pdfFactory_.clearStaleObjects();
    svgFactory_.clearStaleObjects();
    movieFactory_.clearStaleObjects();
    pixelStreamFactory_.clearStaleObjects();
    pdfFactory_.clearStaleObjects();

    glWindows_[0]->purgeTextures();
}

void MainWindow::finalize()
{
    textureFactory_.clear();
    dynamicTextureFactory_.clear();
    pdfFactory_.clear();
    svgFactory_.clear();
    movieFactory_.clear();
    pixelStreamFactory_.clear();
    pdfFactory_.clear();

    glWindows_[0]->purgeTextures();
}
