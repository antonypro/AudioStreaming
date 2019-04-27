#ifndef BARSWIDGET_H
#define BARSWIDGET_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include "spectrumanalyzer.h"

#define lowfrequency float(0)

#define highfrequency float(20000)

#define barscount 50

class BarsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BarsWidget(QWidget *parent = nullptr);

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

#endif // BARSWIDGET_H
