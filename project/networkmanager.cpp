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
    // --- UDP ЧАСТЬ ---
    udpSocket = new QUdpSocket(this);

    // Слушаем (bind) на всех интерфейсах (AnyIPv4). Это надежно.
    if (!udpSocket->bind(QHostAddress::AnyIPv4, broadcastPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        qWarning() << "UDP Socket could not bind:" << udpSocket->errorString();
    } else {
        qDebug() << "UDP Socket is listening on port" << broadcastPort;
    }

    connect(udpSocket, &QUdpSocket::readyRead, this, &NetworkManager::processPendingDatagrams);

    broadcastTimer = new QTimer(this);
    connect(broadcastTimer, &QTimer::timeout, this, &NetworkManager::sendBroadcast);
    broadcastTimer->start(5000); // <-- ЭТА СТРОКА БЫЛА ПРОПУЩЕНА
    sendBroadcast();
    // ----------------------

    // --- TCP ЧАСТЬ ---
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
    qDebug() << "Sending broadcast:" << datagram; // Можно раскомментировать для отладки
    udpSocket->writeDatagram(datagram, QHostAddress::Broadcast, broadcastPort);
}

void NetworkManager::processPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();
        QByteArray data = datagram.data();
        QHostAddress senderAddress = datagram.senderAddress();

        // Раскомментируйте для подробной отладки сети
        // qDebug() << "Received datagram from" << senderAddress.toString() << "with data:" << data;

        if (data.startsWith("DISCOVER:")) {
            QString discoveredUserLogin = QString::fromUtf8(data.mid(9));
            if (discoveredUserLogin == m_currentUserLogin)
                continue;

            // IPv4-адрес может прийти в формате "::ffff:192.168.1.5", нужно его очистить
            if (senderAddress.protocol() == QAbstractSocket::IPv4Protocol) {
                senderAddress = QHostAddress(senderAddress.toIPv4Address());
            }

            if (!m_discoveredUsers.contains(discoveredUserLogin) || m_discoveredUsers.value(discoveredUserLogin) != senderAddress) {
                qDebug() << "Discovered/updated user:" << discoveredUserLogin << "at" << senderAddress.toString();
                m_discoveredUsers[discoveredUserLogin] = senderAddress;
                emit userListUpdated(m_discoveredUsers.keys());
            }
        }
    }
}
