#include <jni.h>
#include <string>


extern "C" {
#include "RtmpPush.h"

RtmpPush *rtmp_push;
int start_vedio;
int start_audio;

static int VIDEO_VERTIAL_BACK = 1;

static int VIDEO_VERTIAL_AFTER = 2;

static int VIDEO_LEFT = 3;

void
Java_com_dzm_rtmppush_RtmpPush_initVideo(JNIEnv *env, jobject /* this */, jstring out_url, jint width,
                                         jint height, jint bitrate) {
    LOGD("rtmp 初始化");
    if(!rtmp_push){
        rtmp_push = new RtmpPush();
    }
    const char *out_url_char = env->GetStringUTFChars(out_url,0);
    start_vedio = rtmp_push->init_vedio(out_url_char,width,height,bitrate);
    env->ReleaseStringUTFChars(out_url,out_url_char);
}

void
Java_com_dzm_rtmppush_RtmpPush_pushVideo(JNIEnv *env,jobject /* this */,jbyteArray data,jint index){
    if(start_vedio != 2){
        return;
    }
//    LOGD("数据进入jni");
    char* data_char = (char *) env->GetByteArrayElements(data, 0);
    if(index == VIDEO_VERTIAL_BACK){
        rtmp_push->video_collect_back(data_char);
    }else if(index == VIDEO_VERTIAL_AFTER){
        rtmp_push->video_collect_above(data_char);
    }else if(index == VIDEO_LEFT){
        rtmp_push->video_collect_left(data_char);
    }
    env->ReleaseByteArrayElements(data, (jbyte *) data_char, 0);
}

void
Java_com_dzm_rtmppush_RtmpPush_initAudio(JNIEnv *env,jobject /* this */,jint sampleRate,jint channel){
    if(!rtmp_push){
        rtmp_push = new RtmpPush();
    }
    start_audio = rtmp_push->init_audio(sampleRate,channel);
}

void
Java_com_dzm_rtmppush_RtmpPush_pushAudio(JNIEnv *env,jobject /* this */,jbyteArray data){
    if(start_audio != 2){
        return;
    }
    char* data_char = (char *) env->GetByteArrayElements(data, 0);
    rtmp_push->audio_cllect(data_char);
    env->ReleaseByteArrayElements(data, (jbyte *) data_char, 0);
}

}