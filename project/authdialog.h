#ifndef AUTHDIALOG_H
#define AUTHDIALOG_H

#include <QDialog>

class Database;
class QLineEdit;
class QPushButton;

class AuthDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AuthDialog(Database* db, QWidget *parent = nullptr);
    ~AuthDialog();
    QString getLogin() const;
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
    QString m_password;
};

#endif // AUTHDIALOG_H
