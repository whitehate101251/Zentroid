#include "keyassigndialog.h"
#include "keynode.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QLabel>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMetaEnum>
#include <QSpinBox>
#include <QScrollArea>

// ============================================================================
// KeyCaptureEdit
// ============================================================================

KeyCaptureEdit::KeyCaptureEdit(QWidget *parent)
    : QLineEdit(parent)
{
    setReadOnly(true);
    setAlignment(Qt::AlignCenter);
    setPlaceholderText("Press a key...");
    setStyleSheet(
        "KeyCaptureEdit {"
        "  border: 2px solid #4285F4;"
        "  border-radius: 4px;"
        "  padding: 6px 12px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "  background: #2a2a2a;"
        "  color: white;"
        "  min-width: 120px;"
        "}"
        "KeyCaptureEdit:focus {"
        "  border-color: #EA4335;"
        "  background: #333;"
        "}"
    );
}

void KeyCaptureEdit::setCapturedKeyString(const QString &key)
{
    m_keyString = key;
    QString display = key;
    if (display.startsWith("Key_"))
        display = display.mid(4);
    // Friendly display names for extra mouse buttons
    static const QMap<QString, QString> displayNames = {
        {"XButton1", "Mouse4 (Back)"},
        {"XButton2", "Mouse5 (Forward)"},
    };
    if (displayNames.contains(display))
        display = displayNames[display];
    setText(display);
}

void KeyCaptureEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        clearFocus();
        return;
    }
    // F12 is reserved for toggling the keymap overlay — don't allow binding it
    if (event->key() == Qt::Key_F12) {
        return;
    }

    QString keyStr = qtKeyToString(event->key());
    if (!keyStr.isEmpty()) {
        m_keyString = keyStr;
        QString display = keyStr;
        if (display.startsWith("Key_"))
            display = display.mid(4);
        setText(display);
        emit keyCaptured(keyStr);
    }
}

void KeyCaptureEdit::mousePressEvent(QMouseEvent *event)
{
    // Allow mouse buttons to be captured too
    QString keyStr;
    if (event->button() == Qt::LeftButton)
        keyStr = "LeftButton";
    else if (event->button() == Qt::RightButton)
        keyStr = "RightButton";
    else if (event->button() == Qt::MiddleButton)
        keyStr = "MiddleButton";
    else if (event->button() == Qt::BackButton || event->button() == Qt::XButton1)
        keyStr = "XButton1";
    else if (event->button() == Qt::ForwardButton || event->button() == Qt::XButton2)
        keyStr = "XButton2";

    if (!keyStr.isEmpty()) {
        m_keyString = keyStr;
        // Friendly display names for extra mouse buttons
        static const QMap<QString, QString> displayNames = {
            {"XButton1", "Mouse4 (Back)"},
            {"XButton2", "Mouse5 (Forward)"},
        };
        setText(displayNames.value(keyStr, keyStr));
        emit keyCaptured(keyStr);
        return;
    }

    QLineEdit::mousePressEvent(event);
}

