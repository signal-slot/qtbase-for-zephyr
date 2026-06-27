// Touch button audio demo for Zephyr RT1170-EVKB.
// Top row: three buttons play ascending tones via QSoundEffect.
// Bottom row: Rec button records from mic, Play button plays it back.

#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSource>
#include <QAudioSink>
#include <QBuffer>
#include <QSoundEffect>
#include <QMediaDevices>
#include <QTimer>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    auto outDev = QMediaDevices::defaultAudioOutput();
    auto inDev = QMediaDevices::defaultAudioInput();
    qDebug() << "output:" << outDev.description();
    qDebug() << "input:" << inDev.description();

    QSoundEffect pi(outDev);
    pi.setSource(QUrl(QStringLiteral("qrc:/tone_pi.wav")));
    pi.setVolume(1.0);

    QSoundEffect po(outDev);
    po.setSource(QUrl(QStringLiteral("qrc:/tone_po.wav")));
    po.setVolume(1.0);

    QSoundEffect pa(outDev);
    pa.setSource(QUrl(QStringLiteral("qrc:/tone_pa.wav")));
    pa.setVolume(1.0);

    QAudioFormat fmt;
    fmt.setSampleRate(48000);
    fmt.setChannelCount(2);
    fmt.setSampleFormat(QAudioFormat::Int16);

    QAudioSource *audioSource = new QAudioSource(inDev, fmt);
    QAudioSink *audioSink = new QAudioSink(outDev, fmt);
    QBuffer *recBuffer = new QBuffer;
    QByteArray recData;
    recBuffer->setBuffer(&recData);
    bool recording = false;

    QWidget window;
    window.setStyleSheet("QWidget { background: #1a1a2e; }"
                         "QPushButton { font-size: 48px; border-radius: 20px;"
                         "  min-height: 140px; min-width: 250px; }"
                         "QLabel { color: white; font-size: 36px; }");

    auto *layout = new QVBoxLayout(&window);
    layout->setSpacing(20);

    auto *title = new QLabel(QStringLiteral("Audio Demo"));
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto *btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(30);

    auto *btnPi = new QPushButton(QStringLiteral("Pi"));
    btnPi->setStyleSheet("background: #e94560; color: white;");
    btnLayout->addWidget(btnPi);

    auto *btnPo = new QPushButton(QStringLiteral("Po"));
    btnPo->setStyleSheet("background: #0f3460; color: white;");
    btnLayout->addWidget(btnPo);

    auto *btnPa = new QPushButton(QStringLiteral("Pa"));
    btnPa->setStyleSheet("background: #533483; color: white;");
    btnLayout->addWidget(btnPa);

    layout->addLayout(btnLayout);

    auto *recLayout = new QHBoxLayout;
    recLayout->setSpacing(30);

    auto *btnRec = new QPushButton(QStringLiteral("Rec"));
    btnRec->setStyleSheet("background: #c0392b; color: white;");
    recLayout->addWidget(btnRec);

    auto *btnPlay = new QPushButton(QStringLiteral("Play"));
    btnPlay->setStyleSheet("background: #27ae60; color: white;");
    recLayout->addWidget(btnPlay);

    auto *statusLabel = new QLabel(QStringLiteral("Ready"));
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: #aaa; font-size: 28px;");

    layout->addLayout(recLayout);
    layout->addWidget(statusLabel);

    QObject::connect(btnPi, &QPushButton::clicked, [&pi]() {
        qDebug() << "pi!";
        pi.play();
    });
    QObject::connect(btnPo, &QPushButton::clicked, [&po]() {
        qDebug() << "po!";
        po.play();
    });
    QObject::connect(btnPa, &QPushButton::clicked, [&pa]() {
        qDebug() << "pa!";
        pa.play();
    });

    QObject::connect(btnRec, &QPushButton::pressed, [&]() {
        if (recording) return;
        recData.clear();
        recBuffer->close();
        recBuffer->setBuffer(&recData);
        recBuffer->open(QIODevice::WriteOnly);
        audioSource->start(recBuffer);
        recording = true;
        statusLabel->setText(QStringLiteral("Recording..."));
        qDebug() << "rec start";
    });

    QObject::connect(btnRec, &QPushButton::released, [&]() {
        if (!recording) return;
        audioSource->stop();
        recBuffer->close();
        recording = false;
        int ms = recData.size() * 1000 / (fmt.sampleRate() * fmt.bytesPerFrame());
        statusLabel->setText(QString("Recorded %1 ms").arg(ms));
        qDebug() << "rec stop:" << recData.size() << "bytes," << ms << "ms";
    });

    QObject::connect(btnPlay, &QPushButton::clicked, [&]() {
        if (recording || recData.isEmpty()) return;
        recBuffer->close();
        recBuffer->setBuffer(&recData);
        recBuffer->open(QIODevice::ReadOnly);
        audioSink->start(recBuffer);
        statusLabel->setText(QStringLiteral("Playing..."));
        qDebug() << "play start:" << recData.size() << "bytes";
    });

    QObject::connect(audioSink, &QAudioSink::stateChanged, [&](QAudio::State state) {
        if (state == QAudio::IdleState) {
            audioSink->stop();
            statusLabel->setText(QStringLiteral("Ready"));
            qDebug() << "play done";
        }
    });

    window.showFullScreen();

    return app.exec();
}
