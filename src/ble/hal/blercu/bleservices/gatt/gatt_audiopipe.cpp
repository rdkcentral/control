/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2017-2020 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Changes made by Comcast
 * Copyright 2024 Comcast Cable Communications Management, LLC
 * Licensed under the Apache License, Version 2.0
 */

//
//  gatt_audiopipe.cpp
//

#include "gatt_audiopipe.h"

#include "ctrlm_log_ble.h"

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "safec_lib.h"

using namespace std;

// -----------------------------------------------------------------------------
/*!
    \class GattAudioPipe
    \brief Reads data from GATT / bluez notification pipe, decodes the audio
    data and writes it to the output pipe.

    The source of the data is a notification pipe from bluez, over this we will
    get 20 byte packets that correspond to a notification from the RCU.  This
    class converts those to frames before writing them to the output pipe.

    When either the input or the output pipe is closed a corresponding signal
    is generated.

    When this object is destroyed both the input and output pipes are closed.

 */



// -----------------------------------------------------------------------------
/*!
    Constructs a new \l{GattAudioPipe} object without an input notifiction pipe.
    Use this constructor if you're manually injecting BLE GATT notifications
    into the pipe.

 */
GattAudioPipe::GattAudioPipe(uint8_t frameSize, uint32_t frameCountMax, cbFrameValidator frameValidator, int outputPipeFd)
    : m_isAlive(make_shared<bool>(true))
    , m_outputPipeRdFd(-1)
    , m_outputPipeWrFd(-1)
    , m_frameSize(frameSize)
    , m_frameBufferOffset(0)
    , m_frameValidator(frameValidator)
    , m_running(false)
    , m_frameCount(0)
    , m_frameCountMax(frameCountMax)
    , m_recordingTimer(0)
    , m_recordingDuration(0)
{

    if(frameSize > sizeof(m_frameBuffer)) {
        XLOGD_ERROR("frame size is too large <%u>", frameSize);
        frameSize = sizeof(m_frameBuffer);
    }

    if (outputPipeFd >= 0) {
        // dup the output file descriptor and use that as write end
        m_outputPipeWrFd = fcntl(outputPipeFd, F_DUPFD_CLOEXEC, 3);
        if (m_outputPipeWrFd < 0) {
            int errsv = errno;
            XLOGD_ERROR("failed to dup ouput file/fifo/pipe: error = <%d>, <%s>", errsv, strerror(errsv));
            return;
        }

        // set the pipe as non-blocking
        if (fcntl(m_outputPipeWrFd, F_SETFL, O_NONBLOCK) != 0) {
            int errsv = errno;
            XLOGD_ERROR("failed to set O_NONBLOCK flag on pipe: error = <%d>, <%s>", errsv, strerror(errsv));
        }


    } else {
        int flags = O_CLOEXEC | O_NONBLOCK | O_DIRECT;

        // create the new pipe for output
        int fds[2];
        if (pipe2(fds, flags) != 0) {
            int errsv = errno;
            XLOGD_ERROR("failed to create output audio pipe: error = <%d>, <%s>", errsv, strerror(errsv));
            return;
        }

        m_outputPipeRdFd = fds[0];
        m_outputPipeWrFd = fds[1];
    }

}

// -----------------------------------------------------------------------------
/*!
    Destructs the object which involves just terminating all the event handlers
    and closing all the pipes.

 */
GattAudioPipe::~GattAudioPipe()
{
    *m_isAlive = false;

    // close any fds that we may still have open
    if ((m_outputPipeRdFd >= 0) && (::close(m_outputPipeRdFd) != 0)) {
        int errsv = errno;
        XLOGD_ERROR("failed to close output read pipe fd: error = <%d>, <%s>", errsv, strerror(errsv));
    }

    if ((m_outputPipeWrFd >= 0) && (::close(m_outputPipeWrFd) != 0)) {
        int errsv = errno;
        XLOGD_ERROR("failed to close output write pipe fd: error = <%d>, <%s>", errsv, strerror(errsv));
    }

    if (m_recordingTimer) {
        g_timer_destroy(m_recordingTimer);
    }

}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if both the input and output pipes are currently open.

 */
bool GattAudioPipe::isValid() const
{
    return (m_outputPipeWrFd >= 0);
}

