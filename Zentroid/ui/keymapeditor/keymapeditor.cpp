#include "keymapeditor.h"
#include "keynode.h"
#include "editorscene.h"
#include "keyassigndialog.h"
#include "propertiespanel.h"
#include "undocommands.h"
#include "layerpanel.h"
#include "../../util/keymappath.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QDateTime>
#include <QGraphicsPixmapItem>
#include <QGraphicsLineItem>
#include <QSplitter>
#include <QActionGroup>
#include <QUndoStack>
#include <QKeyEvent>
#include <QShowEvent>
#include <QResizeEvent>
#include <QMetaEnum>

KeymapEditorDialog::KeymapEditorDialog(const QPixmap &deviceScreenshot, 
                                       const QSize &deviceSize,
                                       QWidget *parent)
    : QDialog(parent)
    , m_deviceScreenshot(deviceScreenshot)
    , m_deviceSize(deviceSize)
    , m_propertiesPanel(nullptr)
    , m_selectedNode(nullptr)
    , m_undoStack(nullptr)
    , m_undoAction(nullptr)
    , m_redoAction(nullptr)
    , m_snapToGrid(false)
    , m_gridSize(0.05)
    , m_snapAction(nullptr)
    , m_templateCombo(nullptr)
    , m_previewMode(false)
    , m_previewAction(nullptr)
    , m_highlightTimer(nullptr)
    , m_modified(false)
    , m_layerPanel(nullptr)
    , m_macroRecording(false)
    , m_macroAction(nullptr)
    , m_currentMode(SelectMode)
    , m_zoomLevel(1.0)
{
    setWindowTitle("Zentroid Keymap Editor");
    resize(1280, 860);
    setMinimumSize(800, 600);
    
    // Initialize undo stack
    m_undoStack = new QUndoStack(this);
    connect(m_undoStack, &QUndoStack::canUndoChanged, [this](bool can) {
        if (m_undoAction) m_undoAction->setEnabled(can);
    });
    connect(m_undoStack, &QUndoStack::canRedoChanged, [this](bool can) {
        if (m_redoAction) m_redoAction->setEnabled(can);
    });
    connect(m_undoStack, &QUndoStack::cleanChanged, [this](bool clean) {
        m_modified = !clean;
    });
    
    initUI();
    setupConnections();
    loadKeymapProfiles();
}

KeymapEditorDialog::~KeymapEditorDialog()
{
    // Clear undo stack first (it may own some nodes)
    if (m_undoStack)
        m_undoStack->clear();
    
    // Clean up nodes still on scene
    qDeleteAll(m_nodes);
    m_nodes.clear();
}

void KeymapEditorDialog::initUI()
{
    // Create main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Initialize components
    // initToolbar must come before initMenuBar because the menu
    // bar references actions (undo/redo/delete/snap/preview)
    // that are created inside initToolbar.
    initToolbar();
    initMenuBar();
    initCanvas();
    initPropertiesPanel();
    initLayerPanel();
    initStatusBar();
    
    // Layer panel + Canvas + properties in a splitter
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->addWidget(m_layerPanel);
    m_splitter->addWidget(m_canvasView);
    m_splitter->addWidget(m_propertiesPanel);
    m_splitter->setStretchFactor(0, 0);   // layer panel fixed width
    m_splitter->setStretchFactor(1, 1);   // canvas stretches
    m_splitter->setStretchFactor(2, 0);   // panel fixed width

    // Remove the grey splitter handles — thin 1px dividers, no dead zones
    m_splitter->setHandleWidth(1);
    m_splitter->setStyleSheet(
        "QSplitter::handle { background: #333; }");

    // Give the canvas maximum initial width (layer=180, props=260, rest=canvas)
    m_splitter->setSizes({180, 840, 260});
    
    // Add to layout
    mainLayout->addWidget(m_toolbar);
    mainLayout->addWidget(m_splitter, 1);
    mainLayout->addWidget(m_statusBar);
}

void KeymapEditorDialog::initCanvas()
{
    // Create custom scene (emits click/double-click signals)
    m_scene = new EditorScene(0, 0, m_deviceSize.width(), m_deviceSize.height(), this);
    
    // Add background screenshot
    if (!m_deviceScreenshot.isNull()) {
        QGraphicsPixmapItem *bgItem = new QGraphicsPixmapItem(
            m_deviceScreenshot.scaled(m_deviceSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        bgItem->setZValue(-1); // Behind all nodes
        m_scene->addItem(bgItem);
    }
    
    // Create view
    m_canvasView = new QGraphicsView(m_scene, this);
    m_canvasView->setRenderHint(QPainter::Antialiasing);
    m_canvasView->setRenderHint(QPainter::SmoothPixmapTransform);
    m_canvasView->setDragMode(QGraphicsView::RubberBandDrag);  // rubber-band for multi-select
    m_canvasView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_canvasView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_canvasView->setBackgroundBrush(QBrush(QColor(30, 30, 30)));
    m_canvasView->setMouseTracking(true);
    m_canvasView->setInteractive(true);
}

void KeymapEditorDialog::initToolbar()
{
    m_toolbar = new QToolBar(this);
    m_toolbar->setMovable(false);
    m_toolbar->setIconSize(QSize(24, 24));
    
    // Mutually-exclusive mode group
    QActionGroup *modeGroup = new QActionGroup(this);
    modeGroup->setExclusive(true);
    
    // Select mode action
    m_selectAction = new QAction("🔲 Select", this);
    m_selectAction->setCheckable(true);
    m_selectAction->setChecked(true);
    m_selectAction->setToolTip("Select and move nodes (S)");
    m_selectAction->setShortcut(QKeySequence("S"));
    modeGroup->addAction(m_selectAction);
    
    // Add node actions
    m_addClickAction = new QAction("🖱 Click", this);
    m_addClickAction->setCheckable(true);
    m_addClickAction->setToolTip("Add click node (C)");
    m_addClickAction->setShortcut(QKeySequence("C"));
    modeGroup->addAction(m_addClickAction);
    
    m_addClickTwiceAction = new QAction("🖱🖱 Click×2", this);
    m_addClickTwiceAction->setCheckable(true);
    m_addClickTwiceAction->setToolTip("Add double-click node (T)");
    m_addClickTwiceAction->setShortcut(QKeySequence("T"));
    modeGroup->addAction(m_addClickTwiceAction);
    
    m_addDragAction = new QAction("↔ Drag", this);
    m_addDragAction->setCheckable(true);
    m_addDragAction->setToolTip("Add drag node (D)");
    m_addDragAction->setShortcut(QKeySequence("D"));
    modeGroup->addAction(m_addDragAction);
    
    m_addWASDAction = new QAction("⊕ WASD", this);
    m_addWASDAction->setCheckable(true);
    m_addWASDAction->setToolTip("Add WASD steering wheel (W)");
    m_addWASDAction->setShortcut(QKeySequence("W"));
    modeGroup->addAction(m_addWASDAction);
    
    m_addClickMultiAction = new QAction("∴ Multi", this);
    m_addClickMultiAction->setCheckable(true);
    m_addClickMultiAction->setToolTip("Add multi-click node (M)");
    m_addClickMultiAction->setShortcut(QKeySequence("M"));
    modeGroup->addAction(m_addClickMultiAction);
    
    m_addGestureAction = new QAction("✋ Gesture", this);
    m_addGestureAction->setCheckable(true);
    m_addGestureAction->setToolTip("Add gesture node — pinch, swipe, rotate (G is taken, use toolbar)");
    modeGroup->addAction(m_addGestureAction);
    
    // Delete action (not in mode group)
    m_deleteAction = new QAction("🗑 Delete", this);
    m_deleteAction->setToolTip("Delete selected node (Del)");
    m_deleteAction->setShortcut(QKeySequence::Delete);
    
    // Undo / Redo actions
    m_undoAction = new QAction("↩ Undo", this);
    m_undoAction->setToolTip("Undo last action (Ctrl+Z)");
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setEnabled(false);
    
    m_redoAction = new QAction("↪ Redo", this);
    m_redoAction->setToolTip("Redo last undone action (Ctrl+Y)");
    m_redoAction->setShortcut(QKeySequence::Redo);
    m_redoAction->setEnabled(false);
    
    connect(m_undoAction, &QAction::triggered, m_undoStack, &QUndoStack::undo);
    connect(m_redoAction, &QAction::triggered, m_undoStack, &QUndoStack::redo);
    
    // Snap-to-grid toggle
    m_snapAction = new QAction("⊞ Snap", this);
    m_snapAction->setCheckable(true);
    m_snapAction->setChecked(false);
    m_snapAction->setToolTip("Toggle snap-to-grid (G)");
    m_snapAction->setShortcut(QKeySequence("G"));
    connect(m_snapAction, &QAction::toggled, this, &KeymapEditorDialog::onToggleSnapToGrid);
    
    // Add to toolbar
    m_toolbar->addAction(m_undoAction);
    m_toolbar->addAction(m_redoAction);
    m_toolbar->addSeparator();
    m_toolbar->addAction(m_selectAction);
    m_toolbar->addSeparator();
    m_toolbar->addAction(m_addClickAction);
    m_toolbar->addAction(m_addClickTwiceAction);
    m_toolbar->addAction(m_addDragAction);
    m_toolbar->addAction(m_addWASDAction);
    m_toolbar->addAction(m_addClickMultiAction);
    m_toolbar->addAction(m_addGestureAction);
    m_toolbar->addSeparator();
    m_toolbar->addAction(m_snapAction);
    m_toolbar->addSeparator();
    
    // Template selector
    m_toolbar->addWidget(new QLabel(" Template: ", this));
    m_templateCombo = new QComboBox(this);
    m_templateCombo->setMinimumWidth(140);
    initTemplatePresets();
    m_toolbar->addWidget(m_templateCombo);
    connect(m_templateCombo, QOverload<int>::of(&QComboBox::activated),
            this, &KeymapEditorDialog::onLoadTemplate);
    m_toolbar->addSeparator();
    
    // Live preview toggle
    m_previewAction = new QAction("▶ Preview", this);
    m_previewAction->setCheckable(true);
    m_previewAction->setChecked(false);
    m_previewAction->setToolTip("Toggle live preview mode — press keys to see which nodes activate (P)");
    m_previewAction->setShortcut(QKeySequence("P"));
    connect(m_previewAction, &QAction::toggled, this, &KeymapEditorDialog::onTogglePreview);
    m_toolbar->addAction(m_previewAction);
    m_toolbar->addSeparator();
    
    // Macro recording toggle
    m_macroAction = new QAction("⏺ Record", this);
    m_macroAction->setCheckable(true);
    m_macroAction->setChecked(false);
    m_macroAction->setToolTip("Record click sequence as a Multi-Click macro (R)");
    m_macroAction->setShortcut(QKeySequence("R"));
    connect(m_macroAction, &QAction::toggled, this, &KeymapEditorDialog::onToggleMacroRecord);
    m_toolbar->addAction(m_macroAction);
    m_toolbar->addSeparator();
    
    // Mode indicator label
    m_modeLabel = new QLabel(" Mode: Select ", this);
    m_modeLabel->setStyleSheet("font-weight: bold; padding: 2px 8px; "
                               "background: #4285F4; color: white; border-radius: 3px;");
    m_toolbar->addWidget(m_modeLabel);
    m_toolbar->addSeparator();
    
    // Profile selector
    m_toolbar->addWidget(new QLabel(" Profile: ", this));
    m_profileCombo = new QComboBox(this);
    m_profileCombo->setMinimumWidth(200);
    m_toolbar->addWidget(m_profileCombo);
    
    QPushButton *refreshBtn = new QPushButton("Refresh", this);
    m_toolbar->addWidget(refreshBtn);
    connect(refreshBtn, &QPushButton::clicked, this, &KeymapEditorDialog::onRefreshProfiles);
}

void KeymapEditorDialog::initMenuBar()
{
    QMenuBar *menuBar = new QMenuBar(this);
    
    // File menu
    QMenu *fileMenu = menuBar->addMenu("&File");
    
    QAction *newAction = fileMenu->addAction("&New");
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &KeymapEditorDialog::onNew);
    
    QAction *openAction = fileMenu->addAction("&Open...");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &KeymapEditorDialog::onOpen);
    
    fileMenu->addSeparator();
    
    QAction *saveAction = fileMenu->addAction("&Save");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &KeymapEditorDialog::onSave);
    
    QAction *saveAsAction = fileMenu->addAction("Save &As...");
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &KeymapEditorDialog::onSaveAs);
    
    fileMenu->addSeparator();
    
    QAction *closeAction = fileMenu->addAction("&Close");
    closeAction->setShortcut(QKeySequence::Close);
    connect(closeAction, &QAction::triggered, this, &QDialog::close);
    
    // Edit menu
    QMenu *editMenu = menuBar->addMenu("&Edit");
    editMenu->addAction(m_undoAction);
    editMenu->addAction(m_redoAction);
    editMenu->addSeparator();
    editMenu->addAction(m_deleteAction);
    editMenu->addSeparator();
    
    // Copy/Paste/Duplicate
    QAction *copyAction = editMenu->addAction("&Copy");
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, this, &KeymapEditorDialog::onCopy);
    
    QAction *pasteAction = editMenu->addAction("&Paste");
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, this, &KeymapEditorDialog::onPaste);
    
    QAction *duplicateAction = editMenu->addAction("D&uplicate");
    duplicateAction->setShortcut(QKeySequence("Ctrl+D"));
    connect(duplicateAction, &QAction::triggered, this, &KeymapEditorDialog::onDuplicate);
    
    editMenu->addSeparator();
    editMenu->addAction(m_snapAction);
    editMenu->addSeparator();
    editMenu->addAction(m_previewAction);
    
    // Layer assignment
    editMenu->addSeparator();
    QAction *assignLayerAction = editMenu->addAction("Assign to &Layer...");
    assignLayerAction->setShortcut(QKeySequence("Ctrl+L"));
    connect(assignLayerAction, &QAction::triggered, this, &KeymapEditorDialog::onAssignSelectedToLayer);
    
    // Align sub-menu
    QMenu *alignMenu = editMenu->addMenu("&Align");
    QAction *alignLeftAct = alignMenu->addAction("Align Left");
    connect(alignLeftAct, &QAction::triggered, this, &KeymapEditorDialog::onAlignLeft);
    QAction *alignRightAct = alignMenu->addAction("Align Right");
    connect(alignRightAct, &QAction::triggered, this, &KeymapEditorDialog::onAlignRight);
    QAction *alignTopAct = alignMenu->addAction("Align Top");
    connect(alignTopAct, &QAction::triggered, this, &KeymapEditorDialog::onAlignTop);
    QAction *alignBottomAct = alignMenu->addAction("Align Bottom");
    connect(alignBottomAct, &QAction::triggered, this, &KeymapEditorDialog::onAlignBottom);
    alignMenu->addSeparator();
    QAction *alignCHAct = alignMenu->addAction("Center Horizontally");
    connect(alignCHAct, &QAction::triggered, this, &KeymapEditorDialog::onAlignCenterH);
    QAction *alignCVAct = alignMenu->addAction("Center Vertically");
    connect(alignCVAct, &QAction::triggered, this, &KeymapEditorDialog::onAlignCenterV);
    alignMenu->addSeparator();
    QAction *distHAct = alignMenu->addAction("Distribute Horizontally");
    connect(distHAct, &QAction::triggered, this, &KeymapEditorDialog::onDistributeH);
    QAction *distVAct = alignMenu->addAction("Distribute Vertically");
    connect(distVAct, &QAction::triggered, this, &KeymapEditorDialog::onDistributeV);
    
    // Add menu bar to layout
    layout()->setMenuBar(menuBar);
    
    // Settings menu
    QMenu *settingsMenu = menuBar->addMenu("&Settings");
    QAction *mouseMoveAct = settingsMenu->addAction("Mouse Move Map...");
    connect(mouseMoveAct, &QAction::triggered, this, &KeymapEditorDialog::onEditMouseMoveMap);
    QAction *switchKeyAct = settingsMenu->addAction("Switch Key...");
    connect(switchKeyAct, &QAction::triggered, this, &KeymapEditorDialog::onEditSwitchKey);
}

