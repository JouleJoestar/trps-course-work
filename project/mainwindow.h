#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "cryptographymanager.h" // <-- ВАЖНЫЙ ИНКЛЮД, исправляет ошибку с CryptographyManager

class QListWidget;
class QTextEdit;
class QLineEdit;
class QPushButton;
class NetworkManager;
class Database;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Database* db, QWidget *parent = nullptr);
    ~MainWindow();
    // ИЗМЕНЕНИЕ: Добавляем параметр password
    void setUserLogin(const QString& login, const QString& password);

private slots:
    void onSendButtonClicked();
    void updateUserList(const QStringList &users);
    // ИЗМЕНЕНИЕ: Теперь принимаем QByteArray
    void onMessageReceived(const QString &senderLogin, const QByteArray &encryptedMessage);
    void onChatSelectionChanged();

private:
    void setupUi();

    QListWidget *chatListWidget;
    QTextEdit *messageHistoryView;
    QLineEdit *messageInput;
    QPushButton *sendButton;

    QString m_userLogin;
    NetworkManager *m_networkManager;
    Database *m_db;

    // ИЗМЕНЕНИЕ: Добавляем поле для приватного ключа
    CryptographyManager::EVP_PKEY_ptr m_privateKey;
};
#endif // MAINWINDOW_H
