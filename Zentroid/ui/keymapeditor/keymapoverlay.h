#ifndef KEYMAPOVERLAY_H
#define KEYMAPOVERLAY_H

#include <QWidget>
#include <QList>
#include <QPointF>
#include <QSize>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QFileSystemWatcher>

/**
 * @brief Lightweight representation of a keymap node for overlay rendering.
 *
 * Unlike the full KeyNode (QGraphicsRectItem), OverlayNode is a simple
 * data struct that can be painted directly on a QWidget without a
 * QGraphicsScene.  This keeps the overlay fast and decoupled from the
 * editor infrastructure.
 */
struct OverlayNode
{
    enum Type {
        Click,
        ClickTwice,
        Drag,
        SteerWheel,
        ClickMulti,
        Gesture
    };

    Type type = Click;
    QPointF relativePos;        // 0.0-1.0
    QString keyCode;
    QString comment;

    // Drag specifics
    QPointF dragEndPos;

    // SteerWheel specifics
    QString leftKey, rightKey, upKey, downKey;
    qreal leftOff = 0.05, rightOff = 0.05, upOff = 0.05, downOff = 0.05;

    // Click specifics
    bool switchMap = false;

    // Gesture specifics
    int gestureType = 0;

    // Visual state
    bool highlighted = false;   // key currently held
    bool selected = false;      // selected for edit-mode drag
    int  draggingIndex = -1;    // -1 = not dragging
};

/**
 * @brief Transparent overlay that renders keymap nodes on top of the
 *        live video stream.
 *
 * Two modes:
 *   • **Play mode** (default): fully transparent to mouse/keyboard events;
 *     the user plays the game normally while semi-transparent node indicators
 *     are painted on top so they can see their key bindings.
 *   • **Edit mode**: captures mouse events to let the user drag nodes,
 *     add new nodes, or right-click to edit.  A small toolbar appears at
 *     the top of the overlay.
 *
 * Toggle between modes with F12 (or the ToolForm button).
 */
class KeymapOverlay : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Construct the overlay as a child of the video widget area.
     * @param parent        The parent widget (typically the video widget)
     * @param serial        Device serial for reloading keymap
     */
    explicit KeymapOverlay(QWidget *parent, const QString &serial);
    ~KeymapOverlay();

    // ── Mode management ────────────────────────────────────────────────
    enum OverlayMode { PlayMode, EditMode };
    OverlayMode mode() const { return m_mode; }
    void setMode(OverlayMode mode);
    void toggleMode();

    // ── Keymap management ──────────────────────────────────────────────
    /** Load node data from a keymap JSON file */
    bool loadKeymap(const QString &filePath);
    /** Reload the currently-loaded file from disk */
    void reloadKeymap();
    /** Save current nodes back to the loaded file */
    bool saveKeymap();

    /** Hot-reload: watch the file for external changes */
    void watchFile(const QString &filePath);

    /** Get the file path currently loaded */
    QString currentFilePath() const { return m_filePath; }

    /** Set visibility of all overlay painting */
    void setOverlayVisible(bool visible);
    bool isOverlayVisible() const { return m_overlayVisible; }

    /** Set the hint text for the switch/activation key */
    void setSwitchKeyHint(const QString &hint) { m_switchKeyHint = hint; update(); }

    /** Save to disk + reload runtime (calls saveKeymap then emits keymapApplied) */
    bool applyKeymap();

signals:
    /** Emitted when user saves from edit mode (disk write only — no runtime reload) */
    void keymapSaved(const QString &filePath);
    /** Emitted when user applies — device should reload keymap at runtime */
    void keymapApplied(const QString &filePath);
    /** Emitted when mode changes */
    void modeChanged(KeymapOverlay::OverlayMode newMode);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    // ── Parsing ────────────────────────────────────────────────────────
    bool parseJsonToNodes(const QJsonObject &json);
    QJsonObject generateJsonFromNodes() const;

    // ── Coordinate conversion ──────────────────────────────────────────
    QPointF relativeToWidget(const QPointF &rel) const;
    QPointF widgetToRelative(const QPointF &widgetPos) const;

    // ── Painting helpers ───────────────────────────────────────────────
    void paintNode(QPainter &p, const OverlayNode &node, const QRectF &rect);
    void paintClickNode(QPainter &p, const OverlayNode &node, const QRectF &rect);
    void paintDragNode(QPainter &p, const OverlayNode &node, const QRectF &rect);
    void paintSteerWheelNode(QPainter &p, const OverlayNode &node, const QRectF &rect);
    void paintGestureNode(QPainter &p, const OverlayNode &node, const QRectF &rect);
    void paintEditToolbar(QPainter &p);
    QRectF nodeRect(const OverlayNode &node) const;

    // ── Hit testing ────────────────────────────────────────────────────
    int hitTestNode(const QPointF &widgetPos) const;

    // ── Edit-mode toolbar ──────────────────────────────────────────────
    void initEditToolbar();

    // ── Toast notification ──────────────────────────────────────────────
    void showToast(const QString &message, int durationMs = 2000);

    // ── Profile combo ──────────────────────────────────────────────────
    void populateProfileCombo();

    // ── Data ───────────────────────────────────────────────────────────
    OverlayMode m_mode = PlayMode;
    bool m_overlayVisible = true;
    QString m_filePath;
    QString m_serial;
    QList<OverlayNode> m_nodes;

    // Global keymap settings (passthrough)
    QString m_switchKey;
    bool m_hasMouseMoveMap = false;
    QJsonObject m_mouseMoveMap;

    // Node sizes in pixels (scaled with widget)
    static constexpr int BASE_NODE_SIZE = 48;
    qreal m_nodeScale = 1.0;

    // Drag state (edit mode)
    int m_dragNodeIndex = -1;
    QPointF m_dragOffset;

    // Key-capture state (edit mode): when >= 0, next key press assigns to this node
    int m_keyCaptureNodeIndex = -1;

    // Switch-key hint text for play-mode display
    QString m_switchKeyHint;

    // Highlight state (play mode)
    QSet<QString> m_activeKeys;
    QTimer *m_highlightTimer = nullptr;

    // File watcher for hot-reload
    QFileSystemWatcher *m_fileWatcher = nullptr;

    // Edit-mode toolbar widgets (children)
    QWidget *m_editBar = nullptr;
    QPushButton *m_saveBtn = nullptr;
    QPushButton *m_applyBtn = nullptr;
    QPushButton *m_doneBtn = nullptr;
    QLabel *m_modeLabel = nullptr;
    QComboBox *m_profileCombo = nullptr;

    // Toast notification
    QLabel *m_toastLabel = nullptr;
    QTimer *m_toastTimer = nullptr;
};

#endif // KEYMAPOVERLAY_H
