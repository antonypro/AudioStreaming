#include "waveformwidget.h"

WaveFormWidget::WaveFormWidget(QWidget *parent) : QWidget(parent)
{
    m_size = 0;

    int maxthread = QThreadPool::globalInstance()->maxThreadCount() + 1;
    QThreadPool::globalInstance()->setMaxThreadCount(maxthread);

    qRegisterMetaType<QVector<QPoint> >("QVector<QPoint>");
    clear();
}

WaveFormWidget::~WaveFormWidget()
{
    m_thread.waitForFinished();

    int maxthread = QThreadPool::globalInstance()->maxThreadCount() - 1;
    QThreadPool::globalInstance()->setMaxThreadCount(maxthread);
}

void WaveFormWidget::start(const QAudioFormat &format)
{
    if (m_initialized)
        return;

    if (!format.isValid())
        return;

    m_initialized = true;
    m_format = format;

    m_size = AudioStreamingLibCore::timeToSize(100, 1, sizeof(float) * 8, m_format.sampleRate());
}

void WaveFormWidget::clear()
{
    m_initialized = false;
    m_point_list.clear();
    repaint();
}

void WaveFormWidget::calculateWaveForm(const QByteArray &data)
{
    if (!m_initialized)
        return;

    if (m_thread.isRunning())
        return;

    m_thread = QtConcurrent::run(this, &WaveFormWidget::calculateWaveFormThread, data, width(), height());
}

void WaveFormWidget::calculateWaveFormThread(const QByteArray &data, int window_width, int window_height)
{
    m_wave_form_buffer.append(data);

    window_height -= 1;
    window_height /= 2;

    while (m_wave_form_buffer.size() >= m_size)
    {
        QByteArray buffer = m_wave_form_buffer.mid(0, m_size);
        m_wave_form_buffer.remove(0, m_size);

        int numSamples = buffer.size() / sizeof(float);

        QVector<float> wave_form_vector;
        wave_form_vector.resize(numSamples);

        const float *samples = (const float*)buffer.constData();

        memcpy(wave_form_vector.data(), samples, buffer.size());

        QVector<QPoint> point_list;

        QPoint initial(0, window_height);
        QPoint final(window_width - 1, window_height);

        int density = numSamples / window_width;

        if (density < 1)
            density = 1;

        for (int i = 0; i < numSamples; i += density)
        {
            float level = wave_form_vector[i];

            for (int j = 1; j < density; j++)
            {
                float sample = wave_form_vector[i + j];

                if (qAbs(sample) > qAbs(level))
                    level = sample;
            }

            QPoint point = QPoint(qRound(i * ((float)(window_width - 1) / numSamples)),
                                  window_height - level * qRound(window_height * 0.75));

            if (point_list.isEmpty() || point.y() == window_height / 2 || point.x() > point_list.last().x() + 2)
                point_list.append(point);
        }

        if (point_list.size() >= 2)
        {
            point_list.first() = initial;
            point_list.last() = final;
        }

        if (!point_list.isEmpty())
            QMetaObject::invokeMethod(this, "prepaint", Qt::QueuedConnection, Q_ARG(QVector<QPoint>, point_list));
    }
}

void WaveFormWidget::prepaint(const QVector<QPoint> &point_list)
{
    m_point_list = point_list;
    repaint();
}

void WaveFormWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    QPen pen;

    pen.setWidth(1);
    pen.setBrush(Qt::green);
    pen.setStyle(Qt::SolidLine);

    painter.setPen(pen);

    for (int i = 0; i < m_point_list.size() - 1; i++)
        painter.drawLine(m_point_list[i], m_point_list[i + 1]);
}
