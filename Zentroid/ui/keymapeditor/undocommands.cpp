#include "undocommands.h"
#include "keymapeditor.h"
#include "keynode.h"

// ============================================================================
// AddNodeCommand
// ============================================================================

AddNodeCommand::AddNodeCommand(KeymapEditorDialog *editor, KeyNode *node, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_editor(editor)
    , m_node(node)
    , m_ownsNode(false)
{
    setText(QString("Add %1 node").arg(node->typeString()));
}

AddNodeCommand::~AddNodeCommand()
{
    if (m_ownsNode) {
        delete m_node;
    }
}

void AddNodeCommand::undo()
{
    m_editor->undoRemoveNode(m_node);
    m_ownsNode = true;
}

void AddNodeCommand::redo()
{
    m_editor->undoAddNode(m_node);
    m_ownsNode = false;
}

// ============================================================================
// DeleteNodeCommand
// ============================================================================

DeleteNodeCommand::DeleteNodeCommand(KeymapEditorDialog *editor, KeyNode *node, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_editor(editor)
    , m_node(node)
    , m_ownsNode(false)
{
    setText(QString("Delete %1 node").arg(node->typeString()));
}

DeleteNodeCommand::~DeleteNodeCommand()
{
    if (m_ownsNode) {
        delete m_node;
    }
}

void DeleteNodeCommand::undo()
{
    m_editor->undoAddNode(m_node);
    m_ownsNode = false;
}

void DeleteNodeCommand::redo()
{
    m_editor->undoRemoveNode(m_node);
    m_ownsNode = true;
}

// ============================================================================
// MoveNodeCommand
// ============================================================================

MoveNodeCommand::MoveNodeCommand(KeyNode *node, const QPointF &oldRelPos, const QPointF &newRelPos,
                                 QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_node(node)
    , m_oldRelPos(oldRelPos)
    , m_newRelPos(newRelPos)
{
    setText("Move node");
}

void MoveNodeCommand::undo()
{
    m_node->setRelativePosition(m_oldRelPos);
    // Reposition on canvas
    QSize devSize = m_node->deviceSize();
    QPointF screenPos(m_oldRelPos.x() * devSize.width(), m_oldRelPos.y() * devSize.height());
    screenPos -= QPointF(m_node->rect().width() / 2.0, m_node->rect().height() / 2.0);
    m_node->setPos(screenPos);
    m_node->update();
}

void MoveNodeCommand::redo()
{
    m_node->setRelativePosition(m_newRelPos);
    QSize devSize = m_node->deviceSize();
    QPointF screenPos(m_newRelPos.x() * devSize.width(), m_newRelPos.y() * devSize.height());
    screenPos -= QPointF(m_node->rect().width() / 2.0, m_node->rect().height() / 2.0);
    m_node->setPos(screenPos);
    m_node->update();
}

bool MoveNodeCommand::mergeWith(const QUndoCommand *other)
{
    if (other->id() != id())
        return false;

    auto *moveCmd = static_cast<const MoveNodeCommand*>(other);
    if (moveCmd->m_node != m_node)
        return false;

    m_newRelPos = moveCmd->m_newRelPos;
    return true;
}

// ============================================================================
// EditNodeCommand
// ============================================================================

EditNodeCommand::EditNodeCommand(KeyNode *node, const QJsonObject &oldState,
                                 const QJsonObject &newState, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_node(node)
    , m_oldState(oldState)
    , m_newState(newState)
{
    setText("Edit node");
}

void EditNodeCommand::undo()
{
    applyState(m_oldState);
}

void EditNodeCommand::redo()
{
    applyState(m_newState);
}

void EditNodeCommand::applyState(const QJsonObject &state)
{
    // Apply common fields
    if (state.contains("key"))
        m_node->setKeyCode(state["key"].toString());
    if (state.contains("comment"))
        m_node->setComment(state["comment"].toString());
    if (state.contains("switchMap"))
        m_node->setSwitchMap(state["switchMap"].toBool());

    // Apply type-specific fields
    if (m_node->nodeType() == KeyNode::Drag) {
        auto *dn = static_cast<DragNode*>(m_node);
        if (state.contains("endPos")) {
            QJsonObject ep = state["endPos"].toObject();
            dn->setEndPosition(QPointF(ep["x"].toDouble(), ep["y"].toDouble()));
        }
        if (state.contains("dragSpeed"))
            dn->setDragSpeed(state["dragSpeed"].toDouble());
    } else if (m_node->nodeType() == KeyNode::SteerWheel) {
        auto *sw = static_cast<SteerWheelNode*>(m_node);
        if (state.contains("leftKey"))
            sw->setDirectionKeys(state["leftKey"].toString(), state["rightKey"].toString(),
                                 state["upKey"].toString(), state["downKey"].toString());
        if (state.contains("leftOffset"))
            sw->setOffsets(state["leftOffset"].toDouble(), state["rightOffset"].toDouble(),
                          state["upOffset"].toDouble(), state["downOffset"].toDouble());
    }

    m_node->update();
}
