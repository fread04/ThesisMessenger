#include "./include/ChatWidget.h"
#include "./include/NetworkConfig.h"
#include "./include/CryptoUtils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QHostAddress>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QTextStream>
#include <QDebug>

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
    username = user;
    password = pass;

    QString host = NetworkConfig::getHost();
    int port = NetworkConfig::getPort();

    loadPrivateKeyFor(username); // загрузим сразу свой приватный ключ
    socket->connectToHost(QHostAddress(host), port);

    if (!socket->waitForConnected(3000)) {
        QMessageBox::critical(this, "Connection Failed", "Could not connect to server.");
        return;
    }

    QString credentials = username + ":" + password;
    socket->write(credentials.toUtf8());
}

void ChatWidget::sendMessage() {
    QString plaintext = messageInput->text();
    if (plaintext.isEmpty()) return;

    QString recipient; // Куда мы будем отправлять
    // Предположим, что у тебя в сообщении явно указан получатель через @имя
    if (plaintext.startsWith("@")) {
        int spaceIndex = plaintext.indexOf(' ');
        if (spaceIndex != -1) {
            recipient = plaintext.mid(1, spaceIndex - 1); // вырезаем имя без @
            plaintext = plaintext.mid(spaceIndex + 1); // оставляем только сам текст
        } else {
            // Неверный формат
            chatBox->append("[System] Invalid message format. Use @recipient your_message");
            return;
        }
    } else {
        chatBox->append("[System] You must specify a recipient using @username");
        return;
    }

    // Загружаем публичный ключ получателя
    PublicKey recipientKey;
    if (!loadPublicKeyFor(recipient, recipientKey)) {
        chatBox->append("[System] Failed to load recipient public key.");
        return;
    }

    // Шифруем для отправителя (для себя)
    QString encryptedForSelf = CryptoUtils::encrypt(plaintext, publicKey);

    // Шифруем для получателя
    QString encryptedForRecipient = CryptoUtils::encrypt(plaintext, recipientKey);

    // Собираем сообщение
    QString finalMessage = "@" + recipient + " " + encryptedForSelf + "|" + encryptedForRecipient;

    // Отправляем
    socket->write(finalMessage.toUtf8());
    socket->flush();

    // Печатаем у себя в чате
    chatBox->append("You (to " + recipient + "): " + plaintext);

    messageInput->clear();
}


void ChatWidget::receiveMessage() {
    QString rawData = QString::fromUtf8(socket->readAll());
    qDebug() << "Raw message received:" << rawData;

    QStringList lines = rawData.split('\n', Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        int colonIndex = line.indexOf(": ");
        if (colonIndex == -1) {
            chatBox->append("<i>Invalid message format</i>");
            continue;
        }

        QString meta = line.left(colonIndex);       // дата и отправитель
        QString encryptedBody = line.mid(colonIndex + 2);

        QString decrypted;

        if (encryptedBody.contains("|")) {
            QStringList parts = encryptedBody.split("|");
            if (parts.size() == 2) {
                decrypted = CryptoUtils::decrypt(parts[0], privateKey);
                if (decrypted.isEmpty()) {
                    decrypted = CryptoUtils::decrypt(parts[1], privateKey);
                }
            } else {
                decrypted = CryptoUtils::decrypt(encryptedBody, privateKey);
            }
        } else {
            decrypted = CryptoUtils::decrypt(encryptedBody, privateKey);
        }

        chatBox->append(QString("<b>%1:</b> %2").arg(meta, decrypted));
    }
}




void ChatWidget::onDisconnected() {
    chatBox->append("<i>Disconnected from server</i>");
}

// --- Key management ---

QString ChatWidget::getConfigDirPath() const {
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString appDirPath = configPath + "/chatApp";
    QDir dir(appDirPath);
    if (!dir.exists()) dir.mkpath(".");
    return appDirPath;
}

QString ChatWidget::getPublicKeyFilePath(const QString &user) const {
    return getConfigDirPath() + "/" + user + "_public_key.txt";
}

QString ChatWidget::getPrivateKeyFilePath(const QString &user) const {
    return getConfigDirPath() + "/" + user + "_private_key.txt";
}

void ChatWidget::savePublicKey(const PublicKey& key, const QString& user) {
    QFile file(getPublicKeyFilePath(user));
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << key.e << " " << key.n;
        file.close();
    }
}

void ChatWidget::savePrivateKey(const PrivateKey& key, const QString& user) {
    QFile file(getPrivateKeyFilePath(user));
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << key.d << " " << key.n;
        file.close();
    }
}

void ChatWidget::checkAndGenerateKeys() {
    CryptoUtils::generateKeys(publicKey, privateKey);
    savePublicKey(publicKey, username);
    savePrivateKey(privateKey, username);
}

void ChatWidget::loadPrivateKeyFor(const QString &user) {
    QFile file(getPrivateKeyFilePath(user));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        in >> privateKey.d >> privateKey.n;
        file.close();
    } else {
        qDebug() << "Private key not found, generating new...";
        checkAndGenerateKeys();
    }
}

bool ChatWidget::loadPublicKeyFor(const QString &user, PublicKey &outKey) {
    QFile file(getPublicKeyFilePath(user));
    if (!file.exists()) return false;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        in >> outKey.e >> outKey.n;
        file.close();
        return true;
    }
    return false;
}
