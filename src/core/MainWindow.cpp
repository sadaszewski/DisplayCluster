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

#include "MainWindow.h"
#include "globals.h"
#include "Options.h"
#include "MPIChannel.h"
#include "configuration/WallConfiguration.h"
#include "configuration/MasterConfiguration.h"
#include "ContentLoader.h"
#include "ContentFactory.h"
#include "ContentWindowManager.h"
#include "log.h"
#include "DisplayGroupManager.h"
#include "DisplayGroupGraphicsViewProxy.h"
#include "DisplayGroupGraphicsView.h"
#include "DisplayGroupListWidgetProxy.h"
#include "BackgroundWidget.h"
#include "StateSerializationHelper.h"
#include "localstreamer/DockPixelStreamer.h"

#include "GLWindow.h"

#if ENABLE_PYTHON_SUPPORT
    #include "PythonConsole.h"
#endif

#if ENABLE_TUIO_TOUCH_LISTENER
    #include "MultiTouchListener.h"
#endif

#define WEBBROWSER_DEFAULT_URL   "http://www.google.ch"

#define DOCK_WIDTH_RELATIVE_TO_WALL   0.175

MainWindow::MainWindow()
    : backgroundWidget_(0)
#if ENABLE_TUIO_TOUCH_LISTENER
    , touchListener_(0)
#endif
{
    // make application quit when last window is closed
    QObject::connect(QApplication::instance(), SIGNAL(lastWindowClosed()),
                     QApplication::instance(), SLOT(quit()));

    if(g_mpiChannel->getRank() == 0)
    {
#if ENABLE_PYTHON_SUPPORT
        PythonConsole::init();
#endif
        // rank 0 window setup
        resize(800,600);
        setAcceptDrops(true);

        setupMasterWindowUI();

        show();
    }
    else
    {
        setupWallOpenGLWindows();

        // setup connection so updateGLWindows() will be called continuously
        // must be queued so we return to the main event loop and avoid infinite recursion
        connect(this, SIGNAL(updateGLWindowsFinished()), this, SLOT(updateGLWindows()), Qt::QueuedConnection);

        // trigger the first update
        updateGLWindows();
    }
}

MainWindow::~MainWindow()
{
#if ENABLE_TUIO_TOUCH_LISTENER
    delete touchListener_;
#endif
}


