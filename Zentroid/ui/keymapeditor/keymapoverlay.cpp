#include "keymapoverlay.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QApplication>
#include <QCoreApplication>
#include <QStyle>
#include <QMessageBox>
#include <QtMath>
#include <QSet>
#include <QMetaEnum>
#include "../../util/keymappath.h"

// ============================================================================
// Construction / Destruction
// ============================================================================

KeymapOverlay::KeymapOverlay(QWidget *parent, const QString &serial)
    : QWidget(parent)
    , m_serial(serial)
{
    // Make overlay transparent
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setMouseTracking(true);

    // Start in play mode — transparent to events
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setFocusPolicy(Qt::NoFocus);

    // Highlight repaint timer (throttle repaints during key hold)
    m_highlightTimer = new QTimer(this);
    m_highlightTimer->setInterval(50);
    connect(m_highlightTimer, &QTimer::timeout, this, [this]() {
        update();
    });

    // File watcher for hot-reload
    m_fileWatcher = new QFileSystemWatcher(this);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &path) {
        Q_UNUSED(path)
        // Small delay to let the writer finish
        QTimer::singleShot(200, this, &KeymapOverlay::reloadKeymap);
    });

    // Build the edit-mode toolbar (hidden initially)
    initEditToolbar();
}

KeymapOverlay::~KeymapOverlay() = default;

// ============================================================================
// Mode Management
// ============================================================================

void KeymapOverlay::setMode(OverlayMode mode)
{
    if (m_mode == mode) return;
    m_mode = mode;

    if (m_mode == PlayMode) {
        // Transparent to mouse — game gets events
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setFocusPolicy(Qt::NoFocus);
        clearFocus();
        if (m_editBar) m_editBar->hide();
        setCursor(Qt::ArrowCursor);

        // Deselect all and cancel key capture
        m_keyCaptureNodeIndex = -1;
        for (auto &n : m_nodes) n.selected = false;
    } else {
        // Edit mode — capture mouse
        setAttribute(Qt::WA_TransparentForMouseEvents, false);
        setFocusPolicy(Qt::StrongFocus);
        setFocus();
        if (m_editBar) m_editBar->show();
        populateProfileCombo();
        setCursor(Qt::CrossCursor);
    }

    emit modeChanged(m_mode);
    update();
}

void KeymapOverlay::toggleMode()
{
    setMode(m_mode == PlayMode ? EditMode : PlayMode);
}

// ============================================================================
// Overlay Visibility
// ============================================================================

void KeymapOverlay::setOverlayVisible(bool visible)
{
    m_overlayVisible = visible;
    update();
}

// ============================================================================
// Keymap Loading / Saving
// ============================================================================

bool KeymapOverlay::loadKeymap(const QString &filePath)
{
    qInfo() << "[Keymap] Overlay: Loading keymap from:" << filePath;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject())
        return false;

    m_filePath = filePath;
    bool ok = parseJsonToNodes(doc.object());
    watchFile(filePath);
    showToast("Loaded: " + filePath, 3000);
    update();
    return ok;
}

void KeymapOverlay::reloadKeymap()
{
    if (m_filePath.isEmpty()) return;
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) return;

    parseJsonToNodes(doc.object());

    // Re-register watcher (some systems remove after signal)
    watchFile(m_filePath);
    update();
}

bool KeymapOverlay::saveKeymap()
{
    if (m_filePath.isEmpty()) {
        showToast("✗ No file loaded — cannot save", 3000);
        return false;
    }

    QJsonObject json = generateJsonFromNodes();
    QJsonDocument doc(json);

    // Temporarily disconnect watcher to avoid self-trigger
    if (m_fileWatcher)
        m_fileWatcher->removePath(m_filePath);

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "[Keymap] Failed to open for writing:" << m_filePath << file.errorString();
        showToast("✗ Save failed: " + file.errorString(), 3000);
        watchFile(m_filePath);
        return false;
    }
    qint64 written = file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    if (written < 0) {
        qWarning() << "[Keymap] Write failed:" << m_filePath;
        showToast("✗ Write failed", 3000);
        watchFile(m_filePath);
        return false;
    }

    qInfo() << "[Keymap] Overlay: Saved keymap to:" << m_filePath << "(" << written << "bytes)";

    // Re-add watcher
    watchFile(m_filePath);

    emit keymapSaved(m_filePath);
    showToast("💾 Saved: " + m_filePath);
    return true;
}

bool KeymapOverlay::applyKeymap()
{
    if (!saveKeymap())
        return false;

    emit keymapApplied(m_filePath);
    showToast("⚡ Applied: " + m_filePath);
    return true;
}

