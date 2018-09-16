#ifdef HAVE_CONFIG_H
  #include "../../config.h"
#endif

#include <QSettings>
#include <QRegExp>
#include <QDesktopServices>
#include <QHelpEngine>
#include <QHelpContentWidget>
#include <QSplitter>

#include "ltr_gui_prefs.h"
#include "help_view.h"
#include "help_viewer.h"
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

void HelpViewer::RaiseWindow()
{
  getHlp().raise();
  getHlp().activateWindow();
}

void HelpViewer::ChangePage(QString name)
{
  getHlp().ChangeHelpPage(name);
}

void HelpViewer::ChangeHelpPage(QString name)
{
  QString tmp = QString::fromUtf8("qthelp://uglyDwarf.com.linuxtrack.1.0/doc/help/") + name;
  viewer->setSource(QUrl(tmp));
}

void HelpViewer::CloseWindow()
{
  getHlp().close();
}

void HelpViewer::LoadPrefs(QSettings &settings)
{
  HelpViewer &hv = getHlp();
  settings.beginGroup(QString::fromUtf8("HelpWindow"));
  hv.resize(settings.value(QString::fromUtf8("size"), QSize(800, 600)).toSize());
  hv.move(settings.value(QString::fromUtf8("pos"), QPoint(0, 0)).toPoint());
  settings.endGroup();
}

void HelpViewer::StorePrefs(QSettings &settings)
{
  HelpViewer &hv = getHlp();
  settings.beginGroup(QString::fromUtf8("HelpWindow"));
  settings.setValue(QString::fromUtf8("size"), hv.size());
  settings.setValue(QString::fromUtf8("pos"), hv.pos());
  settings.endGroup();
}

HelpViewer::HelpViewer(QWidget *parent) : QWidget(parent)
{
  ui.setupUi(this);
  setWindowTitle(QString::fromUtf8("Help viewer"));

  QString helpFile = PREF.getDataPath(QString::fromUtf8("/help/") +
                     QString::fromUtf8(HELP_BASE) + QString::fromUtf8("/help.qhc"));
  helpEngine = new QHelpEngine(helpFile);
  helpEngine->setupData();
  contents = helpEngine->contentWidget();
  splitter = new QSplitter();
  ui.horizontalLayout->addWidget((QWidget*)splitter);

  viewer = new HelpViewWidget(helpEngine, this);
  layout = new QHBoxLayout();
  splitter = new QSplitter();
  layout->addWidget(splitter);
  splitter->addWidget(contents);
  splitter->addWidget(viewer);
  ui.verticalLayout->insertLayout(0, layout);
  QObject::connect(contents, SIGNAL(linkActivated(const QUrl &)), viewer, SLOT(setSource(const QUrl &)));
  QObject::connect(contents, SIGNAL(clicked(const QModelIndex &)), this, SLOT(itemClicked(const QModelIndex &)));
  QObject::connect(helpEngine->contentModel(), SIGNAL(contentsCreated()), this, SLOT(helpInitialized()));
  QObject::connect(viewer, SIGNAL(linkClicked(const QUrl&)), this, SLOT(followLink(const QUrl&)));
  //viewer->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
  splitter->setStretchFactor(1, 4);
}

HelpViewer::~HelpViewer()
{
  ui.verticalLayout->removeItem(layout);
  layout->removeWidget(contents);
  layout->removeWidget(viewer);
  delete(layout);
  delete(contents);
  delete(viewer);
}

void HelpViewer::itemClicked(const QModelIndex &index)
{
  const QHelpContentItem *ci = helpEngine->contentModel()->contentItemAt(index);
  if(ci){
    viewer->setSource(ci->url());
  }
}

void HelpViewer::helpInitialized()
{
  contents->expandAll();
}

void HelpViewer::on_CloseButton_pressed()
{
  close();
}


void HelpViewer::followLink(const QUrl &url)
{
  if(QString::fromUtf8("http").compare(url.scheme(), Qt::CaseInsensitive) == 0){
    QDesktopServices::openUrl(url);
  }else{
    viewer->setSource(url);
  }
}



