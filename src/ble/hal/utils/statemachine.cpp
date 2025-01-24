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
//  statemachine.cpp
//

#include "statemachine.h"
#include "ctrlm_log_ble.h"

#include <algorithm>

using namespace std;


StateMachine::StateMachine()
    : m_isAlive(make_shared<bool>(true))
    , m_GMainLoop(NULL)
    , m_currentState(-1)
    , m_initialState(-1)
    , m_finalState(-1)
    , m_running(false)
    , m_stopPending(false)
    , m_withinStateMover(false)
    , m_delayedEventIdCounter(1)
{
}

StateMachine::~StateMachine()
{
    *m_isAlive = false;
    cleanUpEvents();
}

void StateMachine::setObjectName(std::string name)
{
    m_objectName = std::move(name);
}
std::string StateMachine::getObjectName()
{
    return m_objectName;
}

void StateMachine::setGMainLoop(GMainLoop* main_loop)
{
    m_GMainLoop = main_loop;
}

// -----------------------------------------------------------------------------
/*!
    \internal

    Creates a log message string using \a oldState and \a newState and sends it
    out the designated log channel and category.

    \see StateMachine::setTransistionLogLevel()
    \see StateMachine::transistionLogLevel()
    \see StateMachine::transistionLogCategory()
 */
void StateMachine::logTransition(int oldState, int newState) const
{
    auto oldStateObj = m_states.find(oldState);
    auto newStateObj = m_states.find(newState);
    string oldStateStr = (oldStateObj == m_states.end()) ? "STATE NOT FOUND" : oldStateObj->second.name;
    string newStateStr = (newStateObj == m_states.end()) ? "STATE NOT FOUND" : newStateObj->second.name;

    if (oldState == newState) {
         XLOGD_DEBUG("[%s] re-entering state %s(%d)", m_objectName.c_str(), newStateStr.c_str(), newState);
    } else if (oldState == -1) {
         XLOGD_DEBUG("[%s] moving to state %s(%d)", m_objectName.c_str(), newStateStr.c_str(), newState);
    } else {
         XLOGD_DEBUG("[%s] moving from state %s(%d) to %s(%d)", m_objectName.c_str(), 
                oldStateStr.c_str(), oldState, 
                newStateStr.c_str(), newState);
    }
}

void StateMachine::cleanUpEvents()
{
    // clear all the queued events
    m_localEvents = {};

    // clean up any delayed events with lock held
    std::lock_guard<std::mutex> lock(m_delayedEventsLock);

    for (auto &event : m_delayedEvents) {
        // kill the timer (safe to do in destructor ?)
        if (event.second.timerId > 0) {
            g_source_remove(event.second.timerId);
            event.second.timerId = 0;
        }
    }

    m_delayedEvents.clear();
}

std::vector<int> StateMachine::stateTreeFor(int state, bool bottomUp) const
{
    vector<int> tree;

    // to speed this up we don't do any checks on the state values, we assume
    // this is done when the states are added
    do {
        if (bottomUp) {
            tree.push_back(state);
        } else {
            tree.insert(tree.begin(), state);
        }

        state = (m_states.find(state) == m_states.end()) ? -1 : m_states.find(state)->second.parentState;

    } while (state >= 0);

    return tree;
}