void KeymapEditorDialog::initStatusBar()
{
    m_statusBar = new QStatusBar(this);
    
    m_coordLabel = new QLabel("Position: (0.00, 0.00)", this);
    m_nodeCountLabel = new QLabel("Nodes: 0", this);
    
    m_statusBar->addWidget(m_coordLabel);
    m_statusBar->addPermanentWidget(m_nodeCountLabel);
}

void KeymapEditorDialog::initPropertiesPanel()
{
    m_propertiesPanel = new PropertiesPanel(this);
}

void KeymapEditorDialog::setupConnections()
{
    // Toolbar mode actions
    connect(m_selectAction,         &QAction::triggered, this, &KeymapEditorDialog::onSelectMode);
    connect(m_addClickAction,       &QAction::triggered, this, &KeymapEditorDialog::onAddClickNode);
    connect(m_addClickTwiceAction,  &QAction::triggered, this, &KeymapEditorDialog::onAddClickTwiceNode);
    connect(m_addDragAction,        &QAction::triggered, this, &KeymapEditorDialog::onAddDragNode);
    connect(m_addWASDAction,        &QAction::triggered, this, &KeymapEditorDialog::onAddWASDNode);
    connect(m_addClickMultiAction,  &QAction::triggered, this, &KeymapEditorDialog::onAddClickMultiNode);
    connect(m_addGestureAction,     &QAction::triggered, this, &KeymapEditorDialog::onAddGestureNode);
    connect(m_deleteAction,         &QAction::triggered, this, &KeymapEditorDialog::onDeleteSelected);
    
    // Profile selector
    connect(m_profileCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &KeymapEditorDialog::onProfileChanged);
    
    // EditorScene signals — the core of Phase 2 interactivity
    connect(m_scene, &EditorScene::canvasClicked,
            this,    &KeymapEditorDialog::onCanvasClicked);
    connect(m_scene, &EditorScene::itemDoubleClicked,
            this,    &KeymapEditorDialog::onItemDoubleClicked);
    connect(m_scene, &EditorScene::mouseMoved,
            this,    &KeymapEditorDialog::onMouseMoved);
    connect(m_scene, &EditorScene::selectionChanged,
            this,    &KeymapEditorDialog::onSelectionChanged);
    
    // Context menu
    connect(m_scene, &EditorScene::contextMenuRequested,
            this,    &KeymapEditorDialog::onContextMenu);
    
    // Drag completion → push MoveNodeCommand for undo
    connect(m_scene, &EditorScene::nodeDragFinished,
            this, [this](KeyNode *node, const QPointF &oldRelPos, const QPointF &newRelPos) {
        // If snap-to-grid is enabled, snap the final position
        QPointF finalPos = m_snapToGrid ? snapToGrid(newRelPos) : newRelPos;
        if (finalPos != newRelPos) {
            node->setRelativePosition(finalPos);
            QPointF screenPos = relativeToScreen(finalPos);
            screenPos -= QPointF(node->rect().width() / 2.0, node->rect().height() / 2.0);
            node->setPos(screenPos);
        }
        m_undoStack->push(new MoveNodeCommand(node, oldRelPos, finalPos));
        m_modified = true;
    });
    
    // Properties panel
    connect(m_propertiesPanel, &PropertiesPanel::nodeModified,
            this,              &KeymapEditorDialog::onNodeModifiedByPanel);
    
    // Layer panel
    connect(m_layerPanel, &LayerPanel::layerVisibilityChanged,
            this,         &KeymapEditorDialog::onLayerVisibilityChanged);
    connect(m_layerPanel, &LayerPanel::activeLayerChanged,
            this,         &KeymapEditorDialog::onActiveLayerChanged);
    connect(m_layerPanel, &LayerPanel::layerRemoved,
            this,         &KeymapEditorDialog::onLayerRemoved);
}

// ============================================================================
// Mode Slot Implementations
// ============================================================================

void KeymapEditorDialog::onSelectMode()
{
    m_currentMode = SelectMode;
    m_canvasView->setCursor(Qt::ArrowCursor);
    m_canvasView->setDragMode(QGraphicsView::RubberBandDrag);
    updateModeActions();
    updateStatusBar("Select mode — click to select, Shift+click for multi-select, drag rectangle to select area");
}

void KeymapEditorDialog::onAddClickNode()
{
    m_currentMode = AddClickMode;
    m_canvasView->setCursor(Qt::CrossCursor);
    m_canvasView->setDragMode(QGraphicsView::NoDrag);
    updateModeActions();
    updateStatusBar("Click on canvas to add Click node");
}

void KeymapEditorDialog::onAddClickTwiceNode()
{
    m_currentMode = AddClickTwiceMode;
    m_canvasView->setCursor(Qt::CrossCursor);
    m_canvasView->setDragMode(QGraphicsView::NoDrag);
    updateModeActions();
    updateStatusBar("Click on canvas to add Click-Twice node");
}

void KeymapEditorDialog::onAddDragNode()
{
    m_currentMode = AddDragMode;
    m_canvasView->setCursor(Qt::CrossCursor);
    m_canvasView->setDragMode(QGraphicsView::NoDrag);
    updateModeActions();
    updateStatusBar("Click on canvas to add Drag node");
}

void KeymapEditorDialog::onAddWASDNode()
{
    m_currentMode = AddWASDMode;
    m_canvasView->setCursor(Qt::CrossCursor);
    m_canvasView->setDragMode(QGraphicsView::NoDrag);
    updateModeActions();
    updateStatusBar("Click on canvas to add WASD node");
}

