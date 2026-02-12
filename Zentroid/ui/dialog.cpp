#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMessageBox>
#include <QGuiApplication>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QScreen>
#include <QTime>
#include <QTimer>
#include <QProcess>

#include "config.h"
#include "dialog.h"
#include "ui_dialog.h"
#include "videoform.h"
#include "../groupcontroller/groupcontroller.h"
#include "keymapeditor/keymapeditor.h"
#include "keymapeditor/keymapoverlay.h"
#include "../util/keymappath.h"
#include "cleanmodewidget.h"
#include "advanceddialog.h"
#include "customdialog.h"

#ifdef Q_OS_WIN32
#include "../util/winutils.h"
#endif

// Use the shared canonical keymap directory from keymappath.h
static inline const QString &getKeyMapPath() { return getCanonicalKeymapDir(); }

Dialog::Dialog(QWidget *parent) : QWidget(parent), ui(new Ui::Widget), m_cleanWidget(nullptr), m_legacyContainer(nullptr), m_legacyModeCheck(nullptr), m_isLegacyMode(false)
{
    ui->setupUi(this);
    initUI();

    // Log the canonical keymap directory at startup
    qInfo() << "[Keymap] Canonical keymap directory:" << getKeyMapPath();
    outLog("Keymap dir: " + getKeyMapPath(), false);

    updateBootConfig(true);

    // Set up clean/legacy mode AFTER config is loaded so saved mode is applied
    setupCleanMode();

    on_useSingleModeCheck_clicked();
    on_updateDevice_clicked();

    connect(&m_autoUpdatetimer, &QTimer::timeout, this, &Dialog::on_updateDevice_clicked);
    if (ui->autoUpdatecheckBox->isChecked()) {
        m_autoUpdatetimer.start(5000);
    }

    connect(&m_adb, &qsc::AdbProcess::adbProcessResult, this, [this](qsc::AdbProcess::ADB_EXEC_RESULT processResult) {
        QString log = "";
        bool newLine = true;
        QStringList args = m_adb.arguments();

        switch (processResult) {
        case qsc::AdbProcess::AER_ERROR_START:
            break;
        case qsc::AdbProcess::AER_SUCCESS_START:
            log = "adb run";
            newLine = false;
            break;
        case qsc::AdbProcess::AER_ERROR_EXEC:
            //log = m_adb.getErrorOut();
            if (args.contains("ifconfig") && args.contains("wlan0")) {
                getIPbyIp();
            }
            break;
        case qsc::AdbProcess::AER_ERROR_MISSING_BINARY:
            log = "adb not found";
            break;
        case qsc::AdbProcess::AER_SUCCESS_EXEC:
            //log = m_adb.getStdOut();
            if (args.contains("devices")) {
                QStringList devices = m_adb.getDevicesSerialFromStdOut();
                ui->serialBox->clear();
                ui->connectedPhoneList->clear();
                for (auto &item : devices) {
                    ui->serialBox->addItem(item);
                    ui->connectedPhoneList->addItem(Config::getInstance().getNickName(item) + "-" + item);
                }
                // Update clean mode device lists
                updateCleanModeDeviceList();
            } else if (args.contains("show") && args.contains("wlan0")) {
                QString ip = m_adb.getDeviceIPFromStdOut();
                if (ip.isEmpty()) {
                    log = "ip not find, connect to wifi?";
                    break;
                }
                ui->deviceIpEdt->setEditText(ip);
            } else if (args.contains("ifconfig") && args.contains("wlan0")) {
                QString ip = m_adb.getDeviceIPFromStdOut();
                if (ip.isEmpty()) {
                    log = "ip not find, connect to wifi?";
                    break;
                }
                ui->deviceIpEdt->setEditText(ip);
            } else if (args.contains("ip -o a")) {
                QString ip = m_adb.getDeviceIPByIpFromStdOut();
                if (ip.isEmpty()) {
                    log = "ip not find, connect to wifi?";
                    break;
                }
                ui->deviceIpEdt->setEditText(ip);
            }
            break;
        }
        if (!log.isEmpty()) {
            outLog(log, newLine);
        }
    });

    m_hideIcon = new QSystemTrayIcon(this);
    m_hideIcon->setIcon(QIcon(":/image/tray/logo.png"));
    m_menu = new QMenu(this);
    m_quit = new QAction(this);
    m_showWindow = new QAction(this);
    m_showWindow->setText(tr("show"));
    m_quit->setText(tr("quit"));
    m_menu->addAction(m_showWindow);
    m_menu->addAction(m_quit);
    m_hideIcon->setContextMenu(m_menu);
    m_hideIcon->show();
    connect(m_showWindow, &QAction::triggered, this, &Dialog::show);
    connect(m_quit, &QAction::triggered, this, [this]() {
        m_hideIcon->hide();
        qApp->quit();
    });
    connect(m_hideIcon, &QSystemTrayIcon::activated, this, &Dialog::slotActivated);

    connect(&qsc::IDeviceManage::getInstance(), &qsc::IDeviceManage::deviceConnected, this, &Dialog::onDeviceConnected);
    connect(&qsc::IDeviceManage::getInstance(), &qsc::IDeviceManage::deviceDisconnected, this, &Dialog::onDeviceDisconnected);
}

Dialog::~Dialog()
{
    qDebug() << "~Dialog()";
    updateBootConfig(false);
    qsc::IDeviceManage::getInstance().disconnectAllDevice();
    delete ui;
}

void Dialog::initUI()
{
    setAttribute(Qt::WA_DeleteOnClose);
    //setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint);

    setWindowTitle(Config::getInstance().getTitle());
#ifdef Q_OS_LINUX
    // Set window icon (inherits from application icon set in main.cpp)
    // If application icon was set, this will use it automatically
    if (!qApp->windowIcon().isNull()) {
        setWindowIcon(qApp->windowIcon());
    }
#endif

#ifdef Q_OS_WIN32
    WinUtils::setDarkBorderToWindow((HWND)this->winId(), true);
#endif

    ui->bitRateEdit->setValidator(new QIntValidator(1, 99999, this));

    ui->maxSizeBox->addItem("640");
    ui->maxSizeBox->addItem("720");
    ui->maxSizeBox->addItem("1080");
    ui->maxSizeBox->addItem("1280");
    ui->maxSizeBox->addItem("1920");
    ui->maxSizeBox->addItem(tr("original"));

    ui->formatBox->addItem("mp4");
    ui->formatBox->addItem("mkv");

    ui->lockOrientationBox->addItem(tr("no lock"));
    ui->lockOrientationBox->addItem("0");
    ui->lockOrientationBox->addItem("90");
    ui->lockOrientationBox->addItem("180");
    ui->lockOrientationBox->addItem("270");
    ui->lockOrientationBox->setCurrentIndex(0);

    // load IP history
    loadIpHistory();

    // load port history
    loadPortHistory();

    // add right-click menu for deviceIpEdt
    if (ui->deviceIpEdt->lineEdit()) {
        ui->deviceIpEdt->lineEdit()->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(ui->deviceIpEdt->lineEdit(), &QWidget::customContextMenuRequested,
                this, &Dialog::showIpEditMenu);
    }
    
    // add right-click menu for devicePortEdt
    if (ui->devicePortEdt->lineEdit()) {
        ui->devicePortEdt->lineEdit()->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(ui->devicePortEdt->lineEdit(), &QWidget::customContextMenuRequested,
                this, &Dialog::showPortEditMenu);
    }
}