// -----------------------------------------------------------------------------
/*!
    Returns \c true if output pipe is not blocked, i.e. the remote end has not
    closed it's pipe.

 */
bool GattAudioPipe::isOutputOpen() const
{
    if (m_outputPipeWrFd < 0) {
        return false;
    }

    // try and empty write to the pipe, this will fail (with EPIPE) if the
    // read size is closed - even though we aren't sending anything
    return (::write(m_outputPipeWrFd, nullptr, 0) == 0);
}

// -----------------------------------------------------------------------------
/*!
    Starts the recording and streaming of data to the output pipe.

 */
bool GattAudioPipe::start()
{
    if (!isValid()) {
        return false;
    }

    if (m_running) {
        XLOGD_WARN("audio pipe already running");
        return false;
    }

    m_recordingTimer = g_timer_new();
    m_frameCount = 0;
    m_recordingDuration = 0;

    m_running = true;

    return true;
}

// -----------------------------------------------------------------------------
/*!
    Stops streaming data to the output pipe.

 */
void GattAudioPipe::stop()
{
    if (!m_running) {
        XLOGD_WARN("audio pipe wasn't running");
        return;
    }

    m_running = false;

    gulong microseconds;
    m_recordingDuration = g_timer_elapsed(m_recordingTimer, &microseconds);
}

// -----------------------------------------------------------------------------
/*!
    Simply returns the number of frames received.

 */
uint32_t GattAudioPipe::framesReceived() const
{
    return m_frameCount;
}

// -----------------------------------------------------------------------------
/*!
    Returns the number of frames expected.

    There are two ways of calculating this, one is to use the time length of
    the recording and then work out how many frames we should have received.
    The other is to use the sequence numbers to calculate how many frames we've
    missed.  Both have problems; the time base is just an estimate, whereas the
    sequence numbers can wrap (if you dropped more than 255 frames).  Also the
    sequence number wouldn't take in account packets dropped at the start of
    the recording.

    So the solution I came up with is to use the sequence numbers if they are
    within the ballpark of the time based estimate, and by ballpark I mean
    a 16 frame difference, which corresponds to 192ms (16 * 12ms per frame).

 */
uint32_t GattAudioPipe::framesExpected(uint32_t lostFrameCount, uint32_t usecPerFrame) const
{
    // calculate the time estimate
    gulong microseconds;

    const double msecsElapsed = m_recordingTimer ?
        g_timer_elapsed(m_recordingTimer, &microseconds) : m_recordingDuration;

    int timeEstimate = static_cast<int>( ((int64_t)msecsElapsed) * 1000 / usecPerFrame);

    XLOGD_DEBUG("audio frames expected: timeBased=%d, seqNumberBased=%d",
           timeEstimate, (m_frameCount + lostFrameCount));

    // if the missed sequence count is within 16 frames of the time estimate
    // then use that, otherwise use the time
    int diff = timeEstimate - (m_frameCount + lostFrameCount);
    if (abs(diff) <= 16) {
        return (m_frameCount + lostFrameCount);
    } else {
        return timeEstimate;
    }
}

// -----------------------------------------------------------------------------
/*!
    Takes the read end of the output pipe, this is typically then passed on
    to vsdk to read the decoded audio from.

 */
int GattAudioPipe::takeOutputReadFd()
{
    if (m_outputPipeRdFd < 0) {
        return -1;
    }

    int dupFd = fcntl(m_outputPipeRdFd, F_DUPFD_CLOEXEC, 3);
    if (dupFd < 0) {
        int errsv = errno;
        XLOGD_ERROR("failed to dup read side of pipe: error = <%d>, <%s>", errsv, strerror(errsv));
        return -1;
    }

    // now close our internal copy
    if (::close(m_outputPipeRdFd) != 0) {
        int errsv = errno;
        XLOGD_ERROR("failed to close internal copy of read end of output pipe: error = <%d>, <%s>", 
                errsv, strerror(errsv));
    }
    m_outputPipeRdFd = -1;

    return dupFd;
}

// -----------------------------------------------------------------------------
/*!
    Call to manually inject a 20 byte notification into the pipe.  Only use this
     if the object wasn't created with a notification pipe.

 */
