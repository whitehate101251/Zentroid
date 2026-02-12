#ifndef KEYMAPEDITOR_H
#define KEYMAPEDITOR_H

#include <QDialog>
#include <QGraphicsView>
#include <QPixmap>
#include <QSize>
#include <QList>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QSplitter>
#include <QUndoStack>
#include <QTimer>
#include <QSet>
#include <QElapsedTimer>

class KeyNode;
class EditorScene;
class PropertiesPanel;
class LayerPanel;
class QToolBar;
class QStatusBar;
class QComboBox;
class QLabel;
class QGraphicsItem;

/**
 * @brief Visual keymap editor for Zentroid
 * 
 * Provides a graphical interface for creating and editing keyboard-to-touch
 * mappings by placing nodes on a device screenshot canvas.
 */
class KeymapEditorDialog : public QDialog
{
    Q_OBJECT

    // Allow undo commands to call undoAddNode / undoRemoveNode
    friend class AddNodeCommand;
    friend class DeleteNodeCommand;

public:
    /**
     * @brief Constructor
     * @param deviceScreenshot Background image of the device screen
     * @param deviceSize Resolution of the device screen
     * @param parent Parent widget
     */
    explicit KeymapEditorDialog(const QPixmap &deviceScreenshot, 
                                const QSize &deviceSize,
                                QWidget *parent = nullptr);
    ~KeymapEditorDialog();

    /**
     * @brief Load keymap from JSON file
     * @param filePath Path to the keymap JSON file
     * @return true if loaded successfully
     */
    bool loadKeymap(const QString &filePath);

    /**
     * @brief Save current keymap to JSON file
     * @param filePath Path to save the keymap
     * @return true if saved successfully
     */
    bool saveKeymap(const QString &filePath);

signals:
    /**
     * @brief Emitted when user applies the keymap
     * @param filePath Path to the keymap file to apply
     */
    void keymapApplied(const QString &filePath);

private slots:
    // Toolbar actions
    void onSelectMode();
    void onAddClickNode();
    void onAddClickTwiceNode();
    void onAddDragNode();
    void onAddWASDNode();
    void onAddClickMultiNode();
    void onAddGestureNode();
    void onDeleteSelected();
    void onZoomIn();
    void onZoomOut();
    void onZoomReset();
    
    /** Fit the device screen to fill the canvas view (aspect-ratio correct) */
    void fitCanvasToView();
    
    // Menu actions
    void onNew();
    void onOpen();
    void onSave();
    void onSaveAs();
    void onExport();
    void onImport();
    
    // Scene events — interactive Phase 2
    void onCanvasClicked(const QPointF &scenePos);
    void onItemDoubleClicked(QGraphicsItem *item, const QPointF &scenePos);
    void onSelectionChanged();
    void onMouseMoved(const QPointF &scenePos);
    
    // Properties panel
    void onNodeModifiedByPanel();
    
    // Profile management
    void onProfileChanged(int index);
    void onRefreshProfiles();
    
    // Snap & alignment
    void onToggleSnapToGrid(bool checked);
    void onAlignLeft();
    void onAlignRight();
    void onAlignTop();
    void onAlignBottom();
    void onAlignCenterH();
    void onAlignCenterV();
    void onDistributeH();
    void onDistributeV();
    
    // Templates
    void onLoadTemplate(int index);
    
    // Live preview
    void onTogglePreview(bool checked);
    void clearAllHighlights();
    
    // Context menu
    void onContextMenu(const QPointF &scenePos, QGraphicsItem *item);
    
    // Copy/paste
    void onCopy();
    void onPaste();
    void onDuplicate();
    
    // Global settings
    void onEditMouseMoveMap();
    void onEditSwitchKey();
    
    // Layers
    void onLayerVisibilityChanged(const QString &name, bool visible);
    void onActiveLayerChanged(const QString &name);
    void onLayerRemoved(const QString &name);
    void onAssignSelectedToLayer();
    
    // Macro recording
    void onToggleMacroRecord(bool checked);

private:
    // UI initialization
    void initUI();
    void initCanvas();
    void initToolbar();
    void initMenuBar();
    void initStatusBar();
    void initPropertiesPanel();
    void setupConnections();
    
