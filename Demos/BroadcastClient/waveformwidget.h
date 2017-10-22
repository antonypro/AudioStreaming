#ifndef WAVEFORMWIDGET_H
#define WAVEFORMWIDGET_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <QtMultimedia>
#include <QtConcurrent>
#include <AudioStreamingLibCore>

class WaveFormWidget : public QWidget
{
    Q_OBJECT
public:
    explicit WaveFormWidget(QWidget *parent = nullptr);
    ~WaveFormWidget();

public slots:
    void start(const QAudioFormat &format);
    void clear();
    void calculateWaveForm(const QByteArray &data);

private slots:
    void prepaint(const QVector<QPoint> &point_list);

private:
    void calculateWaveFormThread(const QByteArray &data, int window_width, int window_height);
    void paintEvent(QPaintEvent *event);

    bool m_initialized;
    QFuture<void> m_thread;
    QAudioFormat m_format;
    QVector<QPoint> m_point_list;
    QByteArray m_wave_form_buffer;
    int m_size;
};

#endif // WAVEFORMWIDGET_H
