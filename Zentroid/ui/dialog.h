#ifndef DIALOG_H
#define DIALOG_H

#include <QWidget>
#include <QPointer>
#include <QMessageBox>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QListWidget>
#include <QTimer>
#include <QPropertyAnimation>


#include "adbprocess.h"
#include "../ZentroidCore/include/ZentroidCore.h"
#ifdef HAS_QT_MULTIMEDIA
#include "audio/audiooutput.h"
#endif

class CleanModeWidget;
class AdvancedDialog;
class CustomDialog;

namespace Ui
{
    class Widget;
}

class QYUVOpenGLWidget;
class Dialog : public QWidget
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

    void outLog(const QString &log, bool newLine = true);
    bool filterLog(const QString &log);
    void getIPbyIp();

private slots:
    void onDeviceConnected(bool success, const QString& serial, const QString& deviceName, const QSize& size);
    void onDeviceDisconnected(QString serial);

    void on_updateDevice_clicked();
    void on_startServerBtn_clicked();
    void on_stopServerBtn_clicked();
    void on_wirelessConnectBtn_clicked();
    void on_startAdbdBtn_clicked();
    void on_getIPBtn_clicked();
    void on_wirelessDisConnectBtn_clicked();
    void on_selectRecordPathBtn_clicked();
    void on_recordPathEdt_textChanged(const QString &arg1);
    void on_adbCommandBtn_clicked();
    void on_stopAdbBtn_clicked();
    void on_clearOut_clicked();
    void on_stopAllServerBtn_clicked();
    void on_refreshGameScriptBtn_clicked();
    void on_applyScriptBtn_clicked();
    void on_keymapEditorBtn_clicked();
    void on_recordScreenCheck_clicked(bool checked);
    void on_usbConnectBtn_clicked();
    void on_wifiConnectBtn_clicked();
    void on_connectedPhoneList_itemDoubleClicked(QListWidgetItem *item);
    void on_updateNameBtn_clicked();
    void on_useSingleModeCheck_clicked();
    void on_serialBox_currentIndexChanged(const QString &arg1);

    void on_startAudioBtn_clicked();

    void on_stopAudioBtn_clicked();

    void on_installSndcpyBtn_clicked();

    void on_autoUpdatecheckBox_toggled(bool checked);

    void showIpEditMenu(const QPoint &pos);

private:
    bool checkAdbRun();
    void initUI();
    void updateBootConfig(bool toView = true);
    void execAdbCmd();
    void delayMs(int ms);
    QString getGameScript(const QString &fileName);
    void createNewKeymap();
    void slotActivated(QSystemTrayIcon::ActivationReason reason);
    int findDeviceFromeSerialBox(bool wifi);
    quint32 getBitRate();
    const QString &getServerPath();
    void loadIpHistory();
    void saveIpHistory(const QString &ip);
    void loadPortHistory();
    void savePortHistory(const QString &port);

    void showPortEditMenu(const QPoint &pos);

    // Clean/Legacy mode switching
    void switchToMode(bool toLegacy);
    void setupCleanMode();
    void updateCleanModeDeviceList();
    void openAdvancedDialog();
    void openCustomDialog();
    void syncPreferencesToLegacyUI();
    QStringList getWifiDeviceSerials();
    QStringList getUsbDeviceSerials();

    // Theme
    void applyTheme(bool isDark);
    QString generateLightThemeQss();

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::Widget *ui;
    qsc::AdbProcess m_adb;
    QSystemTrayIcon *m_hideIcon;
    QMenu *m_menu;
    QAction *m_showWindow;
    QAction *m_quit;
#ifdef HAS_QT_MULTIMEDIA
    AudioOutput m_audioOutput;
#endif
    QTimer m_autoUpdatetimer;

    // Clean/Legacy mode
    CleanModeWidget *m_cleanWidget;
    QWidget *m_legacyContainer;
    QCheckBox *m_legacyModeCheck;     // legacy-side checkbox to switch back
    QPointer<CustomDialog> m_customDialog;
    bool m_isLegacyMode;
    bool m_isDarkTheme = true;
    QString m_darkStyleSheet;
    QString m_cleanModeConnectedSerial;
};

#endif // DIALOG_H