QString KeyCaptureEdit::qtKeyToString(int key) const
{
    // Map common Qt::Key values to Zentroid's expected strings
    static const QMap<int, QString> keyMap = {
        {Qt::Key_A, "Key_A"}, {Qt::Key_B, "Key_B"}, {Qt::Key_C, "Key_C"},
        {Qt::Key_D, "Key_D"}, {Qt::Key_E, "Key_E"}, {Qt::Key_F, "Key_F"},
        {Qt::Key_G, "Key_G"}, {Qt::Key_H, "Key_H"}, {Qt::Key_I, "Key_I"},
        {Qt::Key_J, "Key_J"}, {Qt::Key_K, "Key_K"}, {Qt::Key_L, "Key_L"},
        {Qt::Key_M, "Key_M"}, {Qt::Key_N, "Key_N"}, {Qt::Key_O, "Key_O"},
        {Qt::Key_P, "Key_P"}, {Qt::Key_Q, "Key_Q"}, {Qt::Key_R, "Key_R"},
        {Qt::Key_S, "Key_S"}, {Qt::Key_T, "Key_T"}, {Qt::Key_U, "Key_U"},
        {Qt::Key_V, "Key_V"}, {Qt::Key_W, "Key_W"}, {Qt::Key_X, "Key_X"},
        {Qt::Key_Y, "Key_Y"}, {Qt::Key_Z, "Key_Z"},
        {Qt::Key_0, "Key_0"}, {Qt::Key_1, "Key_1"}, {Qt::Key_2, "Key_2"},
        {Qt::Key_3, "Key_3"}, {Qt::Key_4, "Key_4"}, {Qt::Key_5, "Key_5"},
        {Qt::Key_6, "Key_6"}, {Qt::Key_7, "Key_7"}, {Qt::Key_8, "Key_8"},
        {Qt::Key_9, "Key_9"},
        {Qt::Key_Space, "Key_Space"},
        {Qt::Key_Return, "Key_Return"},   {Qt::Key_Enter, "Key_Enter"},
        {Qt::Key_Tab, "Key_Tab"},
        {Qt::Key_Escape, "Key_Escape"},
        {Qt::Key_Backspace, "Key_Backspace"},
        {Qt::Key_Shift, "Key_Shift"},     {Qt::Key_Control, "Key_Control"},
        {Qt::Key_Alt, "Key_Alt"},         {Qt::Key_Meta, "Key_Meta"},
        {Qt::Key_Up, "Key_Up"},           {Qt::Key_Down, "Key_Down"},
        {Qt::Key_Left, "Key_Left"},       {Qt::Key_Right, "Key_Right"},
        {Qt::Key_F1, "Key_F1"},   {Qt::Key_F2, "Key_F2"},
        {Qt::Key_F3, "Key_F3"},   {Qt::Key_F4, "Key_F4"},
        {Qt::Key_F5, "Key_F5"},   {Qt::Key_F6, "Key_F6"},
        {Qt::Key_F7, "Key_F7"},   {Qt::Key_F8, "Key_F8"},
        {Qt::Key_F9, "Key_F9"},   {Qt::Key_F10, "Key_F10"},
        {Qt::Key_F11, "Key_F11"},
        // F12 intentionally excluded — reserved for overlay toggle
        {Qt::Key_Equal, "Key_Equal"},
        {Qt::Key_Minus, "Key_Minus"},
        {Qt::Key_BracketLeft, "Key_BracketLeft"},
        {Qt::Key_BracketRight, "Key_BracketRight"},
        {Qt::Key_Semicolon, "Key_Semicolon"},
        {Qt::Key_Apostrophe, "Key_Apostrophe"},
        {Qt::Key_Comma, "Key_Comma"},
        {Qt::Key_Period, "Key_Period"},
        {Qt::Key_Slash, "Key_Slash"},
        {Qt::Key_Backslash, "Key_Backslash"},
        {Qt::Key_QuoteLeft, "Key_QuoteLeft"},
        {Qt::Key_Delete, "Key_Delete"},
        {Qt::Key_Home, "Key_Home"},
        {Qt::Key_End, "Key_End"},
        {Qt::Key_PageUp, "Key_PageUp"},
        {Qt::Key_PageDown, "Key_PageDown"},
        {Qt::Key_Insert, "Key_Insert"},
        {Qt::Key_CapsLock, "Key_CapsLock"},
    };

    if (keyMap.contains(key))
        return keyMap[key];

    // Fallback: use Qt's enum name
    QMetaEnum metaEnum = QMetaEnum::fromType<Qt::Key>();
    const char *name = metaEnum.valueToKey(key);
    if (name)
        return QString("Key_") + QString(name).mid(4); // strip "Key_"

    return {};
}

// ============================================================================
// KeyAssignDialog
// ============================================================================

KeyAssignDialog::KeyAssignDialog(Mode mode, QWidget *parent)
    : QDialog(parent)
    , m_mode(mode)
{
    setWindowTitle(mode == EditNode ? "Edit Key Mapping" : "New Key Mapping");
    setMinimumWidth(380);
    buildUI();
}

