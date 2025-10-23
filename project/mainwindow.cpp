#include "mainwindow.h"
#include "networkmanager.h"
#include "database.h"
#include "messagewidget.h"

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
    setWindowTitle("Коллектив");
    resize(800, 600);

    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    chatListWidget = new QListWidget(this);
    chatListWidget->setMaximumWidth(200);

    QVBoxLayout *chatLayout = new QVBoxLayout();
    messageView = new QListWidget(this);
    messageView->setSpacing(5);
    messageView->setFocusPolicy(Qt::NoFocus);
    messageView->setStyleSheet("QListWidget { border: none; }");
    messageView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    messageView->setSelectionMode(QAbstractItemView::NoSelection);

    QHBoxLayout *inputLayout = new QHBoxLayout();
    messageInput = new QLineEdit(this);
    messageInput->setPlaceholderText("Введите сообщение...");
    sendButton = new QPushButton("Отправить", this);

    inputLayout->addWidget(messageInput);
    inputLayout->addWidget(sendButton);

    chatLayout->addWidget(messageView);
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
    setWindowTitle("Коллектив - " + m_userLogin);

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
        m_db->addMessage(m_userLogin, "__broadcast__", messageText);
        addMessageToView(m_userLogin, messageText, QDateTime::currentDateTime().toString("hh:mm"));
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

    addMessageToView(m_userLogin, messageText, QDateTime::currentDateTime().toString("hh:mm"));
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
        addMessageToView(senderLogin, messageText, QDateTime::currentDateTime().toString("hh:mm"));
    } else {
        qDebug() << "Received a new message from" << senderLogin << "but chat is not active.";
    }
}

void MainWindow::onBroadcastMessageReceived(const QString &senderLogin, const QString &message)
{
    m_db->addMessage(senderLogin, "__broadcast__", message);
    if (chatListWidget->currentItem() && chatListWidget->currentItem()->text() == "Общий чат") {
        addMessageToView(senderLogin, message, QDateTime::currentDateTime().toString("hh:mm"));
    } else {
        QList<QListWidgetItem*> items = chatListWidget->findItems("Общий чат", Qt::MatchExactly);
        if (!items.isEmpty()) {
            items.first()->setForeground(Qt::yellow);
        }
    }
}

void MainWindow::onChatSelectionChanged()
{
    QListWidgetItem *currentItem = chatListWidget->currentItem();
    messageView->clear();
    if (!currentItem) return;

    currentItem->setForeground(Qt::white);

    QString selectedChat = currentItem->text();
    QList<Message> history;

    if (selectedChat == "Общий чат") {
         history = m_db->getBroadcastMessages();
    } else {
        history = m_db->getMessages(m_userLogin, selectedChat);
    }

    for (const Message &msg : history) {
        addMessageToView(msg.senderLogin, msg.content, msg.timestamp.toString("hh:mm"));
    }
}

void MainWindow::updateUserList(const QStringList &onlineUsers)
{
    m_onlineUsers = QSet<QString>(onlineUsers.begin(), onlineUsers.end());
    qDebug() << "Updating online users:" << m_onlineUsers;

    QString selectedChat;
    if (chatListWidget->currentItem()) {
        selectedChat = chatListWidget->currentItem()->text();
    }

    if (chatListWidget->findItems("Общий чат", Qt::MatchExactly).isEmpty()) {
        chatListWidget->insertItem(0, "Общий чат");
    }

    QStringList allKnownUsers = m_db->getAllChatPartners(m_userLogin);
    allKnownUsers.append(onlineUsers);
    allKnownUsers.removeDuplicates();

    for (const QString& user : allKnownUsers) {
        if (user == m_userLogin || user == "__broadcast__") {
            continue;
        }
        if (chatListWidget->findItems(user, Qt::MatchExactly).isEmpty()) {
            chatListWidget->addItem(user);
        }
    }

    for (int i = 0; i < chatListWidget->count(); ++i) {
        QListWidgetItem *item = chatListWidget->item(i);
        QString login = item->text();

        if (login == "Общий чат") {
            continue;
        }

        if (m_onlineUsers.contains(login)) {
            item->setForeground(Qt::white);
            item->setFont(QFont("Arial", 10, QFont::Bold));
        } else {
            item->setForeground(Qt::gray);
            item->setFont(QFont("Arial", 10, QFont::Normal));
        }
    }

    if (!selectedChat.isEmpty()) {
        QList<QListWidgetItem*> items = chatListWidget->findItems(selectedChat, Qt::MatchExactly);
        if (!items.isEmpty()) {
            chatListWidget->blockSignals(true);
            chatListWidget->setCurrentItem(items.first());
            chatListWidget->blockSignals(false);
        }
    }
}

void MainWindow::addMessageToView(const QString &sender, const QString &text, const QString &time)
{
    bool isMyMessage = (sender == m_userLogin);
    bool isGeneralChat = (chatListWidget->currentItem() && chatListWidget->currentItem()->text() == "Общий чат");

    MessageWidget *messageWidget = new MessageWidget(sender, text, time, isMyMessage, isGeneralChat, messageView);

    QListWidgetItem *listItem = new QListWidgetItem();

    listItem->setSizeHint(messageWidget->sizeHint());

    messageView->addItem(listItem);

    messageView->setItemWidget(listItem, messageWidget);


    messageView->scrollToBottom();
}
