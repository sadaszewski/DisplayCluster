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

#ifndef MPICHANNEL_H
#define MPICHANNEL_H

#include "types.h"

#include "Factory.hpp"

#include <QObject>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <mpi.h>

struct MessageHeader;

/**
 * Handle MPI communications between all DisplayCluster instances.
 */
class MPIChannel : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor, initialize the MPI communication.
     * Only one instance is of this class per program is allowed.
     * @param argc main program arguments count
     * @param argv main program arguments
     */
    MPIChannel(int argc, char* argv[]);

    /** Destructor, finalize the MPI communication. */
    ~MPIChannel();

    /** Get the rank of this process. */
    int getRank() const;

    /** Block execution until all programs have reached the barrier. */
    void globalBarrier() const;

    /**
     * Get the sum of the given local values across all processes.
     * @param localValue The value to sum
     * @return the sum of the localValues
     */
    int globalSum(const int localValue) const;

    /** Synchronize clock time across all processes. */
    void synchronizeClock();

    /** Get the current timestamp, synchronized accross processes. */
    boost::posix_time::ptime getTime() const;

    /**
     * Ranks 1-N: Receive messages.
     * Will emit a signal if an object was reveived.
     * @see received(DisplayGroupManagerPtr)
     * @see received(OptionsPtr)
     */
    void receiveMessages();

    /**
     * Rank0: Calibrate the offset between its local clock and the rank1 clock.
     * @note The rank1 clock is used across ranks 1-N.
     */
    void calibrateTimestampOffset();

    /**
     * Rank0: send quit message to ranks 1-N, terminating the processes.
     */
    void sendQuit();

    // TODO remove content dimension requests (DISCL-21)
    /**
     * Rank0: ask rank1 to provide the dimensions for the given Contents.
     * @param contentWindows The Contents for which to update dimensions.
     */
    void sendContentsDimensionsRequest(ContentWindowManagerPtrs contentWindows);

    /** Set the factories on Rank1 to respond to Content Dimensions request */
    void setFactories(FactoriesPtr factories);

public slots:
    /**
     * Rank0: send the given DisplayGroup to ranks 1-N
     * @param displayGroup The DisplayGroup to send
     */
    void send(DisplayGroupManagerPtr displayGroup);

    /**
     * Rank0: send the given Options to ranks 1-N
     * @param options The options to send
     */
    void send(OptionsPtr options);

    /**
     * Rank 0: Send pixel stream frame to ranks 1-N
     * @param frame The frame to send
     */
    void send(PixelStreamFramePtr frame);

signals:
    /**
     * Rank 1-N: Emitted when a displayGroup was recieved
     * @see receiveMessages()
     * @param displayGroup The DisplayGroup that was received
     */
    void received(DisplayGroupManagerPtr displayGroup);

    /**
     * Rank 1-N: Emitted when new Options were recieved
     * @see receiveMessages()
     * @param options The options that was received
     */
    void received(OptionsPtr options);

    /**
     * Rank 1-N: Emitted when a new PixelStream frame was recieved
     * @see receiveMessages()
     * @param frame The frame that was received
     */
    void received(PixelStreamFramePtr frame);

private:
    int mpiRank_;
    int mpiSize_;
    MPI_Comm mpiRenderComm_;

    boost::posix_time::ptime timestamp_; // frame timing
    boost::posix_time::time_duration timestampOffset_; // rank1 - rank0 offset

    void sendFrameClockUpdate();
    void receiveFrameClockUpdate();

    // Ranks 1-n recieve data through MPI
    DisplayGroupManagerPtr receiveDisplayGroup(const MessageHeader& messageHeader);
    OptionsPtr receiveOptions(const MessageHeader& messageHeader);
    void receivePixelStreams(const MessageHeader& messageHeader);

    // TODO remove content dimension requests (DISCL-21)
    void receiveContentsDimensionsRequest();
    // Storing the DisplayGroup (on Rank1) to serve contentDimensionsRequests
    DisplayGroupManagerPtr displayGroup_;
    FactoriesPtr factories_;
};

#endif // MPICHANNEL_H
