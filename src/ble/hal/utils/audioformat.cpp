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
//  audioformat.cpp
//

#include "audioformat.h"
#include "ctrlm_log_ble.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

// -----------------------------------------------------------------------------
/*!
    \class AudioFormat
    \brief Light wrapper around audio format so it can be used by multiple classes
 */


AudioFormat::AudioFormat()
{
}


AudioFormat::~AudioFormat()
{
}

void AudioFormat::setFrameInfo(uint8_t sizeFrame, uint8_t sizeHeader)
{
    m_sizeFrame    = sizeFrame;
    m_sizeHeader   = sizeHeader;
    m_frameInfoSet = true;
}

bool AudioFormat::getFrameInfo(uint8_t &sizeFrame, uint8_t &sizeHeader) const
{
    if(!m_frameInfoSet) {
        XLOGD_ERROR("frame info not set");
        return(false);
    }
    sizeFrame  = m_sizeFrame;
    sizeHeader = m_sizeHeader;
    return(true);
}

void AudioFormat::setHeaderInfoAdpcm(uint8_t offsetStepSizeIndex, uint8_t offsetPredictedSampleLsb, uint8_t offsetPredictedSampleMsb, uint8_t offsetSequenceValue, uint8_t sequenceValueMin, uint8_t sequenceValueMax)
{
    m_offsetStepSizeIndex      = offsetStepSizeIndex;
    m_offsetPredictedSampleLsb = offsetPredictedSampleLsb;
    m_offsetPredictedSampleMsb = offsetPredictedSampleMsb;
    m_offsetSequenceValue      = offsetSequenceValue;
    m_sequenceValueMin         = sequenceValueMin;
    m_sequenceValueMax         = sequenceValueMax;
    m_headerInfoSet            = true;
}

bool AudioFormat::getHeaderInfoAdpcm(uint8_t &offsetStepSizeIndex, uint8_t &offsetPredictedSampleLsb, uint8_t &offsetPredictedSampleMsb, uint8_t &offsetSequenceValue, uint8_t &sequenceValueMin, uint8_t &sequenceValueMax) const
{
    if(!m_headerInfoSet) {
        XLOGD_ERROR("header info not set");
        return(false);
    }
    offsetStepSizeIndex      = m_offsetStepSizeIndex;
    offsetPredictedSampleLsb = m_offsetPredictedSampleLsb;
    offsetPredictedSampleMsb = m_offsetPredictedSampleMsb;
    offsetSequenceValue      = m_offsetSequenceValue;
    sequenceValueMin         = m_sequenceValueMin;
    sequenceValueMax         = m_sequenceValueMax;
    return(true);
}

void AudioFormat::setPressAndHoldSupport(bool supported)
{
    m_supportsPressAndHold = supported;
}

bool AudioFormat::getPressAndHoldSupport() const
{
    return m_supportsPressAndHold;
}
