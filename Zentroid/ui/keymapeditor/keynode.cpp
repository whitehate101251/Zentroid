#include "keynode.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QCursor>
#include <QJsonArray>
#include <QtMath>

// ============================================================================
// KeyNode Base Class Implementation
// ============================================================================

KeyNode::KeyNode(NodeType type, const QPointF &relativePos)
    : QGraphicsRectItem()
    , m_type(type)
    , m_relativePos(relativePos)
    , m_switchMap(false)
    , m_hovered(false)
    , m_dragging(false)
    , m_highlighted(false)
{
    // Set default appearance
    setRect(0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    setFlags(ItemIsSelectable | ItemIsMovable | ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    setCursor(Qt::PointingHandCursor);
    
    // Default colors
    m_normalColor = QColor(66, 133, 244, 180);     // Blue
    m_hoverColor = QColor(66, 133, 244, 220);       // Lighter blue
    m_selectedColor = QColor(234, 67, 53, 200);     // Red
}

void KeyNode::setRelativePosition(const QPointF &pos)
{
    m_relativePos = pos;
}

void KeyNode::setKeyCode(const QString &key)
{
    m_keyCode = key;
    update();
}

void KeyNode::setComment(const QString &comment)
{
    m_comment = comment;
}

void KeyNode::setSwitchMap(bool switchMap)
{
    m_switchMap = switchMap;
}

QString KeyNode::typeString() const
{
    switch (m_type) {
        case Click: return "KMT_CLICK";
        case ClickTwice: return "KMT_CLICK_TWICE";
        case Drag: return "KMT_DRAG";
        case SteerWheel: return "KMT_STEER_WHEEL";
        case ClickMulti: return "KMT_CLICK_MULTI";
        case Gesture: return "KMT_GESTURE";
    }
    return "UNKNOWN";
}

bool KeyNode::isValid() const
{
    return !m_keyCode.isEmpty() && 
           m_relativePos.x() >= 0.0 && m_relativePos.x() <= 1.0 &&
           m_relativePos.y() >= 0.0 && m_relativePos.y() <= 1.0;
}

void KeyNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    // Draw node-specific visuals
    paintNode(painter);
    
    // Draw key label
    paintKeyLabel(painter);
    
    // Draw coordinates
    paintCoordinates(painter);
}

void KeyNode::paintNode(QPainter *painter)
{
    // Determine color based on state
    QColor bgColor = m_normalColor;
    if (m_highlighted) {
        bgColor = QColor(0, 255, 100, 240);  // bright green flash
    } else if (isSelected()) {
        bgColor = m_selectedColor;
    } else if (m_hovered) {
        bgColor = m_hoverColor;
    }
    
    // Draw background
    painter->setBrush(bgColor);
    QPen borderPen(Qt::white, 2);
    if (m_highlighted) {
        borderPen = QPen(QColor(0, 255, 100), 3);  // green glow border
    }
    painter->setPen(borderPen);
    painter->drawRoundedRect(rect(), 5, 5);
    
    // Draw type indicator icon
    QRectF iconRect(5, 5, 20, 20);
    painter->setPen(QPen(Qt::white, 1.5));
    painter->setBrush(Qt::NoBrush);
    
    switch (m_type) {
        case Click:
            // Circle for click
            painter->drawEllipse(iconRect);
            break;
        case ClickTwice:
            // Two circles for double click
            painter->drawEllipse(iconRect.adjusted(0, 0, -7, -7));
            painter->drawEllipse(iconRect.adjusted(7, 7, 0, 0));
            break;
        default:
            break;
    }
}

void KeyNode::paintKeyLabel(QPainter *painter)
{
    // Extract readable key from Qt::Key_X format
    QString displayKey = m_keyCode;
    if (displayKey.startsWith("Key_")) {
        displayKey = displayKey.mid(4); // Remove "Key_" prefix
    }
    
    // Draw key text
    QFont font("Arial", 14, QFont::Bold);
    painter->setFont(font);
    painter->setPen(Qt::white);
    
    QRectF textRect = rect().adjusted(0, 15, 0, -15);
    painter->drawText(textRect, Qt::AlignCenter, displayKey);
}

void KeyNode::paintCoordinates(QPainter *painter)
{
    QString coordText = QString("(%1, %2)")
        .arg(m_relativePos.x(), 0, 'f', 2)
        .arg(m_relativePos.y(), 0, 'f', 2);
    
    QFont font("Arial", 8);
    painter->setFont(font);
    painter->setPen(Qt::white);
    
    QRectF coordRect = rect().adjusted(2, rect().height() - 15, -2, -2);
    painter->drawText(coordRect, Qt::AlignCenter, coordText);
}

void KeyNode::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->scenePos();
        m_dragStartRelPos = m_relativePos;  // Save for undo
        setCursor(Qt::ClosedHandCursor);
    }
    QGraphicsRectItem::mousePressEvent(event);
}

