#ifndef CRYPTOUTILS_H
#define CRYPTOUTILS_H

#include <QString>

struct PublicKey {
    long long e;
    long long n;
};

struct PrivateKey {
    long long d;
    long long n;
};

class CryptoUtils {
public:
    static void generateKeys(PublicKey &pub, PrivateKey &priv);
    static QString encrypt(const QString &message, const PublicKey &pub);
    static QString decrypt(const QString &ciphertext, const PrivateKey &priv);
private:
    static bool isPrime(long long n, int k);
    static long long gcd(long long a, long long b);
    static long long modPow(long long base, long long exp, long long mod);
    static long long modInverse(long long e, long long phi);
    static long long generatePrime(long long min, long long max);
};

#endif // CRYPTOUTILS_H
