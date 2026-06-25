// Touch button audio demo for Zephyr RT1170-EVKB.
// Three on-screen buttons play ascending tones via QSoundEffect + WM8962 I2S.

#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QAudioDevice>
#include <QSoundEffect>
#include <QMediaDevices>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    auto dev = QMediaDevices::defaultAudioOutput();
    qDebug() << "audio:" << dev.description();

    QSoundEffect pi(dev);
    pi.setSource(QUrl(QStringLiteral("qrc:/tone_pi.wav")));
    pi.setVolume(1.0);

    QSoundEffect po(dev);
    po.setSource(QUrl(QStringLiteral("qrc:/tone_po.wav")));
    po.setVolume(1.0);

    QSoundEffect pa(dev);
    pa.setSource(QUrl(QStringLiteral("qrc:/tone_pa.wav")));
    pa.setVolume(1.0);

    QWidget window;
    window.setStyleSheet("QWidget { background: #1a1a2e; }"
                         "QPushButton { font-size: 48px; border-radius: 20px;"
                         "  min-height: 180px; min-width: 300px; }"
                         "QLabel { color: white; font-size: 36px; }");

    auto *layout = new QVBoxLayout(&window);
    layout->setSpacing(30);

    auto *title = new QLabel(QStringLiteral("Audio Demo"));
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto *btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(40);

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

    window.showFullScreen();
    return app.exec();
}
