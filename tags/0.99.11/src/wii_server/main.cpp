#include <QApplication>
#include <QLocale>
#include <utils.h>
#include <locale.h>
#include <stdlib.h>

#include "wii_server.h"

 int main(int argc, char *argv[])
 {
     ltr_int_log_message("Starting Wiimote server!\n");
     setenv("LC_ALL", "C", 1);
     setlocale(LC_ALL, "C");
     QLocale::setDefault(QLocale::c());
     QApplication app(argc, argv);
     WiiServerWindow win;
     win.show();
     return app.exec();
 }

