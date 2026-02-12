#include "cleanmodewidget.h"
#include "toggleswitch.h"
#include "../fontawesome/iconhelper.h"

#include <QApplication>
#include <QFontDatabase>
#include <QGraphicsDropShadowEffect>
#include <QPainter>

CleanModeWidget::CleanModeWidget(QWidget *parent)
    : QWidget(parent)
    , m_wifiPanelVisible(false)
    , m_usbPanelVisible(false)
    , m_isDark(true)
{
    setupUI();
    applyStyles();
}

void CleanModeWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 16, 24, 24);
    mainLayout->setSpacing(0);

    // ====== TOP BAR ======
    QHBoxLayout *topBar = new QHBoxLayout();
    topBar->setSpacing(8);

    m_titleLabel = new QLabel("Zentroid", this);
    m_titleLabel->setObjectName("cleanTitle");
    topBar->addWidget(m_titleLabel);

    topBar->addStretch();

    m_statusDot = new QLabel(this);
    m_statusDot->setObjectName("statusDot");
    m_statusDot->setFixedSize(10, 10);
    topBar->addWidget(m_statusDot, 0, Qt::AlignVCenter);

    topBar->addSpacing(6);

    // Theme toggle: [sun] [toggle] [moon]
    m_sunLabel = new QLabel(this);
    m_sunLabel->setObjectName("themeIconLabel");
    m_sunLabel->setFixedSize(16, 16);
    m_sunLabel->setAlignment(Qt::AlignCenter);
    IconHelper::Instance()->SetIcon(m_sunLabel, QChar(0xf185), 10);
    m_sunLabel->setStyleSheet("color: #666; background: transparent; border: none;");
    topBar->addWidget(m_sunLabel, 0, Qt::AlignVCenter);

    m_themeToggle = new ToggleSwitch(this);
    m_themeToggle->setChecked(true);  // default dark
    m_themeToggle->setToolTip("Toggle Dark/Light Theme");
    m_themeToggle->setFixedSize(42, 22);
    topBar->addWidget(m_themeToggle, 0, Qt::AlignVCenter);

    m_moonLabel = new QLabel(this);
    m_moonLabel->setObjectName("themeIconLabel");
    m_moonLabel->setFixedSize(16, 16);
    m_moonLabel->setAlignment(Qt::AlignCenter);
    IconHelper::Instance()->SetIcon(m_moonLabel, QChar(0xf186), 10);
    m_moonLabel->setStyleSheet("color: #C0C0FF; background: transparent; border: none;");
    topBar->addWidget(m_moonLabel, 0, Qt::AlignVCenter);

    topBar->addSpacing(10);

    // Legacy mode toggle
    m_legacyLabel = new QLabel("Legacy", this);
    m_legacyLabel->setObjectName("legacyLabel");
    topBar->addWidget(m_legacyLabel, 0, Qt::AlignVCenter);

    m_legacyToggle = new ToggleSwitch(this);
    m_legacyToggle->setChecked(false);
    m_legacyToggle->setToolTip("Switch to Legacy Mode");
    m_legacyToggle->setFixedSize(42, 22);
    topBar->addWidget(m_legacyToggle, 0, Qt::AlignVCenter);

    topBar->addSpacing(4);

    m_gearBtn = new QPushButton(this);
    m_gearBtn->setObjectName("gearBtn");
    m_gearBtn->setFixedSize(32, 32);
    m_gearBtn->setCursor(Qt::PointingHandCursor);
    IconHelper::Instance()->SetIcon(m_gearBtn, QChar(0xf013), 13);
    topBar->addWidget(m_gearBtn, 0, Qt::AlignVCenter);

    mainLayout->addLayout(topBar);

    // Separator
    QFrame *sep = new QFrame(this);
    sep->setObjectName("cleanSeparator");
    sep->setFrameShape(QFrame::HLine);
    sep->setFixedHeight(1);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(sep);
    mainLayout->addSpacing(10);

    // ====== CONTENT STACK ======
    m_contentStack = new QStackedWidget(this);

    // ------ PAGE 0: Disconnected ------
    m_disconnectedPage = new QWidget(this);
    QHBoxLayout *disHLayout = new QHBoxLayout(m_disconnectedPage);
    disHLayout->setContentsMargins(0, 0, 0, 0);
    disHLayout->addStretch(1);

    QWidget *disContainer = new QWidget(m_disconnectedPage);
    disContainer->setObjectName("disContainer");
    disContainer->setMaximumWidth(520);
    disContainer->setMinimumWidth(340);
    QVBoxLayout *disLayout = new QVBoxLayout(disContainer);
    disLayout->setContentsMargins(20, 0, 20, 0);
    disLayout->setSpacing(0);

    disLayout->addStretch(1);

    // WiFi Button (FontAwesome wifi icon 0xf1eb)
    m_wifiBtn = new QPushButton(this);
    m_wifiBtn->setObjectName("wifiConnectBtn");
    m_wifiBtn->setCursor(Qt::PointingHandCursor);
    m_wifiBtn->setMinimumHeight(60);
    m_wifiBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    {
        QHBoxLayout *wbl = new QHBoxLayout(m_wifiBtn);
        wbl->setContentsMargins(24, 16, 24, 16);
        wbl->setSpacing(12);
        QLabel *wifiIcon = new QLabel(m_wifiBtn);
        wifiIcon->setObjectName("btnIcon");
        IconHelper::Instance()->SetIcon(wifiIcon, QChar(0xf1eb), 20);
        wifiIcon->setFixedSize(24, 24);
        wifiIcon->setAlignment(Qt::AlignCenter);
        wbl->addWidget(wifiIcon);
        QLabel *wifiText = new QLabel("WiFi Connect", m_wifiBtn);
        wifiText->setObjectName("btnLabel");
        wbl->addWidget(wifiText);
        wbl->addStretch();
    }
    disLayout->addWidget(m_wifiBtn);

    // WiFi Panel (hidden initially)
    m_wifiPanel = new QWidget(this);
    m_wifiPanel->setObjectName("devicePanel");
    QVBoxLayout *wfpLayout = new QVBoxLayout(m_wifiPanel);
    wfpLayout->setContentsMargins(10, 8, 10, 8);
    wfpLayout->setSpacing(6);

    m_wifiDeviceList = new QListWidget(m_wifiPanel);
    m_wifiDeviceList->setObjectName("deviceList");
    m_wifiDeviceList->setMaximumHeight(90);
    m_wifiDeviceList->setSelectionMode(QAbstractItemView::SingleSelection);
    wfpLayout->addWidget(m_wifiDeviceList);

    QHBoxLayout *wfpBtns = new QHBoxLayout();
    wfpBtns->setSpacing(6);
    m_wifiRefreshBtn = new QPushButton(m_wifiPanel);
    m_wifiRefreshBtn->setObjectName("refreshBtn");
    m_wifiRefreshBtn->setFixedSize(36, 30);
    m_wifiRefreshBtn->setCursor(Qt::PointingHandCursor);
    m_wifiRefreshBtn->setToolTip("Refresh devices");
    IconHelper::Instance()->SetIcon(m_wifiRefreshBtn, QChar(0xf021), 11);

    m_wifiAutoSetupBtn = new QPushButton("Auto WiFi Setup", m_wifiPanel);
    m_wifiAutoSetupBtn->setObjectName("autoSetupBtn");
    m_wifiAutoSetupBtn->setCursor(Qt::PointingHandCursor);

    m_wifiConnectBtn = new QPushButton("Connect", m_wifiPanel);
    m_wifiConnectBtn->setObjectName("panelConnectBtn");
    m_wifiConnectBtn->setCursor(Qt::PointingHandCursor);
    m_wifiConnectBtn->setEnabled(false);

    wfpBtns->addWidget(m_wifiRefreshBtn);
    wfpBtns->addWidget(m_wifiAutoSetupBtn);
    wfpBtns->addStretch();
    wfpBtns->addWidget(m_wifiConnectBtn);
    wfpLayout->addLayout(wfpBtns);

    m_wifiPanel->hide();
    disLayout->addWidget(m_wifiPanel);

    disLayout->addSpacing(16);

    // USB Button (FontAwesome plug icon 0xf1e6)
    m_usbBtn = new QPushButton(this);
    m_usbBtn->setObjectName("usbConnectBtn");
    m_usbBtn->setCursor(Qt::PointingHandCursor);
    m_usbBtn->setMinimumHeight(60);
    m_usbBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    {
        QHBoxLayout *ubl = new QHBoxLayout(m_usbBtn);
        ubl->setContentsMargins(24, 16, 24, 16);
        ubl->setSpacing(12);
        QLabel *usbIcon = new QLabel(m_usbBtn);
        usbIcon->setObjectName("btnIcon");
        IconHelper::Instance()->SetIcon(usbIcon, QChar(0xf1e6), 20);
        usbIcon->setFixedSize(24, 24);
        usbIcon->setAlignment(Qt::AlignCenter);
        ubl->addWidget(usbIcon);
        QLabel *usbText = new QLabel("USB Connect", m_usbBtn);
        usbText->setObjectName("btnLabel");
        ubl->addWidget(usbText);
        ubl->addStretch();
    }
    disLayout->addWidget(m_usbBtn);

    // USB Panel (hidden initially)
    m_usbPanel = new QWidget(this);
    m_usbPanel->setObjectName("devicePanel");
    QVBoxLayout *usbpLayout = new QVBoxLayout(m_usbPanel);
    usbpLayout->setContentsMargins(10, 8, 10, 8);
    usbpLayout->setSpacing(6);

    m_usbDeviceList = new QListWidget(m_usbPanel);
    m_usbDeviceList->setObjectName("deviceList");
    m_usbDeviceList->setMaximumHeight(90);
    m_usbDeviceList->setSelectionMode(QAbstractItemView::SingleSelection);
    usbpLayout->addWidget(m_usbDeviceList);

    QHBoxLayout *usbpBtns = new QHBoxLayout();
    usbpBtns->setSpacing(6);
    m_usbRefreshBtn = new QPushButton(m_usbPanel);
    m_usbRefreshBtn->setObjectName("refreshBtn");
    m_usbRefreshBtn->setFixedSize(36, 30);
    m_usbRefreshBtn->setCursor(Qt::PointingHandCursor);
    m_usbRefreshBtn->setToolTip("Refresh devices");
    IconHelper::Instance()->SetIcon(m_usbRefreshBtn, QChar(0xf021), 11);

    m_usbConnectBtn = new QPushButton("Connect", m_usbPanel);
    m_usbConnectBtn->setObjectName("panelConnectBtn");
    m_usbConnectBtn->setCursor(Qt::PointingHandCursor);
    m_usbConnectBtn->setEnabled(false);

    usbpBtns->addWidget(m_usbRefreshBtn);
    usbpBtns->addStretch();
    usbpBtns->addWidget(m_usbConnectBtn);
    usbpLayout->addLayout(usbpBtns);

    m_usbPanel->hide();
    disLayout->addWidget(m_usbPanel);

    disLayout->addStretch(1);

    disHLayout->addWidget(disContainer);
    disHLayout->addStretch(1);

    m_contentStack->addWidget(m_disconnectedPage);  // index 0

    // ------ PAGE 1: Connected ------
    m_connectedPage = new QWidget(this);
    QHBoxLayout *conHLayout = new QHBoxLayout(m_connectedPage);
    conHLayout->setContentsMargins(0, 0, 0, 0);
    conHLayout->addStretch(1);

    QWidget *conContainer = new QWidget(m_connectedPage);
    conContainer->setObjectName("conContainer");
    conContainer->setMaximumWidth(520);
    conContainer->setMinimumWidth(340);
    QVBoxLayout *conLayout = new QVBoxLayout(conContainer);
    conLayout->setContentsMargins(20, 10, 20, 10);
    conLayout->setSpacing(16);

    conLayout->addStretch(1);

    m_connectedLabel = new QLabel("Connected to: ", this);
    m_connectedLabel->setObjectName("connectedLabel");
    m_connectedLabel->setAlignment(Qt::AlignCenter);
    m_connectedLabel->setWordWrap(true);
    conLayout->addWidget(m_connectedLabel);

    conLayout->addSpacing(8);

    m_startMirrorBtn = new QPushButton(QString::fromUtf8("▶  Start Mirroring"), this);
    m_startMirrorBtn->setObjectName("startMirrorBtn");
    m_startMirrorBtn->setCursor(Qt::PointingHandCursor);
    m_startMirrorBtn->setMinimumHeight(60);
    m_startMirrorBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    conLayout->addWidget(m_startMirrorBtn);

    m_disconnectBtn = new QPushButton("Disconnect", this);
    m_disconnectBtn->setObjectName("disconnectBtn");
    m_disconnectBtn->setCursor(Qt::PointingHandCursor);
    m_disconnectBtn->setMinimumHeight(52);
    m_disconnectBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    conLayout->addWidget(m_disconnectBtn);

    conLayout->addStretch(1);

    conHLayout->addWidget(conContainer);
    conHLayout->addStretch(1);

    m_contentStack->addWidget(m_connectedPage);  // index 1

    mainLayout->addWidget(m_contentStack, 1);

    // ====== GEAR MENU ======
    m_gearMenu = new QMenu(this);
    m_gearMenu->setObjectName("gearMenu");
    QAction *advAction = m_gearMenu->addAction("Advanced");
    QAction *cusAction = m_gearMenu->addAction("Custom");

    // ====== CONNECTIONS ======
    connect(m_legacyToggle, &QAbstractButton::toggled, this, &CleanModeWidget::legacyModeToggled);
    connect(m_themeToggle, &QAbstractButton::toggled, this, [this](bool isDark) {
        m_sunLabel->setStyleSheet(isDark
            ? "color: #666; background: transparent; border: none;"
            : "color: #FFB300; background: transparent; border: none;");
        m_moonLabel->setStyleSheet(!isDark
            ? "color: #666; background: transparent; border: none;"
            : "color: #C0C0FF; background: transparent; border: none;");
        emit themeToggled(isDark);
    });
    connect(m_gearBtn, &QPushButton::clicked, this, &CleanModeWidget::onGearClicked);
    connect(m_wifiBtn, &QPushButton::clicked, this, &CleanModeWidget::onWifiBtnClicked);
    connect(m_usbBtn, &QPushButton::clicked, this, &CleanModeWidget::onUsbBtnClicked);

    connect(m_wifiDeviceList, &QListWidget::itemClicked, this, [this](QListWidgetItem *) {
        m_wifiConnectBtn->setEnabled(m_wifiDeviceList->currentItem() != nullptr);
    });
    connect(m_usbDeviceList, &QListWidget::itemClicked, this, [this](QListWidgetItem *) {
        m_usbConnectBtn->setEnabled(m_usbDeviceList->currentItem() != nullptr);
    });

    connect(m_wifiRefreshBtn, &QPushButton::clicked, this, &CleanModeWidget::refreshDevicesRequested);
    connect(m_usbRefreshBtn, &QPushButton::clicked, this, &CleanModeWidget::refreshDevicesRequested);

    connect(m_wifiConnectBtn, &QPushButton::clicked, this, &CleanModeWidget::onWifiConnectClicked);
    connect(m_usbConnectBtn, &QPushButton::clicked, this, &CleanModeWidget::onUsbConnectClicked);
    connect(m_wifiAutoSetupBtn, &QPushButton::clicked, this, &CleanModeWidget::autoWifiSetupRequested);

    connect(m_startMirrorBtn, &QPushButton::clicked, this, &CleanModeWidget::startMirroringRequested);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &CleanModeWidget::disconnectRequested);

    connect(advAction, &QAction::triggered, this, &CleanModeWidget::advancedSettingsRequested);
    connect(cusAction, &QAction::triggered, this, &CleanModeWidget::customSettingsRequested);

    // Initial state
    m_contentStack->setCurrentIndex(0);
    setStatusConnected(false);
}

