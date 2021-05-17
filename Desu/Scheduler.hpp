#pragma once
#include <functional>
#include <algorithm>
#include <vector>
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

    std::vector<Event> initialEventList; // to store reschedulable events (HBlank start/end, etc.)
    std::list<Event> eventList;
    uint32_t cyclesPassedSinceLastFrame = 0;

    void addEventToBack(std::function<uint32_t()>,uint32_t, bool);
    // for interrupts and events that don't have a schedule
    void scheduleInterruptCheck(std::function<uint32_t()>,uint32_t);

    // takes the return value of an event fuction and reschedules it
    // only use for events that should be rescheduled!
    void rescheduleFront(uint32_t);
    void step();
    void getInitialEventList();
    void resetEventList();
};

inline void Scheduler::addEventToBack(std::function<uint32_t()> func, uint32_t cycleTimeStamp, bool reschedule) {
    eventList.emplace_back(func,cycleTimeStamp,reschedule);
}

inline void Scheduler::scheduleInterruptCheck(std::function<uint32_t()> func, uint32_t cycleTimeStamp) {
    eventList.emplace_front(func,cycleTimeStamp,0);
    eventList.splice(std::next(eventList.begin(),2),eventList,eventList.begin());
}
// todo: ask about and optimize this bihh, rescheduling seems slow for some reason
inline void Scheduler::rescheduleFront(uint32_t cycleTimeStamp) {
    uint32_t processRescheduledTime = eventList.front().timestamp + cycleTimeStamp;
    
    if(processRescheduledTime <= 280896) {
        for(auto it = std::next(eventList.begin(),1); it != eventList.end(); ++it) {
            if(processRescheduledTime <= it->timestamp) {
                eventList.front().timestamp = processRescheduledTime;
                eventList.splice(it,eventList,eventList.begin());
                return;
            }
        }
    } else
        eventList.pop_front(); // else don't reschedule
}
inline void Scheduler::step() {
    if(cyclesPassedSinceLastFrame >= 280896)
        printf("fugglishousce");
    if(cyclesPassedSinceLastFrame >= eventList.front().timestamp) {
        if(eventList.front().shouldBeRescheduled)
            rescheduleFront(eventList.front().process());
        else {
            eventList.front().process();
            eventList.pop_front();
        }
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