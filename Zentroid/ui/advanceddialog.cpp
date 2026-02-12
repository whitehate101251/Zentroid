#include "advanceddialog.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QVBoxLayout>

AdvancedDialog::AdvancedDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Advanced Settings");
    setMinimumWidth(420);
    setupUI();
    applyStyles();
}

void AdvancedDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 20, 24, 20);

    // ====== Streaming Settings ======
    QGroupBox *streamGroup = new QGroupBox("Streaming Settings", this);
    streamGroup->setObjectName("advGroup");
    QFormLayout *streamForm = new QFormLayout(streamGroup);
    streamForm->setSpacing(10);
    streamForm->setContentsMargins(16, 20, 16, 12);

    // Bitrate row
    QHBoxLayout *bitRateRow = new QHBoxLayout();
    bitRateRow->setSpacing(6);
    m_bitRateEdit = new QLineEdit(this);
    m_bitRateEdit->setValidator(new QIntValidator(1, 99999, this));
    m_bitRateEdit->setPlaceholderText("20");
    m_bitRateEdit->setFixedWidth(100);
    m_bitRateUnitBox = new QComboBox(this);
    m_bitRateUnitBox->addItem("Mbps");
    m_bitRateUnitBox->addItem("Kbps");
    m_bitRateUnitBox->setFixedWidth(80);
    bitRateRow->addWidget(m_bitRateEdit);
    bitRateRow->addWidget(m_bitRateUnitBox);
    bitRateRow->addStretch();
    streamForm->addRow("Bitrate:", bitRateRow);

    // Max Size
    m_maxSizeBox = new QComboBox(this);
    m_maxSizeBox->addItem("640");
    m_maxSizeBox->addItem("720");
    m_maxSizeBox->addItem("1080");
    m_maxSizeBox->addItem("1280");
    m_maxSizeBox->addItem("1920");
    m_maxSizeBox->addItem(tr("original"));
    streamForm->addRow("Max Size:", m_maxSizeBox);

    // Lock Orientation
    m_lockOrientationBox = new QComboBox(this);
    m_lockOrientationBox->addItem(tr("no lock"));
    m_lockOrientationBox->addItem("0");
    m_lockOrientationBox->addItem("90");
    m_lockOrientationBox->addItem("180");
    m_lockOrientationBox->addItem("270");
    streamForm->addRow("Lock Orientation:", m_lockOrientationBox);

    mainLayout->addWidget(streamGroup);

    // ====== Recording Settings ======
    QGroupBox *recGroup = new QGroupBox("Recording", this);
    recGroup->setObjectName("advGroup");
    QFormLayout *recForm = new QFormLayout(recGroup);
    recForm->setSpacing(10);
    recForm->setContentsMargins(16, 20, 16, 12);

    // Format
    m_formatBox = new QComboBox(this);
    m_formatBox->addItem("mp4");
    m_formatBox->addItem("mkv");
    recForm->addRow("Record Format:", m_formatBox);

    // Record Path
    QHBoxLayout *pathRow = new QHBoxLayout();
    pathRow->setSpacing(6);
    m_recordPathEdit = new QLineEdit(this);
    m_recordPathEdit->setReadOnly(true);
    m_recordPathEdit->setPlaceholderText("Select recording save path...");
    m_selectPathBtn = new QPushButton("Select Path", this);
    m_selectPathBtn->setObjectName("selectPathBtn");
    m_selectPathBtn->setCursor(Qt::PointingHandCursor);
    pathRow->addWidget(m_recordPathEdit, 1);
    pathRow->addWidget(m_selectPathBtn);
    recForm->addRow("Save Path:", pathRow);

    mainLayout->addWidget(recGroup);

    // ====== Render Driver Settings ======
    QGroupBox *renderGroup = new QGroupBox("Render Driver", this);
    renderGroup->setObjectName("advGroup");
    QFormLayout *renderForm = new QFormLayout(renderGroup);
    renderForm->setSpacing(10);
    renderForm->setContentsMargins(16, 20, 16, 12);

    m_renderDriverBox = new QComboBox(this);
    m_renderDriverBox->addItem("Auto (Recommended)");   // index 0 → value -1
    m_renderDriverBox->addItem("Desktop OpenGL");        // index 1 → value  2
