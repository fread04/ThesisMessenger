#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

#include <QString>
#include <QSettings>

class NetworkConfig {
public:
    static QString getConfigPath();
    static QString getHost();
    static int getPort();
};

#endif // NETWORK_CONFIG_H
