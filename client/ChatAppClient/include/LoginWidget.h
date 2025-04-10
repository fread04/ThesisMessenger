#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

class LoginWidget : public QWidget {
    Q_OBJECT

public:
    explicit LoginWidget(QWidget *parent = nullptr);

signals:
    void loginSuccess(const QString &username, const QString &password);

private slots:
    void attemptLogin();

private:
    QLineEdit *usernameInput;
    QLineEdit *passwordInput;
};

#endif // LOGINWIDGET_H
