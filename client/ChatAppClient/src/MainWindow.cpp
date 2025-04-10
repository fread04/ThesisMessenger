#include "./include/MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), stack(new QStackedWidget(this)) {

    loginWidget = new LoginWidget(this);
    chatWidget = new ChatWidget(this);

    stack->addWidget(loginWidget);
    stack->addWidget(chatWidget);

    setCentralWidget(stack);
    setWindowTitle("Chat App");

    connect(loginWidget, &LoginWidget::loginSuccess, this, &MainWindow::handleLogin);
}

MainWindow::~MainWindow() {}

void MainWindow::handleLogin(const QString &username, const QString &password) {
    chatWidget->startSession(username, password);
    stack->setCurrentWidget(chatWidget);
}
