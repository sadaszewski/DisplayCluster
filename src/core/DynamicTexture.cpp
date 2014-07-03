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

#include "DynamicTexture.h"
#include "RenderContext.h"
#include "GLWindow.h"
#include "log.h"

#include <fstream>
#include <boost/tokenizer.hpp>
#include <QDir>
#include <QImageReader>
#include <QtConcurrentRun>

#ifdef __APPLE__
    #include <OpenGL/glu.h>
#else
    #include <GL/glu.h>
#endif

#define TEXTURE_SIZE 512

#undef DYNAMIC_TEXTURE_SHOW_BORDER // define this to show borders around image tiles

#define PYRAMID_METADATA_FILE_EXTENSION    "pyr"
#define PYRAMID_METADATA_FILE_NAME         "pyramid.pyr"
#define PYRAMID_FOLDER_SUFFIX              ".pyramid/"
#define IMAGE_EXTENSION                    "jpg"

const QString DynamicTexture::pyramidFileExtension = QString(PYRAMID_METADATA_FILE_EXTENSION);
const QString DynamicTexture::pyramidFolderSuffix = QString(PYRAMID_FOLDER_SUFFIX);

DynamicTexture::DynamicTexture(const QString& uri, DynamicTexturePtr parent,
                               const QRectF& parentCoordinates, const int childIndex)
    : uri_(uri)
    , useImagePyramid_(false)
    , threadCount_(0)
    , parent_(parent)
    , imageCoordsInParentImage_(parentCoordinates)
    , depth_(0)
    , loadImageThreadStarted_(false)
    , renderedChildren_(false)
{
    // if we're a child...
    if(parent)
    {
        setRenderContext(parent->getRenderContext());
        depth_ = parent->depth_ + 1;

        // append childIndex to parent's path to form this object's path
        treePath_ = parent->treePath_;
        treePath_.push_back(childIndex);
    }

    // if we're the top-level object
    if(isRoot())
    {
        // this is the top-level object, so its path is 0
        treePath_.push_back(0);

        // Try to read the pyramid metadata from file
        const QString extension = QString(".").append(pyramidFileExtension);
        if(uri_.endsWith(extension))
            readPyramidMetadataFromFile(uri_);
        // Read the whole image to get the size. There might be a more optimized solution.
        else
            loadFullResImage();
    }
}

DynamicTexture::~DynamicTexture()
{
}

bool DynamicTexture::isRoot() const
{
    return depth_ == 0;
}

bool DynamicTexture::readPyramidMetadataFromFile(const QString& uri)
{
    std::ifstream ifs(uri.toAscii());

    // read the whole line
    std::string lineString;
    getline(ifs, lineString);

    // parse the arguments, allowing escaped characters, quotes, etc., and assign them to a vector
    std::string separator1("\\"); // allow escaped characters
    std::string separator2(" "); // split on spaces
    std::string separator3("\"\'"); // allow quoted arguments

    boost::escaped_list_separator<char> els(separator1, separator2, separator3);
    boost::tokenizer<boost::escaped_list_separator<char> > tokenizer(lineString, els);

    std::vector<std::string> tokens;
    tokens.assign(tokenizer.begin(), tokenizer.end());

    if(tokens.size() != 3)
    {
        put_flog(LOG_ERROR, "require 3 arguments, got %i", tokens.size());
        return false;
    }

    imagePyramidPath_ = QString(tokens[0].c_str());

    imageSize_.setWidth(atoi(tokens[1].c_str()));
    imageSize_.setHeight(atoi(tokens[2].c_str()));

    useImagePyramid_ = true;

    put_flog(LOG_DEBUG, "got image pyramid path %s, imageWidth = %i, imageHeight = %i",
             imagePyramidPath_.toLocal8Bit().constData(), imageSize_.width(), imageSize_.height());

    return true;
}

bool DynamicTexture::writeMetadataFile(const QString& pyramidFolder, const QString& filename) const
{
    std::ofstream ofs(filename.toStdString().c_str());
    if(!ofs.good())
    {
        put_flog(LOG_WARN, "could not write second metadata file %s",
                 filename.toStdString().c_str());
        return false;
    }

    ofs << "\"" << pyramidFolder.toStdString() << "\" " << imageSize_.width()
        << " " << imageSize_.height();
    return true;
}