void KeymapOverlay::watchFile(const QString &filePath)
{
    if (!m_fileWatcher) return;
    // Remove all existing watched files
    auto files = m_fileWatcher->files();
    if (!files.isEmpty())
        m_fileWatcher->removePaths(files);
    if (!filePath.isEmpty() && QFile::exists(filePath))
        m_fileWatcher->addPath(filePath);
}

// ============================================================================
// JSON ↔ OverlayNode
// ============================================================================

bool KeymapOverlay::parseJsonToNodes(const QJsonObject &json)
{
    m_nodes.clear();

    if (json.contains("switchKey"))
        m_switchKey = json["switchKey"].toString();
    else
        m_switchKey = "Key_QuoteLeft";

    m_hasMouseMoveMap = json.contains("mouseMoveMap");
    if (m_hasMouseMoveMap)
        m_mouseMoveMap = json["mouseMoveMap"].toObject();

    if (!json.contains("keyMapNodes"))
        return false;

    QJsonArray arr = json["keyMapNodes"].toArray();
    for (const QJsonValue &val : arr) {
        QJsonObject obj = val.toObject();
        QString typeStr = obj["type"].toString();

        OverlayNode node;

        // Determine type
        if (typeStr == "KMT_CLICK")
            node.type = OverlayNode::Click;
        else if (typeStr == "KMT_CLICK_TWICE")
            node.type = OverlayNode::ClickTwice;
        else if (typeStr == "KMT_DRAG")
            node.type = OverlayNode::Drag;
        else if (typeStr == "KMT_STEER_WHEEL")
            node.type = OverlayNode::SteerWheel;
        else if (typeStr == "KMT_CLICK_MULTI")
            node.type = OverlayNode::ClickMulti;
        else if (typeStr == "KMT_GESTURE")
            node.type = OverlayNode::Gesture;
        else
            node.type = OverlayNode::Click; // fallback

        // Common fields
        node.keyCode = obj["key"].toString();
        node.comment = obj["comment"].toString();

        // switchMap (for KMT_CLICK)
        node.switchMap = obj["switchMap"].toBool(false);

        // Position
        if (typeStr == "KMT_STEER_WHEEL") {
            QJsonObject ctr = obj["centerPos"].toObject();
            node.relativePos = QPointF(ctr["x"].toDouble(0.5), ctr["y"].toDouble(0.5));
            node.leftKey  = obj["leftKey"].toString();
            node.rightKey = obj["rightKey"].toString();
            node.upKey    = obj["upKey"].toString();
            node.downKey  = obj["downKey"].toString();
            node.leftOff  = obj["leftOffset"].toDouble(0.05);
            node.rightOff = obj["rightOffset"].toDouble(0.05);
            node.upOff    = obj["upOffset"].toDouble(0.05);
            node.downOff  = obj["downOffset"].toDouble(0.05);
        } else if (typeStr == "KMT_DRAG") {
            QJsonObject sp = obj["startPos"].toObject();
            QJsonObject ep = obj["endPos"].toObject();
            node.relativePos = QPointF(sp["x"].toDouble(0.5), sp["y"].toDouble(0.5));
            node.dragEndPos  = QPointF(ep["x"].toDouble(0.5), ep["y"].toDouble(0.5));
        } else if (typeStr == "KMT_GESTURE") {
            QJsonObject pos = obj["pos"].toObject();
            node.relativePos = QPointF(pos["x"].toDouble(0.5), pos["y"].toDouble(0.5));
            node.gestureType = obj["gestureType"].toInt(0);
        } else {
            QJsonObject pos = obj["pos"].toObject();
            node.relativePos = QPointF(pos["x"].toDouble(0.5), pos["y"].toDouble(0.5));
        }

        m_nodes.append(node);
    }

    return true;
}

