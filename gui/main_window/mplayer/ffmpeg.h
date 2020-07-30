#ifndef _FFMPEG_H
#define _FFMPEG_H

#include <stdint.h>

namespace ffmpeg
{
    extern "C"
    {
        #include "libavcodec/avcodec.h"
        #include "libavformat/avformat.h"
        #include "libavutil/mathematics.h"
        #include "libavutil/time.h"
        #include "libavutil/rational.h"
        #include "libavutil/opt.h"
        #include "libavutil/avstring.h"
        #include "libavutil/imgutils.h"
        #include "libavutil/samplefmt.h"
        #include "libavutil/error.h"
        #include "libavutil/channel_layout.h"
        #include "libavutil/common.h"
        #include "libavutil/frame.h"
        #include "libswscale/swscale.h"
        #include "libswresample/swresample.h"
    }
}

#endif // _FFMPEG_H
