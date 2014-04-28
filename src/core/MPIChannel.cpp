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

#include "MPIChannel.h"

#include "MessageHeader.h"
#include "DisplayGroupManager.h"
#include "Options.h"

#include "log.h"

#include <boost/serialization/vector.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>

// Will be removed when implementing DISCL-21
#include "ContentWindowManager.h"
#include "Content.h"

MPIChannel::MPIChannel(int argc, char * argv[])
    : mpiRank(-1)
    , mpiSize(-1)
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    MPI_Comm_split(MPI_COMM_WORLD, mpiRank != 0, mpiRank, &mpiRenderComm);
}

MPIChannel::~MPIChannel()
{
    MPI_Comm_free(&mpiRenderComm);
    MPI_Finalize();
}

int MPIChannel::getRank() const
{
    return mpiRank;
}

void MPIChannel::globalBarrier() const
{
    MPI_Barrier(mpiRenderComm);
}

int MPIChannel::globalSum(const int localValue) const
{
    int globalValue = 0;
    MPI_Allreduce((void *)&localValue, (void *)&globalValue,
                  1, MPI_INT, MPI_SUM, mpiRenderComm);
    return globalValue;
}

boost::posix_time::ptime MPIChannel::getTime() const
{
    // rank 0 will return a timestamp calibrated to rank 1's clock
    if(mpiRank == 0)
        return boost::posix_time::microsec_clock::universal_time() + timestampOffset_;

    return timestamp_;
}

