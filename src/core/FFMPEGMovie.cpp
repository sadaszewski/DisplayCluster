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

#include "FFMPEGMovie.h"

#include "log.h"

#include "globals.h"

FFMPEGMovie::FFMPEGMovie(const QString& uri)
    : avFormatContext_(0)
    , avCodecContext_(0)
    , swsContext_(0)
    , avFrame_(0)
    , avFrameRGB_(0)
    , streamIdx_(-1)
    , videostream_(0)
    // Seeking parameters
    , den2_(0)
    , num2_(0)
    , numFrames_(0)
    , frameIndex_(0)
    , frameDurationInSeconds_(0)
    // Internal
    , loop_(false)
    , frameDecodingComplete_(0)
    , isValid_(false)
{
    FFMPEGMovie::initGlobalState();
    isValid_ = open(uri);
}

FFMPEGMovie::~FFMPEGMovie()
{
    // free contexts
    avcodec_close( avCodecContext_ );
    avformat_close_input(&avFormatContext_);
    sws_freeContext(swsContext_);

    // free frames
    avpicture_free( (AVPicture *)avFrameRGB_ );
    av_free(avFrame_);
    av_free(avFrameRGB_);
}

bool FFMPEGMovie::open(const QString& uri)
{
    // open movie file
    if(avformat_open_input(&avFormatContext_, uri.toAscii(), NULL, NULL) != 0)
    {
        put_flog(LOG_ERROR, "could not open movie file %s", uri.toLocal8Bit().constData());
        return false;
    }

    // get stream information
    if(avformat_find_stream_info(avFormatContext_, NULL) < 0)
    {
        put_flog(LOG_ERROR, "could not find stream information");
        return false;
    }

    // dump format information to stderr
    av_dump_format(avFormatContext_, 0, uri.toAscii(), 0);

    // find the first video stream
    streamIdx_ = -1;

    for(unsigned int i=0; i<avFormatContext_->nb_streams; i++)
    {
        if(avFormatContext_->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            streamIdx_ = i;
            break;
        }
    }

    if(streamIdx_ == -1)
    {
        put_flog(LOG_ERROR, "could not find video stream");
        return false;
    }

    // Shortcuts - no need to free
    videostream_ = avFormatContext_->streams[streamIdx_];
    avCodecContext_ = videostream_->codec;

    // find the decoder for the video stream
    AVCodec * codec = avcodec_find_decoder(avCodecContext_->codec_id);

    if(!codec)
    {
        put_flog(LOG_ERROR, "unsupported codec");
        return false;
    }

    // open codec
    int ret = avcodec_open2(avCodecContext_, codec, NULL);

    if(ret < 0)
    {
        char errbuf[256];
        av_strerror(ret, errbuf, 256);

        put_flog(LOG_ERROR, "could not open codec, error code %i: %s", ret, errbuf);
        return false;
    }

    // allocate video frame for video decoding
    avFrame_ = avcodec_alloc_frame();

    // allocate video frame for RGB conversion
    avFrameRGB_ = avcodec_alloc_frame();

    if( !avFrame_ || !avFrameRGB_ )
    {
        put_flog(LOG_ERROR, "error allocating frames");
        return false;
    }

    // alloc buffer for avFrameRGB_
    avpicture_alloc( (AVPicture *)avFrameRGB_, PIX_FMT_RGBA,
                     avCodecContext_->width, avCodecContext_->height );

    // create sws scaler context
    swsContext_ = sws_getContext(avCodecContext_->width, avCodecContext_->height,
                                 avCodecContext_->pix_fmt, avCodecContext_->width,
                                 avCodecContext_->height, PIX_FMT_RGBA, SWS_FAST_BILINEAR,
                                 NULL, NULL, NULL);
    if( !swsContext_ )
    {
        put_flog(LOG_ERROR, "Allocate an SwsContext");
        return false;
    }

    // generate seeking parameters
    den2_ = videostream_->time_base.den * videostream_->r_frame_rate.den;
    num2_ = videostream_->time_base.num * videostream_->r_frame_rate.num;

    numFrames_ = av_rescale(videostream_->duration, num2_, den2_);
    frameDurationInSeconds_ = (double)videostream_->r_frame_rate.den /
                              (double)videostream_->r_frame_rate.num;

    // Boost time format for frame duration
    frameDurationPosix_ = boost::posix_time::microseconds(frameDurationInSeconds_ * 1000000.);

    put_flog(LOG_DEBUG, "seeking parameters: start_time = %i, duration_ = %i, num frames = %i",
             videostream_->start_time, videostream_->duration, numFrames_);
    put_flog(LOG_DEBUG, "                  : frame_rate = %f, time_base = %f",
             (float)videostream_->r_frame_rate.num/(float)videostream_->r_frame_rate.den,
             (float)videostream_->time_base.num/(float)videostream_->time_base.den);

    return true;
}

void FFMPEGMovie::initGlobalState()
{
    static bool initialized = false;

    if (!initialized)
    {
        av_register_all();
        initialized = true;
    }
}

bool FFMPEGMovie::isValid() const
{
    return isValid_;
}

unsigned int FFMPEGMovie::getWidth() const
{
    return avCodecContext_->width;
}

unsigned int FFMPEGMovie::getHeight() const
{
    return avCodecContext_->height;
}

const void* FFMPEGMovie::getData() const
{
    return avFrameRGB_->data[0];
}

void FFMPEGMovie::setLoop(const bool loop)
{
    loop_ = loop;
}