QJsonObject KeymapOverlay::generateJsonFromNodes() const
{
    QJsonObject json;
    json["switchKey"] = m_switchKey.isEmpty() ? "Key_QuoteLeft" : m_switchKey;
    if (m_hasMouseMoveMap)
        json["mouseMoveMap"] = m_mouseMoveMap;

    QJsonArray arr;
    for (const OverlayNode &node : m_nodes) {
        QJsonObject obj;
        switch (node.type) {
        case OverlayNode::Click:
            obj["type"] = "KMT_CLICK";
            obj["key"] = node.keyCode;
            { QJsonObject p; p["x"] = node.relativePos.x(); p["y"] = node.relativePos.y(); obj["pos"] = p; }
            obj["switchMap"] = node.switchMap;
            break;
        case OverlayNode::ClickTwice:
            obj["type"] = "KMT_CLICK_TWICE";
            obj["key"] = node.keyCode;
            { QJsonObject p; p["x"] = node.relativePos.x(); p["y"] = node.relativePos.y(); obj["pos"] = p; }
            break;
        case OverlayNode::Drag:
            obj["type"] = "KMT_DRAG";
            obj["key"] = node.keyCode;
            { QJsonObject sp; sp["x"] = node.relativePos.x(); sp["y"] = node.relativePos.y(); obj["startPos"] = sp; }
            { QJsonObject ep; ep["x"] = node.dragEndPos.x();  ep["y"] = node.dragEndPos.y();  obj["endPos"] = ep; }
            break;
        case OverlayNode::SteerWheel:
            obj["type"] = "KMT_STEER_WHEEL";
            { QJsonObject cp; cp["x"] = node.relativePos.x(); cp["y"] = node.relativePos.y(); obj["centerPos"] = cp; }
            obj["leftKey"]  = node.leftKey;
            obj["rightKey"] = node.rightKey;
            obj["upKey"]    = node.upKey;
            obj["downKey"]  = node.downKey;
            obj["leftOffset"]  = node.leftOff;
            obj["rightOffset"] = node.rightOff;
            obj["upOffset"]    = node.upOff;
            obj["downOffset"]  = node.downOff;
            break;
        case OverlayNode::ClickMulti:
            obj["type"] = "KMT_CLICK_MULTI";
            obj["key"] = node.keyCode;
            { QJsonObject p; p["x"] = node.relativePos.x(); p["y"] = node.relativePos.y(); obj["pos"] = p; }
            break;
        case OverlayNode::Gesture:
            obj["type"] = "KMT_GESTURE";
            obj["key"] = node.keyCode;
            { QJsonObject p; p["x"] = node.relativePos.x(); p["y"] = node.relativePos.y(); obj["pos"] = p; }
            obj["gestureType"] = node.gestureType;
            break;
        }
        if (!node.comment.isEmpty())
            obj["comment"] = node.comment;
        arr.append(obj);
    }
    json["keyMapNodes"] = arr;
    return json;
}

// ============================================================================
// Coordinate Conversion
// ============================================================================

QPointF KeymapOverlay::relativeToWidget(const QPointF &rel) const
{
    return QPointF(rel.x() * width(), rel.y() * height());
}

QPointF KeymapOverlay::widgetToRelative(const QPointF &widgetPos) const
{
    if (width() <= 0 || height() <= 0) return QPointF(0.5, 0.5);
    return QPointF(
        qBound(0.0, widgetPos.x() / width(), 1.0),
        qBound(0.0, widgetPos.y() / height(), 1.0)
    );
}

// ============================================================================
// Geometry Helpers
// ============================================================================

QRectF KeymapOverlay::nodeRect(const OverlayNode &node) const
{
    QPointF center = relativeToWidget(node.relativePos);
    qreal sz = BASE_NODE_SIZE * m_nodeScale;
    if (node.type == OverlayNode::SteerWheel)
        sz *= 2.0;
    return QRectF(center.x() - sz / 2.0, center.y() - sz / 2.0, sz, sz);
}

int KeymapOverlay::hitTestNode(const QPointF &widgetPos) const
{
    // Reverse iterate so top-drawn nodes are hit first
    for (int i = m_nodes.size() - 1; i >= 0; --i) {
        if (nodeRect(m_nodes[i]).contains(widgetPos))
            return i;
    }
    return -1;
}

// ============================================================================
// Painting
// ============================================================================

void KeymapOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    if (!m_overlayVisible) return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // In edit mode, draw a very subtle tinted background
    if (m_mode == EditMode) {
        p.fillRect(rect(), QColor(0, 0, 0, 30));
    }

    // Paint all nodes
    for (int i = 0; i < m_nodes.size(); ++i) {
        const OverlayNode &node = m_nodes[i];
        QRectF r = nodeRect(node);
        paintNode(p, node, r);
    }

    // Edit mode indicator
    if (m_mode == EditMode) {
        p.setPen(QColor(0, 200, 255, 180));
        p.setBrush(Qt::NoBrush);
        p.drawRect(rect().adjusted(1, 1, -1, -1));

        // Mode label at bottom
        QFont f = p.font();
        f.setPixelSize(12);
        f.setBold(true);
        p.setFont(f);
        p.setPen(QColor(0, 200, 255, 200));
        QString editHint = "EDIT MODE — Double-click node to assign key • Right-click for options • F12 to exit";
        p.drawText(QRectF(0, height() - 24, width(), 24), Qt::AlignCenter, editHint);

        // Key-capture indicator
        if (m_keyCaptureNodeIndex >= 0) {
            p.setPen(QColor(255, 220, 50, 220));
            p.drawText(QRectF(0, height() - 48, width(), 24), Qt::AlignCenter,
                       "⌨ WAITING FOR KEY INPUT...");
        }
    } else if (m_mode == PlayMode) {
        // Show activation hint in play mode
        QFont f = p.font();
        f.setPixelSize(11);
        f.setBold(true);
        p.setFont(f);
        p.setPen(QColor(255, 255, 255, 120));
        QString hint = "Press ";
        if (!m_switchKeyHint.isEmpty())
            hint += m_switchKeyHint;
        else if (!m_switchKey.isEmpty()) {
            QString sk = m_switchKey;
            if (sk.startsWith("Key_")) sk = sk.mid(4);
            if (sk == "QuoteLeft") sk = "` (backtick)";
            hint += sk;
        } else {
            hint += "` (backtick)";
        }
        hint += " to activate • F12 to edit";
        p.drawText(QRectF(0, height() - 20, width(), 20), Qt::AlignCenter, hint);
    }
}

