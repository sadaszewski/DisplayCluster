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

#define BOOST_TEST_MODULE ContentWindowManagerTests
#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

#include <globals.h>
#include <Options.h>
#include <ContentWindowManager.h>
#include <configuration/MasterConfiguration.h>

#include "MinimalGlobalQtApp.h"
BOOST_GLOBAL_FIXTURE( MinimalGlobalQtApp )

#include "DummyContent.h"

const int WIDTH = 100;
const int HEIGHT = 100;

BOOST_AUTO_TEST_CASE( testInitialSize )
{
    g_configuration =
        new Configuration( "configuration.xml" );

    ContentPtr content( new DummyContent );
    content->setDimensions( WIDTH, HEIGHT );
    ContentWindowManager window( content );

    const QRectF& coords = window.getCoordinates();

    const double normWidth = double(WIDTH) / g_configuration->getTotalWidth();
    const double normHeight = double(HEIGHT) / g_configuration->getTotalHeight();

    // default 1:1 size, center on wall
    BOOST_CHECK_EQUAL( coords.x(), (1. - normWidth) * .5 );
    BOOST_CHECK_EQUAL( coords.y(), (1. - normHeight) * .5 );
    BOOST_CHECK_EQUAL( coords.width(), normWidth );
    BOOST_CHECK_EQUAL( coords.height(), normHeight );

    delete g_configuration;
}

BOOST_AUTO_TEST_CASE( testFullScreenSize )
{
    g_configuration =
        new MasterConfiguration( "configuration.xml" );

    ContentPtr content( new DummyContent );
    content->setDimensions( WIDTH, HEIGHT );
    ContentWindowManager window( content );

    window.toggleFullscreen();

    const QRectF& coords = window.getCoordinates();

    const double wallAR = 1. / g_configuration->getAspectRatio();
    const double normWidth = 1. * wallAR;
    const double normHeight = 1.;

    // full screen, center on wall
    BOOST_CHECK_EQUAL( coords.x(), (1. - normWidth) * .5 );
    BOOST_CHECK_EQUAL( coords.y(), (1. - normHeight) * .5 );
    BOOST_CHECK_EQUAL( coords.width(), normWidth );
    BOOST_CHECK_EQUAL( coords.height(), normHeight );

    delete g_configuration;
}

BOOST_AUTO_TEST_CASE( testFromFullscreenBackToNormalized )
{
    g_configuration =
        new MasterConfiguration( "configuration.xml" );

    ContentPtr content( new DummyContent );
    content->setDimensions( WIDTH, HEIGHT );
    ContentWindowManager window( content );

    QRectF target( 0.9, 0.7, 0.2, 1 );
    target.setHeight( target.width() * g_configuration->getAspectRatio());

    window.setSize( target.width(), target.height( ));
    window.setPosition( target.x(), target.y( ));

    QRectF coords = window.getCoordinates();
    BOOST_CHECK_EQUAL( coords.x(), target.x( ));
    BOOST_CHECK_EQUAL( coords.y(), target.y( ));
    BOOST_CHECK_EQUAL( coords.width(), target.width( ));
    BOOST_CHECK_EQUAL( coords.height(), target.height( ));

    window.toggleFullscreen();
    window.toggleFullscreen();

    coords = window.getCoordinates();

    // back to original position and size
    BOOST_CHECK_EQUAL( coords.x(), target.x( ));
    BOOST_CHECK_EQUAL( coords.y(), target.y( ));
    BOOST_CHECK_EQUAL( coords.width(), target.width( ));
    BOOST_CHECK_EQUAL( coords.height(), target.height( ));

    delete g_configuration;
}

BOOST_AUTO_TEST_CASE( testValidID )
{
    g_configuration =
        new MasterConfiguration( "configuration.xml" );

    ContentPtr content( new DummyContent );
    content->setDimensions( WIDTH, HEIGHT );
    ContentWindowManager window( content );

    BOOST_CHECK( window.getID() != QUuid());

    delete g_configuration;
}

BOOST_AUTO_TEST_CASE( testUniqueID )
{
    g_configuration =
        new MasterConfiguration( "configuration.xml" );

    ContentPtr content( new DummyContent );
    content->setDimensions( WIDTH, HEIGHT );

    ContentWindowManager window1( content );
    BOOST_CHECK( window1.getID() != QUuid());

    ContentWindowManager window2( content );
    BOOST_CHECK( window2.getID() != QUuid());

    BOOST_CHECK( window1.getID() != window2.getID());

    delete g_configuration;
}

BOOST_AUTO_TEST_CASE( testSetContent )
{
    g_configuration =
        new MasterConfiguration( "configuration.xml" );

    ContentPtr content( new DummyContent );
    content->setDimensions( WIDTH, HEIGHT );

    ContentWindowManager window;
    BOOST_CHECK(!window.getContent());

    int contentWidth, contentHeight;
    window.getContentDimensions(contentWidth, contentHeight);
    BOOST_CHECK_EQUAL(contentWidth, 0);
    BOOST_CHECK_EQUAL(contentHeight, 0);

    window.setContent(content);
    BOOST_CHECK_EQUAL(window.getContent(), content);
    window.getContentDimensions(contentWidth, contentHeight);
    BOOST_CHECK_EQUAL(contentWidth, WIDTH);
    BOOST_CHECK_EQUAL(contentHeight, HEIGHT);

    window.setContent(ContentPtr());
    BOOST_CHECK(!window.getContent());
    window.getContentDimensions(contentWidth, contentHeight);
    BOOST_CHECK_EQUAL(contentWidth, 0);
    BOOST_CHECK_EQUAL(contentHeight, 0);

    delete g_configuration;
}
