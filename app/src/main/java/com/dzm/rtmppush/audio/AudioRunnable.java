package com.dzm.rtmppush.audio;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;

import com.dzm.rtmppush.RtmpPush;

/**
 * @author 邓治民
 *         date 2017/2/23 15:20
 */

class AudioRunnable implements Runnable {

    private int audio_size;

    private RtmpPush rtmpPush;

    private boolean isAudio;

    AudioRunnable(RtmpPush rtmpPush){
        this.rtmpPush = rtmpPush;
        isAudio = true;
        audio_size = AudioRecord.getMinBufferSize(AudioController.sampleRate, AudioFormat.CHANNEL_IN_MONO,
                AudioFormat.ENCODING_PCM_16BIT);

    }

    @Override
    public void run() {
        AudioRecord audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC,
                AudioController.sampleRate, AudioFormat.CHANNEL_IN_MONO,
                AudioFormat.ENCODING_PCM_16BIT, audio_size);
        try {
            audioRecord.startRecording();
        } catch (Exception e) {
            e.printStackTrace();
            audioRecord.release();
            return;
        }

        while (isAudio){
            byte[] bytes = new byte[2048];
            int len = audioRecord.read(bytes, 0, bytes.length);
            if(null != rtmpPush && len >0)
                rtmpPush.pushAudio(bytes);
        }
        audioRecord.stop();
        audioRecord.release();
    }

    void destroy() {
        isAudio = false;
    }
}
