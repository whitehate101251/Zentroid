#ifndef KEYNODE_H
#define KEYNODE_H

#include <QGraphicsRectItem>
#include <QPointF>
#include <QSize>
#include <QString>
#include <QJsonObject>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>

/**
 * @brief Base class for all keymap nodes
 * 
 * Represents a single key mapping that can be placed on the canvas.
 * Handles rendering, interaction, and serialization.
 */
class KeyNode : public QGraphicsRectItem
{
public:
    /**
     * @brief Types of key mappings supported
     */
    enum NodeType {
        Click,          // KMT_CLICK - Single touch
        ClickTwice,     // KMT_CLICK_TWICE - Double tap
        Drag,           // KMT_DRAG - Swipe/drag gesture
        SteerWheel,     // KMT_STEER_WHEEL - WASD joystick
        ClickMulti,     // KMT_CLICK_MULTI - Multi-touch sequence
        Gesture         // KMT_GESTURE - Multi-finger gesture (pinch, swipe, rotate)
    };

    /**
     * @brief Constructor
     * @param type The type of key mapping
     * @param relativePos Position in relative coordinates (0.0-1.0)
     */
    explicit KeyNode(NodeType type, const QPointF &relativePos);
    virtual ~KeyNode() = default;

    // Getters
    NodeType nodeType() const { return m_type; }
    QPointF relativePosition() const { return m_relativePos; }
    QString keyCode() const { return m_keyCode; }
    QString comment() const { return m_comment; }
    bool switchMap() const { return m_switchMap; }

    // Setters
    void setRelativePosition(const QPointF &pos);
    void setKeyCode(const QString &key);
    void setComment(const QString &comment);
    void setSwitchMap(bool switchMap);
    
    // Layer (editor-only grouping)
    QString layer() const { return m_layer; }
    void setLayer(const QString &layer) { m_layer = layer; }

    /**
     * @brief Store device size for coordinate conversion during drag
     */
    void setDeviceSize(const QSize &size) { m_deviceSize = size; }

    /**
     * @brief Get device size for coordinate conversion
     */
    QSize deviceSize() const { return m_deviceSize; }

    /**
     * @brief Get the relative position saved at the start of a drag operation
     */
    QPointF dragStartRelativePos() const { return m_dragStartRelPos; }

    /**
     * @brief Save the current relative position as drag start (called on mouse press)
     */
    void saveDragStartRelativePos() { m_dragStartRelPos = m_relativePos; }

    /**
     * @brief Set preview-highlight state (visual flash when key is pressed in preview mode)
     */
    void setHighlighted(bool highlighted) { m_highlighted = highlighted; update(); }
    bool isHighlighted() const { return m_highlighted; }

    /**
     * @brief Serialize to JSON
     * @return JSON object representing this node
     */
    virtual QJsonObject toJson() const = 0;

    /**
     * @brief Create node from JSON
     * @param obj JSON object
     * @return Pointer to new KeyNode or nullptr if failed
     */
    static KeyNode* fromJson(const QJsonObject &obj);

    /**
     * @brief Get type name as string
     * @return Type name for JSON (e.g., "KMT_CLICK")
     */
    QString typeString() const;

    /**
     * @brief Check if node configuration is valid
     * @return true if valid
     */
    virtual bool isValid() const;

protected:
    // QGraphicsItem overrides
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, 
               QWidget *widget = nullptr) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    // Helper methods
    virtual void paintNode(QPainter *painter);
    void paintKeyLabel(QPainter *painter);
    void paintCoordinates(QPainter *painter);
    
    // Common properties
    NodeType m_type;
    QPointF m_relativePos;  // 0.0-1.0 range
    QString m_keyCode;      // Qt::Key_X format
    QString m_comment;
    bool m_switchMap;
    QString m_layer;  // editor layer/group name (e.g. "Movement", "Combat")
    
    // Visual state
    bool m_hovered;
    bool m_dragging;
    bool m_highlighted;  // preview mode flash
    QPointF m_dragStartPos;
    QPointF m_dragStartRelPos;  // relative pos at drag start (for undo)
    QSize m_deviceSize;  // for position sync during drag
    
    // Appearance
    static const int DEFAULT_WIDTH = 60;
    static const int DEFAULT_HEIGHT = 60;
    QColor m_normalColor;
    QColor m_hoverColor;
    QColor m_selectedColor;
};

/**
 * @brief Click node (KMT_CLICK)
 */
class ClickNode : public KeyNode
{
public:
    explicit ClickNode(const QPointF &relativePos);
    QJsonObject toJson() const override;
    static ClickNode* fromJson(const QJsonObject &obj);
};

/**
 * @brief Click twice node (KMT_CLICK_TWICE)
 */
class ClickTwiceNode : public KeyNode
{
public:
    explicit ClickTwiceNode(const QPointF &relativePos);
    QJsonObject toJson() const override;
    static ClickTwiceNode* fromJson(const QJsonObject &obj);

protected:
    void paintNode(QPainter *painter) override;
};

/**
 * @brief Drag node (KMT_DRAG)
 */
class DragNode : public KeyNode
{
public:
    explicit DragNode(const QPointF &startPos, const QPointF &endPos = QPointF(0.5, 0.5));
    
