#ifndef CRYPTOGRAPHYMANAGER_H
#define CRYPTOGRAPHYMANAGER_H

#include <QString>
#include <QPair>
#include <QByteArray>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <memory>

class CryptographyManager {
public:
    using EVP_PKEY_ptr = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
    using BIO_ptr = std::unique_ptr<BIO, decltype(&BIO_free)>;
    using EVP_PKEY_CTX_ptr = std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>;

    static void init();
    static EVP_PKEY_ptr generateGostKeys();
    static QByteArray pkeyToPem(EVP_PKEY* pkey, bool isPrivate);
    static QByteArray encryptPrivateKey(EVP_PKEY* pkey, const QString& password);
private:
    static QString getOpenSSLError();
};

#endif
