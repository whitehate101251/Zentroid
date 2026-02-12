// #include <QDesktopWidget>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QShortcut>
#include <QStyle>
#include <QStyleOption>
#include <QTimer>
#include <QWindow>
#include <QtWidgets/QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>

#if defined(Q_OS_WIN32)
#include <Windows.h>
#endif

#include "config.h"
#include "iconhelper.h"
#include "qyuvopenglwidget.h"
#include "toolform.h"
#include "mousetap/mousetap.h"
#include "ui_videoform.h"
#include "videoform.h"
#include "keymapeditor/keymapoverlay.h"
#include "../util/keymappath.h"

VideoForm::VideoForm(bool framelessWindow, bool skin, bool showToolbar, QWidget *parent) : QWidget(parent), ui(new Ui::videoForm), m_skin(skin)
{
    ui->setupUi(this);
    initUI();
    installShortcut();
    updateShowSize(size());
    bool vertical = size().height() > size().width();
    this->show_toolbar = showToolbar;
    if (m_skin) {
        updateStyleSheet(vertical);
    }
    if (framelessWindow) {
        setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    }
}

VideoForm::~VideoForm()
{
    delete ui;
}

QPixmap VideoForm::getScreenshot() const
{
    if (m_videoWidget) {
        return m_videoWidget->grab();
    }
    return QPixmap();
}

void VideoForm::initUI()
{
    if (m_skin) {
        QPixmap phone;
        if (phone.load(":/res/phone.png")) {
            m_widthHeightRatio = 1.0f * phone.width() / phone.height();
        }

#ifndef Q_OS_OSX
        // removing title bar on macOS affects showFullScreen
        // remove title bar
        setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
        // construct shaped window based on image
        setAttribute(Qt::WA_TranslucentBackground);
#endif
    }

    m_videoWidget = new QYUVOpenGLWidget();
    m_videoWidget->hide();
    ui->keepRatioWidget->setWidget(m_videoWidget);
    ui->keepRatioWidget->setWidthHeightRatio(m_widthHeightRatio);

    m_fpsLabel = new QLabel(m_videoWidget);
    QFont ft;
    ft.setPointSize(15);
    ft.setWeight(QFont::Light);
    ft.setBold(true);
    m_fpsLabel->setFont(ft);
    m_fpsLabel->move(5, 15);
    m_fpsLabel->setMinimumWidth(100);
    m_fpsLabel->setStyleSheet(R"(QLabel {color: #00FF00;})");

    // Create overlay (hidden initially)
    m_keymapOverlay = new KeymapOverlay(m_videoWidget, m_serial);
    m_keymapOverlay->hide();

    // ── Overlay signal connections (always connected from creation) ──
    // Apply = save to disk + runtime reload
    connect(m_keymapOverlay, &KeymapOverlay::keymapApplied, this, [this](const QString &filePath) {
        qInfo() << "[Keymap] ⚡ keymapApplied received in VideoForm:" << filePath;
        m_activeKeymapPath = filePath;
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (device) {
            QFile f(filePath);
            if (f.open(QIODevice::ReadOnly)) {
                QString script = f.readAll();
                f.close();
                device->updateScript(script);
                qInfo() << "[Keymap] ⚡ Runtime reload complete, script length:" << script.length();

                // Update switch-key hint
                QJsonDocument doc = QJsonDocument::fromJson(script.toUtf8());
                if (doc.isObject()) {
                    QString switchKey = doc.object().value("switchKey").toString("Key_QuoteLeft");
                    QString displayKey = switchKey;
                    if (displayKey.startsWith("Key_")) displayKey = displayKey.mid(4);
                    if (displayKey == "QuoteLeft") displayKey = "` (backtick)";
                    m_keymapOverlay->setSwitchKeyHint(displayKey);
                }
            } else {
                qWarning() << "[Keymap] Failed to read file for apply:" << filePath;
            }
        } else {
            qInfo() << "[Keymap] No device connected — saved to disk, will apply on next server start";
        }
    });

    // Save = disk only (log it, no runtime reload)
    connect(m_keymapOverlay, &KeymapOverlay::keymapSaved, this, [this](const QString &filePath) {
        qInfo() << "[Keymap] 💾 keymapSaved received (disk only):" << filePath;
    });

    // Catch video widget resize so overlay always matches its actual geometry
    m_videoWidget->installEventFilter(this);

    setMouseTracking(true);
    m_videoWidget->setMouseTracking(true);
    ui->keepRatioWidget->setMouseTracking(true);
}

