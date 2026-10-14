//
// Created by Timur Korsakov on 12.01.2026.
//

#include "NodesTab.h"

#ifndef FORCENODEDEBUG_OFF
#include "SpinesTab.h"
#include "common/nodeDebugger/NodeDebugger.h"
#include "common/nodeDebugger/ND_Defines.h"

void NodesTab::onGuiCall()
{
    ND_Util::ImGuiNodeSettings();

    if (ImGui::BeginTabBar("NodesTabBar"))
    {
        if (!m_staticNodes.empty())
        {
            if (ImGui::BeginTabItem("Added Nodes"))
            {
                onGuiAddedNodes();
                ImGui::EndTabItem();
            }
        }
        if (ImGui::BeginTabItem("Nodes Hierarchy"))
        {
            onGuiHierarchy();
            ImGui::EndTabItem();
        }
    }
    ImGui::EndTabBar();
}

void NodesTab::addItem(std::any item, const std::string& altName)
{
    std::string name = altName;
    sptr<Node> inNode = std::any_cast<sptr<Node>>(item);
    if (!inNode)
        return;

    if (inNode->is<Spine>())
    {
        NODEDEBUG->addSpine(inNode->as<Spine>(), name);
        //return;
    }
    if (inNode->is<SpotLight>())
    {
        NODEDEBUG->addSpotLight(inNode->as<SpotLight>(), name);
        return;
    }

    if (name == "default")
        name = inNode->getName();
    m_needUpdateNodesFilter = true;

    //prevents duplicate names
    std::string zeroes = "00"; //for better sorting
    if (m_staticNodes.contains(name))
    {
        //checks if the list contains this node already
        bool isAlreadyAdded = false;
        std::visit(Visitor{
            [&isAlreadyAdded, inNode, this](auto& nodeW)
            {
                if (nodeW.expired())
                    return;
                auto node = nodeW.lock();
                if (node.get() == inNode.get())
                    isAlreadyAdded = true;
            }},
            m_staticNodes.at(name).node);
        if (isAlreadyAdded) return;

        for (size_t i = 0; i < 500; i++)
        {
            if (i == 10 || i == 100)
                zeroes.pop_back();

            if (!m_staticNodes.contains(name + "(" + zeroes + STR(i) + ")"))
            {
                name += "(" + zeroes + STR(i) + ")";
                break;
            }
            //checks if the list contains this node already
            // else if (!m_staticNodes.at(name + "(" + zeroes + STR(i) + ")").markedForDeletion /*&&
            //          m_staticNodes.at(name + "(" + zeroes + STR(i) + ")").node.lock().get() == inNode.get()*/)
            //     return;
        }
    }

    ImG_Node nodeStruct = {
        .node = inNode,
        .name = name,
        .markedForDeletion = false
    };
    if (inNode->is<Spine>())
    {
        nodeStruct.node = wptr<Spine>(inNode->as<Spine>());
    }
    if (inNode->is<Mesh>())
    {
        nodeStruct.node = wptr<Mesh>(inNode->as<Mesh>());
    }
    if (inNode->is<ParticlesEmitter>())
    {
        nodeStruct.node = wptr<ParticlesEmitter>(inNode->as<ParticlesEmitter>());
    }
    m_staticNodes.insert({ name, nodeStruct });
}

void NodesTab::removeItem(const std::string& nodeName)
{

    if (m_staticNodes.contains(nodeName))
    {
        m_staticNodes.erase(nodeName);
        return;
    }

#ifdef NDSPINES
    if (NODEDEBUG->getCustomTabByName("Spines"))
    {
        NODEDEBUG->getCustomTabByName("Spines")->removeItem(nodeName);
    }
#endif

    bool isFound = false;
    for (auto& [name, ImG_NodeStruct] : m_staticNodes)
    {
        std::visit(Visitor{
            [&isFound, this, nodeName, name = ImG_NodeStruct.name](auto& node)
            {
                if (!node.expired() && node.lock()->getName() == nodeName)
                {
                    m_staticNodes.erase(name);
                    isFound = true;
                }
            }},
            ImG_NodeStruct.node);

        if (isFound) return;
    }
}

void NodesTab::purgeValues()
{
    m_staticNodes.clear();
    m_nodesFilter = "";
    m_lastNodeFilterList.clear();
    m_nodeFilterItems = 0;
}

void NodesTab::nodesScrapperBtn()
{
    if (ImGui::Button("Add all existing nodes"))
    {
        Render::getRoot()->traversalTyped<Node>([this](sptr<Node> _node)
            {
                if (_node && _node != Render::getRoot() && _node != Render::getScreenCamera())
                    addItem(_node);
            });
        Render::getHudPass().getRoot()->traversalTyped<Node>([this](sptr<Node> _node)
            {
                if (_node && _node != Render::getRoot() && _node != Render::getScreenCamera())
                    addItem(_node);
            });
        for (auto& [name, ImG_NodeStruct] : m_staticNodes)
        {
            std::visit(Visitor{
                [&ImG_NodeStruct, this](auto& node)
                {
                    if (node.expired())
                        ImG_NodeStruct.markedForDeletion = true;
                }},
                ImG_NodeStruct.node);
        }
    }
}

