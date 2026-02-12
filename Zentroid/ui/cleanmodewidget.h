#ifndef CLEANMODEWIDGET_H
#define CLEANMODEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QStackedWidget>
#include <QFrame>

class ToggleSwitch;

class CleanModeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CleanModeWidget(QWidget *parent = nullptr);

    void updateDeviceLists(const QStringList &wifiDevices, const QStringList &usbDevices);
    void showConnectedState(const QString &serial, const QString &connectionType);
    void showDisconnectedState();
    void setStatusConnected(bool connected);

    QAbstractButton *legacyModeToggle() const { return (QAbstractButton*)m_legacyToggle; }
    void setThemeChecked(bool isDark);
    void setDarkTheme(bool isDark);

signals:
    void legacyModeToggled(bool enabled);
    void themeToggled(bool isDark);
    void connectToDevice(const QString &serial, bool isWifi);
    void autoWifiSetupRequested();
    void startMirroringRequested();
    void disconnectRequested();
    void refreshDevicesRequested();
    void advancedSettingsRequested();
    void customSettingsRequested();

private slots:
    void onWifiBtnClicked();
    void onUsbBtnClicked();
    void onGearClicked();
    void onWifiConnectClicked();
    void onUsbConnectClicked();

private:
    void setupUI();
    void applyStyles();

    // Top bar
    QLabel *m_titleLabel;
    QLabel *m_statusDot;
    ToggleSwitch *m_legacyToggle;
    QLabel *m_legacyLabel;
    ToggleSwitch *m_themeToggle;
    QLabel *m_sunLabel;
    QLabel *m_moonLabel;
    QPushButton *m_gearBtn;
    QMenu *m_gearMenu;

    // Content stack (0=disconnected, 1=connected)
    QStackedWidget *m_contentStack;

    // Disconnected page
    QWidget *m_disconnectedPage;
    QPushButton *m_wifiBtn;
    QPushButton *m_usbBtn;

    // WiFi panel
    QWidget *m_wifiPanel;
    QListWidget *m_wifiDeviceList;
    QPushButton *m_wifiRefreshBtn;
    QPushButton *m_wifiConnectBtn;
    QPushButton *m_wifiAutoSetupBtn;

    // USB panel
    QWidget *m_usbPanel;
    QListWidget *m_usbDeviceList;
    QPushButton *m_usbRefreshBtn;
    QPushButton *m_usbConnectBtn;

    // Connected page
    QWidget *m_connectedPage;
    QLabel *m_connectedLabel;
    QPushButton *m_startMirrorBtn;
    QPushButton *m_disconnectBtn;

    bool m_wifiPanelVisible;
    bool m_usbPanelVisible;
    bool m_isDark;
    QString m_connectedSerial;
};

#endif // CLEANMODEWIDGET_H