// --- Getters ---------------------------------------------------------------

QString KeyAssignDialog::keyCode() const       { return m_keyEdit->capturedKeyString(); }
QString KeyAssignDialog::comment() const       { return m_commentEdit->text(); }
bool    KeyAssignDialog::switchMap() const      { return m_switchMapCheck->isChecked(); }
int     KeyAssignDialog::nodeTypeIndex() const  { return m_typeCombo->currentIndex(); }

QPointF KeyAssignDialog::dragEndPos() const     { return {m_dragEndX->value(), m_dragEndY->value()}; }
double  KeyAssignDialog::dragSpeed() const      { return m_dragSpeedSpin->value(); }

QString KeyAssignDialog::leftKey() const        { return m_wasdLeft->capturedKeyString(); }
QString KeyAssignDialog::rightKey() const       { return m_wasdRight->capturedKeyString(); }
QString KeyAssignDialog::upKey() const          { return m_wasdUp->capturedKeyString(); }
QString KeyAssignDialog::downKey() const        { return m_wasdDown->capturedKeyString(); }
double  KeyAssignDialog::leftOffset() const     { return m_offLeft->value(); }
double  KeyAssignDialog::rightOffset() const    { return m_offRight->value(); }
double  KeyAssignDialog::upOffset() const       { return m_offUp->value(); }
double  KeyAssignDialog::downOffset() const     { return m_offDown->value(); }

// --- Setters (for Edit mode) -----------------------------------------------

void KeyAssignDialog::setKeyCode(const QString &key)   { m_keyEdit->setCapturedKeyString(key); }
void KeyAssignDialog::setComment(const QString &c)     { m_commentEdit->setText(c); }
void KeyAssignDialog::setSwitchMap(bool v)              { m_switchMapCheck->setChecked(v); }
void KeyAssignDialog::setNodeTypeIndex(int idx)         { m_typeCombo->setCurrentIndex(idx); }

void KeyAssignDialog::setDragEndPos(const QPointF &p)   { m_dragEndX->setValue(p.x()); m_dragEndY->setValue(p.y()); }
void KeyAssignDialog::setDragSpeed(double s)            { m_dragSpeedSpin->setValue(s); }

void KeyAssignDialog::setWASDKeys(const QString &l, const QString &r,
                                   const QString &u, const QString &d)
{
    m_wasdLeft->setCapturedKeyString(l);
    m_wasdRight->setCapturedKeyString(r);
    m_wasdUp->setCapturedKeyString(u);
    m_wasdDown->setCapturedKeyString(d);
}

void KeyAssignDialog::setWASDOffsets(double l, double r, double u, double d)
{
    m_offLeft->setValue(l);
    m_offRight->setValue(r);
    m_offUp->setValue(u);
    m_offDown->setValue(d);
}

QList<KeyAssignDialog::ClickPointEntry> KeyAssignDialog::clickMultiPoints() const
{
    QList<ClickPointEntry> points;
    for (const ClickPointRow &row : m_clickPointRows) {
        ClickPointEntry e;
        e.delay = row.delay->value();
        e.pos = QPointF(row.x->value(), row.y->value());
        points.append(e);
    }
    return points;
}