void KeymapEditorDialog::onAddClickMultiNode()
{
    m_currentMode = AddClickMultiMode;
    m_canvasView->setCursor(Qt::CrossCursor);
    m_canvasView->setDragMode(QGraphicsView::NoDrag);
    updateModeActions();
    updateStatusBar("Click on canvas to add Multi-Click node");
}

void KeymapEditorDialog::onAddGestureNode()
{
    m_currentMode = AddGestureMode;
    m_canvasView->setCursor(Qt::CrossCursor);
    m_canvasView->setDragMode(QGraphicsView::NoDrag);
    updateModeActions();
    updateStatusBar("Click on canvas to add Gesture node (pinch/swipe/rotate)");
}

void KeymapEditorDialog::updateModeActions()
{
    static const char *labels[] = {"Select", "Click", "Click×2", "Drag", "WASD", "Multi", "Gesture"};
    static const char *colors[] = {"#4285F4", "#0F9D58", "#0F9D58", "#FBBC05", "#EA4335", "#9C27B0", "#9C27B0"};
    int idx = static_cast<int>(m_currentMode);
    m_modeLabel->setText(QString(" Mode: %1 ").arg(labels[idx]));
    m_modeLabel->setStyleSheet(
        QString("font-weight: bold; padding: 2px 8px; "
                "background: %1; color: white; border-radius: 3px;")
            .arg(colors[idx]));
}

void KeymapEditorDialog::onDeleteSelected()
{
    QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
    
    if (selectedItems.isEmpty()) {
        updateStatusBar("No node selected");
        return;
    }
    
    // Collect all selected KeyNodes
    QList<KeyNode*> nodesToDelete;
    for (QGraphicsItem *item : selectedItems) {
        KeyNode *node = dynamic_cast<KeyNode*>(item);
        if (node) {
            nodesToDelete.append(node);
        }
    }
    
    if (nodesToDelete.isEmpty())
        return;
    
    // Use macro command for multiple deletions
    if (nodesToDelete.size() > 1) {
        m_undoStack->beginMacro(QString("Delete %1 nodes").arg(nodesToDelete.size()));
    }
    
    for (KeyNode *node : nodesToDelete) {
        m_undoStack->push(new DeleteNodeCommand(this, node));
    }
    
    if (nodesToDelete.size() > 1) {
        m_undoStack->endMacro();
    }
    
    m_nodeCountLabel->setText(QString("Nodes: %1").arg(m_nodes.count()));
    updateStatusBar(QString("Deleted %1 node(s)").arg(nodesToDelete.size()));
}

void KeymapEditorDialog::onZoomIn()
{
    // No-op: zoom removed, canvas is always fitted
}

void KeymapEditorDialog::onZoomOut()
{
    // No-op: zoom removed, canvas is always fitted
}

void KeymapEditorDialog::onZoomReset()
{
    fitCanvasToView();
}

void KeymapEditorDialog::fitCanvasToView()
{
    if (!m_canvasView || !m_scene) return;
    // Fit the entire device screen inside the view, preserving aspect ratio.
    // Use a small margin (2px) so edges aren't clipped.
    m_canvasView->resetTransform();
    QRectF sr = m_scene->sceneRect();
    m_canvasView->fitInView(sr.adjusted(-2, -2, 2, 2), Qt::KeepAspectRatio);
    m_zoomLevel = 1.0;
}

void KeymapEditorDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    // Defer so the splitter and view have their final geometry
    QTimer::singleShot(0, this, &KeymapEditorDialog::fitCanvasToView);
}

void KeymapEditorDialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    fitCanvasToView();
}

// ============================================================================
// Interactive Scene Handlers (Phase 2 core)
// ============================================================================

void KeymapEditorDialog::onCanvasClicked(const QPointF &scenePos)
{
    // Macro recording: capture clicks
    if (m_macroRecording) {
        macroRecordClick(scenePos);
        return;
    }
    
    if (m_currentMode == SelectMode) {
        // Click on empty space → deselect
        deselectAll();
        return;
    }
    
    // We're in an Add mode — create the node at click position
    createNodeAtPosition(scenePos);
}

void KeymapEditorDialog::createNodeAtPosition(const QPointF &scenePos)
{
    QPointF relPos = screenToRelative(scenePos);
    
    // Determine dialog mode
    KeyAssignDialog::Mode dlgMode;
    switch (m_currentMode) {
        case AddClickMode:      dlgMode = KeyAssignDialog::CreateClick;      break;
        case AddClickTwiceMode: dlgMode = KeyAssignDialog::CreateClickTwice; break;
        case AddDragMode:       dlgMode = KeyAssignDialog::CreateDrag;       break;
        case AddWASDMode:       dlgMode = KeyAssignDialog::CreateWASD;       break;
        case AddClickMultiMode: dlgMode = KeyAssignDialog::CreateClickMulti; break;
        case AddGestureMode:    dlgMode = KeyAssignDialog::CreateGesture;    break;
        default: return;
    }
    
    // Show dialog
    KeyAssignDialog dlg(dlgMode, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    
    // Build the correct node type
    KeyNode *node = nullptr;
    
    switch (m_currentMode) {
        case AddClickMode: {
            auto *cn = new ClickNode(relPos);
            cn->setKeyCode(dlg.keyCode());
            cn->setComment(dlg.comment());
            cn->setSwitchMap(dlg.switchMap());
            node = cn;
            break;
        }
        case AddClickTwiceMode: {
            auto *cn = new ClickTwiceNode(relPos);
            cn->setKeyCode(dlg.keyCode());
            cn->setComment(dlg.comment());
            node = cn;
            break;
        }
        case AddDragMode: {
            auto *dn = new DragNode(relPos, dlg.dragEndPos());
            dn->setKeyCode(dlg.keyCode());
            dn->setComment(dlg.comment());
            dn->setDragSpeed(dlg.dragSpeed());
            node = dn;
            break;
        }
        case AddWASDMode: {
            auto *sw = new SteerWheelNode(relPos);
            sw->setComment(dlg.comment());
            sw->setDirectionKeys(dlg.leftKey(), dlg.rightKey(),
                                 dlg.upKey(), dlg.downKey());
            sw->setOffsets(dlg.leftOffset(), dlg.rightOffset(),
                          dlg.upOffset(), dlg.downOffset());
            node = sw;
            break;
        }
        case AddClickMultiMode: {
            auto *cm = new ClickMultiNode(relPos);
            cm->setKeyCode(dlg.keyCode());
            cm->setComment(dlg.comment());
            cm->setSwitchMap(dlg.switchMap());
            QList<ClickMultiNode::ClickPoint> points;
            for (const auto &entry : dlg.clickMultiPoints()) {
                ClickMultiNode::ClickPoint cp;
                cp.delay = entry.delay;
                cp.pos = entry.pos;
                points.append(cp);
            }
            cm->setClickPoints(points);
            node = cm;
            break;
        }
        case AddGestureMode: {
            auto *gn = new GestureNode(relPos);
            gn->setKeyCode(dlg.keyCode());
            gn->setComment(dlg.comment());
            gn->setSwitchMap(dlg.switchMap());
            auto gtype = static_cast<GestureNode::GestureType>(dlg.gestureTypeIndex());
            gn->applyPreset(gtype, dlg.gestureRadius());
            gn->setDuration(dlg.gestureDuration());
            node = gn;
            break;
        }
        default:
            return;
    }
    
    // Give the node device size before adding (needed for position sync)
    node->setDeviceSize(m_deviceSize);
    
    // Assign active layer
    if (m_layerPanel)
        node->setLayer(m_layerPanel->activeLayer());
    
    m_undoStack->push(new AddNodeCommand(this, node));
    selectNode(node);
    updateStatusBar(QString("Added %1 node").arg(node->typeString()));
}

void KeymapEditorDialog::onItemDoubleClicked(QGraphicsItem *item, const QPointF & /*scenePos*/)
{
    KeyNode *node = dynamic_cast<KeyNode*>(item);
    if (!node)
        return;
    
    editNodeViaDialog(node);
}

void KeymapEditorDialog::editNodeViaDialog(KeyNode *node)
{
    KeyAssignDialog dlg(KeyAssignDialog::EditNode, this);
    
    // Pre-fill common fields
    dlg.setKeyCode(node->keyCode());
    dlg.setComment(node->comment());
    dlg.setSwitchMap(node->switchMap());
    
    // Pre-fill type-specific and select correct type
    switch (node->nodeType()) {
        case KeyNode::Click:
            dlg.setNodeTypeIndex(0);
            break;
        case KeyNode::ClickTwice:
            dlg.setNodeTypeIndex(1);
            break;
        case KeyNode::Drag: {
            dlg.setNodeTypeIndex(2);
            auto *dn = static_cast<DragNode*>(node);
            dlg.setDragEndPos(dn->endPosition());
            dlg.setDragSpeed(dn->dragSpeed());
            break;
        }
        case KeyNode::SteerWheel: {
            dlg.setNodeTypeIndex(3);
            auto *sw = static_cast<SteerWheelNode*>(node);
            dlg.setWASDKeys(sw->leftKey(), sw->rightKey(),
                            sw->upKey(), sw->downKey());
            dlg.setWASDOffsets(sw->leftOffset(), sw->rightOffset(),
                               sw->upOffset(), sw->downOffset());
            break;
        }
        case KeyNode::ClickMulti: {
            dlg.setNodeTypeIndex(4);
            auto *cm = static_cast<ClickMultiNode*>(node);
            QList<KeyAssignDialog::ClickPointEntry> entries;
            for (const auto &cp : cm->clickPoints()) {
                KeyAssignDialog::ClickPointEntry e;
                e.delay = cp.delay;
                e.pos = cp.pos;
                entries.append(e);
            }
            dlg.setClickMultiPoints(entries);
            break;
        }
        case KeyNode::Gesture: {
            dlg.setNodeTypeIndex(5);
            auto *gn = static_cast<GestureNode*>(node);
            dlg.setGestureTypeIndex(static_cast<int>(gn->gestureType()));
            dlg.setGestureDuration(gn->duration());
            break;
        }
        default:
            break;
    }
    
    if (dlg.exec() != QDialog::Accepted)
        return;
    
    // Capture old state for undo
    QJsonObject oldState = node->toJson();
    
    // Apply changes back to node
    node->setKeyCode(dlg.keyCode());
    node->setComment(dlg.comment());
    node->setSwitchMap(dlg.switchMap());
    
    if (node->nodeType() == KeyNode::Drag) {
        auto *dn = static_cast<DragNode*>(node);
        dn->setEndPosition(dlg.dragEndPos());
        dn->setDragSpeed(dlg.dragSpeed());
    } else if (node->nodeType() == KeyNode::SteerWheel) {
        auto *sw = static_cast<SteerWheelNode*>(node);
        sw->setDirectionKeys(dlg.leftKey(), dlg.rightKey(),
                             dlg.upKey(), dlg.downKey());
        sw->setOffsets(dlg.leftOffset(), dlg.rightOffset(),
                       dlg.upOffset(), dlg.downOffset());
    } else if (node->nodeType() == KeyNode::ClickMulti) {
        auto *cm = static_cast<ClickMultiNode*>(node);
        QList<ClickMultiNode::ClickPoint> points;
        for (const auto &entry : dlg.clickMultiPoints()) {
            ClickMultiNode::ClickPoint cp;
            cp.delay = entry.delay;
            cp.pos = entry.pos;
            points.append(cp);
        }
        cm->setClickPoints(points);
    } else if (node->nodeType() == KeyNode::Gesture) {
        auto *gn = static_cast<GestureNode*>(node);
        auto gtype = static_cast<GestureNode::GestureType>(dlg.gestureTypeIndex());
        gn->applyPreset(gtype, 0.08);
        gn->setDuration(dlg.gestureDuration());
    }
    
    // Capture new state and push undo command
    QJsonObject newState = node->toJson();
    m_undoStack->push(new EditNodeCommand(node, oldState, newState));
    
    node->update();
    m_modified = true;
    
    // Refresh the properties panel
    if (m_selectedNode == node)
        m_propertiesPanel->setNode(node);
    
    updateStatusBar("Node updated");
}

void KeymapEditorDialog::onSelectionChanged()
{
    QList<QGraphicsItem*> items = m_scene->selectedItems();
    
    if (items.isEmpty()) {
        m_selectedNode = nullptr;
        m_propertiesPanel->setNode(nullptr);
        return;
    }
    
    // Count selected nodes
    int nodeCount = 0;
    KeyNode *firstNode = nullptr;
    for (QGraphicsItem *item : items) {
        KeyNode *node = dynamic_cast<KeyNode*>(item);
        if (node) {
            ++nodeCount;
            if (!firstNode) firstNode = node;
        }
    }
    
    if (firstNode) {
        m_selectedNode = firstNode;
        m_propertiesPanel->setNode(firstNode);
        if (nodeCount > 1) {
            updateStatusBar(QString("%1 nodes selected").arg(nodeCount));
        }
    }
}

void KeymapEditorDialog::onMouseMoved(const QPointF &scenePos)
{
    QPointF rel = screenToRelative(scenePos);
    m_coordLabel->setText(
        QString("Position: (%1, %2)")
            .arg(rel.x(), 0, 'f', 3)
            .arg(rel.y(), 0, 'f', 3));
}

void KeymapEditorDialog::onNodeModifiedByPanel()
{
    if (!m_selectedNode)
        return;
    
    // Sync position on canvas from relative coords
    QPointF screenPos = relativeToScreen(m_selectedNode->relativePosition());
    m_selectedNode->setPos(screenPos);
    m_selectedNode->update();
    m_modified = true;
}

void KeymapEditorDialog::selectNode(KeyNode *node)
{
    deselectAll();
    if (node) {
        node->setSelected(true);
        m_selectedNode = node;
        m_propertiesPanel->setNode(node);
    }
}

void KeymapEditorDialog::deselectAll()
{
    m_scene->clearSelection();
    m_selectedNode = nullptr;
    m_propertiesPanel->setNode(nullptr);
}

void KeymapEditorDialog::onNew()
{
    if (m_modified) {
        auto ret = QMessageBox::question(
            this, "Unsaved Changes",
            "Current keymap has unsaved changes. Discard?",
            QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::No)
            return;
    }
    
    clearCanvas();
    m_currentProfile.clear();
    m_currentFilePath.clear();
    m_modified = false;
    setWindowTitle("Zentroid Keymap Editor — New");
    updateStatusBar("New keymap created");
}

void KeymapEditorDialog::onOpen()
{
    if (m_modified) {
        auto ret = QMessageBox::question(
            this, "Unsaved Changes",
            "Current keymap has unsaved changes. Discard?",
            QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::No)
            return;
    }
    
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Open Keymap",
        getCanonicalKeymapDir(),
        "JSON Files (*.json)"
    );
    
    if (!fileName.isEmpty()) {
        if (loadKeymap(fileName)) {
            m_currentFilePath = fileName;
            m_currentProfile = QFileInfo(fileName).baseName();
            setWindowTitle("Keymap Editor — " + m_currentProfile + " [" + fileName + "]");
            updateStatusBar("Loaded: " + fileName);
        }
    }
}

