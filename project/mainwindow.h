#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "cryptographymanager.h"
#include <QSet>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QDebug>
#include <QLabel>

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
    void addMessageToView(const QString& sender, const QString& text, const QString& time);

private:
    void setupUi();

    QListWidget *chatListWidget;
    QListWidget *messageView;
    QLineEdit *messageInput;
    QPushButton *sendButton;
    QSet<QString> m_onlineUsers;

    QString m_userLogin;
    NetworkManager *m_networkManager;
    Database *m_db;

    CryptographyManager::EVP_PKEY_ptr m_privateKey;
};
#endif // MAINWINDOW_H