bool DynamicTexture::writePyramidMetadataFiles(const QString& pyramidFolder) const
{
    // First metadata file in the pyramid folder
    const QString metadataFilename = pyramidFolder + PYRAMID_METADATA_FILE_NAME;

    // Second more conveniently named metadata file in the same directory as the original image
    QString secondMetadataFilename = pyramidFolder;
    const int lastIndex = secondMetadataFilename.lastIndexOf(pyramidFolderSuffix);
    secondMetadataFilename.truncate(lastIndex);
    secondMetadataFilename.append(".").append(pyramidFileExtension);

    return writeMetadataFile(pyramidFolder, metadataFilename) &&
           writeMetadataFile(pyramidFolder, secondMetadataFilename);
}

QString DynamicTexture::getPyramidImageFilename() const
{
    QString filename;

    for(unsigned int i=0; i<treePath_.size(); i++)
    {
        filename.append(QString::number(treePath_[i]));

        if(i != treePath_.size() - 1)
            filename.append("-");
    }

    filename.append(".").append(IMAGE_EXTENSION);

    return filename;
}

void loadImageInThread(DynamicTexturePtr dynamicTexture)
{
    try
    {
        dynamicTexture->loadImage();
        dynamicTexture->decrementGlobalThreadCount();
    }
    catch(const boost::bad_weak_ptr&)
    {
        put_flog(LOG_INFO, "The parent image was deleted during image loading.");
    }
}

void DynamicTexture::loadImageAsync()
{
    // only start the thread if this DynamicTexture tree has one available
    // each DynamicTexture tree is limited to (maxThreads - 2) threads, where
    // the max is determined by the global QThreadPool instance
    // we increase responsiveness / interactivity by not queuing up image loading
    // const int maxThreads = std::max(QThreadPool::globalInstance()->maxThreadCount() - 2, 1);
    // todo: this doesn't perform well with too many threads; restricting to 1 thread for now
    const int maxThreads = 1;

    if(getGlobalThreadCount() < maxThreads)
    {
        loadImageThreadStarted_ = true;
        incrementGlobalThreadCount();
        loadImageThread_ = QtConcurrent::run(loadImageInThread, shared_from_this());
    }
}

bool DynamicTexture::loadFullResImage()
{
    if(!fullscaleImage_.load(uri_))
    {
        put_flog(LOG_ERROR, "error loading %s", uri_.toLocal8Bit().constData());
        return false;
    }
    imageSize_= fullscaleImage_.size();
    return true;
}

QImage DynamicTexture::loadImageRegionFromFullResImageFile(const QString& filename)
{
    QImageReader imageReader(filename);

    if(!imageReader.canRead())
    {
        put_flog(LOG_ERROR, "image cannot be read. aborting.");
        return QImage();
    }

    // get image rectangle for this object in the root's coordinates
    QRect rootRect;

    if(isRoot())
        rootRect = QRect(0,0, imageReader.size().width(), imageReader.size().height());
    else
        rootRect = getRootImageCoordinates(0., 0., 1., 1.);

    // save the image dimensions (in terms of the root image) that this object represents
    imageSize_ = rootRect.size();

    put_flog(LOG_DEBUG, "reading clipped region of image");

    imageReader.setClipRect(rootRect);
    QImage image = imageReader.read();

    if(!image.isNull())
        return image.scaled(TEXTURE_SIZE, TEXTURE_SIZE, Qt::KeepAspectRatio);

    put_flog(LOG_DEBUG, "failed to read clipped region of image; attempting to read clipped and scaled region of image");
    imageReader.setScaledSize(QSize(TEXTURE_SIZE, TEXTURE_SIZE));
    return imageReader.read();
}