void CleanModeWidget::applyStyles()
{
    // Theme-aware clean mode styling using @@placeholder tokens
    QString css = R"(
        #cleanTitle {
            font-size: 20px;
            font-weight: bold;
            color: @@textPri;
            background: transparent;
            border: none;
            letter-spacing: 0.5px;
        }
        #statusDot {
            border-radius: 5px;
            min-width: 10px; max-width: 10px;
            min-height: 10px; max-height: 10px;
        }
        #legacyLabel {
            color: @@textSec;
            font-size: 11px;
            font-weight: bold;
            background: transparent;
            border: none;
            letter-spacing: 0.3px;
        }
        #themeIconLabel {
            background: transparent;
            border: none;
        }
        #gearBtn {
            background: transparent;
            border: 1px solid @@border;
            border-radius: 16px;
            color: @@textMuted;
        }
        #gearBtn:hover {
            background: @@hoverBg;
            border-color: #00BB9E;
            color: @@textBright;
        }
        #cleanSeparator {
            background: @@sep;
            border: none;
        }

        /* Centered containers */
        #disContainer, #conContainer {
            background: transparent;
            border: none;
        }

        /* Button icon & label (theme-aware) */
        #btnIcon {
            color: @@textPri;
            background: transparent;
            border: none;
        }
        #btnLabel {
            color: @@textPri;
            font-size: 16px;
            font-weight: bold;
            background: transparent;
            border: none;
            letter-spacing: 0.3px;
        }

        /* Big connect buttons */
        #wifiConnectBtn, #usbConnectBtn {
            padding: 0px;
            min-height: 60px;
            border-radius: 14px;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 @@btnStart, stop:1 @@btnEnd);
            border: 1px solid @@border;
        }
        #wifiConnectBtn:hover, #usbConnectBtn:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 @@btnHovStart, stop:1 @@btnHovEnd);
            border: 1px solid #00BB9E;
        }
        #wifiConnectBtn:pressed, #usbConnectBtn:pressed {
            background: @@pressedBg;
            border: 1px solid #00BB9E;
        }

        /* Device panels */
        #devicePanel {
            background: @@panelBg;
            border: 1px solid @@panelBorder;
            border-radius: 8px;
        }
        #deviceList {
            background: @@listBg;
            border: 1px solid @@sep;
            border-radius: 4px;
            color: @@listText;
            font-size: 13px;
        }
        #deviceList::item {
            padding: 4px 8px;
            border-radius: 3px;
        }
        #deviceList::item:selected {
            background: #00BB9E;
            color: #FFF;
        }
        #deviceList::item:hover {
            background: @@listHov;
        }

        /* Panel buttons */
        #refreshBtn {
            background: @@actionBg;
            border: 1px solid @@border;
            border-radius: 6px;
            color: @@textMuted;
        }
        #refreshBtn:hover {
            background: @@hoverBg;
            border-color: #00BB9E;
        }
        #autoSetupBtn {
            background: @@actionBg;
            border: 1px solid @@border;
            border-radius: 6px;
            padding: 4px 10px;
            font-size: 12px;
            color: @@textMuted;
        }
        #autoSetupBtn:hover {
            background: @@hoverBg;
            border-color: #00BB9E;
            color: @@textBright;
        }
        #panelConnectBtn {
            background: #00BB9E;
            border: none;
            border-radius: 6px;
            padding: 5px 16px;
            font-size: 13px;
            font-weight: bold;
            color: #FFF;
        }
        #panelConnectBtn:hover {
            background: #00D4B1;
        }
        #panelConnectBtn:disabled {
            background: @@disabledBg;
            color: @@disabledText;
        }

        /* Connected state */
        #connectedLabel {
            font-size: 15px;
            color: @@textMuted;
            background: transparent;
            border: none;
        }
        #startMirrorBtn {
            font-size: 16px;
            font-weight: bold;
            min-height: 60px;
            padding: 16px 32px;
            border-radius: 14px;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #00CC9E, stop:1 #00AA88);
            border: none;
            color: #FFF;
        }
        #startMirrorBtn:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #00E0B0, stop:1 #00BB9E);
        }
        #startMirrorBtn:pressed {
            background: #009977;
        }
        #disconnectBtn {
            font-size: 15px;
            font-weight: bold;
            min-height: 52px;
            padding: 14px 32px;
            border-radius: 12px;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #c0392b, stop:1 #a93226);
            border: none;
            color: #FFF;
        }
        #disconnectBtn:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #e74c3c, stop:1 #c0392b);
        }
        #disconnectBtn:pressed {
            background: #922b21;
        }

        /* Gear menu */
        #gearMenu {
            background: @@menuBg;
            border: 1px solid @@border;
            border-radius: 6px;
            padding: 4px;
        }
        #gearMenu::item {
            padding: 6px 24px;
            border-radius: 4px;
            color: @@listText;
        }
        #gearMenu::item:selected {
            background: #00BB9E;
            color: #FFF;
        }
    )";

    // Apply theme-specific color tokens
    if (m_isDark) {
        css.replace("@@textPri",     "#E0E0E0");
        css.replace("@@textSec",     "#AAAAAA");
        css.replace("@@textMuted",   "#CCCCCC");
        css.replace("@@textBright",  "#FFFFFF");
        css.replace("@@listText",    "#DDDDDD");
        css.replace("@@border",      "#555555");
        css.replace("@@sep",         "#444444");
        css.replace("@@panelBg",     "#333333");
        css.replace("@@panelBorder", "#4a4a4a");
        css.replace("@@listBg",      "#2b2b2b");
        css.replace("@@listHov",     "#404040");
        css.replace("@@btnStart",    "#3e3e3e");
        css.replace("@@btnEnd",      "#333333");
        css.replace("@@btnHovStart", "#4a4a4a");
        css.replace("@@btnHovEnd",   "#3e3e3e");
        css.replace("@@pressedBg",   "#2a2a2a");
        css.replace("@@actionBg",    "#3a3a3a");
        css.replace("@@hoverBg",     "#484848");
        css.replace("@@menuBg",      "#3a3a3a");
        css.replace("@@disabledBg",  "#555555");
        css.replace("@@disabledText","#888888");
    } else {
        css.replace("@@textPri",     "#333333");
        css.replace("@@textSec",     "#666666");
        css.replace("@@textMuted",   "#555555");
        css.replace("@@textBright",  "#111111");
        css.replace("@@listText",    "#333333");
        css.replace("@@border",      "#CCCCCC");
        css.replace("@@sep",         "#DDDDDD");
        css.replace("@@panelBg",     "#FFFFFF");
        css.replace("@@panelBorder", "#DDDDDD");
        css.replace("@@listBg",      "#FAFAFA");
        css.replace("@@listHov",     "#F0F0F0");
        css.replace("@@btnStart",    "#F5F5F5");
        css.replace("@@btnEnd",      "#EBEBEB");
        css.replace("@@btnHovStart", "#EEEEEE");
        css.replace("@@btnHovEnd",   "#E4E4E4");
        css.replace("@@pressedBg",   "#E0E0E0");
        css.replace("@@actionBg",    "#F0F0F0");
        css.replace("@@hoverBg",     "#E8E8E8");
        css.replace("@@menuBg",      "#FFFFFF");
        css.replace("@@disabledBg",  "#CCCCCC");
        css.replace("@@disabledText","#999999");
    }

    setStyleSheet(css);
}

