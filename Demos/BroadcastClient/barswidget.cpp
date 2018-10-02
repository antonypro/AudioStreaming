#include "barswidget.h"

BarsWidget::BarsWidget(QWidget *parent) : QWidget(parent)
{
    m_bars.resize(barscount);

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, static_cast<void (BarsWidget::*)()>(&BarsWidget::repaint));
    timer->start(50);

    m_timer_clear = new QTimer(this);
    connect(m_timer_clear, &QTimer::timeout, this, &BarsWidget::clear);
    m_timer_clear->setInterval(250);

    m_timer_clear->start();
}

void BarsWidget::setValues(const QVector<SpectrumStruct> &values)
{
    m_values = values;

    calculate();

    m_timer_clear->stop();
    m_timer_clear->start();
}

void BarsWidget::calculate()
{
    for (int i = 0; i < m_values.size(); i++)
    {
        SpectrumStruct value = m_values[i];

        if (value.frequency < lowfrequency || value.frequency >= highfrequency)
            continue;

        const float bandWidth = (highfrequency - lowfrequency) / barscount;
        const int index = int((value.frequency - lowfrequency) / bandWidth);

        m_bars[index] = qMax(value.amplitude, m_bars[index]);
    }
}

void BarsWidget::clear()
{
    QVector<SpectrumStruct> values;

    SpectrumStruct value;
    value.frequency = 0;
    value.amplitude = 0;

    values.append(value);

    setValues(values);

    m_bars.fill(0);
}

void BarsWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    calculate();

    float w = float(qFloor(width() / qreal(barscount) - 1));

    float h = height();

    QPainter painter(this);

    painter.fillRect(rect(), Qt::black);

    int pos = int(width() - ((w + 1) * barscount)) / 2 + 1;

    for (int i = 0; i < barscount; i++)
    {
        float value = m_bars[i];
        painter.fillRect(pos, int(h - (value * h)), int(w - 2), int(value * h), Qt::yellow);
        pos += w + 1;
    }

    m_bars.fill(0);
}