void MainWindow::setupMasterWindowUI()
{
    // create menus in menu bar
    QMenu * fileMenu = menuBar()->addMenu("&File");
    QMenu * viewMenu = menuBar()->addMenu("&View");
    QMenu * viewStreamingMenu = viewMenu->addMenu("&Streaming");
#if ENABLE_PYTHON_SUPPORT
    // add Window menu for Python console. if we add any other entries to it we'll need to remove the #if
    QMenu * windowMenu = menuBar()->addMenu("&Window");
#endif

#if ENABLE_SKELETON_SUPPORT
    QMenu * skeletonMenu = menuBar()->addMenu("&Skeleton Tracking");
#endif

    // create tool bar
    QToolBar * toolbar = addToolBar("toolbar");

    // open content action
    QAction * openContentAction = new QAction("Open Content", this);
    openContentAction->setStatusTip("Open content");
    connect(openContentAction, SIGNAL(triggered()), this, SLOT(openContent()));

    // open contents directory action
    QAction * openContentsDirectoryAction = new QAction("Open Contents Directory", this);
    openContentsDirectoryAction->setStatusTip("Open contents directory");
    connect(openContentsDirectoryAction, SIGNAL(triggered()), this, SLOT(openContentsDirectory()));

    // clear contents action
    QAction * clearContentsAction = new QAction("Clear", this);
    clearContentsAction->setStatusTip("Clear");
    connect(clearContentsAction, SIGNAL(triggered()), this, SLOT(clearContents()));

    // save state action
    QAction * saveStateAction = new QAction("Save State", this);
    saveStateAction->setStatusTip("Save state");
    connect(saveStateAction, SIGNAL(triggered()), this, SLOT(saveState()));

    // load state action
    QAction * loadStateAction = new QAction("Load State", this);
    loadStateAction->setStatusTip("Load state");
    connect(loadStateAction, SIGNAL(triggered()), this, SLOT(loadState()));

    // compute image pyramid action
    QAction * computeImagePyramidAction = new QAction("Compute Image Pyramid", this);
    computeImagePyramidAction->setStatusTip("Compute image pyramid");
    connect(computeImagePyramidAction, SIGNAL(triggered()), this, SLOT(computeImagePyramid()));

    // load background content action
    QAction * backgroundAction = new QAction("Background", this);
    backgroundAction->setStatusTip("Select the background color and content");
    connect(backgroundAction, SIGNAL(triggered()), this, SLOT(showBackgroundWidget()));

    // Open webbrowser action
    QAction * webbrowserAction = new QAction("Web Browser", this);
    webbrowserAction->setStatusTip("Open a web browser");
    connect(webbrowserAction, SIGNAL(triggered()), this, SLOT(openWebBrowser()));

#if ENABLE_PYTHON_SUPPORT
    // Python console action
    QAction * pythonConsoleAction = new QAction("Open Python Console", this);
    pythonConsoleAction->setStatusTip("Open Python console");
    connect(pythonConsoleAction, SIGNAL(triggered()), PythonConsole::self(), SLOT(show()));
#endif

    // quit action
    QAction * quitAction = new QAction("Quit", this);
    quitAction->setStatusTip("Quit application");
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    OptionsPtr options = g_configuration->getOptions();

    // show window borders action
    QAction * showWindowBordersAction = new QAction("Show Window Borders", this);
    showWindowBordersAction->setStatusTip("Show window borders");
    showWindowBordersAction->setCheckable(true);
    showWindowBordersAction->setChecked(options->getShowWindowBorders());
    connect(showWindowBordersAction, SIGNAL(toggled(bool)), options.get(), SLOT(setShowWindowBorders(bool)));

    // show mouse cursor action
    QAction * showMouseCursorAction = new QAction("Show Mouse Cursor", this);
    showMouseCursorAction->setStatusTip("Show mouse cursor");
    showMouseCursorAction->setCheckable(true);
    showMouseCursorAction->setChecked(options->getShowMouseCursor());
    connect(showMouseCursorAction, SIGNAL(toggled(bool)), options.get(), SLOT(setShowMouseCursor(bool)));

    // show touch points action
    QAction * showTouchPoints = new QAction("Show Touch Points", this);
    showTouchPoints->setStatusTip("Show touch points");
    showTouchPoints->setCheckable(true);
    showTouchPoints->setChecked(options->getShowTouchPoints());
    connect(showTouchPoints, SIGNAL(toggled(bool)), options.get(), SLOT(setShowTouchPoints(bool)));

    // show movie controls action
    QAction * showMovieControlsAction = new QAction("Show Movie Controls", this);
    showMovieControlsAction->setStatusTip("Show movie controls");
    showMovieControlsAction->setCheckable(true);
    showMovieControlsAction->setChecked(options->getShowMovieControls());
    connect(showMovieControlsAction, SIGNAL(toggled(bool)), options.get(), SLOT(setShowMovieControls(bool)));

    // show test pattern action
    QAction * showTestPatternAction = new QAction("Show Test Pattern", this);
    showTestPatternAction->setStatusTip("Show test pattern");
    showTestPatternAction->setCheckable(true);
    showTestPatternAction->setChecked(options->getShowTestPattern());
    connect(showTestPatternAction, SIGNAL(toggled(bool)), options.get(), SLOT(setShowTestPattern(bool)));

    // enable mullion compensation action
    QAction * enableMullionCompensationAction = new QAction("Enable Mullion Compensation", this);
    enableMullionCompensationAction->setStatusTip("Enable mullion compensation");
    enableMullionCompensationAction->setCheckable(true);
    enableMullionCompensationAction->setChecked(options->getEnableMullionCompensation());
    connect(enableMullionCompensationAction, SIGNAL(toggled(bool)), options.get(), SLOT(setEnableMullionCompensation(bool)));

    // show zoom context action
    QAction * showZoomContextAction = new QAction("Show Zoom Context", this);
    showZoomContextAction->setStatusTip("Show zoom context");
    showZoomContextAction->setCheckable(true);
    showZoomContextAction->setChecked(options->getShowZoomContext());
    connect(showZoomContextAction, SIGNAL(toggled(bool)), options.get(), SLOT(setShowZoomContext(bool)));

    // show streaming segments action
    QAction * showStreamingSegmentsAction = new QAction("Show Segments", this);
    showStreamingSegmentsAction->setStatusTip("Show segments");
    showStreamingSegmentsAction->setCheckable(true);
    showStreamingSegmentsAction->setChecked(options->getShowStreamingSegments());
    connect(showStreamingSegmentsAction, SIGNAL(toggled(bool)), options.get(), SLOT(setShowStreamingSegments(bool)));

    // show streaming statistics action
    QAction * showStreamingStatisticsAction = new QAction("Show Statistics", this);
    showStreamingStatisticsAction->setStatusTip("Show statistics");
    showStreamingStatisticsAction->setCheckable(true);
    showStreamingStatisticsAction->setChecked(options->getShowStreamingStatistics());
    connect(showStreamingStatisticsAction, SIGNAL(toggled(bool)), options.get(), SLOT(setShowStreamingStatistics(bool)));

#if ENABLE_SKELETON_SUPPORT
    // enable skeleton tracking action
    QAction * enableSkeletonTrackingAction = new QAction("Enable Skeleton Tracking", this);
    enableSkeletonTrackingAction->setStatusTip("Enable skeleton tracking");
    enableSkeletonTrackingAction->setCheckable(true);
    enableSkeletonTrackingAction->setChecked(true); // timer is started by default
    connect(enableSkeletonTrackingAction, SIGNAL(toggled(bool)), this, SLOT(setEnableSkeletonTracking(bool)));

    // show skeletons action
    QAction * showSkeletonsAction = new QAction("Show Skeletons", this);
    showSkeletonsAction->setStatusTip("Show skeletons");
    showSkeletonsAction->setCheckable(true);
    showSkeletonsAction->setChecked(options->getShowSkeletons());
    connect(showSkeletonsAction, SIGNAL(toggled(bool)), options.get(), SLOT(setShowSkeletons(bool)));
#endif

    // add actions to menus
    fileMenu->addAction(openContentAction);
    fileMenu->addAction(openContentsDirectoryAction);
    fileMenu->addAction(webbrowserAction);
    fileMenu->addAction(clearContentsAction);
    fileMenu->addAction(saveStateAction);
    fileMenu->addAction(loadStateAction);
    fileMenu->addAction(computeImagePyramidAction);
    fileMenu->addAction(quitAction);
    viewMenu->addAction(backgroundAction);
    viewMenu->addAction(showWindowBordersAction);
    viewMenu->addAction(showMouseCursorAction);
    viewMenu->addAction(showTouchPoints);
    viewMenu->addAction(showMovieControlsAction);
    viewMenu->addAction(showTestPatternAction);
    viewMenu->addAction(enableMullionCompensationAction);
    viewMenu->addAction(showZoomContextAction);
    viewStreamingMenu->addAction(showStreamingSegmentsAction);
    viewStreamingMenu->addAction(showStreamingStatisticsAction);

#if ENABLE_PYTHON_SUPPORT
    windowMenu->addAction(pythonConsoleAction);
#endif

#if ENABLE_SKELETON_SUPPORT
    skeletonMenu->addAction(enableSkeletonTrackingAction);
    skeletonMenu->addAction(showSkeletonsAction);
#endif

    // add actions to toolbar
    toolbar->addAction(openContentAction);
    toolbar->addAction(openContentsDirectoryAction);
    toolbar->addAction(clearContentsAction);
    toolbar->addAction(saveStateAction);
    toolbar->addAction(loadStateAction);
    toolbar->addAction(computeImagePyramidAction);
    toolbar->addAction(backgroundAction);
    toolbar->addAction(webbrowserAction);
#if ENABLE_PYTHON_SUPPORT
    toolbar->addAction(pythonConsoleAction);
#endif
    // main widget / layout area
    QTabWidget * mainWidget = new QTabWidget();
    setCentralWidget(mainWidget);

    // add the local renderer group
    DisplayGroupGraphicsViewProxy * dggv = new DisplayGroupGraphicsViewProxy(g_displayGroupManager);
    mainWidget->addTab((QWidget *)dggv->getGraphicsView(), "Display group 0");
    // Forward background touch events
    connect(dggv->getGraphicsView(), SIGNAL(backgroundTap(QPointF)),
            this, SIGNAL(hideDock()));
    connect(dggv->getGraphicsView(), SIGNAL(backgroundTapAndHold(QPointF)),
            this, SLOT(openDock(QPointF)));
    connect(g_configuration->getOptions().get(), SIGNAL(updated(OptionsPtr)),
            dggv, SLOT(optionsUpdated(OptionsPtr)));

#if ENABLE_TUIO_TOUCH_LISTENER
    touchListener_ = new MultiTouchListener( dggv );
#endif

    // create contents dock widget
    QDockWidget * contentsDockWidget = new QDockWidget("Contents", this);
    QWidget * contentsWidget = new QWidget();
    QVBoxLayout * contentsLayout = new QVBoxLayout();
    contentsWidget->setLayout(contentsLayout);
    contentsDockWidget->setWidget(contentsWidget);
    addDockWidget(Qt::LeftDockWidgetArea, contentsDockWidget);

    // add the list widget
    DisplayGroupListWidgetProxy * dglwp = new DisplayGroupListWidgetProxy(g_displayGroupManager);
    contentsLayout->addWidget(dglwp->getListWidget());
}