// ===== Slot implementations =====

void CleanModeWidget::onWifiBtnClicked()
{
    m_wifiPanelVisible = !m_wifiPanelVisible;
    m_wifiPanel->setVisible(m_wifiPanelVisible);

    // Hide USB panel when WiFi is toggled
    if (m_wifiPanelVisible && m_usbPanelVisible) {
        m_usbPanelVisible = false;
        m_usbPanel->hide();
    }

    if (m_wifiPanelVisible) {
        emit refreshDevicesRequested();
    }
}

void CleanModeWidget::onUsbBtnClicked()
{
    m_usbPanelVisible = !m_usbPanelVisible;
    m_usbPanel->setVisible(m_usbPanelVisible);

    // Hide WiFi panel when USB is toggled
    if (m_usbPanelVisible && m_wifiPanelVisible) {
        m_wifiPanelVisible = false;
        m_wifiPanel->hide();
    }

    if (m_usbPanelVisible) {
        emit refreshDevicesRequested();
    }
}

void CleanModeWidget::onGearClicked()
{
    QPoint pos = m_gearBtn->mapToGlobal(QPoint(0, m_gearBtn->height()));
    m_gearMenu->popup(pos);
}

void CleanModeWidget::onWifiConnectClicked()
{
    QListWidgetItem *item = m_wifiDeviceList->currentItem();
    if (item) {
        QString serial = item->data(Qt::UserRole).toString();
        if (serial.isEmpty()) serial = item->text();
        emit connectToDevice(serial, true);
    }
}