void StateMachine::moveToState(int newState)
{
    // if the new state is equal to the current state then this is not an error
    // and just means we haveto issue the exited, transistion and entered
    // signals for the state
    if (newState == m_currentState) {

        logTransition(m_currentState, newState);

        m_exited.invoke(m_currentState);
        m_transition.invoke(m_currentState, m_currentState);
        m_entered.invoke(m_currentState);

    } else {

        // lookup the new state to check if we should be moving to an initial state
        map<int, State>::const_iterator it = m_states.find(newState);

        // if the state has one or more children then it's a super state and
        // we should be moving to the initial state
        if (it->second.hasChildren) {

            // sanity check we have an initial state
            if (it->second.initialState == -1) {
                XLOGD_WARN("try to move to super state %s(%d) but no initial state set",
                         it->second.name.c_str(), newState);
                return;
            }

            // set the new state to be the initial state of the super state
            newState = it->second.initialState;
        }

        //
        int oldState = m_currentState;
        m_currentState = newState;

        logTransition(oldState, newState);


        // get the set of states we're currently in (includes parents)
        vector<int> newStates = stateTreeFor(newState, false);
        vector<int> oldStates = stateTreeFor(oldState, true);


        // emit the exit signal for any states we left
        for (const int &_oldState : oldStates) {
            if (std::find(newStates.begin(), newStates.end(), _oldState) == newStates.end()) {
                m_exited.invoke(_oldState);
            }
        }

        // emit a transition signal
        m_transition.invoke(oldState, m_currentState);

        // emit the entry signal for any states we've now entered
        for (const int &_newState : newStates) {
            if (std::find(oldStates.begin(), oldStates.end(), _newState) == oldStates.end()) {
                m_entered.invoke(_newState);
            }
        }
    }


    // check if the new state is a final state of a super state, in which case
    // post a FinishedEvent to the message loop
    if (m_states[newState].isFinal) {
        postEvent(FinishedEvent);
    }


    // check if the new state is a final state for the state machine and if so
    // stop the state machine
    if ((m_currentState == m_finalState) || m_stopPending) {

        m_running = false;
        cleanUpEvents();

        if (m_currentState == m_finalState) {
            m_finished.invoke();
        }
        m_currentState = -1;
    }
}

void StateMachine::triggerStateMove(int newState)
{
    if (g_main_loop_is_running(m_GMainLoop) && !g_main_context_is_owner(g_main_loop_get_context(m_GMainLoop))) {
        XLOGD_ERROR("[%s] state transition triggered from wrong thread, asserting...", m_objectName.c_str());
        g_assert(g_main_context_is_owner(g_main_loop_get_context(m_GMainLoop)));
    }

    m_withinStateMover = true;

    // move to the new state, this will emit signals that may result in more
    // events being added to local event queue
    moveToState(newState);

    // then check if we have any other events on the queue, note we can get
    // into an infinite loop here if the code using the statemachine is
    // poorly designed, however that's their fault not mine
    while (m_running && !m_localEvents.empty()) {
        const Event::Type eventType = m_localEvents.front();
        m_localEvents.pop();

        // check if this event should trigger a state move and if so
        // move the state once again
        newState = shouldMoveState(eventType);
        if (newState != -1) {
            moveToState(newState);
        }
    }

    m_withinStateMover = false;
}

int StateMachine::shouldMoveState(Event::Type eventType) const
{
    // check if this event triggers any transactions
    int state = m_currentState;
    do {
        // find the current state and sanity check it is in the map
        map<int, State>::const_iterator it = m_states.find(state);
        if (it == m_states.end()) {
            XLOGD_ERROR("invalid state %d (this shouldn't happen)", state);
            return -1;
        }

        // iterate through the transitions of this state and see if any trigger on this event
        for (const Transition &transition : it->second.transitions) {
            if ((transition.type == Transition::EventTransition) &&
                (transition.eventType == eventType)) {

                // some extra sanity checks that the target state is valid on debug builds
                if (m_states.find(transition.targetState) == m_states.end()) {
                    XLOGD_ERROR("invalid target state %d (this shouldn't happen)", transition.targetState);
                } else if ((m_states.find(transition.targetState)->second.hasChildren == true) &&
                           (m_states.find(transition.targetState)->second.initialState == -1)) {
                    XLOGD_ERROR("trying to move to a super state with no initial state set");
                }
                // return the state we should be moving to
                return transition.targetState;
            }
        }

        // if this state had a parent state, then see if that matches the
        // event and therefore should transition
        state = it->second.parentState;

    } while (state != -1);

    return -1;
}