void DynamicTexture::loadImage()
{
    if(isRoot())
    {
        if(useImagePyramid_)
        {
            scaledImage_.load(imagePyramidPath_+'/'+getPyramidImageFilename(), IMAGE_EXTENSION);
        }
        else
        {
            if (!fullscaleImage_.isNull() || loadFullResImage())
                scaledImage_ = fullscaleImage_.scaled(TEXTURE_SIZE, TEXTURE_SIZE, Qt::KeepAspectRatio);
        }
    }
    else
    {
        DynamicTexturePtr root = getRoot();

        if(root->useImagePyramid_)
        {
            scaledImage_.load(root->imagePyramidPath_+'/'+getPyramidImageFilename(), IMAGE_EXTENSION);
        }
        else
        {
            DynamicTexturePtr parent(parent_);
            const QImage image = parent->getImageFromParent(imageCoordsInParentImage_, this);

            // if we managed to get a valid image, go ahead and scale it
            if(!image.isNull())
            {
                imageSize_= image.size();
                scaledImage_ = image.scaled(TEXTURE_SIZE, TEXTURE_SIZE, Qt::KeepAspectRatio);
            }
            else
            {
                // try alternative methods of reading it using QImageReader
                scaledImage_ = loadImageRegionFromFullResImageFile(root->uri_);
            }
        }
    }

    if(scaledImage_.isNull())
    {
        put_flog(LOG_ERROR, "failed to load the image. aborting.");
        return;
    }
}

void DynamicTexture::getDimensions(int &width, int &height) const
{
    // if we don't have a width and height, and the load image thread is running, wait for it to finish
    if(imageSize_.isEmpty() && loadImageThreadStarted_)
    {
        loadImageThread_.waitForFinished();
    }

    width = imageSize_.width();
    height = imageSize_.height();
}

void DynamicTexture::render(const QRectF& texCoords)
{
    if(!isVisibleInCurrentGLView())
        return;

    if(canHaveChildren() && !isResolutionSufficientForCurrentGLView())
    {
        renderChildren(texCoords);
        renderedChildren_ = true;
        return;
    }

    // Normal rendering: load the texture if not already available
    if(!loadImageThreadStarted_)
        loadImageAsync();

    render_(texCoords);
}

void DynamicTexture::postRenderUpdate()
{
    // Root needs to always have a texture for renderInParent()
    if (isRoot() && !loadImageThreadStarted_)
        loadImageAsync();

    clearOldChildren();
    renderedChildren_ = false;
}

bool DynamicTexture::isVisibleInCurrentGLView()
{
    // TODO This objects visibility should be determined by using the GLWindow
    // as a pre-render step, not retro-fitted in here!
    const QRectF screenRect = renderContext_->getGLWindow()->getProjectedPixelRect(true);
    return screenRect.width()*screenRect.height() > 0.;
}

bool DynamicTexture::isResolutionSufficientForCurrentGLView()
{
    const QRectF fullRect = renderContext_->getGLWindow()->getProjectedPixelRect(false);
    return fullRect.width() <= TEXTURE_SIZE && fullRect.height() <= TEXTURE_SIZE;
}

bool DynamicTexture::canHaveChildren()
{
    return (getRoot()->imageSize_.width() / (1 << depth_) > TEXTURE_SIZE ||
            getRoot()->imageSize_.height() / (1 << depth_) > TEXTURE_SIZE);
}

void DynamicTexture::render_(const QRectF& texCoords)
{
    if(!texture_.isValid() && loadImageThreadStarted_ && loadImageThread_.isFinished())
        generateTexture();

    if(texture_.isValid())
    {
#ifdef DYNAMIC_TEXTURE_SHOW_BORDER
        renderTextureBorder();
#endif
        renderTexturedUnitQuad(texCoords);
    }
    else
    {
        // If we don't yet have a texture, try to render from parent's texture
        DynamicTexturePtr parent = parent_.lock();
        if(parent)
            parent->render_(getImageRegionInParentImage(texCoords));
    }
}

void DynamicTexture::renderTextureBorder()
{
    glPushAttrib(GL_CURRENT_BIT);

    glColor4f(0.,1.,0.,1.);

    quad_.setEnableTexture(false);
    quad_.setRenderMode(GL_LINE_LOOP);
    quad_.render();

    glPopAttrib();
}

void DynamicTexture::renderTexturedUnitQuad(const QRectF& texCoords)
{
    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);

    texture_.bind();

    quad_.setEnableTexture(true);
    quad_.setTexCoords(texCoords);
    quad_.setRenderMode(GL_QUADS);
    quad_.render();

    glPopAttrib();
}

void DynamicTexture::clearOldChildren()
{
    if(!renderedChildren_ && !children_.empty() && getThreadsDoneDescending())
        children_.clear();

    // run on my children (if i still have any)
    for(unsigned int i=0; i<children_.size(); i++)
        children_[i]->clearOldChildren();
}

