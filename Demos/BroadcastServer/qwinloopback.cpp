#include "qwinloopback.h"

QWinLoopback::QWinLoopback(QObject *parent) : QObject(parent)
{
    QLibrary avrtlib("avrt");

    pAvRevertMmThreadCharacteristics = nullptr;
    pAvSetMmThreadCharacteristics = nullptr;

    if (avrtlib.load())
    {
        pAvRevertMmThreadCharacteristics = (tAvRevertMmThreadCharacteristics)avrtlib.resolve("AvRevertMmThreadCharacteristics");
        pAvSetMmThreadCharacteristics = (tAvSetMmThreadCharacteristics)avrtlib.resolve("AvSetMmThreadCharacteristicsW");
    }

    running = false;
}

QWinLoopback::~QWinLoopback()
{
    Stop();
}

bool QWinLoopback::Start()
{
    int maxthread = QThreadPool::globalInstance()->maxThreadCount() + 2;
    QThreadPool::globalInstance()->setMaxThreadCount(maxthread);

    if (!pAvRevertMmThreadCharacteristics)
        return false;

    if (!pAvSetMmThreadCharacteristics)
        return false;

    thread1 = QtConcurrent::run(this, &QWinLoopback::PlaySilence);
    thread2 = QtConcurrent::run(this, &QWinLoopback::LoopbackRecord);

    running = true;

    return true;
}

void QWinLoopback::Stop()
{
    if (!running)
        return;

    running = false;

    sem1.release();

    thread1.waitForFinished();
    thread2.waitForFinished();

    int threadcount = qMax(1, QThreadPool::globalInstance()->maxThreadCount() - 2);
    QThreadPool::globalInstance()->setMaxThreadCount(threadcount);
}

const QAudioFormat QWinLoopback::format()
{
    sem1.acquire();
    return f;
}

HRESULT QWinLoopback::get_default_device(IMMDevice **ppMMDevice) {
    HRESULT hr = S_OK;
    IMMDeviceEnumerator *pMMDeviceEnumerator;

    // activate a device enumerator
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        (void**)&pMMDeviceEnumerator
    );
    if (FAILED(hr)) {
        printf("CoCreateInstance(IMMDeviceEnumerator) failed: hr = 0x%08x\n", (uint)hr);
        return hr;
    }

    // get the default render endpoint
    hr = pMMDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, ppMMDevice);
    pMMDeviceEnumerator->Release();
    if (FAILED(hr)) {
        printf("IMMDeviceEnumerator::GetDefaultAudioEndpoint failed: hr = 0x%08x\n", (uint)hr);
        return hr;
    }

    return S_OK;
}

bool QWinLoopback::LoopbackRecord()
{
    return (bool)LoopbackCaptureThreadFunction();
}

bool QWinLoopback::PlaySilence()
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        printf("CoInitialize failed: hr = 0x%08x\n", (uint)hr);
        return false;
    }

    hr = PlaySilenceThreadFunction();

    CoUninitialize();

    return SUCCEEDED(hr);
}

BOOL QWinLoopback::LoopbackCaptureThreadFunction() {

    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        printf("CoInitialize failed: hr = 0x%08x\n", (uint)hr);
        sem1.release();
        return FALSE;
    }

    IMMDevice *m_pMMDevice = NULL;
    hr = get_default_device(&m_pMMDevice);

    if (FAILED(hr)) {
        sem1.release();
        return FALSE;
    }

    UINT32 nFrames = 0;
    hr = LoopbackCapture(
                m_pMMDevice,
                &nFrames);

    CoUninitialize();

    if (FAILED(hr))
        return FALSE;

    return TRUE;
}

