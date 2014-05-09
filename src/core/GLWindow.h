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

#ifndef GL_WINDOW_H
#define GL_WINDOW_H

#include <QGLWidget>
#include <QMutex>
#include <QList>

#include "types.h"
#include "FpsCounter.h"

class WallConfiguration;

/**
 * An OpenGL window used by Wall applications to render contents.
 */
class GLWindow : public QGLWidget
{
public:
    /**
     * Create a new window.
     * @param tileIndex A unique index associated with this window.
     * @param windowRect The position and dimensions for the window.
     * @param shareWidget An optional widget to share an existing GLContext.
     *                    A new GLContext is allocated if not provided.
     */
    GLWindow(const int tileIndex, QRect windowRect, QGLWidget* shareWidget = 0);
    ~GLWindow();

    /** Get the unique tile index identifier. */
    int getTileIndex() const;

    // TODO See if we can remove this method of deleting textures.
    // It is only used in DynamicTexture.
    /** Insert the id of a texture to be deleted. */
    void insertPurgeTextureId(GLuint textureId);
    /** Delete the textures added with insertPurgeTextureId()/ */
    void purgeTextures();

    /** Add a renderable, currently only supports DisplayGroupRenderer. */
    void addRenderable(DisplayGroupRendererPtr renderable);

    /**
     * Is the given region visible in this window.
     * @param rect The region in normalized global screen space, i.e. top-left
     *        of tiled display is (0,0) and bottom-right is (1,1)
     * @return true if (partially) visible, false otherwise
     */
    bool isRegionVisible(const QRectF& region) const;

    /** Used by PDF and SVG renderers */
    QRectF getProjectedPixelRect(const bool clampToWindowArea) const;

    /** Draw an un-textured rectangle.
     * @param x,y postion
     * @param w,h dimensions
     */
    static void drawRectangle(double x, double y, double w, double h);

protected:
    ///@{
    /** Overloaded methods from QGLWidget */
    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int w, int h);
    ///@}

private:
    const WallConfiguration* configuration_;

    int tileIndex_;

    // Postion and dimensions of the GLWindow in normalized Wall coordinates
    double left_;
    double right_;
    double bottom_;
    double top_;

    // mutex and vector of texture id's to purge
    // this allows other threads to trigger deletion of a texture during
    // the main OpenGL thread execution
    QMutex purgeTexturesMutex_;
    std::vector<GLuint> purgeTextureIds_;

    FpsCounter fpsCounter_;

    QList<DisplayGroupRendererPtr> renderables_;

    void clear(const QColor& clearColor);
    void setOrthographicView();
#if ENABLE_SKELETON_SUPPORT
    bool setPerspectiveView(double x=0., double y=0., double w=1., double h=1.);
    void renderSkeletons(const std::vector< boost::shared_ptr<SkeletonState> >& skeletons);
#endif

    void renderTestPattern();
    void drawFps();
};

#endif
