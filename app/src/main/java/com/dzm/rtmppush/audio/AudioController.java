package com.dzm.rtmppush.audio;

import com.dzm.rtmppush.RtmpPush;


/**
 *
 * @author 邓治民
 * date 2016/12/28 20:17
 * audio 控制器
 */

public class AudioController {

    static final int sampleRate = 44100;

    private static final int channel = 1;

    private AudioRunnable runnable;

    public void start(RtmpPush rtmpPush_){
        rtmpPush_.initAudio(sampleRate,channel);
        new Thread(runnable = new AudioRunnable(rtmpPush_)).start();
    }

    public void destroy() {
        runnable.destroy();
    }
}
