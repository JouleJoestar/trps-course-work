#include "cryptographymanager.h"
#include <QDebug>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/bio.h>

// --- ФУНКЦИИ ГЕНЕРАЦИИ И ШИФРОВАНИЯ КЛЮЧЕЙ ---

CryptographyManager::EVP_PKEY_ptr CryptographyManager::generateRsaKeys() {
    EVP_PKEY* pkey_raw = nullptr;
    CryptographyManager::EVP_PKEY_CTX_ptr ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr), &EVP_PKEY_CTX_free);

    if (!ctx) { qWarning() << "Failed to create RSA PKEY context:" << getOpenSSLError(); return EVP_PKEY_ptr(nullptr, &EVP_PKEY_free); }
    if (EVP_PKEY_keygen_init(ctx.get()) <= 0) { qWarning() << "Failed to init RSA keygen:" << getOpenSSLError(); return EVP_PKEY_ptr(nullptr, &EVP_PKEY_free); }
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx.get(), 2048) <= 0) { qWarning() << "Failed to set RSA key bits:" << getOpenSSLError(); return EVP_PKEY_ptr(nullptr, &EVP_PKEY_free); }
    if (EVP_PKEY_keygen(ctx.get(), &pkey_raw) <= 0) { qWarning() << "Failed to generate RSA key pair:" << getOpenSSLError(); return EVP_PKEY_ptr(nullptr, &EVP_PKEY_free); }

    qDebug() << "Successfully generated RSA-2048 key pair.";
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

    const EVP_CIPHER* cipher = EVP_aes_256_cbc();
    if (!cipher) { qWarning() << "Failed to get AES-256 cipher:" << getOpenSSLError(); return {}; }

    int success = PEM_write_bio_PrivateKey(bio.get(), pkey, cipher, (unsigned char*)passBytes.constData(), passBytes.length(), nullptr, nullptr);
    if (success <= 0) { qWarning() << "Failed to write and encrypt private key:" << getOpenSSLError(); return {}; }
    char* key_str = nullptr;
    long key_len = BIO_get_mem_data(bio.get(), &key_str);
    qDebug() << "Private key successfully encrypted with AES-256.";
    return QByteArray(key_str, key_len);
}


// --- ВСПОМОГАТЕЛЬНАЯ СТРУКТУРА И КОЛБЭК ДЛЯ ПАРОЛЯ ---

struct PasswordCallbackData {
    QByteArray password;
};

static int password_callback(char* buf, int size, int rwflag, void* userdata) {
    if (!userdata) return 0;
    PasswordCallbackData* cb_data = static_cast<PasswordCallbackData*>(userdata);
    if (cb_data->password.length() > size) return 0;
    memcpy(buf, cb_data->password.constData(), cb_data->password.length());
    return cb_data->password.length();
}


// --- ФУНКЦИИ ДЛЯ РАБОТЫ С PEM И ШИФРОВАНИЯ/ДЕШИФРОВАНИЯ ДАННЫХ ---

CryptographyManager::EVP_PKEY_ptr CryptographyManager::pemToPkey(const QByteArray& pem, bool isPrivate, const QString& password)
{
    if (pem.isEmpty()) return EVP_PKEY_ptr(nullptr, &EVP_PKEY_free);
    CryptographyManager::BIO_ptr bio(BIO_new_mem_buf(pem.constData(), pem.length()), &BIO_free);
    if (!bio) return EVP_PKEY_ptr(nullptr, &EVP_PKEY_free);

    EVP_PKEY* pkey_raw = nullptr;
    if (isPrivate) {
        PasswordCallbackData cb_data;
        cb_data.password = password.toUtf8();
        pkey_raw = PEM_read_bio_PrivateKey(bio.get(), nullptr, password_callback, &cb_data);
    } else {
        pkey_raw = PEM_read_bio_PUBKEY(bio.get(), nullptr, nullptr, nullptr);
    }

    if (!pkey_raw) {
        qWarning() << "Failed to read key from PEM:" << getOpenSSLError();
        return EVP_PKEY_ptr(nullptr, &EVP_PKEY_free);
    }
    return EVP_PKEY_ptr(pkey_raw, &EVP_PKEY_free);
}

