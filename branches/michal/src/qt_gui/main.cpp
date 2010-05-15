#include <QApplication>

#include "ltr_gui.h"
#include <locale.h>

 int main(int argc, char *argv[])
 {
     QApplication app(argc, argv);
     setlocale(LC_NUMERIC,"C");
     LinuxtrackGui gui;
     gui.show();
     return app.exec();
 }
