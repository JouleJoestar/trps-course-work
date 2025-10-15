#ifndef AUTHDIALOG_H
#define AUTHDIALOG_H

#include <QDialog>
#include "database.h"

// Forward declarations
class QLineEdit;
class QPushButton;

class AuthDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AuthDialog(QWidget *parent = nullptr);
    ~AuthDialog();
    QString getLogin() const;

private slots:
    void onLoginClicked();
    void onRegisterClicked();

private:
    void setupUi(); // Метод для создания интерфейса

    QLineEdit *loginLineEdit;
    QLineEdit *passwordLineEdit;
    QPushButton *loginButton;
    QPushButton *registerButton;

    Database *db;
    QString m_login;
};

#endif // AUTHDIALOG_H