QByteArray CryptographyManager::hybridEncrypt(const QByteArray& data, EVP_PKEY* publicKey)
{
    if (data.isEmpty() || !publicKey) return {};
    CryptographyManager::EVP_CIPHER_CTX_ptr ctx(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
    if (!ctx) return {};

    int pk_len = EVP_PKEY_size(publicKey);
    unsigned char* encrypted_key = new unsigned char[pk_len];
    int encrypted_key_len;
    unsigned char iv[EVP_MAX_IV_LENGTH];

    if (EVP_SealInit(ctx.get(), EVP_aes_256_cbc(), &encrypted_key, &encrypted_key_len, iv, &publicKey, 1) != 1) {
        delete[] encrypted_key;
        return {};
    }

    QByteArray encryptedData;
    encryptedData.resize(encrypted_key_len + EVP_MAX_IV_LENGTH + data.length() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));

    memcpy(encryptedData.data(), encrypted_key, encrypted_key_len);
    memcpy(encryptedData.data() + encrypted_key_len, iv, EVP_MAX_IV_LENGTH);

    int out_len = 0;
    if (EVP_SealUpdate(ctx.get(), (unsigned char*)encryptedData.data() + encrypted_key_len + EVP_MAX_IV_LENGTH, &out_len, (const unsigned char*)data.constData(), data.length()) != 1) {
        delete[] encrypted_key;
        return {};
    }
    int final_len = 0;
    if (EVP_SealFinal(ctx.get(), (unsigned char*)encryptedData.data() + encrypted_key_len + EVP_MAX_IV_LENGTH + out_len, &final_len) != 1) {
        delete[] encrypted_key;
        return {};
    }

    encryptedData.resize(encrypted_key_len + EVP_MAX_IV_LENGTH + out_len + final_len);
    delete[] encrypted_key;
    return encryptedData;
}

QByteArray CryptographyManager::hybridDecrypt(const QByteArray& encryptedData, EVP_PKEY* privateKey)
{
    if (encryptedData.isEmpty() || !privateKey) return {};
    CryptographyManager::EVP_CIPHER_CTX_ptr ctx(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
    if (!ctx) return {};

    int pk_len = EVP_PKEY_size(privateKey);
    if (encryptedData.length() < pk_len + EVP_MAX_IV_LENGTH) return {};

    unsigned char* encrypted_key = (unsigned char*)encryptedData.constData();
    unsigned char* iv = (unsigned char*)encryptedData.constData() + pk_len;

    if (EVP_OpenInit(ctx.get(), EVP_aes_256_cbc(), encrypted_key, pk_len, iv, privateKey) != 1) {
        return {};
    }

    QByteArray decryptedData;
    decryptedData.resize(encryptedData.length());

    const unsigned char* encrypted_content = (const unsigned char*)encryptedData.constData() + pk_len + EVP_MAX_IV_LENGTH;
    const int encrypted_content_len = encryptedData.length() - pk_len - EVP_MAX_IV_LENGTH;

    int out_len = 0;
    if (EVP_OpenUpdate(ctx.get(), (unsigned char*)decryptedData.data(), &out_len, encrypted_content, encrypted_content_len) != 1) {
        return {};
    }
    int final_len = 0;
    if (EVP_OpenFinal(ctx.get(), (unsigned char*)decryptedData.data() + out_len, &final_len) != 1) {
        qWarning() << "Decryption failed: incorrect private key or corrupted data." << getOpenSSLError();
        return {};
    }

    decryptedData.resize(out_len + final_len);
    return decryptedData;
}


// --- ФУНКЦИЯ ПОЛУЧЕНИЯ ОШИБОК ---

QString CryptographyManager::getOpenSSLError() {
    CryptographyManager::BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);
    unsigned long errCode = 0;
    QString errString;
    while ((errCode = ERR_get_error()) != 0) {
        char errBuf[256];
        ERR_error_string_n(errCode, errBuf, sizeof(errBuf));
        errString += QString::fromLatin1(errBuf) + "\n";
    }
    return errString.trimmed();
}
