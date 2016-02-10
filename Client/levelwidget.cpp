#include "levelwidget.h"

LevelWidget::LevelWidget(QWidget *parent) : QOpenGLWidget(parent)
{

}

void LevelWidget::setlevel(float size)
{
    level = size;
    repaint();
}

void LevelWidget::paintGL()
{
    QPainter painter(this);
    QRect currentlevelrect = rect();
    painter.fillRect(currentlevelrect, Qt::white);
    int height = currentlevelrect.height();
    currentlevelrect.adjust(0, height - level * height, 0, 0);
    painter.fillRect(currentlevelrect, Qt::red);
}
