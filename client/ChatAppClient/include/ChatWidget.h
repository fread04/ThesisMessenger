#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>
#include <QTcpSocket>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>

class ChatWidget : public QWidget {
    Q_OBJECT

public:
    explicit ChatWidget(QWidget *parent = nullptr);
    void startSession(const QString &username, const QString &password);

private slots:
    void sendMessage();
    void receiveMessage();
    void onDisconnected();

private:
    QTcpSocket *socket;
    QTextEdit *chatBox;
    QLineEdit *messageInput;
    QString username;
    QString password;
};

#endif // CHATWIDGET_H
