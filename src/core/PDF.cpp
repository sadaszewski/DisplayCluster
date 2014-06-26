/*********************************************************************/
/* Copyright (c) 2013, EPFL/Blue Brain Project                       */
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

#include "PDF.h"

// detect Qt version
#if QT_VERSION >= 0x050000
#define POPPLER_QT5
#include <poppler-qt5.h>
#elif QT_VERSION >= 0x040000
#define POPPLER_QT4
#include <poppler-qt4.h>
#else
#error PopplerPixelStreamer needs Qt4 or Qt5
#endif

#include "RenderContext.h"
#include "GLWindow.h"
#include "log.h"

#define INVALID_PAGE_NUMBER -1

PDF::PDF(const QString& uri)
    : uri_(uri)
    , pdfDoc_(0)
    , pdfPage_(0)
    , pageNumber_(INVALID_PAGE_NUMBER)
{
    openDocument(uri_);
}

PDF::~PDF()
{
    closeDocument();
}

bool PDF::isValid() const
{
    return (pdfDoc_ != 0);
}

void PDF::closePage()
{
    if (pdfPage_)
    {
        delete pdfPage_;
        pdfPage_ = 0;
        pageNumber_ = INVALID_PAGE_NUMBER;
        textureRect_ = QRect();
    }
}

void PDF::closeDocument()
{
    if (pdfDoc_)
    {
        closePage();
        delete pdfDoc_;
        pdfDoc_ = 0;
    }
}

void PDF::openDocument(const QString& filename)
{
    closeDocument();

    pdfDoc_ = Poppler::Document::load(filename);
    if (!pdfDoc_ || pdfDoc_->isLocked())
    {
        put_flog(LOG_DEBUG, "Could not open document %s", filename.toLocal8Bit().constData());
        closeDocument();
        return;
    }

    pdfDoc_->setRenderHint(Poppler::Document::TextAntialiasing);

    setPage(0);
}

bool PDF::isValid(const int pageNumber) const
{
    return pageNumber >=0 && pageNumber < pdfDoc_->numPages();
}

void PDF::setPage(const int pageNumber)
{
    if (pageNumber == pageNumber_ || !isValid(pageNumber))
        return;

    closePage();

    pdfPage_ = pdfDoc_->page(pageNumber); // Document starts at page 0
    if (!pdfPage_)
    {
        put_flog(LOG_DEBUG, "Could not open page %d", pageNumber);
        return;
    }

    pageNumber_ = pageNumber;
}

int PDF::getPageCount() const
{
    return pdfDoc_->numPages();
}

QImage PDF::renderToImage() const
{
    return pdfPage_->renderToImage();
}

void PDF::getDimensions(int &width, int &height) const
{
    width = pdfPage_ ? pdfPage_->pageSize().width() : 0;
    height = pdfPage_ ? pdfPage_->pageSize().height() : 0;
}

void PDF::render(const QRectF& texCoords)
{
    if (!pdfPage_)
        return;

    // get on-screen and full rectangle corresponding to the window
    const QRectF screenRect = renderContext_->getGLWindow()->getProjectedPixelRect(true);
    const QRectF fullRect = renderContext_->getGLWindow()->getProjectedPixelRect(false);

    // if we're not visible or we don't have a valid SVG, we're done...
    if(screenRect.isEmpty())
    {
        // TODO clear existing FBO for this OpenGL window
        return;
    }

    // generate texture corresponding to the visible part of these texture coordinates
    generateTexture(screenRect, fullRect, texCoords);

    if(!texture_.isValid())
        return;

    // figure out what visible region is for screenRect, a subregion of [0, 0, 1, 1]
    const float xp = (screenRect.x() - fullRect.x()) / fullRect.width();
    const float yp = (screenRect.y() - fullRect.y()) / fullRect.height();
    const float wp = screenRect.width() / fullRect.width();
    const float hp = screenRect.height() / fullRect.height();

    // Render the entire texture on a (scaled) unit textured quad
    glPushMatrix();

    glTranslatef(xp, yp, 0);
    glScalef(wp, hp, 1.f);

    drawUnitTexturedQuad();

    glPopMatrix();
}

void PDF::drawUnitTexturedQuad()
{
    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);

    texture_.bind();
    quad_.render();

    glPopAttrib();
}

void PDF::generateTexture(const QRectF screenRect, const QRectF fullRect, const QRectF& texCoords)
{
    // figure out the coordinates of the topLeft corner of the texture in the PDF page
    const double tXp = texCoords.x()/texCoords.width()*fullRect.width()  + (screenRect.x() - fullRect.x());
    const double tYp = texCoords.y()/texCoords.height()*fullRect.height() + (screenRect.y() - fullRect.y());

    // Compute the actual texture dimensions
    const QRect textureRect(tXp, tYp, screenRect.width(), screenRect.height());

    // TODO The instance of this class is shared between GLWindows, so the
    // texture is constantly regenerated because the size changes...
    if(textureRect == textureRect_)
        return; // no need to regenerate texture

    // Adjust the quality to match the actual displayed size
    // Multiply resolution by the zoom factor (1/t[W,H])
    const double resFactorX = fullRect.width() / pdfPage_->pageSize().width() / texCoords.width();
    const double resFactorY = fullRect.height() / pdfPage_->pageSize().height() / texCoords.height();

    // Generate a QImage of the rendered page
    QImage image = pdfPage_->renderToImage(72.0*resFactorX , 72.0*resFactorY,
                                            textureRect.x(), textureRect.y(),
                                            textureRect.width(), textureRect.height()
                                            );

    if (image.isNull())
    {
        put_flog(LOG_DEBUG, "Could not render pdf to image");
        return;
    }

    texture_.update(image, GL_BGRA);
    textureRect_ = textureRect; // keep rendered texture information so we know when to rerender
}