void KeyAssignDialog::setClickMultiPoints(const QList<ClickPointEntry> &points)
{
    // Clear existing rows
    m_clickPointRows.clear();
    
    // Remove all children from the layout except the add/stretch
    QLayoutItem *item;
    while ((item = m_clickMultiLayout->takeAt(0)) != nullptr) {
        if (item->widget())
            delete item->widget();
        delete item;
    }
    
    // Re-add each point
    for (const ClickPointEntry &pt : points) {
        ClickPointRow row;
        QGroupBox *box = new QGroupBox(QString("Click %1").arg(m_clickPointRows.size() + 1));
        QGridLayout *g = new QGridLayout(box);
        g->addWidget(new QLabel("X:"), 0, 0);
        row.x = new QDoubleSpinBox;
        row.x->setRange(0.0, 1.0); row.x->setDecimals(3); row.x->setSingleStep(0.01);
        row.x->setValue(pt.pos.x());
        g->addWidget(row.x, 0, 1);
        g->addWidget(new QLabel("Y:"), 1, 0);
        row.y = new QDoubleSpinBox;
        row.y->setRange(0.0, 1.0); row.y->setDecimals(3); row.y->setSingleStep(0.01);
        row.y->setValue(pt.pos.y());
        g->addWidget(row.y, 1, 1);
        g->addWidget(new QLabel("Delay (ms):"), 2, 0);
        row.delay = new QSpinBox;
        row.delay->setRange(0, 10000); row.delay->setSingleStep(50);
        row.delay->setValue(pt.delay);
        g->addWidget(row.delay, 2, 1);
        m_clickPointRows.append(row);
        m_clickMultiLayout->addWidget(box);
    }
    
    // Re-add the add button + stretch
    QPushButton *addBtn = new QPushButton("+ Add Click Point");
    connect(addBtn, &QPushButton::clicked, this, [this, addBtn]() {
        ClickPointEntry newPt;
        newPt.delay = 500;
        newPt.pos = QPointF(0.5, 0.5);
        // Minimal version: just add one more row manually
        ClickPointRow row;
        QGroupBox *box = new QGroupBox(QString("Click %1").arg(m_clickPointRows.size() + 1));
        QGridLayout *g = new QGridLayout(box);
        g->addWidget(new QLabel("X:"), 0, 0);
        row.x = new QDoubleSpinBox;
        row.x->setRange(0.0, 1.0); row.x->setDecimals(3); row.x->setSingleStep(0.01);
        row.x->setValue(0.5);
        g->addWidget(row.x, 0, 1);
        g->addWidget(new QLabel("Y:"), 1, 0);
        row.y = new QDoubleSpinBox;
        row.y->setRange(0.0, 1.0); row.y->setDecimals(3); row.y->setSingleStep(0.01);
        row.y->setValue(0.5);
        g->addWidget(row.y, 1, 1);
        g->addWidget(new QLabel("Delay (ms):"), 2, 0);
        row.delay = new QSpinBox;
        row.delay->setRange(0, 10000); row.delay->setSingleStep(50);
        row.delay->setValue(500);
        g->addWidget(row.delay, 2, 1);
        m_clickPointRows.append(row);
        // Insert before the last item (stretch)
        int insertIdx = m_clickMultiLayout->count() - 1;
        m_clickMultiLayout->insertWidget(insertIdx, box);
    });
    m_clickMultiLayout->addWidget(addBtn);
    m_clickMultiLayout->addStretch();
}

// --- Build UI --------------------------------------------------------------

