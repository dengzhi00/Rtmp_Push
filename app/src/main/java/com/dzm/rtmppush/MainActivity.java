package com.dzm.rtmppush;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;

import com.dzm.rtmppush.audio.AudioController;
import com.dzm.rtmppush.view.CameraSurfacevView;
import com.dzm.rtmppush.view.MediaSerfaceView;

import tv.danmaku.ijk.media.player.IjkMediaPlayer;

public class MainActivity extends AppCompatActivity {

    private RtmpPush rtmpPush;


    private MediaSerfaceView mediaSurface;

    private AudioController audioController;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        initPlayer();
        initRtmp();
        initAudio();
        CameraSurfacevView cameraSurface = (CameraSurfacevView) findViewById(R.id.cameraSurface);
        cameraSurface.setmRtmpPush(rtmpPush);

        mediaSurface = (MediaSerfaceView) findViewById(R.id.mediaSurface);
        mediaSurface.setZOrderOnTop(true);
        mediaSurface.play("rtmp://192.168.1.125/live");

    }

    private void initAudio() {
        audioController = new AudioController();
        audioController.start(rtmpPush);
    }

    private void initPlayer(){
        IjkMediaPlayer.loadLibrariesOnce(null);
        IjkMediaPlayer.native_profileBegin("libijkplayer.so");
    }

    private void initRtmp(){
        rtmpPush = new RtmpPush();
//        rtmpPush.initVideo("rtmp://192.168.1.125/live",480,640,750_000);
    }

    @Override
    protected void onResume() {
        mediaSurface.onResume();
        super.onResume();
    }

    @Override
    protected void onPause() {
        mediaSurface.onPause();
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        mediaSurface.onDestroy();
        audioController.destroy();
        super.onDestroy();
    }

}