static gboolean timerEvent(gpointer user_data)
{
    StateMachine *sm = (StateMachine*)user_data;
    if (sm == nullptr || !sm->isRunning()) {
        return false;
    }

    unsigned int timerId = g_source_get_id(g_main_current_source());

    // take the lock before accessing the delay events map
    sm->m_delayedEventsLock.lock();

    // if a timer then use the id to look-up the actual event the was put in the delayed queue
    map<int64_t, StateMachine::DelayedEvent>::iterator it = sm->m_delayedEvents.begin();
    for (; it != sm->m_delayedEvents.end(); ++it) {
        if (it->second.timerId == timerId) {
            break;
        }
    }

    if (it != sm->m_delayedEvents.end()) {
        // we found timerId of this event

        // if we have a valid delayed event then swap out the event type
        // to the one in the delayed list
        Event::Type delayedEventType = it->second.eventType;

        // XLOGD_DEBUG("[%s] Found delayed event, type = %d", sm->getObjectName().c_str(), delayedEventType);

        // free the delayed entry
        sm->m_delayedEvents.erase(it);

        // release the lock so clients can add other delayed events in their state change callbacks
        sm->m_delayedEventsLock.unlock();

        // check if this event triggers any transactions
        int newState = sm->shouldMoveState(delayedEventType);
        if (newState != -1) {
            sm->triggerStateMove(newState);
        }

    } else {
        XLOGD_WARN("[%s] Delayed event triggered (id = %u) but didn't find it in our map, suspect something went wrong....", 
                sm->getObjectName().c_str(), timerId);
        sm->m_delayedEventsLock.unlock();
    }
    
    return false;
}

bool StateMachine::addState(int state, const string &name)
{
    return addState(-1, state, name);
}

bool StateMachine::addState(int parentState, int state, const string &name)
{
    // can't add states while running (really - we're single threaded, why not?)
    if (m_running) {
        XLOGD_WARN("can't add states while running");
        return false;
    }

    // check the state is a positive integer
    if (state < 0) {
        XLOGD_WARN("state's must be positive integers");
        return false;
    }

    // check we don't already have this state
    if (m_states.find(state) != m_states.end()) {
        XLOGD_WARN("already have state %s(%d), not adding again",
                 m_states[state].name.c_str(), state);
        return false;
    }

    // if a parent was supplied then increment it's child count
    if (parentState != -1) {
        map<int, State>::iterator parent = m_states.find(parentState);

        // if a parent was supplied make sure we have that parent state
        if (parent == m_states.end()) {
            XLOGD_WARN("try to add state %s(%d) with missing parent state %d",
                     name.c_str(), state, parentState);
            return false;
        }

        // increment the number of child states
        parent->second.hasChildren = true;
    }

    // add the state
    State stateStruct;

    stateStruct.parentState = parentState;
    stateStruct.initialState = -1;
    stateStruct.hasChildren = false;
    stateStruct.isFinal = false;
    stateStruct.name = name;

    m_states[state] = std::move(stateStruct);

    return true;
}

bool StateMachine::addTransition(int fromState, Event::Type eventType, int toState)
{
    // can't add transitions while running (really - we're single threaded, why not?)
    if (m_running) {
        XLOGD_WARN("can't add transitions while running");
        return false;
    }

    // sanity check the event type
    if (eventType == Event::None) {
        XLOGD_WARN("eventType is invalid (%d)", int(eventType));
        return false;
    }

    // sanity check we have a 'from' state
    map<int, State>::iterator from = m_states.find(fromState);
    if (from == m_states.end()) {
        XLOGD_WARN("missing 'fromState' %d", fromState);
        return false;
    }

    // and we have a 'to' state
    map<int, State>::const_iterator to = m_states.find(toState);
    if (to == m_states.end()) {
        XLOGD_WARN("missing 'toState' %d", toState);
        return false;
    }

    // also check if the to state is a super state that it has in initial
    // state set
    if ((to->second.hasChildren == true) && (to->second.initialState == -1)) {
        XLOGD_WARN("'toState' %s(%d) is a super state with no initial state set",
                 to->second.name.c_str(), toState);
        return false;
    }

    // add the transition
    Transition transition;
    transition.targetState = toState;
    transition.type = Transition::EventTransition;
    transition.eventType = eventType;

    from->second.transitions.push_back(std::move(transition));

    return true;
}


// -----------------------------------------------------------------------------
/*!
    Sets the initial \a state of the state machine, this must be called before
    starting the state machine.

 */
bool StateMachine::setInitialState(int state)
{
    // can't set initial state while running (really - we're single threaded, why not?)
    if (m_running) {
        XLOGD_WARN("can't set initial state while running");
        return false;
    }

    // sanity check we know about the state
    if (m_states.find(state) == m_states.end()) {
        XLOGD_WARN("can't set initial state to %d as don't have that state", state);
        return false;
    }

    m_initialState = state;
    return true;
}

