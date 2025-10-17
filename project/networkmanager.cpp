#include "networkmanager.h"
#include <QUdpSocket>
#include <QTimer>
#include <QNetworkDatagram>
#include <QDebug>
#include <QNetworkInterface>
#include <QAbstractSocket>
#include <QTcpServer>
#include <QTcpSocket>

NetworkManager::NetworkManager(const QString &currentUserLogin, QObject *parent)
    : QObject(parent), m_currentUserLogin(currentUserLogin)
{
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
    // ----------------------

    tcpServer = new QTcpServer(this);
    if (!tcpServer->listen(QHostAddress::Any, tcpPort)) {
        qWarning() << "TCP Server could not start on port" << tcpPort;
    } else {
        qDebug() << "TCP Server started, listening on port" << tcpPort;
    }
    connect(tcpServer, &QTcpServer::newConnection, this, &NetworkManager::onNewTcpConnection);

    qDebug() << "NetworkManager initialized for user" << m_currentUserLogin;
}

void NetworkManager::sendMessage(const QString &receiverLogin, const QString &message)
{
    if (!m_discoveredUsers.contains(receiverLogin)) {
        qWarning() << "Cannot send message: user" << receiverLogin << "not found.";
        return;
    }

    QTcpSocket *socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);

    QHostAddress receiverAddress = m_discoveredUsers.value(receiverLogin);
    socket->connectToHost(receiverAddress, tcpPort);

    if (socket->waitForConnected(3000)) {
        qDebug() << "Connected to" << receiverLogin << "to send a message.";
        QByteArray data = (m_currentUserLogin + ":" + message).toUtf8();
        socket->write(data);
        socket->flush();
        socket->waitForBytesWritten(1000);
        socket->disconnectFromHost();
        qDebug() << "Message sent.";
    } else {
        qWarning() << "Could not connect to" << receiverLogin << ":" << socket->errorString();
    }
}

void NetworkManager::onNewTcpConnection()
{
    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();
    if (!clientSocket) return;

    qDebug() << "New TCP connection from" << clientSocket->peerAddress().toString();

    if (clientSocket->waitForReadyRead(1000)) {
        QByteArray data = clientSocket->readAll();
        QString rawData = QString::fromUtf8(data);
        qDebug() << "Received TCP data:" << rawData;

        int separatorIndex = rawData.indexOf(':');
        if (separatorIndex != -1) {
            QString senderLogin = rawData.left(separatorIndex);
            QString message = rawData.mid(separatorIndex + 1);
            emit messageReceived(senderLogin, message);
        }
    }

    clientSocket->disconnectFromHost();
    connect(clientSocket, &QTcpSocket::disconnected, clientSocket, &QTcpSocket::deleteLater);
}

void NetworkManager::sendBroadcast()
{
    QByteArray datagram = "DISCOVER:" + m_currentUserLogin.toUtf8();
    qDebug() << "Sending broadcast:" << datagram;
    udpSocket->writeDatagram(datagram, QHostAddress::Broadcast, broadcastPort);
}

void NetworkManager::processPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();
        QByteArray data = datagram.data();
        QHostAddress senderAddress = datagram.senderAddress();

        qDebug() << "[RAW] Received datagram from" << senderAddress.toString() << "with data:" << data;

        if (data.startsWith("DISCOVER:")) {
            QString discoveredUserLogin = QString::fromUtf8(data.mid(9));
            if (discoveredUserLogin == m_currentUserLogin)
                continue;

            // --- НАЧАЛО НОВОЙ ЛОГИКИ ---
            qDebug() << "[DEBUG] Processing packet from user:" << discoveredUserLogin;

            // Жестко приводим адрес к IPv4, чтобы гарантировать одинаковый формат
            QHostAddress cleanAddress(senderAddress.toIPv4Address());

            qDebug() << "[DEBUG] Original address:" << senderAddress.toString() << "| Cleaned address:" << cleanAddress.toString();

            // Проверяем, есть ли такой пользователь в нашем списке
            if (m_discoveredUsers.contains(discoveredUserLogin)) {
                // Если есть, сравниваем адреса
                if (m_discoveredUsers.value(discoveredUserLogin) == cleanAddress) {
                    // Адрес совпадает, ничего не делаем.
                    // qDebug() << "[DEBUG] Address for" << discoveredUserLogin << "is unchanged. Skipping.";
                    continue;
                } else {
                    qDebug() << "[DEBUG] Address for" << discoveredUserLogin << "has CHANGED from" << m_discoveredUsers.value(discoveredUserLogin).toString() << "to" << cleanAddress.toString();
                }
            } else {
                qDebug() << "[DEBUG] Discovered NEW user:" << discoveredUserLogin;
            }

            // Добавляем или обновляем пользователя
            m_discoveredUsers[discoveredUserLogin] = cleanAddress;

            qDebug() << "[ACTION] Emitting userListUpdated with users:" << m_discoveredUsers.keys();
            emit userListUpdated(m_discoveredUsers.keys());
            // --- КОНЕЦ НОВОЙ ЛОГИКИ ---
        }
    }
}
