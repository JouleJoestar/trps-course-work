#include "mainwindow.h"
#include "networkmanager.h"

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidgetItem> // Необходимо для работы с элементами списка

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_networkManager(nullptr) // Инициализируем указатель как null
{
    setupUi();
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onSendButtonClicked);
}

MainWindow::~MainWindow()
{
    // Qt автоматически удалит дочерние объекты, включая m_networkManager
}

void MainWindow::setupUi()
{
    setWindowTitle("Мессенджер");
    resize(800, 600);

    // Центральный виджет и главный компоновщик
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // Левая панель (список чатов)
    chatListWidget = new QListWidget(this);
    chatListWidget->setMaximumWidth(200);

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

    // Собираем главный компоновщик
    mainLayout->addWidget(chatListWidget);
    mainLayout->addLayout(chatLayout);
    mainLayout->setStretch(1, 4); // Правая панель будет в 4 раза шире левой

    setCentralWidget(centralWidget);
}

void MainWindow::setUserLogin(const QString &login)
{
    m_userLogin = login;
    setWindowTitle("Мессенджер - " + m_userLogin);

    // Создаем NetworkManager ПОСЛЕ того, как получили имя пользователя
    m_networkManager = new NetworkManager(m_userLogin, this);

    // Соединяем сигналы от NetworkManager со слотами для обновления UI
    connect(m_networkManager, &NetworkManager::userListUpdated, this, &MainWindow::updateUserList);
    connect(m_networkManager, &NetworkManager::messageReceived, this, &MainWindow::onMessageReceived);
}

void MainWindow::onSendButtonClicked()
{
    QString message = messageInput->text().trimmed();
    if (message.isEmpty()) {
        return;
    }

    // Определяем, кому отправлять, по выбранному элементу в списке
    QListWidgetItem *currentItem = chatListWidget->currentItem();
    if (!currentItem) {
        return; // Никто не выбран, ничего не делаем
    }
    QString receiverLogin = currentItem->text();

    // Если это "Общий чат" (пока не реализовано) или мы пытаемся отправить себе
    if (receiverLogin == "Общий чат" || receiverLogin == m_userLogin) {
        // Просто выводим сообщение у себя и ничего не отправляем
        messageHistoryView->append(m_userLogin + ": " + message);
        messageInput->clear();
        messageInput->setFocus();
        return;
    }

    // Отправляем сообщение через NetworkManager
    m_networkManager->sendMessage(receiverLogin, message);

    // Отображаем наше отправленное сообщение в окне чата
    messageHistoryView->append(m_userLogin + ": " + message);
    messageInput->clear();
    messageInput->setFocus();
}

void MainWindow::updateUserList(const QStringList &users)
{
    // Запоминаем, кто был выбран
    QString selectedUser;
    if (chatListWidget->currentItem()) {
        selectedUser = chatListWidget->currentItem()->text();
    }

    chatListWidget->clear();
    chatListWidget->addItem("Общий чат"); // Всегда добавляем "Общий чат"

    // Добавляем обнаруженных пользователей
    for (const QString &user : users) {
        // Проверка, чтобы не добавлять самого себя в список
        if (user != m_userLogin) {
            chatListWidget->addItem(user);
        }
    }

    // Пытаемся восстановить выбор
    QList<QListWidgetItem*> items = chatListWidget->findItems(selectedUser, Qt::MatchExactly);
    if (!items.isEmpty()) {
        chatListWidget->setCurrentItem(items.first());
    }
}

void MainWindow::onMessageReceived(const QString &senderLogin, const QString &message)
{
    // В будущем здесь будет проверка, активен ли чат с этим пользователем
    // А пока что просто выводим сообщение в текущее окно
    messageHistoryView->append(senderLogin + ": " + message);
}
