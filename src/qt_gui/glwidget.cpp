#include <QtGui>
#include <QtOpenGL>
#ifndef DARWIN
#include <GL/glu.h>
#endif
#include <iostream>
#include <math.h>
#include "objreader.h"
#include "glwidget.h"
#include <iostream> 
#include "pathconfig.h"

ReaderThread::ReaderThread(): QThread()
{
}

void ReaderThread::run()
{
     read_obj();
     std::cout<<"Object read!"<<std::endl;
     emit done();
}


void GLWidget::objectsRead()
{
  emit ready();
}

GLWidget::GLWidget(QWidget *parent)
     : QGLWidget(parent), rt(new ReaderThread())
 {
     xRot = 0;
     yRot = 0;
     zRot = 0;
     xTrans = 0;
     yTrans = 0;
     zTrans = 0;
     trolltechPurple = QColor::fromCmykF(0.39, 0.39, 0.0, 0.0);
     
     connect(rt, SIGNAL(done()), this, SLOT(objectsRead()));
     rt->start();
 }

 GLWidget::~GLWidget()
 {
     rt->terminate();
     makeCurrent();
     std::vector<GLuint>::iterator i;
     for(i = objects.begin(); i != objects.end(); ++i){
       glDeleteLists(*i, 1);
     }
     delete rt;
 }

 QSize GLWidget::minimumSizeHint() const
 {
     return QSize(50, 50);
 }

 QSize GLWidget::sizeHint() const
 {
     return QSize(400, 400);
 }

 void GLWidget::setXRotation(float angle)
 {
     if (angle != xRot) {
         xRot = angle;
     }
 }

 void GLWidget::setYRotation(float angle)
 {
     if (angle != yRot) {
         yRot = angle;
     }
 }

 void GLWidget::setZRotation(float angle)
 {
     if (angle != zRot) {
         zRot = angle;
     }
 }

 void GLWidget::setXTrans(float val)
 {
   xTrans = val;
 }

 void GLWidget::setYTrans(float val)
 {
   yTrans = val;
 }

 void GLWidget::setZTrans(float val)
 {
   zTrans = val;
 }

 void GLWidget::initializeGL()
 {
     qglClearColor(trolltechPurple.dark());
     makeObjects();
     glShadeModel(GL_FLAT);
     glEnable(GL_DEPTH_TEST);
     glEnable(GL_CULL_FACE);
     glEnable(GL_TEXTURE_2D);
 }

void GLWidget::paintGL()
{
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
     glLoadIdentity();
     
     glRotated(-xRot, 1.0, 0.0, 0.0);
     glRotated(-yRot, 0.0, 1.0, 0.0);
     glRotated(-zRot, 0.0, 0.0, 1.0);
     glTranslated(-xTrans, -yTrans, -zTrans);
     
     glPushMatrix();
     glTranslated(0.0, -0.7, -2.265);
     std::vector<GLuint>::const_iterator i;
     for(i = objects.begin(); i != objects.end(); ++i){
       glCallList(*i);
     }
     glPopMatrix();

     glPushMatrix();
     glTranslated(0.0, 5.0, -2.265);
     glRotated(180.0, 0.0, 0.0, 1.0);
     for(i = objects.begin(); i != objects.end(); ++i){
       glCallList(*i);
     }
     glPopMatrix();

     glPushMatrix();
     glTranslated(0.0, 5.0, -20.265);
     for(i = objects.begin(); i != objects.end(); ++i){
       glCallList(*i);
     }
     glPopMatrix();

     glPushMatrix();
     glTranslated(-10.0, 3.0, -5.265);
     for(i = objects.begin(); i != objects.end(); ++i){
       glCallList(*i);
     }
     glPopMatrix();

     glPushMatrix();
     glTranslated(10.0, 3.0, -5.265);
     for(i = objects.begin(); i != objects.end(); ++i){
       glCallList(*i);
     }
     glPopMatrix();
}

 void GLWidget::resizeGL(int width, int height)
 {
     glViewport(0, 0, width, height);

     glMatrixMode(GL_PROJECTION);
     glLoadIdentity();
     gluPerspective(55.0, (double)width/height, 0.1, 45.0);
     glMatrixMode(GL_MODELVIEW);
 }


bool textured;
object_t obj;


static void make_vortex(int index)
{
  vtx_t vtx = obj.vtx_table[index];
  glNormal3f(vtx.nx, vtx.ny, vtx.nz);
  if(textured)
    glTexCoord2f(vtx.s, vtx.t);
  glVertex3f(vtx.x, vtx.y, vtx.z);
}

static void make_triangle(int index1, int index2, int index3)
{
  make_vortex(index3);
  make_vortex(index2);
  make_vortex(index1);
}

bool GLWidget::makeObjects()
 {
     std::vector<object_t>::const_iterator obj_index;
     std::vector<tri_t>::const_iterator tris_index;
     for(obj_index = object_table.begin(); obj_index != object_table.end(); ++obj_index){
       obj = *obj_index;
       GLuint texture;
       GLuint list = glGenLists(obj.tris_table.size());
       
       if(obj.texture.isEmpty()){
         textured = false;
       }else{
         textured = true;
       }
       if(textured){
         texture = bindTexture(QImage(QString(obj.texture)), GL_TEXTURE_2D);
         glBindTexture(GL_TEXTURE_2D, texture);
       }
       for(tris_index = obj.tris_table.begin(); 
           tris_index != obj.tris_table.end(); ++tris_index){
         glNewList(list, GL_COMPILE);
	 if(tris_index->glass){
           glEnable (GL_BLEND); 
           glDepthMask (GL_FALSE);
           glBlendFunc (GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	 }
         glBegin(GL_TRIANGLES);
         
	 int offset = tris_index->offset;
	 int count = tris_index->count;
         for(int i = 0; i < count; i+=3){
           make_triangle(obj.vtx_indices[offset + i], obj.vtx_indices[offset + i + 1], 
                     obj.vtx_indices[offset + i + 2]);
	 }
         glEnd();
         glDepthMask (GL_TRUE);
	 glDisable(GL_BLEND);
         glEndList();
	 objects.push_back(list);
         ++list;
       }
     }
     
     return true;
 }


