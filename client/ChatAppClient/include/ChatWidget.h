#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>
#include <QTcpSocket>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include "CryptoUtils.h" // Подключаем CryptoUtils

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
    PublicKey publicKey; // Добавляем открытый ключ
    PrivateKey privateKey; // Добавляем закрытый ключ

    QString getConfigDirPath() const; // Метод для получения пути к конфигурационной директории
    QString getPublicKeyFilePath(const QString &username) const; // Измененный метод для получения пути к публичному ключу
    QString getPrivateKeyFilePath(const QString &username) const; // Измененный метод для получения пути к приватному ключу
    bool loadPublicKeyFor(const QString &user, PublicKey &outKey);
    void loadPrivateKeyFor(const QString &user);
    void savePublicKey(const PublicKey& key, const QString& path); // Метод для сохранения публичного ключа
    void savePrivateKey(const PrivateKey& key, const QString& path); // Метод для сохранения приватного ключа
    QString getPublicKeyAsString(const PublicKey& key); // Метод для получения публичного ключа как строки
    void checkAndGenerateKeys(); // Проверка и генерация ключей
    void loadKeys(const QString &username); // Метод для загрузки ключей конкретного пользователя

    bool isKeyGenerated = false; // Флаг, который указывает, что ключи были сгенерированы
};

#endif // CHATWIDGET_H