void Dialog::updateBootConfig(bool toView)
{
    if (toView) {
        UserBootConfig config = Config::getInstance().getUserBootConfig();

        if (config.bitRate == 0) {
            ui->bitRateBox->setCurrentText("Mbps");
        } else if (config.bitRate % 1000000 == 0) {
            ui->bitRateEdit->setText(QString::number(config.bitRate / 1000000));
            ui->bitRateBox->setCurrentText("Mbps");
        } else {
            ui->bitRateEdit->setText(QString::number(config.bitRate / 1000));
            ui->bitRateBox->setCurrentText("Kbps");
        }

        ui->maxSizeBox->setCurrentIndex(config.maxSizeIndex);
        ui->formatBox->setCurrentIndex(config.recordFormatIndex);
        ui->recordPathEdt->setText(config.recordPath);
        ui->lockOrientationBox->setCurrentIndex(config.lockOrientationIndex);
        ui->framelessCheck->setChecked(config.framelessWindow);
        ui->recordScreenCheck->setChecked(config.recordScreen);
        ui->notDisplayCheck->setChecked(config.recordBackground);
        ui->useReverseCheck->setChecked(config.reverseConnect);
        ui->fpsCheck->setChecked(config.showFPS);
        ui->alwaysTopCheck->setChecked(config.windowOnTop);
        ui->closeScreenCheck->setChecked(config.autoOffScreen);
        ui->stayAwakeCheck->setChecked(config.keepAlive);
        ui->useSingleModeCheck->setChecked(config.simpleMode);
        ui->autoUpdatecheckBox->setChecked(config.autoUpdateDevice);
        ui->showToolbar->setChecked(config.showToolbar);

        // Apply clean/legacy mode from saved config
        m_isLegacyMode = !config.cleanMode;
        m_isDarkTheme = config.darkMode;
    } else {
        UserBootConfig config;

        config.bitRate = getBitRate();
        config.maxSizeIndex = ui->maxSizeBox->currentIndex();
        config.recordFormatIndex = ui->formatBox->currentIndex();
        config.recordPath = ui->recordPathEdt->text();
        config.lockOrientationIndex = ui->lockOrientationBox->currentIndex();
        config.recordScreen = ui->recordScreenCheck->isChecked();
        config.recordBackground = ui->notDisplayCheck->isChecked();
        config.reverseConnect = ui->useReverseCheck->isChecked();
        config.showFPS = ui->fpsCheck->isChecked();
        config.windowOnTop = ui->alwaysTopCheck->isChecked();
        config.autoOffScreen = ui->closeScreenCheck->isChecked();
        config.framelessWindow = ui->framelessCheck->isChecked();
        config.keepAlive = ui->stayAwakeCheck->isChecked();
        config.simpleMode = ui->useSingleModeCheck->isChecked();
        config.autoUpdateDevice = ui->autoUpdatecheckBox->isChecked();
        config.showToolbar = ui->showToolbar->isChecked();
        config.cleanMode = !m_isLegacyMode;
        config.darkMode = m_isDarkTheme;

        // save current IP to history
        QString currentIp = ui->deviceIpEdt->currentText().trimmed();
        if (!currentIp.isEmpty()) {
            saveIpHistory(currentIp);
        }

        Config::getInstance().setUserBootConfig(config);
    }
}

void Dialog::execAdbCmd()
{
    if (checkAdbRun()) {
        return;
    }
    QString cmd = ui->adbCommandEdt->text().trimmed();
    outLog("adb " + cmd, false);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    m_adb.execute(ui->serialBox->currentText().trimmed(), cmd.split(" ", Qt::SkipEmptyParts));
#else
    m_adb.execute(ui->serialBox->currentText().trimmed(), cmd.split(" ", QString::SkipEmptyParts));
#endif
}

void Dialog::delayMs(int ms)
{
    QTime dieTime = QTime::currentTime().addMSecs(ms);

    while (QTime::currentTime() < dieTime) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
}

QString Dialog::getGameScript(const QString &fileName)
{
    if (fileName.isEmpty()) {
        return "";
    }

    QString fullPath = getKeyMapPath() + "/" + fileName;
    qInfo() << "[Keymap] Loading script from:" << fullPath;
    QFile loadFile(fullPath);
    if (!loadFile.open(QIODevice::ReadOnly)) {
        outLog("open file failed: " + fullPath, true);
        return "";
    }

    QString ret = loadFile.readAll();
    loadFile.close();
    return ret;
}

void Dialog::slotActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
#ifdef Q_OS_WIN32
        this->show();
#endif
        break;
    default:
        break;
    }
}

void Dialog::closeEvent(QCloseEvent *event)
{
    this->hide();
    if (!Config::getInstance().getTrayMessageShown()) {
        Config::getInstance().setTrayMessageShown(true);
        m_hideIcon->showMessage(tr("Notice"),
                                tr("Hidden here!"),
                                QSystemTrayIcon::Information,
                                3000);
    }
    event->ignore();
}

void Dialog::on_updateDevice_clicked()
{
    if (checkAdbRun()) {
        return;
    }
    outLog("update devices...", false);
    m_adb.execute("", QStringList() << "devices");
}

void Dialog::on_startServerBtn_clicked()
{
    outLog("start server...", false);

    // this is ok that "original" toUshort is 0
    quint16 videoSize = ui->maxSizeBox->currentText().trimmed().toUShort();
    qsc::DeviceParams params;
    params.serial = ui->serialBox->currentText().trimmed();
    params.maxSize = videoSize;
    params.bitRate = getBitRate();
    // on devices with Android >= 10, the capture frame rate can be limited
    // MaxFps=0 means auto-detect: use the monitor's native refresh rate
    int configuredFps = Config::getInstance().getMaxFps();
    if (configuredFps <= 0) {
        // Auto-detect: query the primary screen's refresh rate
        QScreen *primaryScreen = QGuiApplication::primaryScreen();
        if (primaryScreen) {
            qreal refreshRate = primaryScreen->refreshRate();
            configuredFps = static_cast<int>(qRound(refreshRate));
            qInfo("Auto-detected monitor refresh rate: %.1f Hz -> maxFps=%d", refreshRate, configuredFps);
        } else {
            configuredFps = 60; // fallback
            qWarning("Could not detect monitor refresh rate, defaulting to 60 fps");
        }
    }
    params.maxFps = static_cast<quint32>(configuredFps);
    params.closeScreen = ui->closeScreenCheck->isChecked();
    params.useReverse = ui->useReverseCheck->isChecked();
    params.display = !ui->notDisplayCheck->isChecked();
    params.renderExpiredFrames = Config::getInstance().getRenderExpiredFrames();
    if (ui->lockOrientationBox->currentIndex() > 0) {
        params.captureOrientationLock = 1;
        params.captureOrientation = (ui->lockOrientationBox->currentIndex() - 1) * 90;
    }
    params.stayAwake = ui->stayAwakeCheck->isChecked();
    params.recordFile = ui->recordScreenCheck->isChecked();
    params.recordPath = ui->recordPathEdt->text().trimmed();
    params.recordFileFormat = ui->formatBox->currentText().trimmed();
    params.serverLocalPath = getServerPath();
    params.serverRemotePath = Config::getInstance().getServerPath();
    params.pushFilePath = Config::getInstance().getPushFilePath();
    params.gameScript = getGameScript(ui->gameBox->currentText());
    params.logLevel = Config::getInstance().getLogLevel();
    params.codecOptions = Config::getInstance().getCodecOptions();
    params.codecName = Config::getInstance().getCodecName();
    params.scid = QRandomGenerator::global()->bounded(1, 10000) & 0x7FFFFFFF;

    qsc::IDeviceManage::getInstance().connectDevice(params);
}

