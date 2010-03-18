#include <QApplication>

#include "ltr_gui.h"
#include "ltr_show.h"

 int main(int argc, char *argv[])
 {
     QApplication app(argc, argv);
     LtrGuiForm show;
     LinuxtrackGui gui;
     gui.show();
     show.show();
     return app.exec();
 }
