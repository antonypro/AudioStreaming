#include "keepandroidawake.h"

KeepAndroidAwake::KeepAndroidAwake()
{
    keepCpuOn();
}

KeepAndroidAwake::~KeepAndroidAwake()
{
    leaveKeepOn();
}

void KeepAndroidAwake::keepCpuOn()
{
    QAndroidJniObject activity = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;");
    if (activity.isValid())
    {
        QAndroidJniObject serviceName = QAndroidJniObject::getStaticObjectField<jstring>("android/content/Context", "POWER_SERVICE");
        if (serviceName.isValid())
        {
            QAndroidJniObject powerMgr = activity.callObjectMethod("getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;", serviceName.object<jobject>());
            if (powerMgr.isValid())
            {
                jint levelAndFlags = QAndroidJniObject::getStaticField<jint>("android/os/PowerManager", "PARTIAL_WAKE_LOCK");

                QAndroidJniObject tag = QAndroidJniObject::fromString("Wake Lock");

                m_wakeLock = powerMgr.callObjectMethod("WakeLock",
                                                       "(ILjava/lang/String;)Landroid/os/PowerManager$WakeLock;",
                                                       levelAndFlags, tag.object<jstring>());
            }
        }
    }

    if (m_wakeLock.isValid())
    {
        m_wakeLock.callMethod<void>("acquire", "()V");
    }
}

void KeepAndroidAwake::leaveKeepOn()
{
    if (m_wakeLock.isValid())
    {
        m_wakeLock.callMethod<void>("release", "()V");
    }
}