void Dialog::on_stopServerBtn_clicked()
{
    if (qsc::IDeviceManage::getInstance().disconnectDevice(ui->serialBox->currentText().trimmed())) {
        outLog("stop server");
    }
}

void Dialog::on_wirelessConnectBtn_clicked()
{
    if (checkAdbRun()) {
        return;
    }
    QString addr = ui->deviceIpEdt->currentText().trimmed();
    if (addr.isEmpty()) {
        outLog("error: device ip is null", false);
        return;
    }

    if (!ui->devicePortEdt->currentText().isEmpty()) {
        addr += ":";
        addr += ui->devicePortEdt->currentText().trimmed();
    } else if (!ui->devicePortEdt->lineEdit()->placeholderText().isEmpty()) {
        addr += ":";
        addr += ui->devicePortEdt->lineEdit()->placeholderText().trimmed();
    } else {
        outLog("error: device port is null", false);
        return;
    }

    // save IP history - only save the IP part, not including port
    QString ip = addr.split(":").first();
    if (!ip.isEmpty()) {
        saveIpHistory(ip);
    }
    
    // save port history
    QString port = addr.split(":").last();
    if (!port.isEmpty() && port != ip) {
        savePortHistory(port);
    }

    outLog("wireless connect...", false);
    QStringList adbArgs;
    adbArgs << "connect";
    adbArgs << addr;
    m_adb.execute("", adbArgs);
}

void Dialog::on_startAdbdBtn_clicked()
{
    if (checkAdbRun()) {
        return;
    }
    outLog("start devices adbd...", false);
    // adb tcpip 5555
    QStringList adbArgs;
    adbArgs << "tcpip";
    adbArgs << "5555";
    m_adb.execute(ui->serialBox->currentText().trimmed(), adbArgs);
}

void Dialog::outLog(const QString &log, bool newLine)
{
    // avoid sub thread update ui
    QString backLog = log;
    QTimer::singleShot(0, this, [this, backLog, newLine]() {
        ui->outEdit->append(backLog);
        if (newLine) {
            ui->outEdit->append("<br/>");
        }
        // Forward to custom dialog if open
        if (m_customDialog && m_customDialog->isVisible()) {
            m_customDialog->appendLog(backLog);
        }
    });
}

bool Dialog::filterLog(const QString &log)
{
    if (log.contains("app_proces")) {
        return true;
    }
    if (log.contains("Unable to set geometry")) {
        return true;
    }
    return false;
}

bool Dialog::checkAdbRun()
{
    if (m_adb.isRuning()) {
        outLog("wait for the end of the current command to run");
    }
    return m_adb.isRuning();
}

void Dialog::on_getIPBtn_clicked()
{
    if (checkAdbRun()) {
        return;
    }

    outLog("get ip...", false);
    // adb -s P7C0218510000537 shell ifconfig wlan0
    // or
    // adb -s P7C0218510000537 shell ip -f inet addr show wlan0
    QStringList adbArgs;
#if 0
    adbArgs << "shell";
    adbArgs << "ip";
    adbArgs << "-f";
    adbArgs << "inet";
    adbArgs << "addr";
    adbArgs << "show";
    adbArgs << "wlan0";
#else
    adbArgs << "shell";
    adbArgs << "ifconfig";
    adbArgs << "wlan0";
#endif
    m_adb.execute(ui->serialBox->currentText().trimmed(), adbArgs);
}

void Dialog::getIPbyIp()
{
    if (checkAdbRun()) {
        return;
    }

    QStringList adbArgs;
    adbArgs << "shell";
    adbArgs << "ip -o a";

    m_adb.execute(ui->serialBox->currentText().trimmed(), adbArgs);
}

void Dialog::onDeviceConnected(bool success, const QString &serial, const QString &deviceName, const QSize &size)
{
    Q_UNUSED(deviceName);
    if (!success) {
        return;
    }
    auto videoForm = new VideoForm(ui->framelessCheck->isChecked(), Config::getInstance().getSkin(), ui->showToolbar->isChecked());
    videoForm->setSerial(serial);

    qsc::IDeviceManage::getInstance().getDevice(serial)->setUserData(static_cast<void*>(videoForm));
    qsc::IDeviceManage::getInstance().getDevice(serial)->registerDeviceObserver(videoForm);


    videoForm->showFPS(ui->fpsCheck->isChecked());

    // Tell the VideoForm which keymap file was selected in the dropdown
    // so that F12 auto-load uses it instead of the alphabetical first file.
    QString selectedKeymap = ui->gameBox->currentText();
    if (!selectedKeymap.isEmpty()) {
        videoForm->setActiveKeymapPath(getKeyMapPath() + "/" + selectedKeymap);
    }

    if (ui->alwaysTopCheck->isChecked()) {
        videoForm->staysOnTop();
    }

#ifndef Q_OS_WIN32
    // must be show before updateShowSize
    videoForm->show();
#endif
    QString name = Config::getInstance().getNickName(serial);
    if (name.isEmpty()) {
        name = Config::getInstance().getTitle();
    }
    videoForm->setWindowTitle(name + "-" + serial);
    videoForm->updateShowSize(size);

    bool deviceVer = size.height() > size.width();
    QRect rc = Config::getInstance().getRect(serial);
    bool rcVer = rc.height() > rc.width();
    // same width/height rate
    if (rc.isValid() && (deviceVer == rcVer)) {
        // mark: resize is for fix setGeometry magneticwidget bug
        videoForm->resize(rc.size());
        videoForm->setGeometry(rc);
    }

#ifdef Q_OS_WIN32
    // on Windows, showing too early reveals the resize process
    QTimer::singleShot(200, videoForm, [videoForm](){videoForm->show();});
#endif

    GroupController::instance().addDevice(serial);

    // Update clean mode widget
    if (m_cleanWidget) {
        QString displayName = Config::getInstance().getNickName(serial);
        if (displayName.isEmpty() || displayName == "Phone") displayName = serial;
        // Determine connection type by checking if serial looks like IP:port
        bool isWifi = serial.contains(":");
        m_cleanWidget->showConnectedState(displayName, isWifi ? "WiFi" : "USB");
        m_cleanModeConnectedSerial = serial;
    }
}