double FFMPEGMovie::getDuration() const
{
    double duration = (double)videostream_->duration *
            (double)videostream_->time_base.num / (double)videostream_->time_base.den;
    return duration > 0.0 ? duration : 0.0;
}

bool FFMPEGMovie::jumpTo(double timePos)
{
    const int64_t frameIndex = timePos/frameDurationInSeconds_;
    if (!seekToNearestFullframe(frameIndex))
        return false;

    do {
        if(!readVideoFrame())
            return false;
    }
    while( !frameDecodingComplete_ );
    return true;
}

void FFMPEGMovie::update(const boost::posix_time::time_duration timeSinceLastFrame, const bool skipDecoding)
{
    // rate limiting
    timeAccum_ += timeSinceLastFrame;

    if (skipDecoding)
        return;

    // Check how by many frames we need to move
    const size_t frameIncrement = computeFrameIncrement();

    if ( frameIncrement == 0 )
        return;

    if( frameIncrement == 1 )
    {
        // Read one frame and return to start if EOF reached
        if(!readVideoFrame() && loop_)
            rewind();
    }
    else
    {
        seekTo( (frameIndex_ + frameIncrement) % numFrames_ ); // Catch up on missed frames
    }
}

size_t FFMPEGMovie::computeFrameIncrement()
{
    size_t frameIncrement = 0;

    while( timeAccum_ > frameDurationPosix_ )
    {
        timeAccum_ -= frameDurationPosix_;
        ++frameIncrement;
    }

    return frameIncrement;
}

int64_t FFMPEGMovie::getTimestampForFrameIndex(const int64_t frameIndex) const
{
    assert (frameIndex >= 0 && frameIndex < numFrames_);

    return videostream_->start_time + av_rescale(frameIndex, den2_, num2_);
}

bool FFMPEGMovie::seekTo(const int64_t frameIndex)
{
    if (frameIndex < 0 || frameIndex >= numFrames_)
    {
        put_flog(LOG_WARN, "Invalid index: %i, seeking aborted.", frameIndex);
        return false;
    }

    const int64_t desiredTimestamp = getTimestampForFrameIndex(frameIndex);

    // Default behaviour is to seek to the nearest keyframe before desiredTimestamp
    // AVSEEK_FLAG_ANY lets us seek to the exact frame index so we don't loose synchronization.
    if(avformat_seek_file(avFormatContext_, streamIdx_, 0, desiredTimestamp,
                          desiredTimestamp, AVSEEK_FLAG_ANY) != 0)
    {
        put_flog(LOG_ERROR, "seeking error, seeking aborted.");
        return false;
    }

    // Always flush buffers after seeking
    avcodec_flush_buffers(avCodecContext_);

    frameIndex_ = frameIndex;
    timeAccum_ = boost::posix_time::time_duration();

    return true;
}

bool FFMPEGMovie::seekToNearestFullframe(const int64_t frameIndex)
{
    if (frameIndex < 0 || frameIndex >= numFrames_)
    {
        put_flog(LOG_WARN, "Invalid index: %i, seeking aborted.", frameIndex);
        return false;
    }

    const int64_t desiredTimestamp = getTimestampForFrameIndex(frameIndex);

    // Seek to nearest fullframe
    if(avformat_seek_file(avFormatContext_, streamIdx_, 0, desiredTimestamp,
                          desiredTimestamp, AVSEEK_FLAG_FRAME) != 0)
    {
        put_flog(LOG_ERROR, "seeking error, seeking aborted.");
        return false;
    }

    // Always flush buffers after seeking
    avcodec_flush_buffers(avCodecContext_);

    frameIndex_ = -1; // Warning: We have no idea where we are!!
    timeAccum_ = boost::posix_time::time_duration();

    return true;
}

bool FFMPEGMovie::readVideoFrame()
{
    int avReadStatus = 0;

    AVPacket packet;
    av_init_packet(&packet);

    // keep reading frames until we decode a valid video frame
    while((avReadStatus = av_read_frame(avFormatContext_, &packet)) >= 0)
    {
        if(isVideoStream(packet))
        {
            ++frameIndex_;

            // Ignore return code, we never skip incorrect frames to maintain synchronization
            decodeVideoFrame(packet);
            // free the packet that was allocated by av_read_frame
            av_free_packet(&packet);
            break;
        }
        // free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }

    //put_flog(LOG_DEBUG, "Rank%i read %i", g_mpiRank, frameIndex_);

    // False if file read error or EOF reached
    return avReadStatus >= 0 && frameIndex_ < numFrames_;
}

bool FFMPEGMovie::isVideoStream(const AVPacket& packet) const
{
    return packet.stream_index == streamIdx_;
}

void FFMPEGMovie::rewind()
{
    seekTo(0);
}

bool FFMPEGMovie::decodeVideoFrame(AVPacket& packet)
{
    // decode video frame
    int errCode = avcodec_decode_video2(avCodecContext_, avFrame_, &frameDecodingComplete_, &packet);
    if(errCode < 0)
    {
        put_flog(LOG_WARN, "avcodec_decode_video2 returned error code: '%i'", errCode);
        return false;
    }

    // make sure we got a full video frame and convert the frame from its native format to RGB
    if(!frameDecodingComplete_)
    {
        put_flog(LOG_INFO, "Frame could not be decoded entierly (may be caused by seeking).");
        return false;
    }

    sws_scale(swsContext_, avFrame_->data, avFrame_->linesize, 0, avCodecContext_->height,
              avFrameRGB_->data, avFrameRGB_->linesize);

    //put_flog(LOG_DEBUG, "Rank%i : Decoded frame with PTS: %i", g_mpiRank, avFrame_->pkt_dts);
    return true;
}
