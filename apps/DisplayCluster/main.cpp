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

#include "config.h"

#include "globals.h"
#include "MPIChannel.h"

#include "WallApplication.h"
#include "MasterApplication.h"

#include "log.h"

#if ENABLE_TUIO_TOUCH_LISTENER
#include <X11/Xlib.h>
#endif

int main(int argc, char * argv[])
{
    g_mpiChannel.reset( new MPIChannel( argc, argv ) );

#if ENABLE_TUIO_TOUCH_LISTENER
    // we need X multithreading support if we're running the
    // TouchListener thread and creating X events
    if (g_mpiChannel->getRank() == 0)
        XInitThreads();
#endif

    Application* app = 0;
    if ( g_mpiChannel->getRank() == 0 )
        app = new MasterApplication(argc, argv, g_mpiChannel);
    else
        app = new WallApplication(argc, argv, g_mpiChannel);

    // calibrate timestamp offset between rank 0 and rank 1 clocks
    g_mpiChannel->calibrateTimestampOffset();
    // wait for render comms to be ready for receiving and rendering background
    g_mpiChannel->globalBarrier(); // previously: MPI_COMM_WORLD

    app->exec(); // enter Qt event loop

    put_flog(LOG_DEBUG, "quitting");

    QThreadPool::globalInstance()->waitForDone();

    if (g_mpiChannel->getRank() == 0)
        g_mpiChannel->sendQuit();

    delete app;

    return EXIT_SUCCESS;
}