void KeymapOverlay::paintNode(QPainter &p, const OverlayNode &node, const QRectF &rect)
{
    switch (node.type) {
    case OverlayNode::Click:
    case OverlayNode::ClickTwice:
    case OverlayNode::ClickMulti:
        paintClickNode(p, node, rect);
        break;
    case OverlayNode::Drag:
        paintDragNode(p, node, rect);
        break;
    case OverlayNode::SteerWheel:
        paintSteerWheelNode(p, node, rect);
        break;
    case OverlayNode::Gesture:
        paintGestureNode(p, node, rect);
        break;
    }
}

void KeymapOverlay::paintClickNode(QPainter &p, const OverlayNode &node, const QRectF &rect)
{
    int baseAlpha = (m_mode == EditMode) ? 160 : 90;

    QColor bg;
    if (node.type == OverlayNode::Click)
        bg = QColor(50, 150, 255, baseAlpha);
    else if (node.type == OverlayNode::ClickTwice)
        bg = QColor(255, 165, 0, baseAlpha);
    else // ClickMulti
        bg = QColor(255, 80, 80, baseAlpha);

    if (node.highlighted) {
        bg.setAlpha(220);
        bg = bg.lighter(130);
    }
    if (node.selected) {
        bg = QColor(0, 255, 200, 180);
    }

    // Circle
    p.setPen(QPen(bg.lighter(140), 2));
    p.setBrush(bg);
    p.drawEllipse(rect);

    // Key label
    QString label = node.keyCode;
    // Strip "Key_" prefix for readability
    if (label.startsWith("Key_"))
        label = label.mid(4);
    if (label.length() > 4)
        label = label.left(4);

    QFont f = p.font();
    f.setPixelSize(qMax(10, (int)(rect.height() * 0.35)));
    f.setBold(true);
    p.setFont(f);
    p.setPen(QColor(255, 255, 255, (m_mode == EditMode) ? 240 : 180));
    p.drawText(rect, Qt::AlignCenter, label);

    // Type indicator for ClickTwice / ClickMulti
    if (node.type == OverlayNode::ClickTwice) {
        QFont sf = f;
        sf.setPixelSize(qMax(8, (int)(rect.height() * 0.2)));
        p.setFont(sf);
        p.drawText(rect.adjusted(0, 0, 0, -rect.height() * 0.55), Qt::AlignCenter, "×2");
    } else if (node.type == OverlayNode::ClickMulti) {
        QFont sf = f;
        sf.setPixelSize(qMax(8, (int)(rect.height() * 0.2)));
        p.setFont(sf);
        p.drawText(rect.adjusted(0, 0, 0, -rect.height() * 0.55), Qt::AlignCenter, "M");
    }
}

void KeymapOverlay::paintDragNode(QPainter &p, const OverlayNode &node, const QRectF &rect)
{
    int baseAlpha = (m_mode == EditMode) ? 160 : 90;
    QColor bg(0, 200, 100, baseAlpha);
    if (node.highlighted) bg = bg.lighter(140);
    if (node.selected) bg = QColor(0, 255, 200, 180);

    // Start circle
    p.setPen(QPen(bg.lighter(140), 2));
    p.setBrush(bg);
    p.drawEllipse(rect);

    // Arrow to end pos
    QPointF startCenter = rect.center();
    QPointF endCenter = relativeToWidget(node.dragEndPos);

    p.setPen(QPen(bg.lighter(120), 2, Qt::DashLine));
    p.drawLine(startCenter, endCenter);

    // End dot
    p.setBrush(bg.lighter(130));
    p.drawEllipse(endCenter, 6, 6);

    // Label
    QString label = node.keyCode;
    if (label.startsWith("Key_")) label = label.mid(4);
    if (label.length() > 4) label = label.left(4);

    QFont f = p.font();
    f.setPixelSize(qMax(10, (int)(rect.height() * 0.35)));
    f.setBold(true);
    p.setFont(f);
    p.setPen(QColor(255, 255, 255, (m_mode == EditMode) ? 240 : 180));
    p.drawText(rect, Qt::AlignCenter, label);
}

