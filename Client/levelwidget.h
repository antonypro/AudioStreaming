#ifndef LEVELWIDGET_H
#define LEVELWIDGET_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

class LevelWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit LevelWidget(QWidget *parent = 0);

public slots:
    void setlevel(float size);

private:
    void paintGL();

    float level;
};

#endif // LEVELWIDGET_H
