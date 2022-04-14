// Author(s): A.J. (Hannes) Pretorius
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file ./bundle.h

#ifndef BUNDLE_H
#define BUNDLE_H

#include "edge.h"
#include "cluster.h"

//class Cluster;

class Bundle
{
  public:
    // -- constructors and destructors ------------------------------
    Bundle();
    Bundle(const std::size_t& idx);
    Bundle(const Bundle& bdl);
    virtual ~Bundle();


    // -- set functions ---------------------------------------------
    void setIndex(const std::size_t& idx) { index = idx; }
    void setParent(Bundle* p) { parent = p; }
    void addChild(Bundle* c) { children.push_back(c); }
    void setInCluster(Cluster* in) { inCluster = in; }
    void setOutCluster(Cluster* out) { outCluster = out; }
    void addEdge(Edge* e);
    void setEdges(const std::vector< Edge* > &e);
    void updateLabel(const std::string& lbl, const std::string& status) { labels[lbl] = status; }


    // -- get functions ---------------------------------------------
    std::size_t getIndex() { return index; }
    Bundle* getParent() { return parent; }
    std::size_t getSizeChildren() { return children.size(); }
    Bundle* getChild(const std::size_t& idx);
    Cluster* getInCluster() { return inCluster; }
    Cluster* getOutCluster() { return outCluster; }
    std::size_t getSizeEdges() { return edges.size(); }
    Edge* getEdge(const std::size_t& idx);
    void getLabels(std::vector< std::string > &lbls);
    void getLabels(
      std::vector< std::string > &lbls,
      std::vector< std::string > &status);
    void getLabels(
      std::string& separator,
      std::string& lbls);

    // -- clear functions -------------------------------------------
    void clearParent() { parent = 0; }
    void clearChildren();
    void clearInCluster() { inCluster = 0; }
    void clearOutCluster() { outCluster = 0; }
    void clearEdges();

  protected:
    // -- data members ----------------------------------------------
    std::size_t index;
    Bundle* parent;
    std::vector< Bundle* > children; // association
    Cluster* inCluster;         // association
    Cluster* outCluster;        // association
    std::vector< Edge* > edges;      // association
    std::map< std::string, std::string > labels;
};

#endif

// -- end -----------------------------------------------------------
