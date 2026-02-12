#pragma once
#include <QString>

namespace qsc {

struct DeviceParams {
    // necessary
    QString serial = "";              // device serial number
    QString serverLocalPath = "";     // local path to the Android server

    // optional
    QString serverRemotePath = "/data/local/tmp/scrcpy-server.jar";    // path to push server on remote device
    quint16 localPort = 27183;        // local listening port for adb reverse
    quint16 maxSize = 720;            // video resolution
    quint32 bitRate = 2000000;        // video bit rate
    quint32 maxFps = 0;               // max video frame rate
    bool useReverse = true;           // true: try adb reverse first, fallback to adb forward; false: use adb forward directly
    int captureOrientationLock = 0;   // lock capture orientation: 0=unlocked, 1=lock to specified, 2=lock to original
    int captureOrientation = 0;       // capture orientation: 0 90 180 270
    bool stayAwake = false;           // keep device awake
    QString serverVersion = "3.3.3";  // server version
    QString logLevel = "debug";     // log level: verbose/debug/info/warn/error
    // codec options, "" means default
    // e.g. CodecOptions="profile=1,level=2"
    // more options: https://d.android.com/reference/android/media/MediaFormat
    QString codecOptions = "";
    // specify encoder name (must be H.264 encoder), "" means default
    // e.g. CodecName="OMX.qcom.video.encoder.avc"
    QString codecName = "";
    quint32 scid = -1; // random number, used as local socket name suffix to allow multiple connections to the same device

    QString recordPath = "";          // video save path
    QString recordFileFormat = "mp4"; // video save format: mp4/mkv
    bool recordFile = false;          // record to file

    QString pushFilePath = "/sdcard/"; // file save path on Android device (must end with /)

    bool closeScreen = false;         // auto turn off screen on start
    bool display = true;              // whether to display video (or just record in background)
    bool renderExpiredFrames = false; // whether to render expired video frames
    QString gameScript = "";          // game mapping script
};
    
}
