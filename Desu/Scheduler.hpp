#pragma once
#include <boost/pool/pool_alloc.hpp>
#include <cstdint>
#include <list>

/* The idea is to place cycle-timed events into a linked list. (addEvent)
   In a running loop, after executing a certain amount of instructions:
   check if cyclesPassed >= the first in list, execute the event, remove the event, then place it at the end. (step)
*/
struct Scheduler {
    
    struct Event {
        
        void (*process)();
        uint32_t timestamp;
        
        Event(void(*newProcess)(),uint32_t processCycles) {
            process = newProcess;
            timestamp = processCycles;
        }
    };
    
    std::list<Event,boost::fast_pool_allocator<Event>> list;
    uint32_t cyclesPassedSinceLastFrame;

    void addEvent(void(*)(),uint32_t);
    void reschedule(uint32_t);
    void step();
};

inline void Scheduler::addEvent(void (*func)(), uint32_t cycleTimeStamp) {
    list.emplace_back(func,cycleTimeStamp);
}

inline void Scheduler::reschedule(uint32_t cycleTimeStamp) {
    
}

inline void Scheduler::step() {
    
}