void KeymapOverlay::paintSteerWheelNode(QPainter &p, const OverlayNode &node, const QRectF &rect)
{
    int baseAlpha = (m_mode == EditMode) ? 140 : 70;
    QColor bg(100, 100, 255, baseAlpha);
    if (node.highlighted) bg = bg.lighter(150);
    if (node.selected) bg = QColor(0, 255, 200, 160);

    // Outer circle
    p.setPen(QPen(bg.lighter(130), 2));
    p.setBrush(QColor(bg.red(), bg.green(), bg.blue(), baseAlpha / 2));
    p.drawEllipse(rect);

    // Direction labels
    QFont f = p.font();
    f.setPixelSize(qMax(9, (int)(rect.height() * 0.15)));
    f.setBold(true);
    p.setFont(f);
    p.setPen(QColor(255, 255, 255, (m_mode == EditMode) ? 230 : 160));

    auto stripKey = [](const QString &k) {
        QString s = k;
        if (s.startsWith("Key_")) s = s.mid(4);
        return s.left(3);
    };

    QRectF center = rect;
    qreal h4 = rect.height() / 4;
    qreal w4 = rect.width() / 4;

    // Up
    p.drawText(QRectF(center.x(), center.y(), center.width(), h4 * 1.5), Qt::AlignCenter, stripKey(node.upKey));
    // Down
    p.drawText(QRectF(center.x(), center.bottom() - h4 * 1.5, center.width(), h4 * 1.5), Qt::AlignCenter, stripKey(node.downKey));
    // Left
    p.drawText(QRectF(center.x(), center.y(), w4 * 1.5, center.height()), Qt::AlignCenter, stripKey(node.leftKey));
    // Right
    p.drawText(QRectF(center.right() - w4 * 1.5, center.y(), w4 * 1.5, center.height()), Qt::AlignCenter, stripKey(node.rightKey));

    // Center crosshair
    QPointF c = rect.center();
    p.setPen(QPen(QColor(255, 255, 255, 80), 1));
    p.drawLine(QPointF(c.x(), rect.top() + 8), QPointF(c.x(), rect.bottom() - 8));
    p.drawLine(QPointF(rect.left() + 8, c.y()), QPointF(rect.right() - 8, c.y()));
}

void KeymapOverlay::paintGestureNode(QPainter &p, const OverlayNode &node, const QRectF &rect)
{
    int baseAlpha = (m_mode == EditMode) ? 150 : 80;
    QColor bg(180, 80, 220, baseAlpha);
    if (node.highlighted) bg = bg.lighter(140);
    if (node.selected) bg = QColor(0, 255, 200, 180);

    p.setPen(QPen(bg.lighter(140), 2));
    p.setBrush(bg);
    p.drawEllipse(rect);

    // Gesture icon (✋)
    QFont f = p.font();
    f.setPixelSize(qMax(14, (int)(rect.height() * 0.4)));
    p.setFont(f);
    p.setPen(QColor(255, 255, 255, (m_mode == EditMode) ? 240 : 180));
    p.drawText(rect, Qt::AlignCenter, "✋");

    // Label below
    QString label = node.keyCode;
    if (label.startsWith("Key_")) label = label.mid(4);
    if (label.length() > 4) label = label.left(4);
    if (!label.isEmpty()) {
        f.setPixelSize(qMax(8, (int)(rect.height() * 0.2)));
        p.setFont(f);
        p.drawText(rect.adjusted(0, rect.height() * 0.5, 0, 0), Qt::AlignCenter, label);
    }
}

void KeymapOverlay::paintEditToolbar(QPainter &p)
{
    Q_UNUSED(p)
    // The edit toolbar is built from actual child widgets, not painted.
}

// ============================================================================
// Mouse Events (Edit Mode)
// ============================================================================

void KeymapOverlay::mousePressEvent(QMouseEvent *event)
{
    if (m_mode != EditMode) {
        event->ignore();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        int idx = hitTestNode(event->position());
        // Deselect all
        for (auto &n : m_nodes) n.selected = false;

        if (idx >= 0) {
            m_nodes[idx].selected = true;
            m_dragNodeIndex = idx;
            m_dragOffset = event->position() - nodeRect(m_nodes[idx]).center();
        } else {
            m_dragNodeIndex = -1;
        }
        update();
        event->accept();
    } else {
        event->ignore();
    }
}

void KeymapOverlay::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_mode != EditMode) {
        event->ignore();
        return;
    }

    if (event->button() == Qt::LeftButton && m_dragNodeIndex >= 0) {
        m_dragNodeIndex = -1;
        update();
        event->accept();
    } else {
        event->ignore();
    }
}