bool GattAudioPipe::addNotification(const uint8_t value[20], const uint8_t length)
{
    bool endOfStream = false;
    if(m_frameBufferOffset + length > m_frameSize) {
        XLOGD_ERROR("received invalid frame size <%zu>", m_frameBufferOffset + length);
        m_frameBufferOffset = 0;
        return endOfStream;
    }

    if(m_frameBufferOffset == 0 && m_frameCount == 0) { // First chunk of first frame
        ctrlm_timestamp_get_monotonic(&m_firstAudioDataTime);
    }

    // add the notification to the buffer, if we have a complete frame
    // then pass on to the decoder
    errno_t safec_rc = memcpy_s(m_frameBuffer + m_frameBufferOffset, m_frameSize - m_frameBufferOffset, value, length);
    ERR_CHK(safec_rc);

    m_frameBufferOffset += length;


    if (m_frameBufferOffset == m_frameSize) {
        if (!m_running) {
            XLOGD_WARN("received GATT notification before pipe was running");
        } else {
            endOfStream = processAudioFrame();
        }
        m_frameBufferOffset = 0;
    }
    return(endOfStream);
}

// -----------------------------------------------------------------------------
/*!
    Call to set the frame count maximum during the stream.

    Returns true if more frames are needed to reach the maximum, otherwise false 
 */
bool GattAudioPipe::setFrameCountMax(uint32_t frameCountMax) {
    XLOGD_INFO("frame count max <%u> still need <%d> frames", frameCountMax, frameCountMax - m_frameCount);
    m_frameCountMax = frameCountMax;
    return(m_frameCount < m_frameCountMax);
}

bool GattAudioPipe::getFirstAudioDataTime(ctrlm_timestamp_t &time) {
    if(m_frameBufferOffset == 0 && m_frameCount == 0) { // No audio data received yet
        return false;
    }
    time = m_firstAudioDataTime;
    return true;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Decodes the audio frame and then writes the PCM 16-bit samples into the
    output pipe.

 */
bool GattAudioPipe::processAudioFrame()
{
    // if not running then just discard the frame
    if (!m_running) {
        return false;
    }
    const uint8_t *frame = m_frameBuffer;

    if(m_frameValidator != NULL) {
        m_frameValidator(frame, m_frameCount);
    }

    // increment the count of audio frames received
    m_frameCount++;

    const size_t bufferSize = m_frameSize;

    // write the pcm data into the output pipe
    if (m_outputPipeWrFd >= 0) {
        ssize_t wr = TEMP_FAILURE_RETRY(::write(m_outputPipeWrFd, frame, bufferSize));
        if (wr < 0) {
            // check if the pipe is full
            if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
                XLOGD_WARN("voice audio pipe is full, frame discarded");
            } else {
                // check if VSDK closed the pipe, this is not an error so don't log it as such
                if (errno == EPIPE) {
                    XLOGD_INFO("output voice audio pipe closed by client");
                } else {
                    int errsv = errno;
                    XLOGD_ERROR("output voice audio pipe write failed: error = <%d>, <%s>", errsv, strerror(errsv));
                }

                // close down the pipe
                onOutputPipeException(m_outputPipeWrFd);
            }
        } else if (static_cast<size_t>(wr) != bufferSize) {
            XLOGD_WARN("only %zd of the possible %zu bytes of audio data could be sent", wr, bufferSize);
        }
    }

    if(m_frameCountMax > 0 && m_frameCount >= m_frameCountMax) { // if frame count max is non-zero then the stream ends when the max is reached 
        XLOGD_INFO("frame count limited reached <%u>", m_frameCount);
        return true;
    }
    return false;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Slot called when the output pipe is closed.  AS does this when it wants
    to stop audio streaming, so it is not handled as an error.

 */
void GattAudioPipe::onOutputPipeException(int pipeFd)
{
    if (pipeFd != m_outputPipeWrFd) {
        return;
    }

    XLOGD_DEBUG("detected close on the client output pipe");

    // close the output pipe
    if ((m_outputPipeWrFd >= 0) && (::close(m_outputPipeWrFd) != 0)) {
        int errsv = errno;
        XLOGD_ERROR("failed to close output pipe: error = <%d>, <%s>", errsv, strerror(errsv));
    }
    m_outputPipeWrFd = -1;

    // let the parent statemachine know that the output pipe is closed (this
    // triggers the state-machine to ask the RCU to stop sending data)
    m_outputPipeClosedSlots.invoke();
}

