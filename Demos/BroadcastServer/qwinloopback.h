#ifndef QWINLOOPBACK_H
#define QWINLOOPBACK_H

#include <QtCore>
#include <QtConcurrent>
#include <QtMultimedia>
#include <windows.h>
#ifndef interface
#define interface __STRUCT__
#endif
#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <stdio.h>
#include <avrt.h>

class QWinLoopback : public QObject
{
    Q_OBJECT
public:
    explicit QWinLoopback(QObject *parent = nullptr);
    ~QWinLoopback();

signals:
    void readyRead(QByteArray);

public slots:
    bool start();
    const QAudioFormat format();

private slots:
    void stop();

private:
    bool m_running;

    QSemaphore sem1;

    typedef BOOL(STDAPICALLTYPE *tAvRevertMmThreadCharacteristics)(HANDLE);
    tAvRevertMmThreadCharacteristics pAvRevertMmThreadCharacteristics;

    typedef HANDLE(STDAPICALLTYPE *tAvSetMmThreadCharacteristics)(LPCTSTR, LPDWORD);
    tAvSetMmThreadCharacteristics pAvSetMmThreadCharacteristics;

    HRESULT get_default_device(IMMDevice **ppMMDevice);
    bool LoopbackRecord();
    bool PlaySilence();
    BOOL LoopbackCaptureThreadFunction();
    HRESULT LoopbackCapture(IMMDevice *pMMDevice, PUINT32 pnFrames);
    HRESULT PlaySilenceThreadFunction();

    QFuture<bool> thread1;
    QFuture<bool> thread2;
    QAudioFormat f;
};

#endif // QWINLOOPBACK_H
