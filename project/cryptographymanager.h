#ifndef CRYPTOGRAPHYMANAGER_H
#define CRYPTOGRAPHYMANAGER_H

#include <QPair>
#include <QByteArray>
#include <QString>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <memory>

class CryptographyManager {
public:
    using EVP_PKEY_ptr = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
    using BIO_ptr = std::unique_ptr<BIO, decltype(&BIO_free)>;
    using EVP_PKEY_CTX_ptr = std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>;
    using EVP_CIPHER_CTX_ptr = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>; // <-- ДОБАВЛЕН НЕДОСТАЮЩИЙ using

    static EVP_PKEY_ptr generateRsaKeys();
    static QByteArray pkeyToPem(EVP_PKEY* pkey, bool isPrivate);
    static QByteArray encryptPrivateKey(EVP_PKEY* pkey, const QString& password);

    static EVP_PKEY_ptr pemToPkey(const QByteArray& pem, bool isPrivate, const QString& password = QString());

    static QByteArray hybridEncrypt(const QByteArray& data, EVP_PKEY* publicKey);
    static QByteArray hybridDecrypt(const QByteArray& encryptedData, EVP_PKEY* privateKey);

private:
    static QString getOpenSSLError();
};

#endif