void KeyAssignDialog::buildUI()
{
    QVBoxLayout *root = new QVBoxLayout(this);

    // ------ Type combo (hidden when mode is fixed) ------
    QHBoxLayout *typeRow = new QHBoxLayout;
    typeRow->addWidget(new QLabel("Type:"));
    m_typeCombo = new QComboBox;
    m_typeCombo->addItems({"Click", "Click Twice", "Drag", "WASD Steering", "Click Multi", "Gesture"});
    typeRow->addWidget(m_typeCombo, 1);
    root->addLayout(typeRow);

    // Pre-select based on mode
    switch (m_mode) {
        case CreateClick:      m_typeCombo->setCurrentIndex(0); break;
        case CreateClickTwice: m_typeCombo->setCurrentIndex(1); break;
        case CreateDrag:       m_typeCombo->setCurrentIndex(2); break;
        case CreateWASD:       m_typeCombo->setCurrentIndex(3); break;
        case CreateClickMulti: m_typeCombo->setCurrentIndex(4); break;
        case CreateGesture:    m_typeCombo->setCurrentIndex(5); break;
        case EditNode:         break;
    }
    if (m_mode != EditNode)
        m_typeCombo->setEnabled(false);

    // ------ Common: Key + Comment + switchMap ------
    QGroupBox *commonBox = new QGroupBox("Key Binding");
    QGridLayout *cg = new QGridLayout(commonBox);

    cg->addWidget(new QLabel("Key:"), 0, 0);
    m_keyEdit = new KeyCaptureEdit;
    cg->addWidget(m_keyEdit, 0, 1);

    cg->addWidget(new QLabel("Comment:"), 1, 0);
    m_commentEdit = new QLineEdit;
    m_commentEdit->setPlaceholderText("e.g. shoot, jump, reload...");
    cg->addWidget(m_commentEdit, 1, 1);

    m_switchMapCheck = new QCheckBox("Switch Map (release mouse on press)");
    cg->addWidget(m_switchMapCheck, 2, 0, 1, 2);

    root->addWidget(commonBox);

    // ------ Stacked pages for type-specific settings ------
    m_stack = new QStackedWidget;

    // Page 0 & 1: Click / ClickTwice — no extra settings (empty widget)
    buildClickPage();
    // Page 2: Drag
    buildDragPage();
    // Page 3: WASD
    buildWASDPage();
    // Page 4: ClickMulti
    buildClickMultiPage();
    // Page 5: Gesture
    buildGesturePage();

    root->addWidget(m_stack);

    // Wire combo → stack
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int idx) {
        // Click=0 ClickTwice=1 → page 0, Drag=2 → page 1, WASD=3 → page 2,
        // ClickMulti=4 → page 3, Gesture=5 → page 4
        if (idx <= 1) m_stack->setCurrentIndex(0);
        else if (idx == 2) m_stack->setCurrentIndex(1);
        else if (idx == 3) m_stack->setCurrentIndex(2);
        else if (idx == 4) m_stack->setCurrentIndex(3);
        else m_stack->setCurrentIndex(4);
    });

    // Set initial page
    int idx = m_typeCombo->currentIndex();
    if (idx <= 1) m_stack->setCurrentIndex(0);
    else if (idx == 2) m_stack->setCurrentIndex(1);
    else if (idx == 3) m_stack->setCurrentIndex(2);
    else if (idx == 4) m_stack->setCurrentIndex(3);
    else m_stack->setCurrentIndex(4);

    // ------ OK / Cancel ------
    QDialogButtonBox *bb = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(bb);
}

void KeyAssignDialog::buildClickPage()
{
    // Empty page for Click / ClickTwice — nothing extra needed
    QWidget *page = new QWidget;
    QVBoxLayout *l = new QVBoxLayout(page);
    l->addWidget(new QLabel("Click on the canvas to set position."));
    l->addStretch();
    m_stack->addWidget(page);
}

void KeyAssignDialog::buildDragPage()
{
    QWidget *page = new QWidget;
    QGridLayout *g = new QGridLayout(page);

    g->addWidget(new QLabel("Drag End X:"), 0, 0);
    m_dragEndX = new QDoubleSpinBox;
    m_dragEndX->setRange(0.0, 1.0);
    m_dragEndX->setDecimals(3);
    m_dragEndX->setSingleStep(0.01);
    m_dragEndX->setValue(0.5);
    g->addWidget(m_dragEndX, 0, 1);

    g->addWidget(new QLabel("Drag End Y:"), 1, 0);
    m_dragEndY = new QDoubleSpinBox;
    m_dragEndY->setRange(0.0, 1.0);
    m_dragEndY->setDecimals(3);
    m_dragEndY->setSingleStep(0.01);
    m_dragEndY->setValue(0.5);
    g->addWidget(m_dragEndY, 1, 1);

    g->addWidget(new QLabel("Drag Speed:"), 2, 0);
    m_dragSpeedSpin = new QDoubleSpinBox;
    m_dragSpeedSpin->setRange(0.0, 1.0);
    m_dragSpeedSpin->setDecimals(2);
    m_dragSpeedSpin->setSingleStep(0.1);
    m_dragSpeedSpin->setValue(1.0);
    g->addWidget(m_dragSpeedSpin, 2, 1);

    g->setRowStretch(3, 1);
    m_stack->addWidget(page);
}