// -----------------------------------------------------------------------------
/*!
    Sets the initial \a initialState of the super state \a parentState. This is
    used when a transition has the super state as a target.

    It is not necessary to define an initial state of the a super state, for
    example if a super state is never a target for a transition there is no
    need to call this method.

 */
bool StateMachine::setInitialState(int parentState, int initialState)
{
    // can't set initial state while running (really - we're single threaded, why not?)
    if (m_running) {
        XLOGD_WARN("can't set initial state while running");
        return false;
    }

    // get the parent state
    map<int, State>::iterator parent = m_states.find(parentState);
    if (parent == m_states.end()) {
        XLOGD_WARN("can't find parent state %d", parentState);
        return false;
    }

    // sanity check we know about the given initial state
    map<int, State>::const_iterator initial = m_states.find(initialState);
    if (initial == m_states.end()) {
        XLOGD_WARN("can't set initial state to %d as don't have that state",
                 initialState);
        return false;
    }

    // sanity check the given initial state has the same parent
    if (initial->second.parentState != parentState) {
        XLOGD_WARN("can't set initial state to %d as parent state doesn't match",
                 initialState);
        return false;
    }

    // check if we already an initial state, this is not fatal but raise a warning
    if (parent->second.initialState != -1) {
        XLOGD_WARN("replacing existing initial state %d to %d",
                 parent->second.initialState, initialState);
    }

    parent->second.initialState = initialState;
    return true;
}

// -----------------------------------------------------------------------------
/*!
    Sets the final \a state of the state machine, this can't be a super state.
    When the state machine reaches this state it is automatically stopped and
    a finished() signal is emitted.

    It is not necessary to define an final state if the state machine never
    finishes.

    \sa setFinalState(int, int)
 */
bool StateMachine::setFinalState(int state)
{
    // can't set final state while running (really - we're single threaded, why not?)
    if (m_running) {
        XLOGD_WARN("can't set final state while running");
        return false;
    }

    // sanity check we know about the state
    if (m_states.find(state) == m_states.end()) {
        XLOGD_WARN("can't set final state to %d as don't have that state", state);
        return false;
    }

    m_finalState = state;
    return true;
}

// -----------------------------------------------------------------------------
/*!
    Sets the final \a finalState of the super state \a parentState. This is used
    when a transition has the super state as a source and an event of
    type StateMachine::FinishedEvent.

    It is not necessary to define an final state of the a super state, for
    example if a super state is never a source for a transition with a
    StateMachine::FinishedEvent event then there is no need to call this method.

    \sa setFinalState(int)
 */
bool StateMachine::setFinalState(int parentState, int finalState)
{
    // can't set final state while running (really - we're single threaded, why not?)
    if (m_running) {
        XLOGD_WARN("can't set final state while running");
        return false;
    }

    // get the parent state
    map<int, State>::const_iterator parent = m_states.find(parentState);
    if (parent == m_states.end()) {
        XLOGD_WARN("can't find parent state %d", parentState);
        return false;
    }

    // sanity check we know about the given final state
    map<int, State>::iterator fin = m_states.find(finalState);
    if (fin == m_states.end()) {
        XLOGD_WARN("can't set final state to %d as don't have that state", finalState);
        return false;
    }

    // sanity check the given initial state has the same parent
    if (fin->second.parentState != parentState) {
        XLOGD_WARN("can't set final state to %d as parent state doesn't match", finalState);
        return false;
    }

    fin->second.isFinal = true;
    return true;
}

// -----------------------------------------------------------------------------
/*!
    \threadsafe


 */

static gboolean postEventInMainThread(gpointer user_data)
{
    StateMachine::PostEventData *event = (StateMachine::PostEventData*)user_data;
    if (event == nullptr) {
        XLOGD_WARN("state machine event data is null, ignoring event...");
        return false;
    }
    if (!event->m_isAlive || *(event->m_isAlive) == false) {
        XLOGD_WARN("state machine is not alive, ignoring event...");
        sem_post(&event->m_semaphore);
        return false;
    }
    if (event->m_sm) {
        event->m_sm->postEvent(event->m_eventType);
    }

    sem_post(&event->m_semaphore);
    return false;
}

