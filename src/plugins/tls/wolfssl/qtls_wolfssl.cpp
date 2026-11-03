// Copyright (C) 2026 Signal Slot Inc.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtls_wolfssl_p.h"
#include "qtlsbackend_wolfssl_p.h"

#include <QtNetwork/qsslcertificate.h>
#include <QtNetwork/qsslcipher.h>
#include <QtNetwork/qsslsocket.h>
#include <QtNetwork/private/qsslcertificate_p.h>
#include <QtNetwork/private/qsslsocket_p.h>

#include <QtCore/qscopedvaluerollback.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qurl.h>

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/error-ssl.h>
#include <wolfssl/wolfcrypt/asn.h>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

TlsCryptographWolfSSL::~TlsCryptographWolfSSL()
{
    destroySslContext();
}

void TlsCryptographWolfSSL::init(QSslSocket *qObj, QSslSocketPrivate *dObj)
{
    Q_ASSERT(qObj);
    Q_ASSERT(dObj);
    q = qObj;
    d = dObj;
    sslErrors.clear();
    shutdown = false;
    inSetAndEmitError = false;
}

QList<QSslError> TlsCryptographWolfSSL::tlsErrors() const
{
    return sslErrors;
}

void TlsCryptographWolfSSL::startClientEncryption()
{
    if (!initSslContext()) {
        Q_ASSERT(d);
        setErrorAndEmit(d, QAbstractSocket::SslInternalError,
                        QSslSocket::tr("Unable to init SSL Context"));
        return;
    }
    startHandshake();
    transmit();
}

void TlsCryptographWolfSSL::startServerEncryption()
{
    if (!initSslContext()) {
        Q_ASSERT(d);
        setErrorAndEmit(d, QAbstractSocket::SslInternalError,
                        QSslSocket::tr("Unable to init SSL Context"));
        return;
    }
    startHandshake();
    transmit();
}