void Dialog::onDeviceDisconnected(QString serial)
{
    GroupController::instance().removeDevice(serial);
    auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
    if (!device) {
        return;
    }
    auto data = device->getUserData();
    if (data) {
        VideoForm* vf = static_cast<VideoForm*>(data);
        qsc::IDeviceManage::getInstance().getDevice(serial)->deRegisterDeviceObserver(vf);
        vf->close();
        vf->deleteLater();
    }

    // Update clean mode widget
    if (m_cleanWidget && m_cleanModeConnectedSerial == serial) {
        m_cleanWidget->showDisconnectedState();
        m_cleanModeConnectedSerial.clear();
    }
}

void Dialog::on_wirelessDisConnectBtn_clicked()
{
    if (checkAdbRun()) {
        return;
    }
    QString addr = ui->deviceIpEdt->currentText().trimmed();
    outLog("wireless disconnect...", false);
    QStringList adbArgs;
    adbArgs << "disconnect";
    adbArgs << addr;
    m_adb.execute("", adbArgs);
}

void Dialog::on_selectRecordPathBtn_clicked()
{
    QFileDialog::Options options = QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly;
    QString directory = QFileDialog::getExistingDirectory(this, tr("select path"), "", options);
    ui->recordPathEdt->setText(directory);
}

void Dialog::on_recordPathEdt_textChanged(const QString &arg1)
{
    ui->recordPathEdt->setToolTip(arg1.trimmed());
    ui->notDisplayCheck->setCheckable(!arg1.trimmed().isEmpty());
}

void Dialog::on_adbCommandBtn_clicked()
{
    execAdbCmd();
}

void Dialog::on_stopAdbBtn_clicked()
{
    m_adb.kill();
}

void Dialog::on_clearOut_clicked()
{
    ui->outEdit->clear();
}

void Dialog::on_stopAllServerBtn_clicked()
{
    qsc::IDeviceManage::getInstance().disconnectAllDevice();
}

void Dialog::on_refreshGameScriptBtn_clicked()
{
    // Remember current selection so we can restore it after repopulating
    QString previousSelection = ui->gameBox->currentText();

    ui->gameBox->clear();
    QDir dir(getKeyMapPath());
    qInfo() << "[Keymap] Refreshing dropdown from:" << dir.absolutePath();
    if (!dir.exists()) {
        outLog("keymap directory not found: " + dir.absolutePath(), true);
        return;
    }
    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    QFileInfoList list = dir.entryInfoList();
    QFileInfo fileInfo;
    int size = list.size();
    for (int i = 0; i < size; ++i) {
        fileInfo = list.at(i);
        ui->gameBox->addItem(fileInfo.fileName());
    }

    // Add separator + "Add New Keymap..." option at the end
    ui->gameBox->insertSeparator(ui->gameBox->count());
    ui->gameBox->addItem("➕ Add New Keymap...");

    // Connect handler for "Add New Keymap..." selection (connect once)
    static bool addNewConnected = false;
    if (!addNewConnected) {
        connect(ui->gameBox, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
            if (ui->gameBox->itemText(index) == "➕ Add New Keymap...") {
                createNewKeymap();
            }
        });
        addNewConnected = true;
    }

    // Restore previous selection if it still exists
    if (!previousSelection.isEmpty()) {
        int idx = ui->gameBox->findText(previousSelection);
        if (idx >= 0) {
            ui->gameBox->setCurrentIndex(idx);
            qInfo() << "[Keymap] Restored dropdown selection:" << previousSelection;
        }
    }
}

void Dialog::createNewKeymap()
{
    bool ok;
    QString name = QInputDialog::getText(this, "New Keymap",
        "Enter keymap name (no extension):", QLineEdit::Normal, "", &ok);

    if (!ok || name.trimmed().isEmpty()) {
        // User cancelled — restore previous selection
        on_refreshGameScriptBtn_clicked();
        return;
    }

    name = name.trimmed();
    // Sanitize: remove characters that are unsafe for filenames
    name.replace(QRegularExpression("[^a-zA-Z0-9_\\-]"), "_");

    QString fileName = name + ".json";
    QString fullPath = getKeyMapPath() + "/" + fileName;

    if (QFile::exists(fullPath)) {
        QMessageBox::warning(this, "Keymap Exists",
            QString("A keymap named '%1' already exists.").arg(fileName));
        on_refreshGameScriptBtn_clicked();
        return;
    }

    // Create default keymap JSON
    QJsonObject root;
    root["switchKey"] = "Ctrl+Key_Backslash";
    root["suspendKey"] = "Key_X";
    root["keyMapNodes"] = QJsonArray();

    QJsonDocument doc(root);
    QFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Error",
            QString("Failed to create keymap file:\n%1").arg(fullPath));
        return;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qInfo() << "[Keymap] Created new keymap:" << fullPath;
    outLog("Created new keymap: " + fullPath, false);

    // Refresh dropdown and select the new file
    on_refreshGameScriptBtn_clicked();
    int idx = ui->gameBox->findText(fileName);
    if (idx >= 0) {
        ui->gameBox->setCurrentIndex(idx);
    }
}

void Dialog::on_applyScriptBtn_clicked()
{
    auto curSerial = ui->serialBox->currentText().trimmed();
    auto device = qsc::IDeviceManage::getInstance().getDevice(curSerial);
    if (!device) {
        return;
    }

    QString scriptName = ui->gameBox->currentText();
    QString fullPath = getKeyMapPath() + "/" + scriptName;
    qInfo() << "[Keymap] Applying keymap:" << fullPath;
    outLog("Applying keymap: " + fullPath, false);
    device->updateScript(getGameScript(scriptName));

    // Tell the VideoForm which keymap is now active so that F12
    // auto-load uses the user's explicit selection, not alphabetical first.
    auto data = device->getUserData();
    if (data) {
        VideoForm *vf = static_cast<VideoForm*>(data);
        vf->setActiveKeymapPath(fullPath);
    }
}

