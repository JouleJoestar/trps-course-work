#include "networkmanager.h"
#include <QUdpSocket>
#include <QTimer>
#include <QNetworkDatagram>
#include <QDebug>
#include <QNetworkInterface> // Добавлено для работы с сетевыми интерфейсами
#include <QAbstractSocket>

NetworkManager::NetworkManager(const QString &currentUserLogin, QObject *parent)
    : QObject(parent), m_currentUserLogin(currentUserLogin)
{
    udpSocket = new QUdpSocket(this);

    // 1. Поиск подходящего локального IPv4 адреса
    QHostAddress localAddress;
    const QList<QHostAddress> allAddresses = QNetworkInterface::allAddresses();

    for (const QHostAddress &address : allAddresses) {
        // Ищем не-локальный (не 127.0.0.1) IPv4 адрес
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost)) {
            QString ip = address.toString();
            // Предпочитаем адреса из частных сетей (192.168.x.x, 10.x.x.x, 172.16.x.x - 172.31.x.x)
            if (ip.startsWith("192.168.") || ip.startsWith("10.") || ip.startsWith("172.16.")) {
                localAddress = address;
                break; // Нашли подходящий, выходим
            }
        }
    }

    if (localAddress.isNull()) {
        qWarning() << "Could not find a suitable private local IPv4 address. Using Any.";
        localAddress = QHostAddress::AnyIPv4; // Если не нашли частный, берем любой IPv4
    }

    qDebug() << "Binding UDP socket to address:" << localAddress.toString() << "on port" << broadcastPort;

    // 2. Биндим сокет на конкретный IP-адрес и порт
    if (!udpSocket->bind(localAddress, broadcastPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        qWarning() << "Failed to bind UDP socket:" << udpSocket->errorString();
    }


    // 3. Настраиваем обработку полученных датаграмм
    connect(udpSocket, &QUdpSocket::readyRead, this, &NetworkManager::processPendingDatagrams);

    // 4. Настраиваем таймер для периодической отправки "приветствий"
    broadcastTimer = new QTimer(this);
    connect(broadcastTimer, &QTimer::timeout, this, &NetworkManager::sendBroadcast);
    broadcastTimer->start(5000); // Отправляем приветствие каждые 5 секунд

    // 5. Отправляем приветствие сразу при запуске
    sendBroadcast();

    qDebug() << "NetworkManager initialized for user" << m_currentUserLogin;
}

void NetworkManager::sendBroadcast()
{
    // Формируем сообщение. Простой текстовый формат: "DISCOVER:имя_пользователя"
    QByteArray datagram = "DISCOVER:" + m_currentUserLogin.toUtf8();

    // Отправляем на широковещательный адрес
    qDebug() << "Sending broadcast:" << datagram;
    udpSocket->writeDatagram(datagram, QHostAddress::Broadcast, broadcastPort);
}

void NetworkManager::processPendingDatagrams()
{
    // Обрабатываем все пришедшие пакеты в цикле
    while (udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();
        QByteArray data = datagram.data();
        QHostAddress senderAddress = datagram.senderAddress();

        qDebug() << "Received datagram from" << senderAddress.toString() << "with data:" << data;

        // Игнорируем пакеты от самого себя (если система их возвращает)
        if (datagram.senderAddress() == QHostAddress(QHostAddress::LocalHost) || datagram.senderAddress() == udpSocket->localAddress()) {
            continue;
        }

        // Проверяем, что это наше "приветствие"
        if (data.startsWith("DISCOVER:")) {
            QString discoveredUserLogin = QString::fromUtf8(data.mid(9));

            // Не добавляем самого себя в список
            if (discoveredUserLogin == m_currentUserLogin)
                continue;

            // IPv4-адрес может быть в виде "::ffff:192.168.1.5"
            // Нам нужно получить чистый IPv4 адрес
            QString senderIP = senderAddress.toIPv4Address() ? senderAddress.toString().remove(0, 7) : senderAddress.toString();

            // Если мы увидели нового пользователя или у старого изменился IP
            if (!m_discoveredUsers.contains(discoveredUserLogin) || m_discoveredUsers.value(discoveredUserLogin) != senderAddress) {
                qDebug() << "Discovered new user:" << discoveredUserLogin << "at IP" << senderIP;
                m_discoveredUsers[discoveredUserLogin] = senderAddress;

                // Отправляем сигнал, что список пользователей обновился
                emit userListUpdated(m_discoveredUsers.keys());
            }
        }
    }
}