QRect VideoForm::getGrabCursorRect()
{
    QRect rc;
#if defined(Q_OS_WIN32)
    rc = QRect(ui->keepRatioWidget->mapToGlobal(m_videoWidget->pos()), m_videoWidget->size());
    // high dpi support
    rc.setTopLeft(rc.topLeft() * m_videoWidget->devicePixelRatioF());
    rc.setBottomRight(rc.bottomRight() * m_videoWidget->devicePixelRatioF());

    rc.setX(rc.x() + 10);
    rc.setY(rc.y() + 10);
    rc.setWidth(rc.width() - 20);
    rc.setHeight(rc.height() - 20);
#elif defined(Q_OS_OSX)
    rc = m_videoWidget->geometry();
    rc.setTopLeft(ui->keepRatioWidget->mapToGlobal(rc.topLeft()));
    rc.setBottomRight(ui->keepRatioWidget->mapToGlobal(rc.bottomRight()));

    rc.setX(rc.x() + 10);
    rc.setY(rc.y() + 10);
    rc.setWidth(rc.width() - 20);
    rc.setHeight(rc.height() - 20);
#elif defined(Q_OS_LINUX)
    rc = QRect(ui->keepRatioWidget->mapToGlobal(m_videoWidget->pos()), m_videoWidget->size());
    // high dpi support -- taken from the WIN32 section and untested
    rc.setTopLeft(rc.topLeft() * m_videoWidget->devicePixelRatioF());
    rc.setBottomRight(rc.bottomRight() * m_videoWidget->devicePixelRatioF());

    rc.setX(rc.x() + 10);
    rc.setY(rc.y() + 10);
    rc.setWidth(rc.width() - 20);
    rc.setHeight(rc.height() - 20);
#endif
    return rc;
}

const QSize &VideoForm::frameSize()
{
    return m_frameSize;
}

void VideoForm::resizeSquare()
{
    QRect screenRect = getScreenRect();
    if (screenRect.isEmpty()) {
        qWarning() << "getScreenRect is empty";
        return;
    }
    resize(screenRect.height(), screenRect.height());
}

void VideoForm::removeBlackRect()
{
    resize(ui->keepRatioWidget->goodSize());
}

void VideoForm::showFPS(bool show)
{
    if (!m_fpsLabel) {
        return;
    }
    m_fpsLabel->setVisible(show);
}

void VideoForm::updateRender(int width, int height, uint8_t* dataY, uint8_t* dataU, uint8_t* dataV, int linesizeY, int linesizeU, int linesizeV)
{
    if (m_videoWidget->isHidden()) {
        if (m_loadingWidget) {
            m_loadingWidget->close();
        }
        m_videoWidget->show();
        // Overlay sync handled by eventFilter on m_videoWidget (QEvent::Resize)
    }

    updateShowSize(QSize(width, height));
    m_videoWidget->setFrameSize(QSize(width, height));
    m_videoWidget->updateTextures(dataY, dataU, dataV, linesizeY, linesizeU, linesizeV);
}

void VideoForm::setSerial(const QString &serial)
{
    m_serial = serial;
}

void VideoForm::showToolForm(bool show)
{
    if (!m_toolForm) {
        m_toolForm = new ToolForm(this, ToolForm::AP_OUTSIDE_RIGHT);
        m_toolForm->setSerial(m_serial);
    }
    m_toolForm->move(pos().x() + geometry().width(), pos().y() + 30);
    m_toolForm->setVisible(show);
}

