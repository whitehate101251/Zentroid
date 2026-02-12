#include "toggleswitch.h"
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>

ToggleSwitch::ToggleSwitch(QWidget *parent)
    : QAbstractButton(parent)
    , m_offset(0)
    , m_margin(3)
    , m_hovered(false)
{
    setCheckable(true);
    setFixedSize(52, 28);

    m_animation = new QPropertyAnimation(this, "offset", this);
    m_animation->setDuration(200);
    m_animation->setEasingCurve(QEasingCurve::InOutCubic);

    connect(this, &QAbstractButton::toggled, this, [this](bool checked) {
        Q_UNUSED(checked);
        updateOffset();
    });

    m_offset = m_margin;
}

QSize ToggleSwitch::sizeHint() const
{
    return QSize(52, 28);
}

void ToggleSwitch::updateOffset()
{
    int thumbSize = height() - 2 * m_margin;
    int target = isChecked() ? (width() - m_margin - thumbSize) : m_margin;
    m_animation->stop();
    m_animation->setStartValue(m_offset);
    m_animation->setEndValue(target);
    m_animation->start();
}

void ToggleSwitch::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    qreal radius = height() / 2.0;
    qreal thumbDiameter = height() - 2.0 * m_margin;

    // Track shadow (subtle depth)
    QPainterPath shadowPath;
    shadowPath.addRoundedRect(QRectF(0, 1.0, width(), height()), radius, radius);
    p.fillPath(shadowPath, QColor(0, 0, 0, 25));

    // Track fill
    QColor trackColor = isChecked() ? QColor(0, 187, 158) : QColor(68, 68, 68);
    if (m_hovered) {
        trackColor = isChecked() ? QColor(0, 212, 177) : QColor(82, 82, 82);
    }
    QPainterPath trackPath;
    trackPath.addRoundedRect(QRectF(0, 0, width(), height()), radius, radius);
    p.fillPath(trackPath, trackColor);

    // Track inner border
    p.setPen(QPen(QColor(0, 0, 0, 35), 0.8));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(0.5, 0.5, width() - 1, height() - 1), radius - 0.5, radius - 0.5);

    // Subtle ON glow
    if (isChecked()) {
        p.setPen(QPen(QColor(0, 187, 158, 60), 1.5));
        p.drawRoundedRect(QRectF(-0.5, -0.5, width() + 1, height() + 1), radius + 0.5, radius + 0.5);
    }

    // Thumb drop shadow
    qreal tx = m_offset;
    qreal ty = m_margin;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 45));
    p.drawEllipse(QRectF(tx + 0.5, ty + 1.5, thumbDiameter, thumbDiameter));

    // Thumb body with gradient
    QRadialGradient thumbGrad(tx + thumbDiameter / 2.0, ty + thumbDiameter / 2.0, thumbDiameter / 2.0);
    thumbGrad.setColorAt(0.0, QColor(255, 255, 255));
    thumbGrad.setColorAt(1.0, QColor(238, 238, 238));
    p.setBrush(thumbGrad);
    p.drawEllipse(QRectF(tx, ty, thumbDiameter, thumbDiameter));

    // Thumb top highlight (glass effect)
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(255, 255, 255, 90), 0.6));
    p.drawEllipse(QRectF(tx + 1.5, ty + 0.8, thumbDiameter - 3, thumbDiameter - 3));
}

void ToggleSwitch::mouseReleaseEvent(QMouseEvent *e)
{
    QAbstractButton::mouseReleaseEvent(e);
}

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
void ToggleSwitch::enterEvent(QEnterEvent *)
#else
void ToggleSwitch::enterEvent(QEvent *)
#endif
{
    m_hovered = true;
    setCursor(Qt::PointingHandCursor);
    update();
}

void ToggleSwitch::leaveEvent(QEvent *)
{
    m_hovered = false;
    unsetCursor();
    update();
}

void ToggleSwitch::resizeEvent(QResizeEvent *e)
{
    QAbstractButton::resizeEvent(e);
    // Snap offset to correct position without animation on resize
    int thumbSize = height() - 2 * m_margin;
    m_offset = isChecked() ? (width() - m_margin - thumbSize) : m_margin;
}