HRESULT QWinLoopback::LoopbackCapture(
        IMMDevice *pMMDevice,
        PUINT32 pnFrames
        ) {

    HRESULT hr;

    // activate an IAudioClient
    IAudioClient *pAudioClient;
    hr = pMMDevice->Activate(
                __uuidof(IAudioClient),
                CLSCTX_ALL, NULL,
                (void**)&pAudioClient
                );
    if (FAILED(hr)) {
        printf("IMMDevice::Activate(IAudioClient) failed: hr = 0x%08x", (uint)hr);
        sem1.release();
        return hr;
    }

    // get the default device periodicity
    REFERENCE_TIME hnsDefaultDevicePeriod;
    hr = pAudioClient->GetDevicePeriod(&hnsDefaultDevicePeriod, NULL);
    if (FAILED(hr)) {
        printf("IAudioClient::GetDevicePeriod failed: hr = 0x%08x\n", (uint)hr);
        pAudioClient->Release();
        pMMDevice->Release();
        sem1.release();
        return hr;
    }

    // get the default device format
    WAVEFORMATEX *pwfx;
    hr = pAudioClient->GetMixFormat(&pwfx);
    if (FAILED(hr)) {
        printf("IAudioClient::GetMixFormat failed: hr = 0x%08x\n", (uint)hr);
        CoTaskMemFree(pwfx);
        pAudioClient->Release();
        pMMDevice->Release();
        sem1.release();
        return hr;
    }

    {
        // coerce float wave format
        // can do this in-place since we're not changing the size of the format
        switch (pwfx->wFormatTag) {
        case WAVE_FORMAT_IEEE_FLOAT:
            pwfx->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
            pwfx->wBitsPerSample = 32;
            pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
            pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
            break;

        case WAVE_FORMAT_EXTENSIBLE:
        {
            // naked scope for case-local variable
            PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pwfx);
            if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, pEx->SubFormat)) {
                pEx->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
                pEx->Samples.wValidBitsPerSample = 32;
                pwfx->wBitsPerSample = 32;
                pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
                pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
            } else {
                printf("Don't know how to coerce mix format to float\n");
                CoTaskMemFree(pwfx);
                pAudioClient->Release();
                sem1.release();
                return E_UNEXPECTED;
            }
        }
            break;

        default:
            printf("Don't know how to coerce WAVEFORMATEX with wFormatTag = 0x%08x to float\n", pwfx->wFormatTag);
            CoTaskMemFree(pwfx);
            pAudioClient->Release();
            pMMDevice->Release();
            sem1.release();
            return E_UNEXPECTED;
        }
    }

    {
        QAudioFormat format;
        format.setCodec("audio/pcm");
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setSampleType(QAudioFormat::Float);
        format.setSampleRate(pwfx->nSamplesPerSec);
        format.setChannelCount(pwfx->nChannels);
        format.setSampleSize(pwfx->wBitsPerSample);

        f = format;
        sem1.release();
    }

    // create a periodic waitable timer
    HANDLE hWakeUp = CreateWaitableTimer(NULL, FALSE, NULL);
    if (NULL == hWakeUp) {
        DWORD dwErr = GetLastError();
        printf("CreateWaitableTimer failed: last error = %u\n", (uint)dwErr);
        CoTaskMemFree(pwfx);
        pAudioClient->Release();
        return HRESULT_FROM_WIN32(dwErr);
    }

    UINT32 nBlockAlign = pwfx->nBlockAlign;
    *pnFrames = 0;

    // call IAudioClient::Initialize
    // note that AUDCLNT_STREAMFLAGS_LOOPBACK and AUDCLNT_STREAMFLAGS_EVENTCALLBACK
    // do not work together...
    // the "data ready" event never gets set
    // so we're going to do a timer-driven loop
    hr = pAudioClient->Initialize(
                AUDCLNT_SHAREMODE_SHARED,
                AUDCLNT_STREAMFLAGS_LOOPBACK,
                0, 0, pwfx, 0
                );
    if (FAILED(hr)) {
        printf("IAudioClient::Initialize failed: hr = 0x%08x\n", (uint)hr);
        CloseHandle(hWakeUp);
        pAudioClient->Release();
        pMMDevice->Release();
        return hr;
    }
    CoTaskMemFree(pwfx);

    // activate an IAudioCaptureClient
    IAudioCaptureClient *pAudioCaptureClient;
    hr = pAudioClient->GetService(
                __uuidof(IAudioCaptureClient),
                (void**)&pAudioCaptureClient
                );
    if (FAILED(hr)) {
        printf("IAudioClient::GetService(IAudioCaptureClient) failed: hr 0x%08x\n", (uint)hr);
        CloseHandle(hWakeUp);
        pAudioClient->Release();
        pMMDevice->Release();
        return hr;
    }

    // register with MMCSS
    DWORD nTaskIndex = 0;
    HANDLE hTask = pAvSetMmThreadCharacteristics(L"Capture", &nTaskIndex);
    if (NULL == hTask) {
        DWORD dwErr = GetLastError();
        printf("AvSetMmThreadCharacteristics failed: last error = %u\n", (uint)dwErr);
        pAudioClient->Stop();
        CancelWaitableTimer(hWakeUp);
        pAvRevertMmThreadCharacteristics(hTask);
        pAudioCaptureClient->Release();
        CloseHandle(hWakeUp);
        pAudioClient->Release();
        pMMDevice->Release();
        return HRESULT_FROM_WIN32(dwErr);
    }

    // set the waitable timer
    LARGE_INTEGER liFirstFire;
    liFirstFire.QuadPart = -hnsDefaultDevicePeriod / 2; // negative means relative time
    LONG lTimeBetweenFires = (LONG)hnsDefaultDevicePeriod / 2 / (10 * 1000); // convert to milliseconds
    BOOL bOK = SetWaitableTimer(
                hWakeUp,
                &liFirstFire,
                lTimeBetweenFires,
                NULL, NULL, FALSE
                );
    if (!bOK) {
        DWORD dwErr = GetLastError();
        printf("SetWaitableTimer failed: last error = %u\n", (uint)dwErr);
        pAudioClient->Stop();
        CancelWaitableTimer(hWakeUp);
        pAvRevertMmThreadCharacteristics(hTask);
        pAudioCaptureClient->Release();
        CloseHandle(hWakeUp);
        pAudioClient->Release();
        pMMDevice->Release();
        return HRESULT_FROM_WIN32(dwErr);
    }

    // call IAudioClient::Start
    hr = pAudioClient->Start();
    if (FAILED(hr)) {
        printf("IAudioClient::Start failed: hr = 0x%08x\n", (uint)hr);
        pAudioClient->Stop();
        CancelWaitableTimer(hWakeUp);
        pAvRevertMmThreadCharacteristics(hTask);
        pAudioCaptureClient->Release();
        CloseHandle(hWakeUp);
        pAudioClient->Release();
        pMMDevice->Release();
        return hr;
    }


    // loopback capture loop
    bool bDone = false;
    while (!bDone && running) {

        // drain data while it is available
        UINT32 nNextPacketSize;
        for (
             hr = pAudioCaptureClient->GetNextPacketSize(&nNextPacketSize);
             SUCCEEDED(hr) && nNextPacketSize > 0;
             hr = pAudioCaptureClient->GetNextPacketSize(&nNextPacketSize)
             ) {
            // get the captured data
            BYTE *pData;
            UINT32 nNumFramesToRead;
            DWORD dwFlags;

            hr = pAudioCaptureClient->GetBuffer(
                        &pData,
                        &nNumFramesToRead,
                        &dwFlags,
                        NULL,
                        NULL
                        );
            if (FAILED(hr)) {
                printf("IAudioCaptureClient::GetBuffer failed after %u frames: hr = 0x%08x\n", *pnFrames, (uint)hr);
                pAudioClient->Stop();
                CancelWaitableTimer(hWakeUp);
                pAvRevertMmThreadCharacteristics(hTask);
                pAudioCaptureClient->Release();
                CloseHandle(hWakeUp);
                pAudioClient->Release();
                pMMDevice->Release();
                return hr;
            }

            if (0 == nNumFramesToRead) {
                printf("IAudioCaptureClient::GetBuffer said to read 0 frames after %u frames\n", *pnFrames);
                pAudioClient->Stop();
                CancelWaitableTimer(hWakeUp);
                pAvRevertMmThreadCharacteristics(hTask);
                pAudioCaptureClient->Release();
                CloseHandle(hWakeUp);
                pAudioClient->Release();
                pMMDevice->Release();
                return E_UNEXPECTED;
            }

            LONG lBytesToWrite = nNumFramesToRead * nBlockAlign;
            LONG lBytesWritten;

            {
                QByteArray data;
                data.resize(lBytesToWrite);
                memcpy(data.data(), pData, data.size());
                emit readyRead(data);
                lBytesWritten = data.size();
            }

            if (lBytesToWrite != lBytesWritten) {
                printf("mmioWrite wrote %u bytes after %u frames: expected %u bytes\n", (uint)lBytesWritten, *pnFrames, (uint)lBytesToWrite);
                pAudioClient->Stop();
                CancelWaitableTimer(hWakeUp);
                pAvRevertMmThreadCharacteristics(hTask);
                pAudioCaptureClient->Release();
                CloseHandle(hWakeUp);
                pAudioClient->Release();
                pMMDevice->Release();
                return E_UNEXPECTED;
            }
            *pnFrames += nNumFramesToRead;

            hr = pAudioCaptureClient->ReleaseBuffer(nNumFramesToRead);
            if (FAILED(hr)) {
                printf("IAudioCaptureClient::ReleaseBuffer failed after %u frames: hr = 0x%08x\n", *pnFrames, (uint)hr);
                pAudioClient->Stop();
                CancelWaitableTimer(hWakeUp);
                pAvRevertMmThreadCharacteristics(hTask);
                pAudioCaptureClient->Release();
                CloseHandle(hWakeUp);
                pAudioClient->Release();
                pMMDevice->Release();
                return hr;
            }
        }

        if (FAILED(hr)) {
            printf("IAudioCaptureClient::GetNextPacketSize failed after %u frames: hr = 0x%08x\n", *pnFrames, (uint)hr);
            pAudioClient->Stop();
            CancelWaitableTimer(hWakeUp);
            pAvRevertMmThreadCharacteristics(hTask);
            pAudioCaptureClient->Release();
            CloseHandle(hWakeUp);
            pAudioClient->Release();
            pMMDevice->Release();
            return hr;
        }

        WaitForSingleObject(hWakeUp, INFINITE);

    } // capture loop

    if (FAILED(hr)) {
        // FinishWaveFile does it's own logging
        pAudioClient->Stop();
        CancelWaitableTimer(hWakeUp);
        pAvRevertMmThreadCharacteristics(hTask);
        pAudioCaptureClient->Release();
        CloseHandle(hWakeUp);
        pAudioClient->Release();
        pMMDevice->Release();
        return hr;
    }

    pAudioClient->Stop();
    CancelWaitableTimer(hWakeUp);
    pAvRevertMmThreadCharacteristics(hTask);
    pAudioCaptureClient->Release();
    CloseHandle(hWakeUp);
    pAudioClient->Release();
    pMMDevice->Release();

    return hr;
}

