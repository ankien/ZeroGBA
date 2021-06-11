#pragma once
#include <functional>
#include <algorithm>
#include <vector>
#include <cstdint>
#include <list>

struct Scheduler {

    enum EventTypes {Timer0,Timer1,Timer2,Timer3,GenericRescheduable,Interrupt,HaltCheck};

    struct Event {

        std::function<uint32_t()> process; // the return value for processes is the timestamp cycle delta (how many cycles it takes for the next occurrence, if there is one)
        uint8_t eventType;
        uint32_t timestamp;
        bool shouldBeRescheduled;
        
        Event(std::function<uint32_t()> newProcess,uint8_t newEventType,uint32_t processCycles,bool reschedule) {
            process = newProcess;
            eventType = newEventType;
            timestamp = processCycles;
            shouldBeRescheduled = reschedule;
        }
    };

    std::list<Event> nextFrameQueue;
    std::list<Event> eventList;
    uint32_t cyclesPassedSinceLastFrame = 0;

    void scheduleEvent(std::function<uint32_t()>,uint8_t,uint32_t,bool); // inserts before event with higher timestamp
    void addEventToFront(std::function<uint32_t()>,uint8_t,uint32_t, bool);
    void addEventToBack(std::function<uint32_t()>,uint8_t,uint32_t, bool);

    // takes the return value of an event fuction and reschedules it
    // only use for events that should be rescheduled!
    void rescheduleEvent(const std::list<Event>::iterator&,uint32_t); // executes then pops or reschedules the front event
    void step();
    void resetEventList();
};

inline void Scheduler::scheduleEvent(std::function<uint32_t()> func, uint8_t eventType, uint32_t cycleTimeStamp, bool reschedule) {
    for(auto it = eventList.begin(); it != eventList.end(); ++it) {
            if(cycleTimeStamp <= it->timestamp) {
                eventList.emplace(it,func,eventType,cycleTimeStamp%280896,reschedule); // move currEvent to "it"
                return;
            }
    }
}

inline void Scheduler::addEventToFront(std::function<uint32_t()> func, uint8_t eventType, uint32_t cycleTimeStamp, bool reschedule) {
    eventList.emplace_front(func,eventType,cycleTimeStamp,reschedule);
}

inline void Scheduler::addEventToBack(std::function<uint32_t()> func, uint8_t eventType, uint32_t cycleTimeStamp, bool reschedule) {
    eventList.emplace_back(func,eventType,cycleTimeStamp,reschedule);
}

// todo: optimize this bitch
inline void Scheduler::rescheduleEvent(const std::list<Event>::iterator& currEvent, uint32_t cycleTimeStamp) {
    uint32_t processRescheduledTime = currEvent->timestamp + cycleTimeStamp;
    
    if(processRescheduledTime <= 280896) {
        for(auto it = currEvent; it != eventList.end(); ++it) {
            if(processRescheduledTime <= it->timestamp) {
                // If we don't need to move the event, just increment the timestamp
                auto itCopy = it;
                if(--itCopy == currEvent)
                    currEvent->timestamp = processRescheduledTime;
                else {
                    currEvent->timestamp = processRescheduledTime;
                    eventList.splice(it,eventList,currEvent); // move currEvent to "it"
                }
                return;
            }
        }
    } else {
        currEvent->timestamp = processRescheduledTime - 280896;
        nextFrameQueue.splice(nextFrameQueue.end(),eventList,currEvent);
    }
}
inline void Scheduler::step() {
    const auto front = eventList.begin();
    if(cyclesPassedSinceLastFrame >= front->timestamp) {
        if(front->shouldBeRescheduled)
            rescheduleEvent(front,front->process());
        else {
            uint8_t eventType = front->eventType;
            front->process();
            if(eventType != HaltCheck)
                eventList.erase(front);
        }

        // check if there's another event we can step through
        if(cyclesPassedSinceLastFrame >= eventList.front().timestamp)
            step();
    }
}
inline void Scheduler::resetEventList() {
    nextFrameQueue.sort([](const Event& a, const Event& b) { return a.timestamp < b.timestamp; });
    eventList.splice(eventList.begin(),nextFrameQueue);
}