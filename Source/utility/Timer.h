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
   
///
/// Timer object which can be used to determine the amount of time that has
/// passed between two time points.
///
class Timer
{
    public:
        
        typedef std::chrono::high_resolution_clock Clock;
    
        Timer(bool startNow = false) :
            m_dt(0.0f),
            m_stopped(true)
        {
            if (startNow)
            {
                Start();
            }
        }
    
        ///
        /// Start the timer. The timer must be stopped in order to be started.
        ///
        inline void Start()
        {
            if (m_stopped)
            {
                m_startTime = Clock::now();
                m_stopped = false;
            }
        }
    
        ///
        /// Stop the timer.
        ///
        /// @return The time in seconds between the last call to start() and this
        /// call to stop() is returned.
        ///
        inline float Stop()
        {
            m_dt = GetElapsedTime();
            m_stopped = true;
            return m_dt;
        }

        ///
        /// Restart the timer from the current time point.
        ///
        inline void Restart()
        {
            Stop();
            Start();
        }
    
        ///
        /// Get the time from the last call to start() or dt() and call start() again. This is
        /// just a convenience function.
        ///
        /// @param Time since the timer has been started (or the last call to dt()).
        ///
        inline float Dt()
        {
            Restart();
            return m_dt;
        }

        ///
        /// Get the time that has elapsed since the last call to start(). After this call finished,
        /// the timer will NOT be stopped.
        ///
        /// @return Time since the lst call to start().
        ///
        inline float GetElapsedTime() const
        {
            if (!m_stopped)
            {
                std::chrono::duration<float> duration = Clock::now() - m_startTime;
                return duration.count();
            }

            return -1.0f;
        }
        
    protected:
        
        Clock::time_point m_startTime; ///< Time recorded with a call to start().
        float m_dt;                     ///< Time between a call to start() and stop() in seconds.
        bool  m_stopped;                ///< If true, the timer is currently stopped.
    
};
    
}