void KeyNode::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_dragging) {
        QGraphicsRectItem::mouseMoveEvent(event);
    }
}

void KeyNode::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        setCursor(Qt::PointingHandCursor);
    }
    QGraphicsRectItem::mouseReleaseEvent(event);
}

void KeyNode::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    m_hovered = true;
    update();
}

void KeyNode::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    m_hovered = false;
    update();
}

QVariant KeyNode::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged && m_deviceSize.width() > 0 && m_deviceSize.height() > 0) {
        QPointF newPos = value.toPointF();
        // Convert screen pos (top-left of node rect) back to relative center
        qreal cx = (newPos.x() + rect().width() / 2.0) / m_deviceSize.width();
        qreal cy = (newPos.y() + rect().height() / 2.0) / m_deviceSize.height();
        m_relativePos = QPointF(qBound(0.0, cx, 1.0), qBound(0.0, cy, 1.0));
    }
    return QGraphicsRectItem::itemChange(change, value);
}

KeyNode* KeyNode::fromJson(const QJsonObject &obj)
{
    QString typeStr = obj["type"].toString();
    
    if (typeStr == "KMT_CLICK") {
        return ClickNode::fromJson(obj);
    } else if (typeStr == "KMT_CLICK_TWICE") {
        return ClickTwiceNode::fromJson(obj);
    } else if (typeStr == "KMT_DRAG") {
        return DragNode::fromJson(obj);
    } else if (typeStr == "KMT_STEER_WHEEL") {
        return SteerWheelNode::fromJson(obj);
    } else if (typeStr == "KMT_CLICK_MULTI") {
        return ClickMultiNode::fromJson(obj);
    } else if (typeStr == "KMT_GESTURE") {
        return GestureNode::fromJson(obj);
    }
    
    return nullptr;
}

// ============================================================================
// ClickNode Implementation
// ============================================================================

ClickNode::ClickNode(const QPointF &relativePos)
    : KeyNode(Click, relativePos)
{
}

QJsonObject ClickNode::toJson() const
{
    QJsonObject obj;
    obj["comment"] = m_comment;
    obj["type"] = "KMT_CLICK";
    obj["key"] = m_keyCode;
    
    QJsonObject posObj;
    posObj["x"] = m_relativePos.x();
    posObj["y"] = m_relativePos.y();
    obj["pos"] = posObj;
    
    obj["switchMap"] = m_switchMap;
    
    return obj;
}

ClickNode* ClickNode::fromJson(const QJsonObject &obj)
{
    QJsonObject posObj = obj["pos"].toObject();
    QPointF pos(posObj["x"].toDouble(), posObj["y"].toDouble());
    
    ClickNode *node = new ClickNode(pos);
    node->setKeyCode(obj["key"].toString());
    node->setComment(obj["comment"].toString());
    node->setSwitchMap(obj["switchMap"].toBool());
    
    return node;
}

// ============================================================================
// ClickTwiceNode Implementation
// ============================================================================

ClickTwiceNode::ClickTwiceNode(const QPointF &relativePos)
    : KeyNode(ClickTwice, relativePos)
{
}

QJsonObject ClickTwiceNode::toJson() const
{
    QJsonObject obj;
    obj["comment"] = m_comment;
    obj["type"] = "KMT_CLICK_TWICE";
    obj["key"] = m_keyCode;
    
    QJsonObject posObj;
    posObj["x"] = m_relativePos.x();
    posObj["y"] = m_relativePos.y();
    obj["pos"] = posObj;
    
    return obj;
}