void VideoForm::moveCenter()
{
    QRect screenRect = getScreenRect();
    if (screenRect.isEmpty()) {
        qWarning() << "getScreenRect is empty";
        return;
    }
    // center window
    move(screenRect.center() - QRect(0, 0, size().width(), size().height()).center());
}

void VideoForm::installShortcut()
{
    QShortcut *shortcut = nullptr;

    // switchFullScreen
    shortcut = new QShortcut(QKeySequence("Ctrl+f"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        switchFullScreen();
    });

    // resizeSquare
    shortcut = new QShortcut(QKeySequence("Ctrl+g"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() { resizeSquare(); });

    // removeBlackRect
    shortcut = new QShortcut(QKeySequence("Ctrl+w"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() { removeBlackRect(); });

    // postGoHome
    shortcut = new QShortcut(QKeySequence("Ctrl+h"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        device->postGoHome();
    });

    // postGoBack
    shortcut = new QShortcut(QKeySequence("Ctrl+b"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        device->postGoBack();
    });

    // postAppSwitch
    shortcut = new QShortcut(QKeySequence("Ctrl+s"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->postAppSwitch();
    });

    // postGoMenu
    shortcut = new QShortcut(QKeySequence("Ctrl+m"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        device->postGoMenu();
    });

    // postVolumeUp
    shortcut = new QShortcut(QKeySequence("Ctrl+up"), this);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->postVolumeUp();
    });

    // postVolumeDown
    shortcut = new QShortcut(QKeySequence("Ctrl+down"), this);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->postVolumeDown();
    });

    // postPower
    shortcut = new QShortcut(QKeySequence("Ctrl+p"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->postPower();
    });

    shortcut = new QShortcut(QKeySequence("Ctrl+o"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->setDisplayPower(false);
    });

    // expandNotificationPanel
    shortcut = new QShortcut(QKeySequence("Ctrl+n"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->expandNotificationPanel();
    });

    // collapsePanel
    shortcut = new QShortcut(QKeySequence("Ctrl+Shift+n"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->collapsePanel();
    });

    // copy
    shortcut = new QShortcut(QKeySequence("Ctrl+c"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->postCopy();
    });

    // cut
    shortcut = new QShortcut(QKeySequence("Ctrl+x"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->postCut();
    });

    // clipboardPaste
    shortcut = new QShortcut(QKeySequence("Ctrl+v"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->setDeviceClipboard();
    });

    // setDeviceClipboard
    shortcut = new QShortcut(QKeySequence("Ctrl+Shift+v"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->clipboardPaste();
    });

    // Toggle keymap overlay edit mode (F12)
    shortcut = new QShortcut(QKeySequence("F12"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        toggleKeymapOverlayEdit();
    });

    // Toggle toolbar visibility (Ctrl+T)
    shortcut = new QShortcut(QKeySequence("Ctrl+t"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        if (m_toolForm) {
            m_toolForm->setVisible(!m_toolForm->isVisible());
        }
    });
}

QRect VideoForm::getScreenRect()
{
    QRect screenRect;
    QScreen *screen = QGuiApplication::primaryScreen();
    QWidget *win = window();
    if (win) {
        QWindow *winHandle = win->windowHandle();
        if (winHandle) {
            screen = winHandle->screen();
        }
    }

    if (screen) {
        screenRect = screen->availableGeometry();
    }
    return screenRect;
}

void VideoForm::updateStyleSheet(bool vertical)
{
    if (vertical) {
        setStyleSheet(R"(
                 #videoForm {
                     border-image: url(:/image/videoform/phone-v.png) 150px 65px 85px 65px;
                     border-width: 150px 65px 85px 65px;
                 }
                 )");
    } else {
        setStyleSheet(R"(
                 #videoForm {
                     border-image: url(:/image/videoform/phone-h.png) 65px 85px 65px 150px;
                     border-width: 65px 85px 65px 150px;
                 }
                 )");
    }
    layout()->setContentsMargins(getMargins(vertical));
}