void KeymapOverlay::mouseMoveEvent(QMouseEvent *event)
{
    if (m_mode != EditMode) {
        event->ignore();
        return;
    }

    if (m_dragNodeIndex >= 0 && (event->buttons() & Qt::LeftButton)) {
        QPointF newCenter = event->position() - m_dragOffset;
        QPointF rel = widgetToRelative(newCenter);
        m_nodes[m_dragNodeIndex].relativePos = rel;
        update();
        event->accept();
    } else {
        // Change cursor based on hover
        int idx = hitTestNode(event->position());
        setCursor(idx >= 0 ? Qt::OpenHandCursor : Qt::CrossCursor);
        event->accept();
    }
}

void KeymapOverlay::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (m_mode != EditMode) {
        event->ignore();
        return;
    }
    int idx = hitTestNode(event->position());
    if (idx >= 0) {
        // Double-click an existing node → enter key-capture mode
        m_keyCaptureNodeIndex = idx;
        // Deselect all, select this node
        for (auto &n : m_nodes) n.selected = false;
        m_nodes[idx].selected = true;
        showToast("Press a key to assign to this node (Esc to cancel)...", 5000);
        update();
    } else {
        // Double-click on empty space → add a new Click node + enter key capture
        OverlayNode node;
        node.type = OverlayNode::Click;
        node.relativePos = widgetToRelative(event->position());
        node.keyCode = "Key_?";
        m_nodes.append(node);
        m_keyCaptureNodeIndex = m_nodes.size() - 1;
        for (auto &n : m_nodes) n.selected = false;
        m_nodes.last().selected = true;
        showToast("New node — press a key to assign (Esc to cancel)...", 5000);
        update();
    }
    event->accept();
}

// ============================================================================
// Key Events
// ============================================================================

void KeymapOverlay::keyPressEvent(QKeyEvent *event)
{
    // F12 toggles mode regardless (but not during key capture)
    if (event->key() == Qt::Key_F12 && !event->isAutoRepeat() && m_keyCaptureNodeIndex < 0) {
        toggleMode();
        event->accept();
        return;
    }

    if (m_mode == EditMode) {
        // ── Key-capture mode: assign the pressed key to the selected node ──
        if (m_keyCaptureNodeIndex >= 0 && m_keyCaptureNodeIndex < m_nodes.size()) {
            if (event->key() == Qt::Key_Escape) {
                // Cancel capture
                m_keyCaptureNodeIndex = -1;
                showToast("Key assignment cancelled", 1500);
                update();
                event->accept();
                return;
            }

            // Map the pressed key to a "Key_X" string
            QString keyName;
            QMetaEnum metaEnum = QMetaEnum::fromType<Qt::Key>();
            const char *name = metaEnum.valueToKey(event->key());
            if (name) {
                keyName = QString("Key_") + QString(name).mid(4); // strip "Key_" prefix from enum, re-add
            } else {
                keyName = QString("Key_%1").arg(event->key());
            }

            m_nodes[m_keyCaptureNodeIndex].keyCode = keyName;
            m_nodes[m_keyCaptureNodeIndex].selected = false;
            showToast(QString("Assigned: %1").arg(keyName), 2000);
            m_keyCaptureNodeIndex = -1;
            update();
            event->accept();
            return;
        }

        // Delete selected
        if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
            for (int i = m_nodes.size() - 1; i >= 0; --i) {
                if (m_nodes[i].selected) {
                    m_nodes.removeAt(i);
                }
            }
            update();
            event->accept();
            return;
        }

        // Ctrl+Shift+S apply (save + runtime reload)
        if (event->key() == Qt::Key_S && (event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)) == (Qt::ControlModifier | Qt::ShiftModifier)) {
            applyKeymap();
            event->accept();
            return;
        }

        // Ctrl+S save (disk only)
        if (event->key() == Qt::Key_S && (event->modifiers() & Qt::ControlModifier)) {
            saveKeymap();
            event->accept();
            return;
        }

        // Escape → go back to play mode
        if (event->key() == Qt::Key_Escape) {
            setMode(PlayMode);
            event->accept();
            return;
        }

        event->accept();
    } else {
        // Play mode — highlight nodes whose keyCode matches
        // We need to let the parent (VideoForm) handle the event
        event->ignore();
    }
}

void KeymapOverlay::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F12) {
        event->accept();
        return;
    }

    if (m_mode == EditMode) {
        event->accept();
    } else {
        event->ignore();
    }
}

// ============================================================================
// Context Menu (Edit Mode)
// ============================================================================

