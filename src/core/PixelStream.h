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

#ifndef PIXEL_STREAM_H
#define PIXEL_STREAM_H

#include "FactoryObject.h"
#include "PixelStreamSegment.h"
#include "types.h"

#include <QRectF>
#include <QString>
#include <boost/shared_ptr.hpp>
#include <vector>

class PixelStreamSegmentRenderer;
class PixelStreamSegmentDecoder;
typedef boost::shared_ptr<PixelStreamSegmentDecoder> PixelStreamSegmentDecoderPtr;
typedef boost::shared_ptr<PixelStreamSegmentRenderer> PixelStreamSegmentRendererPtr;

class PixelStream : public FactoryObject
{
public:
    PixelStream(const QString& uri);

    void getDimensions(int &width, int &height) const;

    void preRenderUpdate(const QRectF& windowRect);
    void render(const QRectF& texCoords);

    void insertNewFrame(const PixelStreamSegments& segments);

private:
    // pixel stream identifier
    QString uri_;

    // dimensions of entire pixel stream
    unsigned int width_;
    unsigned int height_;

    // The front buffer is decoded by the frameDecoders and then used to upload the frameRenderers
    PixelStreamSegments frontBuffer_;
    // The back buffer contains the next frame to process (last frame received)
    PixelStreamSegments backBuffer_;
    bool buffersSwapped_;

    // The list of decoded images for the next frame
    std::vector<PixelStreamSegmentDecoderPtr> frameDecoders_;

    // For each segment, object for image decoding, rendering and storing parameters
    std::vector<PixelStreamSegmentRendererPtr> segmentRenderers_;

    // The coordinates of the ContentWindow of this PixelStream
    QRectF contentWindowRect_;

    void updateRenderers(const PixelStreamSegments& segments);
    void updateVisibleTextures(const QRectF& windowRect);
    void swapBuffers();
    void recomputeDimensions(const PixelStreamSegments& segments);
    void decodeVisibleTextures(const QRectF& windowRect);

    void adjustFrameDecodersCount(const size_t count);
    void adjustSegmentRendererCount(const size_t count);

    bool isDecodingInProgress();

    bool isVisible(const QRect& segment, const QRectF& windowRect);
    bool isVisible(const PixelStreamSegment& segment, const QRectF& windowRect);
};


#endif