void MainWindow::setupWallOpenGLWindows()
{
    // if we have just one tile for this process, make the GL window the central widget
    // otherwise, create multiple windows

    WallConfiguration* configuration = static_cast<WallConfiguration*>(g_configuration);

    if(configuration->getScreenCount() == 1)
    {
        move(configuration->getScreenPosition(0));
        resize(configuration->getScreenWidth(), configuration->getScreenHeight());

        GLWindowPtr glw(new GLWindow(0));
        glWindows_.push_back(glw);

        setCentralWidget(glw.get());

        if(configuration->getFullscreen())
        {
            showFullScreen();
        }
        else
        {
            show();
        }
    }
    else
    {
        for(int i=0; i<configuration->getScreenCount(); i++)
        {
            QRect windowRect = QRect(configuration->getScreenPosition(i), QSize(configuration->getScreenWidth(), configuration->getScreenHeight()));

            // setup shared OpenGL contexts
            GLWindow * shareWidget = NULL;

            if(i > 0)
            {
                shareWidget = glWindows_[0].get();
            }

            GLWindowPtr glw(new GLWindow(i, windowRect, shareWidget));
            glWindows_.push_back(glw);

            if(configuration->getFullscreen())
            {
                glw->showFullScreen();
            }
            else
            {
                glw->show();
            }
        }
    }
}

