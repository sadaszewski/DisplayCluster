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

#ifndef MASTERWINDOW_H
#define MASTERWINDOW_H

#include "config.h"
#include "types.h"

#include <QMainWindow>
#include <QMimeData>

class MultiTouchListener;
class BackgroundWidget;

/**
 * The main UI window for Master applications.
 *
 * It lets users control the contents displayed on the wall.
 */
class MasterWindow : public QMainWindow
{
    Q_OBJECT

public:
    /** Constructor. */
    MasterWindow(DisplayGroupManagerPtr displayGroup);

    /** Destructor. */
    ~MasterWindow();

signals:
    /** Emitted when users want to open a dock. */
    void openDock(QPointF pos, QSize size, QString rootDir);

    /** Emitted when users want to hide the dock. */
    void hideDock();

    /** Emitted when users want to open a webbrowser. */
    void openWebBrowser(QPointF pos, QSize size, QString url);

#if ENABLE_SKELETON_SUPPORT
    void enableSkeletonTracking();
    void disableSkeletonTracking();
#endif

protected:
    ///@{
    /** Drag events re-implemented from QMainWindow. */
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    ///@}

private slots:
    void openContent();
    void openContentsDirectory();

    void saveState();
    void loadState();

    void computeImagePyramid();
    void showBackgroundWidget();

    void openWebBrowser();
    void openDock(const QPointF position);

#if ENABLE_SKELETON_SUPPORT
    void setEnableSkeletonTracking(bool enable);
#endif

private:
    void setupMasterWindowUI();

    void addContentDirectory(const QString &directoryName, unsigned int gridX=0, unsigned int gridY=0);
    void loadState(const QString &filename);

    void estimateGridSize(unsigned int numElem, unsigned int& gridX, unsigned int& gridY);

    QStringList extractValidContentUrls(const QMimeData* mimeData);
    QStringList extractFolderUrls(const QMimeData *mimeData);
    QString extractStateFile(const QMimeData *mimeData);

    DisplayGroupManagerPtr displayGroup_;
    BackgroundWidget* backgroundWidget_;

#if ENABLE_TUIO_TOUCH_LISTENER
    MultiTouchListener* touchListener_;
#endif
};

#endif // MASTERWINDOW_H
