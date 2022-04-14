// Author(s): A.J. (Hannes) Pretorius
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file ./combnplot.h

#ifndef COMBNPLOT_H
#define COMBNPLOT_H

#include "diagram.h"

class CombnPlot : public Visualizer
{
  Q_OBJECT

  public:
    // -- constructors and destructor -------------------------------
    CombnPlot(
      QWidget *parent,
      Graph* g,
      const std::vector<std::size_t> &attributeIndices
      );
    // -- set data functions ----------------------------------------
    void setDiagram(Diagram* dgrm);

    // -- set vis settings functions --------------------------------

    // -- visualization functions  ----------------------------------
    void visualize(const bool& inSelectMode);
    void drawAxes(const bool& inSelectMode);
    void drawAxesBC(const bool& inSelectMode);
    void drawAxesCP(const bool& inSelectMode);
    void drawLabels(const bool& inSelectMode);
    void drawLabelsBC(const bool& inSelectMode);
    void drawLabelsCP(const bool& inSelectMode);
    void drawPlot(const bool& inSelectMode);
    void drawPlotBC(const bool& inSelectMode);
    void drawPlotCP(const bool& inSelectMode);
    void drawMousePos(const bool& inSelectMode);
    void drawDiagram(const bool& inSelectMode);

    // -- input event handlers --------------------------------------
    void handleMouseEvent(QMouseEvent* e);

    QSize sizeHint() const { return QSize(400,400); }

  protected:
    // -- utility data functions ------------------------------------
    void initLabels();
    void calcMaxAttrCard();
    void calcMaxNumberPerComb();

    // -- utility drawing functions ---------------------------------
    // ***
    //void clear();
    void setScalingTransf();
    void displTooltip(const std::size_t& posIdx);

    void calcPositions();
    void calcPosBC();
    void calcPosCP();
    void clearPositions();

    // -- hit detection ---------------------------------------------
    void processHits(
      GLint hits,
      GLuint buffer[]);

    // -- data members ----------------------------------------------

    // data
    std::vector< std::string >        attributeLabels;
    std::vector< Attribute *> attributes;
    std::size_t                     maxAttrCard;
    std::vector< std::vector< std::size_t > > combinations;
    std::vector< std::size_t >           numberPerComb;
    std::size_t                     maxNumberPerComb;

    // bar chart
    int    minHgtHintPixBC;     // bar height cannot be less
    int    maxWthHintPixBC;     // bar width cannot be more
    double widthBC;             // actual width calculated & used for every bar
    std::vector< Position2D > posBC; // top, center

    // combination plot
    std::vector< std::vector< Position2D > > posLftTop;
    std::vector< std::vector< Position2D > > posRgtBot;
    std::size_t    mouseCombnIdx;

    // diagram
    Diagram*      diagram;         // association, user-defined diagram
    double        scaleDgrm;       // scale factor for diagram
    Position2D    posDgrm;         // positions of diagram
    bool          showDgrm;        // show or hide diagram
    std::vector< double > attrValIdcsDgrm; // value idx of attribute associated with diagram
    std::string        msgDgrm;         // message to show with diagram
};

#endif

// -- end -----------------------------------------------------------
