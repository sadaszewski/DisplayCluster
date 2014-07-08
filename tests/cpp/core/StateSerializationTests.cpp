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

#define BOOST_TEST_MODULE StateSerializationTests
#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

#include "State.h"

#include "types.h"
#include "Content.h"
#include "DummyContent.h"
#include "ContentWindowManager.h"

#define PLEASE_REMOVE_ME

#ifdef PLEASE_REMOVE_ME
#include "globals.h"
#include "configuration/Configuration.h"

#include "MinimalGlobalQtApp.h"
BOOST_GLOBAL_FIXTURE( MinimalGlobalQtApp )
#endif

const int CONTENT_WIDTH = 100;
const int CONTENT_HEIGHT = 100;
const int DUMMY_PARAM_VALUE = 10;
const QString DUMMY_URI = "/dummuy/uri";

BOOST_AUTO_TEST_CASE( testWhenStateIsSerializedAndDeserializedThenContentPropertiesArePreserved )
{
#ifdef PLEASE_REMOVE_ME
    g_configuration = new Configuration("configuration.xml");
#endif

    // Serialize
    std::stringstream ss;
    {
        DummyContent* dummyContent = new DummyContent(DUMMY_URI);
        ContentPtr content( dummyContent );
        dummyContent->dummyParam_ = DUMMY_PARAM_VALUE;

        content->setDimensions( CONTENT_WIDTH, CONTENT_HEIGHT );
        ContentWindowManagerPtr window( new ContentWindowManager(content) );

        ContentWindowManagerPtrs contentWindows;
        contentWindows.push_back(window);
        State state(contentWindows);
        boost::archive::xml_oarchive oa(ss);
        oa << BOOST_SERIALIZATION_NVP(state);
    }

    // Deserialize
    ContentWindowManagerPtrs contentWindows;
    {
        State state;
        boost::archive::xml_iarchive ia(ss);
        ia >> BOOST_SERIALIZATION_NVP(state);
        contentWindows = state.getContentWindows();
    }

    BOOST_REQUIRE_EQUAL( contentWindows.size(), 1 );
    DummyContent* dummyContent = dynamic_cast<DummyContent*>(contentWindows[0]->getContent().get());
    BOOST_REQUIRE( dummyContent );

    int width, height;
    dummyContent->getDimensions(width, height);

    BOOST_CHECK_EQUAL( width, CONTENT_WIDTH );
    BOOST_CHECK_EQUAL( height, CONTENT_HEIGHT );
    BOOST_CHECK_EQUAL( dummyContent->dummyParam_, DUMMY_PARAM_VALUE );
    BOOST_CHECK_EQUAL( dummyContent->getType(), CONTENT_TYPE_ANY );
    BOOST_CHECK_EQUAL( dummyContent->getURI().toStdString(), DUMMY_URI.toStdString() );
}

