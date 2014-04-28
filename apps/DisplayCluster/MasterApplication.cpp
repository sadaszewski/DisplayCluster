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

#include "MasterApplication.h"

#include "MasterWindow.h"
#include "DisplayGroupManager.h"
#include "configuration/MasterConfiguration.h"
#include "MPIChannel.h"
#include "Options.h"

#if ENABLE_JOYSTICK_SUPPORT
    #include "JoystickThread.h"
#endif

#if ENABLE_SKELETON_SUPPORT
    #include "SkeletonThread.h"
#endif

#include "NetworkListener.h"
#include "localstreamer/PixelStreamerLauncher.h"
#include "StateSerializationHelper.h"
#include "PixelStreamWindowManager.h"

#include "CommandHandler.h"
#include "SessionCommandHandler.h"
#include "FileCommandHandler.h"
#include "WebbrowserCommandHandler.h"

#include "ws/WebServiceServer.h"
#include "ws/TextInputDispatcher.h"
#include "ws/TextInputHandler.h"
#include "ws/DisplayGroupManagerAdapter.h"


MasterApplication::MasterApplication(int& argc_, char** argv_, MPIChannelPtr mpiChannel)
    : Application(argc_, argv_)
    , masterWindow_(0)
    , displayGroup_(new DisplayGroupManager)
    , networkListener_(0)
    , pixelStreamerLauncher_(0)
    , pixelStreamWindowManager_(0)
    , webServiceServer_(0)
    , textInputDispatcher_(0)
    #if ENABLE_JOYSTICK_SUPPORT
        , joystickThread_(0)
    #endif
    #if ENABLE_SKELETON_SUPPORT
        , skeletonThread_(0)
    #endif
{
    MasterConfiguration* config = new MasterConfiguration(getConfigFilename());
    g_configuration = config;

    if( argc_ == 2 )
        StateSerializationHelper(displayGroup_).load( argv_[1] );

    // Distribute the DisplayGroup through MPI whenever it is modified
    connect(displayGroup_.get(), SIGNAL(modified(DisplayGroupManagerPtr)),
            mpiChannel.get(), SLOT(send(DisplayGroupManagerPtr)));

    // Distribute the options when they are updated
    connect(config->getOptions().get(), SIGNAL(updated(OptionsPtr)),
            mpiChannel.get(), SLOT(send(OptionsPtr)));

    init(config);
}

MasterApplication::~MasterApplication()
{
#if ENABLE_SKELETON_SUPPORT
    delete skeletonThread_;
    skeletonThread_ = 0;
#endif

#if ENABLE_JOYSTICK_SUPPORT
    delete joystickThread_;
    joystickThread_ = 0;
#endif

    // Clear all ContentWindows
    displayGroup_->clear();

    delete masterWindow_;
    masterWindow_ = 0;

    delete pixelStreamerLauncher_;
    pixelStreamerLauncher_ = 0;
    delete networkListener_;
    networkListener_ = 0;

    delete textInputDispatcher_;
    textInputDispatcher_ = 0;

    webServiceServer_->stop();
    webServiceServer_->wait();
    delete webServiceServer_;
    webServiceServer_ = 0;

    delete pixelStreamWindowManager_;
    pixelStreamWindowManager_ = 0;
}


void MasterApplication::init(const MasterConfiguration* config)
{
    masterWindow_ = new MasterWindow(displayGroup_);

    pixelStreamWindowManager_ = new PixelStreamWindowManager(*displayGroup_);

    initPixelStreamLauncher();
    startNetworkListener(config);
    startWebservice(config->getWebServicePort());
    restoreBackground(config);

#if ENABLE_JOYSTICK_SUPPORT
    startJoystickThread();
#endif

#if ENABLE_SKELETON_SUPPORT
    startSkeletonThread();
#endif
}

