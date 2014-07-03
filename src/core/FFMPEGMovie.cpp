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

#include "FFMPEGVideoFrameConverter.h"

#include "log.h"

#include "globals.h"

#define INVALID_STREAM_INDEX -1

#define MICROSEC 1000000.0

#pragma clang diagnostic ignored "-Wdeprecated"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

FFMPEGMovie::FFMPEGMovie(const QString& uri)
    : avFormatContext_(0)
    , videoCodecContext_(0)
    , avFrame_(0)
    , videoStream_(0)
    , videoFrameConverter_(0)
    // Seeking parameters
    , den2_(0)
    , num2_(0)
    , numFrames_(0)
    , frameDurationInSeconds_(0)
    // Internal
    , loop_(false)
    , frameDecodingComplete_(0)
    , isValid_(false)
    , skippedFrames_(false)
    // Public status
    , newFrameAvailable_(false)
{
    FFMPEGMovie::initGlobalState();

    isValid_ = open(uri);
}

FFMPEGMovie::~FFMPEGMovie()
{
    closeVideoStreamDecoder();
    releaseAvFormatContext();

    av_free(avFrame_);
}

bool FFMPEGMovie::open(const QString& uri)
{
    if (!createAvFormatContext(uri))
        return false;

    if(!findVideoStream())
        return false;

    if (!openVideoStreamDecoder())
        return false;

    avFrame_ = avcodec_alloc_frame();
    if( !avFrame_ )
    {
        put_flog(LOG_ERROR, "error allocating frames, can't initialize.");
        return false;
    }

    videoFrameConverter_ = new FFMPEGVideoFrameConverter(*videoCodecContext_, PIX_FMT_RGBA);

    generateSeekingParameters();

    return true;
}

bool FFMPEGMovie::createAvFormatContext(const QString& uri)
{
    // Read movie file header information into avFormatContext_ (and allocating it if null)
    if(avformat_open_input(&avFormatContext_, uri.toAscii(), NULL, NULL) != 0)
    {
        put_flog(LOG_ERROR, "could not open movie file %s", uri.toLocal8Bit().constData());
        return false;
    }

    // Read stream information into avFormatContext_->streams
    if(avformat_find_stream_info(avFormatContext_, NULL) < 0)
    {
        put_flog(LOG_ERROR, "could not find stream information");
        return false;
    }

    av_dump_format(avFormatContext_, 0, uri.toAscii(), 0); // dump format information to stderr
    return true;
}

void FFMPEGMovie::releaseAvFormatContext()
{
    avformat_close_input(&avFormatContext_);
}

bool FFMPEGMovie::findVideoStream()
{
    for(unsigned int i=0; i<avFormatContext_->nb_streams; ++i)
    {
        if(avFormatContext_->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream_ = avFormatContext_->streams[i]; // Shortcut pointer - don't free
            return true;
        }
    }

    put_flog(LOG_ERROR, "could not find video stream");

    return false;
}

bool FFMPEGMovie::openVideoStreamDecoder()
{
    // Contains information about the codec that the stream is using
    videoCodecContext_ = videoStream_->codec; // Shortcut - don't free

    AVCodec * codec = avcodec_find_decoder(videoCodecContext_->codec_id);
    if(!codec)
    {
        put_flog(LOG_ERROR, "unsupported codec");
        return false;
    }

    // open codec
    const int ret = avcodec_open2(videoCodecContext_, codec, NULL);

    if(ret < 0)
    {
        char errbuf[256];
        av_strerror(ret, errbuf, 256);

        put_flog(LOG_ERROR, "could not open codec, error code %i: %s", ret, errbuf);
        return false;
    }

    return true;
}

void FFMPEGMovie::closeVideoStreamDecoder() const
{
    avcodec_close( videoCodecContext_ );
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
    return videoCodecContext_->width;
}

unsigned int FFMPEGMovie::getHeight() const
{
    return videoCodecContext_->height;
}

const void* FFMPEGMovie::getData() const
{
    return videoFrameConverter_->getData();
}

void FFMPEGMovie::setLoop(const bool loop)
{
    loop_ = loop;
}

double FFMPEGMovie::getDuration() const
{
    const double duration = (double)videoStream_->duration *
            (double)videoStream_->time_base.num / (double)videoStream_->time_base.den;
    return std::max(duration, 0.0);
}

bool FFMPEGMovie::jumpTo(const double timePosInSeconds)
{
    newFrameAvailable_ = false;

    const int64_t frameIndex = timePosInSeconds / frameDurationInSeconds_;
    if (!seekToNearestFullframe(frameIndex))
        return false;

    do {
        if(!readVideoFrame())
            return false;
    }
    while( !frameDecodingComplete_ );

    timePosition_ = boost::posix_time::microseconds(timePosInSeconds * MICROSEC);

    convertVideoFrame();
    return true;
}

