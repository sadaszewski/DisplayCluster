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

#include "GLWindow.h"

#include "log.h"
#include "globals.h"
#include "configuration/WallConfiguration.h"
#include "Options.h"
#include "Renderable.h"

#include <QtOpenGL>
#include <boost/shared_ptr.hpp>

#ifdef __APPLE__
    #include <OpenGL/glu.h>
#else
    #include <GL/glu.h>
#endif

GLWindow::GLWindow(const int tileIndex, QRect windowRect, QGLWidget* shareWidget)
  : QGLWidget(0, shareWidget)
  , configuration_(static_cast<WallConfiguration*>(g_configuration))
  , tileIndex_(tileIndex)
  , left_(0)
  , right_(0)
  , bottom_(0)
  , top_(0)
{
    setGeometry(windowRect);

    if(shareWidget && !isSharing())
    {
        put_flog(LOG_FATAL, "failed to share OpenGL context");
        exit(-1);
    }

    setAutoBufferSwap(false);
}

GLWindow::~GLWindow()
{
}

int GLWindow::getTileIndex() const
{
    return tileIndex_;
}

void GLWindow::insertPurgeTextureId(GLuint textureId)
{
    QMutexLocker locker(&purgeTexturesMutex_);

    purgeTextureIds_.push_back(textureId);
}

void GLWindow::purgeTextures()
{
    QMutexLocker locker(&purgeTexturesMutex_);

    for(size_t i=0; i<purgeTextureIds_.size(); ++i)
        deleteTexture(purgeTextureIds_[i]);

    purgeTextureIds_.clear();
}

void GLWindow::addRenderable(RenderablePtr renderable)
{
    renderables_.append(renderable);
}

void GLWindow::setTestPattern(RenderablePtr testPattern)
{
    testPattern_ = testPattern;
}

void GLWindow::initializeGL()
{
    // enable depth testing; disable lighting
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
}

void GLWindow::paintGL()
{
    OptionsPtr options = configuration_->getOptions();

    clear(options->getBackgroundColor());
    setOrthographicView();

    if(options->getShowTestPattern())
    {
        testPattern_->render();
        return;
    }

    foreach (RenderablePtr renderable, renderables_) {
        renderable->render();
    }

    if (options->getShowStreamingStatistics())
        drawFps();
}

void GLWindow::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    update();
}

void GLWindow::clear(const QColor& clearColor)
{
    glClearColor(clearColor.redF(), clearColor.greenF(), clearColor.blueF(), clearColor.alpha());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLWindow::setOrthographicView()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // invert y-axis to put origin at lower-left corner
    glScalef(1.,-1.,1.);

    // tiled display parameters
    double tileI = (double)configuration_->getGlobalScreenIndex(tileIndex_).x();
    double numTilesX = (double)configuration_->getTotalScreenCountX();
    double screenWidth = (double)configuration_->getScreenWidth();
    double mullionWidth = (double)configuration_->getMullionWidth();

    double tileJ = (double)configuration_->getGlobalScreenIndex(tileIndex_).y();
    double numTilesY = (double)configuration_->getTotalScreenCountY();
    double screenHeight = (double)configuration_->getScreenHeight();
    double mullionHeight = (double)configuration_->getMullionHeight();

    // border calculations
    left_ = tileI / numTilesX * ( numTilesX * screenWidth ) + tileI * mullionWidth;
    right_ = left_ + screenWidth;
    bottom_ = tileJ / numTilesY * ( numTilesY * screenHeight ) + tileJ * mullionHeight;
    top_ = bottom_ + screenHeight;

    // normalize to 0->1
    double totalWidth = (double)configuration_->getTotalWidth();
    double totalHeight = (double)configuration_->getTotalHeight();

    left_ /= totalWidth;
    right_ /= totalWidth;
    bottom_ /= totalHeight;
    top_ /= totalHeight;

    gluOrtho2D(left_, right_, bottom_, top_);
    glPushMatrix();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

}

bool GLWindow::isRegionVisible(const QRectF& region) const
{
    const QRectF screenRect(left_, bottom_, right_-left_, top_-bottom_);

    return screenRect.intersects(region);
}

void GLWindow::drawFps()
{
    fpsCounter_.tick();

    const int fontSize = 32;
    QFont textFont;
    textFont.setPixelSize(fontSize);

    glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);

    glDisable(GL_DEPTH_TEST);
    glColor4f(0.,0.,1.,1.);

    renderText(10, fontSize, fpsCounter_.toString(), textFont);

    glPopAttrib();
}

QRectF GLWindow::getProjectedPixelRect(const bool clampToWindowArea) const
{
    // get four corners in object space (recall we're in normalized 0->1 dimensions)
    const double corners[4][3] =
    {
        {0.,0.,0.},
        {1.,0.,0.},
        {1.,1.,0.},
        {0.,1.,0.}
    };

    // get four corners in screen space
    GLdouble modelview[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);

    GLdouble projection[16];
    glGetDoublev(GL_PROJECTION_MATRIX, projection);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    GLdouble xWin[4][3];

    for(size_t i=0; i<4; i++)
    {
        gluProject(corners[i][0], corners[i][1], corners[i][2], modelview, projection, viewport, &xWin[i][0], &xWin[i][1], &xWin[i][2]);

        if( clampToWindowArea )
        {
            // clamp to on-screen portion
            if(xWin[i][0] < 0.)
                xWin[i][0] = 0.;

            if(xWin[i][0] > (double)width())
                xWin[i][0] = (double)width();

            if(xWin[i][1] < 0.)
                xWin[i][1] = 0.;

            if(xWin[i][1] > (double)height())
                xWin[i][1] = (double)height();
        }
    }
    const QPointF topleft( xWin[0][0], (double)height() - xWin[0][1] );
    const QPointF bottomright( xWin[2][0], (double)height() - xWin[2][1] );

    return QRectF( topleft, bottomright );
}