void StateMachine::postEvent(Event::Type eventType)
{
    if (!m_running) {
        XLOGD_WARN("cannot post event when the state machine is not running");
        return;
    }
    if ((eventType != FinishedEvent) && ((eventType < Event::User) || (eventType > Event::MaxUser))) {
        XLOGD_WARN("event type must be in user event range (%d <= %d <= %d)", 
                Event::User, eventType, Event::MaxUser);
        return;
    }

    if (!g_main_loop_is_running(m_GMainLoop) || g_main_context_is_owner(g_main_loop_get_context(m_GMainLoop))) {

        // the calling thread is the same as ours so post the event to our
        // local queue if inside a handler, otherwise just process the event
        // immediately
        if (m_withinStateMover) {

            // just for debugging
            if (m_localEvents.size() > 1024) {
                XLOGD_WARN("state machine event queue getting large");
            }

            // queue it up
            m_localEvents.push(eventType);

        } else {

            // not being called from within our own state mover so check if
            // this event will trigger the current state to move, if so
            // trigger that
            int newState = shouldMoveState(eventType);

            // check if we should be moving to a new state
            if (newState != -1) {
                triggerStateMove(newState);
            }
        }

    } else {

        XLOGD_DEBUG("[%s] state machine event triggered outside main context, sending now to the main context thread...", m_objectName.c_str());
        PostEventData *event = new PostEventData(m_isAlive, this, eventType);
        g_timeout_add(0, postEventInMainThread, (gpointer)event);
        // Need to wait for the state transitions to complete because these state machines
        // were written under the assumption that transitions would block the caller.
        sem_wait(&event->m_semaphore);
        delete event;
    }
}

// -----------------------------------------------------------------------------
/*!
    \threadsafe


 */
int64_t StateMachine::postDelayedEvent(Event::Type eventType, int delay)
{
    if (!m_running) {
        XLOGD_WARN("cannot post delayed event when the state machine is not running");
        return -1;
    }
    if ((eventType != FinishedEvent) &&
        ((eventType < Event::User) || (eventType > Event::MaxUser))) {
        XLOGD_WARN("event type must be in user event range (%d <= %d <= %d)",
                 Event::User, eventType, Event::MaxUser);
        return -1;
    }
    if (delay < 0) {
        XLOGD_WARN("delay cannot be negative");
        return -1;
    }

    // take the lock before accessing the delay events map
    std::lock_guard<std::mutex> lock(m_delayedEventsLock);

    // create a timer and then pin the event to the timer
    const unsigned int timerId = g_timeout_add(delay, timerEvent, this);

    // get a unique id
    int64_t id = m_delayedEventIdCounter++;
    m_delayedEvents[id] = { timerId, eventType };
    
    return id;
}

// -----------------------------------------------------------------------------
/*!
    \threadsafe


 */
bool StateMachine::cancelDelayedEvent(int64_t id)
{
    XLOGD_DEBUG("Enter...");
    if (!m_running) {
        XLOGD_WARN("the state machine is not running");
        return false;
    }
    if (id < 0) {
        XLOGD_WARN("invalid delayed event id");
        return false;
    }

    // take the lock before accessing the delay events map
    std::lock_guard<std::mutex> lock(m_delayedEventsLock);

    // try and find the id in the delayed events map
    map<int64_t, DelayedEvent>::iterator it = m_delayedEvents.find(id);
    if (it == m_delayedEvents.end()) {
        return false;
    }

    if (it->second.timerId > 0) {
        g_source_remove(it->second.timerId);
        it->second.timerId = 0;
    }

    m_delayedEvents.erase(it);

    return true;
}

// -----------------------------------------------------------------------------
/*!
    \threadsafe

    Cancels all previously posted delayed event using the \a eventType.  Unlike
    the version that takes an id, this version may cancel more that one delayed
    event.

    Returns \c true if one or more delayed events were cancelled, otherwise
    \c false.

 */