bool TlsCryptographWolfSSL::initSslContext()
{
    Q_ASSERT(q);
    Q_ASSERT(d);

    wolfSSL_Init();
    wc_AsnSetSkipDateCheck(1);

    const auto mode = d->tlsMode();
    const auto configuration = q->sslConfiguration();
    const auto protocol = configuration.protocol();

    WOLFSSL_METHOD *method = nullptr;
    if (mode == QSslSocket::SslClientMode) {
        switch (protocol) {
        case QSsl::TlsV1_2:
            method = wolfTLSv1_2_client_method();
            break;
        case QSsl::TlsV1_3:
        case QSsl::TlsV1_3OrLater:
            method = wolfTLSv1_3_client_method();
            break;
        default:
            method = wolfSSLv23_client_method();
            break;
        }
    } else {
        switch (protocol) {
        case QSsl::TlsV1_2:
            method = wolfTLSv1_2_server_method();
            break;
        case QSsl::TlsV1_3:
        case QSsl::TlsV1_3OrLater:
            method = wolfTLSv1_3_server_method();
            break;
        default:
            method = wolfSSLv23_server_method();
            break;
        }
    }

    ctx = wolfSSL_CTX_new(method);
    if (!ctx) {
        qCWarning(lcTlsBackendWolfSSL) << "wolfSSL_CTX_new failed, method =" << (void *)method;
        return false;
    }

    // Peer verification
    int verifyMode = SSL_VERIFY_NONE;
    switch (configuration.peerVerifyMode()) {
    case QSslSocket::VerifyNone:
        verifyMode = SSL_VERIFY_NONE;
        break;
    case QSslSocket::QueryPeer:
        verifyMode = SSL_VERIFY_PEER;
        break;
    case QSslSocket::VerifyPeer:
        verifyMode = SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
        break;
    case QSslSocket::AutoVerifyPeer:
        if (mode == QSslSocket::SslClientMode)
            verifyMode = SSL_VERIFY_PEER;
        else
            verifyMode = SSL_VERIFY_NONE;
        break;
    }
    wolfSSL_CTX_set_verify(ctx, verifyMode, nullptr);

    // Load CA certificates from QSslConfiguration
    const auto caCertificates = configuration.caCertificates();
    for (const QSslCertificate &cert : caCertificates) {
        const QByteArray der = cert.toDer();
        if (der.isEmpty())
            continue;
        wolfSSL_CTX_load_verify_buffer_ex(ctx,
                    reinterpret_cast<const unsigned char *>(der.constData()),
                    static_cast<long>(der.size()),
                    SSL_FILETYPE_ASN1, 0,
                    WOLFSSL_LOAD_FLAG_DATE_ERR_OKAY);
    }

    // Load local certificate if present
    const QSslCertificate localCert = configuration.localCertificate();
    if (!localCert.isNull()) {
        const QByteArray der = localCert.toDer();
        wolfSSL_CTX_use_certificate_buffer(ctx,
                    reinterpret_cast<const unsigned char *>(der.constData()),
                    static_cast<long>(der.size()),
                    SSL_FILETYPE_ASN1);
    }

    // Load private key if present
    const QSslKey privateKey = configuration.privateKey();
    if (!privateKey.isNull()) {
        const QByteArray der = privateKey.toDer();
        wolfSSL_CTX_use_PrivateKey_buffer(ctx,
                    reinterpret_cast<const unsigned char *>(der.constData()),
                    static_cast<long>(der.size()),
                    SSL_FILETYPE_ASN1);
    }

    // Set up I/O callbacks on CTX BEFORE wolfSSL_new() — wolfSSL_new
    // copies CTX callbacks into the ssl object at creation time.
    wolfSSL_CTX_SetIORecv(ctx, &TlsCryptographWolfSSL::wolfSSLReceiveCallback);
    wolfSSL_CTX_SetIOSend(ctx, &TlsCryptographWolfSSL::wolfSSLSendCallback);

    // Create SSL object
    ssl = wolfSSL_new(ctx);
    if (!ssl) {
        qCWarning(lcTlsBackendWolfSSL) << "wolfSSL_new failed";
        wolfSSL_CTX_free(ctx);
        ctx = nullptr;
        return false;
    }

    wolfSSL_SetIOReadCtx(ssl, this);
    wolfSSL_SetIOWriteCtx(ssl, this);


    // SNI (Server Name Indication)
    if (mode == QSslSocket::SslClientMode) {
        const auto verificationPeerName = d->verificationName();
        QString tlsHostName = verificationPeerName.isEmpty() ? q->peerName() : verificationPeerName;
        if (tlsHostName.isEmpty())
            tlsHostName = d->tlsHostName();
        QByteArray ace = QUrl::toAce(tlsHostName);
        if (!ace.isEmpty() && !QHostAddress().setAddress(tlsHostName)) {
            if (ace.endsWith('.'))
                ace.chop(1);
            wolfSSL_UseSNI(ssl, WOLFSSL_SNI_HOST_NAME,
                           ace.constData(), static_cast<unsigned short>(ace.size()));
        }
    }

    // ALPN
    const auto alpnProtocols = configuration.allowedNextProtocols();
    if (!alpnProtocols.isEmpty()) {
        QByteArray alpnList;
        for (const QByteArray &proto : alpnProtocols) {
            if (!alpnList.isEmpty())
                alpnList.append(',');
            alpnList.append(proto);
        }
        wolfSSL_UseALPN(ssl, const_cast<char *>(alpnList.constData()),
                        static_cast<unsigned int>(alpnList.size()),
                        WOLFSSL_ALPN_FAILED_ON_MISMATCH);
    }

    readBuffer.clear();
    writeBuffer.clear();

    return true;
}

void TlsCryptographWolfSSL::destroySslContext()
{
    if (ssl) {
        wolfSSL_free(ssl);
        ssl = nullptr;
    }
    if (ctx) {
        wolfSSL_CTX_free(ctx);
        ctx = nullptr;
    }
}

// wolfSSL I/O callback: called by wolfSSL_read/connect/accept when it
// needs to receive TLS record data.  We return bytes from readBuffer
// (filled by transmit() from the plain TCP socket).
int TlsCryptographWolfSSL::wolfSSLReceiveCallback(WOLFSSL *, char *buf, int sz, void *context)
{
    auto *self = static_cast<TlsCryptographWolfSSL *>(context);

    if (self->readBuffer.isEmpty())
        return WOLFSSL_CBIO_ERR_WANT_READ;

    int toRead = qMin(sz, static_cast<int>(self->readBuffer.size()));
    memcpy(buf, self->readBuffer.constData(), static_cast<size_t>(toRead));
    self->readBuffer.remove(0, toRead);
    return toRead;
}

// wolfSSL I/O callback: called by wolfSSL_write/connect/accept when it
// needs to send TLS record data.  We append to writeBuffer (drained
// by transmit() into the plain TCP socket).
int TlsCryptographWolfSSL::wolfSSLSendCallback(WOLFSSL *, char *buf, int sz, void *context)
{
    auto *self = static_cast<TlsCryptographWolfSSL *>(context);
    self->writeBuffer.append(buf, sz);
    return sz;
}

