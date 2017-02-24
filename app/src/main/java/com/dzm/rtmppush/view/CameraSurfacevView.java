package com.dzm.rtmppush.view;

import android.app.Activity;
import android.content.Context;
import android.hardware.Camera;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.dzm.rtmppush.RtmpPush;

import java.io.IOException;

/**
 * @author 邓治民
 *         date 2016/12/28 10:52
 *         获取摄像头 头像 承载surfaceview
 */

public class CameraSurfacevView extends SurfaceView implements SurfaceHolder.Callback, Camera.PreviewCallback {

    /** 相机 */
    private Camera mCamera;

    /** 是否打开camera */
    private boolean mPreviewRunning;

    /** rtmp native */
    private RtmpPush mRtmpPush;

    /** 0代表前置摄像头，1代表后置摄像头 */
    private int cameraPosition = 1;

    /** 分辨率宽度 */
    private static final int CAMERAWIDTH = 640;
    /** 分辨率 */
    private static final int CAMERAHEIGHT = 480;

    private final static int SCREEN_PORTRAIT = 0;
    private final static int SCREEN_LANDSCAPE_LEFT = 90;
    private final static int SCREEN_LANDSCAPE_RIGHT = 270;
    private int screen;
    private byte[] raw;

    private int cameraId;

    public CameraSurfacevView(Context context) {
        this(context, null);
    }

    public CameraSurfacevView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public CameraSurfacevView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    private void init() {
        getHolder().addCallback(this);
        getHolder().setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        raw = new byte[CAMERAWIDTH * CAMERAHEIGHT * 3 / 2];
    }