#ifdef Q_OS_WIN32
    m_renderDriverBox->addItem("ANGLE / DirectX");      // index 2 → value  1
#else
    m_renderDriverBox->addItem("OpenGL ES");             // index 2 → value  1
#endif
    m_renderDriverBox->addItem("Software (Compatibility)"); // index 3 → value 0
    renderForm->addRow("Driver:", m_renderDriverBox);

    QHBoxLayout *restartRow = new QHBoxLayout();
    restartRow->setSpacing(8);
    QLabel *renderNote = new QLabel("⚠ Restart required after changing driver.", this);
    renderNote->setObjectName("renderNote");
    renderNote->setWordWrap(true);
    m_restartBtn = new QPushButton("↻  Restart Now", this);
    m_restartBtn->setObjectName("restartBtn");
    m_restartBtn->setCursor(Qt::PointingHandCursor);
    m_restartBtn->setFixedWidth(120);
    m_restartBtn->setVisible(false);
    restartRow->addWidget(renderNote, 1);
    restartRow->addWidget(m_restartBtn);
    renderForm->addRow(restartRow);

    // Show restart button when driver selection changes
    connect(m_renderDriverBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        m_restartBtn->setVisible(true);
    });
    // Restart: accept dialog then emit signal
    connect(m_restartBtn, &QPushButton::clicked, this, [this]() {
        emit restartRequested();
        accept();
    });

    mainLayout->addWidget(renderGroup);

    // ====== Buttons ======
    mainLayout->addSpacing(8);
    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    QPushButton *okBtn = new QPushButton("OK", this);
    okBtn->setObjectName("dialogOkBtn");
    okBtn->setCursor(Qt::PointingHandCursor);
    okBtn->setFixedWidth(90);
    QPushButton *cancelBtn = new QPushButton("Cancel", this);
    cancelBtn->setObjectName("dialogCancelBtn");
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setFixedWidth(90);
    btnRow->addWidget(okBtn);
    btnRow->addWidget(cancelBtn);
    mainLayout->addLayout(btnRow);

    connect(m_selectPathBtn, &QPushButton::clicked, this, &AdvancedDialog::onSelectPath);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void AdvancedDialog::applyStyles()
{
    QString css = R"(
        AdvancedDialog {
            background: @@bg;
        }
        #advGroup {
            border: 1px solid @@border;
            border-radius: 8px;
            margin-top: 12px;
            padding-top: 8px;
            font-weight: bold;
            color: @@textMuted;
        }
        #advGroup::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 6px;
            color: #00BB9E;
        }
        QLabel {
            color: @@textMuted;
            background: transparent;
            border: none;
        }
        QLineEdit {
            background: @@inputBg;
            border: 1px solid @@border;
            border-radius: 4px;
            padding: 4px 8px;
            color: @@text;
        }
        QLineEdit:focus {
            border-color: #00BB9E;
        }
        QComboBox {
            background: @@inputBg;
            border: 1px solid @@border;
            border-radius: 4px;
            padding: 4px 8px;
            color: @@text;
        }
        QComboBox:hover {
            border-color: #00BB9E;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        #selectPathBtn {
            background: @@actionBg;
            border: 1px solid @@border;
            border-radius: 4px;
            padding: 4px 12px;
            color: @@textMuted;
        }
        #selectPathBtn:hover {
            background: @@hoverBg;
            border-color: #00BB9E;
            color: @@textBright;
        }
        #dialogOkBtn {
            background: #00BB9E;
            border: none;
            border-radius: 6px;
            padding: 6px 16px;
            font-weight: bold;
            color: #FFF;
        }
        #dialogOkBtn:hover {
            background: #00D4B1;
        }
        #dialogCancelBtn {
            background: @@actionBg;
            border: 1px solid @@border;
            border-radius: 6px;
            padding: 6px 16px;
            color: @@textMuted;
        }
        #renderNote {
            color: #E8A035;
            font-size: 11px;
            background: transparent;
            border: none;
            padding: 2px 0;
        }
        #restartBtn {
            background: #D94040;
            border: none;
            border-radius: 6px;
            padding: 5px 12px;
            font-weight: bold;
            font-size: 11px;
            color: #FFF;
        }
        #restartBtn:hover {
            background: #E85555;
        }
        #restartBtn:pressed {
            background: #C03030;
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
    } else {
        css.replace("@@bg",        "#FAFAFA");
        css.replace("@@border",    "#D0D0D0");
        css.replace("@@textMuted", "#555555");
        css.replace("@@text",      "#333333");
        css.replace("@@textBright","#111111");
        css.replace("@@inputBg",   "#FFFFFF");
        css.replace("@@actionBg",  "#F0F0F0");
        css.replace("@@hoverBg",   "#E8E8E8");
    }

    setStyleSheet(css);
}

