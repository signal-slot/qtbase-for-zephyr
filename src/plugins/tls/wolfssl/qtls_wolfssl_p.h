// Copyright (C) 2026 Signal Slot Inc.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTLS_WOLFSSL_P_H
#define QTLS_WOLFSSL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include <QtNetwork/private/qtlsbackend_p.h>

#include <QtNetwork/qsslcertificate.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qglobal.h>
#include <QtCore/qlist.h>

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

class TlsCryptographWolfSSL : public TlsCryptograph
{
public:
    ~TlsCryptographWolfSSL() override;

    void init(QSslSocket *qObj, QSslSocketPrivate *dObj) override;

    QList<QSslError> tlsErrors() const override;

    void startClientEncryption() override;
    void startServerEncryption() override;
    void continueHandshake() override;
    void transmit() override;
    void disconnectFromHost() override;
    void disconnected() override;
    QSslCipher sessionCipher() const override;
    QSsl::SslProtocol sessionProtocol() const override;

private:
    bool initSslContext();
    void destroySslContext();
    bool startHandshake();
    void storePeerCertificates();

    static int wolfSSLReceiveCallback(WOLFSSL *ssl, char *buf, int sz, void *ctx);
    static int wolfSSLSendCallback(WOLFSSL *ssl, char *buf, int sz, void *ctx);

    QSslSocket *q = nullptr;
    QSslSocketPrivate *d = nullptr;

    WOLFSSL_CTX *ctx = nullptr;
    WOLFSSL *ssl = nullptr;

    QByteArray readBuffer;
    QByteArray writeBuffer;

    QList<QSslError> sslErrors;

    bool shutdown = false;
    bool inSetAndEmitError = false;
};

} // namespace QTlsPrivate

QT_END_NAMESPACE

#endif // QTLS_WOLFSSL_P_H
