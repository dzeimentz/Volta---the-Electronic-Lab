#include "audio_route.hpp"

#include <QAudioDevice>
#include <QMediaDevices>

#ifdef Q_OS_ANDROID
#include <QJniEnvironment>
#include <QJniObject>
#include <QtCore/qcoreapplication_platform.h>
#endif

namespace volta::services {

AudioRoute::AudioRoute(QObject* parent) : QObject(parent), m_devices(new QMediaDevices(this)) {
    connect(m_devices, &QMediaDevices::audioOutputsChanged, this, &AudioRoute::refresh);
    m_poll.setInterval(2000);
    connect(&m_poll, &QTimer::timeout, this, &AudioRoute::refresh);
    m_poll.start();
    m_headphones = detect();
}

void AudioRoute::refresh() {
    const bool now = detect();
    if (now == m_headphones) return;
    m_headphones = now;
    emit changed();
}

bool AudioRoute::looksLikeHeadphones(const QString& deviceName) {
    const QString n = deviceName.toLower();
    for (const char* word : {"kopfh", "headphone", "headset", "casque", "couteur", "earphone", "airpods", "buds"})
        if (n.contains(QLatin1String(word))) return true;
    return false;
}

bool AudioRoute::detect() const {
#ifdef Q_OS_ANDROID
    // AudioManager.getDevices(GET_DEVICES_OUTPUTS = 2); Typen: 3 Kabel-Headset, 4 Kabel-Kopfhoerer,
    // 8 Bluetooth A2DP, 22 USB-Headset, 26 BLE-Headset
    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    if (!context.isValid()) return false;
    const QJniObject name = QJniObject::fromString(QStringLiteral("audio"));
    const QJniObject manager = context.callObjectMethod("getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;",
                                                        name.object<jstring>());
    if (!manager.isValid()) return false;
    const QJniObject devices = manager.callObjectMethod("getDevices", "(I)[Landroid/media/AudioDeviceInfo;", 2);
    if (!devices.isValid()) return false;
    QJniEnvironment env;
    const auto array = devices.object<jobjectArray>();
    const jsize count = env->GetArrayLength(array);
    for (jsize i = 0; i < count; ++i) {
        const QJniObject device = QJniObject::fromLocalRef(env->GetObjectArrayElement(array, i));
        const int type = device.callMethod<jint>("getType", "()I");
        if (type == 3 || type == 4 || type == 8 || type == 22 || type == 26) return true;
    }
    return false;
#else
    return looksLikeHeadphones(QMediaDevices::defaultAudioOutput().description());
#endif
}

} // namespace volta::services
