#ifndef KEEPANDROIDAWAKE_H
#define KEEPANDROIDAWAKE_H

#include <QtCore>
#include <QtAndroidExtras>

class KeepAndroidAwake
{
public:
    KeepAndroidAwake();
    ~KeepAndroidAwake();

private:
    QAndroidJniObject m_wakeLock;
    void keepCpuOn();
    void leaveKeepOn();
};

#endif // KEEPANDROIDAWAKE_H
