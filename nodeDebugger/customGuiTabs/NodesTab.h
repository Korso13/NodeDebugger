//
// Created by Timur Korsakov on 12.01.2026.
//

#pragma once
#include "common/nodeDebugger/ND_iGuiTab.h"

class SpinesTab;

class NodesTab : public iGuiTab
{
    struct ImG_Node
    {
        using NodeVariantHolder = std::variant<wptr<Node>, wptr<Spine>, wptr<Mesh>, wptr<ParticlesEmitter>>;

        NodeVariantHolder node;
        //wptr<Node> node;
        std::string name;
        bool markedForDeletion = false;
    };

public:

    NodesTab() : iGuiTab("Nodes") {};

    void onGuiCall() override NOCOMP;
    bool isEmpty() override {return false;}

    void addItem(std::any item, const std::string& altName = "default") override NOCOMP;

    void removeItem(const std::string& itemName) override NOCOMP;

    void purgeValues() override NOCOMP;

private:

    void nodesScrapperBtn() NOCOMP;
    // Renders NodeVariant. Returns true if it needs to be deleted (for onGuiAddedNodes())
    bool renderNodeVariant(ImG_Node::NodeVariantHolder node, const std::string& name) NOCOMP;
    bool isNodeVariantExpired(ImG_Node::NodeVariantHolder node) const NOCOMP;
    std::string getVariantNodeName(ImG_Node::NodeVariantHolder node) const NOCOMP;
    sptr<Node> castVariantToNode(ImG_Node::NodeVariantHolder node) NOCOMP;
    void onGuiAddedNodes() NOCOMP;
    void onGuiHierarchy() NOCOMP;

    //debug
    std::string getVariantTypeInfo(ImG_Node::NodeVariantHolder node) NOCOMP;
private:

    bool m_needUpdateNodesFilter = false;
    int m_nodeFilterItems = 0;

    std::map<std::string, ImG_Node> m_staticNodes;
    std::string m_nodesFilter = "";
    std::set<std::string> m_lastNodeFilterList; //list of hidden nodes after last filter change

    wptr<SpinesTab> m_spinesTabRef;

    ImG_Node::NodeVariantHolder m_lastHierarchyNode;
};

