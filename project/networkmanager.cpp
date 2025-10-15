#include "networkmanager.h"
#include <QUdpSocket>
#include <QTimer>
#include <QNetworkDatagram>
#include <QDebug>
#include <QNetworkInterface>
#include <QAbstractSocket>
#include <QTcpServer> // Важные инклюды для TCP
#include <QTcpSocket> // Важные инклюды для TCP

NetworkManager::NetworkManager(const QString &currentUserLogin, QObject *parent)
    : QObject(parent), m_currentUserLogin(currentUserLogin)
{
    // --- UDP ЧАСТЬ ---
    udpSocket = new QUdpSocket(this);

    QHostAddress localAddress;
    const QList<QHostAddress> allAddresses = QNetworkInterface::allAddresses();
    for (const QHostAddress &address : allAddresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost)) {
            QString ip = address.toString();
            if (ip.startsWith("192.168.") || ip.startsWith("10.") || ip.startsWith("172.16.")) {
                localAddress = address;
                break;
            }
        }
    }
    if (localAddress.isNull()) {
        localAddress = QHostAddress::AnyIPv4;
    }

    udpSocket->bind(localAddress, broadcastPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(udpSocket, &QUdpSocket::readyRead, this, &NetworkManager::processPendingDatagrams);

    broadcastTimer = new QTimer(this);
    connect(broadcastTimer, &QTimer::timeout, this, &NetworkManager::sendBroadcast);
    broadcastTimer->start(5000);
    sendBroadcast();

    // --- TCP ЧАСТЬ (которой у вас не было) ---
    tcpServer = new QTcpServer(this);
    if (!tcpServer->listen(QHostAddress::Any, tcpPort)) {
        qWarning() << "TCP Server could not start on port" << tcpPort;
    } else {
        qDebug() << "TCP Server started, listening on port" << tcpPort;
    }
    connect(tcpServer, &QTcpServer::newConnection, this, &NetworkManager::onNewTcpConnection);

    qDebug() << "NetworkManager initialized for user" << m_currentUserLogin;
}

// --- РЕАЛИЗАЦИЯ sendMessage (которой у вас не было) ---
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

// --- РЕАЛИЗАЦИЯ onNewTcpConnection (которой у вас не было) ---
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

// --- UDP функции, которые у вас уже были ---
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

        if (data.startsWith("DISCOVER:")) {
            QString discoveredUserLogin = QString::fromUtf8(data.mid(9));
            if (discoveredUserLogin == m_currentUserLogin)
                continue;

            if (!m_discoveredUsers.contains(discoveredUserLogin) || m_discoveredUsers.value(discoveredUserLogin) != senderAddress) {
                m_discoveredUsers[discoveredUserLogin] = senderAddress;
                emit userListUpdated(m_discoveredUsers.keys());
            }
        }
    }
}
