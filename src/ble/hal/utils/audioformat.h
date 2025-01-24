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
//  audioformat.h
//

#ifndef AUDIOFORMAT_H
#define AUDIOFORMAT_H

#include <stdint.h>

class AudioFormat
{
public:
    AudioFormat();
    ~AudioFormat();

public:
    bool isValid() const;
    void setFrameInfo(uint8_t sizeFrame, uint8_t sizeHeader);
    bool getFrameInfo(uint8_t &sizeFrame, uint8_t &sizeHeader) const;

    void setHeaderInfoAdpcm(uint8_t offsetStepSizeIndex, uint8_t offsetPredictedSampleLsb, uint8_t offsetPredictedSampleMsb, uint8_t offsetSequenceValue, uint8_t sequenceValueMin, uint8_t sequenceValueMax);
    bool getHeaderInfoAdpcm(uint8_t &offsetStepSizeIndex, uint8_t &offsetPredictedSampleLsb, uint8_t &offsetPredictedSampleMsb, uint8_t &offsetSequenceValue, uint8_t &sequenceValueMin, uint8_t &sequenceValueMax) const;

    void setPressAndHoldSupport(bool supported);
    bool getPressAndHoldSupport() const;
private:
    uint8_t m_sizeFrame                = 0;
    uint8_t m_sizeHeader               = 0;
    uint8_t m_offsetStepSizeIndex      = 0;
    uint8_t m_offsetPredictedSampleLsb = 0;
    uint8_t m_offsetPredictedSampleMsb = 0;
    uint8_t m_offsetSequenceValue      = 0;
    uint8_t m_sequenceValueMin         = 0;
    uint8_t m_sequenceValueMax         = 0;

    bool    m_frameInfoSet             = false;
    bool    m_headerInfoSet            = false;

    bool    m_supportsPressAndHold     = false;
};

#endif // !defined(AUDIOFORMAT_H)