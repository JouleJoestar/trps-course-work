#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "cryptographymanager.h"
#include <QSet>

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
    void setUserLogin(const QString& login, const QString& password);

private slots:
    void onSendButtonClicked();
    void updateUserList(const QStringList &users);
    void onMessageReceived(const QString &senderLogin, const QByteArray &encryptedMessage);
    void onBroadcastMessageReceived(const QString& senderLogin, const QString& message);
    void onChatSelectionChanged();

private:
    void setupUi();

    QListWidget *chatListWidget;
    QTextEdit *messageHistoryView;
    QLineEdit *messageInput;
    QPushButton *sendButton;
    QSet<QString> m_onlineUsers;

    QString m_userLogin;
    NetworkManager *m_networkManager;
    Database *m_db;

    CryptographyManager::EVP_PKEY_ptr m_privateKey;
};
#endif // MAINWINDOW_H
