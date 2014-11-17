#include <QApplication>

#include "ltr_gui.h"
#include <utils.h>
#include <locale.h>
#include <stdlib.h>

 int main(int argc, char *argv[])
 {
     ltr_int_check_root();
     ltr_int_log_message("Starting ltr_gui\n");
     setenv("LC_ALL", "C", 1);
     setlocale(LC_ALL, "C");
     QLocale::setDefault(QLocale::c());
     QApplication app(argc, argv);
     LinuxtrackGui gui;
     gui.show();
     return app.exec();
 }

