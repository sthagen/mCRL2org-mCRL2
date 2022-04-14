// Author(s): A.J. (Hannes) Pretorius
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file ./graph.h

#ifndef GRAPH_H
#define GRAPH_H

#include "attrdiscr.h"
#include "bundle.h"

class Graph : public QObject
{
  Q_OBJECT

  public:
    // -- constructors and destructors ------------------------------
    Graph();
    ~Graph();

    // -- set functions ---------------------------------------------
    void setFileName(QString filename);
    /*
        void addAttribute(
            const std::string &name,
            const std::string &type,
            const int &idx,
            const std::vector< std::string > &vals );
        void addAttribute(
            const std::string &name,
            const std::string &type,
            const int &idx,
            const double &lwrBnd,
            const double &uprBnd );
    */
    void addAttrDiscr(
      QString name,
      QString type,
      const std::size_t& idx,
      const std::vector< std::string > &vals);

    void moveAttribute(
      const std::size_t& idxFr,
      const std::size_t& idxTo);
    void configAttributes(
      std::map< std::size_t , std::size_t > &idcsFrTo,
      std::map< std::size_t, std::vector< std::string > > &attrCurDomains,
      std::map< std::size_t, std::map< std::size_t, std::size_t  > > &attrOrigToCurDomains);
    void duplAttributes(const std::vector< std::size_t > &idcs);
    void deleteAttribute(const std::size_t& idx);

    void addNode(const std::vector< double > &tpl);
    void addEdge(
      const std::string& lbl,
      const std::size_t& inNodeIdx,
      const std::size_t& outNodeIdx);

    void initGraph();

    // -- get functions  --------------------------------------------
    QString filename();
    std::size_t getSizeAttributes();
    Attribute* getAttribute(const std::size_t& idx);
    Attribute* getAttribute(QString name);
    std::size_t getSizeNodes();
    Node* getNode(const std::size_t& idx);
    std::size_t getSizeEdges();
    Edge* getEdge(const std::size_t& idx);
    Cluster* getRoot();
    Cluster* getCluster(const std::vector< std::size_t > coord);
    Cluster* getLeaf(const std::size_t& idx);
    std::size_t getSizeLeaves();
    Bundle* getBundle(const std::size_t& idx);
    std::size_t getSizeBundles();

    // -- calculation functions -------------------------------------
    void calcAttrDistr(
      const std::size_t& attrIdx,
      std::vector< std::size_t > &distr);

    void calcAttrCorrl(
      const std::size_t& attrIdx1,
      const std::size_t& attrIdx2,
      std::vector< std::vector< std::size_t > > &corrlMap,
      std::vector< std::vector< int > > &number);

    void calcAttrCombn(
      const std::vector< std::size_t > &attrIndcs,
      std::vector< std::vector< std::size_t > > &combs,
      std::vector< std::size_t > &number);
    void calcAttrCombn(
      Cluster* clust,
      const std::vector< std::size_t > &attrIndcs,
      std::vector< std::vector< std::size_t > > &combs,
      std::vector< std::size_t > &number);
    void calcAttrCombn(
      const std::vector< std::size_t > &attrIndcs,
      std::vector< std::vector< std::size_t > > &combs);
    void calcAttrCombn(
      Cluster* clust,
      const std::vector< std::size_t > &attrIndcs,
      std::vector< std::vector< std::size_t > > &combs);
    void calcAttrCombn(
      Cluster* clust,
      const std::vector< std::size_t > &attrIndcs,
      std::vector< std::vector< Node* > > &combs);
    void calcAttrCombn(
      Cluster* clust,
      const std::vector< Attribute* > &attrs,
      std::vector< Cluster* > &combs);

    bool hasMultAttrCombns(
      Cluster* clust,
      const std::vector< int > &attrIndcs);

    // -- cluster & bundle functions --------------------------------
    void clustNodesOnAttr(const std::vector< std::size_t > &attrIdcs);
    void clearSubClusters(const std::vector< std::size_t > &coord);

    std::size_t sumNodesInCluster(const std::vector< std::size_t > &coord);
    void sumNodesInCluster(
      Cluster* clust,
      std::size_t& total);
    void getDescNodesInCluster(
      const std::vector< std::size_t > &coord,
      std::vector< Node* > &nodes);
    void getDescNodesInCluster(
      Cluster* clust,
      std::vector< Node* > &nodes);
    std::size_t calcMaxNumCombns(const std::vector< std::size_t > &attrIdcs);

  protected slots:
    void recluster();

  signals:
    void startedClusteringNodes(int steps);
    void startedClusteringEdges(int steps);
    void progressedClustering(int steps);
    void clusteringChanged();
    void deletedAttribute();

  protected:
    // -- private utility functions ---------------------------------
    void deleteAttributes();
    void addNode(Node* n);
    void deleteNodes();
    void addEdge(Edge* e);
    void deleteEdges();

    void initRoot();

    void clustNodesOnAttr(
      Cluster* clust,
      std::vector< std::size_t > attrIdcs,
      std::size_t& progress);
    void clustClusterOnAttr(
      const std::vector< std::size_t > coord,
      const std::size_t& attrIdx);
    void clustClusterOnAttr(
      Cluster* clust,
      const std::size_t& attrIdx);
    void clearSubClusters(Cluster* clust);

    void updateLeaves();
    void updateLeaves(Cluster* clust);
    //void updateLeaves( std::vector< Cluster* > &clusts );
    void clearLeaves();
    void deleteClusters();

    void updateBundles(std::size_t& progress);
    void updateBundles();
    void deleteBundles();

    // -- data members ----------------------------------------------
    QString                   m_filename; // file name
    std::vector< Attribute* > attributes; // attributes
    std::vector< Node* >      nodes;      // composition
    std::vector< Edge* >      edges;      // composition
    Cluster*                  root;       // composition
    std::vector< Cluster* >   leaves;     // association
    std::vector< Bundle* >    bundles;    // composition
};

#endif

// -- end -----------------------------------------------------------