void KeymapOverlay::contextMenuEvent(QContextMenuEvent *event)
{
    if (m_mode != EditMode) {
        event->ignore();
        return;
    }

    int idx = hitTestNode(event->pos());
    QMenu menu(this);

    if (idx >= 0) {
        // ── Set Key (key capture) ──
        menu.addAction("🎹 Set Key (double-click)", this, [this, idx]() {
            if (idx >= 0 && idx < m_nodes.size()) {
                m_keyCaptureNodeIndex = idx;
                for (auto &n : m_nodes) n.selected = false;
                m_nodes[idx].selected = true;
                showToast("Press a key to assign (Esc to cancel)...", 5000);
                update();
            }
        });

        // ── Set Type submenu ──
        QMenu *typeMenu = menu.addMenu("Change Type");
        auto setType = [this, idx](OverlayNode::Type newType) {
            if (idx >= 0 && idx < m_nodes.size()) {
                m_nodes[idx].type = newType;
                if (newType == OverlayNode::SteerWheel) {
                    if (m_nodes[idx].leftKey.isEmpty()) m_nodes[idx].leftKey = "Key_A";
                    if (m_nodes[idx].rightKey.isEmpty()) m_nodes[idx].rightKey = "Key_D";
                    if (m_nodes[idx].upKey.isEmpty()) m_nodes[idx].upKey = "Key_W";
                    if (m_nodes[idx].downKey.isEmpty()) m_nodes[idx].downKey = "Key_S";
                }
                if (newType == OverlayNode::Drag && m_nodes[idx].dragEndPos.isNull()) {
                    m_nodes[idx].dragEndPos = m_nodes[idx].relativePos + QPointF(0.1, 0.0);
                }
                update();
            }
        };
        typeMenu->addAction("Click", this, [setType]() { setType(OverlayNode::Click); });
        typeMenu->addAction("Click Twice", this, [setType]() { setType(OverlayNode::ClickTwice); });
        typeMenu->addAction("Drag", this, [setType]() { setType(OverlayNode::Drag); });
        typeMenu->addAction("WASD Steer", this, [setType]() { setType(OverlayNode::SteerWheel); });
        typeMenu->addAction("Gesture", this, [setType]() { setType(OverlayNode::Gesture); });

        menu.addSeparator();

        // Node-specific actions
        menu.addAction("🗑️ Delete Node", this, [this, idx]() {
            if (idx >= 0 && idx < m_nodes.size()) {
                m_nodes.removeAt(idx);
                update();
            }
        });
        menu.addSeparator();
    }

    // Add node submenu
    QMenu *addMenu = menu.addMenu("Add Node Here");
    addMenu->addAction("Click", this, [this, event]() {
        OverlayNode node;
        node.type = OverlayNode::Click;
        node.relativePos = widgetToRelative(event->pos());
        node.keyCode = "Key_?";
        m_nodes.append(node);
        update();
    });
    addMenu->addAction("Click Twice", this, [this, event]() {
        OverlayNode node;
        node.type = OverlayNode::ClickTwice;
        node.relativePos = widgetToRelative(event->pos());
        node.keyCode = "Key_?";
        m_nodes.append(node);
        update();
    });
    addMenu->addAction("Drag", this, [this, event]() {
        OverlayNode node;
        node.type = OverlayNode::Drag;
        node.relativePos = widgetToRelative(event->pos());
        node.dragEndPos = node.relativePos + QPointF(0.1, 0.0);
        node.keyCode = "Key_?";
        m_nodes.append(node);
        update();
    });
    addMenu->addAction("WASD Steer", this, [this, event]() {
        OverlayNode node;
        node.type = OverlayNode::SteerWheel;
        node.relativePos = widgetToRelative(event->pos());
        node.leftKey  = "Key_A";
        node.rightKey = "Key_D";
        node.upKey    = "Key_W";
        node.downKey  = "Key_S";
        m_nodes.append(node);
        update();
    });
    addMenu->addAction("Gesture", this, [this, event]() {
        OverlayNode node;
        node.type = OverlayNode::Gesture;
        node.relativePos = widgetToRelative(event->pos());
        node.keyCode = "Key_?";
        m_nodes.append(node);
        update();
    });

    menu.addSeparator();
    menu.addAction("Save (Ctrl+S)", this, &KeymapOverlay::saveKeymap);
    menu.addAction("Apply (Ctrl+Shift+S)", this, &KeymapOverlay::applyKeymap);
    menu.addAction("Exit Edit Mode (F12)", this, [this]() { setMode(PlayMode); });

    menu.exec(event->globalPos());
    event->accept();
}

// ============================================================================
// Resize
// ============================================================================

void KeymapOverlay::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // Scale nodes proportionally (target ~48px at 720p height)
    m_nodeScale = qMax(0.5, height() / 720.0);

    // Reposition edit toolbar
    if (m_editBar) {
        m_editBar->setGeometry(0, 0, width(), 36);
    }
}

// ============================================================================
// Edit-mode toolbar
// ============================================================================

