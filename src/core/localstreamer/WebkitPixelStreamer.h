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

#ifndef WEBKITPIXELSTREAMER_H
#define WEBKITPIXELSTREAMER_H

#include "PixelStreamer.h"

#include <QString>
#include <QImage>
#include <QTimer>
#include <QWebView>
#include <QMutex>

#include <boost/smart_ptr/scoped_ptr.hpp>

class QRect;
class QWebHitTestResult;
class QWebElement;

class WebkitAuthenticationHelper;
class WebkitHtmlSelectReplacer;

/**
 * Stream webpages with user interaction support.
 */
class WebkitPixelStreamer : public PixelStreamer
{
    Q_OBJECT

public:
    /**
     * Constructor.
     *
     * @param webpageSize The desired size of the webpage viewport. The actual stream
     *        dimensions will be: size * default zoom factor (2x).
     * @param url The webpage to load.
     */
    WebkitPixelStreamer(const QSize& webpageSize, const QString& url);

    /** Destructor. */
    ~WebkitPixelStreamer();

    /** Get the size of the webpage images. */
    QSize size() const override;

    /**
     * Open a webpage.
     *
     * @param url The address of the webpage to load.
     */
    void setUrl(QString url);

    /** Get the QWebView used internally by the streamer. */
    const QWebView* getView() const;

public slots:
    /** Process an Event. */
    void processEvent(dc::Event event) override;

private slots:
    void update();

private:
    QWebView webView_;
    boost::scoped_ptr<WebkitAuthenticationHelper> authenticationHelper_;
    boost::scoped_ptr<WebkitHtmlSelectReplacer> selectReplacer_;
    QTimer timer_;
    QMutex mutex_;

    QImage image_;

    bool interactionModeActive_;

    unsigned int initialWidth_;

    void processClickEvent(const Event &clickEvent);
    void processPressEvent(const Event &pressEvent);
    void processMoveEvent(const Event &moveEvent);
    void processReleaseEvent(const Event &releaseEvent);
    void processWheelEvent(const Event &wheelEvent);
    void processKeyPress(const Event &keyEvent);
    void processKeyRelease(const Event &keyEvent);
    void processViewSizeChange(const Event &sizeEvent);

    QWebHitTestResult performHitTest(const Event &dcEvent) const;
    QPoint getPointerPosition(const Event &dcEvent) const;
    bool isWebGLElement(const QWebElement &element) const;
    void setSize(const QSize& webpageSize);
    void recomputeZoomFactor();
};

#endif // WEBKITPIXELSTREAMER_H
