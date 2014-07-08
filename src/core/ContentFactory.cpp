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

#include "ContentFactory.h"

#include "globals.h"
#include "configuration/Configuration.h"

#include "log.h"
#include "config.h"

#include "Content.h"
#include "TextureContent.h"
#include "DynamicTextureContent.h"
#include "SVGContent.h"
#include "MovieContent.h"
#if ENABLE_PDF_SUPPORT
#  include "PDFContent.h"
#  include "DisplayGroupManager.h"
#endif
#include "PixelStreamContent.h"

#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QTextStream>

#define ERROR_IMAGE_FILENAME ":/img/error.png"

ContentPtr ContentFactory::getContent(const QString& uri)
{
    // make sure file exists; otherwise use error image
    if( !QFile::exists( uri ))
    {
        put_flog(LOG_ERROR, "could not find file '%s'", uri.toLocal8Bit().constData());
        return getErrorContent();
    }

    // convert to lower case for case-insensitivity in determining file type
    const QString extension = QFileInfo(uri).suffix().toLower();

#if ENABLE_PDF_SUPPORT
    // See if this is a PDF document
    if(PDFContent::getSupportedExtensions().contains(extension))
    {
        ContentPtr content(new PDFContent(uri));

        if (!content->readMetadata())
            return getErrorContent();

        return content;
    }
#endif

    // see if this is an SVG image (must do this first, since SVG can also be read as an image directly)
    if(SVGContent::getSupportedExtensions().contains(extension))
    {
        ContentPtr content(new SVGContent(uri));

        if (!content->readMetadata())
            return getErrorContent();

        return content;
    }

    // see if this is an image
    QImageReader imageReader(uri);
    if(imageReader.canRead())
    {
        // get its size
        const QSize size = imageReader.size();

        if(!size.isValid())
            return getErrorContent();

        // small images will use Texture; larger images will use DynamicTexture
        ContentPtr content;
        if(size.width() <= g_configuration->getTotalWidth() && size.height() <= g_configuration->getTotalHeight())
            content = ContentPtr(new TextureContent(uri));
        else
            content = ContentPtr(new DynamicTextureContent(uri));

        content->setDimensions(size.width(), size.height());

        return content;
    }

    // see if this is an image pyramid
    if(extension == "pyr")
    {
        ContentPtr content(new DynamicTextureContent(uri));

        if (!content->readMetadata())
            return getErrorContent();

        return content;
    }

    // see if this is a movie
    if(MovieContent::getSupportedExtensions().contains(extension))
    {
        ContentPtr content(new MovieContent(uri));

        if (!content->readMetadata())
            return getErrorContent();

        return content;
    }

    put_flog(LOG_ERROR, "Unsupported or invalid file %s", uri.toLocal8Bit().constData());

    return getErrorContent();
}

ContentPtr ContentFactory::getPixelStreamContent(const QString& uri)
{
    return ContentPtr(new PixelStreamContent(uri));
}

ContentPtr ContentFactory::getErrorContent()
{
    return ContentPtr(new TextureContent(ERROR_IMAGE_FILENAME));
}

const QStringList& ContentFactory::getSupportedExtensions()
{
    static QStringList extensions;

    if (extensions.empty())
    {
#if ENABLE_PDF_SUPPORT
        extensions.append(PDFContent::getSupportedExtensions());
#endif
        extensions.append(SVGContent::getSupportedExtensions());
        extensions.append(TextureContent::getSupportedExtensions());
        extensions.append(DynamicTextureContent::getSupportedExtensions());
        extensions.append(MovieContent::getSupportedExtensions());
        extensions.removeDuplicates();
    }

    return extensions;
}

const QStringList& ContentFactory::getSupportedFilesFilter()
{
    static QStringList filters;

    if (filters.empty())
    {
        const QStringList& extensions = getSupportedExtensions();
        foreach( const QString ext, extensions )
            filters.append( "*." + ext );
    }

    return filters;
}

QString ContentFactory::getSupportedFilesFilterAsString()
{
    const QStringList& extensions = getSupportedFilesFilter();

    QString s;
    QTextStream out(&s);

    out << "Content files (";
    foreach( const QString ext, extensions )
        out << ext << " ";
    out << ")";

    return s;
}