void KeymapEditorDialog::onSave()
{
    if (m_currentFilePath.isEmpty()) {
        onSaveAs();
    } else {
        if (saveKeymap(m_currentFilePath)) {
            m_modified = false;
            updateStatusBar("Saved: " + m_currentFilePath);
        }
    }
}

void KeymapEditorDialog::onSaveAs()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Save Keymap As",
        getCanonicalKeymapDir() + "/" + generateDefaultName(),
        "JSON Files (*.json)"
    );
    
    if (!fileName.isEmpty()) {
        if (saveKeymap(fileName)) {
            m_currentFilePath = fileName;
            m_currentProfile = QFileInfo(fileName).baseName();
            m_modified = false;
            onRefreshProfiles();
            setWindowTitle("Keymap Editor — " + m_currentProfile + " [" + fileName + "]");
            updateStatusBar("Saved: " + fileName);
        }
    }
}

void KeymapEditorDialog::onExport()
{
    // Same as Save As for now
    onSaveAs();
}

void KeymapEditorDialog::onImport()
{
    // Same as Open for now
    onOpen();
}

void KeymapEditorDialog::onProfileChanged(int index)
{
    if (index < 0) return;
    
    QString profilePath = m_profileCombo->itemData(index).toString();
    if (!profilePath.isEmpty() && QFile::exists(profilePath)) {
        if (m_modified) {
            auto ret = QMessageBox::question(
                this, "Unsaved Changes",
                "Current keymap has unsaved changes. Discard?",
                QMessageBox::Yes | QMessageBox::No);
            if (ret == QMessageBox::No)
                return;
        }
        loadKeymap(profilePath);
    }
}

void KeymapEditorDialog::onRefreshProfiles()
{
    loadKeymapProfiles();
    updateStatusBar("Profiles refreshed");
}

// ============================================================================
// Public Methods
// ============================================================================

bool KeymapEditorDialog::loadKeymap(const QString &filePath)
{
    qInfo() << "[Keymap] Editor: Loading keymap from:" << filePath;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Could not open file: " + filePath);
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        QMessageBox::warning(this, "Error", "Invalid JSON format");
        return false;
    }
    
    clearCanvas();
    bool success = parseKeymapJson(doc.object());
    
    if (success) {
        m_currentFilePath = filePath;
        m_currentProfile = QFileInfo(filePath).baseName();
        setWindowTitle("Keymap Editor — " + m_currentProfile + " [" + filePath + "]");
        m_modified = false;
    }
    
    return success;
}

bool KeymapEditorDialog::saveKeymap(const QString &filePath)
{
    QJsonObject jsonObj = generateKeymapJson();
    
    QJsonDocument doc(jsonObj);
    QByteArray data = doc.toJson(QJsonDocument::Indented);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "[Keymap] Editor: Failed to open for writing:" << filePath << file.errorString();
        QMessageBox::warning(this, "Error", "Could not save file: " + filePath + "\n" + file.errorString());
        return false;
    }
    
    qint64 written = file.write(data);
    file.close();
    
    if (written < 0) {
        qWarning() << "[Keymap] Editor: Write failed:" << filePath;
        QMessageBox::warning(this, "Error", "Write failed for: " + filePath);
        return false;
    }

    qInfo() << "[Keymap] Editor: Saved keymap to:" << filePath << "(" << written << "bytes)";
    
    // Notify listeners (Dialog / Device) that keymap was saved — reload into runtime
    emit keymapApplied(filePath);
    
    return true;
}

// ============================================================================
// Private Methods
// ============================================================================

void KeymapEditorDialog::clearCanvas()
{
    // Clear undo stack first (it may own some nodes)
    if (m_undoStack)
        m_undoStack->clear();
    
    // Remove all nodes
    for (KeyNode *node : m_nodes) {
        m_scene->removeItem(node);
        delete node;
    }
    m_nodes.clear();
    m_selectedNode = nullptr;
    if (m_propertiesPanel)
        m_propertiesPanel->setNode(nullptr);
    m_nodeCountLabel->setText("Nodes: 0");
    
    // Reset layers
    if (m_layerPanel)
        m_layerPanel->clear();
    
    // Remove grid lines if any
    removeGridOverlay();
}

void KeymapEditorDialog::addNodeToScene(KeyNode *node)
{
    if (!node) return;
    
    // Give the node device size for position sync during drag
    node->setDeviceSize(m_deviceSize);
    
    QPointF screenPos = relativeToScreen(node->relativePosition());
    // Center the node on the click position
    screenPos -= QPointF(node->rect().width() / 2.0, node->rect().height() / 2.0);
    node->setPos(screenPos);
    
    m_scene->addItem(node);
    m_nodes.append(node);
    
    m_nodeCountLabel->setText(QString("Nodes: %1").arg(m_nodes.count()));
    m_modified = true;
}

void KeymapEditorDialog::removeNodeFromScene(KeyNode *node)
{
    if (!node) return;
    m_nodes.removeOne(node);
    m_scene->removeItem(node);
    if (m_selectedNode == node) {
        m_selectedNode = nullptr;
        m_propertiesPanel->setNode(nullptr);
    }
    delete node;
    m_nodeCountLabel->setText(QString("Nodes: %1").arg(m_nodes.count()));
    m_modified = true;
}

void KeymapEditorDialog::refreshCanvas()
{
    // Reposition all nodes from their relative positions
    for (KeyNode *node : m_nodes) {
        QPointF screenPos = relativeToScreen(node->relativePosition());
        screenPos -= QPointF(node->rect().width() / 2.0, node->rect().height() / 2.0);
        node->setPos(screenPos);
        node->update();
    }
}

QPointF KeymapEditorDialog::screenToRelative(const QPointF &screenPos) const
{
    return QPointF(
        screenPos.x() / m_deviceSize.width(),
        screenPos.y() / m_deviceSize.height()
    );
}

QPointF KeymapEditorDialog::relativeToScreen(const QPointF &relativePos) const
{
    return QPointF(
        relativePos.x() * m_deviceSize.width(),
        relativePos.y() * m_deviceSize.height()
    );
}

void KeymapEditorDialog::loadKeymapProfiles()
{
    m_profileCombo->clear();
    
    QDir keymapDir(getCanonicalKeymapDir());
    if (!keymapDir.exists()) {
        keymapDir.mkpath(".");
    }
    
    QStringList filters;
    filters << "*.json";
    QFileInfoList files = keymapDir.entryInfoList(filters, QDir::Files);
    
    for (const QFileInfo &fileInfo : files) {
        m_profileCombo->addItem(fileInfo.baseName(), fileInfo.absoluteFilePath());
    }
}