ClickTwiceNode* ClickTwiceNode::fromJson(const QJsonObject &obj)
{
    QJsonObject posObj = obj["pos"].toObject();
    QPointF pos(posObj["x"].toDouble(), posObj["y"].toDouble());
    
    ClickTwiceNode *node = new ClickTwiceNode(pos);
    node->setKeyCode(obj["key"].toString());
    node->setComment(obj["comment"].toString());
    
    return node;
}

void ClickTwiceNode::paintNode(QPainter *painter)
{
    // Call base implementation first
    KeyNode::paintNode(painter);
}

// ============================================================================
// DragNode Implementation
// ============================================================================

DragNode::DragNode(const QPointF &startPos, const QPointF &endPos)
    : KeyNode(Drag, startPos)
    , m_startPos(startPos)
    , m_endPos(endPos)
    , m_dragSpeed(1.0)
{
    m_normalColor = QColor(15, 157, 88, 180);  // Green for drag
}

void DragNode::setStartPosition(const QPointF &pos)
{
    m_startPos = pos;
    m_relativePos = pos; // Update base position
    update();
}

void DragNode::setEndPosition(const QPointF &pos)
{
    m_endPos = pos;
    update();
}

void DragNode::setDragSpeed(qreal speed)
{
    m_dragSpeed = qBound(0.0, speed, 1.0);
}

QJsonObject DragNode::toJson() const
{
    QJsonObject obj;
    obj["comment"] = m_comment;
    obj["type"] = "KMT_DRAG";
    obj["key"] = m_keyCode;
    
    QJsonObject startObj;
    startObj["x"] = m_startPos.x();
    startObj["y"] = m_startPos.y();
    obj["startPos"] = startObj;
    
    QJsonObject endObj;
    endObj["x"] = m_endPos.x();
    endObj["y"] = m_endPos.y();
    obj["endPos"] = endObj;
    
    if (m_dragSpeed != 1.0) {
        obj["dragSpeed"] = m_dragSpeed;
    }
    
    return obj;
}

DragNode* DragNode::fromJson(const QJsonObject &obj)
{
    QJsonObject startObj = obj["startPos"].toObject();
    QJsonObject endObj = obj["endPos"].toObject();
    
    QPointF startPos(startObj["x"].toDouble(), startObj["y"].toDouble());
    QPointF endPos(endObj["x"].toDouble(), endObj["y"].toDouble());
    
    DragNode *node = new DragNode(startPos, endPos);
    node->setKeyCode(obj["key"].toString());
    node->setComment(obj["comment"].toString());
    
    if (obj.contains("dragSpeed")) {
        node->setDragSpeed(obj["dragSpeed"].toDouble());
    }
    
    return node;
}

bool DragNode::isValid() const
{
    return KeyNode::isValid() &&
           m_endPos.x() >= 0.0 && m_endPos.x() <= 1.0 &&
           m_endPos.y() >= 0.0 && m_endPos.y() <= 1.0;
}

void DragNode::paintNode(QPainter *painter)
{
    // Call base paint
    KeyNode::paintNode(painter);
    
    // Draw a line from the node center to the end-position on canvas
    if (m_deviceSize.width() > 0 && m_deviceSize.height() > 0) {
        // End position in scene coords, relative to this item
        QPointF endScreen(m_endPos.x() * m_deviceSize.width(),
                          m_endPos.y() * m_deviceSize.height());
        QPointF startScreen(m_startPos.x() * m_deviceSize.width(),
                            m_startPos.y() * m_deviceSize.height());
        QPointF delta = endScreen - startScreen;
        QPointF localEnd = rect().center() + delta;
        
        // Draw dashed line
        QPen dashPen(Qt::white, 2, Qt::DashLine);
        painter->setPen(dashPen);
        painter->drawLine(rect().center(), localEnd);
        
        // Draw endpoint circle
        painter->setBrush(QColor(15, 157, 88, 200));
        painter->setPen(QPen(Qt::white, 2));
        painter->drawEllipse(localEnd, 8, 8);
        
        // Arrow head on the line
        QPointF dir = localEnd - rect().center();
        qreal len = qSqrt(dir.x()*dir.x() + dir.y()*dir.y());
        if (len > 20) {
            QPointF norm = dir / len;
            QPointF perp(-norm.y(), norm.x());
            QPointF arrowBase = localEnd - norm * 12;
            QPolygonF arrowHead;
            arrowHead << localEnd
                      << (arrowBase + perp * 6)
                      << (arrowBase - perp * 6);
            painter->setBrush(Qt::white);
            painter->setPen(Qt::NoPen);
            painter->drawPolygon(arrowHead);
        }
    } else {
        // Fallback: simplified arrow indicator
        painter->setPen(QPen(Qt::white, 2));
        QPointF center = rect().center();
        QPointF arrowEnd = center + QPointF(15, 15);
        painter->drawLine(center, arrowEnd);
    }
}

