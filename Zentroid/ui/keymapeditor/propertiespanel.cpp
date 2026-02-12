#include "propertiespanel.h"
#include "keynode.h"
#include "keyassigndialog.h"   // for KeyCaptureEdit

#include <QVBoxLayout>
#include <QScrollArea>

PropertiesPanel::PropertiesPanel(QWidget *parent)
    : QWidget(parent)
{
    setFixedWidth(260);
    buildUI();
}

// ============================================================================
// Public
// ============================================================================

void PropertiesPanel::setNode(KeyNode *node)
{
    m_node = node;

    if (!node) {
        m_rootStack->setCurrentWidget(m_emptyPage);
        return;
    }
    m_rootStack->setCurrentWidget(m_mainPage);

    blockAllSignals(true);

    // Common fields
    m_titleLabel->setText(node->typeString());
    m_typeLabel->setText(node->comment().isEmpty() ? "—" : node->comment());
    m_posX->setValue(node->relativePosition().x());
    m_posY->setValue(node->relativePosition().y());
    m_keyEdit->setCapturedKeyString(node->keyCode());
    m_commentEdit->setText(node->comment());
    m_switchMapCheck->setChecked(node->switchMap());

    // Type-specific page
    switch (node->nodeType()) {
        case KeyNode::Click:
        case KeyNode::ClickTwice:
        case KeyNode::ClickMulti:
            m_stack->setCurrentIndex(0); // empty/click page
            break;
        case KeyNode::Drag: {
            m_stack->setCurrentIndex(1);
            auto *dn = static_cast<DragNode*>(node);
            m_dragEndX->setValue(dn->endPosition().x());
            m_dragEndY->setValue(dn->endPosition().y());
            m_dragSpeed->setValue(dn->dragSpeed());
            break;
        }
        case KeyNode::SteerWheel: {
            m_stack->setCurrentIndex(2);
            auto *sw = static_cast<SteerWheelNode*>(node);
            m_wasdL->setCapturedKeyString(sw->leftKey());
            m_wasdR->setCapturedKeyString(sw->rightKey());
            m_wasdU->setCapturedKeyString(sw->upKey());
            m_wasdD->setCapturedKeyString(sw->downKey());
            m_offL->setValue(sw->leftOffset());
            m_offR->setValue(sw->rightOffset());
            m_offU->setValue(sw->upOffset());
            m_offD->setValue(sw->downOffset());
            break;
        }
        case KeyNode::Gesture: {
            m_stack->setCurrentIndex(3);
            auto *gn = static_cast<GestureNode*>(node);
            m_gestureTypeCombo->setCurrentIndex(static_cast<int>(gn->gestureType()));
            m_gestureDurationSpin->setValue(gn->duration());
            m_gestureFingerInfo->setText(QString("%1 finger path(s)").arg(gn->fingerPaths().size()));
            break;
        }
    }

    blockAllSignals(false);
}

// ============================================================================
// Slots — push edits into the node
// ============================================================================

void PropertiesPanel::onPosXChanged(double v)
{
    if (!m_node) return;
    QPointF p = m_node->relativePosition();
    p.setX(v);
    m_node->setRelativePosition(p);
    emit nodeModified();
}

void PropertiesPanel::onPosYChanged(double v)
{
    if (!m_node) return;
    QPointF p = m_node->relativePosition();
    p.setY(v);
    m_node->setRelativePosition(p);
    emit nodeModified();
}

void PropertiesPanel::onKeyChanged(const QString &key)
{
    if (!m_node) return;
    m_node->setKeyCode(key);
    m_node->update();
    emit nodeModified();
}

void PropertiesPanel::onCommentChanged(const QString &text)
{
    if (!m_node) return;
    m_node->setComment(text);
    m_typeLabel->setText(text.isEmpty() ? "—" : text);
    emit nodeModified();
}

void PropertiesPanel::onSwitchMapToggled(bool v)
{
    if (!m_node) return;
    m_node->setSwitchMap(v);
    emit nodeModified();
}

void PropertiesPanel::onDragEndXChanged(double v)
{
    if (!m_node || m_node->nodeType() != KeyNode::Drag) return;
    auto *dn = static_cast<DragNode*>(m_node);
    QPointF ep = dn->endPosition();
    ep.setX(v);
    dn->setEndPosition(ep);
    emit nodeModified();
}

void PropertiesPanel::onDragEndYChanged(double v)
{
    if (!m_node || m_node->nodeType() != KeyNode::Drag) return;
    auto *dn = static_cast<DragNode*>(m_node);
    QPointF ep = dn->endPosition();
    ep.setY(v);
    dn->setEndPosition(ep);
    emit nodeModified();
}

void PropertiesPanel::onDragSpeedChanged(double v)
{
    if (!m_node || m_node->nodeType() != KeyNode::Drag) return;
    static_cast<DragNode*>(m_node)->setDragSpeed(v);
    emit nodeModified();
}

