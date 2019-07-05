#ifndef THREADCOMMON_H
#define THREADCOMMON_H

#include <QtCore>

extern int m_worker_count;
extern QMutex m_worker_mutex;
extern QSemaphore m_worker_semaphore;

#define START_THREAD \
{\
    QMutexLocker locker(&m_worker_mutex);\
    Q_UNUSED(locker)\
    \
    setParent(nullptr);\
    \
    QThread *new_thread = new QThread();\
    moveToThread(new_thread);\
    \
    connect(parent, &QObject::destroyed, this, [=]{stop();});\
    \
    m_worker_count++;\
    \
    new_thread->start();\
}

#define STOP_THREAD \
{\
    QMutexLocker locker(&m_worker_mutex);\
    Q_UNUSED(locker)\
    \
    thread()->quit();\
    \
    bool empty = (--m_worker_count == 0);\
    \
    if (empty)\
        m_worker_semaphore.release();\
}

#define CLASS_MEMBER_HEADER \
public slots:\
    void stop();\
\
private slots:\
    void stopPrivate();\

#endif // THREADCOMMON_H
