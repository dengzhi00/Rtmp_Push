package com.dzm.rtmppush;

/**
 *
 * @author 邓治民
 * date 2017/2/22 16:43
 */

public class RtmpPush {

    static {
        System.loadLibrary("native-lib");
    }


    public native void pushVideo(byte[] data,int index);

    public native void initVideo(String url,int width,int height,int bitrate);

    public native void initAudio(int sampleRate,int channel);

    public native void pushAudio(byte[] data);
}