bool DynamicTexture::makePyramidFolder(const QString& pyramidFolder)
{
    // make directory if necessary
    if(!QDir(pyramidFolder).exists())
    {
        if(!QDir().mkdir(pyramidFolder))
        {
            put_flog(LOG_ERROR, "error creating directory %s",
                     pyramidFolder.toLocal8Bit().constData());
            return false;
        }
    }
    return true;
}

bool DynamicTexture::generateImagePyramid(const QString& pyramidFolder)
{
    if(isRoot())
    {
        if(loadImageThreadStarted_)
            loadImageThread_.waitForFinished();

        if (!makePyramidFolder(pyramidFolder))
            return false;

        if (!writePyramidMetadataFiles(pyramidFolder))
            return false;
    }

    // load this object's scaledImage_
    loadImage();

    const QString filename = pyramidFolder + getPyramidImageFilename();
    put_flog(LOG_DEBUG, "saving %s", filename.toLocal8Bit().constData());

    if (!scaledImage_.save(filename, IMAGE_EXTENSION))
        return false;
    scaledImage_ = QImage(); // no longer need scaled image

    // if we need to descend further...
    if(canHaveChildren())
    {
        // image rectangle a child quadrant contains
        QRectF imageBounds[4];
        imageBounds[0] = QRectF(0.,0.,0.5,0.5);
        imageBounds[1] = QRectF(0.5,0.,0.5,0.5);
        imageBounds[2] = QRectF(0.5,0.5,0.5,0.5);
        imageBounds[3] = QRectF(0.,0.5,0.5,0.5);

        // generate and compute children
#pragma omp parallel for
        for(unsigned int i=0; i<4; i++)
        {
            DynamicTexturePtr child(new DynamicTexture("", shared_from_this(), imageBounds[i], i));

            child->generateImagePyramid(pyramidFolder);
        }
    }

    return true;
}

void DynamicTexture::decrementGlobalThreadCount()
{
    if(isRoot())
    {
        QMutexLocker locker(&threadCountMutex_);
        threadCount_ = threadCount_ - 1;
    }
    else
    {
        return getRoot()->decrementGlobalThreadCount();
    }
}

DynamicTexturePtr DynamicTexture::getRoot()
{
    if(isRoot())
        return shared_from_this();
    else
        return DynamicTexturePtr(parent_)->getRoot();
}

QRect DynamicTexture::getRootImageCoordinates(float x, float y, float w, float h)
{
    if(isRoot())
    {
        // if necessary, block and wait for image loading to complete
        if( loadImageThreadStarted_ && !loadImageThread_.isFinished( ))
            loadImageThread_.waitForFinished();

        return QRect(x*imageSize_.width(), y*imageSize_.height(),
                     w*imageSize_.width(), h*imageSize_.height());
    }
    else
    {
        DynamicTexturePtr parent = parent_.lock();

        float pX = imageCoordsInParentImage_.x() + x * imageCoordsInParentImage_.width();
        float pY = imageCoordsInParentImage_.y() + y * imageCoordsInParentImage_.height();
        float pW = w * imageCoordsInParentImage_.width();
        float pH = h * imageCoordsInParentImage_.height();

        return parent->getRootImageCoordinates(pX, pY, pW, pH);
    }
}

QRectF DynamicTexture::getImageRegionInParentImage(const QRectF& imageRegion) const
{
    QRectF parentRegion;

    parentRegion.setX(imageCoordsInParentImage_.x() + imageRegion.x() * imageCoordsInParentImage_.width());
    parentRegion.setY(imageCoordsInParentImage_.y() + imageRegion.y() * imageCoordsInParentImage_.height());
    parentRegion.setWidth(imageRegion.width() * imageCoordsInParentImage_.width());
    parentRegion.setHeight(imageRegion.height() * imageCoordsInParentImage_.height());

    return parentRegion;
}

