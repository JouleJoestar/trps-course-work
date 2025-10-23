#include "mainwindow.h"
#include "networkmanager.h"
#include "database.h"
#include "cryptographymanager.h"
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

MainWindow::MainWindow(Database* db, QWidget *parent)
    : QMainWindow(parent)
    , m_networkManager(nullptr)
    , m_db(db)
    , m_privateKey(nullptr, &EVP_PKEY_free)
{
    setupUi();
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onSendButtonClicked);
    connect(m_networkManager, &NetworkManager::broadcastMessageReceived, this, &MainWindow::onBroadcastMessageReceived);
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

    chatListWidget = new QListWidget(this);
    chatListWidget->setMaximumWidth(200);

    QVBoxLayout *chatLayout = new QVBoxLayout();
    messageHistoryView = new QTextEdit(this);
    messageHistoryView->setReadOnly(true);

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
    mainLayout->setStretch(1, 4);

    setCentralWidget(centralWidget);
    connect(messageInput, &QLineEdit::returnPressed, this, &MainWindow::onSendButtonClicked);
}

void MainWindow::setUserLogin(const QString &login, const QString &password)
{
    m_userLogin = login;
    setWindowTitle("Мессенджер - " + m_userLogin);

    QString privateKeyPem = m_db->getEncryptedPrivateKey(m_userLogin);
    if (privateKeyPem.isEmpty()) {
        QMessageBox::critical(this, "Критическая ошибка", "Не удалось загрузить приватный ключ из базы данных.");
        return;
    }

    m_privateKey = CryptographyManager::pemToPkey(privateKeyPem.toUtf8(), true, password);

    if (!m_privateKey) {
        QMessageBox::critical(this, "Критическая ошибка", "Не удалось расшифровать приватный ключ. Вероятно, введен неверный пароль или ключ поврежден.");
        return;
    }
    qDebug() << "Private key successfully decrypted and loaded for" << m_userLogin;
    // ------------------------------------------
    m_networkManager = new NetworkManager(m_userLogin, m_db->getPublicKey(m_userLogin), this);

    connect(m_networkManager, &NetworkManager::userListUpdated, this, &MainWindow::updateUserList);
    connect(m_networkManager, &NetworkManager::messageReceived, this, &MainWindow::onMessageReceived);
}

void MainWindow::onSendButtonClicked()
{
    QString messageText = messageInput->text().trimmed();
    if (messageText.isEmpty()) return;

    QListWidgetItem *currentItem = chatListWidget->currentItem();
    if (!currentItem) return;

    QString receiverLogin = currentItem->text();

    if (receiverLogin == "Общий чат") {
        m_networkManager->sendBroadcastMessage(messageText);
        messageHistoryView->append(m_userLogin + ": " + messageText);
        messageInput->clear();
        messageInput->setFocus();
        return;
    }
    if (receiverLogin == m_userLogin) return;

    if (!m_onlineUsers.contains(receiverLogin)) {
        QMessageBox::warning(this, "Пользователь оффлайн", "Невозможно отправить сообщение, так как пользователь не в сети.");
        return;
    }

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
    messageInput->clear();
    messageInput->setFocus();
}

void MainWindow::onMessageReceived(const QString &senderLogin, const QByteArray &encryptedMessage)
{
    QByteArray decryptedMessage = CryptographyManager::hybridDecrypt(encryptedMessage, m_privateKey.get());

    if (decryptedMessage.isEmpty()) {
        qWarning() << "Failed to decrypt a message from" << senderLogin;
        return;
    }
    QString messageText = QString::fromUtf8(decryptedMessage);

    m_db->addMessage(senderLogin, m_userLogin, messageText);

    if (chatListWidget->currentItem() && chatListWidget->currentItem()->text() == senderLogin) {
        messageHistoryView->append(senderLogin + ": " + messageText);
    } else {
        qDebug() << "Received a new message from" << senderLogin << "but chat is not active.";
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

void MainWindow::onChatSelectionChanged()
{
    QListWidgetItem *currentItem = chatListWidget->currentItem();
    messageHistoryView->clear();
    if (!currentItem || currentItem->text() == "Общий чат") {
        return;
    }
    QString selectedUser = currentItem->text();

    QList<Message> history = m_db->getMessages(m_userLogin, selectedUser);

    for (const Message &msg : history) {
        QString timeString = msg.timestamp.toString("hh:mm");
        QString formattedMessage = QString("[%1] %2: %3")
                                       .arg(timeString)
                                       .arg(msg.senderLogin)
                                       .arg(msg.content);
        messageHistoryView->append(formattedMessage);
    }
}

void MainWindow::updateUserList(const QStringList &onlineUsers)
{
    m_onlineUsers = QSet<QString>(onlineUsers.begin(), onlineUsers.end());
    qDebug() << "Updating online users:" << m_onlineUsers;

    QStringList allChatPartners = m_db->getAllChatPartners(m_userLogin);
    for (const QString& user : allChatPartners) {
        if (chatListWidget->findItems(user, Qt::MatchExactly).isEmpty()) {
            chatListWidget->addItem(user);
        }
    }
    for (const QString& user : onlineUsers) {
        if (user != m_userLogin && chatListWidget->findItems(user, Qt::MatchExactly).isEmpty()) {
            chatListWidget->addItem(user);
        }
    }

    for (int i = 0; i < chatListWidget->count(); ++i) {
        QListWidgetItem *item = chatListWidget->item(i);
        QString login = item->text();

        if (login == "Общий чат") continue;

        if (m_onlineUsers.contains(login)) {
            item->setForeground(Qt::white);
            item->setFont(QFont("Arial", 10, QFont::Bold));
        } else {
            item->setForeground(Qt::gray);
            item->setFont(QFont("Arial", 10, QFont::Normal));
        }
    }
}
