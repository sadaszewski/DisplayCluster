/*********************************************************************/
/* Copyright (c) 2014, EPFL/Blue Brain Project                       */
/*                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>     */
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

#define BOOST_TEST_MODULE PixelStreamWindowManagerTests
#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

#include "ContentWindowManager.h"
#include "DisplayGroupManager.h"
#include "Options.h"
#include "PixelStreamWindowManager.h"
#include "configuration/MasterConfiguration.h"
#include "globals.h"

#include "MinimalGlobalQtApp.h"
BOOST_GLOBAL_FIXTURE( MinimalGlobalQtApp )

#define CONFIGURATION_FILE "configuration.xml"
#define CONTENT_URI "bla"

BOOST_AUTO_TEST_CASE( testNoStreamerWindowCreation )
{
    DisplayGroupManagerPtr displayGroupManager( new DisplayGroupManager );
    g_configuration = new MasterConfiguration( CONFIGURATION_FILE );

    PixelStreamWindowManager windowManager( *displayGroupManager );

    const QString uri = CONTENT_URI;
    const QPointF pos( .4, .3 );
    const QSizeF size( .1, .2 );

    ContentWindowManagerPtr window = windowManager.createContentWindow( uri, pos, size );
    BOOST_REQUIRE( window );

    BOOST_CHECK_EQUAL( window, windowManager.getContentWindow( uri ));

    const QRectF& coords = window->getCoordinates();
    BOOST_CHECK_EQUAL( coords.center().x(), pos.x( ));
    BOOST_CHECK_EQUAL( coords.center().y(), pos.y( ));
    BOOST_CHECK_EQUAL( coords.width(), size.width( ));
    BOOST_CHECK_EQUAL( coords.height(), size.height( ));

    windowManager.removeContentWindow( uri );
    BOOST_CHECK( !windowManager.getContentWindow( uri ));

    delete g_configuration;
}

BOOST_AUTO_TEST_CASE( testExplicitWindowCreation )
{
    DisplayGroupManagerPtr displayGroupManager( new DisplayGroupManager );
    g_configuration = new MasterConfiguration( CONFIGURATION_FILE );

    PixelStreamWindowManager windowManager( *displayGroupManager );

    const QString uri = CONTENT_URI;
    const QPointF pos( .4, .3 );
    const QSizeF size( .1, .2 );
    const QSize pixels( g_configuration->getTotalWidth() * size.width(),
                        g_configuration->getTotalHeight() * size.height( ));

    ContentWindowManagerPtr window = windowManager.createContentWindow( uri, pos, size );
    BOOST_REQUIRE( window );

    BOOST_CHECK_EQUAL( window, windowManager.getContentWindow( uri ));

    windowManager.openPixelStreamWindow( uri, pixels );
    BOOST_CHECK_EQUAL( window, windowManager.getContentWindow( uri ));

    ContentPtr content = window->getContent();
    BOOST_REQUIRE( content );
    BOOST_CHECK( content->getURI() == uri );
    BOOST_CHECK_EQUAL( content->getType(), CONTENT_TYPE_PIXEL_STREAM );

    const QRectF& coords = window->getCoordinates();
    BOOST_CHECK_EQUAL( coords.center().x(), pos.x( ));
    BOOST_CHECK_EQUAL( coords.center().y(), pos.y( ));
    BOOST_CHECK_EQUAL( coords.width(), size.width( ));
    BOOST_CHECK_EQUAL( coords.height(), size.height( ));

    windowManager.closePixelStreamWindow( uri );
    BOOST_CHECK( !windowManager.getContentWindow( uri ));

    delete g_configuration;
}

BOOST_AUTO_TEST_CASE( testImplicitWindowCreation )
{
    DisplayGroupManagerPtr displayGroupManager( new DisplayGroupManager );
    g_configuration = new MasterConfiguration( CONFIGURATION_FILE );

    PixelStreamWindowManager windowManager( *displayGroupManager );

    const QString uri = CONTENT_URI;
    const QPointF pos( .5, .5 ); // window will be positioned centerred
    const QSizeF size( .4, .3 ); // window will be 1to1 size
    const QSize pixels( g_configuration->getTotalWidth() * size.width(),
                        g_configuration->getTotalHeight() * size.height( ));

    windowManager.openPixelStreamWindow( uri, pixels );
    ContentWindowManagerPtr window = windowManager.getContentWindow( uri );
    BOOST_REQUIRE( window );

    ContentPtr content = window->getContent();
    BOOST_REQUIRE( content );
    BOOST_CHECK( content->getURI() == uri );
    BOOST_CHECK_EQUAL( content->getType(), CONTENT_TYPE_PIXEL_STREAM );

    const QRectF& coords = window->getCoordinates();
    BOOST_CHECK_EQUAL( coords.center().x(), pos.x( ));
    BOOST_CHECK_EQUAL( coords.center().y(), pos.y( ));
    BOOST_CHECK_CLOSE( coords.width(), size.width(), 0.05 );
    BOOST_CHECK_CLOSE( coords.height(), size.height(), 0.05 );

    windowManager.closePixelStreamWindow( uri );
    BOOST_CHECK( !windowManager.getContentWindow( uri ));

    delete g_configuration;
}
