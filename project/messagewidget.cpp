#include "messagewidget.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QResizeEvent>

MessageWidget::MessageWidget(const QString &sender, const QString &text, const QString &time, bool isMyMessage, QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout* mainLayout = new QHBoxLayout(this);

    QWidget* bubble = new QWidget(this);
    bubble->setObjectName("bubble");

    QVBoxLayout* bubbleLayout = new QVBoxLayout(bubble);
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

    if (isMyMessage) {
        bubble->setStyleSheet("QWidget#bubble { background-color: #2b5278; border-radius: 10px; }");
        mainLayout->addStretch();
        mainLayout->addWidget(bubble);
    } else {
        bubble->setStyleSheet("QWidget#bubble { background-color: #334e6d; border-radius: 10px; }");
        mainLayout->addWidget(bubble);
        mainLayout->addStretch();
        if (sender != "Общий чат") senderLabel->setVisible(false);
    }
}

void MessageWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    this->adjustSize();
}

