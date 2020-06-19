
#include "StreamPuller.h"
#include "error_code.h"
#include "hiaiengine/ai_memory.h"
#include "utils_common.h"
#include <chrono>

HIAI_REGISTER_SERIALIZE_FUNC("EngineTransNewT", EngineTransNewT, GetTransSearPtr, GetTransDearPtr);
HIAI_REGISTER_SERIALIZE_FUNC("StreamRawData", StreamRawData, GetStreamRawDataSearPtr, GetStreamRawDataDearPtr);


std::shared_ptr<AVFormatContext> createFormatContext(const std::string& streamName)
{
    AVFormatContext* formatContext = nullptr;
    AVDictionary* options = nullptr;
    av_dict_set(&options, "rtsp_transport", "tcp", 0);
    av_dict_set(&options, "stimeout", "3000000", 0);
    int ret = avformat_open_input(&formatContext, streamName.c_str(), nullptr, &options);
    if (nullptr != options) {
        av_dict_free(&options);
    }
    if (0 != ret) {
        printf("Couldn't open input stream %s, ret=%d\n", streamName.c_str(), ret);
        return nullptr;
    }
    ret = avformat_find_stream_info(formatContext, nullptr);
    if (0 != ret) {
        printf("Couldn't find stream information\n");
        return nullptr;
    }
    return std::shared_ptr<AVFormatContext>(formatContext,
        [](AVFormatContext* p) { if (p) avformat_close_input(&p); });
}

StreamPuller::~StreamPuller()
{
    stopStream();
}

void StreamPuller::getStreamInfo()
{
    if (nullptr != pFormatCtx) {
        videoIndex = -1;
        for (int i = 0; i < pFormatCtx->nb_streams; i++) {
            AVStream* inStream = pFormatCtx->streams[i];
            if (inStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoIndex = i;
                mHeight = inStream->codecpar->height;
                mWidth = inStream->codecpar->width;
                AVCodecID codecId = inStream->codecpar->codec_id;
                if (codecId == AV_CODEC_ID_H264) {
                    format = H264;
                } else if (codecId == AV_CODEC_ID_H265) {
                    format = H265;
                } else {
                    printf("\033[0;31mError unsupported format %d\033[0m\n", codecId);
                    return;
                }
            } else {
                printf("Error codec_type %d\n", inStream->codecpar->codec_type);
            }
        }
        if (videoIndex == -1) {
            printf("Didn't find a video stream\n");
        }
    }
}

void StreamPuller::pullStreamDataLoop()
{
    AVPacket pkt;
    while (1) {
        if (stop || nullptr == pFormatCtx) {
            break;
        }
        av_init_packet(&pkt);
        int ret = av_read_frame(pFormatCtx.get(), &pkt);
        if (0 != ret) {
            printf("channel %d Read frame failed, continue!\n", channelId);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        } else if (pkt.stream_index == videoIndex) {
            if (pkt.size <= 0) {
                printf("channel %d Invalid pkt.size %d\n", channelId, pkt.size);
                continue;
            }

            uint8_t* buffer;
            ret = hiai::HIAIMemory::HIAI_DMalloc(pkt.size, (void*&)buffer,
                hiai::MALLOC_DEFAULT_TIME_OUT,
                hiai::HIAI_MEMORY_ATTR_MANUAL_FREE);

            if (HIAI_OK != ret) {
                printf("channel %d HIAI_DMalloc buffer faild\n", channelId);
                av_packet_unref(&pkt);
                break;
            }

            memcpy_s(buffer, pkt.size, pkt.data, pkt.size);
            dataBuffer.transBuff = std::shared_ptr<uint8_t>(buffer, hiai::HIAIMemory::HIAI_DFree);
            dataBuffer.bufferSize = pkt.size;
            blockId++;
            std::shared_ptr<StreamRawData> output = std::make_shared<StreamRawData>();
            output->buf = dataBuffer;

            printf("DEBUG: StreamPuller send data to channeld %d\n", channelId);
            output->info.contextId = 3;
            output->info.channelId = channelId;
            if (channelId >= 15) {
                channelId = 0;
            } else channelId++;

            output->info.format = format;
            output->info.isEOS = 0;

            int ret = SendData(0, "StreamRawData", std::static_pointer_cast<void>(output));

            if (ret != HIAI_OK) {
                printf("channel %d StreamPuller send data failed %d != %u\n", channelId, ret, HIAI_OK);
                printf("HIAI_OK %d\nHIAI_GRAPH_ENGINE_NOT_EXIST %d\n HIAI_GRAPH_NOT_EXIST %d\nHIAI_GRAPH_NO_USEFUL_MEMORY %d\n HIAI_GRAPH_SENDMSG_FAILED %d\n HIAI_GRAPH_INVALID_VALUE %d", HIAI_OK, HIAI_GRAPH_ENGINE_NOT_EXIST, HIAI_GRAPH_NOT_EXIST, HIAI_GRAPH_NO_USEFUL_MEMORY, HIAI_GRAPH_SENDMSG_FAILED, HIAI_GRAPH_INVALID_VALUE);
                if (1) {
                    printf("channel %d sleep for 1 seconds\n", channelId);
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            } else {
                printf("channel %d pkt.size %d\n", channelId, pkt.size);
            }

            av_packet_unref(&pkt);

        }
    }
    std::shared_ptr<StreamRawData> output = std::make_shared<StreamRawData>();
    output->info.channelId = channelId;
    output->info.format = format;
    output->info.isEOS = 1;
    HIAI_StatusT ret = SendData(0, "StreamRawData", std::static_pointer_cast<void>(output));
    printf("channel %d pullStreamDataLoop end of stream\n", channelId);
    av_init_packet(&pkt);
    stop = 1;
}

void StreamPuller::stopStream()
{
    stop = 1;
    if (sendDataRunner.joinable()) {
        sendDataRunner.join();
    }
}

HIAI_StatusT StreamPuller::startStream(const string& streamName)
{
    stopStream();
    stop = 0;
    pFormatCtx = createFormatContext(streamName);
    if (nullptr == pFormatCtx) {
        return HIAI_ERROR;
    }
    // for debug dump
    av_dump_format(pFormatCtx.get(), 0, streamName.c_str(), 0);
    // get stream infomation
    getStreamInfo();
    // set up loop threads
    sendDataRunner = std::thread(&StreamPuller::pullStreamDataLoop, this);
    return HIAI_OK;
}

HIAI_StatusT StreamPuller::Init(const hiai::AIConfig& config, const std::vector<hiai::AIModelDescription>& model_desc)
{
    //printf("StreamPuller Init start\n");
    /*auto aimap = kvmap(config);
    if (aimap.count("format")) {
        if (aimap["format"] == "h264") {
            format = H264;
        } else {
            format = H265;
        }
    }
    CHECK_RETURN_IF(aimap.count("channel_id") <= 0);*/

    format = H264;
    channelId = 0;//std::stoi(aimap["channel_id"]);
    streamName = "rtsp://192.168.15.119:7654/test1-rtsp";

    avformat_network_init();

    /*if (aimap.count("stream_name")) {
        streamName = aimap["stream_name"];
    }*/

    return HIAI_OK;
}

HIAI_IMPL_ENGINE_PROCESS("StreamPuller", StreamPuller, RP_INPUT_SIZE)
{
    if (nullptr != arg0) {
        std::shared_ptr<string> inputArg = std::static_pointer_cast<string>(arg0);
        if (!inputArg->empty()) {
            //streamName = *inputArg;
        }
        startStream(streamName);
    }
    return HIAI_OK;
}