void FFMPEGMovie::update(const boost::posix_time::time_duration timeSinceLastFrame, const bool skipDecoding)
{
    newFrameAvailable_ = false;

    // If decoding is slower than frame rate, slow down decoding speed
    const double timeIncrementInSec = timeSinceLastFrame.total_microseconds() / MICROSEC;
    if (timeIncrementInSec < frameDurationInSeconds_)
        timePosition_ += timeSinceLastFrame;
    else
    {
        const double factor = frameDurationInSeconds_ / timeIncrementInSec;
        const double increment = factor * frameDurationInSeconds_;
        timePosition_ += boost::posix_time::microseconds(increment * MICROSEC);
    }

    if (skipDecoding)
    {
        skippedFrames_ = true;
        return;
    }

    if (skippedFrames_)
        clampTimePosition();

    const double timePositionInSec = timePosition_.total_microseconds() / MICROSEC;
    const int64_t index = timePositionInSec / frameDurationInSeconds_;

    bool readNewVideoFrame = false;

    // Catch up on missed frames
    if (skippedFrames_)
    {
        readNewVideoFrame = seekToNearestFullframe(index);
        skippedFrames_ = false;
    }

    // Read frames until we reach the correct timestamp
    while(avFrame_->pkt_dts < getTimestampForFrameIndex(index))
    {
        readNewVideoFrame = true;

        // Read one frame and return to start if EOF reached
        if(!readVideoFrame() && loop_)
        {
            rewind();
            break;
        }
    }

    if (readNewVideoFrame)
        convertVideoFrame();
}

bool FFMPEGMovie::isNewFrameAvailable() const
{
    return newFrameAvailable_;
}

void FFMPEGMovie::clampTimePosition()
{
    const double duration = getDuration();

    if(duration <= 0.0)
        return;

    while (timePosition_.total_microseconds() / MICROSEC > duration)
        timePosition_ -= boost::posix_time::microseconds(duration * MICROSEC);
}

int64_t FFMPEGMovie::getTimestampForFrameIndex(const int64_t frameIndex) const
{
    if (frameIndex < 0 || (numFrames_ && frameIndex >= numFrames_))
    {
        put_flog(LOG_WARN, "Invalid index: %i - valid range: [0, %i[",
                 frameIndex, numFrames_);
    }

    int64_t timestamp = av_rescale(frameIndex, den2_, num2_);

    if (videoStream_->start_time != (int64_t)AV_NOPTS_VALUE)
        timestamp += videoStream_->start_time;

    return timestamp;
}

bool FFMPEGMovie::seekToNearestFullframe(const int64_t frameIndex)
{
    if (frameIndex < 0 || (numFrames_ && frameIndex >= numFrames_))
    {
        put_flog(LOG_WARN, "Invalid index: %i, seeking aborted.", frameIndex);
        return false;
    }

    const int64_t desiredTimestamp = getTimestampForFrameIndex(frameIndex);

    // Seek to the nearest keyframe before desiredTimestamp.
    if(avformat_seek_file(avFormatContext_, videoStream_->index, 0, desiredTimestamp,
                          desiredTimestamp, AVSEEK_FLAG_FRAME) != 0)
    {
        put_flog(LOG_ERROR, "seeking error, seeking aborted.");
        return false;
    }

    // Always flush buffers after seeking
    avcodec_flush_buffers(videoCodecContext_);

    // Read a valid frame after seeking to get a meaningful avFrame_->pkt_dts
    return readVideoFrame();
}

bool FFMPEGMovie::readVideoFrame()
{
    int avReadStatus = 0;

    AVPacket packet;
    av_init_packet(&packet);

    // keep reading frames until we decode a valid video frame
    while((avReadStatus = av_read_frame(avFormatContext_, &packet)) >= 0)
    {
        if(isVideoStream(packet) && decodeVideoFrame(packet))
        {
            // free the packet that was allocated by av_read_frame
            av_free_packet(&packet);
            break;
        }
        // free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }

    // False if file read error or EOF reached
    return avReadStatus >= 0;
}

bool FFMPEGMovie::isVideoStream(const AVPacket& packet) const
{
    return packet.stream_index == videoStream_->index;
}

void FFMPEGMovie::rewind()
{
    if (av_seek_frame(avFormatContext_, videoStream_->index, 0, AVSEEK_FLAG_BACKWARD) >= 0)
    {
        put_flog(LOG_DEBUG, "Time: %lf", timePosition_.total_microseconds() / MICROSEC );

        timePosition_ = boost::posix_time::time_duration();

        // Always flush buffers after seeking
        avcodec_flush_buffers(videoCodecContext_);

        readVideoFrame();
    }
}

bool FFMPEGMovie::decodeVideoFrame(AVPacket& packet)
{
    // decode video frame
    int errCode = avcodec_decode_video2(videoCodecContext_, avFrame_, &frameDecodingComplete_, &packet);
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

    return true;
}

bool FFMPEGMovie::convertVideoFrame()
{
    if (!videoFrameConverter_->convert(avFrame_))
        return false;

    newFrameAvailable_ = true;
    return true;
}

void FFMPEGMovie::generateSeekingParameters()
{
    // generate seeking parameters
    den2_ = videoStream_->time_base.den * videoStream_->r_frame_rate.den;
    num2_ = videoStream_->time_base.num * videoStream_->r_frame_rate.num;

    numFrames_ = (videoStream_->duration > 0) ?
                 av_rescale(videoStream_->duration, num2_, den2_) : 0;

    frameDurationInSeconds_ = (double)videoStream_->r_frame_rate.den /
                              (double)videoStream_->r_frame_rate.num;

    put_flog(LOG_DEBUG, "seeking parameters: start_time = %i, duration_ = %i, numFrames_ = %i",
             videoStream_->start_time, videoStream_->duration, numFrames_);
    put_flog(LOG_DEBUG, "                    frame_rate = %f, time_base = %f",
             (float)videoStream_->r_frame_rate.num/(float)videoStream_->r_frame_rate.den,
             (float)videoStream_->time_base.num/(float)videoStream_->time_base.den);
    put_flog(LOG_DEBUG, "                    frameDurationInSeconds_ = %f",
             (float)frameDurationInSeconds_);
}