bool KeymapEditorDialog::parseKeymapJson(const QJsonObject &json)
{
    if (!json.contains("keyMapNodes")) {
        return false;
    }
    
    // Read global settings
    if (json.contains("switchKey"))
        m_switchKey = json["switchKey"].toString();
    else
        m_switchKey = "Key_QuoteLeft";
    
    m_hasMouseMoveMap = json.contains("mouseMoveMap");
    if (m_hasMouseMoveMap)
        m_mouseMoveMap = json["mouseMoveMap"].toObject();
    
    QJsonArray nodes = json["keyMapNodes"].toArray();
    
    for (const QJsonValue &value : nodes) {
        QJsonObject nodeObj = value.toObject();
        KeyNode *node = KeyNode::fromJson(nodeObj);
        
        if (node) {
            // Restore editor layer
            QString layer = nodeObj["_editorLayer"].toString("Default");
            node->setLayer(layer);
            if (m_layerPanel && !m_layerPanel->layerNames().contains(layer))
                m_layerPanel->addLayer(layer);
            addNodeToScene(node);
        }
    }
    
    updateNodeVisibilityByLayers();
    return true;
}

QJsonObject KeymapEditorDialog::generateKeymapJson() const
{
    QJsonObject json;
    
    // Global switch key
    json["switchKey"] = m_switchKey.isEmpty() ? "Key_QuoteLeft" : m_switchKey;
    
    // Preserve mouseMoveMap if present
    if (m_hasMouseMoveMap)
        json["mouseMoveMap"] = m_mouseMoveMap;
    
    // Add keyMapNodes array
    QJsonArray nodesArray;
    for (KeyNode *node : m_nodes) {
        QJsonObject obj = node->toJson();
        // Store editor layer metadata
        if (!node->layer().isEmpty() && node->layer() != "Default")
            obj["_editorLayer"] = node->layer();
        nodesArray.append(obj);
    }
    json["keyMapNodes"] = nodesArray;
    
    return json;
}

void KeymapEditorDialog::updateStatusBar(const QString &message)
{
    m_statusBar->showMessage(message, 3000);
}

QString KeymapEditorDialog::generateDefaultName() const
{
    return QString("keymap_%1.json")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
}

// ============================================================================
// Undo/Redo Helpers (called by AddNodeCommand / DeleteNodeCommand)
// ============================================================================

void KeymapEditorDialog::undoAddNode(KeyNode *node)
{
    if (!node) return;
    
    node->setDeviceSize(m_deviceSize);
    QPointF screenPos = relativeToScreen(node->relativePosition());
    screenPos -= QPointF(node->rect().width() / 2.0, node->rect().height() / 2.0);
    node->setPos(screenPos);
    
    m_scene->addItem(node);
    m_nodes.append(node);
    m_nodeCountLabel->setText(QString("Nodes: %1").arg(m_nodes.count()));
    m_modified = true;
}

void KeymapEditorDialog::undoRemoveNode(KeyNode *node)
{
    if (!node) return;
    
    m_nodes.removeOne(node);
    m_scene->removeItem(node);
    
    if (m_selectedNode == node) {
        m_selectedNode = nullptr;
        m_propertiesPanel->setNode(nullptr);
    }
    
    m_nodeCountLabel->setText(QString("Nodes: %1").arg(m_nodes.count()));
    m_modified = true;
}

// ============================================================================
// Snap-to-Grid
// ============================================================================

void KeymapEditorDialog::onToggleSnapToGrid(bool checked)
{
    m_snapToGrid = checked;
    if (checked) {
        drawGridOverlay();
        updateStatusBar(QString("Snap-to-grid ON (grid size: %1%)").arg(qRound(m_gridSize * 100)));
    } else {
        removeGridOverlay();
        updateStatusBar("Snap-to-grid OFF");
    }
}

QPointF KeymapEditorDialog::snapToGrid(const QPointF &relPos) const
{
    if (!m_snapToGrid || m_gridSize <= 0.0)
        return relPos;
    
    qreal x = qRound(relPos.x() / m_gridSize) * m_gridSize;
    qreal y = qRound(relPos.y() / m_gridSize) * m_gridSize;
    return QPointF(qBound(0.0, x, 1.0), qBound(0.0, y, 1.0));
}

void KeymapEditorDialog::drawGridOverlay()
{
    removeGridOverlay();
    
    QPen gridPen(QColor(255, 255, 255, 40), 1, Qt::DotLine);
    
    int w = m_deviceSize.width();
    int h = m_deviceSize.height();
    
    // Vertical lines
    for (qreal rx = m_gridSize; rx < 1.0; rx += m_gridSize) {
        qreal x = rx * w;
        auto *line = m_scene->addLine(x, 0, x, h, gridPen);
        line->setZValue(-0.5); // Above background, below nodes
        m_gridLines.append(line);
    }
    
    // Horizontal lines
    for (qreal ry = m_gridSize; ry < 1.0; ry += m_gridSize) {
        qreal y = ry * h;
        auto *line = m_scene->addLine(0, y, w, y, gridPen);
        line->setZValue(-0.5);
        m_gridLines.append(line);
    }
}

void KeymapEditorDialog::removeGridOverlay()
{
    for (QGraphicsItem *item : m_gridLines) {
        m_scene->removeItem(item);
        delete item;
    }
    m_gridLines.clear();
}

// ============================================================================
// Alignment Tools
// ============================================================================

static QList<KeyNode*> getSelectedKeyNodes(EditorScene *scene)
{
    QList<KeyNode*> nodes;
    for (QGraphicsItem *item : scene->selectedItems()) {
        KeyNode *n = dynamic_cast<KeyNode*>(item);
        if (n) nodes.append(n);
    }
    return nodes;
}

void KeymapEditorDialog::onAlignLeft()
{
    QList<KeyNode*> nodes = getSelectedKeyNodes(m_scene);
    if (nodes.size() < 2) {
        updateStatusBar("Select 2+ nodes to align");
        return;
    }
    
    qreal minX = 1.0;
    for (KeyNode *n : nodes)
        minX = qMin(minX, n->relativePosition().x());
    
    m_undoStack->beginMacro("Align Left");
    for (KeyNode *n : nodes) {
        QPointF oldRel = n->relativePosition();
        QPointF newRel(minX, oldRel.y());
        if (oldRel != newRel) {
            m_undoStack->push(new MoveNodeCommand(n, oldRel, newRel));
        }
    }
    m_undoStack->endMacro();
    updateStatusBar("Aligned left");
}

void KeymapEditorDialog::onAlignRight()
{
    QList<KeyNode*> nodes = getSelectedKeyNodes(m_scene);
    if (nodes.size() < 2) {
        updateStatusBar("Select 2+ nodes to align");
        return;
    }
    
    qreal maxX = 0.0;
    for (KeyNode *n : nodes)
        maxX = qMax(maxX, n->relativePosition().x());
    
    m_undoStack->beginMacro("Align Right");
    for (KeyNode *n : nodes) {
        QPointF oldRel = n->relativePosition();
        QPointF newRel(maxX, oldRel.y());
        if (oldRel != newRel)
            m_undoStack->push(new MoveNodeCommand(n, oldRel, newRel));
    }
    m_undoStack->endMacro();
    updateStatusBar("Aligned right");
}

void KeymapEditorDialog::onAlignTop()
{
    QList<KeyNode*> nodes = getSelectedKeyNodes(m_scene);
    if (nodes.size() < 2) {
        updateStatusBar("Select 2+ nodes to align");
        return;
    }
    
    qreal minY = 1.0;
    for (KeyNode *n : nodes)
        minY = qMin(minY, n->relativePosition().y());
    
    m_undoStack->beginMacro("Align Top");
    for (KeyNode *n : nodes) {
        QPointF oldRel = n->relativePosition();
        QPointF newRel(oldRel.x(), minY);
        if (oldRel != newRel)
            m_undoStack->push(new MoveNodeCommand(n, oldRel, newRel));
    }
    m_undoStack->endMacro();
    updateStatusBar("Aligned top");
}

void KeymapEditorDialog::onAlignBottom()
{
    QList<KeyNode*> nodes = getSelectedKeyNodes(m_scene);
    if (nodes.size() < 2) {
        updateStatusBar("Select 2+ nodes to align");
        return;
    }
    
    qreal maxY = 0.0;
    for (KeyNode *n : nodes)
        maxY = qMax(maxY, n->relativePosition().y());
    
    m_undoStack->beginMacro("Align Bottom");
    for (KeyNode *n : nodes) {
        QPointF oldRel = n->relativePosition();
        QPointF newRel(oldRel.x(), maxY);
        if (oldRel != newRel)
            m_undoStack->push(new MoveNodeCommand(n, oldRel, newRel));
    }
    m_undoStack->endMacro();
    updateStatusBar("Aligned bottom");
}

void KeymapEditorDialog::onAlignCenterH()
{
    QList<KeyNode*> nodes = getSelectedKeyNodes(m_scene);
    if (nodes.size() < 2) {
        updateStatusBar("Select 2+ nodes to align");
        return;
    }
    
    qreal sum = 0.0;
    for (KeyNode *n : nodes)
        sum += n->relativePosition().x();
    qreal avg = sum / nodes.size();
    
    m_undoStack->beginMacro("Center Horizontally");
    for (KeyNode *n : nodes) {
        QPointF oldRel = n->relativePosition();
        QPointF newRel(avg, oldRel.y());
        if (oldRel != newRel)
            m_undoStack->push(new MoveNodeCommand(n, oldRel, newRel));
    }
    m_undoStack->endMacro();
    updateStatusBar("Centered horizontally");
}

void KeymapEditorDialog::onAlignCenterV()
{
    QList<KeyNode*> nodes = getSelectedKeyNodes(m_scene);
    if (nodes.size() < 2) {
        updateStatusBar("Select 2+ nodes to align");
        return;
    }
    
    qreal sum = 0.0;
    for (KeyNode *n : nodes)
        sum += n->relativePosition().y();
    qreal avg = sum / nodes.size();
    
    m_undoStack->beginMacro("Center Vertically");
    for (KeyNode *n : nodes) {
        QPointF oldRel = n->relativePosition();
        QPointF newRel(oldRel.x(), avg);
        if (oldRel != newRel)
            m_undoStack->push(new MoveNodeCommand(n, oldRel, newRel));
    }
    m_undoStack->endMacro();
    updateStatusBar("Centered vertically");
}