void Dialog::on_keymapEditorBtn_clicked()
{
    auto curSerial = ui->serialBox->currentText().trimmed();
    QPixmap screenshot;
    QSize deviceSize(1920, 1080); // default fallback — landscape for games

    // Try to get screenshot and size from connected device
    auto device = qsc::IDeviceManage::getInstance().getDevice(curSerial);
    if (device) {
        auto data = device->getUserData();
        if (data) {
            VideoForm *vf = static_cast<VideoForm*>(data);
            screenshot = vf->getScreenshot();
            if (!vf->frameSize().isEmpty()) {
                deviceSize = vf->frameSize();
            }
        }
    }

    if (screenshot.isNull()) {
        // No device connected — create a placeholder canvas
        screenshot = QPixmap(deviceSize);
        screenshot.fill(QColor(40, 40, 40));
        outLog("No device screenshot available, using blank canvas", true);
    }

    KeymapEditorDialog *editor = new KeymapEditorDialog(screenshot, deviceSize, this);
    editor->setAttribute(Qt::WA_DeleteOnClose);

    // If a keymap is currently selected, load it
    QString currentKeymap = ui->gameBox->currentText();
    if (!currentKeymap.isEmpty()) {
        QString keymapPath = getKeyMapPath() + "/" + currentKeymap;
        qInfo() << "[Keymap] Opening editor with:" << keymapPath;
        outLog("Editor opening: " + keymapPath, false);
        editor->loadKeymap(keymapPath);
    }

    // When user applies from editor, update the device and overlay
    connect(editor, &KeymapEditorDialog::keymapApplied, this, [this](const QString &filePath) {
        auto curSerial = ui->serialBox->currentText().trimmed();
        auto device = qsc::IDeviceManage::getInstance().getDevice(curSerial);
        if (device) {
            QFile f(filePath);
            if (f.open(QIODevice::ReadOnly)) {
                device->updateScript(f.readAll());
                f.close();
                outLog("Applied keymap: " + filePath, true);
            }
            // Reload overlay if visible
            auto data = device->getUserData();
            if (data) {
                VideoForm *vf = static_cast<VideoForm*>(data);
                if (vf->keymapOverlay() && vf->keymapOverlay()->isVisible()) {
                    vf->showKeymapOverlay(filePath);
                }
            }
        }
        // Refresh profile list
        on_refreshGameScriptBtn_clicked();
    });

    editor->show();
}

void Dialog::on_recordScreenCheck_clicked(bool checked)
{
    if (!checked) {
        return;
    }

    QString fileDir(ui->recordPathEdt->text().trimmed());
    if (fileDir.isEmpty()) {
        qWarning() << "please select record save path!!!";
        ui->recordScreenCheck->setChecked(false);
    }
}

void Dialog::on_usbConnectBtn_clicked()
{
    on_stopAllServerBtn_clicked();
    delayMs(200);
    on_updateDevice_clicked();
    delayMs(200);

    int firstUsbDevice = findDeviceFromeSerialBox(false);
    if (-1 == firstUsbDevice) {
        qWarning() << "No use device is found!";
        return;
    }
    ui->serialBox->setCurrentIndex(firstUsbDevice);

    on_startServerBtn_clicked();
}

int Dialog::findDeviceFromeSerialBox(bool wifi)
{
    QString regStr = "\\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\:([0-9]|[1-9]\\d|[1-9]\\d{2}|[1-9]\\d{3}|[1-5]\\d{4}|6[0-4]\\d{3}|65[0-4]\\d{2}|655[0-2]\\d|6553[0-5])\\b";
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QRegExp regIP(regStr);
#else
    QRegularExpression regIP(regStr);
#endif
    for (int i = 0; i < ui->serialBox->count(); ++i) {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        bool isWifi = regIP.exactMatch(ui->serialBox->itemText(i));
#else
        bool isWifi = regIP.match(ui->serialBox->itemText(i)).hasMatch();
#endif
        bool found = wifi ? isWifi : !isWifi;
        if (found) {
            return i;
        }
    }

    return -1;
}

void Dialog::on_wifiConnectBtn_clicked()
{
    on_stopAllServerBtn_clicked();
    delayMs(200);

    on_updateDevice_clicked();
    delayMs(200);

    int firstUsbDevice = findDeviceFromeSerialBox(false);
    if (-1 == firstUsbDevice) {
        qWarning() << "No use device is found!";
        return;
    }
    ui->serialBox->setCurrentIndex(firstUsbDevice);

    on_getIPBtn_clicked();
    delayMs(200);

    on_startAdbdBtn_clicked();
    delayMs(1000);

    on_wirelessConnectBtn_clicked();
    delayMs(2000);

    on_updateDevice_clicked();
    delayMs(200);

    int firstWifiDevice = findDeviceFromeSerialBox(true);
    if (-1 == firstWifiDevice) {
        qWarning() << "No wifi device is found!";
        return;
    }
    ui->serialBox->setCurrentIndex(firstWifiDevice);

    on_startServerBtn_clicked();
}

void Dialog::on_connectedPhoneList_itemDoubleClicked(QListWidgetItem *item)
{
    Q_UNUSED(item);
    ui->serialBox->setCurrentIndex(ui->connectedPhoneList->currentRow());
    on_startServerBtn_clicked();
}

void Dialog::on_updateNameBtn_clicked()
{
    if (ui->serialBox->count() != 0) {
        if (ui->userNameEdt->text().isEmpty()) {
            Config::getInstance().setNickName(ui->serialBox->currentText(), "Phone");
        } else {
            Config::getInstance().setNickName(ui->serialBox->currentText(), ui->userNameEdt->text());
        }

        on_updateDevice_clicked();

        qDebug() << "Update OK!";
    } else {
        qWarning() << "No device is connected!";
    }
}

void Dialog::on_useSingleModeCheck_clicked()
{
    if (ui->useSingleModeCheck->isChecked()) {
        ui->rightWidget->hide();
    } else {
        ui->rightWidget->show();
    }

    adjustSize();
}

void Dialog::on_serialBox_currentIndexChanged(const QString &arg1)
{
    ui->userNameEdt->setText(Config::getInstance().getNickName(arg1));
}

quint32 Dialog::getBitRate()
{
    return ui->bitRateEdit->text().trimmed().toUInt() *
            (ui->bitRateBox->currentText() == QString("Mbps") ? 1000000 : 1000);
}

const QString &Dialog::getServerPath()
{
    static QString serverPath;
    if (serverPath.isEmpty()) {
        serverPath = QString::fromLocal8Bit(qgetenv("ZENTROID_SERVER_PATH"));
        QFileInfo fileInfo(serverPath);
        if (serverPath.isEmpty() || !fileInfo.isFile()) {
            serverPath = QCoreApplication::applicationDirPath() + "/scrcpy-server";
        }
    }
    return serverPath;
}

#ifdef HAS_QT_MULTIMEDIA
void Dialog::on_startAudioBtn_clicked()
{
    if (ui->serialBox->count() == 0) {
        qWarning() << "No device is connected!";
        return;
    }

    m_audioOutput.start(ui->serialBox->currentText(), 28200);
}

void Dialog::on_stopAudioBtn_clicked()
{
    m_audioOutput.stop();
}

void Dialog::on_installSndcpyBtn_clicked()
{
    if (ui->serialBox->count() == 0) {
        qWarning() << "No device is connected!";
        return;
    }
    m_audioOutput.installonly(ui->serialBox->currentText(), 28200);
}
#else
void Dialog::on_startAudioBtn_clicked()
{
    qWarning() << "Audio not available - Qt Multimedia not found";
}

void Dialog::on_stopAudioBtn_clicked()
{
    qWarning() << "Audio not available - Qt Multimedia not found";
}

