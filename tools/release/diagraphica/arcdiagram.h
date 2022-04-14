// Author(s): A.J. (Hannes) Pretorius
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file ./arcdiagram.h

// --- arcdiagram.h -------------------------------------------------
// (c) 2007  -  A.J. Pretorius  -  Eindhoven University of Technology
// ---------------------------  *  ----------------------------------

#ifndef ARCDIAGRAM_H
#define ARCDIAGRAM_H

#include <QTimer>
#include "diagram.h"
#include "settings.h"


enum RenderMode
{
  HQRender,
  LQRender,
  HitRender
};

class ArcDiagram : public Visualizer
{
  Q_OBJECT
  public:
    // -- constructors and destructor -------------------------------
    ArcDiagram(
        QWidget *parent,
        Settings* s,
        Graph* g);
    virtual ~ArcDiagram();

    void getAttrsTree(std::vector< std::size_t > &idcs);

    void setAttrsTree(const std::vector< std::size_t > idcs);

    void setDiagram(Diagram* dgrm);
    void hideAllDiagrams();

    void markLeaf(
        const std::size_t& leafIdx,
        QColor col);
    void unmarkLeaves();
    void markBundle(const std::size_t& idx);
    void unmarkBundles();

    // -- visualization functions  ----------------------------------
    void visualize(const bool& inSelectMode);
    void visualizeParts(const bool& inSelectMode);
    void drawBundles(const bool& inSelectMode);
    void drawLeaves(const bool& inSelectMode);
    void drawTree(const bool& inSelectMode);
    void drawTreeLvls(const bool& inSelectMode);
    void drawBarTree(const bool& inSelectMode);
    void drawMarkedLeaves(const bool& inSelectMode);
    void drawDiagrams(const bool& inSelectMode);

    // -- input event handlers --------------------------------------

    void handleMouseEvent(QMouseEvent* e);

    QSize sizeHint() const { return QSize(600,600); }

    void updateDiagramData();

  signals:
    void routingCluster(Cluster *cluster, QList<Cluster *> clusterSet, QList<Attribute *> attributes);
    void hoverCluster(Cluster *cluster, QList<Attribute *> attributes = QList<Attribute *>());
    void clickedCluster(Cluster *cluster);

  protected:
    // -- utility drawing functions ---------------------------------
    void clear();
    QColor calcColor(std::size_t iter, std::size_t numr);
    void calcSettingsGeomBased();
    void calcSettingsDataBased();

    void calcSettingsLeaves();
    void calcSettingsBundles();
    void calcSettingsTree();
    void calcPositionsTree(
        Cluster* c,
        const std::size_t& maxLvl,
        const double& itvHgt);
    void calcSettingsBarTree();
    void calcPositionsBarTree(
        Cluster* c,
        const double& yBot,
        const double& height);
    void calcSettingsDiagram();
    void updateMarkBundles();

    void clearSettings();
    void clearSettingsLeaves();
    void clearSettingsBundles();
    void clearSettingsTree();
    void clearSettingsBarTree();
    void clearSettingsDiagram();

    // -- utility event handlers ------------------------------------
  protected slots:
    void animate();
    void clustersChanged();

  protected:
    void handleHits(const std::vector< int > &ids);

    void handleHoverCluster(
        const std::size_t& i,
        const std::size_t& j);
    void handleHoverBundle(const std::size_t& bndlIdx);
    void handleHoverBarTree(
        const int& i,
        const int& j);

    void handleShowDiagram(const std::size_t& dgrmIdx);
    void handleDragDiagram();
    void handleDragDiagram(const int& dgrmIdx);
    void handleRwndDiagram(const std::size_t& dgrmIdx);
    void handlePrevDiagram(const std::size_t& dgrmIdx);
    void handlePlayDiagram(const std::size_t& dgrmIdx);
    void handleNextDiagram(const std::size_t& dgrmIdx);

    void showDiagram(const std::size_t& dgrmIdx);
    void hideDiagram(const std::size_t& dgrmIdx);

    // -- hit detection ---------------------------------------------
    void processHits(
        GLint hits,
        GLuint buffer[]);

    // -- static variables ------------------------------------------
    // -- data members ----------------------------------------------
    Settings* settings;

    QPoint m_lastMousePos;

    // vis settings bundles
    std::vector< Position2D > posBundles;
    std::vector< double >     radiusBundles;
    std::vector< double >     widthBundles;
    std::vector< int >        orientBundles;
    std::vector< bool >       markBundles;

    // vis settings leaves
    std::vector< Position2D > posLeaves;
    double               radLeaves;
    std::size_t                  idxInitStLeaves;

    // vis settings hierarchy
    std::vector< Attribute* >           attrsTree;
    std::vector< std::vector< Position2D > > posTreeTopLft;
    std::vector< std::vector< Position2D > > posTreeBotRgt;
    std::vector< std::vector< Cluster* > >   mapPosToClust;

    // vis settings bar tree
    std::vector< std::vector< Position2D > > posBarTreeTopLft;
    std::vector< std::vector< Position2D > > posBarTreeBotRgt;

    // diagrams
    Diagram*                       diagram;         // association, user-defined diagram
    std::vector< bool >                 showDgrm;        // show/hide diagram for every leaf node
    std::vector< std::vector< Attribute* > > attrsDgrm;       // association, attributes linked to shown diagrams
    std::vector< std::vector< Cluster* > >   framesDgrm;      // composition, clusters of identical states for shown diagrams
    std::vector< std::size_t >                  frameIdxDgrm;    // current index into framesDgrm
    std::vector< Position2D >           posDgrm;         // positions of diagrams
    std::size_t                         dragIdxDgrm;     // diagram currently being dragged
    std::size_t                         animIdxDgrm;     // diagram currently being animated
    std::size_t                         currIdxDgrm;

    // simulator
    std::size_t prevFrameIdxClust;
    std::size_t currFrameIdxClust;
    std::size_t nextFrameIdxClust;
    std::map< std::size_t, std::vector< QColor > > markLeaves;

    // animation
    QTimer m_animationTimer;

    // -- constants -------------------------------------------------
    enum
    {
      ID_TIMER,
      ID_CANVAS,
      ID_TREE_NODE,
      ID_LEAF_NODE,
      ID_BAR_TREE,
      ID_BUNDLES,
      ID_DIAGRAM,
      ID_DIAGRAM_CLSE,
      ID_DIAGRAM_MORE,
      ID_DIAGRAM_RWND,
      ID_DIAGRAM_PREV,
      ID_DIAGRAM_PLAY,
      ID_DIAGRAM_NEXT
    };

    static int MIN_RAD_HINT_PX; // radius cannot be smaller than this
    static int MAX_RAD_HINT_PX; // radius cannot be larger than this
    static int SEGM_HINT_HQ;
    static int SEGM_HINT_LQ;
};

#endif

// -- end -----------------------------------------------------------