GLWindowPtr MainWindow::getGLWindow(int index)
{
    return glWindows_[index];
}

GLWindowPtr MainWindow::getActiveGLWindow()
{
    return activeGLWindow_;
}

bool MainWindow::isRegionVisible(const QRectF& region) const
{
    for(unsigned int i=0; i<glWindows_.size(); i++)
    {
        if(glWindows_[i]->isRegionVisible(region))
        {
            return true;
        }
    }
    return false;
}

void MainWindow::openContent()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Choose content"), QString(), ContentFactory::getSupportedFilesFilterAsString());

    if(!filename.isEmpty())
    {
        const bool success = ContentLoader(g_displayGroupManager).load(filename);

        if ( !success )
        {
            QMessageBox messageBox;
            messageBox.setText("Unsupported file format.");
            messageBox.exec();
        }
    }
}

void MainWindow::estimateGridSize(unsigned int numElem, unsigned int &gridX, unsigned int &gridY)
{
    assert(numElem > 0);
    gridX = (unsigned int)(ceil(sqrt(numElem)));
    assert(gridX > 0);
    gridY = (gridX*(gridX-1)>=numElem) ? gridX-1 : gridX;
}

void MainWindow::addContentDirectory(const QString& directoryName, unsigned int gridX, unsigned int gridY)
{
    QDir directory(directoryName);
    directory.setFilter(QDir::Files);
    directory.setNameFilters( ContentFactory::getSupportedFilesFilter() );

    QFileInfoList list = directory.entryInfoList();

    // Prevent opening of folders with an excessively large number of items
    if (list.size() > 16)
    {
        QString msg = "Opening this folder will create " + QString::number(list.size()) + " content elements. Are you sure you want to continue?";
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Warning", msg, QMessageBox::Yes|QMessageBox::No);
        if (reply != QMessageBox::Yes)
            return;
    }

    // If the grid size is unspecified, compute one large enough to hold all the elements
    if (gridX == 0 || gridY == 0)
    {
        estimateGridSize(list.size(), gridX, gridY);
    }

    const float w = 1./(float)gridX;
    const float h = 1./(float)gridY;

    unsigned int contentIndex = 0;

    for(int i=0; i<list.size() && contentIndex < gridX*gridY; i++)
    {
        const QFileInfo& fileInfo = list.at(i);
        const QString& filename = fileInfo.absoluteFilePath();

        const unsigned int x_coord = contentIndex % gridX;
        const unsigned int y_coord = contentIndex / gridX;
        const QPointF position(x_coord*w + 0.5*w, y_coord*h + 0.5*h);
        const QSizeF windowSize(w, h);

        const bool success = ContentLoader(g_displayGroupManager).load(filename, position, windowSize);

        if(success)
        {
            ++contentIndex;
            put_flog(LOG_DEBUG, "added file %s", fileInfo.absoluteFilePath().toStdString().c_str());
        }
        else
        {
            put_flog(LOG_DEBUG, "ignoring unsupported file %s", fileInfo.absoluteFilePath().toStdString().c_str());
        }
    }
}

