#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include "LoginWidget.h"
#include "ChatWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void handleLogin(const QString &username, const QString &password);

private:
    QStackedWidget *stack;
    LoginWidget *loginWidget;
    ChatWidget *chatWidget;
};

#endif // MAINWINDOW_H