    QPointF startPosition() const { return m_startPos; }
    QPointF endPosition() const { return m_endPos; }
    void setStartPosition(const QPointF &pos);
    void setEndPosition(const QPointF &pos);
    
    qreal dragSpeed() const { return m_dragSpeed; }
    void setDragSpeed(qreal speed);
    
    QJsonObject toJson() const override;
    static DragNode* fromJson(const QJsonObject &obj);
    bool isValid() const override;

protected:
    void paintNode(QPainter *painter) override;

private:
    QPointF m_startPos;
    QPointF m_endPos;
    qreal m_dragSpeed;  // 0.0-1.0
};

/**
 * @brief Steering wheel node (KMT_STEER_WHEEL)
 */
class SteerWheelNode : public KeyNode
{
public:
    explicit SteerWheelNode(const QPointF &centerPos);
    
    QPointF centerPosition() const { return m_centerPos; }
    void setCenterPosition(const QPointF &pos);
    
    // WASD keys
    QString leftKey() const { return m_leftKey; }
    QString rightKey() const { return m_rightKey; }
    QString upKey() const { return m_upKey; }
    QString downKey() const { return m_downKey; }
    void setDirectionKeys(const QString &left, const QString &right,
                          const QString &up, const QString &down);
    
    // Offsets
    qreal leftOffset() const { return m_leftOffset; }
    qreal rightOffset() const { return m_rightOffset; }
    qreal upOffset() const { return m_upOffset; }
    qreal downOffset() const { return m_downOffset; }
    void setOffsets(qreal left, qreal right, qreal up, qreal down);
    
    QJsonObject toJson() const override;
    static SteerWheelNode* fromJson(const QJsonObject &obj);
    bool isValid() const override;

protected:
    void paintNode(QPainter *painter) override;

private:
    QPointF m_centerPos;
    QString m_leftKey;
    QString m_rightKey;
    QString m_upKey;
    QString m_downKey;
    qreal m_leftOffset;
    qreal m_rightOffset;
    qreal m_upOffset;
    qreal m_downOffset;
};

/**
 * @brief Click multi node (KMT_CLICK_MULTI) — one key triggers sequential touches
 */
class ClickMultiNode : public KeyNode
{
public:
    struct ClickPoint {
        int delay;      // ms before this click
        QPointF pos;    // relative 0.0-1.0
    };

    explicit ClickMultiNode(const QPointF &relativePos);

    QList<ClickPoint> clickPoints() const { return m_clickPoints; }
    void setClickPoints(const QList<ClickPoint> &points) { m_clickPoints = points; update(); }
    void addClickPoint(const ClickPoint &pt) { m_clickPoints.append(pt); update(); }
    void removeClickPoint(int index);

    QJsonObject toJson() const override;
    static ClickMultiNode* fromJson(const QJsonObject &obj);
    bool isValid() const override;

protected:
    void paintNode(QPainter *painter) override;

private:
    QList<ClickPoint> m_clickPoints;
};

/**
 * @brief Gesture node (KMT_GESTURE) — multi-finger gestures (pinch, swipe, rotate)
 *
 * Each gesture consists of N "fingers", each with a start and end position.
 * The gesture is triggered by a key press and plays back the multi-finger
 * touch sequence with configurable duration.
 */
class GestureNode : public KeyNode
{
public:
    /**
     * @brief Predefined gesture type presets
     */
    enum GestureType {
        PinchIn,        // Two fingers moving inward (zoom out)
        PinchOut,       // Two fingers moving outward (zoom in)
        TwoFingerSwipeUp,
        TwoFingerSwipeDown,
        TwoFingerSwipeLeft,
        TwoFingerSwipeRight,
        Rotate,         // Two fingers rotating around center
        Custom          // User-defined finger paths
    };

    /**
     * @brief A single finger path in the gesture
     */
    struct FingerPath {
        QPointF startPos;   // relative 0.0-1.0
        QPointF endPos;     // relative 0.0-1.0
        int     touchId;    // touch ID (0-9)
    };

    explicit GestureNode(const QPointF &relativePos);

    // Gesture properties
    GestureType gestureType() const { return m_gestureType; }
    void setGestureType(GestureType type);

    int duration() const { return m_duration; }
    void setDuration(int ms) { m_duration = ms; }

    QList<FingerPath> fingerPaths() const { return m_fingerPaths; }
    void setFingerPaths(const QList<FingerPath> &paths) { m_fingerPaths = paths; update(); }

    /**
     * @brief Apply a preset gesture type, auto-generating finger paths
     * around the node's center position.
     */
    void applyPreset(GestureType type, qreal radius = 0.08);

    static QString gestureTypeName(GestureType type);
    static GestureType gestureTypeFromName(const QString &name);

    QJsonObject toJson() const override;
    static GestureNode* fromJson(const QJsonObject &obj);
    bool isValid() const override;

protected:
    void paintNode(QPainter *painter) override;

private:
    GestureType m_gestureType;
    int m_duration;             // total gesture duration in ms
    QList<FingerPath> m_fingerPaths;
};

#endif // KEYNODE_H
