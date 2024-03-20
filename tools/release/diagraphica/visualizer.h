// Author(s): A.J. (Hannes) Pretorius
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file ./visualizer.h

#ifndef VISUALIZER_H
#define VISUALIZER_H

#include "graph.h"
#include "visutils.h"

#include <QOpenGLWidget>
#include <QOpenGLFramebufferObject>
#include <QOpenGLDebugLogger>

class Visualizer : public QOpenGLWidget
{
  Q_OBJECT

  public:
    // -- constructors and destructor -------------------------------
    Visualizer(
      QWidget *parent,
      Graph *m_graph);
    virtual ~Visualizer() {}

    virtual void initializeGL() override;
    virtual void paintGL() override;

    QSizeF worldSize();
    double pixelSize();
    QPointF worldCoordinate(QPointF deviceCoordinate);
    void logMessage(const QOpenGLDebugMessage& debugMessage);

    // -- set functions ---------------------------------------------
    virtual void setClearColor(
      const double& r,
      const double& g,
      const double& b);

    // -- visualization functions -----------------------------------
    enum Mode { Visualizing, Marking };

    virtual void visualize() = 0;
    virtual void mark() = 0;
    virtual void setGeomChanged(const bool& flag);
    virtual void setDataChanged(const bool& flag);

    // -- event handlers --------------------------------------------
    virtual void handleSizeEvent();

    virtual void handleMouseEvent(QMouseEvent* e);
    virtual void handleWheelEvent(QWheelEvent* /*e*/) { }
    virtual void handleMouseEnterEvent() { }
    virtual void handleMouseLeaveEvent() { }
    virtual void handleKeyEvent(QKeyEvent* e);

    virtual void enterEvent(QEnterEvent *event) override { handleMouseEnterEvent(); QOpenGLWidget::enterEvent(event); }
    virtual void leaveEvent(QEvent *event) override { handleMouseLeaveEvent(); QOpenGLWidget::leaveEvent(event); }
    virtual void keyPressEvent(QKeyEvent *event) override { handleKeyEvent(event); QOpenGLWidget::keyPressEvent(event); }
    virtual void keyReleaseEvent(QKeyEvent *event) override { handleKeyEvent(event); QOpenGLWidget::keyReleaseEvent(event); }
    virtual void wheelEvent(QWheelEvent *event) override { handleWheelEvent(event); QOpenGLWidget::wheelEvent(event); }
    virtual void mouseMoveEvent(QMouseEvent *event) override { handleMouseEvent(event); QOpenGLWidget::mouseMoveEvent(event); }
    virtual void mousePressEvent(QMouseEvent *event) override { handleMouseEvent(event); QOpenGLWidget::mousePressEvent(event); }
    virtual void mouseReleaseEvent(QMouseEvent *event) override { handleMouseEvent(event); QOpenGLWidget::mouseReleaseEvent(event); }
    virtual void resizeEvent(QResizeEvent *event) override { handleSizeEvent(); QOpenGLWidget::resizeEvent(event); }

    QSize sizeHint() const override { return QSize(200,200); } // Reimplement to change preferred size
  public slots:
    void updateSelection();

  protected:
    // -- protected utility functions -------------------------------
    virtual void clear();
    virtual void initMouse();

    virtual void startSelectMode(
      GLint hits,
      GLuint selectBuf[],
      double pickWth,
      double pickHgt);
    virtual void finishSelectMode(
      GLint hits,
      GLuint selectBuf[]);

    void genCharTex();
    void genCushTex();

    // -- hit detection ---------------------------------------------
    QOpenGLFramebufferObject m_selectionBuffer;
    virtual void processHits(
      GLint hits,
      GLuint buffer[]) = 0;

    // -- mouse -----------------------------------------------------
    std::unique_ptr<QMouseEvent> m_lastMouseEvent;   // The latest received event
    bool m_mouseDrag;               // The mouse is being dragged
    bool m_mouseDragReleased;       // The cursor was released after dragging
    QPoint m_mouseDragStart;        // The position where the drag started, only valid if (m_mouseDrag or m_mouseDragReleased)

    Qt::Key m_lastKeyCode;

    bool showMenu;

    QColor clearColor;

    Graph *m_graph;

    bool geomChanged; // canvas resized
    bool dataChanged; // data has changed

    bool texCharOK;
    GLuint texCharId[CHARSETSIZE];
    GLubyte texChar[CHARSETSIZE][CHARHEIGHT* CHARWIDTH];

    bool texCushOK;
    GLuint texCushId;
    float texCush[CUSHSIZE];
  
    QOpenGLDebugLogger* m_logger; ///< Logs OpenGL debug messages.
};

#endif