void MasterApplication::startNetworkListener(const MasterConfiguration* configuration)
{
    if (networkListener_)
        return;

    networkListener_ = new NetworkListener(*pixelStreamWindowManager_);

    CommandHandler& handler = networkListener_->getCommandHandler();
    handler.registerCommandHandler(new FileCommandHandler(displayGroup_, *pixelStreamWindowManager_));
    handler.registerCommandHandler(new SessionCommandHandler(*displayGroup_));

    const QString& url = configuration->getWebBrowserDefaultURL();
    handler.registerCommandHandler(new WebbrowserCommandHandler(
                                       *pixelStreamWindowManager_,
                                       *pixelStreamerLauncher_,
                                       url));
}

void MasterApplication::startWebservice(const int webServicePort)
{
    if (webServiceServer_)
        return;

    webServiceServer_ = new WebServiceServer(webServicePort);

    DisplayGroupManagerAdapterPtr adapter(new DisplayGroupManagerAdapter(displayGroup_));
    TextInputHandler* textInputHandler = new TextInputHandler(adapter);
    webServiceServer_->addHandler("/dcapi/textinput", dcWebservice::HandlerPtr(textInputHandler));

    textInputHandler->moveToThread(webServiceServer_);
    textInputDispatcher_ = new TextInputDispatcher(displayGroup_);
    textInputDispatcher_->connect(textInputHandler, SIGNAL(receivedKeyInput(char)),
                         SLOT(sendKeyEventToActiveWindow(char)));

    webServiceServer_->start();
}

void MasterApplication::restoreBackground(const MasterConfiguration* configuration)
{
    // Must be done after everything else is setup (or in the MainWindow constructor)
    displayGroup_->setBackgroundColor( configuration->getBackgroundColor( ));
    ContentPtr content = ContentFactory::getContent( configuration->getBackgroundUri( ));
    displayGroup_->setBackgroundContent( content );
}

void MasterApplication::initPixelStreamLauncher()
{
    pixelStreamerLauncher_ = new PixelStreamerLauncher(*pixelStreamWindowManager_);

    pixelStreamerLauncher_->connect(masterWindow_, SIGNAL(openWebBrowser(QPointF,QSize,QString)),
                                       SLOT(openWebBrowser(QPointF,QSize,QString)));
    pixelStreamerLauncher_->connect(masterWindow_, SIGNAL(openDock(QPointF,QSize,QString)),
                                       SLOT(openDock(QPointF,QSize,QString)));
    pixelStreamerLauncher_->connect(masterWindow_, SIGNAL(hideDock()),
                                       SLOT(hideDock()));
}

#if ENABLE_JOYSTICK_SUPPORT
void MasterApplication::startJoystickThread()
{
    // do this before the thread starts to avoid X callback race conditions
    // we need SDL_INIT_VIDEO for events to work
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
    {
        put_flog(LOG_ERROR, "could not initial SDL joystick subsystem");
        return;
    }

    // create thread to monitor joystick events (all joysticks handled in same event queue)
    JoystickThread * joystickThread_ = new JoystickThread(displayGroup_);
    joystickThread_->start();

    // wait for thread to start
    while(!joystickThread_->isRunning() || joystickThread_->isFinished())
    {
        usleep(1000);
    }
}
#endif

#if ENABLE_SKELETON_SUPPORT
void MasterApplication::startSkeletonThread()
{
    skeletonThread_ = new SkeletonThread(displayGroup_);
    skeletonThread_->start();

    // wait for thread to start
    while( !skeletonThread_->isRunning() || skeletonThread_->isFinished() )
    {
        usleep(1000);
    }

    connect(masterWindow_, SIGNAL(enableSkeletonTracking()), skeletonThread_, SLOT(startTimer()));
    connect(masterWindow_, SIGNAL(disableSkeletonTracking()), skeletonThread_, SLOT(stopTimer()));

    connect(skeletonThread_, SIGNAL(skeletonsUpdated(std::vector< boost::shared_ptr<SkeletonState> >)),
            displayGroup_.get(), SLOT(setSkeletons(std::vector<boost::shared_ptr<SkeletonState> >)),
            Qt::QueuedConnection);
}
#endif
