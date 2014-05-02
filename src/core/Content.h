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

#ifndef CONTENT_H
#define CONTENT_H

#include "ContentFactory.h"
#include "ContentType.h"
#include "types.h"

#include <QtGui>

#include <boost/serialization/access.hpp>
#include <boost/serialization/assume_abstract.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/nvp.hpp>

class ContentWindowManager;
class Factories;

/**
 * An abstract Content displayed in a ContentWindowManager.
 *
 * This class does not actually hold any content data because it
 * is meant to be sent through MPI to Rank>0 processes.
 * The content data is held by FactoryObjects on Rank>0 processes.
 * A Content object references a FactoryObject of the same ContentType based on its URI.
 * It is possible for multiple Content objects to reference the same FactoryObject.
 */
class Content : public QObject
{
    Q_OBJECT

    public:
        /** Constructor **/
        Content(QString uri = "");

        /** Get the content URI **/
        const QString& getURI() const;

        /** Get the content type **/
        virtual CONTENT_TYPE getType() = 0;

        /**
         * Read content metadata from the data source.
         * Used on Rank0 for file-based content types to refresh data from source URI.
         * @return true if the informations could be read.
        **/
        virtual bool readMetadata() = 0;

        void getDimensions(int &width, int &height);
        void setDimensions(int width, int height);
        virtual void getFactoryObjectDimensions(FactoriesPtr factories,
                                                int &width, int &height) = 0;
        void blockAdvance( bool block ) { blockAdvance_ = block; }

        // virtual method for implementing actions on advancing to a new frame
        // useful when a process has multiple GLWindows
        virtual void advance(FactoriesPtr, ContentWindowManagerPtr) { }

        virtual void renderFactoryObject(FactoriesPtr factories,
                                         const QRectF& texCoords) = 0;
    signals:

        /** Emitted when dimensions have changed */
        void dimensionsChanged(int width, int height);

        /** Emitted by any Content subclass when its state has been modified */
        void modified();

    protected:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int)
        {
            ar & boost::serialization::make_nvp("uri", uri_);
            ar & boost::serialization::make_nvp("width", width_);
            ar & boost::serialization::make_nvp("height", height_);
            ar & boost::serialization::make_nvp("block_advance", blockAdvance_);
        }

        QString uri_;
        int width_;
        int height_;
        bool blockAdvance_;
};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Content)

// typedef needed for SIP
typedef ContentPtr pContent;

class pyContent {

    public:

        pyContent(const char * str)
        {
            ContentPtr c(ContentFactory::getContent(QString::fromAscii(str)));
            ptr_ = c;
        }

        pyContent(ContentPtr c)
        {
            ptr_ = c;
        }

        ContentPtr get()
        {
            return ptr_;
        }

        const char * getURI()
        {
            return ptr_->getURI().toAscii();
        }

    protected:

        ContentPtr ptr_;
};

#endif
