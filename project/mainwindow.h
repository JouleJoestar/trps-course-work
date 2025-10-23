#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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
    void setUserLogin(const QString& login);

private slots:
    void onSendButtonClicked();
    void updateUserList(const QStringList &users);
<<<<<<< Updated upstream
    void onMessageReceived(const QString &senderLogin, const QString &message);
=======
    void onMessageReceived(const QString &senderLogin, const QByteArray &encryptedMessage);
    void onBroadcastMessageReceived(const QString& senderLogin, const QString& message);
>>>>>>> Stashed changes
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
};
#endif // MAINWINDOW_H