bool TlsCryptographWolfSSL::startHandshake()
{
    Q_ASSERT(q);
    Q_ASSERT(d);

    if (inSetAndEmitError)
        return false;

    const auto mode = d->tlsMode();
    int result = (mode == QSslSocket::SslClientMode)
                    ? wolfSSL_connect(ssl)
                    : wolfSSL_accept(ssl);

    if (result != SSL_SUCCESS) {
        int err = wolfSSL_get_error(ssl, result);
        switch (err) {
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            break;
        default: {
            QString errorString = QSslSocket::tr("Error during SSL handshake: %1")
                                    .arg(QTlsBackendWolfSSL::getErrorsFromWolfSSL(err));
            qCWarning(lcTlsBackendWolfSSL) << errorString;
            {
                QScopedValueRollback<bool> bg(inSetAndEmitError, true);
                setErrorAndEmit(d, QAbstractSocket::SslHandshakeFailedError, errorString);
            }
            q->abort();
            return false;
        }
        }
        return false;
    }

    storePeerCertificates();

    // Verify hostname
    const auto configuration = q->sslConfiguration();
    const bool doVerifyPeer = configuration.peerVerifyMode() == QSslSocket::VerifyPeer
                              || (configuration.peerVerifyMode() == QSslSocket::AutoVerifyPeer
                                  && mode == QSslSocket::SslClientMode);

    QList<QSslError> errors;

    if (!configuration.peerCertificate().isNull()) {
        if (mode == QSslSocket::SslClientMode) {
            const auto verificationPeerName = d->verificationName();
            QString peerName = verificationPeerName.isEmpty() ? q->peerName() : verificationPeerName;
            if (!isMatchingHostname(configuration.peerCertificate(), peerName)) {
                QSslError error(QSslError::HostNameMismatch, configuration.peerCertificate());
                errors << error;
                emit q->peerVerifyError(error);
                if (q->state() != QAbstractSocket::ConnectedState)
                    return false;
            }
        }
    } else if (doVerifyPeer) {
        QSslError error(QSslError::NoPeerCertificate);
        errors << error;
        emit q->peerVerifyError(error);
        if (q->state() != QAbstractSocket::ConnectedState)
            return false;
    }

    // Check blacklisted certificates
    const auto &peerCertificateChain = configuration.peerCertificateChain();
    for (const QSslCertificate &cert : peerCertificateChain) {
        if (QSslCertificatePrivate::isBlacklisted(cert)) {
            QSslError error(QSslError::CertificateBlacklisted, cert);
            errors << error;
            emit q->peerVerifyError(error);
            if (q->state() != QAbstractSocket::ConnectedState)
                return false;
        }
    }

    if (!errors.isEmpty()) {
        sslErrors = errors;
        emit q->sslErrors(sslErrors);

        const bool doVerifyPeer = configuration.peerVerifyMode() == QSslSocket::VerifyPeer
                                  || (configuration.peerVerifyMode() == QSslSocket::AutoVerifyPeer
                                      && mode == QSslSocket::SslClientMode);
        const bool doEmitSslError = !d->verifyErrorsHaveBeenIgnored();

        if (doVerifyPeer && doEmitSslError) {
            if (q->pauseMode() & QAbstractSocket::PauseOnSslErrors) {
                QSslSocketPrivate::pauseSocketNotifiers(q);
                d->setPaused(true);
            } else {
                QScopedValueRollback<bool> bg(inSetAndEmitError, true);
                setErrorAndEmit(d, QAbstractSocket::SslHandshakeFailedError,
                                sslErrors.constFirst().errorString());
                d->plainTcpSocket()->disconnectFromHost();
            }
            return false;
        }
    } else {
        sslErrors.clear();
    }

    continueHandshake();
    return true;
}

void TlsCryptographWolfSSL::continueHandshake()
{
    Q_ASSERT(q);
    Q_ASSERT(d);

    auto *plainSocket = d->plainTcpSocket();
    Q_ASSERT(plainSocket);

    if (const auto maxSize = d->maxReadBufferSize())
        plainSocket->setReadBufferSize(maxSize);

    // ALPN negotiation result
    char *alpnProto = nullptr;
    unsigned short alpnProtoSz = 0;
    int alpnErr = wolfSSL_ALPN_GetProtocol(ssl, &alpnProto, &alpnProtoSz);
    if (alpnErr == SSL_SUCCESS && alpnProto && alpnProtoSz > 0) {
        QByteArray negotiated(alpnProto, static_cast<int>(alpnProtoSz));
        QTlsBackend::setNegotiatedProtocol(d, negotiated);
        QTlsBackend::setAlpnStatus(d, QSslConfiguration::NextProtocolNegotiationNegotiated);
    } else {
        QTlsBackend::setNegotiatedProtocol(d, {});
        QTlsBackend::setAlpnStatus(d, QSslConfiguration::NextProtocolNegotiationUnsupported);
    }

    d->setEncrypted(true);
    emit q->encrypted();
}

