#include "authdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QCryptographicHash>
#include "cryptographymanager.h"
#include "database.h"

AuthDialog::AuthDialog(Database* db, QWidget *parent) :
    QDialog(parent),
    m_db(db)
{
    setupUi();

    connect(loginButton, &QPushButton::clicked, this, &AuthDialog::onLoginClicked);
    connect(registerButton, &QPushButton::clicked, this, &AuthDialog::onRegisterClicked);
}

AuthDialog::~AuthDialog()
{
}

void AuthDialog::setupUi()
{
    setWindowTitle("Аутентификация");
    setMinimumSize(300, 150);

    QLabel *loginLabel = new QLabel("Логин:", this);
    loginLineEdit = new QLineEdit(this);
    loginLineEdit->setPlaceholderText("Введите ваш логин");

    QLabel *passwordLabel = new QLabel("Пароль:", this);
    passwordLineEdit = new QLineEdit(this);
    passwordLineEdit->setPlaceholderText("Введите ваш пароль");
    passwordLineEdit->setEchoMode(QLineEdit::Password);

    loginButton = new QPushButton("Войти", this);
    registerButton = new QPushButton("Регистрация", this);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    mainLayout->addWidget(loginLabel);
    mainLayout->addWidget(loginLineEdit);
    mainLayout->addWidget(passwordLabel);
    mainLayout->addWidget(passwordLineEdit);
    mainLayout->addStretch();

    buttonLayout->addWidget(registerButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(loginButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

QString AuthDialog::getLogin() const
{
    return m_login;
}

void AuthDialog::onLoginClicked()
{
    const QString login = loginLineEdit->text();
    const QString password = passwordLineEdit->text();

    const QString passwordHash = QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());

    if (m_db->checkCredentials(login, passwordHash)) {
        m_login = login;
        m_password = password;
        accept();
    } else {
        QMessageBox::warning(this, "Ошибка входа", "Неверный логин или пароль.");
    }
}

void AuthDialog::onRegisterClicked()
{
    const QString login = loginLineEdit->text();
    const QString password = passwordLineEdit->text();

    if (login.length() < 3 || password.length() < 4) {
        QMessageBox::warning(this, "Ошибка регистрации", "Логин должен быть не менее 3 символов, а пароль - не менее 4.");
        return;
    }

    if (m_db->userExists(login)) {
        QMessageBox::warning(this, "Ошибка регистрации", "Пользователь с таким именем уже существует.");
        return;
    }

    // 1. Генерируем ключевую пару RSA
    auto pkey = CryptographyManager::generateRsaKeys();
    if (!pkey) {
        QMessageBox::critical(this, "Ошибка", "Не удалось сгенерировать ключевую пару RSA.");
        return;
    }

    // 2. Преобразуем публичный ключ в PEM
    QByteArray publicKeyPem = CryptographyManager::pkeyToPem(pkey.get(), false);

    // 3. Шифруем приватный ключ паролем (AES-256)
    QByteArray privateKeyEncryptedPem = CryptographyManager::encryptPrivateKey(pkey.get(), password);

    if (publicKeyPem.isEmpty() || privateKeyEncryptedPem.isEmpty()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось преобразовать или зашифровать ключи.");
        return;
    }

    // 4. Хешируем пароль для хранения в БД
    QString passwordHash = QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());

    // 5. Сохраняем все в базу данных
    if (m_db->addUser(login, passwordHash, QString::fromUtf8(publicKeyPem), QString::fromUtf8(privateKeyEncryptedPem))) {
        QMessageBox::information(this, "Успех", "Регистрация прошла успешно. Теперь вы можете войти.");
    } else {
        QMessageBox::warning(this, "Ошибка регистрации", "Не удалось добавить пользователя в базу данных.");
    }
}

QString AuthDialog::getPassword() const
{
    return m_password;
}
