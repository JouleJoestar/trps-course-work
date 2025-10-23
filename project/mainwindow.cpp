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
#include <QListWidgetItem>

MainWindow::MainWindow(Database* db, QWidget *parent)
    : QMainWindow(parent)
    , m_networkManager(nullptr)
    , m_db(db)
{
    setupUi();
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onSendButtonClicked);
    connect(chatListWidget, &QListWidget::currentItemChanged, this, &MainWindow::onChatSelectionChanged);
    connect(m_networkManager, &NetworkManager::broadcastMessageReceived, this, &MainWindow::onBroadcastMessageReceived);

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

<<<<<<< Updated upstream
    // ИСПОЛЬЗУЕМ ЛОГИНЫ ВМЕСТО ID
    m_db->addMessage(m_userLogin, receiverLogin, message);

    m_networkManager->sendMessage(receiverLogin, message);

    messageHistoryView->append(m_userLogin + ": " + message);
=======
    if (receiverLogin == "Общий чат") {
        m_networkManager->sendBroadcastMessage(messageText);
        messageHistoryView->append(m_userLogin + ": " + messageText);
        messageInput->clear();
        messageInput->setFocus();
        return;
    }

    if (receiverLogin == m_userLogin) return;

    // Шифрование
    QString publicKeyPem = m_networkManager->getPublicKeyForUser(receiverLogin);
    if (publicKeyPem.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось найти публичный ключ для пользователя. Возможно, он оффлайн или еще не обнаружен.");
        return;
    }
    auto publicKey = CryptographyManager::pemToPkey(publicKeyPem.toUtf8(), false);
    if (!publicKey) {
        QMessageBox::warning(this, "Ошибка", "Публичный ключ пользователя некорректен.");
        return;
    }

    QByteArray encryptedMessage = CryptographyManager::hybridEncrypt(messageText.toUtf8(), publicKey.get());
    if (encryptedMessage.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось зашифровать сообщение.");
        return;
    }

    m_networkManager->sendMessage(receiverLogin, encryptedMessage);

    m_db->addMessage(m_userLogin, receiverLogin, messageText);

    messageHistoryView->append(m_userLogin + ": " + messageText);
>>>>>>> Stashed changes
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
<<<<<<< Updated upstream
=======

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

void MainWindow::onBroadcastMessageReceived(const QString &senderLogin, const QString &message)
{
    if (chatListWidget->currentItem() && chatListWidget->currentItem()->text() == "Общий чат") {
        messageHistoryView->append(senderLogin + ": " + message);
    } else {
        qDebug() << "Received a new broadcast message, but 'Общий чат' is not active.";
    }
}
>>>>>>> Stashed changes
