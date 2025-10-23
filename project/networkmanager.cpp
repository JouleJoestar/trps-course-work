#include "networkmanager.h"

NetworkManager::NetworkManager(const QString &currentUserLogin, const QString& publicKey, QObject *parent)
    : QObject(parent), m_currentUserLogin(currentUserLogin), m_publicKey(publicKey)
{
    // --- UDP Setup ---
    udpSocket = new QUdpSocket(this);
    if (!udpSocket->bind(QHostAddress::AnyIPv4, broadcastPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        qWarning() << "UDP Socket could not bind:" << udpSocket->errorString();
    } else {
        qDebug() << "UDP Socket is listening on port" << broadcastPort;
    }
    connect(udpSocket, &QUdpSocket::readyRead, this, &NetworkManager::processPendingDatagrams);

    broadcastTimer = new QTimer(this);
    connect(broadcastTimer, &QTimer::timeout, this, &NetworkManager::sendBroadcast);
    broadcastTimer->start(5000);
    sendBroadcast();

    // --- TCP Setup ---
    tcpServer = new QTcpServer(this);
    if (!tcpServer->listen(QHostAddress::Any, tcpPort)) {
        qWarning() << "TCP Server could not start on port" << tcpPort;
    } else {
        qDebug() << "TCP Server started, listening on port" << tcpPort;
    }
    connect(tcpServer, &QTcpServer::newConnection, this, &NetworkManager::onNewTcpConnection);

    cleanupTimer = new QTimer(this);
    connect(cleanupTimer, &QTimer::timeout, this, &NetworkManager::checkInactiveUsers);
    cleanupTimer->start(5000);

    qDebug() << "NetworkManager initialized for user" << m_currentUserLogin;
}

void NetworkManager::sendBroadcastDatagram(const QByteArray &datagram)
{
    udpSocket->writeDatagram(datagram, QHostAddress::Broadcast, broadcastPort);

    for (const QNetworkInterface &interface : QNetworkInterface::allInterfaces()) {
        if ((interface.flags() & QNetworkInterface::IsUp) && (interface.flags() & QNetworkInterface::CanBroadcast)) {
            for (const QNetworkAddressEntry &entry : interface.addressEntries()) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    udpSocket->writeDatagram(datagram, entry.broadcast(), broadcastPort);
                }
            }
        }
    }
}

void NetworkManager::sendMessage(const QString &receiverLogin, const QByteArray &encryptedMessage)
{
    if (!m_discoveredUsers.contains(receiverLogin)) {
        qWarning() << "Cannot send private message: user" << receiverLogin << "not found.";
        return;
    }
    QTcpSocket *socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    QHostAddress receiverAddress = m_discoveredUsers.value(receiverLogin).ipAddress;
    socket->connectToHost(receiverAddress, tcpPort);

    if (socket->waitForConnected(3000)) {
        QByteArray data = m_currentUserLogin.toUtf8() + ":" + encryptedMessage;
        socket->write(data);
        socket->flush();
        socket->waitForBytesWritten(1000);
        socket->disconnectFromHost();
    } else {
        qWarning() << "Could not connect to" << receiverLogin << "for private message:" << socket->errorString();
    }
}

void NetworkManager::sendBroadcastMessage(const QString &message)
{
    QByteArray datagram = "MSG_ALL:" + m_currentUserLogin.toUtf8() + ":" + message.toUtf8();
    sendBroadcastDatagram(datagram);
}

QString NetworkManager::getPublicKeyForUser(const QString &login)
{
    return m_userKeys.value(login, QString());
}

void NetworkManager::onNewTcpConnection()
{
    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();
    if (!clientSocket) return;

    if (clientSocket->waitForReadyRead(1000)) {
        QByteArray rawData = clientSocket->readAll();
        int separatorIndex = rawData.indexOf(':');
        if (separatorIndex != -1) {
            QString senderLogin = QString::fromUtf8(rawData.left(separatorIndex));
            QByteArray encryptedMessage = rawData.mid(separatorIndex + 1);
            emit messageReceived(senderLogin, encryptedMessage);
        }
    }
    clientSocket->disconnectFromHost();
    connect(clientSocket, &QTcpSocket::disconnected, clientSocket, &QTcpSocket::deleteLater);
}

void NetworkManager::sendBroadcast()
{
    QByteArray datagram = "DISCOVER:" + m_currentUserLogin.toUtf8() + ":" + m_publicKey.toUtf8();
    sendBroadcastDatagram(datagram);
}

void NetworkManager::processPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();
        QByteArray data = datagram.data();
        QHostAddress senderAddress = datagram.senderAddress();

        if (data.startsWith("DISCOVER:")) {
            QList<QByteArray> parts = data.mid(9).split(':');
            if (parts.size() < 2) {
                continue;
            }

            QString discoveredUserLogin = QString::fromUtf8(parts.takeFirst());
            if (discoveredUserLogin == m_currentUserLogin) {
                continue;
            }

            QString discoveredUserKey = QString::fromUtf8(parts.join(':'));
            QHostAddress cleanAddress(senderAddress.toIPv4Address());

            if (!m_discoveredUsers.contains(discoveredUserLogin)) {
                QByteArray responseDatagram = "DISCOVER:" + m_currentUserLogin.toUtf8() + ":" + m_publicKey.toUtf8();
                udpSocket->writeDatagram(responseDatagram, cleanAddress, broadcastPort);
            }

            UserInfo userInfo;
            userInfo.ipAddress = cleanAddress;
            userInfo.lastSeen = QDateTime::currentDateTime();

            m_discoveredUsers[discoveredUserLogin] = userInfo;
            m_userKeys[discoveredUserLogin] = discoveredUserKey;

        } else if (data.startsWith("MSG_ALL:")) {
            QList<QByteArray> parts = data.mid(8).split(':');
            if (parts.size() < 2) {
                continue;
            }

            QString senderLogin = QString::fromUtf8(parts.takeFirst());
            if (senderLogin == m_currentUserLogin) {
                continue;
            }

            QString message = QString::fromUtf8(parts.join(':'));
            emit broadcastMessageReceived(senderLogin, message);
        }
    }
}

void NetworkManager::checkInactiveUsers()
{
    QDateTime currentTime = QDateTime::currentDateTime();
    QStringList activeUsers;
    QStringList allKnownUsers = m_discoveredUsers.keys();

    for (const QString& login : allKnownUsers) {
        if (m_discoveredUsers[login].lastSeen.secsTo(currentTime) <= 15) {
            activeUsers.append(login);
        }
    }

    emit userListUpdated(activeUsers);
}