void Dialog::on_installSndcpyBtn_clicked()
{
    qWarning() << "Audio not available - Qt Multimedia not found";
}
#endif

void Dialog::on_autoUpdatecheckBox_toggled(bool checked)
{
    if (checked) {
        m_autoUpdatetimer.start(5000);
    } else {
        m_autoUpdatetimer.stop();
    }
}

void Dialog::loadIpHistory()
{
    QStringList ipList = Config::getInstance().getIpHistory();
    ui->deviceIpEdt->clear();
    ui->deviceIpEdt->addItems(ipList);
    ui->deviceIpEdt->setContentsMargins(0, 0, 0, 0);

    if (ui->deviceIpEdt->lineEdit()) {
        ui->deviceIpEdt->lineEdit()->setMaxLength(128);
        ui->deviceIpEdt->lineEdit()->setPlaceholderText("192.168.0.1");
    }
}

void Dialog::saveIpHistory(const QString &ip)
{
    if (ip.isEmpty()) {
        return;
    }
    
    Config::getInstance().saveIpHistory(ip);
    
    // update ComboBox
    loadIpHistory();
    ui->deviceIpEdt->setCurrentText(ip);
}

void Dialog::showIpEditMenu(const QPoint &pos)
{
    QMenu *menu = ui->deviceIpEdt->lineEdit()->createStandardContextMenu();
    menu->addSeparator();
    
    QAction *clearHistoryAction = new QAction(tr("Clear History"), menu);
    connect(clearHistoryAction, &QAction::triggered, this, [this]() {
        Config::getInstance().clearIpHistory();
        loadIpHistory();
    });
    
    menu->addAction(clearHistoryAction);
    menu->exec(ui->deviceIpEdt->lineEdit()->mapToGlobal(pos));
    delete menu;
}

void Dialog::loadPortHistory()
{
    QStringList portList = Config::getInstance().getPortHistory();
    ui->devicePortEdt->clear();
    ui->devicePortEdt->addItems(portList);
    ui->devicePortEdt->setContentsMargins(0, 0, 0, 0);

    if (ui->devicePortEdt->lineEdit()) {
        ui->devicePortEdt->lineEdit()->setMaxLength(6);
        ui->devicePortEdt->lineEdit()->setPlaceholderText("5555");
    }
}

void Dialog::savePortHistory(const QString &port)
{
    if (port.isEmpty()) {
        return;
    }
    
    Config::getInstance().savePortHistory(port);
    
    // update ComboBox
    loadPortHistory();
    ui->devicePortEdt->setCurrentText(port);
}

void Dialog::showPortEditMenu(const QPoint &pos)
{
    QMenu *menu = ui->devicePortEdt->lineEdit()->createStandardContextMenu();
    menu->addSeparator();
    
    QAction *clearHistoryAction = new QAction(tr("Clear History"), menu);
    connect(clearHistoryAction, &QAction::triggered, this, [this]() {
        Config::getInstance().clearPortHistory();
        loadPortHistory();
    });
    
    menu->addAction(clearHistoryAction);
    menu->exec(ui->devicePortEdt->lineEdit()->mapToGlobal(pos));
    delete menu;
}

// =============================================================================
// Clean Mode / Legacy Mode switching
// =============================================================================

void Dialog::setupCleanMode()
{
    // 1. Create the clean mode widget
    m_cleanWidget = new CleanModeWidget(this);

    // 2. Create a legacy container and reparent existing UI into it
    m_legacyContainer = new QWidget(this);
    QVBoxLayout *legacyOuterLayout = new QVBoxLayout(m_legacyContainer);
    legacyOuterLayout->setContentsMargins(0, 0, 0, 0);
    legacyOuterLayout->setSpacing(0);

    // Add a small "Legacy Mode" checkbox at the top of legacy view (to switch back)
    QHBoxLayout *legacyTopBar = new QHBoxLayout();
    legacyTopBar->setContentsMargins(8, 4, 8, 4);
    m_legacyModeCheck = new QCheckBox("Legacy Mode", this);
    m_legacyModeCheck->setChecked(true);
    m_legacyModeCheck->setStyleSheet("QCheckBox { color: #00BB9E; font-weight: bold; font-size: 12px; }");
    legacyTopBar->addWidget(m_legacyModeCheck);
    legacyTopBar->addStretch();
    legacyOuterLayout->addLayout(legacyTopBar);

    // Move existing left and right widgets into the legacy container
    QWidget *legacyContent = new QWidget(this);
    QHBoxLayout *legacyHLayout = new QHBoxLayout(legacyContent);
    legacyHLayout->setContentsMargins(0, 0, 0, 0);

    ui->horizontalLayout_11->removeWidget(ui->leftWidget);
    ui->horizontalLayout_11->removeWidget(ui->rightWidget);
    legacyHLayout->addWidget(ui->leftWidget);
    legacyHLayout->addWidget(ui->rightWidget);
    legacyOuterLayout->addWidget(legacyContent, 1);

    // 3. Add both widgets to the main layout
    ui->horizontalLayout_11->addWidget(m_cleanWidget);
    ui->horizontalLayout_11->addWidget(m_legacyContainer);

    // 4. Connect clean mode signals
    connect(m_cleanWidget, &CleanModeWidget::legacyModeToggled, this, [this](bool toLegacy) {
        switchToMode(toLegacy);
    });
    connect(m_legacyModeCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (!checked) {
            switchToMode(false);  // Back to clean mode
        }
    });

    connect(m_cleanWidget, &CleanModeWidget::refreshDevicesRequested, this, &Dialog::on_updateDevice_clicked);

    connect(m_cleanWidget, &CleanModeWidget::connectToDevice, this, [this](const QString &serial, bool isWifi) {
        Q_UNUSED(isWifi);
        // Select the serial in the legacy serialBox
        int idx = ui->serialBox->findText(serial);
        if (idx >= 0) {
            ui->serialBox->setCurrentIndex(idx);
        }
        m_cleanModeConnectedSerial = serial;
        // Show connected state immediately
        QString name = Config::getInstance().getNickName(serial);
        if (name.isEmpty() || name == "Phone") name = serial;
        m_cleanWidget->showConnectedState(name, isWifi ? "WiFi" : "USB");
    });

    connect(m_cleanWidget, &CleanModeWidget::autoWifiSetupRequested, this, [this]() {
        on_wifiConnectBtn_clicked();
    });

    connect(m_cleanWidget, &CleanModeWidget::startMirroringRequested, this, [this]() {
        // Ensure correct serial is selected
        if (!m_cleanModeConnectedSerial.isEmpty()) {
            int idx = ui->serialBox->findText(m_cleanModeConnectedSerial);
            if (idx >= 0) {
                ui->serialBox->setCurrentIndex(idx);
            }
        }
        on_startServerBtn_clicked();
    });

    connect(m_cleanWidget, &CleanModeWidget::disconnectRequested, this, [this]() {
        on_stopServerBtn_clicked();
        m_cleanWidget->showDisconnectedState();
        m_cleanModeConnectedSerial.clear();
    });

    connect(m_cleanWidget, &CleanModeWidget::advancedSettingsRequested, this, &Dialog::openAdvancedDialog);
    connect(m_cleanWidget, &CleanModeWidget::customSettingsRequested, this, &Dialog::openCustomDialog);

    // Theme toggle
    connect(m_cleanWidget, &CleanModeWidget::themeToggled, this, &Dialog::applyTheme);

    // 5. Apply initial mode
    if (m_isLegacyMode) {
        m_cleanWidget->hide();
        m_legacyContainer->show();
    } else {
        m_legacyContainer->hide();
        m_cleanWidget->show();
        // Set compact window size for clean mode
        QTimer::singleShot(0, this, [this]() {
            resize(480, 440);
        });
    }

    // 6. Apply initial theme
    m_cleanWidget->setThemeChecked(m_isDarkTheme);
    if (!m_isDarkTheme) {
        applyTheme(false);
    }
}

