  #include "mainwindow.h"
#include "authdialog.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    AuthDialog authDialog;
    if (authDialog.exec() == QDialog::Accepted) {
        MainWindow w;
        w.setUserLogin(authDialog.getLogin());
        w.show();
        return a.exec();
    }

    return 0;
}
