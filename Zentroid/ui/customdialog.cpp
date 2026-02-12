#include "customdialog.h"
#include "toggleswitch.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

CustomDialog::CustomDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Custom Settings");
    setMinimumSize(460, 600);
    resize(480, 700);
    setupUI();
    applyStyles();
}

QWidget *CustomDialog::createToggleRow(const QString &label, ToggleSwitch *toggle)
{
    QWidget *row = new QWidget(this);
    row->setFixedHeight(40);
    row->setObjectName("toggleRow");
    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->setContentsMargins(12, 0, 12, 0);
    layout->setSpacing(12);

    QLabel *lbl = new QLabel(label, row);
    lbl->setObjectName("toggleLabel");
    layout->addWidget(lbl, 1);
    layout->addWidget(toggle, 0, Qt::AlignRight | Qt::AlignVCenter);

    return row;
}

void CustomDialog::setupUI()
{
    // Outer layout with scroll area for fullscreen support
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setObjectName("customScrollArea");
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);

    // Constrained content widget (max 800px, centered)
    QWidget *contentWidget = new QWidget(this);
    contentWidget->setMaximumWidth(800);
    QVBoxLayout *mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(24, 16, 24, 16);

    // ====== KEYMAP SECTION ======
    QGroupBox *keymapGroup = new QGroupBox("Keymap", this);
    keymapGroup->setObjectName("customGroup");
    QVBoxLayout *kmLayout = new QVBoxLayout(keymapGroup);
    kmLayout->setContentsMargins(14, 20, 14, 10);
    kmLayout->setSpacing(8);

    m_keymapBox = new QComboBox(this);
    m_keymapBox->setObjectName("keymapDropdown");
    kmLayout->addWidget(m_keymapBox);

    QHBoxLayout *kmBtns = new QHBoxLayout();
    kmBtns->setSpacing(8);

    QPushButton *refreshKmBtn = new QPushButton("Refresh", this);
    refreshKmBtn->setObjectName("kmActionBtn");
    refreshKmBtn->setCursor(Qt::PointingHandCursor);

    m_addKeymapBtn = new QPushButton("+ Add New", this);
    m_addKeymapBtn->setObjectName("kmAddNewBtn");
    m_addKeymapBtn->setCursor(Qt::PointingHandCursor);

    m_applyKeymapBtn = new QPushButton("Apply", this);
    m_applyKeymapBtn->setObjectName("kmApplyBtn");
    m_applyKeymapBtn->setCursor(Qt::PointingHandCursor);

    m_editKeymapBtn = new QPushButton("Edit Keymap", this);
    m_editKeymapBtn->setObjectName("kmActionBtn");
    m_editKeymapBtn->setCursor(Qt::PointingHandCursor);

    kmBtns->addWidget(refreshKmBtn);
    kmBtns->addWidget(m_addKeymapBtn);
    kmBtns->addStretch();
    kmBtns->addWidget(m_applyKeymapBtn);
    kmBtns->addWidget(m_editKeymapBtn);
    kmLayout->addLayout(kmBtns);

    mainLayout->addWidget(keymapGroup);

    // ====== PREFERENCES SECTION ======
    QGroupBox *prefGroup = new QGroupBox("Preferences", this);
    prefGroup->setObjectName("customGroup");
    QVBoxLayout *prefLayout = new QVBoxLayout(prefGroup);
    prefLayout->setContentsMargins(6, 20, 6, 10);
    prefLayout->setSpacing(0);

    m_recordScreenToggle = new ToggleSwitch(this);
    m_bgRecordToggle = new ToggleSwitch(this);
    m_reverseConnToggle = new ToggleSwitch(this);
    m_showFpsToggle = new ToggleSwitch(this);
    m_alwaysTopToggle = new ToggleSwitch(this);
    m_screenOffToggle = new ToggleSwitch(this);
    m_framelessToggle = new ToggleSwitch(this);
    m_stayAwakeToggle = new ToggleSwitch(this);
    m_showToolbarToggle = new ToggleSwitch(this);

    prefLayout->addWidget(createToggleRow("Record Screen", m_recordScreenToggle));
    prefLayout->addWidget(createToggleRow("Background Record", m_bgRecordToggle));
    prefLayout->addWidget(createToggleRow("Reverse Connection", m_reverseConnToggle));
    prefLayout->addWidget(createToggleRow("Show FPS", m_showFpsToggle));
    prefLayout->addWidget(createToggleRow("Always on Top", m_alwaysTopToggle));
    prefLayout->addWidget(createToggleRow("Screen Off", m_screenOffToggle));
    prefLayout->addWidget(createToggleRow("Frameless", m_framelessToggle));
    prefLayout->addWidget(createToggleRow("Stay Awake", m_stayAwakeToggle));
    prefLayout->addWidget(createToggleRow("Show Toolbar", m_showToolbarToggle));

    mainLayout->addWidget(prefGroup);

    // ====== ADB LOGS SECTION ======
    QGroupBox *logGroup = new QGroupBox("ADB Logs", this);
    logGroup->setObjectName("customGroup");
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
    logLayout->setContentsMargins(10, 20, 10, 10);

    m_logEdit = new QTextEdit(this);
    m_logEdit->setObjectName("logTextEdit");
    m_logEdit->setReadOnly(true);
    m_logEdit->setMinimumHeight(120);
    logLayout->addWidget(m_logEdit);

    QPushButton *clearLogBtn = new QPushButton("Clear Logs", this);
    clearLogBtn->setObjectName("kmActionBtn");
    clearLogBtn->setCursor(Qt::PointingHandCursor);
    logLayout->addWidget(clearLogBtn, 0, Qt::AlignRight);

    mainLayout->addWidget(logGroup, 1);  // stretch

    // ====== CLOSE BUTTON ======
    QHBoxLayout *bottomBtns = new QHBoxLayout();
    bottomBtns->addStretch();
    QPushButton *closeBtn = new QPushButton("Close", this);
    closeBtn->setObjectName("dialogCancelBtn");
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFixedWidth(90);
    bottomBtns->addWidget(closeBtn);
    mainLayout->addLayout(bottomBtns);

    // Set scroll area content and add to outer layout
    scrollArea->setWidget(contentWidget);
    scrollArea->setAlignment(Qt::AlignHCenter);
    outerLayout->addWidget(scrollArea, 1);

    // ====== CONNECTIONS ======
    connect(refreshKmBtn, &QPushButton::clicked, this, &CustomDialog::keymapRefreshRequested);
    connect(m_applyKeymapBtn, &QPushButton::clicked, this, [this]() {
        QString name = m_keymapBox->currentText();
        if (!name.isEmpty() && !name.startsWith(QString::fromUtf8("➕"))) {
            emit keymapApplyRequested(name);
        }
    });
    connect(m_editKeymapBtn, &QPushButton::clicked, this, &CustomDialog::keymapEditRequested);
    connect(m_addKeymapBtn, &QPushButton::clicked, this, &CustomDialog::keymapAddNewRequested);
    connect(clearLogBtn, &QPushButton::clicked, m_logEdit, &QTextEdit::clear);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);

    // Preference change signals (any toggle change notifies the parent)
    auto notifyChange = [this](bool) { emit preferencesChanged(); };
    connect(m_recordScreenToggle, &QAbstractButton::toggled, this, notifyChange);
    connect(m_bgRecordToggle, &QAbstractButton::toggled, this, notifyChange);
    connect(m_reverseConnToggle, &QAbstractButton::toggled, this, notifyChange);
    connect(m_showFpsToggle, &QAbstractButton::toggled, this, notifyChange);
    connect(m_alwaysTopToggle, &QAbstractButton::toggled, this, notifyChange);
    connect(m_screenOffToggle, &QAbstractButton::toggled, this, notifyChange);
    connect(m_framelessToggle, &QAbstractButton::toggled, this, notifyChange);
    connect(m_stayAwakeToggle, &QAbstractButton::toggled, this, notifyChange);
    connect(m_showToolbarToggle, &QAbstractButton::toggled, this, notifyChange);
}

