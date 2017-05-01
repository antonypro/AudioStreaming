#ifndef BARS_H
#define BARS_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include "spectrumanalyzer.h"

#define lowfrequency float(0.0)

#define highfrequency float(1000.0)

#define barscount 10

class Bars : public QWidget
{
    Q_OBJECT
public:
    explicit Bars(QWidget *parent = 0);

signals:

public slots:
    void setValues(const QVector<SpectrumStruct> &values);
    void clear();

private slots:
    void calculate();

private:
    void paintEvent(QPaintEvent *event);

    QVector<float> m_bars;
    QVector<SpectrumStruct> m_values;
    QTimer *m_timer_clear;
};

#endif // BARS_H
