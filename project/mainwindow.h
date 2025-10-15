#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

// Forward declarations
class QListWidget;
class QTextEdit;
class QLineEdit;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setUserLogin(const QString& login);

private slots:
    void onSendButtonClicked();

private:
    void setupUi(); // Метод для создания интерфейса

    QListWidget *chatListWidget;
    QTextEdit *messageHistoryView;
    QLineEdit *messageInput;
    QPushButton *sendButton;

    QString m_userLogin;
};
#endif // MAINWINDOW_H
