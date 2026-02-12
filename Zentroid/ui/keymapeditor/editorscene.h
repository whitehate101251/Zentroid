#ifndef EDITORSCENE_H
#define EDITORSCENE_H

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneContextMenuEvent>
#include "keynode.h"

/**
 * @brief Custom QGraphicsScene that emits click/double-click signals.
 *
 * Used by KeymapEditorDialog to detect canvas clicks for adding nodes
 * and double-clicks for editing existing nodes.
 */
class EditorScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit EditorScene(QObject *parent = nullptr)
        : QGraphicsScene(parent) {}

    EditorScene(qreal x, qreal y, qreal w, qreal h, QObject *parent = nullptr)
        : QGraphicsScene(x, y, w, h, parent) {}

signals:
    /** Emitted when the user clicks on empty space (no item hit). */
    void canvasClicked(const QPointF &scenePos);

    /** Emitted when the user double-clicks an item. */
    void itemDoubleClicked(QGraphicsItem *item, const QPointF &scenePos);

    /** Emitted when the mouse moves over the canvas. */
    void mouseMoved(const QPointF &scenePos);

    /** Emitted when a node drag operation finishes (mouse released after drag). */
    void nodeDragFinished(KeyNode *node, const QPointF &oldRelPos, const QPointF &newRelPos);

    /** Emitted on right-click for context menu. */
    void contextMenuRequested(const QPointF &scenePos, QGraphicsItem *item);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        // Let the base class handle item selection & dragging first
        QGraphicsScene::mousePressEvent(event);

        // If no item was grabbed (click on empty space) emit signal
        if (event->button() == Qt::LeftButton && !mouseGrabberItem()) {
            emit canvasClicked(event->scenePos());
        }
    }

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override
    {
        QGraphicsScene::mouseDoubleClickEvent(event);

        if (event->button() == Qt::LeftButton) {
            QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
            if (item) {
                emit itemDoubleClicked(item, event->scenePos());
            }
        }
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override
    {
        QGraphicsScene::mouseMoveEvent(event);
        emit mouseMoved(event->scenePos());
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        // Check if an item was being dragged before the base class processes release
        QGraphicsItem *grabber = mouseGrabberItem();
        
        QGraphicsScene::mouseReleaseEvent(event);
        
        if (event->button() == Qt::LeftButton && grabber) {
            KeyNode *node = dynamic_cast<KeyNode*>(grabber);
            if (node) {
                QPointF oldRel = node->dragStartRelativePos();
                QPointF newRel = node->relativePosition();
                if (oldRel != newRel) {
                    emit nodeDragFinished(node, oldRel, newRel);
                }
            }
        }
    }

    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override
    {
        QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
        emit contextMenuRequested(event->scenePos(), item);
        event->accept();
    }
};

#endif // EDITORSCENE_H
