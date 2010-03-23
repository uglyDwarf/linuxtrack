#include <QApplication>

#include "ltr_gui.h"

 int main(int argc, char *argv[])
 {
     QApplication app(argc, argv);
     LinuxtrackGui gui;
     gui.show();
     return app.exec();
 }
