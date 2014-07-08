/*********************************************************************/
/* Copyright (c) 2014, EPFL/Blue Brain Project                       */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
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

#ifndef GLTEXTURE2D_H
#define GLTEXTURE2D_H

#include <QtOpenGL/qgl.h>
#include <boost/noncopyable.hpp>

/**
 * A 2D GLTexture object.
 * All methods of this class must be called from the OpenGL thread.
 */
class GLTexture2D : public boost::noncopyable
{
public:
    /** Create an empty texture */
    GLTexture2D();

    /** Free the GLTexture. */
    ~GLTexture2D();

    /** Init the texture using the given image. */
    bool init(const QImage image, const GLenum format = GL_RGBA, bool mipmaps = false);

    /** Update the texture using the given image. */
    void update(const QImage image, const GLenum format = GL_RGBA);

    /**
     * Update the texture using the given image
     * @param data A buffer of getSize() dimensions with "format" bytes per pixels
     * @param format The image format of the data buffer
     */
    void update(const void* data, const GLenum format = GL_RGBA);

    /** Get the texture size. */
    QSize getSize() const;

    /** Bind the texture. */
    void bind();

    /** Is the texture valid. */
    bool isValid() const;

    /** Free the GLTexture. */
    void free();

private:
    GLuint textureId_;
    QSize size_;
};

#endif // GLTEXTURE2D_H