    /**
     * 设置rtmp native
     * @param mRtmpPush native
     */
    public void setmRtmpPush(RtmpPush mRtmpPush) {
        this.mRtmpPush = mRtmpPush;
    }

    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {
        mCamera = Camera.open(Camera.CameraInfo.CAMERA_FACING_FRONT);//代表摄像头的方位，CAMERA_FACING_FRONT前置
        setCameraDisplayOrientation((Activity) getContext(), Camera.CameraInfo.CAMERA_FACING_FRONT, mCamera);
        cameraId = Camera.CameraInfo.CAMERA_FACING_FRONT;
        cameraPosition = 0;

    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {
        if (mPreviewRunning) {
            mCamera.stopPreview();
        }


        Camera.Parameters p = mCamera.getParameters();
//        List<Camera.Size> sizeList = p.getSupportedPreviewSizes();
//        for(Camera.Size ss:sizeList){
//            Log.d("size", ss.width+"::"+ss.height);
//        }
        p.setPreviewSize(CAMERAWIDTH, CAMERAHEIGHT);
//		p.setPreviewSize(720, 480);
//        p.setRotation(90);
//        mCamera.setDisplayOrientation();
        mCamera.setPreviewCallback(this);
        mCamera.setParameters(p);
        try {
            mCamera.setPreviewDisplay(getHolder());
        } catch (IOException e) {
            e.printStackTrace();
        }
        mCamera.startPreview();
        mPreviewRunning = true;
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {
        if (null != mCamera) {
            mCamera.setPreviewCallback(null);
            mCamera.stopPreview();
            mCamera.release();
            mCamera = null;
        }
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        if(null == data){
            return;
        }
        switch (screen) {
            case SCREEN_PORTRAIT:
                if(cameraId == Camera.CameraInfo.CAMERA_FACING_BACK){
                    mRtmpPush.pushVideo(data,1);
                }else{
                    mRtmpPush.pushVideo(data,2);
                }
                break;
            case SCREEN_LANDSCAPE_LEFT:
                mRtmpPush.pushVideo(data,3);
                break;
            case SCREEN_LANDSCAPE_RIGHT:
                landscapeData2Raw(data);
                mRtmpPush.pushVideo(raw,3);
                break;
        }
    }

    /**
     * 前后屏切换
     */
    public void change() {
        //切换前后摄像头
        int cameraCount = 0;
        Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
        cameraCount = Camera.getNumberOfCameras();//得到摄像头的个数

        for (int i = 0; i < cameraCount; i++) {
            Camera.getCameraInfo(i, cameraInfo);//得到每一个摄像头的信息
            if (cameraPosition == 1) {
                //现在是后置，变更为前置
                if (cameraInfo.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {//代表摄像头的方位，CAMERA_FACING_FRONT前置      CAMERA_FACING_BACK后置
                    mCamera.stopPreview();//停掉原来摄像头的预览
                    mCamera.release();//释放资源
                    mCamera = null;//取消原来摄像头
                    mCamera = Camera.open(i);//打开当前选中的摄像头
                    try {
                        Camera.Parameters p = mCamera.getParameters();
                        p.setPreviewSize(CAMERAWIDTH, CAMERAHEIGHT);
                        mCamera.setPreviewCallback(this);
                        mCamera.setParameters(p);
                        mCamera.setPreviewDisplay(getHolder());//通过surfaceview显示取景画面
                    } catch (IOException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                    mCamera.startPreview();//开始预览
                    cameraPosition = 0;
                    break;
                }
            } else {
                //现在是前置， 变更为后置
                if (cameraInfo.facing == Camera.CameraInfo.CAMERA_FACING_BACK) {//代表摄像头的方位，CAMERA_FACING_FRONT前置      CAMERA_FACING_BACK后置
                    mCamera.stopPreview();//停掉原来摄像头的预览
                    mCamera.release();//释放资源
                    mCamera = null;//取消原来摄像头
                    mCamera = Camera.open(i);//打开当前选中的摄像头
                    try {
                        Camera.Parameters p = mCamera.getParameters();
                        p.setPreviewSize(CAMERAWIDTH, CAMERAHEIGHT);
                        mCamera.setPreviewCallback(this);
                        mCamera.setParameters(p);
                        mCamera.setPreviewDisplay(getHolder());//通过surfaceview显示取景画面
                    } catch (IOException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                    mCamera.startPreview();//开始预览
                    cameraPosition = 1;
                    break;
                }
            }

        }
    }

    private void setCameraDisplayOrientation(Activity activity, int cameraId, android.hardware.Camera camera) {
        android.hardware.Camera.CameraInfo info = new android.hardware.Camera.CameraInfo();
        android.hardware.Camera.getCameraInfo(cameraId, info);//得到每一个摄像头的信息
        int rotation = activity.getWindowManager().getDefaultDisplay().getRotation();
        int degrees = 0;
        switch (rotation) {
            case Surface.ROTATION_0:
                screen = SCREEN_PORTRAIT;
                degrees = 0;
                break;
            case Surface.ROTATION_90:// 横屏 左边是头部(home键在右边)
                screen = SCREEN_LANDSCAPE_LEFT;
                degrees = 90;
                break;
            case Surface.ROTATION_180:
                screen = 180;
                degrees = 180;
                break;
            case Surface.ROTATION_270:// 横屏 头部在右边
                screen = SCREEN_LANDSCAPE_RIGHT;
                degrees = 270;
                break;
        }
        int result;
        if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
            result = (info.orientation + degrees) % 360;
            result = (360 - result) % 360;   // compensate the mirror
        } else {
            // back-facing
            result = (info.orientation - degrees + 360) % 360;
        }
        camera.setDisplayOrientation(result);
    }

    private void landscapeData2Raw(byte[] data) {
        int width = CAMERAWIDTH, height = CAMERAHEIGHT;
        int y_len = width * height;
        int k = 0;
        // y数据倒叙插入raw中
        for (int i = y_len - 1; i > -1; i--) {
            raw[k] = data[i];
            k++;
        }
        // System.arraycopy(data, y_len, raw, y_len, uv_len);
        // v1 u1 v2 u2
        // v3 u3 v4 u4
        // 需要转换为:
        // v4 u4 v3 u3
        // v2 u2 v1 u1
        int maxpos = data.length - 1;
        int uv_len = y_len >> 2; // 4:1:1
        for (int i = 0; i < uv_len; i++) {
            int pos = i << 1;
            raw[y_len + i * 2] = data[maxpos - pos - 1];
            raw[y_len + i * 2 + 1] = data[maxpos - pos];
        }
    }
}