// ============================================================================
// SteerWheelNode Implementation
// ============================================================================

SteerWheelNode::SteerWheelNode(const QPointF &centerPos)
    : KeyNode(SteerWheel, centerPos)
    , m_centerPos(centerPos)
    , m_leftKey("Key_A")
    , m_rightKey("Key_D")
    , m_upKey("Key_W")
    , m_downKey("Key_S")
    , m_leftOffset(0.1)
    , m_rightOffset(0.1)
    , m_upOffset(0.1)
    , m_downOffset(0.1)
{
    m_normalColor = QColor(251, 188, 5, 180);  // Yellow for WASD
    setRect(0, 0, 80, 80); // Larger for WASD
}

void SteerWheelNode::setCenterPosition(const QPointF &pos)
{
    m_centerPos = pos;
    m_relativePos = pos;
}

void SteerWheelNode::setDirectionKeys(const QString &left, const QString &right,
                                      const QString &up, const QString &down)
{
    m_leftKey = left;
    m_rightKey = right;
    m_upKey = up;
    m_downKey = down;
    update();
}

void SteerWheelNode::setOffsets(qreal left, qreal right, qreal up, qreal down)
{
    m_leftOffset = left;
    m_rightOffset = right;
    m_upOffset = up;
    m_downOffset = down;
}

QJsonObject SteerWheelNode::toJson() const
{
    QJsonObject obj;
    obj["comment"] = m_comment;
    obj["type"] = "KMT_STEER_WHEEL";
    
    QJsonObject centerObj;
    centerObj["x"] = m_centerPos.x();
    centerObj["y"] = m_centerPos.y();
    obj["centerPos"] = centerObj;
    
    obj["leftKey"] = m_leftKey;
    obj["rightKey"] = m_rightKey;
    obj["upKey"] = m_upKey;
    obj["downKey"] = m_downKey;
    
    obj["leftOffset"] = m_leftOffset;
    obj["rightOffset"] = m_rightOffset;
    obj["upOffset"] = m_upOffset;
    obj["downOffset"] = m_downOffset;
    
    return obj;
}

SteerWheelNode* SteerWheelNode::fromJson(const QJsonObject &obj)
{
    QJsonObject centerObj = obj["centerPos"].toObject();
    QPointF centerPos(centerObj["x"].toDouble(), centerObj["y"].toDouble());
    
    SteerWheelNode *node = new SteerWheelNode(centerPos);
    node->setComment(obj["comment"].toString());
    node->setDirectionKeys(
        obj["leftKey"].toString(),
        obj["rightKey"].toString(),
        obj["upKey"].toString(),
        obj["downKey"].toString()
    );
    node->setOffsets(
        obj["leftOffset"].toDouble(),
        obj["rightOffset"].toDouble(),
        obj["upOffset"].toDouble(),
        obj["downOffset"].toDouble()
    );
    
    return node;
}

bool SteerWheelNode::isValid() const
{
    return !m_leftKey.isEmpty() && !m_rightKey.isEmpty() &&
           !m_upKey.isEmpty() && !m_downKey.isEmpty() &&
           KeyNode::isValid();
}

