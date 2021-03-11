#pragma once
#include <list>
#include <cstdint>
#include <filesystem>
#include <cstdio>
#include <SDL.h>
#include "core/ARM7TDMI.hpp"
#include "hardware/GBAMemory.hpp"
#include "hardware/LCD.hpp"
#include "hardware/Keypad.hpp"

struct GBA {

    /// Hardware ///
    ARM7TDMI arm7tdmi{};
    GBAMemory* systemMemory;
    LCD lcd{};
    Keypad keypad{};

    GBA();

    // todo: implement a cached interpreter (fetch a batch of instructions before interpretation)
    void interpretARM();
    void interpretTHUMB();

    // Control helpers
    struct {
        uint8_t jit : 1; // not implemented lol
    } runtimeOptions{};
    bool parseArguments(uint64_t argc, char* argv[]);
    void run(char*);

    // Scheduler stuff
    struct Event {

        uint32_t (GBA::*process)();
        uint32_t timestamp;
        
        Event(uint32_t(GBA::*newProcess)(),uint32_t processCycles) {
            process = newProcess;
            timestamp = processCycles;
        }
    };

    std::vector<Event> initialEventList;
    std::list<Event> eventList;
    uint32_t cyclesPassedSinceLastFrame = 0;

    void addEvent(uint32_t(GBA::*)(),uint32_t);
    void rescheduleFront(uint32_t(GBA::*)(),uint32_t); // takes the return value of an event fuction and reschedules it
    void step();
    void getInitialEventList();
    void resetEventList();

    uint32_t postFrame();
    uint32_t startHBlank();
    uint32_t endHBlank();
    uint32_t startVBlank();
};

inline void GBA::addEvent(uint32_t (GBA::*func)(), uint32_t cycleTimeStamp) {
    eventList.emplace_back(func,cycleTimeStamp);
}

inline void GBA::rescheduleFront(uint32_t (GBA::*func)(),uint32_t cycleTimeStamp) {
    uint32_t processRescheduledTime = eventList.front().timestamp + cycleTimeStamp;

    eventList.pop_front();
    
    if(processRescheduledTime <= 280896) {
        for(auto it = eventList.begin(); it != eventList.end(); ++it) {
            if(processRescheduledTime <= it->timestamp) {
                eventList.emplace(it,func,processRescheduledTime);
                return;
            }
        }
    }
    // else reschedule later? currently there aren't any events that don't evenly align w/ 280896 cycles
}
inline void GBA::step() {
    if(cyclesPassedSinceLastFrame >= eventList.front().timestamp) {
        rescheduleFront(eventList.front().process,(this->*eventList.front().process)());
    }
}
inline void GBA::getInitialEventList() {
    initialEventList.reserve(eventList.size());
    initialEventList.insert(initialEventList.begin(),eventList.begin(),eventList.end());
}
inline void GBA::resetEventList() {
    for(Event e : initialEventList)
        eventList.push_back(e);
}