void KeyAssignDialog::buildWASDPage()
{
    QWidget *page = new QWidget;
    QGridLayout *g = new QGridLayout(page);

    int row = 0;
    auto addKeyRow = [&](const QString &label, KeyCaptureEdit *&edit, const QString &def) {
        g->addWidget(new QLabel(label), row, 0);
        edit = new KeyCaptureEdit;
        edit->setCapturedKeyString(def);
        g->addWidget(edit, row, 1);
        ++row;
    };
    addKeyRow("Left Key:",  m_wasdLeft,  "Key_A");
    addKeyRow("Right Key:", m_wasdRight, "Key_D");
    addKeyRow("Up Key:",    m_wasdUp,    "Key_W");
    addKeyRow("Down Key:",  m_wasdDown,  "Key_S");

    auto addOffsetRow = [&](const QString &label, QDoubleSpinBox *&spin, double def) {
        g->addWidget(new QLabel(label), row, 0);
        spin = new QDoubleSpinBox;
        spin->setRange(0.0, 1.0);
        spin->setDecimals(3);
        spin->setSingleStep(0.01);
        spin->setValue(def);
        g->addWidget(spin, row, 1);
        ++row;
    };
    addOffsetRow("Left Offset:",  m_offLeft,  0.1);
    addOffsetRow("Right Offset:", m_offRight, 0.1);
    addOffsetRow("Up Offset:",    m_offUp,    0.1);
    addOffsetRow("Down Offset:",  m_offDown,  0.1);

    g->setRowStretch(row, 1);
    m_stack->addWidget(page);
}

void KeyAssignDialog::buildClickMultiPage()
{
    m_clickMultiPage = new QWidget;
    m_clickMultiLayout = new QVBoxLayout(m_clickMultiPage);
    
    QLabel *info = new QLabel("Define sequential click points.\nEach point will be tapped after its delay.");
    info->setWordWrap(true);
    info->setStyleSheet("color: #aaa; font-size: 11px; margin-bottom: 4px;");
    m_clickMultiLayout->addWidget(info);
    
    // Start with 2 default click points
    for (int i = 0; i < 2; ++i) {
        ClickPointRow row;
        QGroupBox *box = new QGroupBox(QString("Click %1").arg(i + 1));
        QGridLayout *g = new QGridLayout(box);
        g->addWidget(new QLabel("X:"), 0, 0);
        row.x = new QDoubleSpinBox;
        row.x->setRange(0.0, 1.0); row.x->setDecimals(3); row.x->setSingleStep(0.01);
        row.x->setValue(0.5);
        g->addWidget(row.x, 0, 1);
        g->addWidget(new QLabel("Y:"), 1, 0);
        row.y = new QDoubleSpinBox;
        row.y->setRange(0.0, 1.0); row.y->setDecimals(3); row.y->setSingleStep(0.01);
        row.y->setValue(0.5);
        g->addWidget(row.y, 1, 1);
        g->addWidget(new QLabel("Delay (ms):"), 2, 0);
        row.delay = new QSpinBox;
        row.delay->setRange(0, 10000); row.delay->setSingleStep(50);
        row.delay->setValue(500);
        g->addWidget(row.delay, 2, 1);
        m_clickPointRows.append(row);
        m_clickMultiLayout->addWidget(box);
    }
    
    // Add point button
    QPushButton *addBtn = new QPushButton("+ Add Click Point");
    connect(addBtn, &QPushButton::clicked, this, [this]() {
        ClickPointRow row;
        QGroupBox *box = new QGroupBox(QString("Click %1").arg(m_clickPointRows.size() + 1));
        QGridLayout *g = new QGridLayout(box);
        g->addWidget(new QLabel("X:"), 0, 0);
        row.x = new QDoubleSpinBox;
        row.x->setRange(0.0, 1.0); row.x->setDecimals(3); row.x->setSingleStep(0.01);
        row.x->setValue(0.5);
        g->addWidget(row.x, 0, 1);
        g->addWidget(new QLabel("Y:"), 1, 0);
        row.y = new QDoubleSpinBox;
        row.y->setRange(0.0, 1.0); row.y->setDecimals(3); row.y->setSingleStep(0.01);
        row.y->setValue(0.5);
        g->addWidget(row.y, 1, 1);
        g->addWidget(new QLabel("Delay (ms):"), 2, 0);
        row.delay = new QSpinBox;
        row.delay->setRange(0, 10000); row.delay->setSingleStep(50);
        row.delay->setValue(500);
        g->addWidget(row.delay, 2, 1);
        m_clickPointRows.append(row);
        int insertIdx = m_clickMultiLayout->count() - 1;  // before stretch
        m_clickMultiLayout->insertWidget(insertIdx, box);
    });
    m_clickMultiLayout->addWidget(addBtn);
    m_clickMultiLayout->addStretch();
    
    // Wrap in scroll area
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidget(m_clickMultiPage);
    scroll->setWidgetResizable(true);
    m_stack->addWidget(scroll);
}