void MainWindow::openContentsDirectory()
{
    QString directoryName = QFileDialog::getExistingDirectory(this);

    if(!directoryName.isEmpty())
    {
        int gridX = QInputDialog::getInt(this, "Grid X dimension", "Grid X dimension", 0, 0);
        int gridY = QInputDialog::getInt(this, "Grid Y dimension", "Grid Y dimension", 0, 0);
        assert( gridX >= 0 && gridY >= 0 );

        addContentDirectory(directoryName, gridX, gridY);
    }
}

void MainWindow::showBackgroundWidget()
{
    assert(g_mpiChannel->getRank() == 0 && "Background widget is intended only for rank 0 application");

    if(!backgroundWidget_)
    {
        backgroundWidget_ = new BackgroundWidget(this);
        backgroundWidget_->setModal(true);
    }

    backgroundWidget_->show();
}

void MainWindow::openWebBrowser()
{
    bool ok;
    QString url = QInputDialog::getText(this, tr("New WebBrowser Content"),
                                         tr("URL:"), QLineEdit::Normal,
                                         WEBBROWSER_DEFAULT_URL, &ok);
    if (ok && !url.isEmpty())
    {
        emit openWebBrowser(QPointF(.5,.5), QSize(), url);
    }
}

void MainWindow::clearContents()
{
    g_displayGroupManager->setContentWindowManagers(ContentWindowManagerPtrs());
}

void MainWindow::saveState()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save State", "", "State files (*.dcx)");

    if(!filename.isEmpty())
    {
        // make sure filename has .dcx extension
        if(!filename.endsWith(".dcx"))
        {
            put_flog(LOG_DEBUG, "appended .dcx filename extension");
            filename.append(".dcx");
        }

        bool success = StateSerializationHelper(g_displayGroupManager).save(filename);

        if(!success)
        {
            QMessageBox::warning(this, "Error", "Could not save state file.", QMessageBox::Ok, QMessageBox::Ok);
        }
    }
}

void MainWindow::loadState()
{
    QString filename = QFileDialog::getOpenFileName(this, "Load State", "", "State files (*.dcx)");

    if(!filename.isEmpty())
    {
        loadState(filename);
    }
}

void MainWindow::loadState(const QString& filename)
{
    if( !StateSerializationHelper(g_displayGroupManager).load(filename ))
    {
        QMessageBox::warning(this, "Error", "Could not load state file.", QMessageBox::Ok, QMessageBox::Ok);
    }
}

void MainWindow::computeImagePyramid()
{
    // get image filename
    const QString imageFilename = QFileDialog::getOpenFileName(this, "Select image");

    if(!imageFilename.isEmpty())
    {
        put_flog(LOG_DEBUG, "source image filename %s", imageFilename.toLocal8Bit().constData());

        const QString imagePyramidPath = imageFilename + DynamicTexture::pyramidFolderSuffix;

        put_flog(LOG_DEBUG, "target image pyramid folder %s", imagePyramidPath.toLocal8Bit().constData());

        DynamicTexturePtr dynamicTexture(new DynamicTexture(imageFilename));
        dynamicTexture->generateImagePyramid(imagePyramidPath);

        put_flog(LOG_DEBUG, "done");
    }
}

#if ENABLE_SKELETON_SUPPORT
void MainWindow::setEnableSkeletonTracking(bool enable)
{
    if(enable == true)
    {
        emit(enableSkeletonTracking());
    }
    else
    {
        emit(disableSkeletonTracking());
    }
}
#endif

