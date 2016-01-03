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
        Timer(bool start_now = false) :
            m_dt(0.0f),
            m_stopped(true)
        {
            if (start_now)
            {
                start();
            }
        }
    
        /// Start the timer.
        inline void start()
        {
            if (m_stopped)
            {
                m_start_time = Clock::now();
                m_stopped = false;
            }
        }
    
        /// Step the timer. The time in seconds between the last call to start() and this
        /// call to stop() is returned.
        inline float stop()
        {
            m_dt = getElapsedTime();
            m_stopped = true;
            return m_dt;
        }

        /// Restart the timer from the current time point.
        inline void restart()
        {
            stop();
            start();
        }
        
        /// Get the time from the last call to start() or dt() and call start() again. This is
        /// just a convenience function.
        inline float dt()
        {
            restart();
            return m_dt;
        }

        /// Get the time that has elapsed since the last call to start(). After this call finished,
        /// the timer will not be stopped.
        inline float getElapsedTime() const
        {
            if (!m_stopped)
            {
                std::chrono::duration<float> duration = Clock::now() - m_start_time;
                return duration.count();
            }

            return -1.0f;
        }
        
    protected:
        
        Clock::time_point m_start_time; // Time recorded with a call to start().
        float m_dt;                     // Time between a call to start() and stop() in seconds.
        bool  m_stopped;                // If true, the timer is currently stopped.
    
};
    
}