QMargins VideoForm::getMargins(bool vertical)
{
    QMargins margins;
    if (vertical) {
        margins = QMargins(10, 68, 12, 62);
    } else {
        margins = QMargins(68, 12, 62, 10);
    }
    return margins;
}

void VideoForm::updateShowSize(const QSize &newSize)
{
    if (m_frameSize != newSize) {
        m_frameSize = newSize;

        m_widthHeightRatio = 1.0f * newSize.width() / newSize.height();
        ui->keepRatioWidget->setWidthHeightRatio(m_widthHeightRatio);

        bool vertical = m_widthHeightRatio < 1.0f ? true : false;
        QSize showSize = newSize;
        QRect screenRect = getScreenRect();
        if (screenRect.isEmpty()) {
            qWarning() << "getScreenRect is empty";
            return;
        }
        if (vertical) {
            showSize.setHeight(qMin(newSize.height(), screenRect.height() - 200));
            showSize.setWidth(showSize.height() * m_widthHeightRatio);
        } else {
            showSize.setWidth(qMin(newSize.width(), screenRect.width() / 2));
            showSize.setHeight(showSize.width() / m_widthHeightRatio);
        }

        if (isFullScreen() && qsc::IDeviceManage::getInstance().getDevice(m_serial)) {
            switchFullScreen();
        }

        if (isMaximized()) {
            showNormal();
        }

        if (m_skin) {
            QMargins m = getMargins(vertical);
            showSize.setWidth(showSize.width() + m.left() + m.right());
            showSize.setHeight(showSize.height() + m.top() + m.bottom());
        }

        if (showSize != size()) {
            resize(showSize);
            if (m_skin) {
                updateStyleSheet(vertical);
            }
            moveCenter();
        }
    }
}

void VideoForm::switchFullScreen()
{
    if (isFullScreen()) {
        // landscape fullscreen fills entire screen; restore keeps aspect ratio
        if (m_widthHeightRatio > 1.0f) {
            ui->keepRatioWidget->setWidthHeightRatio(m_widthHeightRatio);
        }

        showNormal();
        // back to normal size.
        resize(m_normalSize);
        // fullscreen window will move (0,0). qt bug?
        move(m_fullScreenBeforePos);

#ifdef Q_OS_OSX
        //setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
        //show();
#endif
        if (m_skin) {
            updateStyleSheet(m_frameSize.height() > m_frameSize.width());
        }
        showToolForm(this->show_toolbar);
#ifdef Q_OS_WIN32
        ::SetThreadExecutionState(ES_CONTINUOUS);
#endif
    } else {
        // landscape fullscreen fills entire screen, don't keep aspect ratio
        if (m_widthHeightRatio > 1.0f) {
            ui->keepRatioWidget->setWidthHeightRatio(-1.0f);
        }

        // record current size before fullscreen, it will be used to rollback size after exit fullscreen.
        m_normalSize = size();

        m_fullScreenBeforePos = pos();
        // this workaround of temporarily adding title bar before fullscreen causes mousemove events to be lost, breaking setmousetrack
        // mac fullscreen must show title bar
#ifdef Q_OS_OSX
        //setWindowFlags(windowFlags() & ~Qt::FramelessWindowHint);
#endif
        showToolForm(false);
        if (m_skin) {
            layout()->setContentsMargins(0, 0, 0, 0);
        }
        showFullScreen();

        // prevent computer sleep/screen off while fullscreen
#ifdef Q_OS_WIN32
        ::SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
#endif
    }
}

bool VideoForm::isHost()
{
    if (!m_toolForm) {
        return false;
    }
    return m_toolForm->isHost();
}

// ============================================================================
// Keymap Overlay
// ============================================================================

