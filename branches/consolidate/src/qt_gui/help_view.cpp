#ifdef HAVE_CONFIG_H
  #include "../../config.h"
#endif

#include <QSettings>
#include <QRegExp>
#include <QDesktopServices>

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
  QString tmp = QString("file://") + PREF.getDataPath(QString("/help/") + QString(HELP_BASE) + name);
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

HelpViewer::HelpViewer(QWidget *parent) : QWidget(parent), contents(NULL), layout(NULL)
{
  ui.setupUi(this);
  setWindowTitle("Help viewer");
  viewer = new QWebView(this);
  contents = new QListWidget(this);
  ReadContents();
  layout = new QHBoxLayout();
  splitter = new QSplitter();
  layout->addWidget(splitter);
  splitter->addWidget(contents);
  splitter->addWidget(viewer);
  ui.verticalLayout->insertLayout(0, layout);
  QObject::connect(contents, SIGNAL(currentTextChanged(const QString &)), 
                   this, SLOT(currentTextChanged(const QString &)));
  
  QObject::connect(viewer, SIGNAL(linkClicked(const QUrl&)), this, SLOT(followLink(const QUrl&)));
  viewer->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
}

bool HelpViewer::ReadContents()
{
  QFile contentsFile(PREF.getDataPath(QString("/help/") + QString(HELP_BASE) + QString("contents.txt")));
  if(!contentsFile.open(QFile::ReadOnly)){
    return false;
  }
  bool res = false;
  QTextStream contents(&contentsFile);
  QString line;
  QRegExp pattern("^\"([^\"]+)\"\\s+\"([^\"]+)\"$");
  QStringList captured;
  while(!(line = contents.readLine()).isNull()){
    if(pattern.exactMatch(line)){
      captured = pattern.capturedTexts();
      if(captured.length() == 3){
        addPage(captured[2], captured[1]);
        res = true;
      }
    }
  }
  return res;
}

void HelpViewer::addPage(QString name, QString page)
{
  pages[name] = page;
  contents->addItem(name);
}

void HelpViewer::currentTextChanged(const QString &currentText)
{
  ChangeHelpPage(pages[currentText]);
}

void HelpViewer::on_CloseButton_pressed()
{
  close();
}

void HelpViewer::followLink(const QUrl &url)
{
  if(QString("http").compare(url.scheme(), Qt::CaseInsensitive) == 0){
    QDesktopServices::openUrl(url);
  }else{
    viewer->load(url);
  }
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