    // Canvas operations
    void clearCanvas();
    void refreshCanvas();
    void addNodeToScene(KeyNode *node);
    void removeNodeFromScene(KeyNode *node);
    
    // Interactive helpers
    void createNodeAtPosition(const QPointF &scenePos);
    void editNodeViaDialog(KeyNode *node);
    void selectNode(KeyNode *node);
    void deselectAll();
    
    // Coordinate conversion
    QPointF screenToRelative(const QPointF &screenPos) const;
    QPointF relativeToScreen(const QPointF &relativePos) const;
    
    // File operations
    void loadKeymapProfiles();
    bool parseKeymapJson(const QJsonObject &json);
    QJsonObject generateKeymapJson() const;
    
    // Helper methods
    void updateStatusBar(const QString &message);
    void updateModeActions();
    QString generateDefaultName() const;
    
    // Undo/redo helpers (called by undo commands via friend access)
    void undoAddNode(KeyNode *node);
    void undoRemoveNode(KeyNode *node);
    
    // Snap-to-grid helpers
    QPointF snapToGrid(const QPointF &relPos) const;
    void drawGridOverlay();
    void removeGridOverlay();
    
    // Template helpers
    void initTemplatePresets();
    
    // Layer helpers
    void initLayerPanel();
    void updateNodeVisibilityByLayers();
    
    // Macro helpers
    void macroRecordClick(const QPointF &scenePos);
    
    // Preview helpers
    void highlightNodesForKey(const QString &keyCode);
    void unhighlightNodesForKey(const QString &keyCode);

protected:
    // Key event interception for live preview
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    // Canvas components
    QGraphicsView *m_canvasView;
    EditorScene   *m_scene;
    QPixmap m_deviceScreenshot;
    QSize m_deviceSize;
    
    // Properties panel
    PropertiesPanel *m_propertiesPanel;
    QSplitter       *m_splitter;
    
    // Keymap nodes
    QList<KeyNode*> m_nodes;
    KeyNode *m_selectedNode;
    
    // Toolbar
    QToolBar *m_toolbar;
    QAction *m_selectAction;
    QAction *m_addClickAction;
    QAction *m_addClickTwiceAction;
    QAction *m_addDragAction;
    QAction *m_addWASDAction;
    QAction *m_addClickMultiAction;
    QAction *m_addGestureAction;
    QAction *m_deleteAction;
    
    // Undo/redo
    QUndoStack *m_undoStack;
    QAction *m_undoAction;
    QAction *m_redoAction;
    
    // Snap-to-grid
    bool m_snapToGrid;
    qreal m_gridSize;  // in relative coords (e.g., 0.05 = 5% of screen)
    QList<QGraphicsItem*> m_gridLines;
    QAction *m_snapAction;
    
    // Templates
    QComboBox *m_templateCombo;
    
    // Live preview
    bool m_previewMode;
    QAction *m_previewAction;
    QTimer *m_highlightTimer;
    QSet<QString> m_activePreviewKeys;  // currently held keys
    
    // Status bar
    QStatusBar *m_statusBar;
    QLabel *m_coordLabel;
    QLabel *m_nodeCountLabel;
    QLabel *m_modeLabel;
    
    // Profile management
    QComboBox *m_profileCombo;
    QString m_currentProfile;
    QString m_currentFilePath;
    bool m_modified;
    
    // Global keymap settings
    QString m_switchKey;
    bool m_hasMouseMoveMap = false;
    QJsonObject m_mouseMoveMap;
    
    // Copy/paste clipboard
    QJsonArray m_clipboard;
    
    // Layer panel
    LayerPanel *m_layerPanel;
    
    // Macro recording
    bool m_macroRecording;
    QAction *m_macroAction;
    QElapsedTimer m_macroTimer;
    struct MacroPoint { int delay; QPointF relPos; };
    QList<MacroPoint> m_macroPoints;
    
    // Editor state
    enum EditorMode {
        SelectMode,
        AddClickMode,
        AddClickTwiceMode,
        AddDragMode,
        AddWASDMode,
        AddClickMultiMode,
        AddGestureMode
    };
    EditorMode m_currentMode;
    qreal m_zoomLevel;
};

#endif // KEYMAPEDITOR_H
