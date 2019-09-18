// The Stack Blur Algorithm was invented by Mario Klingemann,
// mario@quasimondo.com and described here:
// http://incubator.quasimondo.com/processing/fast_blur_deluxe.php

// This is C++ RGBA (32 bit color) multi-threaded version
// by Victor Laskin (victor.laskin@gmail.com)
// More details: http://vitiy.info/stackblur-algorithm-multi-threaded-blur-for-cpp

// This code is using MVThread class from my cross-platform framework
// You can exchange it with any thread implementation you like


// -------------------------------------- stackblur ----------------------------------------->
#pragma once

namespace Utils
{
    /// Stackblur algorithm body
    void stackblurJob(unsigned char* src,   ///< input image data
        const unsigned int w,               ///< image width
        const unsigned int h,               ///< image height
        const unsigned int radius,          ///< blur intensity (should be in 2..254 range)
        const int cores,                    ///< total number of working threads
        const int core,                     ///< current thread number
        const int step,                     ///< step of processing (1,2)
        unsigned char* stack                ///< stack buffer
    );

    /// Stackblur algorithm by Mario Klingemann
    /// Details here:
    /// http://www.quasimondo.com/StackBlurForCanvas/StackBlurDemo.html
    /// C++ implemenation base from:
    /// https://gist.github.com/benjamin9999/3809142
    /// http://www.antigrain.com/__code/include/agg_blur.h.html
    /// Adapation for Qt ICQ by Alexander Pershin
    /// This version works only with RGBA color
    void stackblur(unsigned char* src,  ///< input image data
        const unsigned int w,           ///< image width
        const unsigned int h,           ///< image height
        const unsigned int radius,      ///< blur intensity (should be in 2..254 range)
        const int coreCount = -1        ///< core count, -1 = auto multithreading
    );

    class StackBlurTask : public QRunnable
    {
    public:
        unsigned char* src_;
        unsigned int w_;
        unsigned int h_;
        unsigned int radius_;
        int cores_;
        int core_;
        int step_;
        unsigned char* stack_;

        StackBlurTask(unsigned char* _src, unsigned int _w, unsigned int _h, unsigned int _radius, int _cores, int _core, int _step, unsigned char* _stack)
            : src_(_src)
            , w_(_w)
            , h_(_h)
            , radius_(_radius)
            , cores_(_cores)
            , core_(_core)
            , step_(_step)
            , stack_(_stack)
        {
        }

        void run() override
        {
            stackblurJob(src_, w_, h_, radius_, cores_, core_, step_, stack_);
        }
    };

    constexpr unsigned int minRadius() noexcept { return 2; }
    constexpr unsigned int maxRadius() noexcept { return 254; }
}