void KeymapEditorDialog::onDistributeH()
{
    QList<KeyNode*> nodes = getSelectedKeyNodes(m_scene);
    if (nodes.size() < 3) {
        updateStatusBar("Select 3+ nodes to distribute");
        return;
    }
    
    // Sort by X
    std::sort(nodes.begin(), nodes.end(), [](KeyNode *a, KeyNode *b) {
        return a->relativePosition().x() < b->relativePosition().x();
    });
    
    qreal minX = nodes.first()->relativePosition().x();
    qreal maxX = nodes.last()->relativePosition().x();
    qreal step = (maxX - minX) / (nodes.size() - 1);
    
    m_undoStack->beginMacro("Distribute Horizontally");
    for (int i = 1; i < nodes.size() - 1; ++i) {
        QPointF oldRel = nodes[i]->relativePosition();
        QPointF newRel(minX + step * i, oldRel.y());
        if (oldRel != newRel)
            m_undoStack->push(new MoveNodeCommand(nodes[i], oldRel, newRel));
    }
    m_undoStack->endMacro();
    updateStatusBar("Distributed horizontally");
}

void KeymapEditorDialog::onDistributeV()
{
    QList<KeyNode*> nodes = getSelectedKeyNodes(m_scene);
    if (nodes.size() < 3) {
        updateStatusBar("Select 3+ nodes to distribute");
        return;
    }
    
    // Sort by Y
    std::sort(nodes.begin(), nodes.end(), [](KeyNode *a, KeyNode *b) {
        return a->relativePosition().y() < b->relativePosition().y();
    });
    
    qreal minY = nodes.first()->relativePosition().y();
    qreal maxY = nodes.last()->relativePosition().y();
    qreal step = (maxY - minY) / (nodes.size() - 1);
    
    m_undoStack->beginMacro("Distribute Vertically");
    for (int i = 1; i < nodes.size() - 1; ++i) {
        QPointF oldRel = nodes[i]->relativePosition();
        QPointF newRel(oldRel.x(), minY + step * i);
        if (oldRel != newRel)
            m_undoStack->push(new MoveNodeCommand(nodes[i], oldRel, newRel));
    }
    m_undoStack->endMacro();
    updateStatusBar("Distributed vertically");
}

// ============================================================================
// Template Presets
// ============================================================================

void KeymapEditorDialog::initTemplatePresets()
{
    m_templateCombo->addItem("(select template)", QVariant());
    m_templateCombo->addItem("FPS / Shooter", 0);
    m_templateCombo->addItem("MOBA", 1);
    m_templateCombo->addItem("Racing", 2);
    m_templateCombo->addItem("Platformer", 3);
}

void KeymapEditorDialog::onLoadTemplate(int index)
{
    if (index <= 0) return;  // "(select template)" placeholder
    
    if (!m_nodes.isEmpty()) {
        auto ret = QMessageBox::question(
            this, "Load Template",
            "Loading a template will replace all current nodes. Continue?",
            QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::No) {
            m_templateCombo->setCurrentIndex(0);
            return;
        }
    }
    
    clearCanvas();
    
    int templateId = m_templateCombo->itemData(index).toInt();
    
    switch (templateId) {
        case 0: {
            // FPS / Shooter template
            // WASD movement center-left
            auto *wasd = new SteerWheelNode(QPointF(0.20, 0.65));
            wasd->setComment("Movement");
            wasd->setDirectionKeys("Key_A", "Key_D", "Key_W", "Key_S");
            wasd->setOffsets(0.08, 0.08, 0.08, 0.08);
            addNodeToScene(wasd);
            
            // Jump
            auto *jump = new ClickNode(QPointF(0.85, 0.75));
            jump->setKeyCode("Key_Space");
            jump->setComment("Jump");
            addNodeToScene(jump);
            
            // Crouch
            auto *crouch = new ClickNode(QPointF(0.75, 0.85));
            crouch->setKeyCode("Key_C");
            crouch->setComment("Crouch");
            addNodeToScene(crouch);
            
            // Reload
            auto *reload = new ClickNode(QPointF(0.90, 0.55));
            reload->setKeyCode("Key_R");
            reload->setComment("Reload");
            addNodeToScene(reload);
            
            // Weapon switch
            auto *weapon = new ClickNode(QPointF(0.50, 0.15));
            weapon->setKeyCode("Key_1");
            weapon->setComment("Weapon 1");
            addNodeToScene(weapon);
            
            auto *weapon2 = new ClickNode(QPointF(0.55, 0.15));
            weapon2->setKeyCode("Key_2");
            weapon2->setComment("Weapon 2");
            addNodeToScene(weapon2);
            
            // Scope/Aim
            auto *scope = new ClickNode(QPointF(0.10, 0.45));
            scope->setKeyCode("Key_Q");
            scope->setComment("Scope");
            addNodeToScene(scope);
            
            // Fire
            auto *fire = new ClickNode(QPointF(0.90, 0.45));
            fire->setKeyCode("Key_F");
            fire->setComment("Fire");
            addNodeToScene(fire);
            
            break;
        }
        case 1: {
            // MOBA template
            auto *wasd = new SteerWheelNode(QPointF(0.15, 0.70));
            wasd->setComment("Move");
            wasd->setDirectionKeys("Key_A", "Key_D", "Key_W", "Key_S");
            wasd->setOffsets(0.10, 0.10, 0.10, 0.10);
            addNodeToScene(wasd);
            
            // Skill buttons (QWER style)
            const char* skillKeys[] = {"Key_Q", "Key_W", "Key_E", "Key_R"};
            const char* skillNames[] = {"Skill 1", "Skill 2", "Skill 3", "Ultimate"};
            for (int i = 0; i < 4; ++i) {
                auto *skill = new ClickNode(QPointF(0.65 + i * 0.08, 0.85));
                skill->setKeyCode(skillKeys[i]);
                skill->setComment(skillNames[i]);
                addNodeToScene(skill);
            }
            
            // Attack
            auto *atk = new ClickNode(QPointF(0.85, 0.65));
            atk->setKeyCode("Key_A");
            atk->setComment("Attack");
            addNodeToScene(atk);
            
            // Recall
            auto *recall = new ClickNode(QPointF(0.50, 0.15));
            recall->setKeyCode("Key_B");
            recall->setComment("Recall");
            addNodeToScene(recall);
            
            break;
        }
        case 2: {
            // Racing template
            auto *accel = new ClickNode(QPointF(0.85, 0.60));
            accel->setKeyCode("Key_W");
            accel->setComment("Accelerate");
            addNodeToScene(accel);
            
            auto *brake = new ClickNode(QPointF(0.15, 0.60));
            brake->setKeyCode("Key_S");
            brake->setComment("Brake");
            addNodeToScene(brake);
            
            // Steering drag
            auto *steerL = new DragNode(QPointF(0.50, 0.80), QPointF(0.30, 0.80));
            steerL->setKeyCode("Key_A");
            steerL->setComment("Steer Left");
            addNodeToScene(steerL);
            
            auto *steerR = new DragNode(QPointF(0.50, 0.80), QPointF(0.70, 0.80));
            steerR->setKeyCode("Key_D");
            steerR->setComment("Steer Right");
            addNodeToScene(steerR);
            
            // Nitro
            auto *nitro = new ClickNode(QPointF(0.85, 0.40));
            nitro->setKeyCode("Key_Space");
            nitro->setComment("Nitro");
            addNodeToScene(nitro);
            
            break;
        }
        case 3: {
            // Platformer template
            auto *wasd = new SteerWheelNode(QPointF(0.20, 0.75));
            wasd->setComment("Move");
            wasd->setDirectionKeys("Key_A", "Key_D", "Key_W", "Key_S");
            wasd->setOffsets(0.12, 0.12, 0.12, 0.12);
            addNodeToScene(wasd);
            
            auto *jump = new ClickNode(QPointF(0.85, 0.70));
            jump->setKeyCode("Key_Space");
            jump->setComment("Jump");
            addNodeToScene(jump);
            
            auto *action = new ClickNode(QPointF(0.80, 0.55));
            action->setKeyCode("Key_E");
            action->setComment("Action");
            addNodeToScene(action);
            
            auto *attack = new ClickNode(QPointF(0.90, 0.55));
            attack->setKeyCode("Key_F");
            attack->setComment("Attack");
            addNodeToScene(attack);
            
            break;
        }
    }
    
    m_undoStack->clear();  // Templates start fresh — no undo for initial layout
    m_modified = false;
    m_templateCombo->setCurrentIndex(0);
    updateStatusBar("Template loaded");
}

// ============================================================================
// Layer Panel
// ============================================================================

void KeymapEditorDialog::initLayerPanel()
{
    m_layerPanel = new LayerPanel(this);
}

void KeymapEditorDialog::onLayerVisibilityChanged(const QString &name, bool visible)
{
    Q_UNUSED(visible);
    updateNodeVisibilityByLayers();
    updateStatusBar(QString("Layer \"%1\" %2").arg(name, visible ? "shown" : "hidden"));
}

void KeymapEditorDialog::onActiveLayerChanged(const QString &name)
{
    updateStatusBar(QString("Active layer: %1").arg(name));
}

void KeymapEditorDialog::onLayerRemoved(const QString &name)
{
    // Move all nodes in the removed layer to "Default"
    for (KeyNode *node : m_nodes) {
        if (node->layer() == name) {
            node->setLayer("Default");
        }
    }
    updateNodeVisibilityByLayers();
    updateStatusBar(QString("Layer \"%1\" removed — nodes moved to Default").arg(name));
}

void KeymapEditorDialog::onAssignSelectedToLayer()
{
    if (!m_layerPanel) return;
    
    QString targetLayer = m_layerPanel->activeLayer();
    QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
    int count = 0;
    
    for (QGraphicsItem *item : selectedItems) {
        KeyNode *node = dynamic_cast<KeyNode*>(item);
        if (node) {
            node->setLayer(targetLayer);
            ++count;
        }
    }
    
    if (count > 0) {
        updateNodeVisibilityByLayers();
        m_modified = true;
        updateStatusBar(QString("Assigned %1 node(s) to layer \"%2\"").arg(count).arg(targetLayer));
    }
}

void KeymapEditorDialog::updateNodeVisibilityByLayers()
{
    if (!m_layerPanel) return;
    
    for (KeyNode *node : m_nodes) {
        bool vis = m_layerPanel->isLayerVisible(node->layer());
        node->setVisible(vis);
    }
}

// ============================================================================
// Macro Recording
// ============================================================================

