#ifndef KEYASSIGNDIALOG_H
#define KEYASSIGNDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QKeyEvent>
#include <QGridLayout>
#include <QStackedWidget>

class KeyNode;

/**
 * @brief Key capture widget — press a key and it records it
 */
class KeyCaptureEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit KeyCaptureEdit(QWidget *parent = nullptr);
    
    QString capturedKeyString() const { return m_keyString; }
    void setCapturedKeyString(const QString &key);

signals:
    void keyCaptured(const QString &keyString);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QString qtKeyToString(int key) const;
    QString m_keyString;
};

/**
 * @brief Dialog to assign a key, comment, and type-specific properties to a node.
 *
 * Used when creating new nodes (click on canvas) and editing existing nodes (double-click).
 */
class KeyAssignDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Mode of the dialog — creating or editing
     */
    enum Mode {
        CreateClick,
        CreateClickTwice,
        CreateDrag,
        CreateWASD,
        CreateClickMulti,
        CreateGesture,
        EditNode
    };

    explicit KeyAssignDialog(Mode mode, QWidget *parent = nullptr);
    ~KeyAssignDialog() = default;

    // --- Getters for result ---
    QString keyCode() const;
    QString comment() const;
    bool switchMap() const;
    int nodeTypeIndex() const;   // 0=Click, 1=ClickTwice, 2=Drag, 3=WASD

    // Drag-specific
    QPointF dragEndPos() const;
    double  dragSpeed() const;

    // WASD-specific
    QString leftKey() const;
    QString rightKey() const;
    QString upKey() const;
    QString downKey() const;
    double  leftOffset() const;
    double  rightOffset() const;
    double  upOffset() const;
    double  downOffset() const;

    // --- Pre-fill for editing ---
    void setKeyCode(const QString &key);
    void setComment(const QString &comment);
    void setSwitchMap(bool v);
    void setNodeTypeIndex(int idx);

    void setDragEndPos(const QPointF &pos);
    void setDragSpeed(double speed);

    void setWASDKeys(const QString &l, const QString &r, const QString &u, const QString &d);
    void setWASDOffsets(double l, double r, double u, double d);

    // ClickMulti-specific
    struct ClickPointEntry { int delay; QPointF pos; };
    QList<ClickPointEntry> clickMultiPoints() const;
    void setClickMultiPoints(const QList<ClickPointEntry> &points);

    // Gesture-specific
    int gestureTypeIndex() const;
    void setGestureTypeIndex(int idx);
    int gestureDuration() const;
    void setGestureDuration(int ms);
    qreal gestureRadius() const;
    void setGestureRadius(qreal r);

private:
    void buildUI();
    void buildClickPage();
    void buildDragPage();
    void buildWASDPage();
    void buildClickMultiPage();
    void buildGesturePage();

    Mode m_mode;

    // Common widgets
    QComboBox      *m_typeCombo;
    KeyCaptureEdit *m_keyEdit;
    QLineEdit      *m_commentEdit;
    QCheckBox      *m_switchMapCheck;
    QStackedWidget *m_stack;

    // Drag page
    QDoubleSpinBox *m_dragEndX;
    QDoubleSpinBox *m_dragEndY;
    QDoubleSpinBox *m_dragSpeedSpin;

    // WASD page
    KeyCaptureEdit *m_wasdLeft;
    KeyCaptureEdit *m_wasdRight;
    KeyCaptureEdit *m_wasdUp;
    KeyCaptureEdit *m_wasdDown;
    QDoubleSpinBox *m_offLeft;
    QDoubleSpinBox *m_offRight;
    QDoubleSpinBox *m_offUp;
    QDoubleSpinBox *m_offDown;

    // ClickMulti page
    QWidget *m_clickMultiPage;
    QVBoxLayout *m_clickMultiLayout;
    struct ClickPointRow {
        QDoubleSpinBox *x;
        QDoubleSpinBox *y;
        QSpinBox *delay;
    };
    QList<ClickPointRow> m_clickPointRows;

    // Gesture page
    QComboBox      *m_gestureTypeCombo;
    QSpinBox       *m_gestureDurationSpin;
    QDoubleSpinBox *m_gestureRadiusSpin;
};

#endif // KEYASSIGNDIALOG_H