void MPIChannel::calibrateTimestampOffset()
{
    if(mpiSize < 2)
    {
        put_flog(LOG_DEBUG, "minimum 2 processes needed! (mpiSize == %i)", mpiSize);
        return;
    }

    // synchronize all processes
    MPI_Barrier(mpiRenderComm);

    // get current timestamp immediately after
    boost::posix_time::ptime timestamp(boost::posix_time::microsec_clock::universal_time());

    // rank 1: send timestamp to rank 0
    if(mpiRank == 1)
    {
        // serialize state
        std::ostringstream oss(std::ostringstream::binary);

        // brace this so destructor is called on archive before we use the stream
        {
            boost::archive::binary_oarchive oa(oss);
            oa << timestamp;
        }

        // serialized data to string
        std::string serializedString = oss.str();
        int size = serializedString.size();

        // send the header and the message
        MessageHeader mh;
        mh.size = size;

        MPI_Send((void *)&mh, sizeof(MessageHeader), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
        MPI_Send((void *)serializedString.data(), size, MPI_BYTE, 0, 0, MPI_COMM_WORLD);
    }
    // rank 0: receive timestamp from rank 1
    else if(mpiRank == 0)
    {
        MessageHeader messageHeader;

        MPI_Status status;
        MPI_Recv((void *)&messageHeader, sizeof(MessageHeader), MPI_BYTE, 1, 0, MPI_COMM_WORLD, &status);

        // receive serialized data
        std::vector<char> buffer(messageHeader.size);

        // read message into the buffer
        MPI_Recv((void *)buffer.data(), messageHeader.size, MPI_BYTE, 1, 0, MPI_COMM_WORLD, &status);

        // de-serialize...
        std::istringstream iss(std::istringstream::binary);

        if(iss.rdbuf()->pubsetbuf(buffer.data(), messageHeader.size) == NULL)
        {
            put_flog(LOG_FATAL, "rank %i: error setting stream buffer", mpiRank);
            return;
        }

        // read to a new timestamp
        boost::posix_time::ptime rank1Timestamp;

        boost::archive::binary_iarchive ia(iss);
        ia >> rank1Timestamp;

        // now, calculate and store the timestamp offset
        timestampOffset_ = rank1Timestamp - timestamp;

        put_flog(LOG_DEBUG, "timestamp offset = %s", (boost::posix_time::to_simple_string(timestampOffset_)).c_str());
    }
}

void MPIChannel::receiveMessages(Factory<PixelStream>& pixelStreamFactory)
{
    if(mpiRank == 0)
    {
        put_flog(LOG_FATAL, "called on rank 0");
        return;
    }

    // check to see if we have a message (non-blocking)
    int flag;
    MPI_Status status;
    MPI_Iprobe(0, 0, MPI_COMM_WORLD, &flag, &status);

    // check to see if all render processes have a message
    int allFlag;
    MPI_Allreduce(&flag, &allFlag, 1, MPI_INT, MPI_LAND, mpiRenderComm);

    // message header
    MessageHeader mh;

    // if all render processes have a message...
    if(allFlag != 0)
    {
        // continue receiving messages until we get to the last one which all render processes have
        // this will "drop frames" and keep all processes synchronized
        while(allFlag)
        {
            // first, get message header
            MPI_Recv((void *)&mh, sizeof(MessageHeader), MPI_BYTE, 0, 0, MPI_COMM_WORLD, &status);

            if(mh.type == MESSAGE_TYPE_CONTENTS)
            {
                displayGroup_ = receiveDisplayGroup(mh);
                emit(received(displayGroup_));
            }
            else if(mh.type == MESSAGE_TYPE_OPTIONS)
            {
                emit(received(receiveOptions(mh)));
            }
            else if(mh.type == MESSAGE_TYPE_CONTENTS_DIMENSIONS)
            {
                receiveContentsDimensionsRequest();
            }
            else if(mh.type == MESSAGE_TYPE_PIXELSTREAM)
            {
                receivePixelStreams(mh, pixelStreamFactory);
            }
            else if(mh.type == MESSAGE_TYPE_QUIT)
            {
                QApplication::instance()->quit();
                return;
            }

            // check to see if we have another message waiting for this process
            // and for all render processes
            MPI_Iprobe(0, 0, MPI_COMM_WORLD, &flag, &status);
            MPI_Allreduce(&flag, &allFlag, 1, MPI_INT, MPI_LAND, mpiRenderComm);
        }

        // at this point, we've received the last message available for all render processes
    }
}

void MPIChannel::send(DisplayGroupManagerPtr displayGroup)
{
    std::ostringstream oss(std::ostringstream::binary);
    {
        // brace this so destructor is called on archive before we use the stream
        boost::archive::binary_oarchive oa(oss);
        oa << displayGroup;
    }

    const std::string serializedString = oss.str();
    const int size = serializedString.size();

    MessageHeader mh;
    mh.size = size;
    mh.type = MESSAGE_TYPE_CONTENTS;

    // Send header via a send so we can probe it on the render processes
    for(int i=1; i<mpiSize; ++i)
        MPI_Send((void *)&mh, sizeof(MessageHeader), MPI_BYTE, i, 0, MPI_COMM_WORLD);

    // Broadcast the message
    MPI_Bcast((void *)serializedString.data(), size, MPI_BYTE, 0, MPI_COMM_WORLD);
}

void MPIChannel::send(OptionsPtr options)
{
    std::ostringstream oss(std::ostringstream::binary);
    {
        // brace this so destructor is called on archive before we use the stream
        boost::archive::binary_oarchive oa(oss);
        oa << options;
    }

    const std::string serializedString = oss.str();
    const int size = serializedString.size();

    MessageHeader mh;
    mh.size = size;
    mh.type = MESSAGE_TYPE_OPTIONS;

    // Send header via a send so we can probe it on the render processes
    for(int i=1; i<mpiSize; ++i)
        MPI_Send((void *)&mh, sizeof(MessageHeader), MPI_BYTE, i, 0, MPI_COMM_WORLD);

    // Broadcast the message
    MPI_Bcast((void *)serializedString.data(), size, MPI_BYTE, 0, MPI_COMM_WORLD);
}

void MPIChannel::sendContentsDimensionsRequest(ContentWindowManagerPtrs contentWindows)
{
    if(mpiSize < 2)
    {
        put_flog(LOG_WARN, "cannot get contents dimension update for mpiSize == %i", mpiSize);
        return;
    }

    if(mpiRank != 0)
    {
        put_flog(LOG_ERROR, "called on rank: %i != 0", mpiRank);
        return;
    }

    // send the header and the message
    MessageHeader mh;
    mh.type = MESSAGE_TYPE_CONTENTS_DIMENSIONS;

    // the header is sent via a send, so that we can probe it on the render processes
    for(int i=1; i<mpiSize; i++)
        MPI_Send((void *)&mh, sizeof(MessageHeader), MPI_BYTE, i, 0, MPI_COMM_WORLD);

    // now, receive response from rank 1
    MPI_Status status;
    MPI_Recv((void *)&mh, sizeof(MessageHeader), MPI_BYTE, 1, 0, MPI_COMM_WORLD, &status);

    // receive serialized data
    std::vector<char> buffer(mh.size);

    // read message into the buffer
    MPI_Recv((void *)buffer.data(), mh.size, MPI_BYTE, 1, 0, MPI_COMM_WORLD, &status);

    // de-serialize...
    std::istringstream iss(std::istringstream::binary);

    if(iss.rdbuf()->pubsetbuf(buffer.data(), mh.size) == NULL)
    {
        put_flog(LOG_FATAL, "rank %i: error setting stream buffer", mpiRank);
        return;
    }

    // read to a new vector
    std::vector<std::pair<int, int> > dimensions;

    boost::archive::binary_iarchive ia(iss);
    ia >> dimensions;

    // overwrite old dimensions
    for(size_t i=0; i<dimensions.size() && i<contentWindows.size(); ++i)
        contentWindows[i]->getContent()->setDimensions(dimensions[i].first, dimensions[i].second);
}

void MPIChannel::synchronizeClock()
{
    if(getRank() == 1)
        sendFrameClockUpdate();
    else
        receiveFrameClockUpdate();
}

void MPIChannel::sendFrameClockUpdate()
{
    // this should only be called by the rank 1 process
    if(mpiRank != 1)
    {
        put_flog(LOG_WARN, "called by rank %i != 1", mpiRank);
        return;
    }

    boost::posix_time::ptime timestamp(boost::posix_time::microsec_clock::universal_time());

    // serialize state
    std::ostringstream oss(std::ostringstream::binary);

    // brace this so destructor is called on archive before we use the stream
    {
        boost::archive::binary_oarchive oa(oss);
        oa << timestamp;
    }

    // serialized data to string
    std::string serializedString = oss.str();
    int size = serializedString.size();

    // send the header and the message
    MessageHeader mh;
    mh.size = size;
    mh.type = MESSAGE_TYPE_FRAME_CLOCK;

    // the header is sent via a send, so that we can probe it on the render processes
    for(int i=2; i<mpiSize; i++)
    {
        MPI_Send((void *)&mh, sizeof(MessageHeader), MPI_BYTE, i, 0, MPI_COMM_WORLD);
    }

    // broadcast it
    MPI_Bcast((void *)serializedString.data(), size, MPI_BYTE, 0, mpiRenderComm);

    // update timestamp
    timestamp_ = timestamp;
}

void MPIChannel::receiveFrameClockUpdate()
{
    // we shouldn't run the broadcast if we're rank 1
    if(mpiRank == 1)
        return;

    // receive the message header
    MessageHeader messageHeader;
    MPI_Status status;
    MPI_Recv((void *)&messageHeader, sizeof(MessageHeader), MPI_BYTE, 1, 0, MPI_COMM_WORLD, &status);

    if(messageHeader.type != MESSAGE_TYPE_FRAME_CLOCK)
    {
        put_flog(LOG_FATAL, "unexpected message type");
        return;
    }

    // receive serialized data
    std::vector<char> buffer(messageHeader.size);

    // read message into the buffer
    MPI_Bcast((void *)buffer.data(), messageHeader.size, MPI_BYTE, 0, mpiRenderComm);

    // de-serialize...
    std::istringstream iss(std::istringstream::binary);

    if(iss.rdbuf()->pubsetbuf(buffer.data(), messageHeader.size) == NULL)
    {
        put_flog(LOG_FATAL, "rank %i: error setting stream buffer", mpiRank);
        return;
    }

    boost::archive::binary_iarchive ia(iss);
    ia >> timestamp_;
}

void MPIChannel::sendQuit()
{
    MessageHeader mh;
    mh.type = MESSAGE_TYPE_QUIT;

    // Send header via a send so that we can probe it on the render processes
    for(int i=1; i<mpiSize; i++)
        MPI_Send((void *)&mh, sizeof(MessageHeader), MPI_BYTE, i, 0, MPI_COMM_WORLD);
}

DisplayGroupManagerPtr MPIChannel::receiveDisplayGroup(const MessageHeader& messageHeader)
{
    if(mpiRank < 1)
    {
        put_flog(LOG_WARN, "called on rank %i < 1", mpiRank);
        return DisplayGroupManagerPtr();
    }

    // receive serialized data
    std::vector<char> buffer(messageHeader.size);

    // read message into the buffer
    MPI_Bcast((void *)buffer.data(), messageHeader.size, MPI_BYTE, 0, MPI_COMM_WORLD);

    // de-serialize...
    std::istringstream iss(std::istringstream::binary);

    if(iss.rdbuf()->pubsetbuf(buffer.data(), messageHeader.size) == NULL)
    {
        put_flog(LOG_FATAL, "rank %i: error setting stream buffer", mpiRank);
        return DisplayGroupManagerPtr();
    }

    boost::archive::binary_iarchive ia(iss);
    DisplayGroupManagerPtr displayGroup;
    ia >> displayGroup;

    return displayGroup;
}

OptionsPtr MPIChannel::receiveOptions(const MessageHeader& messageHeader)
{
    if(mpiRank < 1)
    {
        put_flog(LOG_WARN, "called on rank %i < 1", mpiRank);
        return OptionsPtr();
    }

    // receive serialized data
    std::vector<char> buffer(messageHeader.size);

    // read message into the buffer
    MPI_Bcast((void *)buffer.data(), messageHeader.size, MPI_BYTE, 0, MPI_COMM_WORLD);

    // de-serialize...
    std::istringstream iss(std::istringstream::binary);

    if(iss.rdbuf()->pubsetbuf(buffer.data(), messageHeader.size) == NULL)
    {
        put_flog(LOG_FATAL, "rank %i: error setting stream buffer", mpiRank);
        return OptionsPtr();
    }

    boost::archive::binary_iarchive ia(iss);
    OptionsPtr options;
    ia >> options;

    return options;
}

void MPIChannel::receiveContentsDimensionsRequest()
{
    if(mpiRank != 1)
        return;

    ContentWindowManagerPtrs contentWindows;
    if (displayGroup_)
        contentWindows = displayGroup_->getContentWindowManagers();
    else
        put_flog(LOG_ERROR, "Cannot send content dimensions before a DisplayGroup was received!");

    std::vector<std::pair<int, int> > dimensions;

    for(size_t i=0; i<contentWindows.size(); ++i)
    {
        int w,h;
        contentWindows[i]->getContent()->getFactoryObjectDimensions(w, h);

        dimensions.push_back(std::pair<int,int>(w,h));
    }

    // serialize
    std::ostringstream oss(std::ostringstream::binary);

    // brace this so destructor is called on archive before we use the stream
    {
        boost::archive::binary_oarchive oa(oss);
        oa << dimensions;
    }

    // serialized data to string
    std::string serializedString = oss.str();
    int size = serializedString.size();

    // send the header and the message
    MessageHeader mh;
    mh.size = size;
    mh.type = MESSAGE_TYPE_CONTENTS_DIMENSIONS;

    MPI_Send((void *)&mh, sizeof(MessageHeader), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
    MPI_Send((void *)serializedString.data(), size, MPI_BYTE, 0, 0, MPI_COMM_WORLD);
}

void MPIChannel::send(const std::vector<PixelStreamSegment> & segments, const QString& uri)
{
    if(mpiRank != 0)
    {
        put_flog(LOG_WARN, "called on rank %i != 0", mpiRank);
        return;
    }

    assert(!segments.empty() && "sendPixelStreamSegments() received an empty vector");

    // serialize the vector
    std::ostringstream oss(std::ostringstream::binary);

    // brace this so destructor is called on archive before we use the stream
    {
        boost::archive::binary_oarchive oa(oss);
        oa << segments;
    }

    // serialized data to string
    std::string serializedString = oss.str();
    int size = serializedString.size();

    // send the header and the message
    MessageHeader mh;
    mh.size = size;
    mh.type = MESSAGE_TYPE_PIXELSTREAM;

    // add the truncated URI to the header
    strncpy(mh.uri, uri.toLocal8Bit().constData(), MESSAGE_HEADER_URI_LENGTH-1);

    // the header is sent via a send, so that we can probe it on the render processes
    for(int i=1; i<mpiSize; i++)
    {
        MPI_Send((void *)&mh, sizeof(MessageHeader), MPI_BYTE, i, 0, MPI_COMM_WORLD);
    }

    // broadcast the message
    MPI_Bcast((void *)serializedString.data(), size, MPI_BYTE, 0, MPI_COMM_WORLD);
}

void MPIChannel::receivePixelStreams(const MessageHeader& messageHeader,
                                     Factory<PixelStream>& pixelStreamFactory)
{
    if(mpiRank < 1)
    {
        put_flog(LOG_WARN, "called on rank %i < 1", mpiRank);
        return;
    }

    // receive serialized data
    std::vector<char> buffer(messageHeader.size);

    // read message into the buffer
    MPI_Bcast((void *)buffer.data(), messageHeader.size, MPI_BYTE, 0, MPI_COMM_WORLD);

    // URI
    const QString uri(messageHeader.uri);

    // de-serialize...
    std::istringstream iss(std::istringstream::binary);

    if(iss.rdbuf()->pubsetbuf(buffer.data(), messageHeader.size) == NULL)
    {
        put_flog(LOG_FATAL, "rank %i: error setting stream buffer", mpiRank);
        return;
    }

    // read to a new segments vector
    std::vector<PixelStreamSegment> segments;

    boost::archive::binary_iarchive ia(iss);
    ia >> segments;

    pixelStreamFactory.getObject(uri)->insertNewFrame(segments);
}
