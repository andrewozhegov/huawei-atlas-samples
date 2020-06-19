
#ifndef ATLASFACEDEMO_STREAMPULLER_H
#define ATLASFACEDEMO_STREAMPULLER_H

#include "common_data_type.h"
#include "hiaiengine/engine.h"
#include <atomic>
#include <mutex>
#include <thread>

extern "C" {
#include "libavformat/avformat.h"
}

#define RP_INPUT_SIZE 1
#define RP_OUTPUT_SIZE 1

class StreamPuller : public hiai::Engine {
public:
    HIAI_StatusT Init(const hiai::AIConfig& config, const std::vector<hiai::AIModelDescription>& model_desc);

    HIAI_DEFINE_PROCESS(RP_INPUT_SIZE, RP_OUTPUT_SIZE)

    ~StreamPuller();

private:
    // todo
    // ?
    void getStreamInfo();
    void pullStreamDataLoop();
    void stopStream();
    HIAI_StatusT startStream(const string& streamName);

    // class member
    std::shared_ptr<AVFormatContext> pFormatCtx;
    // stream info
    uint64_t blockId = 0;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t channelId = 0;
    uint32_t format = H264;
    int videoIndex;
    std::atomic<int> stop = { 0 };
    std::thread sendDataRunner;
    EngineTransNewT dataBuffer;
    uint64_t curBlockId = 0;
    std::string streamName;
};

#endif
