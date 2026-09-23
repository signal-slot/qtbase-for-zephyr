#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <stdio.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>

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
    static char arg0[] = "netdiag";
    static char *argv[] = { arg0, nullptr };
    int argc = 1;

    qputenv("QT_QPA_PLATFORM", "zephyr");
    QApplication app(argc, argv);

    QWidget w;
    auto *layout = new QVBoxLayout(&w);
    auto *ipLabel = new QLabel("IP: waiting...");
    auto *udpLabel = new QLabel("UDP: -");
    auto *tcpLabel = new QLabel("TCP: -");
    ipLabel->setStyleSheet("font-size: 28px;");
    udpLabel->setStyleSheet("font-size: 22px;");
    tcpLabel->setStyleSheet("font-size: 22px;");
    layout->addWidget(ipLabel);
    layout->addWidget(udpLabel);
    layout->addWidget(tcpLabel);
    w.showFullScreen();

    auto *udpSocket = new QUdpSocket(&app);
    auto *udpRecv = new QUdpSocket(&app);
    auto *tcpSocket = new QTcpSocket(&app);
    int messageNo = 0;
    int udpRecvCount = 0;

    if (udpRecv->bind(QHostAddress::AnyIPv4, 12345))
        printf("[netdiag] UDP recv socket bound to :12345\n");
    else
        printf("[netdiag] UDP bind failed: %s\n", udpRecv->errorString().toUtf8().constData());

    QObject::connect(udpRecv, &QUdpSocket::readyRead, [&]() {
        while (udpRecv->hasPendingDatagrams()) {
            QByteArray data;
            data.resize(udpRecv->pendingDatagramSize());
            QHostAddress sender;
            quint16 senderPort;
            udpRecv->readDatagram(data.data(), data.size(), &sender, &senderPort);
            ++udpRecvCount;
            printf("[netdiag] UDP RECV #%d: %dB from %s:%d\n",
                   udpRecvCount, (int)data.size(),
                   sender.toString().toUtf8().constData(), senderPort);
        }
    });

    QObject::connect(tcpSocket, &QTcpSocket::connected, [&]() {
        tcpLabel->setText("TCP: connected!");
        printf("[netdiag] TCP connected to server\n");
        tcpSocket->write("Hello from Qt6 on Zephyr RT1170!\n");
        printf("[netdiag] TCP write done\n");
    });
    QObject::connect(tcpSocket, &QTcpSocket::readyRead, [&]() {
        printf("[netdiag] TCP readyRead fired\n");
        QByteArray data = tcpSocket->readAll();
        printf("[netdiag] TCP recv %d bytes\n", (int)data.size());
        QString msg = QString("TCP: recv %1B").arg(data.size());
        tcpLabel->setText(msg);
    });
    QObject::connect(tcpSocket, &QTcpSocket::disconnected, [&]() {
        printf("[netdiag] TCP disconnected\n");
        tcpLabel->setText("TCP: disconnected");
    });
    QObject::connect(tcpSocket, &QTcpSocket::errorOccurred, [&]() {
        printf("[netdiag] TCP error: %s\n", tcpSocket->errorString().toUtf8().constData());
        tcpLabel->setText("TCP: err=" + tcpSocket->errorString());
    });

    // HTTP test: GET from PC's simple HTTP server
    auto *nam = new QNetworkAccessManager(&app);
    bool httpTested = false;
    auto *httpLabel = new QLabel("HTTP: -");
    httpLabel->setStyleSheet("font-size: 22px;");
    layout->addWidget(httpLabel);

    auto *timer = new QTimer(&app);
    QObject::connect(timer, &QTimer::timeout, [&]() {
        QString ip = getZephyrIp();
        ipLabel->setText("IP: " + ip);

        // UDP broadcast
        QByteArray datagram = "Broadcast message " + QByteArray::number(messageNo);
        qint64 ret = udpSocket->writeDatagram(datagram, QHostAddress::Broadcast, 45454);
        QString status;
        if (ret == datagram.size()) {
            status = QString("UDP #%1 OK (%2B)").arg(messageNo).arg(ret);
        } else {
            status = QString("UDP #%1 FAIL: %2").arg(messageNo).arg(udpSocket->errorString());
        }
        udpLabel->setText(status);
        printf("[netdiag] %s  ip=%s\n", status.toUtf8().constData(), ip.toUtf8().constData());

        // TCP: retry connect every 5 seconds until connected
        if (messageNo >= 3 && messageNo % 5 == 3
            && tcpSocket->state() == QAbstractSocket::UnconnectedState) {
            QString serverIp = "192.168.1.82";
            tcpLabel->setText("TCP: connecting to " + serverIp + ":7777...");
            printf("[netdiag] TCP connecting to %s:7777\n", serverIp.toUtf8().constData());
            tcpSocket->connectToHost(serverIp, 7777);
        }

        // HTTP: retry every 10 seconds until success
        if (!httpTested && messageNo >= 5 && messageNo % 10 == 5) {
            QUrl url("http://192.168.1.82:8080/hello");
            printf("[netdiag] HTTP GET %s\n", url.toString().toUtf8().constData());
            httpLabel->setText("HTTP: requesting...");
            QNetworkReply *reply = nam->get(QNetworkRequest(url));
            QObject::connect(reply, &QNetworkReply::finished, [reply, &httpLabel, &httpTested]() {
                if (reply->error() == QNetworkReply::NoError) {
                    QByteArray body = reply->readAll();
                    printf("[netdiag] HTTP OK %d bytes: %s\n", (int)body.size(), body.left(60).constData());
                    httpLabel->setText(QString("HTTP: %1B OK").arg(body.size()));
                    httpTested = true;
                } else {
                    printf("[netdiag] HTTP error: %s\n", reply->errorString().toUtf8().constData());
                    httpLabel->setText("HTTP: " + reply->errorString().left(40));
                }
                reply->deleteLater();
            });
        }

        ++messageNo;
    });
    timer->start(1000);

    return app.exec();
}
