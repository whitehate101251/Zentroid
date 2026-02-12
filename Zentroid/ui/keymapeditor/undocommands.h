#ifndef UNDOCOMMANDS_H
#define UNDOCOMMANDS_H

#include <QUndoCommand>
#include <QPointF>
#include <QString>
#include <QJsonObject>

class KeyNode;
class KeymapEditorDialog;

/**
 * @brief Command for adding a node to the scene
 */
class AddNodeCommand : public QUndoCommand
{
public:
    AddNodeCommand(KeymapEditorDialog *editor, KeyNode *node, QUndoCommand *parent = nullptr);
    ~AddNodeCommand() override;

    void undo() override;
    void redo() override;

private:
    KeymapEditorDialog *m_editor;
    KeyNode *m_node;
    bool m_ownsNode;  // true when node is not on scene (after undo)
};

/**
 * @brief Command for deleting a node from the scene
 */
class DeleteNodeCommand : public QUndoCommand
{
public:
    DeleteNodeCommand(KeymapEditorDialog *editor, KeyNode *node, QUndoCommand *parent = nullptr);
    ~DeleteNodeCommand() override;

    void undo() override;
    void redo() override;

private:
    KeymapEditorDialog *m_editor;
    KeyNode *m_node;
    bool m_ownsNode;  // true when node is not on scene (after redo/delete)
};

/**
 * @brief Command for moving a node
 */
class MoveNodeCommand : public QUndoCommand
{
public:
    MoveNodeCommand(KeyNode *node, const QPointF &oldRelPos, const QPointF &newRelPos,
                    QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    KeyNode *m_node;
    QPointF m_oldRelPos;
    QPointF m_newRelPos;
};

/**
 * @brief Command for editing node properties
 */
class EditNodeCommand : public QUndoCommand
{
public:
    EditNodeCommand(KeyNode *node, const QJsonObject &oldState, const QJsonObject &newState,
                    QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    void applyState(const QJsonObject &state);

    KeyNode *m_node;
    QJsonObject m_oldState;
    QJsonObject m_newState;
};

#endif // UNDOCOMMANDS_H