void SteerWheelNode::paintNode(QPainter *painter)
{
    // Call base paint
    KeyNode::paintNode(painter);
    
    // Show the actual assigned keys at cardinal positions
    QFont font("Arial", 9, QFont::Bold);
    painter->setFont(font);
    painter->setPen(Qt::white);
    
    auto shortKey = [](const QString &k) -> QString {
        if (k.startsWith("Key_")) return k.mid(4);
        return k;
    };
    
    QRectF r = rect();
    qreal cx = r.width() / 2.0;
    qreal cy = r.height() / 2.0;
    
    // Up
    painter->drawText(QRectF(cx - 10, 5, 20, 16), Qt::AlignCenter, shortKey(m_upKey));
    // Left
    painter->drawText(QRectF(5, cy - 8, 20, 16), Qt::AlignCenter, shortKey(m_leftKey));
    // Right
    painter->drawText(QRectF(r.width() - 25, cy - 8, 20, 16), Qt::AlignCenter, shortKey(m_rightKey));
    // Down
    painter->drawText(QRectF(cx - 10, r.height() - 20, 20, 16), Qt::AlignCenter, shortKey(m_downKey));
    
    // Cross-hair lines
    painter->setPen(QPen(QColor(255, 255, 255, 80), 1));
    painter->drawLine(QPointF(cx, 20), QPointF(cx, r.height() - 20));
    painter->drawLine(QPointF(20, cy), QPointF(r.width() - 20, cy));
}

// ============================================================
//  ClickMultiNode
// ============================================================

ClickMultiNode::ClickMultiNode(const QPointF &relativePos)
    : KeyNode(ClickMulti, relativePos)
{
    setRect(0, 0, 48, 48);
}

void ClickMultiNode::removeClickPoint(int index)
{
    if (index >= 0 && index < m_clickPoints.size()) {
        m_clickPoints.removeAt(index);
        update();
    }
}

QJsonObject ClickMultiNode::toJson() const
{
    QJsonObject obj;
    obj["type"] = QString("KMT_CLICK_MULTI");

    // Primary key
    obj["key"] = keyCode();

    // Comment
    if (!comment().isEmpty())
        obj["comment"] = comment();

    // switchMap
    if (switchMap())
        obj["switchMap"] = true;

    // Click nodes array
    QJsonArray arr;
    for (const ClickPoint &cp : m_clickPoints) {
        QJsonObject cpObj;
        cpObj["delay"] = cp.delay;
        QJsonObject posObj;
        posObj["x"] = cp.pos.x();
        posObj["y"] = cp.pos.y();
        cpObj["pos"] = posObj;
        arr.append(cpObj);
    }
    obj["clickNodes"] = arr;

    return obj;
}

ClickMultiNode* ClickMultiNode::fromJson(const QJsonObject &obj)
{
    // Use first click-point's position, or center if empty
    QPointF firstPos(0.5, 0.5);
    QJsonArray arr = obj["clickNodes"].toArray();
    if (!arr.isEmpty()) {
        QJsonObject first = arr[0].toObject();
        QJsonObject pos = first["pos"].toObject();
        firstPos = QPointF(pos["x"].toDouble(0.5), pos["y"].toDouble(0.5));
    }

    ClickMultiNode *node = new ClickMultiNode(firstPos);
    node->setKeyCode(obj["key"].toString());
    node->setComment(obj["comment"].toString());
    node->setSwitchMap(obj["switchMap"].toBool(false));

    QList<ClickPoint> points;
    for (int i = 0; i < arr.size(); ++i) {
        QJsonObject cpObj = arr[i].toObject();
        ClickPoint cp;
        cp.delay = cpObj["delay"].toInt(0);
        QJsonObject pos = cpObj["pos"].toObject();
        cp.pos = QPointF(pos["x"].toDouble(0.5), pos["y"].toDouble(0.5));
        points.append(cp);
    }
    node->setClickPoints(points);

    return node;
}

bool ClickMultiNode::isValid() const
{
    return !m_clickPoints.isEmpty() && KeyNode::isValid();
}

void ClickMultiNode::paintNode(QPainter *painter)
{
    // Base paint for background / border / highlight
    KeyNode::paintNode(painter);

    QRectF r = rect();

    // Title label
    QFont titleFont("Arial", 7, QFont::Bold);
    painter->setFont(titleFont);
    painter->setPen(Qt::white);
    painter->drawText(QRectF(0, 2, r.width(), 14), Qt::AlignCenter, "MULTI");

    // Key label
    QFont keyFont("Arial", 10, QFont::Bold);
    painter->setFont(keyFont);
    QString shortKey = keyCode();
    if (shortKey.startsWith("Key_"))
        shortKey = shortKey.mid(4);
    painter->drawText(QRectF(0, 14, r.width(), 18), Qt::AlignCenter, shortKey);

    // Number of click points
    QFont countFont("Arial", 7);
    painter->setFont(countFont);
    painter->setPen(QColor(200, 200, 200));
    painter->drawText(QRectF(0, r.height() - 16, r.width(), 14), Qt::AlignCenter,
                      QString("%1 pts").arg(m_clickPoints.size()));
}

