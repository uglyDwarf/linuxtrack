#include <QApplication>

#include "ltr_gui.h"
#include "ltr_show.h"
#include "ltr_dev_help.h"

 int main(int argc, char *argv[])
 {
     QApplication app(argc, argv);
     LtrGuiForm show;
     LinuxtrackGui gui;
     
     LtrDevHelp helper;
     helper.show();
     
     gui.show();
     show.show();
     return app.exec();
 }
