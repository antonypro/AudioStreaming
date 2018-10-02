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
    m_rms_size = 0;
    m_sum_rms = 0;
}

void LevelWidget::setlevel(float level)
{
    m_timer->stop();
    m_level = level;
    repaint();
    m_timer->start();

    m_rms_size = 0;
    m_sum_rms = 0;
}

void LevelWidget::calculateRMSLevel(const QByteArray &data)
{
    int size = data.size() / int(sizeof(float));

    const float *samples = reinterpret_cast<const float*>(data.constData());

    m_rms_size += size;

    for (int i = 0; i < size; i++)
        m_sum_rms += (samples[i] * samples[i]);
}

void LevelWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    float m_rms_level = 0;

    if (m_rms_size > 0)
        m_rms_level = float(qSqrt((1 / qreal(m_rms_size)) * qreal(m_sum_rms)));

    QPainter painter(this);
    QRect currentlevelrect = rect();
    painter.fillRect(currentlevelrect, Qt::black);
    int height = currentlevelrect.height();
    currentlevelrect.adjust(0, int(height - m_level * height), 0, 0);
    painter.fillRect(currentlevelrect, Qt::green);
    currentlevelrect = rect();
    currentlevelrect.adjust(0, int(height - m_rms_level * height), 0, 0);
    painter.fillRect(currentlevelrect, Qt::red);
}
