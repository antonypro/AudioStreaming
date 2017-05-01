#include "levelwidget.h"

LevelWidget::LevelWidget(QWidget *parent) : QWidget(parent)
{
    setFixedWidth(10);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &LevelWidget::zerolevel);
    m_timer->setInterval(250);

    zerolevel();
}

void LevelWidget::zerolevel()
{
    setlevel(0);
}

void LevelWidget::setlevel(qreal level)
{
    m_timer->stop();
    m_level = level;
    repaint();
    m_timer->start();
}

void LevelWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    QRect currentlevelrect = rect();
    painter.fillRect(currentlevelrect, Qt::white);
    int height = currentlevelrect.height();
    currentlevelrect.adjust(0, height - m_level * height, 0, 0);
    painter.fillRect(currentlevelrect, Qt::red);
}
