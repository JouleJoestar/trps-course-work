#include "messagewidget.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QDebug>

MessageWidget::MessageWidget(const QString &sender, const QString &text, const QString &time, bool isMyMessage, bool isGeneralChat, QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout* mainLayout = new QHBoxLayout(this);

    QWidget* bubble = new QWidget(this);
    bubble->setObjectName("bubble");

    QHBoxLayout* bubbleLayout = new QHBoxLayout(bubble);
    bubbleLayout->setContentsMargins(10, 5, 10, 5);

    QLabel* senderLabel = new QLabel(sender, this);
    senderLabel->setObjectName("senderLabel");
    senderLabel->setStyleSheet("font-weight: bold; color: #63a5ee;");

    QLabel* textLabel = new QLabel(text, this);
    textLabel->setWordWrap(true);
    textLabel->setObjectName("textLabel");

    QLabel* timeLabel = new QLabel(time, this);
    timeLabel->setObjectName("timeLabel");
    timeLabel->setStyleSheet("color: #8e99a4; font-size: 11px;");

    bubbleLayout->addWidget(senderLabel);
    bubbleLayout->addWidget(textLabel);
    bubbleLayout->addWidget(timeLabel);
    bubbleLayout->setAlignment(timeLabel, Qt::AlignRight);

    if (isGeneralChat) {
        senderLabel->setVisible(true);
    } else {
        senderLabel->setVisible(false);
    }
    // Видимость имени отправителя
    if (isGeneralChat) {
        senderLabel->setVisible(true);
    } else {
        senderLabel->setVisible(false);
    }

    // --- ИСПРАВЛЕНИЕ И ОТЛАДКА ---
    // Давайте сделаем логику выравнивания абсолютно явной.
    if (isMyMessage) {
        qDebug() << "Creating MY message bubble for sender:" << sender;
        // Это мое сообщение, оно должно быть СПРАВА
        bubble->setStyleSheet("QWidget#bubble { background-color: #2b5278; border-radius: 10px; }");
        mainLayout->addStretch(1); // Добавляем растягивающийся элемент слева
        mainLayout->addWidget(bubble, 0); // Добавляем виджет справа
    } else {
        qDebug() << "Creating INCOMING message bubble for sender:" << sender;
        // Это входящее сообщение, оно должно быть СЛЕВА
        bubble->setStyleSheet("QWidget#bubble { background-color: #334e6d; border-radius: 10px; }");
        mainLayout->addWidget(bubble, 0); // Добавляем виджет слева
        mainLayout->addStretch(1); // Добавляем растягивающийся элемент справа
    }
}
void MessageWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    this->adjustSize();
}
