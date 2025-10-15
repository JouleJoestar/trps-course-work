#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QListWidget;
class QTextEdit;
class QLineEdit;
class QPushButton;
class NetworkManager;

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

private:
    void setupUi();

    QListWidget *chatListWidget;
    QTextEdit *messageHistoryView;
    QLineEdit *messageInput;
    QPushButton *sendButton;

    QString m_userLogin;
    NetworkManager *m_networkManager;
};
#endif // MAINWINDOW_H
