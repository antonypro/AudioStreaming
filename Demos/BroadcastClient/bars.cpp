#include "bars.h"

Bars::Bars(QWidget *parent) : QWidget(parent)
{
    m_bars.resize(barscount);

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, static_cast<void (Bars::*)()>(&Bars::repaint));
    timer->start(50);

    m_timer_clear = new QTimer(this);
    connect(m_timer_clear, &QTimer::timeout, this, &Bars::clear);
    m_timer_clear->setInterval(250);

    m_timer_clear->start();
}

void Bars::setValues(const QVector<SpectrumStruct> &values)
{
    m_values = values;

    calculate();

    m_timer_clear->stop();
    m_timer_clear->start();
}

void Bars::calculate()
{
    for (int i = 0; i < m_values.size(); i++)
    {
        SpectrumStruct value = m_values[i];

        if (value.frequency < lowfrequency || value.frequency >= highfrequency)
            continue;

        const float bandWidth = (highfrequency - lowfrequency) / barscount;
        const int index = (value.frequency - lowfrequency) / bandWidth;

        m_bars[index] = qMax(value.amplitude, m_bars[index]);
    }
}

void Bars::clear()
{
    QVector<SpectrumStruct> values;

    SpectrumStruct value;
    value.frequency = 0.0;
    value.amplitude = 0.0;

    values.append(value);

    setValues(values);

    m_bars.fill(0.0);
}

void Bars::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    calculate();

    float w = width() / float(barscount);

    float h = height();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(rect(), Qt::black);

    for (int i = 0; i < barscount; i++)
    {
        float value = m_bars[i];
        painter.fillRect(i * w + 5, h - (value * h), w - 10, value * h, Qt::red);
    }

    m_bars.fill(0.0);
}
