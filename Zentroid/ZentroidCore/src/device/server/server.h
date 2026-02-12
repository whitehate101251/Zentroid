#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QPointer>
#include <QSize>

#include "adbprocess.h"
#include "tcpserver.h"
#include "videosocket.h"

class Server : public QObject
{
    Q_OBJECT

    enum SERVER_START_STEP
    {
        SSS_NULL,
        SSS_PUSH,
        SSS_ENABLE_TUNNEL_REVERSE,
        SSS_ENABLE_TUNNEL_FORWARD,
        SSS_EXECUTE_SERVER,
        SSS_RUNNING,
    };

public:
    struct ServerParams
    {
        // necessary
        QString serial = "";              // device serial number
        QString serverLocalPath = "";     // local path to the Android server

        // optional
        QString serverRemotePath = "/data/local/tmp/scrcpy-server.jar";    // path to push server on remote device
        quint16 localPort = 27183;     // local listening port for adb reverse
        quint16 maxSize = 720;         // video resolution
        quint32 bitRate = 8000000;     // video bit rate
        quint32 maxFps = 0;            // max video frame rate
        bool useReverse = true;        // true: try adb reverse first, fallback to adb forward; false: use adb forward directly
        int captureOrientationLock = 0; // lock capture orientation: 0=unlocked, 1=lock to specified, 2=lock to original
        int captureOrientation = 0;     // capture orientation: 0 90 180 270
        int stayAwake = false;         // keep device awake
        QString serverVersion = "3.3.3"; // server version
        QString logLevel = "debug";  // log level: verbose/debug/info/warn/error
        // codec options, "" means default
        // e.g. CodecOptions="profile=1,level=2"
        // more options: https://d.android.com/reference/android/media/MediaFormat
        QString codecOptions = "";
        // specify encoder name (must be H.264 encoder), "" means default
        // e.g. CodecName="OMX.qcom.video.encoder.avc"
        QString codecName = "";

        QString crop = "";             // video crop
        bool control = true;           // whether Android device accepts keyboard/mouse control
        qint32 scid = -1;             // random number, used as local socket name suffix to allow multiple connections to the same device
    };

    explicit Server(QObject *parent = nullptr);
    virtual ~Server();

    bool start(Server::ServerParams params);
    void stop();
    bool isReverse();
    Server::ServerParams getParams();
    VideoSocket *removeVideoSocket();
    QTcpSocket *getControlSocket();

signals:
    void serverStarted(bool success, const QString &deviceName = "", const QSize &size = QSize());
    void serverStoped();

private slots:
    void onWorkProcessResult(qsc::AdbProcess::ADB_EXEC_RESULT processResult);

protected:
    void timerEvent(QTimerEvent *event);

private:
    bool pushServer();
    bool enableTunnelReverse();
    bool disableTunnelReverse();
    bool enableTunnelForward();
    bool disableTunnelForward();
    bool execute();
    bool connectTo();
    bool startServerByStep();
    bool readInfo(VideoSocket *videoSocket, QString &deviceName, QSize &size);
    void startAcceptTimeoutTimer();
    void stopAcceptTimeoutTimer();
    void startConnectTimeoutTimer();
    void stopConnectTimeoutTimer();
    void onConnectTimer();

private:
    qsc::AdbProcess m_workProcess;
    qsc::AdbProcess m_serverProcess;
    TcpServer m_serverSocket; // only used if !tunnel_forward
    QPointer<VideoSocket> m_videoSocket = Q_NULLPTR;
    QPointer<QTcpSocket> m_controlSocket = Q_NULLPTR;
    bool m_tunnelEnabled = false;
    bool m_tunnelForward = false; // use "adb forward" instead of "adb reverse"
    int m_acceptTimeoutTimer = 0;
    int m_connectTimeoutTimer = 0;
    quint32 m_connectCount = 0;
    quint32 m_restartCount = 0;
    QString m_deviceName = "";
    QSize m_deviceSize = QSize();
    ServerParams m_params;

    SERVER_START_STEP m_serverStartStep = SSS_NULL;
};

#endif // SERVER_H
