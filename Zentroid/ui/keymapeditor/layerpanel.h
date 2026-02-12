#ifndef LAYERPANEL_H
#define LAYERPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QMap>
#include <QColor>
#include <QInputDialog>
#include <QColorDialog>

/**
 * @brief Layer management panel for organizing keymap nodes into groups.
 *
 * Each layer has a name, color, and visibility toggle.
 * Nodes can be assigned to a layer; hidden layers hide their nodes on canvas.
 */
class LayerPanel : public QWidget
{
    Q_OBJECT

public:
    struct LayerInfo {
        QString name;
        QColor  color;
        bool    visible = true;
    };

    explicit LayerPanel(QWidget *parent = nullptr);

    // Layer queries
    QStringList layerNames() const;
    LayerInfo   layerInfo(const QString &name) const;
    bool        isLayerVisible(const QString &name) const;
    QColor      layerColor(const QString &name) const;
    QString     activeLayer() const { return m_activeLayer; }
    
    // Programmatic manipulation
    void addLayer(const QString &name, const QColor &color = QColor());
    void removeLayer(const QString &name);
    void clear();
    void setActiveLayer(const QString &name);

signals:
    /** A layer's visibility was toggled. */
    void layerVisibilityChanged(const QString &name, bool visible);

    /** The active/selected layer changed. */
    void activeLayerChanged(const QString &name);

    /** A layer was added. */
    void layerAdded(const QString &name);

    /** A layer was removed (nodes should be reassigned to Default). */
    void layerRemoved(const QString &name);

    /** A layer's color changed. */
    void layerColorChanged(const QString &name, const QColor &color);

private slots:
    void onAddLayer();
    void onRemoveLayer();
    void onLayerItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void onItemDoubleClicked(QListWidgetItem *item);

private:
    void rebuildList();
    QColor nextAutoColor() const;

    QListWidget *m_list;
    QPushButton *m_addBtn;
    QPushButton *m_removeBtn;

    QList<LayerInfo> m_layers;
    QString m_activeLayer;

    static const QColor s_autoColors[];
    static const int s_autoColorCount;
};

#endif // LAYERPANEL_H
