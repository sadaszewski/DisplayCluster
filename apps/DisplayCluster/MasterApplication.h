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

#ifndef MASTERAPPLICATION_H
#define MASTERAPPLICATION_H

#include "Application.h"

class MasterWindow;
class NetworkListener;
class PixelStreamerLauncher;
class PixelStreamWindowManager;
class WebServiceServer;
class TextInputDispatcher;
class MasterConfiguration;

class MasterApplication : public Application
{
    Q_OBJECT

public:
    /**
     * Constructor
     * @param argc Command line argument count (required by QApplication)
     * @param argv Command line arguments (required by QApplication)
     * @param mpiChannel The interprocess communication channel
     */
    MasterApplication(int &argc, char **argv, MPIChannelPtr mpiChannel);

    /** Destructor */
    virtual ~MasterApplication();

private:
    MasterWindow* masterWindow_;
    DisplayGroupManagerPtr displayGroup_;
    NetworkListener* networkListener_;
    PixelStreamerLauncher* pixelStreamerLauncher_;
    PixelStreamWindowManager* pixelStreamWindowManager_;
    WebServiceServer* webServiceServer_;
    TextInputDispatcher* textInputDispatcher_;

#if ENABLE_JOYSTICK_SUPPORT
    JoystickThread* joystickThread_;
#endif

#if ENABLE_SKELETON_SUPPORT
    SkeletonThread* skeletonThread_;
#endif

    void init(const MasterConfiguration* config);
    void startNetworkListener(const MasterConfiguration* configuration);
    void startWebservice(const int webServicePort);
    void restoreBackground(const MasterConfiguration* configuration);
    void initPixelStreamLauncher();

#if ENABLE_JOYSTICK_SUPPORT
    void startJoystickThread();
#endif

#if ENABLE_SKELETON_SUPPORT
    void startSkeletonThread();
#endif
};

#endif // MASTERAPPLICATION_H