void VideoForm::showKeymapOverlay(const QString &keymapFilePath)
{
    if (!m_keymapOverlay) return;

    m_activeKeymapPath = keymapFilePath;
    m_keymapOverlay->loadKeymap(keymapFilePath);
    // Resize to current video widget size; the eventFilter will re-sync
    // if keepRatioWidget adjusts the geometry after this point.
    m_keymapOverlay->resize(m_videoWidget->size());
    m_keymapOverlay->show();
    m_keymapOverlay->raise();

    // Also load the keymap into the device so key bindings are actually active
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (device) {
        QFile f(keymapFilePath);
        if (f.open(QIODevice::ReadOnly)) {
            QString script = f.readAll();
            f.close();
            device->updateScript(script);
            qInfo() << "[Keymap] Loaded script from:" << keymapFilePath
                     << " length:" << script.length();

            // Parse switchKey from JSON to show user what activates gameplay
            QJsonDocument doc = QJsonDocument::fromJson(script.toUtf8());
            if (doc.isObject()) {
                QString switchKey = doc.object().value("switchKey").toString("Key_QuoteLeft");
                QString displayKey = switchKey;
                if (displayKey.startsWith("Key_")) displayKey = displayKey.mid(4);
                if (displayKey == "QuoteLeft") displayKey = "` (backtick)";
                qInfo() << "[Keymap] Switch key:" << switchKey;
                m_keymapOverlay->setSwitchKeyHint(displayKey);
            }
        }
    }

}

void VideoForm::hideKeymapOverlay()
{
    if (!m_keymapOverlay) return;
    m_keymapOverlay->setMode(KeymapOverlay::PlayMode);
    m_keymapOverlay->hide();
}

void VideoForm::toggleKeymapOverlayEdit()
{
    if (!m_keymapOverlay) return;

    if (!m_keymapOverlay->isVisible()) {
        // If no keymap is loaded yet, try to use the keymap that the device
        // is actually running (set via dialog dropdown / Apply button).
        // Only fall back to alphabetically first file as a last resort.
        if (m_keymapOverlay->currentFilePath().isEmpty()) {
            QString pathToLoad;

            // Prefer the keymap file the VideoForm was given at start
            if (!m_activeKeymapPath.isEmpty() && QFile::exists(m_activeKeymapPath)) {
                pathToLoad = m_activeKeymapPath;
            } else {
                // Last resort: pick the first .json in keymap dir
                QString keymapDir = getCanonicalKeymapDir();
                QDir dir(keymapDir);
                QStringList files = dir.entryList(QStringList() << "*.json",
                                                  QDir::Files, QDir::Name);
                if (!files.isEmpty())
                    pathToLoad = dir.absoluteFilePath(files.first());
            }
            if (!pathToLoad.isEmpty())
                showKeymapOverlay(pathToLoad);
        } else {
            // Re-show with the previously loaded keymap
            m_keymapOverlay->resize(m_videoWidget->size());
            m_keymapOverlay->show();
            m_keymapOverlay->raise();
        }
        return;
    }

    // If visible — cycle: PlayMode → EditMode → hide
    if (m_keymapOverlay->mode() == KeymapOverlay::PlayMode) {
        m_keymapOverlay->toggleMode(); // → EditMode
    } else {
        m_keymapOverlay->setMode(KeymapOverlay::PlayMode);
        m_keymapOverlay->hide();
    }
}

void VideoForm::updateFPS(quint32 fps)
{
    if (!m_fpsLabel) {
        return;
    }
    m_fpsLabel->setText(QString("FPS:%1").arg(fps));
}

void VideoForm::grabCursor(bool grab)
{
#if defined(Q_OS_LINUX)
    // On Linux (both Wayland and X11) use Qt's cross-platform widget grab
    // instead of the XCB-specific MouseTap which is broken on Wayland and
    // requires qt6-gui-private on X11.
    if (grab && m_videoWidget) {
        m_videoWidget->grabMouse();
        // Center the cursor inside the video widget so it doesn't start at an edge
        QPoint center = m_videoWidget->mapToGlobal(
            QPoint(m_videoWidget->width() / 2, m_videoWidget->height() / 2));
        QCursor::setPos(center);
        m_cursorGrabbed = true;
    } else if (m_videoWidget) {
        m_videoWidget->releaseMouse();
        m_cursorGrabbed = false;
    }
#else
    QRect rc = getGrabCursorRect();
    MouseTap::getInstance()->enableMouseEventTap(rc, grab);
#endif
}