void CleanModeWidget::onUsbConnectClicked()
{
    QListWidgetItem *item = m_usbDeviceList->currentItem();
    if (item) {
        QString serial = item->data(Qt::UserRole).toString();
        if (serial.isEmpty()) serial = item->text();
        emit connectToDevice(serial, false);
    }
}

// ===== Public methods =====

void CleanModeWidget::updateDeviceLists(const QStringList &wifiDevices, const QStringList &usbDevices)
{
    // WiFi devices
    m_wifiDeviceList->clear();
    for (const QString &dev : wifiDevices) {
        QListWidgetItem *item = new QListWidgetItem(dev, m_wifiDeviceList);
        item->setData(Qt::UserRole, dev);
    }
    if (wifiDevices.isEmpty()) {
        QListWidgetItem *item = new QListWidgetItem("No WiFi devices found", m_wifiDeviceList);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled);
        item->setForeground(QColor(120, 120, 120));
    }
    m_wifiConnectBtn->setEnabled(false);

    // USB devices
    m_usbDeviceList->clear();
    for (const QString &dev : usbDevices) {
        QListWidgetItem *item = new QListWidgetItem(dev, m_usbDeviceList);
        item->setData(Qt::UserRole, dev);
    }
    if (usbDevices.isEmpty()) {
        QListWidgetItem *item = new QListWidgetItem("No USB devices found", m_usbDeviceList);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled);
        item->setForeground(QColor(120, 120, 120));
    }
    m_usbConnectBtn->setEnabled(false);
}

