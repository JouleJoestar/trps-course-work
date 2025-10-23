#include "mainwindow.h"
#include "authdialog.h"
#include "database.h"
#include <QApplication>
#include "cryptographymanager.h"
#include <QMessageBox>
#include <QFile>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile styleFile(":/style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        a.setStyleSheet(styleSheet);
        styleFile.close();
    } else {
        qWarning() << "Could not open stylesheet file!";
    }

    Database db;
    if (!db.connect()) {
        QMessageBox::critical(nullptr, "Критическая ошибка", "Не удалось подключиться к базе данных. Приложение будет закрыто.");
        return 1;
    }

    AuthDialog authDialog(&db);
    if (authDialog.exec() == QDialog::Accepted) {
        MainWindow w(&db);
        w.setUserLogin(authDialog.getLogin(), authDialog.getPassword());
        w.show();
        return a.exec();
    }

    return 0;
}