bool NodesTab::renderNodeVariant(ImG_Node::NodeVariantHolder node, const std::string& name)
{
    bool needsDeletion = false;

    //boilerplate code
#define IMGUI_NODE_RENDER \
    if (nodeW.expired()) \
    { \
        needsDeletion = true; \
        return; \
    } \
    auto node = nodeW.lock(); \
    ND_Util::ImGuiNodeRender(node, name);

    // Handler for ImG_Node.node variant field
    Visitor visitor = {
        [this, name, &needsDeletion](wptr<Spine>& nodeW)
        {
            IMGUI_NODE_RENDER;
            //W4_LOG_DEBUG("[%s][%s] is Spine", name.c_str(), node->getTypeInfo().name());
        },
        [this, name, &needsDeletion](wptr<Mesh>& nodeW)
        {
            IMGUI_NODE_RENDER;
            //W4_LOG_DEBUG("[%s][%s] is Mesh", name.c_str(), node->getTypeInfo().name());
        },
        [this, name, &needsDeletion](wptr<ParticlesEmitter>& nodeW)
        {
            IMGUI_NODE_RENDER;
            //W4_LOG_DEBUG("[%s][%s] is ParticlesEmitter", name.c_str(), node->getTypeInfo().name());
        },
        [this, name, &needsDeletion](wptr<Node>& nodeW)
        {
            IMGUI_NODE_RENDER;
            //if (std::string(node->getTypeInfo().name()) == "Object_Spine") //sometimes Visitor fails...
            //    tryRenderSpineProperties(cast_sptr<Spine>(node), name);
            //W4_LOG_DEBUG("[%s][%s] is Node", name.c_str(), node->getTypeInfo().name());
        }
    };
#undef IMGUI_NODE_RENDER

    std::visit(visitor, node); //handles ImG_Node.node using appropriate lambda above
    return needsDeletion;
}

bool NodesTab::isNodeVariantExpired(ImG_Node::NodeVariantHolder node) const
{
    bool isExpired = false;
    Visitor visitor = {[&isExpired](const auto& node)
        {
           isExpired = node.expired();
        }};
    std::visit(visitor, node);
    return isExpired;
}

std::string NodesTab::getVariantNodeName(ImG_Node::NodeVariantHolder node) const
{
    std::string nodeName = "INVALID";
    Visitor visitor = {[&nodeName](const auto& node)
        {
            if (node.expired())
            {
                nodeName = "INVALID";
                return;
            }
            nodeName = node.lock()->getName();
        }};
    std::visit(visitor, node);
    return nodeName;
}

sptr<Node> NodesTab::castVariantToNode(ImG_Node::NodeVariantHolder node)
{
    sptr<Node> nodeCast = nullptr;
    Visitor visitor = {[&nodeCast](const auto& node)
        {
            if (node.expired())
                return;
            nodeCast = node.lock();
        }};
    std::visit(visitor, node);
    return nodeCast;
}

void NodesTab::onGuiAddedNodes()
{
    bool isFilterChanged = false;
    if (m_staticNodes.size() > 6)
        isFilterChanged = (ND_Util::ImGuiSearchBar(m_nodesFilter) || m_needUpdateNodesFilter);
    else if (!m_nodesFilter.empty())
        m_nodesFilter.clear();

    if (m_needUpdateNodesFilter) m_needUpdateNodesFilter = false;

    ImGui::SameLine();
    nodesScrapperBtn();

    // Updating filters
    if (m_nodesFilter.empty())
    {
        m_nodeFilterItems = m_staticNodes.size();
        m_lastNodeFilterList.clear();
    }
    else if (isFilterChanged) //filter changed. resetting list of matching node names
    {
        m_nodeFilterItems = 0;
        m_lastNodeFilterList.clear();

        for (auto& [name, ImG_NodeStruct] : m_staticNodes)
        {
            //filtering marked for deletion or not matching filter nodes
            if (
                ImG_NodeStruct.markedForDeletion
                || !ND_Util::checkFilterConditions(name, m_nodesFilter)
                )
                m_lastNodeFilterList.emplace(name);
        }
        m_nodeFilterItems = m_staticNodes.size() - m_lastNodeFilterList.size();
    }

    ImGui::SameLine();
    ImGui::Text("Nodes shown: %d", m_nodeFilterItems);

    if (ImGui::BeginChild("Filtered Nodes"))
    {
        for (auto& [name, ImG_NodeStruct] : m_staticNodes)
        {
            if (ImG_NodeStruct.markedForDeletion || m_lastNodeFilterList.contains(name))
                continue;

            //Node item render code
            if (ImGui::TreeNode(name.c_str()))
            {
                ImGui::SameLine();
                if (ImGui::Button("x")) //Button for deleting node
                {
                    m_staticNodes[name].markedForDeletion = true;
                }

                bool needsDeletion = renderNodeVariant(ImG_NodeStruct.node, name);
                if (needsDeletion) m_staticNodes[name].markedForDeletion = true;

                ImGui::TreePop();
            }
        }
    }
    ImGui::EndChild(); //ends nodes list scrollable zone

    std::erase_if(m_staticNodes,
        [](const auto& record)
        {
            return record.second.markedForDeletion;
        });
}

