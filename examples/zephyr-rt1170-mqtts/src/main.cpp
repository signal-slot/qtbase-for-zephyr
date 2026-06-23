#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QSslCertificate>

#include <qmqttclient.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>

static const unsigned char ca_cert_der[] =
#include "ca_cert.inc"
;

static QString getZephyrIp()
{
    struct net_if *iface = net_if_get_default();
    if (!iface)
        return QStringLiteral("no iface");
    struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
    if (!ipv4)
        return QStringLiteral("no IPv4");
    for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
        if (ipv4->unicast[i].ipv4.is_used &&
            ipv4->unicast[i].ipv4.address.in_addr.s_addr != 0) {
            char buf[NET_IPV4_ADDR_LEN];
            net_addr_ntop(AF_INET, &ipv4->unicast[i].ipv4.address.in_addr,
                          buf, sizeof(buf));
            return QString::fromLatin1(buf);
        }
    }
    return QStringLiteral("0.0.0.0");
}

int main(int /*argc_zephyr*/, char * /*argv_zephyr*/[])
{
    static char arg0[] = "mqtts";
    static char *argv[] = { arg0, nullptr };
    int argc = 1;

    qputenv("QT_QPA_PLATFORM", "zephyr");
    QApplication app(argc, argv);

    QWidget w;
    auto *layout = new QVBoxLayout(&w);
    auto *ipLabel = new QLabel("IP: waiting...");
    auto *mqttLabel = new QLabel("MQTT: not connected");
    auto *msgLabel = new QLabel("Last msg: -");
    ipLabel->setStyleSheet("font-size: 28px;");
    mqttLabel->setStyleSheet("font-size: 22px;");
    msgLabel->setStyleSheet("font-size: 20px;");
    layout->addWidget(ipLabel);
    layout->addWidget(mqttLabel);
    layout->addWidget(msgLabel);
    w.showFullScreen();

    static const QString brokerHost = QStringLiteral("192.168.1.82");
    static const quint16 brokerPort = 8883;

    QSslCertificate caCert(QByteArray(reinterpret_cast<const char *>(ca_cert_der),
                                       sizeof(ca_cert_der)), QSsl::Der);

    auto *sslSocket = new QSslSocket(&app);
    QSslConfiguration sslConfig = sslSocket->sslConfiguration();
    sslConfig.setCaCertificates({caCert});
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyPeer);
    sslSocket->setSslConfiguration(sslConfig);

    auto *mqtt = new QMqttClient(&app);
    mqtt->setHostname(brokerHost);
    mqtt->setPort(brokerPort);
    mqtt->setClientId(QStringLiteral("rt1170-zephyr"));
    mqtt->setTransport(sslSocket, QMqttClient::SecureSocket);

    QObject::connect(sslSocket, &QAbstractSocket::errorOccurred, [&](QAbstractSocket::SocketError e) {
        QString detail = sslSocket->errorString().left(80);
        printk("[mqtts] socket error %d: %s\n", (int)e,
               detail.toUtf8().constData());
        mqttLabel->setText(QString("err%1: %2").arg((int)e).arg(detail));
    });

    QObject::connect(sslSocket, &QSslSocket::encrypted, [&]() {
        printk("[mqtts] TLS handshake done (wolfSSL), sending MQTT CONNECT\n");
        mqttLabel->setText(QStringLiteral("MQTT: TLS up, connecting..."));
        mqtt->connectToHost();
    });

    QObject::connect(mqtt, &QMqttClient::connected, [&]() {
        printk("[mqtts] MQTT connected!\n");
        mqttLabel->setText(QStringLiteral("MQTT: CONNECTED"));

        auto *sub = mqtt->subscribe(QMqttTopicFilter(QStringLiteral("test/zephyr")));
        QObject::connect(sub, &QMqttSubscription::messageReceived,
                         [&](const QMqttMessage &msg) {
            QString text = msg.payload().left(60);
            printk("[mqtts] recv: %s\n", text.toUtf8().constData());
            msgLabel->setText(QStringLiteral("Recv: ") + text);
        });

        mqtt->publish(QMqttTopicName(QStringLiteral("test/zephyr/status")),
                      "RT1170 online via MQTTS (wolfSSL)");
    });

    QObject::connect(mqtt, &QMqttClient::disconnected, [&]() {
        printk("[mqtts] MQTT disconnected\n");
        mqttLabel->setText(QStringLiteral("MQTT: disconnected"));
    });

    QObject::connect(mqtt, &QMqttClient::errorChanged, [&]() {
        printk("[mqtts] MQTT error: %d\n", (int)mqtt->error());
        mqttLabel->setText(QString("MQTT err: %1").arg((int)mqtt->error()));
    });

    int connectAttempt = 0;
    auto *timer = new QTimer(&app);
    QObject::connect(timer, &QTimer::timeout, [&]() {
        ipLabel->setText(QStringLiteral("IP: ") + getZephyrIp());

        if (sslSocket->state() == QAbstractSocket::UnconnectedState
            && mqtt->state() == QMqttClient::Disconnected
            && connectAttempt < 10) {
            ++connectAttempt;
            printk("[mqtts] Connecting (attempt %d)...\n", connectAttempt);
            mqttLabel->setText(QString("MQTT: connecting #%1...").arg(connectAttempt));
            sslSocket->connectToHostEncrypted(brokerHost, brokerPort);
        }
    });
    timer->start(5000);

    return app.exec();
}
