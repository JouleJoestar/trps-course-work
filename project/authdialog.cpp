#include "authdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QCryptographicHash>
#include "cryptographymanager.h"

AuthDialog::AuthDialog(QWidget *parent) :
    QDialog(parent)
{
    setupUi();

    db = new Database(this);
    if (!db->connect()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось подключиться к базе данных.");
        loginButton->setEnabled(false);
        registerButton->setEnabled(false);
    }

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

    if (db->checkCredentials(login, passwordHash)) {
        m_login = login;
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

    if (db->userExists(login)) {
        QMessageBox::warning(this, "Ошибка регистрации", "Пользователь с таким именем уже существует.");
        return;
    }

    auto keys = CryptographyManager::generateKeys(password);
    QString publicKey = keys.first;
    QString encryptedPrivateKey = keys.second;

    QString passwordHash = QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());

    if (db->addUser(login, passwordHash, publicKey, encryptedPrivateKey)) {
        QMessageBox::information(this, "Успех", "Регистрация прошла успешно. Теперь вы можете войти.");
    } else {
        QMessageBox::warning(this, "Ошибка регистрации", "Не удалось зарегистрировать пользователя. Попробуйте еще раз.");
    }
}