HRESULT QWinLoopback::PlaySilenceThreadFunction() {

    HRESULT hr;

    IMMDevice *pMMDevice = NULL;
    hr = get_default_device(&pMMDevice);

    if (FAILED(hr))
        return hr;

    // activate an IAudioClient
    IAudioClient *pAudioClient;
    hr = pMMDevice->Activate(
        __uuidof(IAudioClient),
        CLSCTX_ALL, NULL,
        (void**)&pAudioClient
    );
    if (FAILED(hr)) {
        printf("IMMDevice::Activate(IAudioClient) failed: hr = 0x%08x", (uint)hr);
        pMMDevice->Release();
        return hr;
    }

    // get the mix format
    WAVEFORMATEX *pwfx;
    hr = pAudioClient->GetMixFormat(&pwfx);
    if (FAILED(hr)) {
        printf("IAudioClient::GetMixFormat failed: hr = 0x%08x\n", (uint)hr);
        pAudioClient->Release();
        pMMDevice->Release();
        return hr;
    }

    // initialize the audio client
    hr = pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        0, 0, pwfx, NULL
    );
    CoTaskMemFree(pwfx);
    if (FAILED(hr)) {
        printf("IAudioClient::Initialize failed: hr 0x%08x\n", (uint)hr);
        pAudioClient->Release();
        pMMDevice->Release();
        return hr;
    }

    // get the buffer size
    UINT32 nFramesInBuffer;
    hr = pAudioClient->GetBufferSize(&nFramesInBuffer);
    if (FAILED(hr)) {
        printf("IAudioClient::GetBufferSize failed: hr 0x%08x\n", (uint)hr);
        pAudioClient->Release();
        pMMDevice->Release();
        return hr;
    }

    // get an IAudioRenderClient
    IAudioRenderClient *pAudioRenderClient;
    hr = pAudioClient->GetService(
        __uuidof(IAudioRenderClient),
        (void**)&pAudioRenderClient
    );
    if (FAILED(hr)) {
        printf("IAudioClient::GetService(IAudioRenderClient) failed: hr 0x%08x\n", (uint)hr);
        pAudioClient->Release();
        pMMDevice->Release();
        return hr;
    }

    // create a "feed me" event
    HANDLE hFeedMe;
    hFeedMe = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == hFeedMe) {
        DWORD dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
        printf("CreateEvent failed: last error is %u\n", (uint)dwErr);
        pAudioRenderClient->Release();
        pAudioClient->Release();
        return hr;
    }

    // set it as the event handle
    hr = pAudioClient->SetEventHandle(hFeedMe);
    if (FAILED(hr)) {
        printf("IAudioClient::SetEventHandle failed: hr = 0x%08x\n", (uint)hr);
        pAudioRenderClient->Release();
        pAudioClient->Release();
        return hr;
    }

    // pre-fill a single buffer of silence
    BYTE *pData;
    hr = pAudioRenderClient->GetBuffer(nFramesInBuffer, &pData);
    if (FAILED(hr)) {
        printf("IAudioRenderClient::GetBuffer failed on pre-fill: hr = 0x%08x\n", (uint)hr);
        pAudioClient->Stop();
        printf("TODO: unregister with MMCSS\n");
        CloseHandle(hFeedMe);
        pAudioRenderClient->Release();
        pAudioClient->Release();
        pMMDevice->Release();
        return hr;
    }

    // release the buffer with the silence flag
    hr = pAudioRenderClient->ReleaseBuffer(nFramesInBuffer, AUDCLNT_BUFFERFLAGS_SILENT);
    if (FAILED(hr)) {
        printf("IAudioRenderClient::ReleaseBuffer failed on pre-fill: hr = 0x%08x\n", (uint)hr);
        pAudioClient->Stop();
        printf("TODO: unregister with MMCSS\n");
        CloseHandle(hFeedMe);
        pAudioRenderClient->Release();
        pAudioClient->Release();
        return hr;
    }

    // register with MMCSS
    DWORD nTaskIndex = 0;
    HANDLE hTask = pAvSetMmThreadCharacteristics(L"Playback", &nTaskIndex);
    if (NULL == hTask) {
        DWORD dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
        printf("AvSetMmThreadCharacteristics failed: last error = %u\n", (uint)dwErr);
        pAudioRenderClient->Release();
        pAudioClient->Release();
        return hr;
    }

    // call Start
    hr = pAudioClient->Start();
    if (FAILED(hr)) {
        printf("IAudioClient::Start failed: hr = 0x%08x\n", (uint)hr);
        printf("TODO: unregister with MMCSS\n");
        pAvRevertMmThreadCharacteristics(hTask);
        pAudioRenderClient->Release();
        pAudioClient->Release();
        return hr;
    }

    bool bDone = false;
    while (!bDone && running) {
        WaitForSingleObject(hFeedMe, INFINITE);

        // got "feed me" event - see how much padding there is
        //
        // padding is how much of the buffer is currently in use
        //
        // note in particular that event-driven (pull-mode) render should not
        // call GetCurrentPadding multiple times
        // in a single processing pass
        // this is in stark contrast to timer-driven (push-mode) render
        UINT32 nFramesOfPadding;
        hr = pAudioClient->GetCurrentPadding(&nFramesOfPadding);
        if (FAILED(hr)) {
            printf("IAudioClient::GetCurrentPadding failed: hr = 0x%08x\n", (uint)hr);
            pAudioClient->Stop();
            pAvRevertMmThreadCharacteristics(hTask);
            CloseHandle(hFeedMe);
            pAudioRenderClient->Release();
            pAudioClient->Release();
            return hr;
        }

        hr = pAudioRenderClient->GetBuffer(nFramesInBuffer - nFramesOfPadding, &pData);
        if (FAILED(hr)) {
            printf("IAudioRenderClient::GetBuffer failed: hr = 0x%08x - glitch?\n", (uint)hr);
            pAudioClient->Stop();
            pAvRevertMmThreadCharacteristics(hTask);
            CloseHandle(hFeedMe);
            pAudioRenderClient->Release();
            pAudioClient->Release();
            return hr;
        }

        // *** AT THIS POINT ***
        // If you wanted to render something besides silence,
        // you would fill the buffer pData
        // with (nFramesInBuffer - nFramesOfPadding) worth of audio data
        // this should be in the same wave format
        // that the stream was initialized with
        //
        // In particular, if you didn't want to use the mix format,
        // you would need to either ask for a different format in IAudioClient::Initialize
        // or do a format conversion
        //
        // If you do, then change the AUDCLNT_BUFFERFLAGS_SILENT flags value below to 0

        // release the buffer with the silence flag
        hr = pAudioRenderClient->ReleaseBuffer(nFramesInBuffer - nFramesOfPadding, AUDCLNT_BUFFERFLAGS_SILENT);
        if (FAILED(hr)) {
            printf("IAudioRenderClient::ReleaseBuffer failed: hr = 0x%08x - glitch?\n", (uint)hr);
            pAudioClient->Stop();
            pAvRevertMmThreadCharacteristics(hTask);
            CloseHandle(hFeedMe);
            pAudioRenderClient->Release();
            pAudioClient->Release();
            return hr;
        }
    } // for each pass

    pAudioClient->Stop();
    pAvRevertMmThreadCharacteristics(hTask);
    CloseHandle(hFeedMe);
    pAudioRenderClient->Release();
    pAudioClient->Release();
    return S_OK;
}