void CleanModeWidget::showConnectedState(const QString &serial, const QString &connectionType)
{
    m_connectedSerial = serial;
    m_connectedLabel->setText(QString("Connected to: <b>%1</b> (%2)").arg(serial, connectionType));
    m_contentStack->setCurrentIndex(1);
    setStatusConnected(true);
}

void CleanModeWidget::showDisconnectedState()
{
    m_connectedSerial.clear();
    m_contentStack->setCurrentIndex(0);
    setStatusConnected(false);

    // Reset panels
    m_wifiPanelVisible = false;
    m_usbPanelVisible = false;
    m_wifiPanel->hide();
    m_usbPanel->hide();
    m_wifiConnectBtn->setEnabled(false);
    m_usbConnectBtn->setEnabled(false);
}

void CleanModeWidget::setStatusConnected(bool connected)
{
    if (connected) {
        m_statusDot->setStyleSheet(
            "background: #00CC88; border-radius: 5px;"
            "min-width:10px; max-width:10px; min-height:10px; max-height:10px;"
        );
    } else {
        m_statusDot->setStyleSheet(
            "background: #CC3333; border-radius: 5px;"
            "min-width:10px; max-width:10px; min-height:10px; max-height:10px;"
        );
    }
}

// ===== Theme support =====

void CleanModeWidget::setThemeChecked(bool isDark)
{
    m_themeToggle->blockSignals(true);
    m_themeToggle->setChecked(isDark);
    m_themeToggle->blockSignals(false);
}

void CleanModeWidget::setDarkTheme(bool isDark)
{
    m_isDark = isDark;
    applyStyles();

    // Update sun/moon icon emphasis
    m_sunLabel->setStyleSheet(isDark
        ? "color: #666; background: transparent; border: none;"
        : "color: #FFB300; background: transparent; border: none;");
    m_moonLabel->setStyleSheet(!isDark
        ? "color: #666; background: transparent; border: none;"
        : "color: #C0C0FF; background: transparent; border: none;");
}