void PropertiesPanel::onWASDKeyChanged()
{
    if (!m_node || m_node->nodeType() != KeyNode::SteerWheel) return;
    auto *sw = static_cast<SteerWheelNode*>(m_node);
    sw->setDirectionKeys(
        m_wasdL->capturedKeyString(),
        m_wasdR->capturedKeyString(),
        m_wasdU->capturedKeyString(),
        m_wasdD->capturedKeyString()
    );
    emit nodeModified();
}

void PropertiesPanel::onWASDOffsetChanged()
{
    if (!m_node || m_node->nodeType() != KeyNode::SteerWheel) return;
    auto *sw = static_cast<SteerWheelNode*>(m_node);
    sw->setOffsets(m_offL->value(), m_offR->value(),
                   m_offU->value(), m_offD->value());
    emit nodeModified();
}

// ============================================================================
// Build UI
// ============================================================================

void PropertiesPanel::buildUI()
{
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    m_rootStack = new QStackedWidget;
    outerLayout->addWidget(m_rootStack);

    // ---------- Empty page (no selection) ----------
    m_emptyPage = new QWidget;
    {
        QVBoxLayout *el = new QVBoxLayout(m_emptyPage);
        QLabel *hint = new QLabel("Select a node to\nedit its properties");
        hint->setAlignment(Qt::AlignCenter);
        hint->setStyleSheet("color: #888; font-size: 13px;");
        el->addStretch();
        el->addWidget(hint);
        el->addStretch();
    }
    m_rootStack->addWidget(m_emptyPage);

    // ---------- Main page ----------
    m_mainPage = new QWidget;
    QVBoxLayout *mainLay = new QVBoxLayout(m_mainPage);
    mainLay->setContentsMargins(6, 6, 6, 6);

    // Title
    m_titleLabel = new QLabel("Node");
    m_titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #4285F4;");
    mainLay->addWidget(m_titleLabel);

    m_typeLabel = new QLabel("—");
    m_typeLabel->setStyleSheet("font-size: 11px; color: #aaa; margin-bottom: 6px;");
    mainLay->addWidget(m_typeLabel);

    // -- Common group --
    QGroupBox *commonBox = new QGroupBox("General");
    QGridLayout *cg = new QGridLayout(commonBox);

    auto makeSpin = [](double min, double max, double step, int dec) {
        auto *s = new QDoubleSpinBox;
        s->setRange(min, max);
        s->setSingleStep(step);
        s->setDecimals(dec);
        return s;
    };

    cg->addWidget(new QLabel("Pos X:"), 0, 0);
    m_posX = makeSpin(0, 1, 0.01, 3);
    cg->addWidget(m_posX, 0, 1);

    cg->addWidget(new QLabel("Pos Y:"), 1, 0);
    m_posY = makeSpin(0, 1, 0.01, 3);
    cg->addWidget(m_posY, 1, 1);

    cg->addWidget(new QLabel("Key:"), 2, 0);
    m_keyEdit = new KeyCaptureEdit;
    cg->addWidget(m_keyEdit, 2, 1);

    cg->addWidget(new QLabel("Comment:"), 3, 0);
    m_commentEdit = new QLineEdit;
    cg->addWidget(m_commentEdit, 3, 1);

    m_switchMapCheck = new QCheckBox("Switch Map");
    cg->addWidget(m_switchMapCheck, 4, 0, 1, 2);

    mainLay->addWidget(commonBox);

    // -- Type-specific stacked --
    m_stack = new QStackedWidget;

    // Page 0: Click (no extra fields)
    {
        QWidget *p = new QWidget;
        QVBoxLayout *l = new QVBoxLayout(p);
        l->addStretch();
        m_stack->addWidget(p);
    }

    // Page 1: Drag
    {
        QGroupBox *box = new QGroupBox("Drag Settings");
        QGridLayout *g = new QGridLayout(box);
        g->addWidget(new QLabel("End X:"), 0, 0);
        m_dragEndX = makeSpin(0, 1, 0.01, 3);
        g->addWidget(m_dragEndX, 0, 1);
        g->addWidget(new QLabel("End Y:"), 1, 0);
        m_dragEndY = makeSpin(0, 1, 0.01, 3);
        g->addWidget(m_dragEndY, 1, 1);
        g->addWidget(new QLabel("Speed:"), 2, 0);
        m_dragSpeed = makeSpin(0, 1, 0.1, 2);
        m_dragSpeed->setValue(1.0);
        g->addWidget(m_dragSpeed, 2, 1);
        m_stack->addWidget(box);
    }

    // Page 2: WASD
    {
        QGroupBox *box = new QGroupBox("Steering Wheel");
        QGridLayout *g = new QGridLayout(box);
        int r = 0;
        auto addKRow = [&](const QString &label, KeyCaptureEdit *&e, const QString &def) {
            g->addWidget(new QLabel(label), r, 0);
            e = new KeyCaptureEdit;
            e->setCapturedKeyString(def);
            g->addWidget(e, r, 1);
            ++r;
        };
        addKRow("Left:",  m_wasdL, "Key_A");
        addKRow("Right:", m_wasdR, "Key_D");
        addKRow("Up:",    m_wasdU, "Key_W");
        addKRow("Down:",  m_wasdD, "Key_S");

        auto addORow = [&](const QString &label, QDoubleSpinBox *&s, double def) {
            g->addWidget(new QLabel(label), r, 0);
            s = makeSpin(0, 1, 0.01, 3);
            s->setValue(def);
            g->addWidget(s, r, 1);
            ++r;
        };
        addORow("L Off:", m_offL, 0.1);
        addORow("R Off:", m_offR, 0.1);
        addORow("U Off:", m_offU, 0.1);
        addORow("D Off:", m_offD, 0.1);
        m_stack->addWidget(box);
    }

    // Page 3: Gesture
    {
        QGroupBox *box = new QGroupBox("Gesture Settings");
        QGridLayout *g = new QGridLayout(box);
        g->addWidget(new QLabel("Type:"), 0, 0);
        m_gestureTypeCombo = new QComboBox;
        m_gestureTypeCombo->addItems({
            "Pinch In", "Pinch Out",
            "2F Swipe Up", "2F Swipe Down",
            "2F Swipe Left", "2F Swipe Right",
            "Rotate", "Custom"
        });
        g->addWidget(m_gestureTypeCombo, 0, 1);
        g->addWidget(new QLabel("Duration (ms):"), 1, 0);
        m_gestureDurationSpin = new QSpinBox;
        m_gestureDurationSpin->setRange(50, 5000);
        m_gestureDurationSpin->setSingleStep(50);
        m_gestureDurationSpin->setValue(400);
        g->addWidget(m_gestureDurationSpin, 1, 1);
        m_gestureFingerInfo = new QLabel("2 finger path(s)");
        m_gestureFingerInfo->setStyleSheet("color: #aaa; font-size: 11px;");
        g->addWidget(m_gestureFingerInfo, 2, 0, 1, 2);
        m_stack->addWidget(box);
    }

    mainLay->addWidget(m_stack);
    mainLay->addStretch();

    m_rootStack->addWidget(m_mainPage);
    m_rootStack->setCurrentWidget(m_emptyPage);

    // ---------- Connections ----------
    connect(m_posX,   QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertiesPanel::onPosXChanged);
    connect(m_posY,   QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertiesPanel::onPosYChanged);
    connect(m_keyEdit, &KeyCaptureEdit::keyCaptured,
            this, &PropertiesPanel::onKeyChanged);
    connect(m_commentEdit, &QLineEdit::textChanged,
            this, &PropertiesPanel::onCommentChanged);
    connect(m_switchMapCheck, &QCheckBox::toggled,
            this, &PropertiesPanel::onSwitchMapToggled);

    // Drag
    connect(m_dragEndX, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertiesPanel::onDragEndXChanged);
    connect(m_dragEndY, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertiesPanel::onDragEndYChanged);
    connect(m_dragSpeed, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertiesPanel::onDragSpeedChanged);

    // WASD keys
    for (auto *e : {m_wasdL, m_wasdR, m_wasdU, m_wasdD})
        connect(e, &KeyCaptureEdit::keyCaptured, this, &PropertiesPanel::onWASDKeyChanged);

    // WASD offsets
    for (auto *s : {m_offL, m_offR, m_offU, m_offD})
        connect(s, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &PropertiesPanel::onWASDOffsetChanged);

    // Gesture
    connect(m_gestureTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PropertiesPanel::onGestureTypeChanged);
    connect(m_gestureDurationSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PropertiesPanel::onGestureDurationChanged);
}