// ============================================================================
// GestureNode Implementation
// ============================================================================

GestureNode::GestureNode(const QPointF &relativePos)
    : KeyNode(Gesture, relativePos)
    , m_gestureType(PinchOut)
    , m_duration(400)
{
    m_normalColor = QColor(156, 39, 176, 180);   // Purple
    m_hoverColor  = QColor(186, 69, 206, 220);
    m_selectedColor = QColor(234, 67, 53, 200);

    // Default: pinch-out centered on relativePos
    applyPreset(PinchOut);
}

void GestureNode::setGestureType(GestureType type)
{
    m_gestureType = type;
    update();
}

void GestureNode::applyPreset(GestureType type, qreal radius)
{
    m_gestureType = type;
    m_fingerPaths.clear();
    QPointF c = m_relativePos;

    switch (type) {
        case PinchIn: {
            FingerPath f1;
            f1.startPos = QPointF(c.x() - radius, c.y());
            f1.endPos   = QPointF(c.x() - radius * 0.2, c.y());
            f1.touchId  = 0;
            FingerPath f2;
            f2.startPos = QPointF(c.x() + radius, c.y());
            f2.endPos   = QPointF(c.x() + radius * 0.2, c.y());
            f2.touchId  = 1;
            m_fingerPaths << f1 << f2;
            break;
        }
        case PinchOut: {
            FingerPath f1;
            f1.startPos = QPointF(c.x() - radius * 0.2, c.y());
            f1.endPos   = QPointF(c.x() - radius, c.y());
            f1.touchId  = 0;
            FingerPath f2;
            f2.startPos = QPointF(c.x() + radius * 0.2, c.y());
            f2.endPos   = QPointF(c.x() + radius, c.y());
            f2.touchId  = 1;
            m_fingerPaths << f1 << f2;
            break;
        }
        case TwoFingerSwipeUp: {
            FingerPath f1;
            f1.startPos = QPointF(c.x() - radius * 0.5, c.y() + radius);
            f1.endPos   = QPointF(c.x() - radius * 0.5, c.y() - radius);
            f1.touchId  = 0;
            FingerPath f2;
            f2.startPos = QPointF(c.x() + radius * 0.5, c.y() + radius);
            f2.endPos   = QPointF(c.x() + radius * 0.5, c.y() - radius);
            f2.touchId  = 1;
            m_fingerPaths << f1 << f2;
            break;
        }
        case TwoFingerSwipeDown: {
            FingerPath f1;
            f1.startPos = QPointF(c.x() - radius * 0.5, c.y() - radius);
            f1.endPos   = QPointF(c.x() - radius * 0.5, c.y() + radius);
            f1.touchId  = 0;
            FingerPath f2;
            f2.startPos = QPointF(c.x() + radius * 0.5, c.y() - radius);
            f2.endPos   = QPointF(c.x() + radius * 0.5, c.y() + radius);
            f2.touchId  = 1;
            m_fingerPaths << f1 << f2;
            break;
        }
        case TwoFingerSwipeLeft: {
            FingerPath f1;
            f1.startPos = QPointF(c.x() + radius, c.y() - radius * 0.5);
            f1.endPos   = QPointF(c.x() - radius, c.y() - radius * 0.5);
            f1.touchId  = 0;
            FingerPath f2;
            f2.startPos = QPointF(c.x() + radius, c.y() + radius * 0.5);
            f2.endPos   = QPointF(c.x() - radius, c.y() + radius * 0.5);
            f2.touchId  = 1;
            m_fingerPaths << f1 << f2;
            break;
        }
        case TwoFingerSwipeRight: {
            FingerPath f1;
            f1.startPos = QPointF(c.x() - radius, c.y() - radius * 0.5);
            f1.endPos   = QPointF(c.x() + radius, c.y() - radius * 0.5);
            f1.touchId  = 0;
            FingerPath f2;
            f2.startPos = QPointF(c.x() - radius, c.y() + radius * 0.5);
            f2.endPos   = QPointF(c.x() + radius, c.y() + radius * 0.5);
            f2.touchId  = 1;
            m_fingerPaths << f1 << f2;
            break;
        }
        case Rotate: {
            // Two fingers rotating 90 degrees clockwise around center
            FingerPath f1;
            f1.startPos = QPointF(c.x() - radius, c.y());
            f1.endPos   = QPointF(c.x(), c.y() - radius);
            f1.touchId  = 0;
            FingerPath f2;
            f2.startPos = QPointF(c.x() + radius, c.y());
            f2.endPos   = QPointF(c.x(), c.y() + radius);
            f2.touchId  = 1;
            m_fingerPaths << f1 << f2;
            break;
        }
        case Custom:
            // Leave empty - user defines manually
            break;
    }
    update();
}

