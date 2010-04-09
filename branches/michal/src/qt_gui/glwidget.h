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
     void setXRotation(float angle);
     void setYRotation(float angle);
     void setZRotation(float angle);
     void setXTrans(float pos);
     void setYTrans(float pos);
     void setZTrans(float pos);

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
     float xRot;
     float yRot;
     float zRot;
     float xTrans;
     float yTrans;
     float zTrans;
     
     QPoint lastPos;
     QColor trolltechGreen;
     QColor trolltechPurple;
 };

 #endif
