#ifdef HAVE_CONFIG_H
  #include "../../config.h"
#endif

#include <QSettings>
#include "ltr_gui_prefs.h"
#include "help_view.h"
#include "iostream"

HelpViewer *HelpViewer::hlp = NULL;

HelpViewer &HelpViewer::getHlp()
{
  if(hlp == NULL){
    hlp = new HelpViewer();
  }
  return *hlp;
}

void HelpViewer::ShowWindow()
{
  getHlp().show();
}

void HelpViewer::ChangePage(QString name)
{
  getHlp().ChangeHelpPage(name);
}

void HelpViewer::ChangeHelpPage(QString name)
{
  QString tmp = QString("file://") + PREF.getDataPath(QString("/help/") + name);
  //std::cout<<tmp.toAscii().data()<<std::endl;
  viewer->load(QUrl(tmp));
}

void HelpViewer::CloseWindow()
{
  getHlp().close();
}

void HelpViewer::LoadPrefs(QSettings &settings)
{
  HelpViewer &hv = getHlp();
  settings.beginGroup("HelpWindow");
  hv.resize(settings.value("size", QSize(800, 600)).toSize());
  hv.move(settings.value("pos", QPoint(0, 0)).toPoint());
  settings.endGroup();
}

void HelpViewer::StorePrefs(QSettings &settings)
{
  HelpViewer &hv = getHlp();
  settings.beginGroup("HelpWindow");
  settings.setValue("size", hv.size());
  settings.setValue("pos", hv.pos());
  settings.endGroup();
}

HelpViewer::HelpViewer(QWidget *parent) : QWidget(parent)
{
  ui.setupUi(this);
  setWindowTitle("Help viewer");
  viewer = new QWebView(this);
  ui.verticalLayout->insertWidget(0, viewer);
  ChangeHelpPage("help.htm");
  QObject::connect(viewer, SIGNAL(linkClicked(const QUrl&)), this, SLOT(followLink(const QUrl&)));
  viewer->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
}

void HelpViewer::on_CloseButton_pressed()
{
  close();
}

void HelpViewer::followLink(const QUrl &url)
{
  viewer->load(url);
  std::cout<<"Following link!"<<std::endl;
}

HelpViewer::~HelpViewer()
{
  ui.verticalLayout->removeWidget(viewer);
  delete(viewer);
}

