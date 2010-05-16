#ifndef GLWIDGET_H
 #define GLWIDGET_H

 #include <QGLWidget>
 #include <QThread>

class ReaderThread : public QThread
{
  Q_OBJECT
 public:
  ReaderThread();
  void run();
 signals:
  void done();  
 private:
};


 class GLWidget : public QGLWidget
 {
     Q_OBJECT

 public:
     GLWidget(QWidget *parent = 0);
     ~GLWidget();

     QSize minimumSizeHint() const;
     QSize sizeHint() const;
 signals:
     void ready();
 public slots:
     void setXRotation(float angle);
     void setYRotation(float angle);
     void setZRotation(float angle);
     void setXTrans(float pos);
     void setYTrans(float pos);
     void setZTrans(float pos);
     void objectsRead();
 protected:
     void initializeGL();
     void paintGL();
     void resizeGL(int width, int height);

 private:
     bool makeObjects();
     void normalizeAngle(int *angle);

     std::vector<GLuint> objects;
     float xRot;
     float yRot;
     float zRot;
     float xTrans;
     float yTrans;
     float zTrans;
     
     QColor trolltechPurple;
     
     ReaderThread *rt;
 };

 #endif
