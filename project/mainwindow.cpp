#include "mainwindow.h"
#include "networkmanager.h"
#include "database.h"
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
    , m_db(nullptr) // <--- ДОБАВЛЕНО: инициализация
    , m_currentUserId(-1) // <--- ДОБАВЛЕНО: инициализация
{
    setupUi();
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onSendButtonClicked);
    connect(chatListWidget, &QListWidget::currentItemChanged, this, &MainWindow::onChatSelectionChanged);
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
    connect(messageInput, &QLineEdit::returnPressed, this, &MainWindow::onSendButtonClicked);
}

void MainWindow::setUserLogin(const QString &login)
{
    m_userLogin = login;
    setWindowTitle("Мессенджер - " + m_userLogin);

    // Создаем и подключаем БД
    m_db = new Database(this);
    m_db->connect();

    // Получаем и сохраняем ID текущего пользователя
    m_currentUserId = m_db->getUserId(m_userLogin);
    if (m_currentUserId == -1) {
        qWarning() << "Could not retrieve current user ID from database!";
        // Здесь можно добавить обработку критической ошибки
    }

    // Создаем NetworkManager
    m_networkManager = new NetworkManager(m_userLogin, this);

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

    qint64 receiverId = m_db->getUserId(receiverLogin);
    if (receiverId != -1) {
        m_db->addMessage(m_currentUserId, receiverId, message);
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
    // --- ЛОГИКА СОХРАНЕНИЯ В БД ---
    qint64 senderId = m_db->getUserId(senderLogin);
    if (senderId != -1) {
        m_db->addMessage(senderId, m_currentUserId, message);
    }
    // ----------------------------

    // Отображаем сообщение, только если сейчас открыт чат с этим отправителем
    if (chatListWidget->currentItem() && chatListWidget->currentItem()->text() == senderLogin) {
        messageHistoryView->append(senderLogin + ": " + message);
    }
}

// --- НОВЫЙ СЛОТ ---
void MainWindow::onChatSelectionChanged()
{
    QListWidgetItem *currentItem = chatListWidget->currentItem();
    messageHistoryView->clear(); // Очищаем окно при смене чата

    if (!currentItem) return;

    QString selectedUser = currentItem->text();
    if (selectedUser == "Общий чат") {
        // Логика для общего чата
        return;
    }

    // --- ЛОГИКА ЗАГРУЗКИ ИЗ БД ---
    qint64 selectedUserId = m_db->getUserId(selectedUser);
    if (selectedUserId != -1) {
        QList<QPair<QString, QString>> history = m_db->getMessages(m_currentUserId, selectedUserId);
        for (const auto &messagePair : history) {
            messageHistoryView->append(messagePair.first + ": " + messagePair.second);
        }
    }
    // ----------------------------
}
