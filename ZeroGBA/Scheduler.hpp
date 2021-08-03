#pragma once
#include <functional>
#include <algorithm>
#include <vector>
#include <cstdint>
#include <list>

struct Scheduler {

    enum EventTypes {
        SoundChannel1,SoundChannel2,SoundChannel3,SoundChannel4,
        Timer0,Timer1,Timer2,Timer3,
        GenericRescheduable,Interrupt,HaltCheck
    };

    struct Event {

        std::function<uint64_t()> process; // the return value for processes is the timestamp cycle delta (how many cycles it takes for the next occurrence, if there is one)
        uint8_t eventType;
        uint64_t timestamp;
        bool shouldBeRescheduled;
        
        Event(std::function<uint64_t()> newProcess,uint8_t newEventType,uint64_t processCycles,bool reschedule) {
            process = newProcess;
            eventType = newEventType;
            timestamp = processCycles;
            shouldBeRescheduled = reschedule;
        }
    };

    std::list<Event> eventList;
    uint64_t cyclesPassed = 0;

    void scheduleEvent(std::function<uint64_t()>,uint8_t,uint64_t,bool); // inserts before event with higher timestamp
    void addEventToFront(std::function<uint64_t()>,uint8_t,uint64_t, bool);
    void addEventToBack(std::function<uint64_t()>,uint8_t,uint64_t, bool);

    // takes the return value of an event fuction and reschedules it
    // only use for events that should be rescheduled!
    void rescheduleEvent(const std::list<Event>::iterator&,uint64_t); // executes then pops or reschedules the front event
    void step();
};

inline void Scheduler::scheduleEvent(std::function<uint64_t()> func, uint8_t eventType, uint64_t cycleTimeStamp, bool reschedule) {
    auto it = eventList.begin();
    for(;it != eventList.end(); ++it) {
        if(cycleTimeStamp <= it->timestamp) {
            eventList.emplace(it, func, eventType, cycleTimeStamp, reschedule); // move currEvent to "it"
            return;
        }
    }

    // if last event
    eventList.emplace(it, func, eventType, cycleTimeStamp, reschedule);
}

inline void Scheduler::addEventToFront(std::function<uint64_t()> func, uint8_t eventType, uint64_t cycleTimeStamp, bool reschedule) {
    eventList.emplace_front(func,eventType,cycleTimeStamp,reschedule);
}

inline void Scheduler::addEventToBack(std::function<uint64_t()> func, uint8_t eventType, uint64_t cycleTimeStamp, bool reschedule) {
    eventList.emplace_back(func,eventType,cycleTimeStamp,reschedule);
}

inline void Scheduler::rescheduleEvent(const std::list<Event>::iterator& currEvent, uint64_t cycleTimeStamp) {
    uint64_t processRescheduledTime = currEvent->timestamp + cycleTimeStamp;
    
    auto it = currEvent;
    for(;it != eventList.end(); ++it) {
        if(processRescheduledTime <= it->timestamp) {
            // If we don't need to move the event, just increment the timestamp
            auto itCopy = it;
            if(--itCopy == currEvent)
                currEvent->timestamp = processRescheduledTime;
            else {
                currEvent->timestamp = processRescheduledTime;
                eventList.splice(it, eventList, currEvent); // move currEvent to "it"
            }
            return;
        }
    }

    // for when the event is rescheduled to be last
    currEvent->timestamp = processRescheduledTime;
    eventList.splice(it, eventList, currEvent);
}
inline void Scheduler::step() {
    const auto front = eventList.begin();
    if(cyclesPassed >= front->timestamp) {
        if(front->shouldBeRescheduled)
            rescheduleEvent(front,front->process());
        else {
            uint8_t eventType = front->eventType;
            front->process();
            if(eventType != HaltCheck)
                eventList.erase(front);
        }

        // check if there's another event we can step through
        if(cyclesPassed >= eventList.front().timestamp)
            step();
    }
}