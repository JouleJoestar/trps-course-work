#ifndef AUTHDIALOG_H
#define AUTHDIALOG_H

#include <QDialog>
#include "database.h"

class QLineEdit;
class QPushButton;

class AuthDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AuthDialog(Database* db, QWidget *parent = nullptr);
    ~AuthDialog();
    QString getLogin() const;

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
};

#endif // AUTHDIALOG_H
