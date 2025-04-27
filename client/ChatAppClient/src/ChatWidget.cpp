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
#include <QRegularExpression>

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

    // Загружаем оба ключа
    loadPrivateKeyFor(username);
    if (!loadPublicKeyFor(username, publicKey)) {
        checkAndGenerateKeys(); // Генерируем новые ключи если не удалось загрузить
    }

    QString host = NetworkConfig::getHost();
    int port = NetworkConfig::getPort();

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

    // Проверяем инициализацию ключей
    if (publicKey.e == 0 || publicKey.n == 0) {
        chatBox->append("[System] Error: Public key not initialized");
        return;
    }

    QString recipient;
    if (plaintext.startsWith("@")) {
        int spaceIndex = plaintext.indexOf(' ');
        if (spaceIndex != -1) {
            recipient = plaintext.mid(1, spaceIndex - 1);
            plaintext = plaintext.mid(spaceIndex + 1);
        } else {
            chatBox->append("[System] Invalid message format. Use @recipient your_message");
            return;
        }
    } else {
        chatBox->append("[System] You must specify a recipient using @username");
        return;
    }

    PublicKey recipientKey;
    if (!loadPublicKeyFor(recipient, recipientKey)) {
        chatBox->append("[System] Failed to load recipient public key for " + recipient);
        return;
    }

    try {
        // Шифруем для себя
        QString encryptedForSelf = CryptoUtils::encrypt(plaintext, publicKey);
        if (encryptedForSelf.isEmpty()) {
            throw std::runtime_error("Self encryption failed");
        }

        // Шифруем для получателя
        QString encryptedForRecipient = CryptoUtils::encrypt(plaintext, recipientKey);
        if (encryptedForRecipient.isEmpty()) {
            throw std::runtime_error("Recipient encryption failed");
        }

        // Собираем сообщение
        QString finalMessage = "@" + recipient + " " + encryptedForSelf + "||" + encryptedForRecipient;

        // Проверяем соединение
        if (socket->state() != QAbstractSocket::ConnectedState) {
            chatBox->append("[System] Connection lost, reconnecting...");
            startSession(username, password);
            return;
        }

        // Отправляем сообщение
        qint64 bytesWritten = socket->write(finalMessage.toUtf8());
        if (bytesWritten == -1) {
            throw std::runtime_error("Failed to write to socket");
        }

        if (!socket->flush()) {
            throw std::runtime_error("Failed to flush socket");
        }

        //chatBox->append("You (to " + recipient + "): " + plaintext);
        messageInput->clear();

    } catch (const std::exception& e) {
        chatBox->append("[System] Error: " + QString(e.what()));
        qCritical() << "Message sending error:" << e.what();
    }
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

        QString meta = line.left(colonIndex);
        QString encryptedBody = line.mid(colonIndex + 2);

        qDebug() << "Message meta: " << meta;
        qDebug() << "Encrypted body: " << encryptedBody;

        QString decrypted;

        if (encryptedBody.contains("||")) {
            qDebug() << "Message contains '||', splitting into two parts.";
            QStringList parts = encryptedBody.split("||");

            // Определяем тип сообщения (история или реальное время)
            bool isHistoryMessage = meta.contains("|");

            if (isHistoryMessage) {
                // Обработка исторических сообщений
                QRegularExpression re("\\| (\\w+)\\(\\d+\\) -> (\\w+)\\(\\d+\\)");
                QRegularExpressionMatch match = re.match(meta);

                if (match.hasMatch()) {
                    QString sender = match.captured(1);
                    QString recipient = match.captured(2);

                    if (recipient == username) {
                        // Мы получатели - дешифруем вторую часть
                        decrypted = CryptoUtils::decrypt(parts[1], privateKey);
                        qDebug() << "Decrypted historical message (as recipient):" << decrypted;
                    } else if (sender == username) {
                        // Мы отправители - дешифруем первую часть
                        decrypted = CryptoUtils::decrypt(parts[0], privateKey);
                        qDebug() << "Decrypted historical message (as sender):" << decrypted;
                    }
                }
            } else {
                // Обработка сообщений в реальном времени
                bool isOutgoing = meta.startsWith("(private) " + username + ":");
                if (isOutgoing) {
                    decrypted = CryptoUtils::decrypt(parts[0], privateKey);
                    qDebug() << "Decrypted real-time message (as sender):" << decrypted;
                } else {
                    decrypted = CryptoUtils::decrypt(parts[1], privateKey);
                    qDebug() << "Decrypted real-time message (as recipient):" << decrypted;
                }
            }
        } else {
            qDebug() << "Message does not contain '||', attempting to decrypt as a single message.";
            decrypted = CryptoUtils::decrypt(encryptedBody, privateKey);
        }

        // Проверка результата дешифровки
        if (decrypted.isEmpty()) {
            chatBox->append(QString("<b>%1:</b> <i>Failed to decrypt message</i>").arg(meta));
        } else {
            QRegularExpression reg("[^\\x20-\\x7E]");
            if (decrypted.contains(reg)) {
                chatBox->append(QString("<b>%1:</b> <i>Invalid decrypted message format</i>").arg(meta));
            } else {
                chatBox->append(QString("<b>%1:</b> %2").arg(meta, decrypted));
            }
        }
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
