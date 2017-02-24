#include "jni.h"
#include "librtmp/rtmp.h"

#include "pthread.h"
#include "strings.h"
#include "include/x264/x264.h"
#include "include/faac/faac.h"
#include "android/log.h"

extern "C"{
#include "queue.h"
#define _RTMP_Free(_rtmp)  if(_rtmp) {RTMP_Free(_rtmp); _rtmp = NULL;}
#define _RTMP_Close(_rtmp)  if(_rtmp && RTMP_IsConnected(_rtmp)) RTMP_Close(_rtmp);

#define LOGD(...) __android_log_print(3,"NDK",__VA_ARGS__)

class RtmpPush{
//资源
private:
    int media_size;
    //x264模块
    x264_t *videoEncHandle;
    x264_picture_t* pic_in;
    x264_picture_t* pic_out;

    //rtmp模块
    RTMP* rtmpClient;

    //rtmp时间
    long start_time;
    //推流是否开启
    int publishing;

    //faac
    faacEncHandle audioEncHandle;
    unsigned long nInputSamples;
    unsigned long nMaxOutputBytes;

    //线程
    pthread_t publisher_tid;
    //线程阻塞
    pthread_mutex_t mutex ;
    pthread_cond_t cond ;

//构造函数
public:
    RtmpPush():media_size(0),videoEncHandle(NULL),pic_in(NULL),pic_out(NULL),rtmpClient(NULL),start_time(0),publishing(0),
               publisher_tid(NULL),audioEncHandle(NULL){};
    int m_width,m_height,m_bitrate;


private:
    int init_rtmp(char* out_url);

    static void *push_thread(void *args);

    //视频编码
    void push_video(char* data,int index);
    //h264头编码
    void add_264_header(char* pps, char* sps, int pps_len, int sps_len);
    //h264体编码
    void add_264_body(char* buf, int len);
    //添加rtmp消息体
    void add_rtmp_packet(RTMPPacket* packet);
    //初始化音频头
    int init_audio_header();
public :
    //初始化视频 rtmp x264
    int init_vedio(const char* out_url,int width,int height,int bitrate);
    //初始化音频 sampleRate采样率 channel 声道数
    int init_audio(int sampleRate, int channel);
    //后置摄像头 数据采集
    void video_collect_back(char* data);
    //前置摄像头 数据采集
    void video_collect_above(char* data);
    //原始头像
    void video_collect_left(char* data);
    //音频流
    void audio_cllect(char* data);

};

}