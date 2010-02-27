/********************************************************************************
** Form generated from reading ui file 'ltr_gui.ui'
**
** Created: Sun Feb 21 14:06:40 2010
**      by: Qt User Interface Compiler version 4.5.0
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef UI_LTR_GUI_H
#define UI_LTR_GUI_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QMainWindow>
#include <QtGui/QMenuBar>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QStatusBar>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Ltr_gui
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout_2;
    QVBoxLayout *pix_box;
    QSpacerItem *verticalSpacer;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QPushButton *startButton;
    QPushButton *pauseButton;
    QPushButton *wakeButton;
    QPushButton *stopButton;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *Ltr_gui)
    {
        if (Ltr_gui->objectName().isEmpty())
            Ltr_gui->setObjectName(QString::fromUtf8("Ltr_gui"));
        Ltr_gui->resize(800, 600);
        centralwidget = new QWidget(Ltr_gui);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        verticalLayout_2 = new QVBoxLayout(centralwidget);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        pix_box = new QVBoxLayout();
        pix_box->setObjectName(QString::fromUtf8("pix_box"));

        verticalLayout_2->addLayout(pix_box);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setSizeConstraint(QLayout::SetMaximumSize);
        startButton = new QPushButton(centralwidget);
        startButton->setObjectName(QString::fromUtf8("startButton"));
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(startButton->sizePolicy().hasHeightForWidth());
        startButton->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(startButton);

        pauseButton = new QPushButton(centralwidget);
        pauseButton->setObjectName(QString::fromUtf8("pauseButton"));
        sizePolicy.setHeightForWidth(pauseButton->sizePolicy().hasHeightForWidth());
        pauseButton->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(pauseButton);

        wakeButton = new QPushButton(centralwidget);
        wakeButton->setObjectName(QString::fromUtf8("wakeButton"));
        sizePolicy.setHeightForWidth(wakeButton->sizePolicy().hasHeightForWidth());
        wakeButton->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(wakeButton);

        stopButton = new QPushButton(centralwidget);
        stopButton->setObjectName(QString::fromUtf8("stopButton"));
        sizePolicy.setHeightForWidth(stopButton->sizePolicy().hasHeightForWidth());
        stopButton->setSizePolicy(sizePolicy);
        stopButton->setMaximumSize(QSize(16777215, 16777215));

        horizontalLayout->addWidget(stopButton);


        verticalLayout->addLayout(horizontalLayout);


        verticalLayout_2->addLayout(verticalLayout);

        Ltr_gui->setCentralWidget(centralwidget);
        menubar = new QMenuBar(Ltr_gui);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 800, 25));
        Ltr_gui->setMenuBar(menubar);
        statusbar = new QStatusBar(Ltr_gui);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        Ltr_gui->setStatusBar(statusbar);

        retranslateUi(Ltr_gui);

        QMetaObject::connectSlotsByName(Ltr_gui);
    } // setupUi

    void retranslateUi(QMainWindow *Ltr_gui)
    {
        Ltr_gui->setWindowTitle(QApplication::translate("Ltr_gui", "MainWindow", 0, QApplication::UnicodeUTF8));
        startButton->setText(QApplication::translate("Ltr_gui", "Start", 0, QApplication::UnicodeUTF8));
        pauseButton->setText(QApplication::translate("Ltr_gui", "Pause", 0, QApplication::UnicodeUTF8));
        wakeButton->setText(QApplication::translate("Ltr_gui", "Resume", 0, QApplication::UnicodeUTF8));
        stopButton->setText(QApplication::translate("Ltr_gui", "Stop", 0, QApplication::UnicodeUTF8));
        Q_UNUSED(Ltr_gui);
    } // retranslateUi

};

namespace Ui {
    class Ltr_gui: public Ui_Ltr_gui {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LTR_GUI_H
