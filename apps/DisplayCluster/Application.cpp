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

#include "Application.h"

#include "DisplayGroupManager.h"
#include "log.h"
#include "configuration/Configuration.h"

#define CONFIGURATION_FILENAME "configuration.xml"
#define DISPLAYCLUSTER_DIR "DISPLAYCLUSTER_DIR"

Application::Application(int &argc_, char **argv_)
    : QApplication(argc_, argv_)
{
    g_displayGroupManager.reset( new DisplayGroupManager );
}

Application::~Application()
{
    delete g_configuration;
    g_configuration = 0;
}

QString Application::getConfigFilename() const
{
    if( !getenv( DISPLAYCLUSTER_DIR ))
    {
        put_flog(LOG_FATAL, "DISPLAYCLUSTER_DIR environment variable must be set");
        exit(EXIT_FAILURE);
    }

    const QString displayClusterDir = QString(getenv( DISPLAYCLUSTER_DIR ));
    put_flog(LOG_DEBUG, "base directory is %s", displayClusterDir.toLatin1().constData());
    return QString( "%1/%2" ).arg( displayClusterDir ).arg( CONFIGURATION_FILENAME );
}
