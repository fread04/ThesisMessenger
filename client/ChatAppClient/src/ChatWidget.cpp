#include "./include/ChatWidget.h"
#include "./include/NetworkConfig.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QHostAddress>

ChatWidget::ChatWidget(QWidget *parent) : QWidget(parent), socket(new QTcpSocket(this)) {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    chatBox = new QTextEdit(this);
    chatBox->setReadOnly(true);

    messageInput = new QLineEdit(this);
    QPushButton *sendButton = new QPushButton("Send", this);

    QHBoxLayout *inputLayout = new QHBoxLayout;
    inputLayout->addWidget(messageInput);
    inputLayout->addWidget(sendButton);

    mainLayout->addWidget(chatBox);
    mainLayout->addLayout(inputLayout);

    connect(sendButton, &QPushButton::clicked, this, &ChatWidget::sendMessage);
    connect(messageInput, &QLineEdit::returnPressed, this, &ChatWidget::sendMessage);
    connect(socket, &QTcpSocket::readyRead, this, &ChatWidget::receiveMessage);
    connect(socket, &QTcpSocket::disconnected, this, &ChatWidget::onDisconnected);
}

void ChatWidget::startSession(const QString &user, const QString &pass) {
    QString serverHost = NetworkConfig::getHost();
    int serverPort = NetworkConfig::getPort();

    username = user;
    password = pass;

    socket->connectToHost(QHostAddress(serverHost), serverPort);
    if (!socket->waitForConnected(3000)) {
        QMessageBox::critical(this, "Connection Failed", "Could not connect to server.");
        return;
    }

    QString credentials = username + ":" + password;
    socket->write(credentials.toUtf8());
}

void ChatWidget::sendMessage() {
    QString message = messageInput->text().trimmed();
    if (!message.isEmpty()) {
        socket->write(message.toUtf8());
        chatBox->append("You: " + message);
        messageInput->clear();
    }
}

void ChatWidget::receiveMessage() {
    while (socket->bytesAvailable()) {
        QString msg = QString::fromUtf8(socket->readAll());
        chatBox->append(msg);
    }
}

void ChatWidget::onDisconnected() {
    chatBox->append("<i>Disconnected from server</i>");
}
