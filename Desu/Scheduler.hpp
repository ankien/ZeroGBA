#pragma once
#include <functional>
#include <algorithm>
#include <cstdint>
#include <list>

struct Scheduler {

    struct Event {

        std::function<uint32_t()> process;
        uint32_t timestamp;
        bool shouldBeRescheduled;
        
        Event(std::function<uint32_t()> newProcess,uint32_t processCycles,bool reschedule) {
            process = newProcess;
            timestamp = processCycles;
            shouldBeRescheduled = reschedule;
        }
    };

    std::vector<Event> initialEventList;
    std::list<Event> eventList;
    uint32_t cyclesPassedSinceLastFrame = 0;

    void addEventToBack(std::function<uint32_t()>,uint32_t, bool);
    // void addEventToFront, todo, for interrupts and stuff

    // takes the return value of an event fuction and reschedules it
    // only use for events that should be rescheduled!
    void rescheduleFront(std::function<uint32_t()>,uint32_t);
    void step();
    void getInitialEventList();
    void resetEventList();
};

inline void Scheduler::addEventToBack(std::function<uint32_t()> func, uint32_t cycleTimeStamp, bool reschedule) {
    eventList.emplace_back(func,cycleTimeStamp,reschedule);
}

inline void Scheduler::rescheduleFront(std::function<uint32_t()> func,uint32_t cycleTimeStamp) {
    uint32_t processRescheduledTime = eventList.front().timestamp + cycleTimeStamp;

    if(processRescheduledTime > 280896)
        int fug = 2 + 2;
    
    if(processRescheduledTime <= 280896) {
        for(auto it = std::next(eventList.begin(),1); it != eventList.end(); ++it) {
            if(processRescheduledTime <= it->timestamp) {
                eventList.front().timestamp = processRescheduledTime;
                eventList.splice(it,eventList,eventList.begin());
                return;
            }
        }
    } else
        eventList.pop_front();
    // else pop or reschedule later? currently there aren't any rescheduable events that don't evenly align w/ 280896 cycles
}
inline void Scheduler::step() {
    if(cyclesPassedSinceLastFrame >= eventList.front().timestamp) {
        if(eventList.front().shouldBeRescheduled)
            rescheduleFront(eventList.front().process,eventList.front().process());
        //else add to front
    }
}
inline void Scheduler::getInitialEventList() {
    initialEventList.reserve(eventList.size());
    initialEventList.insert(initialEventList.begin(),eventList.begin(),eventList.end());
}
inline void Scheduler::resetEventList() {
    for(Event e : initialEventList)
        eventList.push_back(e);
}