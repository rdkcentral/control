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
//  bleconnectionparameters.h
//

#ifndef BLECONNECTIONPARAMETERS_H
#define BLECONNECTIONPARAMETERS_H


class BleConnectionParameters
{
public:
    BleConnectionParameters();
    BleConnectionParameters(const BleConnectionParameters &other);
    BleConnectionParameters(double minInterval, double maxInterval, int latency,
                            int supervisionTimeout);
    ~BleConnectionParameters();

    BleConnectionParameters &operator=(const BleConnectionParameters &other);

    bool operator!=(const BleConnectionParameters &other) const;
    bool operator==(const BleConnectionParameters &other) const;

public:
    int latency() const;
    void setLatency(int latency);

    double maximumInterval() const;
    double minimumInterval() const;
    void setIntervalRange(double minimum, double maximum);

    void setSupervisionTimeout(int timeout);
    int supervisionTimeout() const;


private:
    double m_maxInterval;
    double m_minInterval;
    int m_latency;
    int m_supvTimeout;
};


#endif // !defined(BLECONNECTIONPARAMETERS_H)