void KeymapEditorDialog::onToggleMacroRecord(bool checked)
{
    m_macroRecording = checked;
    
    if (checked) {
        // Start recording
        m_macroPoints.clear();
        m_macroTimer.start();
        
        m_canvasView->setCursor(Qt::CrossCursor);
        m_canvasView->setDragMode(QGraphicsView::NoDrag);
        
        m_modeLabel->setText(" Mode: ⏺ REC ");
        m_modeLabel->setStyleSheet("font-weight: bold; padding: 2px 8px; "
                                   "background: #D32F2F; color: white; border-radius: 3px;");
        
        updateStatusBar("⏺ RECORDING — Click on canvas to record touch points. Press R or toggle to stop.");
    } else {
        // Stop recording — build a ClickMultiNode from recorded points
        m_canvasView->setCursor(Qt::ArrowCursor);
        updateModeActions();
        
        if (m_macroPoints.size() < 2) {
            updateStatusBar("Macro cancelled — need at least 2 clicks");
            m_macroPoints.clear();
            return;
        }
        
        // Ask for key assignment
        KeyAssignDialog dlg(KeyAssignDialog::CreateClickMulti, this);
        
        // Pre-fill the recorded points
        QList<KeyAssignDialog::ClickPointEntry> entries;
        for (const MacroPoint &mp : m_macroPoints) {
            KeyAssignDialog::ClickPointEntry e;
            e.delay = mp.delay;
            e.pos   = mp.relPos;
            entries.append(e);
        }
        dlg.setClickMultiPoints(entries);
        
        if (dlg.exec() != QDialog::Accepted) {
            m_macroPoints.clear();
            updateStatusBar("Macro cancelled");
            return;
        }
        
        // Build the node
        QPointF firstPos = m_macroPoints.first().relPos;
        auto *cm = new ClickMultiNode(firstPos);
        cm->setKeyCode(dlg.keyCode());
        cm->setComment(dlg.comment());
        cm->setSwitchMap(dlg.switchMap());
        
        QList<ClickMultiNode::ClickPoint> points;
        for (const auto &entry : dlg.clickMultiPoints()) {
            ClickMultiNode::ClickPoint cp;
            cp.delay = entry.delay;
            cp.pos   = entry.pos;
            points.append(cp);
        }
        cm->setClickPoints(points);
        
        cm->setDeviceSize(m_deviceSize);
        if (m_layerPanel)
            cm->setLayer(m_layerPanel->activeLayer());
        m_undoStack->push(new AddNodeCommand(this, cm));
        
        m_macroPoints.clear();
        updateStatusBar(QString("Macro recorded — %1 click points").arg(points.size()));
    }
}

void KeymapEditorDialog::macroRecordClick(const QPointF &scenePos)
{
    QPointF relPos = screenToRelative(scenePos);
    
    MacroPoint mp;
    mp.relPos = relPos;
    mp.delay  = m_macroPoints.isEmpty() ? 0 : static_cast<int>(m_macroTimer.elapsed());
    m_macroPoints.append(mp);
    
    // Restart timer for next interval
    m_macroTimer.restart();
    
    // Visual feedback: draw a small red dot
    auto *dot = m_scene->addEllipse(
        scenePos.x() - 5, scenePos.y() - 5, 10, 10,
        QPen(Qt::red, 2), QBrush(QColor(255, 0, 0, 120)));
    dot->setZValue(10);
    
    // Number label
    auto *label = m_scene->addSimpleText(
        QString::number(m_macroPoints.size()),
        QFont("Arial", 8, QFont::Bold));
    label->setPos(scenePos.x() + 6, scenePos.y() - 8);
    label->setBrush(Qt::red);
    label->setZValue(10);
    
    updateStatusBar(QString("⏺ REC — Point %1 at (%.2f, %.2f) — delay %3ms")
                    .arg(m_macroPoints.size())
                    .arg(relPos.x()).arg(relPos.y())
                    .arg(mp.delay));
}

// ============================================================================
// Live Preview Mode
// ============================================================================

void KeymapEditorDialog::onTogglePreview(bool checked)
{
    m_previewMode = checked;
    
    if (checked) {
        // Create highlight auto-clear timer
        if (!m_highlightTimer) {
            m_highlightTimer = new QTimer(this);
            m_highlightTimer->setInterval(150);
            connect(m_highlightTimer, &QTimer::timeout, this, &KeymapEditorDialog::clearAllHighlights);
        }
        
        // Grab keyboard focus
        setFocus();
        m_canvasView->setFocus();
        
        updateStatusBar("▶ PREVIEW MODE — Press keys to see which nodes they activate. Press P or toggle to exit.");
        
        // Change mode label to indicate preview
        m_modeLabel->setText(" Mode: PREVIEW ");
        m_modeLabel->setStyleSheet("font-weight: bold; padding: 2px 8px; "
                                   "background: #9C27B0; color: white; border-radius: 3px;");
    } else {
        // Clear all highlights
        clearAllHighlights();
        m_activePreviewKeys.clear();
        
        // Restore mode label
        updateModeActions();
        updateStatusBar("Preview mode OFF");
    }
}

void KeymapEditorDialog::clearAllHighlights()
{
    // Only clear highlights for keys not currently held
    for (KeyNode *node : m_nodes) {
        if (node->isHighlighted()) {
            // Check if any of the node's keys are still held
            bool stillActive = false;
            
            // Check main key
            if (m_activePreviewKeys.contains(node->keyCode())) {
                stillActive = true;
            }
            
            // Check WASD keys for SteerWheel nodes
            if (!stillActive && node->nodeType() == KeyNode::SteerWheel) {
                auto *sw = static_cast<SteerWheelNode*>(node);
                if (m_activePreviewKeys.contains(sw->leftKey()) ||
                    m_activePreviewKeys.contains(sw->rightKey()) ||
                    m_activePreviewKeys.contains(sw->upKey()) ||
                    m_activePreviewKeys.contains(sw->downKey())) {
                    stillActive = true;
                }
            }
            
            if (!stillActive) {
                node->setHighlighted(false);
            }
        }
    }
}

void KeymapEditorDialog::highlightNodesForKey(const QString &keyCode)
{
    for (KeyNode *node : m_nodes) {
        // Check main keyCode
        if (node->keyCode() == keyCode) {
            node->setHighlighted(true);
            continue;
        }
        
        // Check WASD direction keys for SteerWheel nodes
        if (node->nodeType() == KeyNode::SteerWheel) {
            auto *sw = static_cast<SteerWheelNode*>(node);
            if (sw->leftKey() == keyCode || sw->rightKey() == keyCode ||
                sw->upKey() == keyCode || sw->downKey() == keyCode) {
                sw->setHighlighted(true);
            }
        }
    }
}

void KeymapEditorDialog::unhighlightNodesForKey(const QString &keyCode)
{
    for (KeyNode *node : m_nodes) {
        if (node->keyCode() == keyCode) {
            // Only unhighlight if no other active keys match this node
            node->setHighlighted(false);
        }
        
        if (node->nodeType() == KeyNode::SteerWheel) {
            auto *sw = static_cast<SteerWheelNode*>(node);
            if (sw->leftKey() == keyCode || sw->rightKey() == keyCode ||
                sw->upKey() == keyCode || sw->downKey() == keyCode) {
                // Check if any other WASD key is still held
                bool otherHeld = false;
                if (keyCode != sw->leftKey() && m_activePreviewKeys.contains(sw->leftKey())) otherHeld = true;
                if (keyCode != sw->rightKey() && m_activePreviewKeys.contains(sw->rightKey())) otherHeld = true;
                if (keyCode != sw->upKey() && m_activePreviewKeys.contains(sw->upKey())) otherHeld = true;
                if (keyCode != sw->downKey() && m_activePreviewKeys.contains(sw->downKey())) otherHeld = true;
                if (!otherHeld) {
                    sw->setHighlighted(false);
                }
            }
        }
    }
}

void KeymapEditorDialog::keyPressEvent(QKeyEvent *event)
{
    if (m_previewMode && !event->isAutoRepeat()) {
        // Convert Qt::Key to "Key_X" string format
        QString keyStr = QString("Key_%1").arg(
            QMetaEnum::fromType<Qt::Key>().valueToKey(event->key()));
        
        // Remove the "Key_" prefix that QMetaEnum gives for Qt::Key_X -> "Key_Key_X"
        // Actually QMetaEnum::valueToKey for Qt::Key_A gives "Key_A", so we get "Key_Key_A"
        // We need just "Key_A", so strip the extra "Key_"
        if (keyStr.startsWith("Key_Key_")) {
            keyStr = keyStr.mid(4);  // "Key_Key_A" -> "Key_A"
        }
        
        m_activePreviewKeys.insert(keyStr);
        highlightNodesForKey(keyStr);
        
        updateStatusBar(QString("▶ PREVIEW: Key %1 pressed").arg(keyStr));
        
        event->accept();
        return;
    }
    
    QDialog::keyPressEvent(event);
}

void KeymapEditorDialog::keyReleaseEvent(QKeyEvent *event)
{
    if (m_previewMode && !event->isAutoRepeat()) {
        QString keyStr = QString("Key_%1").arg(
            QMetaEnum::fromType<Qt::Key>().valueToKey(event->key()));
        
        if (keyStr.startsWith("Key_Key_")) {
            keyStr = keyStr.mid(4);
        }
        
        m_activePreviewKeys.remove(keyStr);
        unhighlightNodesForKey(keyStr);
        
        event->accept();
        return;
    }
    
    QDialog::keyReleaseEvent(event);
}

// ============================================================================
// Context Menu
// ============================================================================

void KeymapEditorDialog::onContextMenu(const QPointF &scenePos, QGraphicsItem *item)
{
    QMenu menu(this);
    
    KeyNode *node = dynamic_cast<KeyNode*>(item);
    
    if (node) {
        // Select the node under the cursor
        selectNode(node);
        
        menu.addAction("✏ Edit...", this, [this, node]() {
            editNodeViaDialog(node);
        });
        menu.addSeparator();
        menu.addAction("📋 Copy\tCtrl+C", this, &KeymapEditorDialog::onCopy);
        menu.addAction("📄 Duplicate\tCtrl+D", this, &KeymapEditorDialog::onDuplicate);
        menu.addSeparator();
        menu.addAction("🗑 Delete\tDel", this, &KeymapEditorDialog::onDeleteSelected);
    } else {
        // Right-click on empty space
        if (!m_clipboard.isEmpty()) {
            menu.addAction("📋 Paste\tCtrl+V", this, [this, scenePos]() {
                // Paste at cursor position
                QPointF relPos = screenToRelative(scenePos);
                
                m_undoStack->beginMacro("Paste nodes");
                for (const QJsonValue &val : m_clipboard) {
                    QJsonObject obj = val.toObject();
                    
                    // Offset paste position slightly from cursor
                    if (obj.contains("pos")) {
                        QJsonObject pos = obj["pos"].toObject();
                        pos["x"] = relPos.x();
                        pos["y"] = relPos.y();
                        obj["pos"] = pos;
                    }
                    
                    KeyNode *newNode = KeyNode::fromJson(obj);
                    if (newNode) {
                        newNode->setRelativePosition(relPos);
                        newNode->setDeviceSize(m_deviceSize);
                        m_undoStack->push(new AddNodeCommand(this, newNode));
                    }
                }
                m_undoStack->endMacro();
            });
        }
        
        menu.addSeparator();
        
        // Quick add submenu
        QMenu *addMenu = menu.addMenu("➕ Add Node Here...");
        addMenu->addAction("Click", this, [this, scenePos]() {
            m_currentMode = AddClickMode;
            createNodeAtPosition(scenePos);
            onSelectMode();
        });
        addMenu->addAction("Click Twice", this, [this, scenePos]() {
            m_currentMode = AddClickTwiceMode;
            createNodeAtPosition(scenePos);
            onSelectMode();
        });
        addMenu->addAction("Drag", this, [this, scenePos]() {
            m_currentMode = AddDragMode;
            createNodeAtPosition(scenePos);
            onSelectMode();
        });
        addMenu->addAction("WASD", this, [this, scenePos]() {
            m_currentMode = AddWASDMode;
            createNodeAtPosition(scenePos);
            onSelectMode();
        });
        addMenu->addAction("Multi-Click", this, [this, scenePos]() {
            m_currentMode = AddClickMultiMode;
            createNodeAtPosition(scenePos);
            onSelectMode();
        });
        addMenu->addAction("Gesture", this, [this, scenePos]() {
            m_currentMode = AddGestureMode;
            createNodeAtPosition(scenePos);
            onSelectMode();
        });
    }
    
    // Map scene position to global screen position for the menu
    QPoint globalPos = m_canvasView->mapToGlobal(
        m_canvasView->mapFromScene(scenePos));
    menu.exec(globalPos);
}