bool StateMachine::cancelDelayedEvents(Event::Type eventType)
{
    if (!m_running) {
        XLOGD_WARN("the state machine is not running");
        return false;
    }

    // take the lock before accessing the delay events map
    std::lock_guard<std::mutex> lock(m_delayedEventsLock);

    // vector of all the timers to kill
    vector<unsigned int> timersToKill(m_delayedEvents.size());

    // try and find the id in the delayed events map
    map<int64_t, DelayedEvent>::iterator it = m_delayedEvents.begin();
    while (it != m_delayedEvents.end()) {

        if (it->second.eventType == eventType) {
            timersToKill.push_back(it->second.timerId);
            it = m_delayedEvents.erase(it);
        } else {
            ++it;
        }
    }

    // clean-up all the timers
    for (unsigned int timerId : timersToKill) {
        if (timerId > 0) {
            g_source_remove(timerId);
            timerId = 0;
        }
    }

    return !timersToKill.empty();
}

// -----------------------------------------------------------------------------
/*!
    Returns the current (non super) state the state machine is in.

    If the state machine is not currently running then \c -1 is returned.
 */
int StateMachine::state() const
{
    if (!m_running) {
        return -1;
    } else {
        return m_currentState;
    }
}

// -----------------------------------------------------------------------------
/*!
    Checks if the state machine is current in the \a state given.  The \a state
    may may refer to a super state.


 */
bool StateMachine::inState(const int state) const
{
    if (!m_running) {
        XLOGD_WARN("the state machine is not running");
        return false;
    }

    // we first check the current state and then walk back up the parent
    // states to check for a match
    int state_ = m_currentState;
    do {

        // check for a match to this state
        if (state_ == state)
            return true;

        // find the current state and sanity check it is in the map
        map<int, State>::const_iterator it = m_states.find(state_);
        if (it == m_states.end()) {
            XLOGD_ERROR("invalid state %d (this shouldn't happen)", state_);
            return false;
        }

        // if this state had a parent state then try that on the next loop
        state_ = it->second.parentState;

    } while (state_ != -1);

    return false;
}

// -----------------------------------------------------------------------------
/*!
    Checks if the state machine is in any one of the supplied set of \a states.
    The states in the set may be super states


 */
bool StateMachine::inState(const unordered_set<int> &states) const
{
    if (!m_running) {
        XLOGD_WARN("the state machine is not running");
        return false;
    }

    // we first check the current state and then walk back up the parent
    // states to check for a match
    int state_ = m_currentState;
    do {
        // check for a match to this state
        unordered_set<int>::const_iterator p_it = states.find(state_);
        if ( p_it != states.end() ) {
            return true;
        }

        // find the current state and sanity check it is in the map
        map<int, State>::const_iterator it = m_states.find(state_);
        if (it == m_states.end()) {
            XLOGD_ERROR("invalid state %d (this shouldn't happen)", state_);
            return false;
        }

        // if this state had a parent state then try that on the next loop
        state_ = it->second.parentState;

    } while (state_ != -1);

    return false;
}

// -----------------------------------------------------------------------------
/*!
    Returns the name of the \a state.  If no \a state is supplied then returns
    the name of the current state.

 */
std::string StateMachine::stateName(int state) const
{
    if (state > 0) {
        auto stateObj = m_states.find(state);
        return (stateObj == m_states.end()) ? string() : stateObj->second.name;

    } else if (m_running) {
        auto stateObj = m_states.find(m_currentState);
        return (stateObj == m_states.end()) ? string() : stateObj->second.name;

    } else {
        return string();
    }
}

bool StateMachine::isRunning() const
{
    return m_running;
}

bool StateMachine::start()
{
    if (m_running) {
        XLOGD_WARN("state machine is already running");
        return false;
    }

    if (m_initialState == -1) {
        XLOGD_WARN("no initial state set, not starting state machine");
        return false;
    }

    m_stopPending = false;
    m_currentState = m_initialState;
    m_running = true;

    logTransition(-1, m_currentState);

    // Start the state machine in the current thread, no need to send to main loop context.
    // Its only after the state machine has started do we need to synchronize state transitions
    // with the main event loop since those will be triggered by async replies from other processes.
    m_entered.invoke(m_currentState);

    return true;
}

void StateMachine::stop()
{
    if (!m_running) {
        XLOGD_WARN("state machine not running");
        return;
    }

    // if being called from within a callback function then just mark the
    // state-machine as pending stop ... this will clean up everything once
    // all the events queued are processed
    if (m_withinStateMover) {
        m_stopPending = true;

    } else {

        m_currentState = -1;
        m_running = false;

        cleanUpEvents();
    }
}