void VideoForm::onFrame(int width, int height, uint8_t *dataY, uint8_t *dataU, uint8_t *dataV, int linesizeY, int linesizeU, int linesizeV)
{
    updateRender(width, height, dataY, dataU, dataV, linesizeY, linesizeU, linesizeV);
}

void VideoForm::staysOnTop(bool top)
{
    bool needShow = false;
    if (isVisible()) {
        needShow = true;
    }
    setWindowFlag(Qt::WindowStaysOnTopHint, top);
    if (m_toolForm) {
        m_toolForm->setWindowFlag(Qt::WindowStaysOnTopHint, top);
    }
    if (needShow) {
        show();
    }
}

void VideoForm::mousePressEvent(QMouseEvent *event)
{
    // If overlay is in edit mode, don't forward mouse to device
    if (m_keymapOverlay && m_keymapOverlay->isVisible()
        && m_keymapOverlay->mode() == KeymapOverlay::EditMode) {
        return;
    }

    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QPointF localPos = event->localPos();
    QPointF globalPos = event->globalPos();
#else
    QPointF localPos = event->position();
    QPointF globalPos = event->globalPosition();
#endif
    if (m_videoWidget->geometry().contains(event->pos())) {
        if (!device) {
            return;
        }
        if (event->button() == Qt::MiddleButton) {
            if (!device) {
                return;
            }
            emit device->postGoHome();
            return;
        }

        if (event->button() == Qt::RightButton && !device->isCurrentCustomKeymap()) {
            emit device->postGoBack();
            return;
        }

        // local check
        QPointF local = m_videoWidget->mapFrom(this, localPos.toPoint());
        QMouseEvent newEvent(event->type(), local, globalPos, event->button(), event->buttons(), event->modifiers());
        emit device->mouseEvent(&newEvent, m_videoWidget->frameSize(), m_videoWidget->size());

        // debug keymap pos
        if (event->button() == Qt::LeftButton) {
            qreal x = localPos.x() / m_videoWidget->size().width();
            qreal y = localPos.y() / m_videoWidget->size().height();
            QString posTip = QString(R"("pos": {"x": %1, "y": %2})").arg(x).arg(y);
            qInfo() << posTip.toStdString().c_str();
        }
    } else {
        if (event->button() == Qt::LeftButton) {
            m_dragPosition = globalPos.toPoint() - frameGeometry().topLeft();
            event->accept();
        }
    }
}

void VideoForm::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_keymapOverlay && m_keymapOverlay->isVisible()
        && m_keymapOverlay->mode() == KeymapOverlay::EditMode) {
        return;
    }

    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (m_dragPosition.isNull()) {
        if (!device) {
            return;
        }
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        QPointF localPos = event->localPos();
        QPointF globalPos = event->globalPos();
#else
        QPointF localPos = event->position();
        QPointF globalPos = event->globalPosition();
#endif
        // local check
        QPointF local = m_videoWidget->mapFrom(this, localPos.toPoint());
        if (local.x() < 0) {
            local.setX(0);
        }
        if (local.x() > m_videoWidget->width()) {
            local.setX(m_videoWidget->width());
        }
        if (local.y() < 0) {
            local.setY(0);
        }
        if (local.y() > m_videoWidget->height()) {
            local.setY(m_videoWidget->height());
        }
        QMouseEvent newEvent(event->type(), local, globalPos, event->button(), event->buttons(), event->modifiers());
        emit device->mouseEvent(&newEvent, m_videoWidget->frameSize(), m_videoWidget->size());
    } else {
        m_dragPosition = QPoint(0, 0);
    }
}

