#ifndef LEVELWIDGET_H
#define LEVELWIDGET_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

class LevelWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LevelWidget(QWidget *parent = nullptr);

public slots:
    void setlevel(float level);
    void calculateRMSLevel(const QByteArray &data);

private slots:
    void zerolevel();

private:
    void paintEvent(QPaintEvent *event);
    QTimer *m_timer;
    float m_level;
    int m_rms_size;
    float m_sum_rms;
};

#endif // LEVELWIDGET_H
