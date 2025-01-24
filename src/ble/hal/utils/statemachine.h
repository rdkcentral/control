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
//  statemachine.h
//

#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <queue>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <semaphore.h>
#include <glib.h>

#include "utils/slot.h"

namespace Event {
    typedef enum {
        None = 0,
        StateMachineWrapped,
        User = 1000,
        MaxUser = 65535
    } Type;
};

class StateMachine
{
public:
    StateMachine();
    virtual ~StateMachine();

public:
    bool addState(int state, const std::string &name = std::string());
    bool addState(int parentState, int state, const std::string &name = std::string());

    bool addTransition(int fromState, Event::Type eventType, int toState);

    bool setInitialState(int state);
    bool setInitialState(int parentState, int state);

    bool setFinalState(int state);
    bool setFinalState(int parentState, int state);

    void postEvent(Event::Type eventType);
    int64_t postDelayedEvent(Event::Type eventType, int delay);
    bool cancelDelayedEvent(int64_t id);
    bool cancelDelayedEvents(Event::Type eventType);

    int state() const;
    bool inState(int state) const;
    bool inState(const std::unordered_set<int> &states) const;

    std::string stateName(int state = -1) const;

    bool isRunning() const;
    void setObjectName(std::string name);
    std::string getObjectName();

    
    void setGMainLoop(GMainLoop* main_loop);
    bool start();
    void stop();

public:
    static const Event::Type FinishedEvent = Event::StateMachineWrapped;


public:
    Slots<> m_finished;
    Slots<int> m_entered;
    Slots<int> m_exited;
    Slots<int, int> m_transition;

    void addFinishedHandler(const Slot<> &func)
    {
        m_finished.addSlot(func);
    }
    void addEnteredHandler(const Slot<int> &func)
    {
        m_entered.addSlot(func);
    }
    void addExitedHandler(const Slot<int> &func)
    {
        m_exited.addSlot(func);
    }
    void addTransitionHandler(const Slot<int, int> &func)
    {
        m_transition.addSlot(func);
    }

    int shouldMoveState(Event::Type eventType) const;
    void triggerStateMove(int newState);
    
    void logTransition(int oldState, int newState) const;

private:
    void moveToState(int newState);

    std::vector<int> stateTreeFor(int state, bool bottomUp) const;

    void cleanUpEvents();

public:
    class PostEventData {
    public:
        PostEventData(std::shared_ptr<bool> isAlive, StateMachine *sm, Event::Type eventType)
            : m_isAlive(std::move(isAlive))
            , m_sm(sm)
            , m_eventType(eventType)
        {
            sem_init(&m_semaphore, 0, 0);
        }

        ~PostEventData()
        {
            sem_destroy(&m_semaphore);
        }

        std::shared_ptr<bool> m_isAlive;
        StateMachine *m_sm;
        Event::Type m_eventType;
        sem_t m_semaphore;
    };


private:
    std::shared_ptr<bool> m_isAlive;
    GMainLoop* m_GMainLoop;

    struct Transition {
        int targetState;
        enum { EventTransition, SignalTransition } type;
        Event::Type eventType;
    };

    struct State {
        int parentState;
        int initialState;
        bool hasChildren;
        bool isFinal;
        std::string name;
        std::vector<Transition> transitions;
    };

    std::map<int, State> m_states;

    int m_currentState;
    int m_initialState;
    int m_finalState;

    bool m_running;
    std::string m_objectName;


    bool m_stopPending;
    bool m_withinStateMover;
    std::queue<Event::Type> m_localEvents;

    uint64_t m_delayedEventIdCounter;

public:

    struct DelayedEvent {
        unsigned int timerId;
        Event::Type eventType;
    };
    std::mutex m_delayedEventsLock;
    std::map<int64_t, DelayedEvent> m_delayedEvents;
};


#endif // !defined(STATEMACHINE_H)
