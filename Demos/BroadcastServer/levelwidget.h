#ifndef LEVELWIDGET_H
#define LEVELWIDGET_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

class LevelWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LevelWidget(QWidget *parent = 0);

public slots:
    void setlevel(qreal level);

private slots:
    void zerolevel();

private:
    void paintEvent(QPaintEvent *event);
    QTimer *m_timer;
    qreal m_level;
};

#endif // LEVELWIDGET_H