// ============================================================================
// Copy / Paste / Duplicate
// ============================================================================

void KeymapEditorDialog::onCopy()
{
    QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
    m_clipboard = QJsonArray();
    
    for (QGraphicsItem *item : selectedItems) {
        KeyNode *node = dynamic_cast<KeyNode*>(item);
        if (node) {
            m_clipboard.append(node->toJson());
        }
    }
    
    if (!m_clipboard.isEmpty()) {
        updateStatusBar(QString("Copied %1 node(s)").arg(m_clipboard.size()));
    }
}

void KeymapEditorDialog::onPaste()
{
    if (m_clipboard.isEmpty()) {
        updateStatusBar("Clipboard is empty");
        return;
    }
    
    m_undoStack->beginMacro(QString("Paste %1 nodes").arg(m_clipboard.size()));
    
    for (const QJsonValue &val : m_clipboard) {
        QJsonObject obj = val.toObject();
        KeyNode *node = KeyNode::fromJson(obj);
        if (node) {
            // Offset slightly so it doesn't land exactly on top
            QPointF pos = node->relativePosition();
            pos += QPointF(0.02, 0.02);
            pos.setX(qBound(0.0, pos.x(), 1.0));
            pos.setY(qBound(0.0, pos.y(), 1.0));
            node->setRelativePosition(pos);
            node->setDeviceSize(m_deviceSize);
            m_undoStack->push(new AddNodeCommand(this, node));
        }
    }
    
    m_undoStack->endMacro();
    updateStatusBar(QString("Pasted %1 node(s)").arg(m_clipboard.size()));
}

void KeymapEditorDialog::onDuplicate()
{
    QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
    if (selectedItems.isEmpty()) {
        updateStatusBar("No node selected to duplicate");
        return;
    }
    
    int count = 0;
    m_undoStack->beginMacro("Duplicate nodes");
    
    for (QGraphicsItem *item : selectedItems) {
        KeyNode *node = dynamic_cast<KeyNode*>(item);
        if (!node) continue;
        
        QJsonObject json = node->toJson();
        KeyNode *clone = KeyNode::fromJson(json);
        if (clone) {
            QPointF pos = clone->relativePosition();
            pos += QPointF(0.03, 0.03);
            pos.setX(qBound(0.0, pos.x(), 1.0));
            pos.setY(qBound(0.0, pos.y(), 1.0));
            clone->setRelativePosition(pos);
            clone->setDeviceSize(m_deviceSize);
            m_undoStack->push(new AddNodeCommand(this, clone));
            ++count;
        }
    }
    
    m_undoStack->endMacro();
    updateStatusBar(QString("Duplicated %1 node(s)").arg(count));
}

// ============================================================================
// Mouse Move Map Editor
// ============================================================================

void KeymapEditorDialog::onEditMouseMoveMap()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Mouse Move Map Settings");
    dlg.setMinimumWidth(350);
    
    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    
    QCheckBox *enableCheck = new QCheckBox("Enable Mouse Move Map (FPS mouse-look)");
    enableCheck->setChecked(m_hasMouseMoveMap);
    layout->addWidget(enableCheck);
    
    QGroupBox *box = new QGroupBox("Settings");
    QGridLayout *g = new QGridLayout(box);
    
    auto makeSpin = [](double min, double max, double val, int dec) {
        auto *s = new QDoubleSpinBox;
        s->setRange(min, max);
        s->setDecimals(dec);
        s->setSingleStep(0.01);
        s->setValue(val);
        return s;
    };
    
    // Read existing values
    QJsonObject startPos = m_mouseMoveMap["startPos"].toObject();
    double sx = startPos["x"].toDouble(0.5);
    double sy = startPos["y"].toDouble(0.5);
    double srx = m_mouseMoveMap["speedRatioX"].toDouble(1.0);
    double sry = m_mouseMoveMap["speedRatioY"].toDouble(1.0);
    double sr = m_mouseMoveMap["speedRatio"].toDouble(10.0);
    
    g->addWidget(new QLabel("Start X:"), 0, 0);
    auto *startX = makeSpin(0.0, 1.0, sx, 3);
    g->addWidget(startX, 0, 1);
    
    g->addWidget(new QLabel("Start Y:"), 1, 0);
    auto *startY = makeSpin(0.0, 1.0, sy, 3);
    g->addWidget(startY, 1, 1);
    
    g->addWidget(new QLabel("Speed Ratio X:"), 2, 0);
    auto *speedX = makeSpin(0.1, 50.0, srx, 2);
    speedX->setSingleStep(0.25);
    g->addWidget(speedX, 2, 1);
    
    g->addWidget(new QLabel("Speed Ratio Y:"), 3, 0);
    auto *speedY = makeSpin(0.1, 50.0, sry, 2);
    speedY->setSingleStep(0.25);
    g->addWidget(speedY, 3, 1);
    
    g->addWidget(new QLabel("Speed Ratio:"), 4, 0);
    auto *speedRatio = makeSpin(0.1, 50.0, sr, 2);
    speedRatio->setSingleStep(1.0);
    g->addWidget(speedRatio, 4, 1);
    
    // Small eyes settings
    QGroupBox *eyesBox = new QGroupBox("Small Eyes (optional scope mode)");
    QGridLayout *eg = new QGridLayout(eyesBox);
    
    QJsonObject smallEyes = m_mouseMoveMap["smallEyes"].toObject();
    bool hasSmallEyes = !smallEyes.isEmpty();
    QCheckBox *eyesEnable = new QCheckBox("Enable Small Eyes");
    eyesEnable->setChecked(hasSmallEyes);
    eg->addWidget(eyesEnable, 0, 0, 1, 2);
    
    QJsonObject sePos = smallEyes["pos"].toObject();
    eg->addWidget(new QLabel("Pos X:"), 1, 0);
    auto *seX = makeSpin(0.0, 1.0, sePos["x"].toDouble(0.5), 3);
    eg->addWidget(seX, 1, 1);
    eg->addWidget(new QLabel("Pos Y:"), 2, 0);
    auto *seY = makeSpin(0.0, 1.0, sePos["y"].toDouble(0.5), 3);
    eg->addWidget(seY, 2, 1);
    eg->addWidget(new QLabel("Speed Ratio X:"), 3, 0);
    auto *seSrx = makeSpin(0.1, 50.0, smallEyes["speedRatioX"].toDouble(1.0), 2);
    eg->addWidget(seSrx, 3, 1);
    eg->addWidget(new QLabel("Speed Ratio Y:"), 4, 0);
    auto *seSry = makeSpin(0.1, 50.0, smallEyes["speedRatioY"].toDouble(1.0), 2);
    eg->addWidget(seSry, 4, 1);
    
    layout->addWidget(box);
    layout->addWidget(eyesBox);
    
    // Enable/disable based on checkbox
    box->setEnabled(m_hasMouseMoveMap);
    eyesBox->setEnabled(m_hasMouseMoveMap && hasSmallEyes);
    connect(enableCheck, &QCheckBox::toggled, box, &QWidget::setEnabled);
    connect(enableCheck, &QCheckBox::toggled, [eyesBox, eyesEnable](bool en) {
        eyesBox->setEnabled(en && eyesEnable->isChecked());
    });
    connect(eyesEnable, &QCheckBox::toggled, [eyesBox, enableCheck](bool en) {
        eyesBox->setEnabled(enableCheck->isChecked() && en);
    });
    
    QDialogButtonBox *bb = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(bb);
    
    if (dlg.exec() != QDialog::Accepted)
        return;
    
    m_hasMouseMoveMap = enableCheck->isChecked();
    if (m_hasMouseMoveMap) {
        QJsonObject mmm;
        QJsonObject sp;
        sp["x"] = startX->value();
        sp["y"] = startY->value();
        mmm["startPos"] = sp;
        mmm["speedRatioX"] = speedX->value();
        mmm["speedRatioY"] = speedY->value();
        mmm["speedRatio"] = speedRatio->value();
        
        if (eyesEnable->isChecked()) {
            QJsonObject se;
            QJsonObject sePos2;
            sePos2["x"] = seX->value();
            sePos2["y"] = seY->value();
            se["pos"] = sePos2;
            se["speedRatioX"] = seSrx->value();
            se["speedRatioY"] = seSry->value();
            mmm["smallEyes"] = se;
        }
        
        m_mouseMoveMap = mmm;
    } else {
        m_mouseMoveMap = QJsonObject();
    }
    
    m_modified = true;
    updateStatusBar("Mouse move map updated");
}

// ============================================================================
// Switch Key Editor
// ============================================================================

void KeymapEditorDialog::onEditSwitchKey()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Switch Key");
    dlg.setMinimumWidth(300);
    
    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    
    layout->addWidget(new QLabel(
        "The switch key toggles the keymap on/off.\n"
        "Press a key or click a mouse button to set it."));
    
    KeyCaptureEdit *keyEdit = new KeyCaptureEdit;
    keyEdit->setCapturedKeyString(m_switchKey.isEmpty() ? "Key_QuoteLeft" : m_switchKey);
    layout->addWidget(keyEdit);
    
    QLabel *currentLabel = new QLabel(
        QString("Current: %1").arg(m_switchKey.isEmpty() ? "Key_QuoteLeft" : m_switchKey));
    currentLabel->setStyleSheet("color: #888; font-size: 11px;");
    layout->addWidget(currentLabel);
    
    connect(keyEdit, &KeyCaptureEdit::keyCaptured, [currentLabel](const QString &key) {
        currentLabel->setText(QString("Current: %1").arg(key));
    });
    
    QDialogButtonBox *bb = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(bb);
    
    if (dlg.exec() != QDialog::Accepted)
        return;
    
    m_switchKey = keyEdit->capturedKeyString();
    m_modified = true;
    updateStatusBar(QString("Switch key set to: %1").arg(m_switchKey));
}
