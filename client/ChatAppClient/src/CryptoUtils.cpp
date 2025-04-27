#include "../include/CryptoUtils.h"

#include <QtMath>
#include <QRandomGenerator>
#include <QStringList>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDebug>
#include <chrono>

// ✅ Miller–Rabin Test with Logs
bool CryptoUtils::isPrime(long long n, int k) {
    if (n < 2) return false;
    if (n != 2 && n % 2 == 0) return false;

    long long d = n - 1;
    int s = 0;
    while (d % 2 == 0) {
        d /= 2;
        s++;
    }

    QRandomGenerator* rng = QRandomGenerator::global();
    for (int i = 0; i < k; ++i) {
        long long a = rng->bounded(2LL, n - 1);
        long long x = CryptoUtils::modPow(a, d, n);

        if (x == 1 || x == n - 1) continue;

        bool passed = false;
        for (int r = 1; r < s; ++r) {
            x = CryptoUtils::modPow(x, 2, n);
            if (x == n - 1) {
                passed = true;
                break;
            }
        }

        if (!passed) {
            qDebug() << "Not prime:" << n << " (failed base:" << a << ")";
            return false;
        }
    }

    qDebug() << "Probably prime:" << n;
    return true;
}

long long CryptoUtils::gcd(long long a, long long b) {
    while (b != 0) {
        long long temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

long long CryptoUtils::modPow(long long base, long long exp, long long mod) {
    long long result = 1;
    base = base % mod;
    while (exp > 0) {
        if (exp % 2 == 1)
            result = (result * base) % mod;
        exp = exp >> 1;
        base = (base * base) % mod;
    }
    return result;
}

long long CryptoUtils::modInverse(long long e, long long phi) {
    long long t = 0, newT = 1;
    long long r = phi, newR = e;

    while (newR != 0) {
        long long quotient = r / newR;

        long long tempT = t;
        t = newT;
        newT = tempT - quotient * newT;

        long long tempR = r;
        r = newR;
        newR = tempR - quotient * newR;
    }

    if (r > 1) return -1; // e и phi не взаимно просты

    if (t < 0)
        t += phi;

    return t;
}


long long CryptoUtils::generatePrime(long long min, long long max) {
    long long prime;
    int maxAttempts = 10000;
    int attempts = 0;
    do {
        prime = QRandomGenerator::global()->bounded(min, max);
        attempts++;
        if (attempts > maxAttempts) {
            qDebug() << "Error: failed to generate prime after maxAttempts";
            return -1;
        }
    } while (!isPrime(prime, 5));
    return prime;
}


void CryptoUtils::generateKeys(PublicKey &pub, PrivateKey &priv) {
    qDebug() << "Start generate keys";

    long long p = generatePrime(100, 1000);
    long long q = generatePrime(100, 1000);
    while (q == p) {
        qDebug() << "q equals p, regenerating q";
        q = generatePrime(100, 1000);
    }

    qDebug() << "p:" << p << "q:" << q;

    long long n = p * q;
    long long phi = (p - 1) * (q - 1);
    qDebug() << "n:" << n << "phi:" << phi;

    long long possible_e[] = {3, 5, 17, 257, 65537};
    int num_possible_e = sizeof(possible_e) / sizeof(possible_e[0]);
    long long e = 0;
    for (int i = 0; i < num_possible_e; ++i) {
        if (gcd(possible_e[i], phi) == 1) {
            e = possible_e[i];
            break;
        }
    }

    qDebug() << "Found suitable e:" << e;

    if (e == 0) {
        qDebug() << "Error: cant find e!";
        return;
    }

    qDebug() << "Found suitable e:" << e;

    long long d = modInverse(e, phi);
    qDebug() << "e:" << e << "d:" << d;

    pub = {e, n};
    priv = {d, n};

    qDebug() << "End generate keys";
}

QString CryptoUtils::encrypt(const QString &message, const PublicKey &pub) {
    QString ciphertext = "";
    for (QChar c : message) {
        long long m = c.unicode();
        long long encrypted_m = modPow(m, pub.e, pub.n);
        ciphertext += QString::number(encrypted_m) + " ";
    }
    return ciphertext.trimmed();
}

QString CryptoUtils::decrypt(const QString &ciphertext, const PrivateKey &priv) {
    QStringList encrypted_blocks = ciphertext.split(" ");
    QString decrypted_message = "";
    for (const QString& block : encrypted_blocks) {
        long long encrypted_m = block.toLongLong();
        long long decrypted_m = modPow(encrypted_m, priv.d, priv.n);
        decrypted_message += QChar(static_cast<uint>(decrypted_m));
    }
    return decrypted_message;
}