void KeymapOverlay::initEditToolbar()
{
    m_editBar = new QWidget(this);
    m_editBar->setStyleSheet(
        "QWidget { background: rgba(20, 20, 30, 200); }"
        "QPushButton { background: rgba(60, 130, 255, 180); color: white; border: none; "
        "  border-radius: 4px; padding: 4px 12px; font-size: 12px; font-weight: bold; }"
        "QPushButton:hover { background: rgba(80, 160, 255, 220); }"
        "QPushButton:pressed { background: rgba(40, 100, 220, 220); }"
        "QLabel { color: rgba(200, 230, 255, 220); font-size: 12px; font-weight: bold; }"
        "QComboBox { background: rgba(40, 50, 70, 220); color: white; border: 1px solid rgba(100,140,200,150); "
        "  border-radius: 4px; padding: 3px 8px; font-size: 11px; min-width: 100px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background: rgba(30,35,50,240); color: white; selection-background-color: rgba(60,130,255,200); }"
    );

    auto *layout = new QHBoxLayout(m_editBar);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(8);

    m_modeLabel = new QLabel("✏️  EDIT MODE", m_editBar);
    layout->addWidget(m_modeLabel);

    // Keymap profile label (read-only — shows which keymap is loaded)
    m_profileCombo = new QComboBox(m_editBar);
    m_profileCombo->setToolTip("Active keymap (change from main dialog before starting server)");
    m_profileCombo->setEnabled(false);  // Read-only: no switching while server is running
    layout->addWidget(m_profileCombo);

    layout->addStretch();

    m_saveBtn = new QPushButton("💾 Save", m_editBar);
    connect(m_saveBtn, &QPushButton::clicked, this, &KeymapOverlay::saveKeymap);
    layout->addWidget(m_saveBtn);

    m_applyBtn = new QPushButton("⚡ Apply", m_editBar);
    m_applyBtn->setToolTip("Save and immediately reload keymap at runtime");
    m_applyBtn->setStyleSheet(
        "QPushButton { background: rgba(40, 180, 80, 200); color: white; border: none; "
        "  border-radius: 4px; padding: 4px 12px; font-size: 12px; font-weight: bold; }"
        "QPushButton:hover { background: rgba(50, 210, 100, 230); }"
        "QPushButton:pressed { background: rgba(30, 150, 60, 230); }"
    );
    connect(m_applyBtn, &QPushButton::clicked, this, &KeymapOverlay::applyKeymap);
    layout->addWidget(m_applyBtn);

    m_doneBtn = new QPushButton("✅ Done (F12)", m_editBar);
    connect(m_doneBtn, &QPushButton::clicked, this, [this]() { setMode(PlayMode); });
    layout->addWidget(m_doneBtn);

    m_editBar->setGeometry(0, 0, 400, 36);
    m_editBar->hide();

    // ── Toast label (centered, hidden) ─────────────────────────────────
    m_toastLabel = new QLabel(this);
    m_toastLabel->setAlignment(Qt::AlignCenter);
    m_toastLabel->setStyleSheet(
        "QLabel { background: rgba(30, 180, 90, 210); color: white; "
        "  border-radius: 8px; padding: 8px 20px; font-size: 14px; font-weight: bold; }"
    );
    m_toastLabel->hide();

    m_toastTimer = new QTimer(this);
    m_toastTimer->setSingleShot(true);
    connect(m_toastTimer, &QTimer::timeout, m_toastLabel, &QLabel::hide);
}

// ============================================================================
// Toast Notification
// ============================================================================

void KeymapOverlay::showToast(const QString &message, int durationMs)
{
    if (!m_toastLabel) return;
    m_toastLabel->setText(message);
    m_toastLabel->adjustSize();
    // Center horizontally, near bottom
    int tx = (width() - m_toastLabel->width()) / 2;
    int ty = height() - m_toastLabel->height() - 50;
    m_toastLabel->move(tx, ty);
    m_toastLabel->show();
    m_toastLabel->raise();
    m_toastTimer->start(durationMs);
}

// ============================================================================
// Profile Combo Population
// ============================================================================

void KeymapOverlay::populateProfileCombo()
{
    if (!m_profileCombo) return;
    m_profileCombo->blockSignals(true);
    m_profileCombo->clear();

    // Use the shared canonical keymap directory
    QString keymapPath = getCanonicalKeymapDir();
    QDir keymapDir(keymapPath);
    QStringList filters;
    filters << "*.json";
    QFileInfoList files = keymapDir.entryInfoList(filters, QDir::Files, QDir::Name);

    int currentIdx = -1;
    for (int i = 0; i < files.size(); ++i) {
        m_profileCombo->addItem(files[i].baseName(), files[i].absoluteFilePath());
        if (files[i].absoluteFilePath() == m_filePath)
            currentIdx = i;
    }
    if (currentIdx >= 0)
        m_profileCombo->setCurrentIndex(currentIdx);

    m_profileCombo->blockSignals(false);
}