QStringList MainWindow::extractValidContentUrls(const QMimeData* mimeData)
{
    QStringList pathList;

    if (mimeData->hasUrls())
    {
        QList<QUrl> urlList = mimeData->urls();

        foreach (QUrl url, urlList)
        {
            QString extension = QFileInfo(url.toLocalFile().toLower()).suffix();
            if (ContentFactory::getSupportedExtensions().contains(extension))
                pathList.append(url.toLocalFile());
        }
    }

    return pathList;
}

QStringList MainWindow::extractFolderUrls(const QMimeData* mimeData)
{
    QStringList pathList;

    if (mimeData->hasUrls())
    {
        QList<QUrl> urlList = mimeData->urls();

        foreach (QUrl url, urlList)
        {
            if (QDir(url.toLocalFile()).exists())
                pathList.append(url.toLocalFile());
        }
    }

    return pathList;
}

QString MainWindow::extractStateFile(const QMimeData* mimeData)
{
    QList<QUrl> urlList = mimeData->urls();
    if (urlList.size() == 1)
    {
        QUrl url = urlList[0];
        QString extension = QFileInfo(url.toLocalFile().toLower()).suffix();
        if (extension == "dcx")
            return url.toLocalFile();
    }
    return QString();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* dragEvent)
{
    const QStringList& pathList = extractValidContentUrls(dragEvent->mimeData());
    const QStringList& dirList = extractFolderUrls(dragEvent->mimeData());
    const QString& stateFile = extractStateFile(dragEvent->mimeData());

    if (!pathList.empty() || !dirList.empty() || !stateFile.isNull())
    {
        dragEvent->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* dropEvt)
{
    const QStringList& pathList = extractValidContentUrls(dropEvt->mimeData());
    foreach (QString url, pathList)
    {
        ContentLoader(g_displayGroupManager).load(url);
    }

    const QStringList& dirList = extractFolderUrls(dropEvt->mimeData());
    if (dirList.size() > 0)
    {
        QString url = dirList[0]; // Only one directory at a time

        addContentDirectory(url);
    }

    const QString& stateFile = extractStateFile(dropEvt->mimeData());
    if (!stateFile.isNull())
    {
        loadState(stateFile);
    }

    dropEvt->acceptProposedAction();
}

void MainWindow::openDock(const QPointF position)
{
    const unsigned int dockWidth = g_configuration->getTotalWidth()*DOCK_WIDTH_RELATIVE_TO_WALL;
    const unsigned int dockHeight = dockWidth * DockPixelStreamer::getDefaultAspectRatio();

    const QString& dockRootDir = static_cast<MasterConfiguration*>(g_configuration)->getDockStartDir();

    emit openDock(position, QSize(dockWidth, dockHeight), dockRootDir);
}

void MainWindow::updateGLWindows()
{
    // receive any waiting messages
    g_mpiChannel->receiveMessages(getGLWindow()->getPixelStreamFactory());

    // synchronize clock right after receiving messages to ensure we have an
    // accurate time for rendering, etc. below
    g_mpiChannel->synchronizeClock();

    if( g_configuration->getOptions()->getShowMouseCursor( ))
        unsetCursor();
    else
        setCursor( QCursor( Qt::BlankCursor ));

    // render all GLWindows
    for(size_t i=0; i<glWindows_.size(); i++)
    {
        activeGLWindow_ = glWindows_[i];
        glWindows_[i]->updateGL();
    }

    // all render processes render simultaneously
    g_mpiChannel->globalBarrier();

    // swap buffers on all windows
    for(size_t i=0; i<glWindows_.size(); i++)
    {
        glWindows_[i]->swapBuffers();
    }

    // advance all contents
    g_displayGroupManager->advanceContents();

    // clear old factory objects and purge any textures
    if(!glWindows_.empty())
    {
        glWindows_[0]->getTextureFactory().clearStaleObjects();
        glWindows_[0]->getDynamicTextureFactory().clearStaleObjects();
        glWindows_[0]->getPDFFactory().clearStaleObjects();
        glWindows_[0]->getSVGFactory().clearStaleObjects();
        glWindows_[0]->getMovieFactory().clearStaleObjects();
        glWindows_[0]->getPixelStreamFactory().clearStaleObjects();
        glWindows_[0]->getPDFFactory().clearStaleObjects();

        glWindows_[0]->purgeTextures();
    }

    // increment frame counter
    ++g_frameCount;

    emit(updateGLWindowsFinished());
}

void MainWindow::finalize()
{
    for(size_t i=0; i<glWindows_.size(); i++)
    {
        glWindows_[i]->finalize();
    }
}
