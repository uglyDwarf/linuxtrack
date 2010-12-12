#include <QApplication>

#include "ltr_gui.h"
#include <locale.h>

 int main(int argc, char *argv[])
 {
     setlocale(LC_ALL, "C");
     QLocale::setDefault(QLocale::c());
     QApplication app(argc, argv);
     LinuxtrackGui gui;
     gui.show();
     return app.exec();
 }