void TlsCryptographWolfSSL::transmit()
{
    Q_ASSERT(q);
    Q_ASSERT(d);

    if (inSetAndEmitError)
        return;

    if (!ssl)
        return;

    auto &tlsWriteBuffer = d->tlsWriteBuffer();
    auto &buffer = d->tlsBuffer();
    auto *plainSocket = d->plainTcpSocket();
    Q_ASSERT(plainSocket);
    bool &emittedBytesWritten = d->tlsEmittedBytesWritten();

    bool transmitting;
    do {
        transmitting = false;

        // 1. If encrypted, take plaintext from Qt's write buffer and
        //    feed it to wolfSSL_write.  wolfSSL encrypts and calls our
        //    send callback, which appends to this->writeBuffer.
        if (q->isEncrypted() && !tlsWriteBuffer.isEmpty()) {
            qint64 totalBytesWritten = 0;
            int nextDataBlockSize;
            while ((nextDataBlockSize = tlsWriteBuffer.nextDataBlockSize()) > 0) {
                int writtenBytes = wolfSSL_write(ssl, tlsWriteBuffer.readPointer(), nextDataBlockSize);
                if (writtenBytes <= 0) {
                    int error = wolfSSL_get_error(ssl, writtenBytes);
                    if (error == SSL_ERROR_WANT_WRITE) {
                        transmitting = true;
                        break;
                    } else if (error == SSL_ERROR_WANT_READ) {
                        transmitting = false;
                        break;
                    } else {
                        QScopedValueRollback<bool> bg(inSetAndEmitError, true);
                        setErrorAndEmit(d, QAbstractSocket::SslInternalError,
                                        QSslSocket::tr("Unable to write data: %1")
                                            .arg(QTlsBackendWolfSSL::getErrorsFromWolfSSL(error)));
                        return;
                    }
                }
                tlsWriteBuffer.free(writtenBytes);
                totalBytesWritten += writtenBytes;

                if (writtenBytes < nextDataBlockSize) {
                    transmitting = true;
                    break;
                }
            }

            if (totalBytesWritten > 0) {
                if (!emittedBytesWritten) {
                    emittedBytesWritten = true;
                    emit q->bytesWritten(totalBytesWritten);
                    emittedBytesWritten = false;
                }
                emit q->channelBytesWritten(0, totalBytesWritten);
            }
        }

        // 2. Drain our internal writeBuffer (encrypted data produced by
        //    wolfSSL) to the underlying TCP socket.
        if (plainSocket->isValid() && !writeBuffer.isEmpty()
            && plainSocket->openMode() != QIODevice::NotOpen) {
            qint64 actualWritten = plainSocket->write(writeBuffer.constData(), writeBuffer.size());
            if (actualWritten < 0) {
                QScopedValueRollback<bool> bg(inSetAndEmitError, true);
                setErrorAndEmit(d, plainSocket->error(), plainSocket->errorString());
                return;
            }
            if (actualWritten > 0) {
                writeBuffer.remove(0, static_cast<int>(actualWritten));
                transmitting = true;
            }
        }

        // 3. Read encrypted data from the TCP socket into our internal
        //    readBuffer (which the receive callback returns to wolfSSL).
        if (!q->isEncrypted() || !d->maxReadBufferSize() || buffer.size() < d->maxReadBufferSize()) {
            while (plainSocket->bytesAvailable() > 0) {
                QByteArray data = plainSocket->readAll();
                if (data.isEmpty())
                    break;
                readBuffer.append(data);
                transmitting = true;
            }
        }

        // 4. If the connection isn't secured yet, retry the handshake
        //    (wolfSSL_connect/accept will consume data from readBuffer
        //    via our receive callback and produce data into writeBuffer
        //    via our send callback).
        if (!q->isEncrypted()) {
            if (startHandshake()) {
                d->setEncrypted(true);
                transmitting = true;
            } else if (plainSocket->state() != QAbstractSocket::ConnectedState) {
                break;
            } else if (d->isPaused()) {
                return;
            }
        }

        if (!ssl)
            continue;

        // 5. Read decrypted application data from wolfSSL into Qt's
        //    read buffer.
        int readBytes = 0;
        const int bytesToRead = 4096;
        do {
            if (q->readChannelCount() == 0)
                break;

            readBytes = wolfSSL_read(ssl, buffer.reserve(bytesToRead), bytesToRead);
            if (readBytes > 0) {
                buffer.chop(bytesToRead - readBytes);
                if (bool *readyReadEmittedPointer = d->readyReadPointer())
                    *readyReadEmittedPointer = true;
                emit q->readyRead();
                emit q->channelReadyRead(0);
                transmitting = true;
                continue;
            }
            buffer.chop(bytesToRead);

            int error = wolfSSL_get_error(ssl, readBytes);
            switch (error) {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                break;
            case SSL_ERROR_ZERO_RETURN:
                shutdown = true;
                {
                    QScopedValueRollback<bool> bg(inSetAndEmitError, true);
                    setErrorAndEmit(d, QAbstractSocket::RemoteHostClosedError,
                                    QSslSocket::tr("The TLS/SSL connection has been closed"));
                }
                return;
            default:
                {
                    QScopedValueRollback<bool> bg(inSetAndEmitError, true);
                    setErrorAndEmit(d, QAbstractSocket::SslInternalError,
                                    QSslSocket::tr("Error while reading: %1")
                                        .arg(QTlsBackendWolfSSL::getErrorsFromWolfSSL(error)));
                }
                return;
            }
        } while (ssl && readBytes > 0);
    } while (ssl && transmitting);
}

