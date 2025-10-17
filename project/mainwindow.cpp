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
    , m_networkManager(nullptr)
    , m_db(nullptr)
{
    setupUi();
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onSendButtonClicked);
    connect(chatListWidget, &QListWidget::currentItemChanged, this, &MainWindow::onChatSelectionChanged);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    setWindowTitle("Мессенджер");
    resize(800, 600);

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
    mainLayout->setStretch(1, 4);

    setCentralWidget(centralWidget);
    connect(messageInput, &QLineEdit::returnPressed, this, &MainWindow::onSendButtonClicked);
}

void MainWindow::setUserLogin(const QString &login)
{
    m_userLogin = login;
    setWindowTitle("Мессенджер - " + m_userLogin);

    m_db = new Database(this);
    m_db->connect();

    m_networkManager = new NetworkManager(m_userLogin, this);

    connect(m_networkManager, &NetworkManager::userListUpdated, this, &MainWindow::updateUserList);
    connect(m_networkManager, &NetworkManager::messageReceived, this, &MainWindow::onMessageReceived);
}

void MainWindow::onSendButtonClicked()
{
    QString message = messageInput->text().trimmed();
    if (message.isEmpty()) return;
    QListWidgetItem *currentItem = chatListWidget->currentItem();
    if (!currentItem) return;
    QString receiverLogin = currentItem->text();
    if (receiverLogin == "Общий чат" || receiverLogin == m_userLogin) return;

    // ИСПОЛЬЗУЕМ ЛОГИНЫ ВМЕСТО ID
    m_db->addMessage(m_userLogin, receiverLogin, message);

    m_networkManager->sendMessage(receiverLogin, message);

    messageHistoryView->append(m_userLogin + ": " + message);
    messageInput->clear();
    messageInput->setFocus();
}

void MainWindow::updateUserList(const QStringList &users)
{
    chatListWidget->blockSignals(true);

    QString selectedUser;
    if (chatListWidget->currentItem()) {
        selectedUser = chatListWidget->currentItem()->text();
    }

    chatListWidget->clear();
    chatListWidget->addItem("Общий чат");

    for (const QString &user : users) {
        if (user != m_userLogin) {
            chatListWidget->addItem(user);
        }
    }

    QList<QListWidgetItem*> items = chatListWidget->findItems(selectedUser, Qt::MatchExactly);
    if (!items.isEmpty()) {
        chatListWidget->setCurrentItem(items.first());
    }

    chatListWidget->blockSignals(false);

    if (chatListWidget->currentItem() && messageHistoryView->toPlainText().isEmpty()) {
        onChatSelectionChanged();
    }
}

void MainWindow::onMessageReceived(const QString &senderLogin, const QString &message)
{
    m_db->addMessage(senderLogin, m_userLogin, message);

    if (chatListWidget->currentItem() && chatListWidget->currentItem()->text() == senderLogin) {
        messageHistoryView->append(senderLogin + ": " + message);
    }
}

// --- НОВЫЙ СЛОТ ---
void MainWindow::onChatSelectionChanged()
{
    QListWidgetItem *currentItem = chatListWidget->currentItem();
    messageHistoryView->clear();
    if (!currentItem || currentItem->text() == "Общий чат") {
        return;
    }
    QString selectedUser = currentItem->text();

    // ИСПОЛЬЗУЕМ ЛОГИНЫ ВМЕСТО ID
    QList<QPair<QString, QString>> history = m_db->getMessages(m_userLogin, selectedUser);
    for (const auto &messagePair : history) {
        messageHistoryView->append(messagePair.first + ": " + messagePair.second);
    }
}
