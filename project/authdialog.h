#ifndef AUTHDIALOG_H
#define AUTHDIALOG_H

#include <QDialog>

class Database; // Используем forward declaration
class QLineEdit;
class QPushButton;

class AuthDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AuthDialog(Database* db, QWidget *parent = nullptr);
    ~AuthDialog();
    QString getLogin() const;
    // ИЗМЕНЕНИЕ: Добавляем метод для получения пароля
    QString getPassword() const;

private slots:
    void onLoginClicked();
    void onRegisterClicked();

private:
    void setupUi();

    QLineEdit *loginLineEdit;
    QLineEdit *passwordLineEdit;
    QPushButton *loginButton;
    QPushButton *registerButton;

    Database *m_db;
    QString m_login;
    // ИЗМЕНЕНИЕ: Добавляем поле для пароля
    QString m_password;
};

#endif // AUTHDIALOG_H
