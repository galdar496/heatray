//
//  Timer.h
//  Heatray
//
//  Created by Cody White on 5/9/14.
//
//

#pragma once

#include <chrono>

namespace util
{
    
class Timer
{
    public:
        
        typedef std::chrono::high_resolution_clock Clock;
    
        /// Default constructor.
        Timer() :
            m_dt(0.0f)
        {
        }
    
        /// Start the timer.
        inline void start()
        {
            m_start_time = Clock::now();
        }
    
        /// Step the timer. The time in seconds between the last call to start() and this
        /// call to stop() is returned.
        inline float stop()
        {
            std::chrono::duration<float> duration = Clock::now() - m_start_time;
            m_dt = duration.count();
            return m_dt;
        }
        
        /// Get the time from the last call to start() or dt() and call start() again. This is
        /// justa  convenience function.
        inline float dt()
        {
            stop();
            start();
            return m_dt;
        }
        
    protected:
        
        Clock::time_point m_start_time; // Time recorded with a call to start().
        float m_dt;                     // Time between a call to start() and stop() in seconds.
    
};
    
}