void AdvancedDialog::setDarkTheme(bool isDark)
{
    m_isDark = isDark;
    applyStyles();
}

void AdvancedDialog::onSelectPath()
{
    QFileDialog::Options options = QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly;
    QString directory = QFileDialog::getExistingDirectory(this, tr("Select Recording Path"), "", options);
    if (!directory.isEmpty()) {
        m_recordPathEdit->setText(directory);
    }
}

// ===== Getters / Setters =====

void AdvancedDialog::setBitRate(quint32 bitRate)
{
    if (bitRate == 0) {
        m_bitRateUnitBox->setCurrentText("Mbps");
        m_bitRateEdit->clear();
    } else if (bitRate % 1000000 == 0) {
        m_bitRateEdit->setText(QString::number(bitRate / 1000000));
        m_bitRateUnitBox->setCurrentText("Mbps");
    } else {
        m_bitRateEdit->setText(QString::number(bitRate / 1000));
        m_bitRateUnitBox->setCurrentText("Kbps");
    }
}

quint32 AdvancedDialog::getBitRate() const
{
    quint32 val = m_bitRateEdit->text().trimmed().toUInt();
    if (m_bitRateUnitBox->currentText() == "Mbps") {
        return val * 1000000;
    }
    return val * 1000;
}

void AdvancedDialog::setMaxSizeIndex(int index) { m_maxSizeBox->setCurrentIndex(index); }
int AdvancedDialog::getMaxSizeIndex() const { return m_maxSizeBox->currentIndex(); }

void AdvancedDialog::setLockOrientationIndex(int index) { m_lockOrientationBox->setCurrentIndex(index); }
int AdvancedDialog::getLockOrientationIndex() const { return m_lockOrientationBox->currentIndex(); }

void AdvancedDialog::setRecordFormatIndex(int index) { m_formatBox->setCurrentIndex(index); }
int AdvancedDialog::getRecordFormatIndex() const { return m_formatBox->currentIndex(); }

void AdvancedDialog::setRecordPath(const QString &path) { m_recordPathEdit->setText(path); }
QString AdvancedDialog::getRecordPath() const { return m_recordPathEdit->text().trimmed(); }

void AdvancedDialog::setRenderDriverValue(int value)
{
    // value: -1=auto, 0=software, 1=ANGLE/ES, 2=OpenGL
    // combo:  0=auto,  3=software, 2=ANGLE/ES, 1=OpenGL
    switch (value) {
    case 0:  m_renderDriverBox->setCurrentIndex(3); break; // software
    case 1:  m_renderDriverBox->setCurrentIndex(2); break; // ANGLE/ES
    case 2:  m_renderDriverBox->setCurrentIndex(1); break; // OpenGL
    case -1:
    default: m_renderDriverBox->setCurrentIndex(0); break; // auto
    }
}

int AdvancedDialog::getRenderDriverValue() const
{
    // combo index → config value
    switch (m_renderDriverBox->currentIndex()) {
    case 1:  return 2;   // Desktop OpenGL
    case 2:  return 1;   // ANGLE / OpenGL ES
    case 3:  return 0;   // Software
    case 0:
    default: return -1;  // Auto
    }
}
