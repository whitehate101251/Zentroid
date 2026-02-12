#ifndef KEYMAPPATH_H
#define KEYMAPPATH_H

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QString>

// Single canonical keymap directory used by ALL components:
//   - Dialog dropdown
//   - Runtime script loading (Apply button)
//   - Keymap Editor (open / save / profiles)
//   - Overlay Editor (F12, load / save)
//
// Resolution order:
//   1. ZENTROID_KEYMAP_PATH environment variable (if set and valid)
//   2. applicationDirPath()/keymap (fallback)
//
// The result is always resolved to an absolute path.

inline const QString &getCanonicalKeymapDir()
{
    static QString s_path;
    if (s_path.isEmpty()) {
        QString envPath = QString::fromLocal8Bit(qgetenv("ZENTROID_KEYMAP_PATH"));
        QFileInfo fi(envPath);
        if (!envPath.isEmpty() && fi.isDir()) {
            s_path = fi.absoluteFilePath();          // resolve to absolute
        } else {
            s_path = QCoreApplication::applicationDirPath() + "/keymap";
        }
    }
    return s_path;
}

#endif // KEYMAPPATH_H