QString GestureNode::gestureTypeName(GestureType type)
{
    switch (type) {
        case PinchIn:             return "PinchIn";
        case PinchOut:            return "PinchOut";
        case TwoFingerSwipeUp:    return "TwoFingerSwipeUp";
        case TwoFingerSwipeDown:  return "TwoFingerSwipeDown";
        case TwoFingerSwipeLeft:  return "TwoFingerSwipeLeft";
        case TwoFingerSwipeRight: return "TwoFingerSwipeRight";
        case Rotate:              return "Rotate";
        case Custom:              return "Custom";
    }
    return "Custom";
}

GestureNode::GestureType GestureNode::gestureTypeFromName(const QString &name)
{
    if (name == "PinchIn")             return PinchIn;
    if (name == "PinchOut")            return PinchOut;
    if (name == "TwoFingerSwipeUp")    return TwoFingerSwipeUp;
    if (name == "TwoFingerSwipeDown")  return TwoFingerSwipeDown;
    if (name == "TwoFingerSwipeLeft")  return TwoFingerSwipeLeft;
    if (name == "TwoFingerSwipeRight") return TwoFingerSwipeRight;
    if (name == "Rotate")              return Rotate;
    return Custom;
}

QJsonObject GestureNode::toJson() const
{
    QJsonObject obj;
    obj["type"] = "KMT_GESTURE";
    obj["key"] = keyCode();

    if (!comment().isEmpty())
        obj["comment"] = comment();
    if (switchMap())
        obj["switchMap"] = true;

    // Center position
    QJsonObject posObj;
    posObj["x"] = m_relativePos.x();
    posObj["y"] = m_relativePos.y();
    obj["pos"] = posObj;

    // Gesture-specific
    obj["gestureType"] = gestureTypeName(m_gestureType);
    obj["duration"] = m_duration;

    // Finger paths
    QJsonArray fingersArr;
    for (const FingerPath &fp : m_fingerPaths) {
        QJsonObject fpObj;
        fpObj["touchId"] = fp.touchId;
        QJsonObject startObj;
        startObj["x"] = fp.startPos.x();
        startObj["y"] = fp.startPos.y();
        fpObj["startPos"] = startObj;
        QJsonObject endObj;
        endObj["x"] = fp.endPos.x();
        endObj["y"] = fp.endPos.y();
        fpObj["endPos"] = endObj;
        fingersArr.append(fpObj);
    }
    obj["fingers"] = fingersArr;

    return obj;
}

GestureNode* GestureNode::fromJson(const QJsonObject &obj)
{
    QJsonObject posObj = obj["pos"].toObject();
    QPointF pos(posObj["x"].toDouble(0.5), posObj["y"].toDouble(0.5));

    GestureNode *node = new GestureNode(pos);
    node->setKeyCode(obj["key"].toString());
    node->setComment(obj["comment"].toString());
    node->setSwitchMap(obj["switchMap"].toBool(false));

    node->m_gestureType = gestureTypeFromName(obj["gestureType"].toString("PinchOut"));
    node->m_duration = obj["duration"].toInt(400);

    QJsonArray fingersArr = obj["fingers"].toArray();
    QList<FingerPath> paths;
    for (int i = 0; i < fingersArr.size(); ++i) {
        QJsonObject fpObj = fingersArr[i].toObject();
        FingerPath fp;
        fp.touchId = fpObj["touchId"].toInt(i);
        QJsonObject sObj = fpObj["startPos"].toObject();
        fp.startPos = QPointF(sObj["x"].toDouble(0.5), sObj["y"].toDouble(0.5));
        QJsonObject eObj = fpObj["endPos"].toObject();
        fp.endPos = QPointF(eObj["x"].toDouble(0.5), eObj["y"].toDouble(0.5));
        paths.append(fp);
    }
    node->setFingerPaths(paths);

    return node;
}

