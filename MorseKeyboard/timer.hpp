#ifndef TIMER_HPP
#define TIMER_HPP

#include <Arduino.h>

class Timer {
    uint32_t _startTime;
    bool _isRunning;

public:
    Timer() : _startTime(0), _isRunning(false) {}

    void start() {
        _startTime = millis();
        _isRunning = true;
    }

    bool isRunning() const {
        return _isRunning;
    }

    uint32_t stop() {
        _isRunning = false;
        return elapsed();
    }

    uint32_t elapsed() const {
        if (_isRunning)
            return millis() - _startTime;
        
        return 0;
    }
};

#endif