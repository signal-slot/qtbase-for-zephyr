// Copyright (C) 2026 Signal Slot Inc.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtlsbackend_wolfssl_p.h"
#include "qtls_wolfssl_p.h"

#include "../shared/qx509_generic_p.h"
#include "../shared/qtlskey_generic_p.h"

#include <QtNetwork/qsslsocket.h>

#include <QtCore/qmutex.h>

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

class TlsKeyWolfSSL final : public TlsKeyGeneric
{
public:
    using TlsKeyGeneric::TlsKeyGeneric;
    QByteArray decrypt(Cipher, const QByteArray &data,
                       const QByteArray &, const QByteArray &) const override
    { return data; }
    QByteArray encrypt(Cipher, const QByteArray &data,
                       const QByteArray &, const QByteArray &) const override
    { return data; }
};

} // namespace QTlsPrivate

Q_LOGGING_CATEGORY(lcTlsBackendWolfSSL, "qt.tlsbackend.wolfssl")

static bool wolfSSLInitialized = false;

QString QTlsBackendWolfSSL::getErrorsFromWolfSSL(int err)
{
    char errBuf[WOLFSSL_MAX_ERROR_SZ];
    wolfSSL_ERR_error_string_n(static_cast<unsigned long>(err), errBuf, sizeof(errBuf));
    return QString::fromLatin1(errBuf);
}

QString QTlsBackendWolfSSL::backendName() const
{
    return QStringLiteral("wolfssl");
}

bool QTlsBackendWolfSSL::isValid() const
{
    return true;
}

long QTlsBackendWolfSSL::tlsLibraryVersionNumber() const
{
    return static_cast<long>(wolfSSL_lib_version_hex());
}

QString QTlsBackendWolfSSL::tlsLibraryVersionString() const
{
    return QString::fromLatin1(wolfSSL_lib_version());
}

long QTlsBackendWolfSSL::tlsLibraryBuildVersionNumber() const
{
    return tlsLibraryVersionNumber();
}

QString QTlsBackendWolfSSL::tlsLibraryBuildVersionString() const
{
    return tlsLibraryVersionString();
}

void QTlsBackendWolfSSL::ensureInitialized() const
{
    if (!wolfSSLInitialized) {
        wolfSSL_Init();
        wolfSSLInitialized = true;
        qCDebug(lcTlsBackendWolfSSL) << "wolfSSL initialized, version:" << wolfSSL_lib_version();
    }
}

QList<QSsl::SslProtocol> QTlsBackendWolfSSL::supportedProtocols() const
{
    return {
        QSsl::TlsV1_2,
        QSsl::TlsV1_2OrLater,
        QSsl::TlsV1_3,
        QSsl::TlsV1_3OrLater,
        QSsl::SecureProtocols,
        QSsl::AnyProtocol,
    };
}

QList<QSsl::SupportedFeature> QTlsBackendWolfSSL::supportedFeatures() const
{
    return {
        QSsl::SupportedFeature::ClientSideAlpn,
    };
}

QList<QSsl::ImplementedClass> QTlsBackendWolfSSL::implementedClasses() const
{
    return {
        QSsl::ImplementedClass::Key,
        QSsl::ImplementedClass::Certificate,
        QSsl::ImplementedClass::Socket,
    };
}

QTlsPrivate::TlsKey *QTlsBackendWolfSSL::createKey() const
{
    return new QTlsPrivate::TlsKeyWolfSSL;
}

QTlsPrivate::X509Certificate *QTlsBackendWolfSSL::createCertificate() const
{
    return new QTlsPrivate::X509CertificateGeneric;
}

QTlsPrivate::TlsCryptograph *QTlsBackendWolfSSL::createTlsCryptograph() const
{
    return new QTlsPrivate::TlsCryptographWolfSSL;
}

QTlsPrivate::X509PemReaderPtr QTlsBackendWolfSSL::X509PemReader() const
{
    return QTlsPrivate::X509CertificateGeneric::certificatesFromPem;
}

QTlsPrivate::X509DerReaderPtr QTlsBackendWolfSSL::X509DerReader() const
{
    return QTlsPrivate::X509CertificateGeneric::certificatesFromDer;
}

QT_END_NAMESPACE

#include "moc_qtlsbackend_wolfssl_p.cpp"
