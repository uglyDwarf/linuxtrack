#include <QApplication>
#include <QLocale>
#include <utils.h>
#include <locale.h>
#include <stdlib.h>

#include "mickey.h"

 int main(int argc, char *argv[])
 {
     ltr_int_log_message("Starting Mickey controller!\n");
     setenv("LC_ALL", "C", 1);
     setlocale(LC_ALL, "C");
     QLocale::setDefault(QLocale::c());
     QApplication app(argc, argv);
     GUI.show();
     int res = app.exec();
     MickeyGUI::deleteInstance();
     return res;
 }