void TlsCryptographWolfSSL::disconnectFromHost()
{
    if (ssl) {
        if (!shutdown) {
            wolfSSL_shutdown(ssl);
            transmit();
        }
    }

    auto *plainSocket = d->plainTcpSocket();
    if (plainSocket)
        plainSocket->disconnectFromHost();
}

void TlsCryptographWolfSSL::disconnected()
{
    destroySslContext();
}

void TlsCryptographWolfSSL::storePeerCertificates()
{
    Q_ASSERT(d);

    QList<QSslCertificate> peerCertificateChain;

    WOLFSSL_X509 *peerX509 = wolfSSL_get_peer_certificate(ssl);
    if (peerX509) {
        int derSz = 0;
        const unsigned char *der = wolfSSL_X509_get_der(peerX509, &derSz);
        if (der && derSz > 0) {
            QByteArray derData(reinterpret_cast<const char *>(der), derSz);
            QList<QSslCertificate> certs = QSslCertificate::fromData(derData, QSsl::Der);
            if (!certs.isEmpty()) {
                QTlsBackend::storePeerCertificate(d, certs.first());
                peerCertificateChain << certs.first();
            }
        }
    }

    // Walk the peer certificate chain
    WOLFSSL_X509_CHAIN *chain = wolfSSL_get_peer_chain(ssl);
    if (chain) {
        int count = wolfSSL_get_chain_count(chain);
        for (int i = 0; i < count; ++i) {
            unsigned char *chainDer = wolfSSL_get_chain_cert(chain, i);
            int chainDerSz = wolfSSL_get_chain_length(chain, i);
            if (chainDer && chainDerSz > 0) {
                QByteArray derData(reinterpret_cast<const char *>(chainDer), chainDerSz);
                QList<QSslCertificate> certs = QSslCertificate::fromData(derData, QSsl::Der);
                for (const QSslCertificate &cert : certs) {
                    if (!peerCertificateChain.contains(cert))
                        peerCertificateChain << cert;
                }
            }
        }
    }

    QTlsBackend::storePeerCertificateChain(d, peerCertificateChain);
}

QSslCipher TlsCryptographWolfSSL::sessionCipher() const
{
    if (!ssl)
        return {};

    WOLFSSL_CIPHER *cipher = wolfSSL_get_current_cipher(ssl);
    if (!cipher)
        return {};

    const char *name = wolfSSL_CIPHER_get_name(cipher);
    if (!name)
        return {};

    return QTlsBackend::createCiphersuite(QString::fromLatin1(name), sessionProtocol(),
                                          QStringLiteral("TLSv1.2"));
}

QSsl::SslProtocol TlsCryptographWolfSSL::sessionProtocol() const
{
    if (!ssl)
        return QSsl::UnknownProtocol;

    int version = wolfSSL_version(ssl);
    switch (version) {
    case TLS1_2_VERSION:
        return QSsl::TlsV1_2;
    case TLS1_3_VERSION:
        return QSsl::TlsV1_3;
    default:
        return QSsl::UnknownProtocol;
    }
}

} // namespace QTlsPrivate

QT_END_NAMESPACE
