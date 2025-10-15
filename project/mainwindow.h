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
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setUserLogin(const QString& login);

private slots:
    void onSendButtonClicked();
    void updateUserList(const QStringList &users);
    void onMessageReceived(const QString &senderLogin, const QString &message);
    void onChatSelectionChanged();

private:
    void setupUi();

    QListWidget *chatListWidget;
    QTextEdit *messageHistoryView;
    QLineEdit *messageInput;
    QPushButton *sendButton;

    QString m_userLogin;
    qint64 m_currentUserId;
    NetworkManager *m_networkManager;
    Database *m_db;
};
#endif // MAINWINDOW_H
