#ifndef CUSTOMDIALOG_H
#define CUSTOMDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QPointer>

class ToggleSwitch;

class CustomDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomDialog(QWidget *parent = nullptr);

    // Keymap
    void setKeymapList(const QStringList &keymaps, const QString &current = "");
    QString selectedKeymap() const;

    // Preferences getters
    bool recordScreen() const;
    bool backgroundRecord() const;
    bool reverseConnection() const;
    bool showFPS() const;
    bool alwaysOnTop() const;
    bool screenOff() const;
    bool frameless() const;
    bool stayAwake() const;
    bool showToolbar() const;

    // Preferences setters
    void setRecordScreen(bool v);
    void setBackgroundRecord(bool v);
    void setReverseConnection(bool v);
    void setShowFPS(bool v);
    void setAlwaysOnTop(bool v);
    void setScreenOff(bool v);
    void setFrameless(bool v);
    void setStayAwake(bool v);
    void setShowToolbar(bool v);

    // Theme
    void setDarkTheme(bool isDark);

    // ADB logs
    void appendLog(const QString &text);

signals:
    void keymapRefreshRequested();
    void keymapApplyRequested(const QString &keymapName);
    void keymapEditRequested();
    void keymapAddNewRequested();
    void preferencesChanged();

private:
    void setupUI();
    void applyStyles();
    QWidget *createToggleRow(const QString &label, ToggleSwitch *toggle);

    // Keymap
    QComboBox *m_keymapBox;
    QPushButton *m_applyKeymapBtn;
    QPushButton *m_editKeymapBtn;
    QPushButton *m_addKeymapBtn;

    // Toggles
    ToggleSwitch *m_recordScreenToggle;
    ToggleSwitch *m_bgRecordToggle;
    ToggleSwitch *m_reverseConnToggle;
    ToggleSwitch *m_showFpsToggle;
    ToggleSwitch *m_alwaysTopToggle;
    ToggleSwitch *m_screenOffToggle;
    ToggleSwitch *m_framelessToggle;
    ToggleSwitch *m_stayAwakeToggle;
    ToggleSwitch *m_showToolbarToggle;

    // ADB logs
    QTextEdit *m_logEdit;

    // Theme
    bool m_isDark = true;
};

#endif // CUSTOMDIALOG_H