void KeyAssignDialog::buildGesturePage()
{
    QWidget *page = new QWidget;
    QGridLayout *g = new QGridLayout(page);

    g->addWidget(new QLabel("Gesture Type:"), 0, 0);
    m_gestureTypeCombo = new QComboBox;
    m_gestureTypeCombo->addItems({
        "Pinch In (Zoom Out)",
        "Pinch Out (Zoom In)",
        "Two-Finger Swipe Up",
        "Two-Finger Swipe Down",
        "Two-Finger Swipe Left",
        "Two-Finger Swipe Right",
        "Rotate",
        "Custom"
    });
    g->addWidget(m_gestureTypeCombo, 0, 1);

    g->addWidget(new QLabel("Duration (ms):"), 1, 0);
    m_gestureDurationSpin = new QSpinBox;
    m_gestureDurationSpin->setRange(50, 5000);
    m_gestureDurationSpin->setSingleStep(50);
    m_gestureDurationSpin->setValue(400);
    g->addWidget(m_gestureDurationSpin, 1, 1);

    g->addWidget(new QLabel("Radius:"), 2, 0);
    m_gestureRadiusSpin = new QDoubleSpinBox;
    m_gestureRadiusSpin->setRange(0.01, 0.5);
    m_gestureRadiusSpin->setDecimals(3);
    m_gestureRadiusSpin->setSingleStep(0.01);
    m_gestureRadiusSpin->setValue(0.08);
    m_gestureRadiusSpin->setToolTip("Distance fingers travel from center (in relative coords)");
    g->addWidget(m_gestureRadiusSpin, 2, 1);

    QLabel *info = new QLabel(
        "Finger paths are auto-generated from the preset.\n"
        "Adjust radius to control gesture extent.\n"
        "The node position defines the gesture center.");
    info->setWordWrap(true);
    info->setStyleSheet("color: #aaa; font-size: 11px; margin-top: 8px;");
    g->addWidget(info, 3, 0, 1, 2);

    g->setRowStretch(4, 1);
    m_stack->addWidget(page);
}

// --- Gesture getters/setters -----------------------------------------------

int KeyAssignDialog::gestureTypeIndex() const
{
    return m_gestureTypeCombo->currentIndex();
}

void KeyAssignDialog::setGestureTypeIndex(int idx)
{
    m_gestureTypeCombo->setCurrentIndex(idx);
}

int KeyAssignDialog::gestureDuration() const
{
    return m_gestureDurationSpin->value();
}

void KeyAssignDialog::setGestureDuration(int ms)
{
    m_gestureDurationSpin->setValue(ms);
}

qreal KeyAssignDialog::gestureRadius() const
{
    return m_gestureRadiusSpin->value();
}

void KeyAssignDialog::setGestureRadius(qreal r)
{
    m_gestureRadiusSpin->setValue(r);
}
