#pragma once
#include <voip/voip3.h>
#include <memory>
#include <map>

namespace webrtc {
  class VideoFrame;
}

namespace rtc {
  template <typename VideoFrameT> class VideoSinkInterface;
}

namespace voip {

class ICallSessionEventSink;    // see CallSessionEventSink.h

const std::string    kLocalCameraStreamId = { "@camera_stream_id" };   // "preview"


class IVoipVideoWindowRenderer
{
public:
    virtual ~IVoipVideoWindowRenderer() = default;

    virtual ICallSessionEventSink   *GetEventSink() = 0;    // lifespan of a sink is assumed to be equal to the object itself

    virtual rtc::VideoSinkInterface<webrtc::VideoFrame> *GetVideoSink(const std::string& streamId) = 0;

protected:
    IVoipVideoWindowRenderer() = default;

public:
    void operator=(const IVoipVideoWindowRenderer&) = delete;
    IVoipVideoWindowRenderer(const IVoipVideoWindowRenderer&) = delete;
};

}
