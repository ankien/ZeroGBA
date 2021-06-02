#pragma once
#include <functional>
#include <algorithm>
#include <vector>
#include <cstdint>
#include <list>

struct Scheduler {

    enum immediateEventTypes {Interrupt,HaltCheck};

    struct Event {

        std::function<uint32_t()> process; // the return valur for processes are the type of event (non-rescheduable) or the timestamp cycle delta (rescheduable)
        uint32_t timestamp;
        bool shouldBeRescheduled;
        
        Event(std::function<uint32_t()> newProcess,uint32_t processCycles,bool reschedule) {
            process = newProcess;
            timestamp = processCycles;
            shouldBeRescheduled = reschedule;
        }
    };

    std::vector<Event> initialEventList; // to store reschedulable events (HBlank start/end, etc.)
    std::list<Event> eventList;
    uint32_t cyclesPassedSinceLastFrame = 0;

    void addEventToFront(std::function<uint32_t()>,uint32_t, bool);
    void addEventToBack(std::function<uint32_t()>,uint32_t, bool);
    // for interrupts and events that don't have a schedule, places event at front
    void scheduleInterruptCheck(std::function<uint32_t()>);

    // takes the return value of an event fuction and reschedules it
    // only use for events that should be rescheduled!
    void rescheduleEvent(const std::list<Event>::iterator&,uint32_t); // executes then pops or reschedules the front event
    void step();
    void resetEventList();
};

inline void Scheduler::addEventToFront(std::function<uint32_t()> func, uint32_t cycleTimeStamp, bool reschedule) {
    eventList.emplace_front(func,cycleTimeStamp,reschedule);
}

inline void Scheduler::addEventToBack(std::function<uint32_t()> func, uint32_t cycleTimeStamp, bool reschedule) {
    eventList.emplace_back(func,cycleTimeStamp,reschedule);
}

inline void Scheduler::scheduleInterruptCheck(std::function<uint32_t()> func) {
    eventList.emplace_front(func,0,false);
}
// todo: ask about and optimize this bihh, rescheduling seems slow for some reason
inline void Scheduler::rescheduleEvent(const std::list<Event>::iterator& currEvent, uint32_t cycleTimeStamp) {
    uint32_t processRescheduledTime = currEvent->timestamp + cycleTimeStamp;
    
    if(processRescheduledTime <= 280896) {
        for(auto it = currEvent; it != eventList.end(); ++it) {
            if(processRescheduledTime <= it->timestamp) {
                currEvent->timestamp = processRescheduledTime;
                eventList.splice(it,eventList,currEvent); // move currEvent to "it"
                return;
            }
        }
    } else
        eventList.erase(currEvent); // else don't reschedule
}
inline void Scheduler::step() {
    const auto front = eventList.begin();
    if(cyclesPassedSinceLastFrame >= front->timestamp) {
        if(front->shouldBeRescheduled)
            rescheduleEvent(front,front->process());
        else {
            if(front->process() == Interrupt)
                eventList.erase(front);
        }

        // check if there's another event we can step through
        if(cyclesPassedSinceLastFrame >= eventList.front().timestamp)
            step();
    }
}
inline void Scheduler::resetEventList() {
    for(Event const& e : initialEventList)
        eventList.emplace_back(e);
}