#include "mainwindow.h"
#include "authdialog.h"
#include "database.h"
#include <QApplication>
#include "cryptographymanager.h"
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    CryptographyManager::init();

    Database db;
    if (!db.connect()) {
        QMessageBox::critical(nullptr, "Критическая ошибка", "Не удалось подключиться к базе данных. Приложение будет закрыто.");
        return 1;
    }

    AuthDialog authDialog(&db);
    if (authDialog.exec() == QDialog::Accepted) {
        MainWindow w(&db);
        w.setUserLogin(authDialog.getLogin());
        w.show();
        return a.exec();
    }

    return 0;
}
