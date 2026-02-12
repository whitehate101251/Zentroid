#ifndef PROPERTIESPANEL_H
#define PROPERTIESPANEL_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QStackedWidget>
#include <QGridLayout>
#include <QSpinBox>

class KeyNode;
class KeyCaptureEdit;

/**
 * @brief Right-side properties panel for the selected keymap node.
 *
 * Shows live-editable fields for position, key, comment, and type-specific
 * properties.  Updates the node in real time.
 */
class PropertiesPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PropertiesPanel(QWidget *parent = nullptr);

    /** Load values from the given node.  nullptr clears the panel. */
    void setNode(KeyNode *node);
    KeyNode *currentNode() const { return m_node; }

signals:
    void nodeModified();

private slots:
    void onPosXChanged(double v);
    void onPosYChanged(double v);
    void onKeyChanged(const QString &key);
    void onCommentChanged(const QString &text);
    void onSwitchMapToggled(bool v);
    // Drag
    void onDragEndXChanged(double v);
    void onDragEndYChanged(double v);
    void onDragSpeedChanged(double v);
    // WASD
    void onWASDKeyChanged();
    void onWASDOffsetChanged();
    // Gesture
    void onGestureTypeChanged(int idx);
    void onGestureDurationChanged(int ms);

private:
    void buildUI();
    void blockAllSignals(bool block);

    KeyNode *m_node = nullptr;

    // Common
    QLabel          *m_titleLabel;
    QLabel          *m_typeLabel;
    QDoubleSpinBox  *m_posX;
    QDoubleSpinBox  *m_posY;
    KeyCaptureEdit  *m_keyEdit;
    QLineEdit       *m_commentEdit;
    QCheckBox       *m_switchMapCheck;

    // Type-specific stacked widget
    QStackedWidget  *m_stack;

    // Drag page
    QDoubleSpinBox  *m_dragEndX;
    QDoubleSpinBox  *m_dragEndY;
    QDoubleSpinBox  *m_dragSpeed;

    // WASD page
    KeyCaptureEdit  *m_wasdL;
    KeyCaptureEdit  *m_wasdR;
    KeyCaptureEdit  *m_wasdU;
    KeyCaptureEdit  *m_wasdD;
    QDoubleSpinBox  *m_offL;
    QDoubleSpinBox  *m_offR;
    QDoubleSpinBox  *m_offU;
    QDoubleSpinBox  *m_offD;

    // Gesture page
    QComboBox       *m_gestureTypeCombo;
    QSpinBox        *m_gestureDurationSpin;
    QLabel          *m_gestureFingerInfo;

    // "No selection" widget
    QWidget *m_emptyPage;
    QWidget *m_mainPage;
    QStackedWidget *m_rootStack;
};

#endif // PROPERTIESPANEL_H