void VideoForm::mouseMoveEvent(QMouseEvent *event)
{
    if (m_keymapOverlay && m_keymapOverlay->isVisible()
        && m_keymapOverlay->mode() == KeymapOverlay::EditMode) {
        return;
    }

#if defined(Q_OS_LINUX)
    // Wayland fallback: grabMouse() captures events but doesn't confine the pointer.
    // Warp the cursor back into the video widget if it escapes while grabbed.
    if (m_cursorGrabbed && m_videoWidget) {
        QRect widgetRect = m_videoWidget->rect();
        QPoint localInWidget = m_videoWidget->mapFromGlobal(event->globalPosition().toPoint());
        if (!widgetRect.contains(localInWidget)) {
            QPoint center = m_videoWidget->mapToGlobal(
                QPoint(m_videoWidget->width() / 2, m_videoWidget->height() / 2));
            QCursor::setPos(center);
            return;
        }
    }
#endif

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        QPointF localPos = event->localPos();
        QPointF globalPos = event->globalPos();
#else
        QPointF localPos = event->position();
        QPointF globalPos = event->globalPosition();
#endif
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (m_videoWidget->geometry().contains(event->pos())) {
        if (!device) {
            return;
        }
        QPointF mappedPos = m_videoWidget->mapFrom(this, localPos.toPoint());
        QMouseEvent newEvent(event->type(), mappedPos, globalPos, event->button(), event->buttons(), event->modifiers());
        emit device->mouseEvent(&newEvent, m_videoWidget->frameSize(), m_videoWidget->size());
    } else if (!m_dragPosition.isNull()) {
        if (event->buttons() & Qt::LeftButton) {
            move(globalPos.toPoint() - m_dragPosition);
            event->accept();
        }
    }
}

void VideoForm::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (m_keymapOverlay && m_keymapOverlay->isVisible()
        && m_keymapOverlay->mode() == KeymapOverlay::EditMode) {
        return;
    }

    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (event->button() == Qt::LeftButton && !m_videoWidget->geometry().contains(event->pos())) {
        if (!isMaximized()) {
            removeBlackRect();
        }
    }

    if (event->button() == Qt::RightButton && device && !device->isCurrentCustomKeymap()) {
        emit device->postBackOrScreenOn(event->type() == QEvent::MouseButtonPress);
    }

    if (m_videoWidget->geometry().contains(event->pos())) {
        if (!device) {
            return;
        }
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        QPointF localPos = event->localPos();
        QPointF globalPos = event->globalPos();
#else
        QPointF localPos = event->position();
        QPointF globalPos = event->globalPosition();
#endif
        QPointF mappedPos = m_videoWidget->mapFrom(this, localPos.toPoint());
        QMouseEvent newEvent(event->type(), mappedPos, globalPos, event->button(), event->buttons(), event->modifiers());
        emit device->mouseEvent(&newEvent, m_videoWidget->frameSize(), m_videoWidget->size());
    }
}

