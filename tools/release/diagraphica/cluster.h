// Author(s): A.J. (Hannes) Pretorius
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file ./cluster.h

#ifndef CLUSTER_H
#define CLUSTER_H

#include "attribute.h"
#include "node.h"

class Bundle;

class Cluster
{
  public:
    // -- constructors and destructors ------------------------------
    Cluster();
    Cluster(const std::vector< std::size_t > &crd);
    Cluster(const Cluster& clst);
    virtual ~Cluster();

    // -- set functions ---------------------------------------------
    void setCoord(const std::vector< std::size_t > &crd) { coord = crd; }
    void setIndex(const std::size_t& idx) { index = idx; }
    void setParent(Cluster* p) { parent = p; }
    void addChild(Cluster* c) { children.push_back(c); }
    void setChildren(const std::vector< Cluster* > &c) { clearChildren(); children = c; }
    void addNode(Node* n) { nodes.push_back(n); }
    void setNodes(const std::vector< Node* > &n) { clearNodes(); nodes = n; }

    void setAttribute(Attribute* attr) { attribute = attr; }
    void setAttrValIdx(const std::size_t& idx) { attrValIdx = idx; }

    void addInBundle(Bundle* b) { inBundles.push_back(b); }
    void setInBundles(const std::vector< Bundle* > b) { clearInBundles(); inBundles = b; }
    void addOutBundle(Bundle* b) { outBundles.push_back(b); }
    void setOutBundles(const std::vector< Bundle* > b) { clearOutBundles(); outBundles = b; }


    // -- get functions ---------------------------------------------
    std::size_t getSizeCoord() { return coord.size(); }
    std::size_t getCoord(const std::size_t& idx);
    void getCoord(std::vector< std::size_t > &crd) { crd = coord; }
    std::size_t getIndex() { return index; }
    Cluster* getParent() { return parent; }
    std::size_t getSizeChildren() { return children.size(); }
    Cluster* getChild(const std::size_t& idx);
    std::size_t getSizeNodes() { return nodes.size(); }
    Node* getNode(const std::size_t& idx);
    std::size_t getSizeDescNodes();

    Attribute* getAttribute() { return attribute; }
    std::size_t getAttrValIdx() { return attrValIdx; }

    std::size_t getSizeInBundles() { return inBundles.size(); }
    Bundle* getInBundle(const std::size_t& idx);
    std::size_t getSizeOutBundles() { return outBundles.size(); }
    Bundle* getOutBundle(const std::size_t& idx);

    // -- clear functions -------------------------------------------
    void clearParent() { parent = 0; }
    void clearChildren();
    void clearNodes();
    void clearAttribute();
    void clearInBundles();
    void clearOutBundles();

  protected:
    // -- utility functions -----------------------------------------
    void getSizeDescNodes(Cluster* curClst, std::size_t& sum);

    // -- data members ----------------------------------------------
    std::vector< std::size_t >      coord;
    std::size_t                index;
    Cluster*           parent;    // association
    std::vector< Cluster* > children;  // association
    std::vector< Node* >    nodes;     // association
    Attribute*         attribute; // association
    std::size_t                attrValIdx;
    std::vector< Bundle* >  inBundles;  // association
    std::vector< Bundle* >  outBundles; // association
};

#endif

// -- end -----------------------------------------------------------