bool GestureNode::isValid() const
{
    return m_fingerPaths.size() >= 2 && KeyNode::isValid();
}

void GestureNode::paintNode(QPainter *painter)
{
    // Base background
    KeyNode::paintNode(painter);

    QRectF r = rect();

    // Title: gesture type
    QFont titleFont("Arial", 6, QFont::Bold);
    painter->setFont(titleFont);
    painter->setPen(Qt::white);

    QString label;
    switch (m_gestureType) {
        case PinchIn:  label = "PINCH IN"; break;
        case PinchOut: label = "PINCH OUT"; break;
        case TwoFingerSwipeUp:    label = "2F UP"; break;
        case TwoFingerSwipeDown:  label = "2F DOWN"; break;
        case TwoFingerSwipeLeft:  label = "2F LEFT"; break;
        case TwoFingerSwipeRight: label = "2F RIGHT"; break;
        case Rotate:   label = "ROTATE"; break;
        case Custom:   label = "GESTURE"; break;
    }
    painter->drawText(QRectF(0, 2, r.width(), 12), Qt::AlignCenter, label);

    // Key label
    QFont keyFont("Arial", 10, QFont::Bold);
    painter->setFont(keyFont);
    QString shortKey = keyCode();
    if (shortKey.startsWith("Key_"))
        shortKey = shortKey.mid(4);
    painter->drawText(QRectF(0, 14, r.width(), 18), Qt::AlignCenter, shortKey);

    // Draw mini finger-path arrows
    if (m_fingerPaths.size() >= 2) {
        QPen arrowPen(QColor(255, 255, 255, 160), 1.5);
        painter->setPen(arrowPen);

        // Map finger positions to node rect space for preview
        qreal margin = 6;
        qreal drawW = r.width() - margin * 2;
        qreal drawH = r.height() - 34 - margin;  // below key label, above bottom
        qreal drawTop = 34;

        for (const FingerPath &fp : m_fingerPaths) {
            // Normalize relative to center +/- range
            qreal cx = m_relativePos.x();
            qreal cy = m_relativePos.y();
            qreal range = 0.15;

            auto mapX = [&](qreal x) {
                return margin + ((x - cx + range) / (2.0 * range)) * drawW;
            };
            auto mapY = [&](qreal y) {
                return drawTop + ((y - cy + range) / (2.0 * range)) * drawH;
            };

            qreal sx = qBound(margin, mapX(fp.startPos.x()), margin + drawW);
            qreal sy = qBound(drawTop, mapY(fp.startPos.y()), drawTop + drawH);
            qreal ex = qBound(margin, mapX(fp.endPos.x()), margin + drawW);
            qreal ey = qBound(drawTop, mapY(fp.endPos.y()), drawTop + drawH);

            // Start dot
            painter->setBrush(QColor(255, 255, 255, 160));
            painter->drawEllipse(QPointF(sx, sy), 2.5, 2.5);

            // Arrow line
            painter->setBrush(Qt::NoBrush);
            painter->drawLine(QPointF(sx, sy), QPointF(ex, ey));

            // Arrowhead
            qreal angle = qAtan2(ey - sy, ex - sx);
            qreal aLen = 5.0;
            QPointF p1(ex - aLen * qCos(angle - 0.4),
                       ey - aLen * qSin(angle - 0.4));
            QPointF p2(ex - aLen * qCos(angle + 0.4),
                       ey - aLen * qSin(angle + 0.4));
            painter->drawLine(QPointF(ex, ey), p1);
            painter->drawLine(QPointF(ex, ey), p2);
        }
    }

    // Duration at bottom
    QFont countFont("Arial", 7);
    painter->setFont(countFont);
    painter->setPen(QColor(200, 200, 200));
    painter->drawText(QRectF(0, r.height() - 16, r.width(), 14), Qt::AlignCenter,
                      QString("%1ms").arg(m_duration));
}