void Dialog::switchToMode(bool toLegacy)
{
    if (toLegacy == m_isLegacyMode) return;
    m_isLegacyMode = toLegacy;

    QRect startGeo = geometry();

    // Block checkbox signals during switch to prevent recursion
    m_cleanWidget->legacyModeToggle()->blockSignals(true);
    m_legacyModeCheck->blockSignals(true);

    if (toLegacy) {
        m_cleanWidget->hide();
        m_legacyContainer->show();
        m_cleanWidget->legacyModeToggle()->setChecked(true);
        m_legacyModeCheck->setChecked(true);

        // Apply simple mode state in legacy
        on_useSingleModeCheck_clicked();
    } else {
        m_legacyContainer->hide();
        m_cleanWidget->show();
        m_cleanWidget->legacyModeToggle()->setChecked(false);
        m_legacyModeCheck->setChecked(false);
    }

    m_cleanWidget->legacyModeToggle()->blockSignals(false);
    m_legacyModeCheck->blockSignals(false);

    // Allow any size during animation
    setMinimumSize(1, 1);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

    // Calculate target size
    QSize targetSize;
    if (toLegacy) {
        // Let layout compute ideal legacy size
        adjustSize();
        targetSize = size();
    } else {
        targetSize = QSize(480, 440);
    }

    // Keep center aligned
    QRect targetGeo(startGeo.topLeft(), targetSize);
    QPoint center = startGeo.center();
    targetGeo.moveCenter(center);

    // Ensure target stays on screen
    QRect screenGeo = QGuiApplication::primaryScreen()->availableGeometry();
    if (targetGeo.left() < screenGeo.left()) targetGeo.moveLeft(screenGeo.left());
    if (targetGeo.top() < screenGeo.top()) targetGeo.moveTop(screenGeo.top());
    if (targetGeo.right() > screenGeo.right()) targetGeo.moveRight(screenGeo.right());
    if (targetGeo.bottom() > screenGeo.bottom()) targetGeo.moveBottom(screenGeo.bottom());

    // Reset to start for animation
    setGeometry(startGeo);

    QPropertyAnimation *anim = new QPropertyAnimation(this, "geometry");
    anim->setDuration(350);
    anim->setStartValue(startGeo);
    anim->setEndValue(targetGeo);
    anim->setEasingCurve(QEasingCurve::InOutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void Dialog::updateCleanModeDeviceList()
{
    if (!m_cleanWidget) return;

    QStringList wifiDevs = getWifiDeviceSerials();
    QStringList usbDevs = getUsbDeviceSerials();
    m_cleanWidget->updateDeviceLists(wifiDevs, usbDevs);
}

QStringList Dialog::getWifiDeviceSerials()
{
    QStringList result;
    QString regStr = "\\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}"
                     "(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\:"
                     "([0-9]|[1-9]\\d|[1-9]\\d{2}|[1-9]\\d{3}|[1-5]\\d{4}|"
                     "6[0-4]\\d{3}|65[0-4]\\d{2}|655[0-2]\\d|6553[0-5])\\b";
    QRegularExpression regIP(regStr);
    for (int i = 0; i < ui->serialBox->count(); ++i) {
        QString serial = ui->serialBox->itemText(i);
        if (regIP.match(serial).hasMatch()) {
            result.append(serial);
        }
    }
    return result;
}

QStringList Dialog::getUsbDeviceSerials()
{
    QStringList result;
    QString regStr = "\\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}"
                     "(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\:"
                     "([0-9]|[1-9]\\d|[1-9]\\d{2}|[1-9]\\d{3}|[1-5]\\d{4}|"
                     "6[0-4]\\d{3}|65[0-4]\\d{2}|655[0-2]\\d|6553[0-5])\\b";
    QRegularExpression regIP(regStr);
    for (int i = 0; i < ui->serialBox->count(); ++i) {
        QString serial = ui->serialBox->itemText(i);
        if (!regIP.match(serial).hasMatch()) {
            result.append(serial);
        }
    }
    return result;
}

void Dialog::openAdvancedDialog()
{
    AdvancedDialog dlg(this);
    dlg.setDarkTheme(m_isDarkTheme);

    // Populate from current legacy UI values
    dlg.setBitRate(getBitRate());
    dlg.setMaxSizeIndex(ui->maxSizeBox->currentIndex());
    dlg.setLockOrientationIndex(ui->lockOrientationBox->currentIndex());
    dlg.setRecordFormatIndex(ui->formatBox->currentIndex());
    dlg.setRecordPath(ui->recordPathEdt->text());

    // Render driver
    UserBootConfig bootCfg = Config::getInstance().getUserBootConfig();
    dlg.setRenderDriverValue(bootCfg.renderDriverIndex);

    bool wantsRestart = false;
    connect(&dlg, &AdvancedDialog::restartRequested, this, [&wantsRestart]() {
        wantsRestart = true;
    });

    if (dlg.exec() == QDialog::Accepted) {
        // Write back to legacy UI widgets (which the rest of the code reads from)
        quint32 bitRate = dlg.getBitRate();
        if (bitRate == 0) {
            ui->bitRateBox->setCurrentText("Mbps");
            ui->bitRateEdit->clear();
        } else if (bitRate % 1000000 == 0) {
            ui->bitRateEdit->setText(QString::number(bitRate / 1000000));
            ui->bitRateBox->setCurrentText("Mbps");
        } else {
            ui->bitRateEdit->setText(QString::number(bitRate / 1000));
            ui->bitRateBox->setCurrentText("Kbps");
        }

        ui->maxSizeBox->setCurrentIndex(dlg.getMaxSizeIndex());
        ui->lockOrientationBox->setCurrentIndex(dlg.getLockOrientationIndex());
        ui->formatBox->setCurrentIndex(dlg.getRecordFormatIndex());
        ui->recordPathEdt->setText(dlg.getRecordPath());

        // Save render driver to user config
        {
            UserBootConfig cfg = Config::getInstance().getUserBootConfig();
            int newDriver = dlg.getRenderDriverValue();
            if (cfg.renderDriverIndex != newDriver) {
                cfg.renderDriverIndex = newDriver;
                Config::getInstance().setUserBootConfig(cfg);
            }
        }

        outLog("Advanced settings updated", true);

        // Restart the application if the user clicked "Restart Now"
        if (wantsRestart) {
            outLog("Restarting Zentroid...", true);
            // Small delay so the user sees the log message
            QTimer::singleShot(300, qApp, []() {
                QProcess::startDetached(QCoreApplication::applicationFilePath(),
                                        QCoreApplication::arguments());
                qApp->quit();
            });
        }
    }
}

void Dialog::openCustomDialog()
{
    if (!m_customDialog) {
        m_customDialog = new CustomDialog(this);
        m_customDialog->setAttribute(Qt::WA_DeleteOnClose);

        connect(m_customDialog, &CustomDialog::keymapRefreshRequested, this, [this]() {
            on_refreshGameScriptBtn_clicked();
            if (m_customDialog) {
                // Update the keymap list in the custom dialog
                QStringList keymaps;
                for (int i = 0; i < ui->gameBox->count(); ++i) {
                    QString text = ui->gameBox->itemText(i);
                    if (!text.isEmpty() && !text.startsWith(QString::fromUtf8("➕"))) {
                        keymaps.append(text);
                    }
                }
                m_customDialog->setKeymapList(keymaps, ui->gameBox->currentText());
            }
        });

        connect(m_customDialog, &CustomDialog::keymapApplyRequested, this, [this](const QString &name) {
            int idx = ui->gameBox->findText(name);
            if (idx >= 0) {
                ui->gameBox->setCurrentIndex(idx);
                on_applyScriptBtn_clicked();
            }
        });

        connect(m_customDialog, &CustomDialog::keymapEditRequested, this, &Dialog::on_keymapEditorBtn_clicked);

        connect(m_customDialog, &CustomDialog::keymapAddNewRequested, this, [this]() {
            createNewKeymap();
            // Refresh the custom dialog's keymap list
            if (m_customDialog) {
                QStringList keymaps;
                for (int i = 0; i < ui->gameBox->count(); ++i) {
                    QString text = ui->gameBox->itemText(i);
                    if (!text.isEmpty() && !text.startsWith(QString::fromUtf8("\xe2\x9e\x95"))) {
                        keymaps.append(text);
                    }
                }
                m_customDialog->setKeymapList(keymaps, ui->gameBox->currentText());
            }
        });

        connect(m_customDialog, &CustomDialog::preferencesChanged, this, &Dialog::syncPreferencesToLegacyUI);
    }

    // Apply current theme to dialog
    m_customDialog->setDarkTheme(m_isDarkTheme);

    // Populate preferences from legacy UI
    m_customDialog->setRecordScreen(ui->recordScreenCheck->isChecked());
    m_customDialog->setBackgroundRecord(ui->notDisplayCheck->isChecked());
    m_customDialog->setReverseConnection(ui->useReverseCheck->isChecked());
    m_customDialog->setShowFPS(ui->fpsCheck->isChecked());
    m_customDialog->setAlwaysOnTop(ui->alwaysTopCheck->isChecked());
    m_customDialog->setScreenOff(ui->closeScreenCheck->isChecked());
    m_customDialog->setFrameless(ui->framelessCheck->isChecked());
    m_customDialog->setStayAwake(ui->stayAwakeCheck->isChecked());
    m_customDialog->setShowToolbar(ui->showToolbar->isChecked());

    // Populate keymap list
    on_refreshGameScriptBtn_clicked();
    QStringList keymaps;
    for (int i = 0; i < ui->gameBox->count(); ++i) {
        QString text = ui->gameBox->itemText(i);
        if (!text.isEmpty() && !text.startsWith(QString::fromUtf8("➕"))) {
            keymaps.append(text);
        }
    }
    m_customDialog->setKeymapList(keymaps, ui->gameBox->currentText());

    m_customDialog->show();
    m_customDialog->raise();
    m_customDialog->activateWindow();
}

void Dialog::syncPreferencesToLegacyUI()
{
    if (!m_customDialog) return;

    // Block signals to prevent cascading updates
    ui->recordScreenCheck->setChecked(m_customDialog->recordScreen());
    ui->notDisplayCheck->setChecked(m_customDialog->backgroundRecord());
    ui->useReverseCheck->setChecked(m_customDialog->reverseConnection());
    ui->fpsCheck->setChecked(m_customDialog->showFPS());
    ui->alwaysTopCheck->setChecked(m_customDialog->alwaysOnTop());
    ui->closeScreenCheck->setChecked(m_customDialog->screenOff());
    ui->framelessCheck->setChecked(m_customDialog->frameless());
    ui->stayAwakeCheck->setChecked(m_customDialog->stayAwake());
    ui->showToolbar->setChecked(m_customDialog->showToolbar());
}

// =============================================================================
// Theme switching
// =============================================================================

void Dialog::applyTheme(bool isDark)
{
    m_isDarkTheme = isDark;

    if (isDark) {
        // Reload the dark theme from resources
        QFile file(":/qss/psblack.css");
        if (file.open(QFile::ReadOnly)) {
            QString qss = QLatin1String(file.readAll());
            QString paletteColor = qss.mid(20, 7);
            qApp->setPalette(QPalette(QColor(paletteColor)));
            qApp->setStyleSheet(qss);
            file.close();
        }
    } else {
        // Generate light theme by color-swapping the dark QSS
        QString lightQss = generateLightThemeQss();
        qApp->setPalette(QPalette(QColor("#F0F0F0")));
        qApp->setStyleSheet(lightQss);
    }

    // Update themed child widgets
    if (m_cleanWidget) {
        m_cleanWidget->setDarkTheme(isDark);
    }
    if (m_customDialog) {
        m_customDialog->setDarkTheme(isDark);
    }
}

QString Dialog::generateLightThemeQss()
{
    QFile file(":/qss/psblack.css");
    if (!file.open(QFile::ReadOnly)) return "";
    QString qss = QLatin1String(file.readAll());
    file.close();

    // Replace the palette background first (exact match in the first line)
    qss.replace("QPalette{background:#444444;}", "QPalette{background:#F0F0F0;}");

    // Swap dark hex colors for light equivalents (6-char codes to avoid partial matches)
    // Order matters: replace more specific patterns first
    qss.replace("#DCDCDC", "#333333");   // TextColor → dark text
    qss.replace("#444444", "#F0F0F0");   // PanelColor → light bg
    qss.replace("#242424", "#D0D0D0");   // BorderColor → light border
    qss.replace("#484848", "#FFFFFF");   // NormalColorStart → white
    qss.replace("#383838", "#F5F5F5");   // NormalColorEnd → off-white
    qss.replace("#646464", "#E8E8E8");   // DarkColorStart → light gray
    qss.replace("#525252", "#DEDEDE");   // DarkColorEnd → lighter gray
    qss.replace("#264F78", "#B3DDF2");   // Selection bg → light blue
    // Keep #00BB9E accent color unchanged

    return qss;
}