QImage DynamicTexture::getImageFromParent(const QRectF& imageRegion, DynamicTexture * start)
{
    // if we're in the starting node, we must ascend
    if(start == this)
    {
        if(isRoot())
        {
            put_flog(LOG_ERROR, "starting from root object and cannot ascend");
            return QImage();
        }

        DynamicTexturePtr parent = parent_.lock();
        return parent->getImageFromParent(getImageRegionInParentImage(imageRegion), this);
    }

    // wait for the load image thread to complete if it's in progress
    if(loadImageThreadStarted_ && !loadImageThread_.isFinished())
        loadImageThread_.waitForFinished();

    if(!fullscaleImage_.isNull())
    {
        // we have a valid image, return the clipped image
        return fullscaleImage_.copy(imageRegion.x()*fullscaleImage_.width(),
                                    imageRegion.y()*fullscaleImage_.height(),
                                    imageRegion.width()*fullscaleImage_.width(),
                                    imageRegion.height()*fullscaleImage_.height());
    }
    else
    {
        // we don't have a valid image
        // if we're the root object, return an empty image
        // otherwise, continue up the tree looking for an image
        if(isRoot())
            return QImage();

        DynamicTexturePtr parent = parent_.lock();
        return parent->getImageFromParent(getImageRegionInParentImage(imageRegion), start);
    }
}

void DynamicTexture::generateTexture()
{
    texture_.init(scaledImage_, GL_BGRA);

    // no longer need the scaled image
    scaledImage_ = QImage();
}

void DynamicTexture::renderChildren(const QRectF& texCoords)
{
    // children rectangles
    const float inf = 1000000.;

    // texture rectangle a child quadrant may contain
    QRectF textureBounds[4];
    textureBounds[0].setCoords(-inf,-inf, 0.5,0.5);
    textureBounds[1].setCoords(0.5,-inf, inf,0.5);
    textureBounds[2].setCoords(0.5,0.5, inf,inf);
    textureBounds[3].setCoords(-inf,0.5, 0.5,inf);

    // image rectange a child quadrant contains
    QRectF imageBounds[4];
    imageBounds[0] = QRectF(0.,0.,0.5,0.5);
    imageBounds[1] = QRectF(0.5,0.,0.5,0.5);
    imageBounds[2] = QRectF(0.5,0.5,0.5,0.5);
    imageBounds[3] = QRectF(0.,0.5,0.5,0.5);

    // see if we need to generate children
    if(children_.empty())
    {
        for(unsigned int i=0; i<4; i++)
        {
            DynamicTexturePtr child(new DynamicTexture("", shared_from_this(), imageBounds[i], i));
            children_.push_back(child);
        }
    }

    // render children
    for(unsigned int i=0; i<children_.size(); i++)
    {
        // portion of texture for this child
        const QRectF childTextureRect = texCoords.intersected(textureBounds[i]);

        // translate and scale to child texture coordinates
        const QRectF childTextureRectTranslated = childTextureRect.translated(-imageBounds[i].x(), -imageBounds[i].y());

        const QRectF childTextureRectTranslatedAndScaled(childTextureRectTranslated.x() / imageBounds[i].width(),
                                                         childTextureRectTranslated.y() / imageBounds[i].height(),
                                                         childTextureRectTranslated.width() / imageBounds[i].width(),
                                                         childTextureRectTranslated.height() / imageBounds[i].height());

        // find rendering position based on portion of textureRect we occupy
        // recall the parent object (this one) is rendered as a (0,0,1,1) rectangle
        const QRectF renderRect((childTextureRect.x()-texCoords.x()) / texCoords.width(),
                                (childTextureRect.y()-texCoords.y()) / texCoords.height(),
                                childTextureRect.width() / texCoords.width(),
                                childTextureRect.height() / texCoords.height());

        glPushMatrix();
        glTranslatef(renderRect.x(), renderRect.y(), 0.);
        glScalef(renderRect.width(), renderRect.height(), 1.);

        children_[i]->render(childTextureRectTranslatedAndScaled);

        glPopMatrix();
    }
}

bool DynamicTexture::getThreadsDoneDescending()
{
    if(!loadImageThread_.isFinished())
        return false;

    for(unsigned int i=0; i<children_.size(); i++)
    {
        if(!children_[i]->getThreadsDoneDescending())
            return false;
    }

    return true;
}

int DynamicTexture::getGlobalThreadCount()
{
    if(isRoot())
    {
        QMutexLocker locker(&threadCountMutex_);
        return threadCount_;
    }
    else
    {
        return getRoot()->getGlobalThreadCount();
    }
}

void DynamicTexture::incrementGlobalThreadCount()
{
    if(isRoot())
    {
        QMutexLocker locker(&threadCountMutex_);
        threadCount_ = threadCount_ + 1;
    }
    else
    {
        return getRoot()->incrementGlobalThreadCount();
    }
}