void CustomDialog::applyStyles()
{
    QString css = R"(
        CustomDialog {
            background: @@bg;
        }
        #customScrollArea {
            background: @@bg;
            border: none;
        }
        #customGroup {
            border: 1px solid @@border;
            border-radius: 8px;
            margin-top: 12px;
            padding-top: 8px;
            font-weight: bold;
            color: @@textMuted;
        }
        #customGroup::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 6px;
            color: #00BB9E;
        }
        #toggleRow {
            background: transparent;
            border: none;
            max-height: 40px;
        }
        #toggleLabel {
            color: @@textMuted;
            font-size: 13px;
            font-weight: normal;
            background: transparent;
            border: none;
        }
        #keymapDropdown {
            background: @@inputBg;
            border: 1px solid @@border;
            border-radius: 4px;
            padding: 6px 10px;
            color: @@text;
            font-size: 13px;
        }
        #keymapDropdown:hover {
            border-color: #00BB9E;
        }
        #keymapDropdown::drop-down {
            border: none;
            width: 20px;
        }
        #kmApplyBtn {
            background: #00BB9E;
            border: none;
            border-radius: 6px;
            padding: 5px 16px;
            font-weight: bold;
            color: #FFF;
            font-size: 12px;
        }
        #kmApplyBtn:hover {
            background: #00D4B1;
        }
        #kmActionBtn {
            background: @@actionBg;
            border: 1px solid @@border;
            border-radius: 6px;
            padding: 5px 12px;
            color: @@textMuted;
            font-size: 12px;
        }
        #kmActionBtn:hover {
            background: @@hoverBg;
            border-color: #00BB9E;
            color: @@textBright;
        }
        #kmAddNewBtn {
            background: transparent;
            border: 1px dashed @@border;
            border-radius: 6px;
            padding: 5px 12px;
            color: #00BB9E;
            font-size: 12px;
            font-weight: bold;
        }
        #kmAddNewBtn:hover {
            background: @@hoverBg;
            border: 1px dashed #00BB9E;
        }
        #logTextEdit {
            background: @@logBg;
            border: 1px solid @@border;
            border-radius: 4px;
            color: @@logText;
            font-family: "Consolas", "Courier New", monospace;
            font-size: 11px;
        }
        #dialogCancelBtn {
            background: @@actionBg;
            border: 1px solid @@border;
            border-radius: 6px;
            padding: 6px 16px;
            color: @@textMuted;
        }
        #dialogCancelBtn:hover {
            background: @@hoverBg;
            border-color: #888;
            color: @@textBright;
        }
    )";

    if (m_isDark) {
        css.replace("@@bg",        "#2e2e2e");
        css.replace("@@border",    "#444444");
        css.replace("@@textMuted", "#CCCCCC");
        css.replace("@@text",      "#DDDDDD");
        css.replace("@@textBright","#FFFFFF");
        css.replace("@@inputBg",   "#383838");
        css.replace("@@actionBg",  "#3a3a3a");
        css.replace("@@hoverBg",   "#484848");
        css.replace("@@logBg",     "#1e1e1e");
        css.replace("@@logText",   "#AAAAAA");
    } else {
        css.replace("@@bg",        "#FAFAFA");
        css.replace("@@border",    "#D0D0D0");
        css.replace("@@textMuted", "#555555");
        css.replace("@@text",      "#333333");
        css.replace("@@textBright","#111111");
        css.replace("@@inputBg",   "#FFFFFF");
        css.replace("@@actionBg",  "#F0F0F0");
        css.replace("@@hoverBg",   "#E8E8E8");
        css.replace("@@logBg",     "#F5F5F5");
        css.replace("@@logText",   "#666666");
    }

    setStyleSheet(css);
}