void NodesTab::onGuiHierarchy()
{
    //rendering hierarchy with selectables
    static size_t indent; indent = 0;
    //std::map<size_t, std::pair<size_t, size_t>> indent_item_counter; //indent level - current element in indent level - total elements in indent level
    //indent_item_counter[0] = {0,0}; //root node is the only one on zero indent
    static std::unordered_multiset<std::string> uniqueNames;
    uniqueNames.reserve(1000);
    uniqueNames.clear();

    auto hierarchyViewSize = ND_Util::getSizeForChild(0.3);
    hierarchyViewSize.x = -1.f;
    if (ImGui::BeginChild("Hierarchy", hierarchyViewSize, true, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static std::function<void(const ImG_Node::NodeVariantHolder&, const std::string&)> hierarchy_handler =
            [this]
            (const ImG_Node::NodeVariantHolder& _hierarchyObject, const std::string& _nameOverride = "")
            {
                std::string name = (_nameOverride.empty()) ?
                                    (getVariantNodeName(_hierarchyObject)) :
                                    _nameOverride;
                if (!uniqueNames.contains(name))
                    uniqueNames.emplace(name);
                else
                {
                    uniqueNames.emplace(name);
                    name.append("[" + STR(uniqueNames.count(name))+"]");
                }

                int indentSpaces = std::max(0, CastI(indent)*2 - 1);
                std::string indentation_prefix(indentSpaces, ' '); //double space per each indent level minus one
                ImGui::Text("%s", indentation_prefix.c_str());
                ImGui::SameLine();

                //Selection mechanic:
                auto variantNode = castVariantToNode(_hierarchyObject);
                const auto& children = variantNode->getChildren();
                std::multimap<std::string, ImG_Node::NodeVariantHolder> childrenSorted;
                for (const auto& child : children)
                    childrenSorted.emplace(child->getName(), child);

                bool unfoldedChildren = false;
                static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

                ImGuiTreeNodeFlags nodeFlags = base_flags;
                if (castVariantToNode(_hierarchyObject) == castVariantToNode(m_lastHierarchyNode)) //is selected
                    nodeFlags |= ImGuiTreeNodeFlags_Selected;
                if (children.empty()) //a leaf - no arrow for tree unfolding
                    nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

                if (!variantNode->isEnabled())
                    ImGui::PushStyleColor(ImGuiCol_Text, IMG_DISABLED);

                //render node name and subtree arrow if there are children. Returns bool - if tree is open or close
                unfoldedChildren = ImGui::TreeNodeEx(name.c_str(), nodeFlags);

                if (!variantNode->isEnabled())
                    ImGui::PopStyleColor(); // restore regular text color

                unfoldedChildren = unfoldedChildren && !children.empty();

                // Item clicked - select item
                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                {
                    m_lastHierarchyNode = _hierarchyObject;
                    //info("[Selected] %s", getVariantTypeInfo(_hierarchyObject).c_str());
                }

                //adding indent
                indent++;
                if (unfoldedChildren)
                {
                    //recursive call to children
                    for (const auto& [name, child] : childrenSorted)
                    {
                        hierarchy_handler(child, "");
                    }
                    ImGui::TreePop();
                }

                //finished working with direct children, removing indent
                indent--;
            }; //end of hierarchy_handler description

        hierarchy_handler(Render::getRoot(), "Root");
        if (Render::getHudPass().isEnabled())
            hierarchy_handler(Render::getHudPass().getRoot(), "HUD Root");
    }
    ImGui::EndChild();

    //Render selected node
    if (ImGui::BeginChild("Selected Node", ImVec2(-1.f, -1.f), true, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (isNodeVariantExpired(m_lastHierarchyNode))
        {
            ImGui::TextColored(IMG_RED, "%s", "No valid node selected");
            ImGui::EndChild();
            return;
        }

        renderNodeVariant(m_lastHierarchyNode, getVariantNodeName(m_lastHierarchyNode));
    }
    ImGui::EndChild(); //ends nodes list scrollable zone
}

std::string NodesTab::getVariantTypeInfo(ImG_Node::NodeVariantHolder node)
{
    std::string tInfo;
    // Handler for ImG_Node.node variant field
    Visitor visitor = {
        [this, &tInfo](wptr<Spine>& nodeW)
        {
            tInfo = nodeW.lock()->getTypeInfo().name();
        },
        [this, &tInfo](wptr<Mesh>& nodeW)
        {
            tInfo = nodeW.lock()->getTypeInfo().name();
        },
        [this, &tInfo](wptr<ParticlesEmitter>& nodeW)
        {
            tInfo = nodeW.lock()->getTypeInfo().name();
        },
        [this, &tInfo](wptr<Node>& nodeW)
        {
            tInfo = nodeW.lock()->getTypeInfo().name();
        }
    };

    std::visit(visitor, node); //handles ImG_Node.node using appropriate lambda above
    return tInfo;
}

#endif //FORCENODEDEBUG_OFF
