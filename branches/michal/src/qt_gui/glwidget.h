#ifndef GLWIDGET_H
 #define GLWIDGET_H

 #include <QGLWidget>

 class GLWidget : public QGLWidget
 {
     Q_OBJECT

 public:
     GLWidget(QWidget *parent = 0);
     ~GLWidget();

     QSize minimumSizeHint() const;
     QSize sizeHint() const;

 public slots:
     void setXRotation(int angle);
     void setYRotation(int angle);
     void setZRotation(int angle);
     void setXTrans(int pos);
     void setYTrans(int pos);
     void setZTrans(int pos);

 signals:
     void xRotationChanged(int angle);
     void yRotationChanged(int angle);
     void zRotationChanged(int angle);

 protected:
     void initializeGL();
     void paintGL();
     void resizeGL(int width, int height);
     void mousePressEvent(QMouseEvent *event);
     void mouseMoveEvent(QMouseEvent *event);

 private:
     bool makeObjects();
     void normalizeAngle(int *angle);

     std::vector<GLuint> objects;
     int xRot;
     int yRot;
     int zRot;
     float xTrans;
     float yTrans;
     float zTrans;
     
     QPoint lastPos;
     QColor trolltechGreen;
     QColor trolltechPurple;
 };

 #endif
