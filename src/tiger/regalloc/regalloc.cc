#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"

#define DEBUGx

#ifdef DEBUG
  #define regalloc_log(fmt, args...) \
    do { \
      printf("[REGALLOC_LOG][%s:%d->%s]" fmt "\n", \
              __FILE__, __LINE__, __FUNCTION__, ##args); \
    } while (0);
#else
  #define regalloc_log(fmt, args...) \
    do {} while (0);
#endif

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */

RegAllocator::RegAllocator(frame::Frame *frame_, std::unique_ptr<cg::AssemInstr> assem_instr_)
    : frame(frame_), assem_instr(std::move(assem_instr_)) {
    result = std::make_unique<Result>();
}

std::unique_ptr<Result> RegAllocator::TransferResult() {
    result->il_ = assem_instr->GetInstrList();
    result->coloring_ = temp::Map::Empty();
    regalloc_log("map color size: %d", color.size());
    for (auto node : color) {
        regalloc_log("temp: %d, string: %s", node.first->NodeInfo()->Int(), (*node.second).c_str());
        result->coloring_->Enter(node.first->NodeInfo(), node.second);
    }
    return std::move(result);
}

void RegAllocator::RegAlloc() {
    Build();
    MakeWorkList();
    while (!simplifyWorklist->GetList().empty() || !worklistMoves->GetList().empty() ||
            !freezeWorklist->GetList().empty() || !spillWorklist->GetList().empty()) {
        if (!simplifyWorklist->GetList().empty()) Simplify();
        else if (!worklistMoves->GetList().empty()) Coalesce();
        else if (!freezeWorklist->GetList().empty()) Freeze();
        else if (!spillWorklist->GetList().empty()) SelectSpill();
        regalloc_log("in loop!");
    }
    AssignColors();
    if (!spilledNodes->GetList().empty()) {
        RewriteProgram();
        RegAlloc();
    }
    
}

void RegAllocator::Build() {
    /* Liveness Analysis */
    fg::FlowGraphFactory *flowGraphFactory = new fg::FlowGraphFactory(this->assem_instr->GetInstrList());
    flowGraphFactory->AssemFlowGraph();
    fg::FGraphPtr flowGraph = flowGraphFactory->GetFlowGraph();
    live_graph_factory = new live::LiveGraphFactory(flowGraph);
    live_graph_factory->Liveness();
    live_graph = live_graph_factory->GetLiveGraph();
    auto temp_node_map = live_graph_factory->GetTempNodeMap();

    /* Debug */
    ShowStatus();
    
    /* Initialize some variables */
    simplifyWorklist = new live::INodeList();
    freezeWorklist = new live::INodeList();
    spillWorklist = new live::INodeList();
    spilledNodes = new live::INodeList();
    coalescedNodes = new live::INodeList();
    coloredNodes = new live::INodeList();
    selectStack = new live::INodeList();

    coalescedMoves = new live::MoveList();
    constrainedMoves = new live::MoveList();
    frozenMoves = new live::MoveList();
    worklistMoves = live_graph->moves;
    activeMoves = new live::MoveList();

    /* Initialize color & coloredNodes */
    for (auto reg : reg_manager->Registers()->GetList()) {
        live::INodePtr node = temp_node_map->Look(reg);
        regalloc_log("machine regs: %d", node->NodeInfo()->Int());
        coloredNodes->Append(node);
        color[node] = reg_manager->temp_map_->Look(reg);
    }

    /* Initialize degree & moveList */
    for (auto node : live_graph->interf_graph->Nodes()->GetList()) {
        /* initialize degree */
        degree[node] = node->Degree();
        /* Initialize moveList */
        live::MoveList *node_move_list = new live::MoveList();
        for (auto move_pair : worklistMoves->GetList()) {
            if (move_pair.first == node || move_pair.second == node)
                node_move_list->Append(move_pair.first, move_pair.second);
        }
        moveList[node] = node_move_list;
    }
}

void RegAllocator::AddEdge(live::INodePtr u, live::INodePtr v) {
    if (!u->Adj(v) && u != v) {
        live_graph->interf_graph->AddEdge(u, v);
        live_graph->interf_graph->AddEdge(v, u);
        degree[u]++;
        degree[v]++;
    }
}

void RegAllocator::MakeWorkList() {
    auto nodes = live_graph->interf_graph->Nodes();
    for (auto node : nodes->GetList()) {
        if (!coloredNodes->Contain(node)) {
            if (degree[node] >= K) spillWorklist->Append(node);
            else if (MoveRelated(node)) freezeWorklist->Append(node);
            else simplifyWorklist->Append(node);
        }
    }
}

live::INodeListPtr RegAllocator::Adjacent(live::INodePtr n) {
    /* adjList[n] \ (selectStack union coalescedNodes)*/
    return n->Adj()->Diff(selectStack->Union(coalescedNodes));
}

live::MoveList *RegAllocator::NodeMoves(live::INodePtr n) {
    /* moveList[n] intersect (activeMoves union worklistMoves)*/
    return moveList[n]->Intersect(activeMoves->Union(worklistMoves));
}

bool RegAllocator::MoveRelated(live::INodePtr n) {
    /* NodeMoves != {} */
    return !NodeMoves(n)->GetList().empty();
}

void RegAllocator::Simplify() {
    regalloc_log("Simplify");
    auto node = simplifyWorklist->GetList().front();
    simplifyWorklist->DeleteNode(node);
    selectStack->Prepend(node);
    for (auto adj_node : Adjacent(node)->GetList())
        DecrementDegree(adj_node);
}

void RegAllocator::DecrementDegree(live::INodePtr m) {
    int m_degree = degree[m];
    degree[m] = m_degree - 1;
    if (m_degree == K) {
        live::INodeListPtr m_adj_list = Adjacent(m);
        m_adj_list->Append(m);
        EnableMoves(m_adj_list);
        spillWorklist->DeleteNode(m);
        if (MoveRelated(m)) freezeWorklist->Append(m);
        else simplifyWorklist->Append(m);
    }
}

void RegAllocator::EnableMoves(live::INodeListPtr nodes) {
    for (auto node : nodes->GetList()) {
        for (auto move_node : NodeMoves(node)->GetList()) {
            if (activeMoves->Contain(move_node.first, move_node.second)) {
                /* activeMoves <- activeMoves \ {move_node} */
                activeMoves->Delete(move_node.first, move_node.second);
                // activeMoves->Delete(move_node.second, move_node.first);
                /* worklistMoves <- worklistMoves union {move_node} */
                worklistMoves->Append(move_node.first, move_node.second);
                // worklistMoves->Append(move_node.second, move_node.first);
            }
        }
    }
}

void RegAllocator::Coalesce() {
    regalloc_log("Coalesce");
    auto node_pair = worklistMoves->GetList().front();
    live::INodePtr x = GetAilas(node_pair.first);
    live::INodePtr y = GetAilas(node_pair.second);
    live::INodePtr u, v;
    if (coloredNodes->Contain(y)) {
        u = y;
        v = x;
    }
    else {
        u = x;
        v = y;
    }
    worklistMoves->Delete(node_pair.first, node_pair.second);
    if (u == v) {
        coalescedMoves->Append(node_pair.first, node_pair.second);
        AddWorkList(u);
    }
    else if (coloredNodes->Contain(v) || u->Adj(v)) {
        constrainedMoves->Append(node_pair.first, node_pair.second);
        AddWorkList(u);
        AddWorkList(v);
    }
    else if (coloredNodes->Contain(u) && OK(v, u) ||
            !coloredNodes->Contain(u) && Conservative(Adjacent(u)->Union(Adjacent(v)))) {
        coalescedMoves->Append(node_pair.first, node_pair.second);
        Combine(u, v);
        AddWorkList(u);
    }
    else activeMoves->Append(node_pair.first, node_pair.second);
    
}

void RegAllocator::AddWorkList(live::INodePtr u) {
    if (!coloredNodes->Contain(u) && !MoveRelated(u) && degree[u] < K) {
        freezeWorklist->DeleteNode(u);
        simplifyWorklist->Append(u);
    }
}

bool RegAllocator::OK(live::INodePtr t, live::INodePtr r) {
    auto adj_t = Adjacent(t);
    bool flag = true;
    for (auto adj_node : adj_t->GetList()) {
        if (degree[adj_node] < K || coloredNodes->Contain(adj_node) 
            || adj_node->Adj(r)) continue;
        else {
            flag = false;
            break;
        }
    }
    return flag;
}

bool RegAllocator::Conservative(live::INodeListPtr nodes) {
    int k = 0;
    for (auto node : nodes->GetList()) {
        if (degree[node] >= K) k++;
    }
    return (k < K);
}

live::INodePtr RegAllocator::GetAilas(live::INodePtr node) {
    // regalloc_log("GetAilas");
    if (coalescedNodes->Contain(node))
        return GetAilas(alias[node]);
    else return node;
}

void RegAllocator::Combine(live::INodePtr u, live::INodePtr v) {
    if (freezeWorklist->Contain(v)) freezeWorklist->DeleteNode(v);
    else spillWorklist->DeleteNode(v);
    coalescedNodes->Append(v);
    alias[v] = u;
    moveList[u] = moveList[u]->Union(moveList[v]);
    live::INodeListPtr node_list = new live::INodeList();
    node_list->Append(v);
    EnableMoves(node_list);
    for (auto node : Adjacent(v)->GetList()) {
        AddEdge(node, u);
        DecrementDegree(node);
    }
    if (degree[u] >= K && freezeWorklist->Contain(u)) {
        freezeWorklist->DeleteNode(u);
        spillWorklist->Append(u);
    }


}

void RegAllocator::Freeze() {
    regalloc_log("Freeze");
    auto u = freezeWorklist->GetList().front();
    freezeWorklist->DeleteNode(u);
    simplifyWorklist->Append(u);
    FreezeMoves(u);
}

void RegAllocator::FreezeMoves(live::INodePtr u) {
    for (auto node_pair : NodeMoves(u)->GetList()) {
        live::INodePtr v;
        if (GetAilas(node_pair.second) == GetAilas(u))
            v = GetAilas(node_pair.first);
        else v = GetAilas(node_pair.second);
        activeMoves->Delete(node_pair.first, node_pair.second);
        frozenMoves->Append(node_pair.first, node_pair.second);
        if (NodeMoves(v)->GetList().empty() && degree[v] < K) {
            freezeWorklist->DeleteNode(v);
            simplifyWorklist->Append(v);
        }
    }
}

void RegAllocator::SelectSpill() {
    regalloc_log("SelectSpill");
    auto m = spillWorklist->GetList().front();
    spillWorklist->DeleteNode(m);
    simplifyWorklist->Append(m);
    FreezeMoves(m);
}

void RegAllocator::AssignColors() {
    while (!selectStack->GetList().empty()) {
        regalloc_log("in assign color loop!")
        auto n = selectStack->GetList().front();
        selectStack->DeleteNode(n);
        std::list<std::string *> okColors;
        for (auto reg : reg_manager->Registers()->GetList()) {
            okColors.push_back(reg_manager->temp_map_->Look(reg));
        }
        for (auto w : n->Succ()->GetList()) {
            if (coloredNodes->Contain(GetAilas(w))) {
                /* okColors <- okColors \ {color[GetAlias(w)]} */
                auto ok_color_it = okColors.begin();
                auto end_flag = okColors.end();
                for (; ok_color_it != end_flag; ++ok_color_it)
                    if (*ok_color_it == color[GetAilas(w)]) break;
                if (ok_color_it != end_flag) okColors.erase(ok_color_it);
            }
        }
        if (okColors.empty())
            spilledNodes->Append(n);
        else {
            coloredNodes->Append(n);
            auto c = okColors.front();
            color[n] = c;
        }
    }
    
    for (auto n : coalescedNodes->GetList()) 
        color[n] = color[GetAilas(n)];
    
}

void RegAllocator::RewriteProgram() {
    regalloc_log("Rewrite Program!");
    /* Alloc stack space for every spilledNodes */
    std::map<temp::Temp *, frame::Access *> temp_access_map;
    for (auto v : spilledNodes->GetList()) {
        frame::Access *new_frame = this->frame->AllocLocal(true);
        temp_access_map[v->NodeInfo()] = new_frame;
    }
    /* Update assem_instr */
    assem::InstrList *new_assem_list = new assem::InstrList();
    for (auto instr : assem_instr->GetInstrList()->GetList()) {
        /* Check if both def&use are NULL */
        temp::TempList *def_temp_list = instr->Def();
        temp::TempList *use_temp_list = instr->Use();
        if (def_temp_list == NULL && use_temp_list == NULL) continue;

        temp::Temp *frame_temp = temp::TempFactory::NewTemp();
        temp::TempList *new_def_list = new temp::TempList();
        temp::TempList *new_use_list = new temp::TempList();
        assem::InstrList *def_assem_list = new assem::InstrList();
        assem::InstrList *use_assem_list = new assem::InstrList();
        /* If instr->Def() contains vi */
        if (def_temp_list != NULL) {
            for (auto def_temp : def_temp_list->GetList()) {
                /* Spilled nodes */
                if (temp_access_map.count(def_temp) != 0) {
                    frame::InFrameAccess *def_temp_access = (frame::InFrameAccess *) temp_access_map[def_temp];
                    temp::Temp *new_temp = temp::TempFactory::NewTemp();
                    def_assem_list->Append(
                        new assem::OperInstr(
                            "movq `s0," + std::to_string(def_temp_access->offset) + "(`s1)",
                            NULL, new temp::TempList{new_temp, frame_temp},
                            NULL
                        )
                    );
                    new_def_list->Append(new_temp);
                }
                /* Not spilled nodes */
                else {
                    new_def_list->Append(def_temp);
                }
            }
            
        }
        /* If instr->Use() contains vi */   
        if (use_temp_list != NULL) {
            for (auto use_temp : use_temp_list->GetList()) {
                /* Spilled nodes */
                if (temp_access_map.count(use_temp) != 0) {
                    frame::InFrameAccess *use_temp_access = (frame::InFrameAccess *) temp_access_map[use_temp];
                    temp::Temp *new_temp = temp::TempFactory::NewTemp();
                    use_assem_list->Append(
                        new assem::OperInstr(
                            "movq " + std::to_string(use_temp_access->offset) + "(`s0),`d0",
                            new temp::TempList{new_temp}, new temp::TempList{frame_temp},
                            NULL
                        )
                    );
                    new_use_list->Append(new_temp);
                }
                /* Not spilled nodes */
                else {
                    new_use_list->Append(use_temp);
                }
            }
        }
        /* Insert new instrs to assem_instr */
        /* 1. Not spilled nodes */
        if (def_assem_list->GetList().size() == 0 
            && use_assem_list->GetList().size() == 0) {
            new_assem_list->Append(instr);
        } 
        /* 2. Spilled nodes */
        else {
            /* Assign value to frame_temp */
            new_assem_list->Append(
                new assem::OperInstr(
                    "leaq " + frame->name_->Name() + "_framesize(%rsp),`d0",
                    new temp::TempList{frame_temp}, NULL,
                    NULL
                )
            );
            /* Append instrs in use_assem_list */
            for (auto use_assem_instr : use_assem_list->GetList())
                new_assem_list->Append(use_assem_instr);
            /* Append current instr */
            if (typeid(*instr) == typeid(assem::OperInstr)) {
                assem::OperInstr *oper_instr = (assem::OperInstr *) instr;
                oper_instr->src_ = use_temp_list;
                oper_instr->dst_ = def_temp_list;
            }
            else if (typeid(*instr) == typeid(assem::MoveInstr)) {
                assem::MoveInstr *move_instr = (assem::MoveInstr *) instr;
                move_instr->src_ = use_temp_list;
                move_instr->dst_ = def_temp_list;
            }
            else {
                regalloc_log("instr can't be label type!");
                assert(0);
            }
            new_assem_list->Append(instr);
            /* Append instrs in def_assem_list */
            for (auto def_assem_instr : def_assem_list->GetList())
                new_assem_list->Append(def_assem_instr);
        }
        /* Realloc some variables */
        delete new_def_list;
        delete new_use_list;
        delete def_assem_list;
        delete use_assem_list;
    }
    /* Reinitialize assem_instr */
    assem_instr.reset();
    assem_instr = std::move(std::make_unique<cg::AssemInstr>(new_assem_list));
    /* Realloc some variables */
    delete live_graph_factory;
    delete live_graph;
    delete simplifyWorklist;
    delete freezeWorklist;
    delete spillWorklist;
    delete spilledNodes;
    delete coalescedNodes;
    delete coloredNodes;
    delete selectStack;
    delete constrainedMoves;
    delete frozenMoves;
    delete worklistMoves;
    delete activeMoves;
    degree.clear();
    moveList.clear();
    alias.clear();
    color.clear();

}

void RegAllocator::ShowStatus() {
    regalloc_log("***********Show Status*************");
    auto inode_list = live_graph->interf_graph->Nodes()->GetList();
    regalloc_log("***Interferece graph***");
    for (auto node : inode_list) {
        int temp_no = node->NodeInfo()->Int();
        std::string interf_str = "";
        for (auto interf_node : node->Succ()->GetList()) 
            interf_str += " " + std::to_string(interf_node->NodeInfo()->Int());
        regalloc_log("temp: %d interferece with%s", temp_no, interf_str.c_str());   
    }
    regalloc_log("**********************");
    regalloc_log("***Move list***");
    auto inode_moves = live_graph->moves;
    for (auto pair : inode_moves->GetList()) {
        int temp_first_no = pair.first->NodeInfo()->Int();
        int temp_second_no = pair.second->NodeInfo()->Int();
        regalloc_log("%d -> %d", temp_first_no, temp_second_no);
    }
    regalloc_log("**********************");
    regalloc_log("***********************************");
}

} // namespace ra