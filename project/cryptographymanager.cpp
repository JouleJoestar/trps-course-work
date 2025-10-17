#include "cryptographymanager.h"
#include <QDebug>
#include <openssl/engine.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/obj_mac.h> // Для NID_id_GostR3410_2012_256

void CryptographyManager::init() {
    ENGINE_load_builtin_engines();
    ENGINE* engine = ENGINE_by_id("gost");
    if (!engine) { qWarning() << "Failed to get GOST engine:" << getOpenSSLError(); return; }
    if (!ENGINE_init(engine)) { qWarning() << "Failed to init GOST engine:" << getOpenSSLError(); ENGINE_free(engine); return; }
    ENGINE_set_default(engine, ENGINE_METHOD_ALL);
    qDebug() << "Successfully initialized GOST engine.";
}

CryptographyManager::EVP_PKEY_ptr CryptographyManager::generateGostKeys() {
    EVP_PKEY* pkey_raw = nullptr;
    // Используем наш псевдоним EVP_PKEY_CTX_ptr
    CryptographyManager::EVP_PKEY_CTX_ptr ctx(EVP_PKEY_CTX_new_id(NID_id_GostR3410_2012_256, nullptr), &EVP_PKEY_CTX_free);

    if (!ctx) { qWarning() << "Failed to create PKEY context:" << getOpenSSLError(); return EVP_PKEY_ptr(nullptr, &EVP_PKEY_free); }
    if (EVP_PKEY_keygen_init(ctx.get()) <= 0) { qWarning() << "Failed to init keygen:" << getOpenSSLError(); return EVP_PKEY_ptr(nullptr, &EVP_PKEY_free); }
    if (EVP_PKEY_CTX_ctrl_str(ctx.get(), "paramset", "A") <= 0) { qWarning() << "Failed to set paramset:" << getOpenSSLError(); return EVP_PKEY_ptr(nullptr, &EVP_PKEY_free); }
    if (EVP_PKEY_keygen(ctx.get(), &pkey_raw) <= 0) { qWarning() << "Failed to generate key pair:" << getOpenSSLError(); return EVP_PKEY_ptr(nullptr, &EVP_PKEY_free); }

    qDebug() << "Successfully generated GOST key pair.";
    return EVP_PKEY_ptr(pkey_raw, &EVP_PKEY_free);
}

QByteArray CryptographyManager::pkeyToPem(EVP_PKEY* pkey, bool isPrivate) {
    if (!pkey) return {};
    CryptographyManager::BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);
    int success = 0;
    if (isPrivate) {
        success = PEM_write_bio_PrivateKey(bio.get(), pkey, nullptr, nullptr, 0, nullptr, nullptr);
    } else {
        success = PEM_write_bio_PUBKEY(bio.get(), pkey);
    }
    if (success <= 0) { qWarning() << "Failed to write key to BIO:" << getOpenSSLError(); return {}; }
    char* key_str = nullptr;
    long key_len = BIO_get_mem_data(bio.get(), &key_str);
    return QByteArray(key_str, key_len);
}

QByteArray CryptographyManager::encryptPrivateKey(EVP_PKEY* pkey, const QString& password) {
    if (!pkey || password.isEmpty()) return {};
    CryptographyManager::BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);
    QByteArray passBytes = password.toUtf8();

    // В OpenSSL 1.1.1 используем EVP_get_cipherbyname
    const EVP_CIPHER* cipher = EVP_get_cipherbyname("gost89");
    if (!cipher) { qWarning() << "Failed to get GOST 89 cipher:" << getOpenSSLError(); return {}; }

    int success = PEM_write_bio_PrivateKey(bio.get(), pkey, cipher,
                                           (unsigned char*)passBytes.constData(), passBytes.length(),
                                           nullptr, nullptr);
    if (success <= 0) { qWarning() << "Failed to write and encrypt private key:" << getOpenSSLError(); return {}; }
    char* key_str = nullptr;
    long key_len = BIO_get_mem_data(bio.get(), &key_str);
    qDebug() << "Private key successfully encrypted.";
    return QByteArray(key_str, key_len);
}

QString CryptographyManager::getOpenSSLError() {
    CryptographyManager::BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);
    ERR_print_errors(bio.get());
    char* buf;
    long len = BIO_get_mem_data(bio.get(), &buf);
    return QString::fromLatin1(buf, len).trimmed();
}
