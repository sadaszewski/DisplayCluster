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

#include "Options.h"

Options::Options()
    : showWindowBorders_(false)
    , showTouchPoints_(false)
    , showMovieControls_(true)
    , showTestPattern_(false)
    , enableMullionCompensation_(true)
    , showZoomContext_(true)
    , showStreamingSegments_(false)
    , showStreamingStatistics_(false)
#if ENABLE_SKELETON_SUPPORT
    , showSkeletons_(true)
#endif
{
}

bool Options::getShowWindowBorders() const
{
    return showWindowBorders_;
}

bool Options::getShowTouchPoints() const
{
    return showTouchPoints_;
}

bool Options::getShowMovieControls() const
{
    return showMovieControls_;
}

bool Options::getShowTestPattern() const
{
    return showTestPattern_;
}

bool Options::getEnableMullionCompensation() const
{
    return enableMullionCompensation_;
}

bool Options::getShowZoomContext() const
{
    return showZoomContext_;
}

bool Options::getShowStreamingSegments() const
{
    return showStreamingSegments_;
}

bool Options::getShowStreamingStatistics() const
{
    return showStreamingStatistics_;
}

QColor Options::getBackgroundColor() const
{
    return backgroundColor_;
}

#if ENABLE_SKELETON_SUPPORT
bool Options::getShowSkeletons() const
{
    return showSkeletons_;
}
#endif

void Options::setShowWindowBorders(bool set)
{
    showWindowBorders_ = set;

    emit(updated(shared_from_this()));
}

void Options::setShowTouchPoints(bool set)
{
    showTouchPoints_ = set;

    emit(updated(shared_from_this()));
}

void Options::setShowMovieControls(bool set)
{
    showMovieControls_ = set;

    emit(updated(shared_from_this()));
}

void Options::setShowTestPattern(bool set)
{
    showTestPattern_ = set;

    emit(updated(shared_from_this()));
}

void Options::setEnableMullionCompensation(bool set)
{
    enableMullionCompensation_ = set;

    emit(updated(shared_from_this()));
}

void Options::setShowZoomContext(bool set)
{
    showZoomContext_ = set;

    emit(updated(shared_from_this()));
}

void Options::setShowStreamingSegments(bool set)
{
    showStreamingSegments_ = set;

    emit(updated(shared_from_this()));
}

void Options::setShowStreamingStatistics(bool set)
{
    showStreamingStatistics_ = set;

    emit(updated(shared_from_this()));
}

void Options::setBackgroundColor(QColor color)
{
    if(color == backgroundColor_)
        return;
    backgroundColor_ = color;

    emit (updated(shared_from_this()));
}

#if ENABLE_SKELETON_SUPPORT
void Options::setShowSkeletons(bool set)
{
    showSkeletons_ = set;

    emit(updated(shared_from_this()));
}
#endif