void VideoForm::wheelEvent(QWheelEvent *event)
{
    if (m_keymapOverlay && m_keymapOverlay->isVisible()
        && m_keymapOverlay->mode() == KeymapOverlay::EditMode) {
        return;
    }

    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    if (m_videoWidget->geometry().contains(event->position().toPoint())) {
        if (!device) {
            return;
        }
        QPointF pos = m_videoWidget->mapFrom(this, event->position().toPoint());
        QWheelEvent wheelEvent(
            pos, event->globalPosition(), event->pixelDelta(), event->angleDelta(), event->buttons(), event->modifiers(), event->phase(), event->inverted());
#else
    if (m_videoWidget->geometry().contains(event->pos())) {
        if (!device) {
            return;
        }
        QPointF pos = m_videoWidget->mapFrom(this, event->pos());

        QWheelEvent wheelEvent(
            pos, event->globalPosF(), event->pixelDelta(), event->angleDelta(), event->delta(), event->orientation(),
            event->buttons(), event->modifiers(), event->phase(), event->source(), event->inverted());
#endif
        emit device->wheelEvent(&wheelEvent, m_videoWidget->frameSize(), m_videoWidget->size());
    }
}

void VideoForm::keyPressEvent(QKeyEvent *event)
{
    // F12 toggles overlay edit mode (handled by shortcut, but guard here too)
    if (event->key() == Qt::Key_F12 && !event->isAutoRepeat()) {
        return;
    }

    // If overlay is in edit mode, don't forward keys to device
    if (m_keymapOverlay && m_keymapOverlay->isVisible()
        && m_keymapOverlay->mode() == KeymapOverlay::EditMode) {
        return;
    }

    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    if (Qt::Key_Escape == event->key() && !event->isAutoRepeat() && isFullScreen()) {
        switchFullScreen();
    }

    emit device->keyEvent(event, m_videoWidget->frameSize(), m_videoWidget->size());
}

void VideoForm::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F12) return;

    // If overlay is in edit mode, don't forward keys to device
    if (m_keymapOverlay && m_keymapOverlay->isVisible()
        && m_keymapOverlay->mode() == KeymapOverlay::EditMode) {
        return;
    }

    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    emit device->keyEvent(event, m_videoWidget->frameSize(), m_videoWidget->size());
}

void VideoForm::paintEvent(QPaintEvent *paint)
{
    Q_UNUSED(paint)
    QStyleOption opt;
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    opt.init(this);
#else
    opt.initFrom(this);
#endif
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void VideoForm::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    if (!isFullScreen() && this->show_toolbar) {
        QTimer::singleShot(500, this, [this](){
            showToolForm(this->show_toolbar);
        });
    }
}

bool VideoForm::eventFilter(QObject *watched, QEvent *event)
{
    // When the video widget is resized (by keepRatioWidget::adjustSubWidget),
    // sync the overlay to match exactly.  This fires AFTER keepRatioWidget
    // has set the final geometry, so m_videoWidget->size() is authoritative.
    if (watched == m_videoWidget && event->type() == QEvent::Resize) {
        if (m_keymapOverlay) {
            m_keymapOverlay->resize(m_videoWidget->size());
            m_keymapOverlay->raise();
        }
    }
    return QWidget::eventFilter(watched, event);
}

void VideoForm::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    QSize goodSize = ui->keepRatioWidget->goodSize();
    if (goodSize.isEmpty()) {
        return;
    }
    QSize curSize = size();
    if (m_widthHeightRatio > 1.0f) {
        if (curSize.height() <= goodSize.height()) {
            setMinimumHeight(goodSize.height());
        } else {
            setMinimumHeight(0);
        }
    } else {
        if (curSize.width() <= goodSize.width()) {
            setMinimumWidth(goodSize.width());
        } else {
            setMinimumWidth(0);
        }
    }

    // Overlay sync is handled by eventFilter on m_videoWidget (QEvent::Resize)
}

void VideoForm::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    // Release cursor grab if active
    if (m_cursorGrabbed && m_videoWidget) {
        m_videoWidget->releaseMouse();
        m_cursorGrabbed = false;
    }
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    Config::getInstance().setRect(device->getSerial(), geometry());
    device->disconnectDevice();
}

void VideoForm::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void VideoForm::dragMoveEvent(QDragMoveEvent *event)
{
    Q_UNUSED(event)
}

void VideoForm::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event)
}

void VideoForm::dropEvent(QDropEvent *event)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    const QMimeData *qm = event->mimeData();
    QList<QUrl> urls = qm->urls();

    for (const QUrl &url : urls) {
        QString file = url.toLocalFile();
        QFileInfo fileInfo(file);

        if (!fileInfo.exists()) {
            QMessageBox::warning(this, "Zentroid", tr("file does not exist"), QMessageBox::Ok);
            continue;
        }

        if (fileInfo.isFile() && fileInfo.suffix() == "apk") {
            emit device->installApkRequest(file);
            continue;
        }
        emit device->pushFileRequest(file, Config::getInstance().getPushFilePath() + fileInfo.fileName());
    }
}