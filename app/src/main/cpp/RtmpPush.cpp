#include "RtmpPush.h"

extern "C" {

int RtmpPush::init_vedio( const char *out_url, int width, int height, int bitrate) {

    media_size = width * height;
    m_width = width;
    m_height = height;
    m_bitrate = bitrate;

    //x264
    x264_param_t param;
    x264_param_default_preset(&param, "ultrafast", "zerolatency");
    //
    param.i_csp = X264_CSP_I420;
    param.i_width = width;
    param.i_height = height;
    param.rc.i_bitrate = bitrate / 1000;
    param.rc.i_rc_method = X264_RC_ABR; //参数i_rc_method表示码率控制，CQP(恒定质量)，CRF(恒定码率)，ABR(平均码率)
    param.rc.i_vbv_buffer_size = bitrate / 1000; //设置了i_vbv_max_bitrate必须设置此参数，码率控制区大小,单位kbps
    param.rc.i_vbv_max_bitrate = bitrate / 1000 * 1.2; //瞬时最大码率
    param.i_keyint_max = 10 * 2;
    param.i_fps_num = 10; //* 帧率分子
    param.i_fps_den = 1; //* 帧率分母
    param.i_threads = 1;
    param.i_timebase_den = param.i_fps_num;
    param.i_timebase_num = param.i_fps_den;
    param.b_repeat_headers = 1; // 是否复制sps和pps放在每个关键帧的前面 该参数设置是让每个关键帧(I帧)都附带sps/pps。

    //
    x264_param_apply_profile(&param, "baseline");

    videoEncHandle = x264_encoder_open(&param);
    pic_in = (x264_picture_t *) malloc(sizeof(x264_picture_t));
    pic_out = (x264_picture_t *) malloc(sizeof(x264_picture_t));
    x264_picture_alloc(pic_in, X264_CSP_I420, width, height);
    x264_picture_init(pic_out);

    return init_rtmp((char *) out_url);
}

int RtmpPush::init_rtmp(char *out_url) {
    //初始化rtm
    //创建一个RTMP会话的句柄
    rtmpClient = RTMP_Alloc();
    //初始化rtmp 句柄
    RTMP_Init(rtmpClient);
    //超时时间
    rtmpClient->Link.timeout = 5;
    rtmpClient->Link.flashVer = RTMP_DefaultFlashVer;

    //设置会话的参数 输出url地址
    if (!RTMP_SetupURL(rtmpClient, (char *) out_url)) {
        RTMP_Free(rtmpClient);
        LOGD("RTMP_SetupURL fail");
        return 1;
    }

    RTMP_EnableWrite(rtmpClient);
    //建立RTMP链接中的网络连接
    if (!RTMP_Connect(rtmpClient, NULL)) {
        RTMP_Free(rtmpClient);
        LOGD("RTMP_Connect fail");
        return 1;
    }
    //建立RTMP链接中的网络流
    if (!RTMP_ConnectStream(rtmpClient, 0)) {
        RTMP_Free(rtmpClient);
        LOGD("RTMP_ConnectStream fail");
        return 1;
    }

    //创建队列
    create_queue();
    start_time = (long) RTMP_GetTime();

    //开启
    publishing = 1;
    mutex = PTHREAD_MUTEX_INITIALIZER;
    cond = PTHREAD_COND_INITIALIZER;
    pthread_create(&publisher_tid, NULL, RtmpPush::push_thread, this);
    return 2;
}

int RtmpPush::init_audio(int sampleRate, int channel) {
    if(audioEncHandle){
        LOGD("音频已打开");
        return 2;
    }
    /**
     * 初始化aac句柄，同时获取最大输入样本，及编码所需最小字节
     * sampleRate 采样率 channel声道数 nInputSamples输入样本数 nMaxOutputBytes输出所需最大空间
     */
    audioEncHandle = faacEncOpen((unsigned long) sampleRate, (unsigned int) channel, &nInputSamples, &nMaxOutputBytes);

    if(!audioEncHandle){
        LOGD("音频打开失败");
        return 1;
    }

    faacEncConfigurationPtr  faac_enc_config = faacEncGetCurrentConfiguration(audioEncHandle);
    faac_enc_config->mpegVersion = MPEG4;
    faac_enc_config->allowMidside = 1;
    //LC编码
    faac_enc_config->aacObjectType = LOW;
    //输出是否包含ADTS头
    faac_enc_config->outputFormat = 0;
    //时域噪音控制,大概就是消爆音
    faac_enc_config->useTns = 1;
    faac_enc_config->useLfe = 0;
    faac_enc_config->inputFormat = FAAC_INPUT_16BIT;
    faac_enc_config->quantqual = 100;
    //频宽
    faac_enc_config->bandWidth = 0;
    faac_enc_config->shortctl = SHORTCTL_NORMAL;

    if(!faacEncSetConfiguration(audioEncHandle,faac_enc_config)){
        LOGD("参数配置失败");
        return 1;
    }
    return init_audio_header();
}

int RtmpPush::init_audio_header() {

    unsigned char* data;
    //长度
    unsigned long len;
    faacEncGetDecoderSpecificInfo(audioEncHandle,&data,&len);
    RTMPPacket *packet = (RTMPPacket *) malloc(sizeof(RTMPPacket));
    if(!packet){
        return 1;
    }
    RTMPPacket_Reset(packet);
    if(!RTMPPacket_Alloc(packet,len + 2)){
        RTMPPacket_Free(packet);
        return 1;
    }

    char *body = packet->m_body;
    body[0] = (char) 0xaf;
    //faac 头为0x00
    body[1] = 0x00;
    memcpy(&body[2],data,len);

    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nBodySize = len + 2;
    packet->m_nChannel = 0x04;
    packet->m_hasAbsTimestamp = 0;
    packet->m_nTimeStamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;

    add_rtmp_packet(packet);
    return 2;
}

void RtmpPush::audio_cllect(char *data) {

    if(!RTMP_IsConnected(rtmpClient)){
        LOGD("rtmp未开启");
        return;
    }
    unsigned char* bitbuf = (unsigned char *) malloc(nMaxOutputBytes * sizeof(unsigned char*));
    int data_len = faacEncEncode(audioEncHandle, (int32_t *) data, nInputSamples, bitbuf, nMaxOutputBytes);

    if(data_len<=0){
        if(bitbuf){
            free(bitbuf);
        }
        return;
    }

    int body_size = data_len + 2;
    RTMPPacket *packet = (RTMPPacket *) malloc(sizeof(RTMPPacket));
    if(!packet){
        return;
    }
    RTMPPacket_Reset(packet);
    if(!RTMPPacket_Alloc(packet, (uint32_t) body_size)){
        RTMPPacket_Free(packet);
        return;
    }
    char *body = packet->m_body;
    body[0] = (char) 0xaf;
    //faac 体为0x01
    body[1] = 0x01;
    memcpy(&body[2], bitbuf, (size_t) data_len);

    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nBodySize = (uint32_t) body_size;
    packet->m_nChannel = 0x04;
    packet->m_hasAbsTimestamp = 0;
    packet->m_nTimeStamp = RTMP_GetTime() - start_time;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    add_rtmp_packet(packet);
}

void RtmpPush::video_collect_back(char *data) {
    int width = m_height;
    int height = m_width;
    int uv_height = height >> 1;
    char *yuv = new char[media_size*3/2];
    int k = 0;
    //y
    /**
     * y4 y8 y12 y16
     * y3 y7 y11 y15
     * y2 y6 y10 y14
     * y1 y5 y9  y13
     */
    for (int i = 0; i < width; i++) {
        for (int j = height - 1; j >= 0; j--) {
            yuv[k++] = data[width * j + i];
        }
    }
    //u v
    for (int j = 0; j < width; j += 2) {
        for (int i = uv_height - 1; i >= 0; i--) {
            yuv[k++] = data[media_size + width * i + j];
            yuv[k++] = data[media_size + width * i + j + 1];
        }
    }
    push_video(yuv,1);
}

void RtmpPush::video_collect_above(char *data) {
//    LOGD("数据开始编码1");
    int width = m_height;
    int height = m_width;
    int uv_height = height >> 1;
    char *yuv = new char[media_size*3/2];
    int k = 0;
    //y
    /**
     * y13  y9   y5  y1
     * y14  y10  y6  y2
     * y15  y11  y7  y3
     * y16  y12  y8  y4
     */
//    LOGD("数据开始编码2");
    for (int i = 0; i < width; i++) {
        int n_pos = width - 1 - i;
        for (int j = 0; j < height; j++) {
            yuv[k++] = data[n_pos];
            n_pos+=width;
        }
    }
//    LOGD("数据开始编码3");
    //u v
    for (int i = 0; i < width; i += 2) {
        int nPos = media_size + width - 1;
        for (int j = 0; j < uv_height; j++) {
            yuv[k] = data[nPos - i - 1];
            yuv[k + 1] = data[nPos - i];
            k += 2;
            nPos += width;
        }
    }
//    LOGD("数据开始编码4");
    push_video(yuv,1);
}

void RtmpPush::video_collect_left(char *data) {
    push_video(data,0);
}


void RtmpPush::push_video(char *data,int index) {
//    LOGD("数据开始编码");
    //android 视频采集yuv420sp格式 转h264
    /**
     * android 采集的是nv21格式数据
     * 在nv21下是
     * 	y1   y2     y3    y4
     * 	y5   y6     y7    y8
     * 	y9   y10   y11  y12
     * 	y13  y14   y15  y16
     * 	v1   u1     v2    u2
     * 	v3   u3     v4    u4
     *  先全是y数据
     *  然后v u 交替
     */
    memcpy(pic_in->img.plane[0], data, (size_t) media_size);
    char* u = (char *) pic_in->img.plane[1];
    char* v = (char *) pic_in->img.plane[2];
    for (int j = 0; j < media_size / 4; j++) {
        //获取u数据
        *(u + j) = *(data + media_size + j * 2 + 1);
        //获取y数据
        *(v + j) = *(data + media_size + j * 2);
    }
    int nNal = -1;
    x264_nal_t *nal = NULL;

    if (x264_encoder_encode(videoEncHandle, &nal, &nNal, pic_in, pic_out) < 0) {
        LOGD("编码失败");
        return;
    }

    pic_in->i_pts++;
    int sps_len = 0, pps_len = 0;
    char *sps = NULL;
    char *pps = NULL;
    for (int i = 0; i < nNal; i++) {
        if (nal[i].i_type == NAL_SPS) {
            sps_len = nal[i].i_payload - 4;
            sps = (char *) malloc((size_t) (sps_len + 1));
            memcpy(sps, nal[i].p_payload + 4, (size_t) sps_len);
        } else if (nal[i].i_type == NAL_PPS) {
            pps_len = nal[i].i_payload - 4;
            pps = (char *) malloc((size_t) (pps_len + 1));
            memcpy(pps, nal[i].p_payload + 4, (size_t) pps_len);
            //添加头编码
            add_264_header(pps, sps, pps_len, sps_len);
            free(sps);
            free(pps);
        } else {
            add_264_body((char *) nal[i].p_payload, nal[i].i_payload);
        }
    }
    if(index){
        free(data);
    }

}

void RtmpPush::add_264_header(char *pps, char *sps, int pps_len, int sps_len) {
    int body_size = 13 + sps_len + 3 + pps_len;
    RTMPPacket *packet = (RTMPPacket *) malloc(sizeof(RTMPPacket));
    if (!packet) {
        return;
    }
    RTMPPacket_Reset(packet);
    if (!RTMPPacket_Alloc(packet, (uint32_t) body_size)) {
        RTMPPacket_Free(packet);
        return;
    }
    char *body = packet->m_body;
    int k = 0;
    body[k++] = 0x17;
    body[k++] = 0x00;
    body[k++] = 0x00;
    body[k++] = 0x00;
    body[k++] = 0x00;

    body[k++] = 0x01;
    body[k++] = sps[1];
    body[k++] = sps[2];
    body[k++] = sps[3];
    body[k++] = (char) 0xff;

    //sps
    body[k++] = (char) 0xe1;
    body[k++] = (char) ((sps_len >> 8) & 0xff);
    body[k++] = (char) (sps_len & 0xff);
    memcpy(&body[k], sps, (size_t) sps_len);
    k += sps_len;

    //pps
    body[k++] = 0x01;
    body[k++] = (char) ((pps_len >> 8) & 0xff);
    body[k++] = (char) (pps_len & 0xff);
    memcpy(&body[k], pps, (size_t) pps_len);
    k += pps_len;

    //设置RTMPPacket参数
    //此处为类型有两种一种是音频,一种是视频
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    //包内容长度
    packet->m_nBodySize = (uint32_t) body_size;
    //块流ID为4
    packet->m_nChannel = 0x04;
    packet->m_nTimeStamp = 0;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;

    //添加到队列中
    add_rtmp_packet(packet);
}

void RtmpPush::add_264_body(char *buf, int len) {
    if (buf[2] == 0x00) {
        buf += 4;
        len -= 4;
    } else if (buf[2] == 0x01) {
        buf += 3;
        len -= 3;
    }
    int body_size = len + 9;
    RTMPPacket *packet = (RTMPPacket *) malloc(sizeof(RTMPPacket));
    if (!packet) {
        return;
    }
    RTMPPacket_Reset(packet);
    if (!RTMPPacket_Alloc(packet, (uint32_t) body_size)) {
        return;
    }
    char *body = packet->m_body;
    int k = 0;
    int type = buf[0] & 0x1f;
    if (type == NAL_SLICE_IDR) {
        body[k++] = 0x17;
    } else {
        body[k++] = 0x27;
    }
    body[k++] = 0x01;
    body[k++] = 0x00;
    body[k++] = 0x00;
    body[k++] = 0x00;

    body[k++] = (char) ((len >> 24) & 0xff);
    body[k++] = (char) ((len >> 16) & 0xff);
    body[k++] = (char) ((len >> 8) & 0xff);
    body[k++] = (char) (len & 0xff);

    memcpy(&body[k++], buf, (size_t) len);
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = (uint32_t) body_size;
    packet->m_nChannel = 0x04;
    packet->m_nTimeStamp = RTMP_GetTime() - start_time;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;

    //添加到队列
    add_rtmp_packet(packet);
}

void RtmpPush::add_rtmp_packet(RTMPPacket *packet) {
    pthread_mutex_lock(&mutex);
    queue_append_last(packet);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

void *RtmpPush::push_thread(void *args) {
    RtmpPush *push = (RtmpPush *) args;

    while (push->publishing) {
        pthread_mutex_lock(&push->mutex);
        pthread_cond_wait(&push->cond, &push->mutex);
        if (!push->publishing) {
            break;
        }
        RTMPPacket *packet = (RTMPPacket *) queue_get_first();
        if (packet) {
            queue_delete_first();
        }
        pthread_mutex_unlock(&push->mutex);
        if (packet) {
            if (RTMP_SendPacket(push->rtmpClient, packet, TRUE)) {
                LOGD("发送成功");
            } else {
                LOGD("发送失败");
            }
            RTMPPacket_Free(packet);
        }
        if(queue_size()>50){
            LOGD("队列数据过多");
            for(int i = 0;i<30;i++){
                queue_delete_first();
            }
        }
    }

    return NULL;
}

}
