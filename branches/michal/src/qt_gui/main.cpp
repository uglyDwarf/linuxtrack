#include <QApplication>

#include "ltr_gui.h"
#include <locale.h>
#include <stdlib.h>

 int main(int argc, char *argv[])
 {
     setenv("LC_ALL", "C", 1);
     setlocale(LC_ALL, "C");
     QLocale::setDefault(QLocale::c());
     QApplication app(argc, argv);
     LinuxtrackGui gui;
     gui.show();
     return app.exec();
 }
