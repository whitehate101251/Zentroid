#ifndef ADVANCEDDIALOG_H
#define ADVANCEDDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

class AdvancedDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AdvancedDialog(QWidget *parent = nullptr);

    // Streaming settings
    void setBitRate(quint32 bitRate);
    quint32 getBitRate() const;
    void setMaxSizeIndex(int index);
    int getMaxSizeIndex() const;
    void setLockOrientationIndex(int index);
    int getLockOrientationIndex() const;

    // Recording settings
    void setRecordFormatIndex(int index);
    int getRecordFormatIndex() const;
    void setRecordPath(const QString &path);
    QString getRecordPath() const;

    // Render driver settings
    void setRenderDriverValue(int value);  // -1=auto, 0=software, 1=ANGLE, 2=OpenGL
    int getRenderDriverValue() const;

    // Theme
    void setDarkTheme(bool isDark);

signals:
    void restartRequested();

private slots:
    void onSelectPath();

private:
    void setupUI();
    void applyStyles();

    // Streaming
    QLineEdit *m_bitRateEdit;
    QComboBox *m_bitRateUnitBox;
    QComboBox *m_maxSizeBox;
    QComboBox *m_lockOrientationBox;

    // Recording
    QComboBox *m_formatBox;
    QLineEdit *m_recordPathEdit;
    QPushButton *m_selectPathBtn;

    // Render driver
    QComboBox *m_renderDriverBox;
    QPushButton *m_restartBtn;

    bool m_isDark = true;
};

#endif // ADVANCEDDIALOG_H
