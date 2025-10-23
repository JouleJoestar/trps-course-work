#ifndef MESSAGEWIDGET_H
#define MESSAGEWIDGET_H

#include <QWidget>

class QLabel;

class MessageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MessageWidget(const QString& sender, const QString& text, const QString& time, bool isMyMessage, QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;
};

#endif // MESSAGEWIDGET_H