void PropertiesPanel::blockAllSignals(bool block)
{
    m_posX->blockSignals(block);
    m_posY->blockSignals(block);
    m_keyEdit->blockSignals(block);
    m_commentEdit->blockSignals(block);
    m_switchMapCheck->blockSignals(block);
    m_dragEndX->blockSignals(block);
    m_dragEndY->blockSignals(block);
    m_dragSpeed->blockSignals(block);
    m_wasdL->blockSignals(block);
    m_wasdR->blockSignals(block);
    m_wasdU->blockSignals(block);
    m_wasdD->blockSignals(block);
    m_offL->blockSignals(block);
    m_offR->blockSignals(block);
    m_offU->blockSignals(block);
    m_offD->blockSignals(block);
    m_gestureTypeCombo->blockSignals(block);
    m_gestureDurationSpin->blockSignals(block);
}

void PropertiesPanel::onGestureTypeChanged(int idx)
{
    if (!m_node || m_node->nodeType() != KeyNode::Gesture) return;
    auto *gn = static_cast<GestureNode*>(m_node);
    auto type = static_cast<GestureNode::GestureType>(idx);
    gn->applyPreset(type);
    m_gestureFingerInfo->setText(QString("%1 finger path(s)").arg(gn->fingerPaths().size()));
    emit nodeModified();
}

void PropertiesPanel::onGestureDurationChanged(int ms)
{
    if (!m_node || m_node->nodeType() != KeyNode::Gesture) return;
    static_cast<GestureNode*>(m_node)->setDuration(ms);
    emit nodeModified();
}
