#include "mainwindow.h"
#include "networkmanager.h"
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) , m_networkManager(nullptr)
{
    setupUi();

    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onSendButtonClicked);
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi()
{
    setWindowTitle("Мессенджер");
    resize(800, 600);

    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // Левая панель (список чатов)
    chatListWidget = new QListWidget(this);
    chatListWidget->setMaximumWidth(200); // Ограничиваем ширину списка чатов

    // Правая панель (окно чата)
    QVBoxLayout *chatLayout = new QVBoxLayout();
    messageHistoryView = new QTextEdit(this);
    messageHistoryView->setReadOnly(true);

    // Нижняя часть (поле ввода и кнопка)
    QHBoxLayout *inputLayout = new QHBoxLayout();
    messageInput = new QLineEdit(this);
    messageInput->setPlaceholderText("Введите сообщение...");
    sendButton = new QPushButton("Отправить", this);

    inputLayout->addWidget(messageInput);
    inputLayout->addWidget(sendButton);

    chatLayout->addWidget(messageHistoryView);
    chatLayout->addLayout(inputLayout);

    mainLayout->addWidget(chatListWidget);
    mainLayout->addLayout(chatLayout);
    mainLayout->setStretch(0, 1); // левая панель
    mainLayout->setStretch(1, 3); // правая панель

    setCentralWidget(centralWidget);
}

void MainWindow::setUserLogin(const QString &login)
{
    m_userLogin = login;
    setWindowTitle("Мессенджер - " + m_userLogin);

    m_networkManager = new NetworkManager(m_userLogin, this);

    connect(m_networkManager, &NetworkManager::userListUpdated, this, &MainWindow::updateUserList);
}

void MainWindow::onSendButtonClicked()
{
    QString message = messageInput->text().trimmed();
    if (!message.isEmpty()) {
        messageHistoryView->append(m_userLogin + ": " + message);
        messageInput->clear();
        messageInput->setFocus(); // Возвращаем фокус на поле ввода
    }
}

void MainWindow::updateUserList(const QStringList &users)
{
    chatListWidget->clear();
    chatListWidget->addItem("Общий чат");

    for (const QString &user : users) {
        if (chatListWidget->findItems(user, Qt::MatchExactly).isEmpty()) {
            chatListWidget->addItem(user);
        }
    }
}
