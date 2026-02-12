#ifndef MAGNETICWIDGET_H
#define MAGNETICWIDGET_H

#include <QPointer>
#include <QWidget>

/*
 * a magnetic widget
 * window title bar support not good
*/

class MagneticWidget : public QWidget
{
    Q_OBJECT

public:
    enum AdsorbPosition
    {
        AP_OUTSIDE_LEFT = 0x01,   // snap to outside left edge
        AP_OUTSIDE_TOP = 0x02,    // snap to outside top edge
        AP_OUTSIDE_RIGHT = 0x04,  // snap to outside right edge
        AP_OUTSIDE_BOTTOM = 0x08, // snap to outside bottom edge
        AP_INSIDE_LEFT = 0x10,    // snap to inside left edge
        AP_INSIDE_TOP = 0x20,     // snap to inside top edge
        AP_INSIDE_RIGHT = 0x40,   // snap to inside right edge
        AP_INSIDE_BOTTOM = 0x80,  // snap to inside bottom edge
        AP_ALL = 0xFF,            // snap to all edges
    };
    Q_DECLARE_FLAGS(AdsorbPositions, AdsorbPosition)

public:
    explicit MagneticWidget(QWidget *adsorbWidget, AdsorbPositions adsorbPos = AP_ALL);
    ~MagneticWidget();

    bool isAdsorbed();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void moveEvent(QMoveEvent *event) override;

private:
    void getGeometry(QRect &relativeWidgetRect, QRect &targetWidgetRect);

private:
    AdsorbPositions m_adsorbPos = AP_ALL;
    QPoint m_relativePos;
    bool m_adsorbed = false;
    QPointer<QWidget> m_adsorbWidget;
    // store adsorbWidgetSize separately because when Widget setGeometry is called,
    // the Move event is received before the Resize event,
    // but the Widget's size() already reflects the size specified by setGeometry when the Move event is received
    QSize m_adsorbWidgetSize;
    AdsorbPosition m_curAdsorbPosition;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MagneticWidget::AdsorbPositions)
#endif // MAGNETICWIDGET_H
