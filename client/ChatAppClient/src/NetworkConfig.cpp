#include "./include/NetworkConfig.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

QString NetworkConfig::getConfigPath() {
    // Path to current file
    QFileInfo fileInfo(__FILE__);
    QString srcDir = fileInfo.absoluteDir().path();

    QDir projectDir(srcDir);
    projectDir.cdUp();

    QString configPath = projectDir.absoluteFilePath(".config/client_config.ini");
    return configPath;
}

QString NetworkConfig::getHost() {
    QString configPath = getConfigPath();
    qDebug() << "Config path:" << configPath;

    QSettings settings(configPath, QSettings::IniFormat);
    QString host = settings.value("Server/host", "default").toString().trimmed();
    qDebug() << "Host:" << host;
    return host;
}

int NetworkConfig::getPort() {
    QString configPath = getConfigPath();
    QSettings settings(configPath, QSettings::IniFormat);
    int port = settings.value("Server/port", 0).toInt();
    qDebug() << "Port:" << port;
    return port;
}
