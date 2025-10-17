// В файле networkmanager.cpp

void NetworkManager::processPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();
        QByteArray data = datagram.data();
        QHostAddress senderAddress = datagram.senderAddress();

        // Мы больше не будем здесь выводить [RAW], чтобы не засорять лог.

        if (data.startsWith("DISCOVER:")) {
            QString discoveredUserLogin = QString::fromUtf8(data.mid(9));
            if (discoveredUserLogin == m_currentUserLogin)
                continue;

            QHostAddress cleanAddress(senderAddress.toIPv4Address());

            // --- НАЧАЛО НОВОЙ ЛОГИКИ "ОТВЕТНОГО ПАКЕТА" ---

            // Если мы впервые видим этого пользователя,
            // отправляем ему свой "DISCOVER" пакет в ответ напрямую.
            if (!m_discoveredUsers.contains(discoveredUserLogin)) {
                qDebug() << "Discovered NEW user:" << discoveredUserLogin << "at" << cleanAddress.toString();
                qDebug() << "Sending direct discovery response to" << cleanAddress.toString();

                QByteArray responseDatagram = "DISCOVER:" + m_currentUserLogin.toUtf8();
                udpSocket->writeDatagram(responseDatagram, cleanAddress, broadcastPort);
            }

            // Теперь обычная логика добавления/обновления
            if (!m_discoveredUsers.contains(discoveredUserLogin) || m_discoveredUsers.value(discoveredUserLogin) != cleanAddress) {
                m_discoveredUsers[discoveredUserLogin] = cleanAddress;
                emit userListUpdated(m_discoveredUsers.keys());
            }
            // --- КОНЕЦ НОВОЙ ЛОГИКИ ---
        }
    }
}
