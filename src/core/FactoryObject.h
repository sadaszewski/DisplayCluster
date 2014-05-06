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

#ifndef FACTORY_OBJECT_H
#define FACTORY_OBJECT_H

#include <stdint.h>
class QRectF;
class RenderContext;

class FactoryObject
{
public:
    /** Constructor */
    FactoryObject();

    /** Destructor */
    virtual ~FactoryObject();

    /**
     * Get the dimensions of the full resolution texture.
     * @param width Returned width
     * @param height Returned height
     */
    virtual void getDimensions(int &width, int &height) const = 0;

    /**
     * Render the FactoryObject
     * @param textCoord The region of the texture to render
     */
    virtual void render(const QRectF& textCoord) = 0;

    /**
     * Set the render context to render the object on Rank 1-N
     * @param renderContext The render context
     */
    void setRenderContext(RenderContext* renderContext);

    /** Get the render context (only set on Rank 1-N) */
    RenderContext* getRenderContext() const;

    /**
     * Get the current frame index for this Object.
     * Used by the Factory to check if the object is still being used/referenced
     * by a ContentWindow.
     */
    uint64_t getFrameIndex() const;

    /**
     * Must be called everytime a derived object is rendered.
     * Failing that, it will be garbage collected by the factory.
     */
    void setFrameIndex(const uint64_t frameIndex);

protected:
    /** A reference to the render context. */
    RenderContext* renderContext_;

private:
    /** Frame index when object was last rendered. */
    uint64_t frameIndex_;
};

#endif