// ===== Keymap methods =====

void CustomDialog::setKeymapList(const QStringList &keymaps, const QString &current)
{
    m_keymapBox->clear();
    m_keymapBox->addItems(keymaps);
    if (!current.isEmpty()) {
        int idx = m_keymapBox->findText(current);
        if (idx >= 0) m_keymapBox->setCurrentIndex(idx);
    }
}

QString CustomDialog::selectedKeymap() const
{
    return m_keymapBox->currentText();
}

// ===== Preference getters =====
bool CustomDialog::recordScreen() const { return m_recordScreenToggle->isChecked(); }
bool CustomDialog::backgroundRecord() const { return m_bgRecordToggle->isChecked(); }
bool CustomDialog::reverseConnection() const { return m_reverseConnToggle->isChecked(); }
bool CustomDialog::showFPS() const { return m_showFpsToggle->isChecked(); }
bool CustomDialog::alwaysOnTop() const { return m_alwaysTopToggle->isChecked(); }
bool CustomDialog::screenOff() const { return m_screenOffToggle->isChecked(); }
bool CustomDialog::frameless() const { return m_framelessToggle->isChecked(); }
bool CustomDialog::stayAwake() const { return m_stayAwakeToggle->isChecked(); }
bool CustomDialog::showToolbar() const { return m_showToolbarToggle->isChecked(); }

// ===== Preference setters =====
void CustomDialog::setRecordScreen(bool v) { m_recordScreenToggle->setChecked(v); }
void CustomDialog::setBackgroundRecord(bool v) { m_bgRecordToggle->setChecked(v); }
void CustomDialog::setReverseConnection(bool v) { m_reverseConnToggle->setChecked(v); }
void CustomDialog::setShowFPS(bool v) { m_showFpsToggle->setChecked(v); }
void CustomDialog::setAlwaysOnTop(bool v) { m_alwaysTopToggle->setChecked(v); }
void CustomDialog::setScreenOff(bool v) { m_screenOffToggle->setChecked(v); }
void CustomDialog::setFrameless(bool v) { m_framelessToggle->setChecked(v); }
void CustomDialog::setStayAwake(bool v) { m_stayAwakeToggle->setChecked(v); }
void CustomDialog::setShowToolbar(bool v) { m_showToolbarToggle->setChecked(v); }

// ===== ADB Logs =====
void CustomDialog::appendLog(const QString &text)
{
    m_logEdit->append(text);
    // Auto-scroll to bottom
    QTextCursor cursor = m_logEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logEdit->setTextCursor(cursor);
}

// ===== Theme =====
void CustomDialog::setDarkTheme(bool isDark)
{
    m_isDark = isDark;
    applyStyles();
}
