 #include <QApplication>

 #include "ltr_gui.h"

 int main(int argc, char *argv[])
 {
     QApplication app(argc, argv);
     LtrGuiForm gui;
     gui.show();
     return app.exec();
 }
