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

private slots:
    void zerolevel();

private:
    void paintEvent(QPaintEvent *event);
    QTimer *m_timer;
    float m_level;
};

#endif // LEVELWIDGET_H
