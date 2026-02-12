#include "layerpanel.h"
#include <QCheckBox>
#include <QMessageBox>
#include <QFontMetrics>

// ============================================================================
// Auto-colors palette (8 distinct colours)
// ============================================================================

const QColor LayerPanel::s_autoColors[] = {
    QColor(66, 133, 244),   // Blue
    QColor(15, 157, 88),    // Green
    QColor(244, 180, 0),    // Yellow
    QColor(234, 67, 53),    // Red
    QColor(156, 39, 176),   // Purple
    QColor(255, 112, 67),   // Orange
    QColor(0, 188, 212),    // Cyan
    QColor(121, 85, 72),    // Brown
};
const int LayerPanel::s_autoColorCount = 8;

// ============================================================================
// Construction
// ============================================================================

LayerPanel::LayerPanel(QWidget *parent)
    : QWidget(parent)
{
    setFixedWidth(180);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(4, 4, 4, 4);
    root->setSpacing(4);

    QLabel *title = new QLabel("Layers");
    title->setStyleSheet("font-weight: bold; font-size: 13px; color: #4285F4;");
    root->addWidget(title);

    m_list = new QListWidget;
    m_list->setStyleSheet(
        "QListWidget { background: #2a2a2a; border: 1px solid #444; }"
        "QListWidget::item { padding: 4px; }"
        "QListWidget::item:selected { background: #4285F4; }");
    root->addWidget(m_list, 1);

    QHBoxLayout *btnRow = new QHBoxLayout;
    m_addBtn = new QPushButton("+");
    m_addBtn->setFixedSize(28, 28);
    m_addBtn->setToolTip("Add layer");
    m_removeBtn = new QPushButton("−");
    m_removeBtn->setFixedSize(28, 28);
    m_removeBtn->setToolTip("Remove selected layer");
    btnRow->addWidget(m_addBtn);
    btnRow->addWidget(m_removeBtn);
    btnRow->addStretch();
    root->addLayout(btnRow);

    connect(m_addBtn,    &QPushButton::clicked, this, &LayerPanel::onAddLayer);
    connect(m_removeBtn, &QPushButton::clicked, this, &LayerPanel::onRemoveLayer);
    connect(m_list, &QListWidget::currentItemChanged,
            this,   &LayerPanel::onLayerItemChanged);
    connect(m_list, &QListWidget::itemDoubleClicked,
            this,   &LayerPanel::onItemDoubleClicked);

    // Start with "Default" layer
    addLayer("Default", s_autoColors[0]);
    m_activeLayer = "Default";
}

// ============================================================================
// Public API
// ============================================================================

QStringList LayerPanel::layerNames() const
{
    QStringList names;
    for (const LayerInfo &li : m_layers)
        names.append(li.name);
    return names;
}

LayerPanel::LayerInfo LayerPanel::layerInfo(const QString &name) const
{
    for (const LayerInfo &li : m_layers)
        if (li.name == name) return li;
    return {};
}

bool LayerPanel::isLayerVisible(const QString &name) const
{
    for (const LayerInfo &li : m_layers)
        if (li.name == name) return li.visible;
    return true;  // unknown layers are visible
}

QColor LayerPanel::layerColor(const QString &name) const
{
    for (const LayerInfo &li : m_layers)
        if (li.name == name) return li.color;
    return QColor(128, 128, 128);
}

void LayerPanel::addLayer(const QString &name, const QColor &color)
{
    // Prevent duplicates
    for (const LayerInfo &li : m_layers)
        if (li.name == name) return;

    LayerInfo li;
    li.name    = name;
    li.color   = color.isValid() ? color : nextAutoColor();
    li.visible = true;
    m_layers.append(li);

    rebuildList();
    emit layerAdded(name);
}

void LayerPanel::removeLayer(const QString &name)
{
    if (name == "Default") return;  // can't remove default

    for (int i = 0; i < m_layers.size(); ++i) {
        if (m_layers[i].name == name) {
            m_layers.removeAt(i);
            break;
        }
    }
    if (m_activeLayer == name)
        m_activeLayer = "Default";

    rebuildList();
    emit layerRemoved(name);
}

void LayerPanel::clear()
{
    m_layers.clear();
    addLayer("Default", s_autoColors[0]);
    m_activeLayer = "Default";
    rebuildList();
}

void LayerPanel::setActiveLayer(const QString &name)
{
    m_activeLayer = name;
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *item = m_list->item(i);
        if (item->data(Qt::UserRole).toString() == name) {
            m_list->setCurrentItem(item);
            break;
        }
    }
}

// ============================================================================
// Private slots
// ============================================================================

void LayerPanel::onAddLayer()
{
    bool ok;
    QString name = QInputDialog::getText(this, "New Layer", "Layer name:",
                                          QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    name = name.trimmed();

    // Check duplicate
    for (const LayerInfo &li : m_layers) {
        if (li.name == name) {
            QMessageBox::information(this, "Duplicate", "A layer with that name already exists.");
            return;
        }
    }

    addLayer(name);
    setActiveLayer(name);
    emit activeLayerChanged(name);
}

void LayerPanel::onRemoveLayer()
{
    QListWidgetItem *cur = m_list->currentItem();
    if (!cur) return;

    QString name = cur->data(Qt::UserRole).toString();
    if (name == "Default") {
        QMessageBox::information(this, "Cannot Remove", "The Default layer cannot be removed.");
        return;
    }

    auto ret = QMessageBox::question(this, "Remove Layer",
        QString("Remove layer \"%1\"?\nNodes in this layer will move to Default.").arg(name),
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    removeLayer(name);
}

void LayerPanel::onLayerItemChanged(QListWidgetItem *current, QListWidgetItem * /*previous*/)
{
    if (!current) return;
    m_activeLayer = current->data(Qt::UserRole).toString();
    emit activeLayerChanged(m_activeLayer);
}

void LayerPanel::onItemDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;
    QString name = item->data(Qt::UserRole).toString();

    // Toggle visibility on double-click
    for (LayerInfo &li : m_layers) {
        if (li.name == name) {
            li.visible = !li.visible;
            rebuildList();
            emit layerVisibilityChanged(name, li.visible);
            return;
        }
    }
}

// ============================================================================
// Helpers
// ============================================================================

void LayerPanel::rebuildList()
{
    m_list->blockSignals(true);
    m_list->clear();

    for (const LayerInfo &li : m_layers) {
        QString label = QString("%1 %2").arg(li.visible ? "👁" : "🚫").arg(li.name);
        QListWidgetItem *item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, li.name);
        item->setForeground(li.visible ? li.color : QColor(100, 100, 100));
        if (li.name == m_activeLayer)
            m_list->setCurrentItem(item);
        m_list->addItem(item);
    }

    m_list->blockSignals(false);
}

QColor LayerPanel::nextAutoColor() const
{
    return s_autoColors[m_layers.size() % s_autoColorCount];
}
