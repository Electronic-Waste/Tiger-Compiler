#include "tiger/liveness/liveness.h"

#define DEBUG

#ifdef DEBUG
  #define liveness_log(fmt, args...) \
    do { \
      printf("[LIVENESS_LOG][%s:%d->%s]" fmt "\n", \
              __FILE__, __LINE__, __FUNCTION__, ##args); \
    } while (0);
#else
  #define liveness_log(fmt, args...) \
    do {} while (0);
#endif

extern frame::RegManager *reg_manager;

namespace live {

bool MoveList::Contain(INodePtr src, INodePtr dst) {
  return std::any_of(move_list_.cbegin(), move_list_.cend(),
                     [src, dst](std::pair<INodePtr, INodePtr> move) {
                       return move.first == src && move.second == dst;
                     });
}

void MoveList::Delete(INodePtr src, INodePtr dst) {
  assert(src && dst);
  auto move_it = move_list_.begin();
  for (; move_it != move_list_.end(); move_it++) {
    if (move_it->first == src && move_it->second == dst) {
      break;
    }
  }
  move_list_.erase(move_it);
}

MoveList *MoveList::Union(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : move_list_) {
    res->move_list_.push_back(move);
  }
  for (auto move : list->GetList()) {
    if (!res->Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

MoveList *MoveList::Intersect(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : list->GetList()) {
    if (Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

bool Contain(temp::Temp *reg, temp::TempList *list) {
  if (list == NULL) return false;
  std::list<temp::Temp *> tmp = list->GetList();
  bool is_contain = false;
  for (auto t : tmp) {
    if (reg == t) {
      is_contain = true;
      break;
    }
  }
  return is_contain;
}

temp::TempList *Subtract(temp::TempList *src, temp::TempList *dst) {
  temp::TempList *result = new temp::TempList();
  if (dst == NULL) return result;
  for (auto t_dst : dst->GetList()) {
    if (!Contain(t_dst, src))
      result->Append(t_dst);
  }
  return result;
}

temp::TempList *Union(temp::TempList *list1, temp::TempList *list2) {
  temp::TempList *result = new temp::TempList();
  if (list2 != NULL) {
    for (auto t : list2->GetList())
      result->Append(t);
  }
  if (list1 != NULL) {
    for (auto t : list1->GetList()) {
      if (!Contain(t, list2))
        result->Append(t);
    }
  }
  return result;
}

bool Equal(temp::TempList *list1, temp::TempList *list2) {
  std::list<temp::Temp *> tmp1 = list1->GetList();
  std::list<temp::Temp *> tmp2 = list2->GetList();
  /* Check size first */
  if (tmp1.size() != tmp2.size()) return false;
  /* Then compare each item in list1 and list2 */
  for (auto t1 : tmp1) {
    bool is_find = false;
    for (auto t2 : tmp2) {
      if (t1 == t2) {
        is_find = true;
        break;
      }
    }
    if (!is_find) return false;
  }
  return true;
}

void LiveGraphFactory::ShowInOut() {
  liveness_log("******* ShowInOut ********")
  int cnt = 0;
  for (auto flow_node : flowgraph_->Nodes()->GetList()) {
    temp::TempList *in_list = in_->Look(flow_node);
    temp::TempList *out_list = out_->Look(flow_node);
    std::string in_str = "";
    std::string out_str = "";
    std::string use_str = "";
    std::string def_str = "";
    ShowInstr(flow_node->NodeInfo());
    for (auto in_temp : in_list->GetList())
      in_str += " " + std::to_string(in_temp->Int());
    for (auto out_temp : out_list->GetList())
      out_str += " " + std::to_string(out_temp->Int());
    if (flow_node->NodeInfo()->Use() != NULL) {
      for (auto use_temp : flow_node->NodeInfo()->Use()->GetList())
        use_str += " " + std::to_string(use_temp->Int());
    }
    if (flow_node->NodeInfo()->Def() != NULL) {
      for (auto def_temp : flow_node->NodeInfo()->Def()->GetList())
        def_str += " " + std::to_string(def_temp->Int());
    }
    liveness_log("in:%s", in_str.c_str());
    liveness_log("out:%s", out_str.c_str());
    liveness_log("def:%s", def_str.c_str());
    liveness_log("use:%s", use_str.c_str());
  }
  liveness_log("**************************");
}

void LiveGraphFactory::ShowInstr(assem::Instr *instr) {
  if (typeid(*instr) == typeid(assem::LabelInstr)) {
    assem::LabelInstr *to_label_instr = (assem::LabelInstr *) instr;
    liveness_log("label instr: %s", to_label_instr->assem_.c_str());
  }
  else if (typeid(*instr) == typeid(assem::MoveInstr)) {
    assem::MoveInstr *to_move_instr = (assem::MoveInstr *) instr;
    int def_size = 0;
    int use_size = 0;
    if (to_move_instr->Def() != NULL) def_size = to_move_instr->Def()->GetList().size();
    if (to_move_instr->Use() != NULL) use_size = to_move_instr->Use()->GetList().size();
    liveness_log("move instr: %s, def_size: %d, use_size: %d", 
                  to_move_instr->assem_.c_str(), def_size, use_size);
  }
  else {
    assem::OperInstr *to_oper_instr = (assem::OperInstr *) instr;
    int def_size = 0;
    int use_size = 0;
    if (to_oper_instr->Def() != NULL) def_size = to_oper_instr->Def()->GetList().size();
    if (to_oper_instr->Use() != NULL) use_size = to_oper_instr->Use()->GetList().size();
    liveness_log("oper instr: %s, def_size: %d, use_size: %d",
                  to_oper_instr->assem_.c_str(), def_size, use_size);
  }
}

void LiveGraphFactory::LiveMap() {
  /* TODO: Put your lab6 code here */
  /* in[n] <- {}; out[n] <- {} */
  for (auto flow_node : flowgraph_->Nodes()->GetList()) {
    in_->Enter(flow_node, new temp::TempList());
    out_->Enter(flow_node, new temp::TempList());
  }
  /* Start iteration */
  while (true) {
    bool is_all_same = true;
    // ShowInOut();
    for (auto flow_node_it : flowgraph_->Nodes()->GetList()) {
      temp::TempList *last_in = in_->Look(flow_node_it);
      temp::TempList *last_out = out_->Look(flow_node_it);
      /* Compute new_in */
      // liveness_log("compute new_in");
      temp::TempList *new_in_def = flow_node_it->NodeInfo()->Def();
      temp::TempList *new_in_use = flow_node_it->NodeInfo()->Use();
      temp::TempList *new_in = Union(new_in_use, Subtract(new_in_def, last_out));
      /* Compute new_out */
      // liveness_log("compute new_out");
      temp::TempList *new_out = new temp::TempList();
      for (auto succ_node : (*flow_node_it).Succ()->GetList()) {
        temp::TempList *succ_node_in = in_->Look(succ_node);
        temp::TempList *tmp = Union(new_out, succ_node_in);
        delete new_out;
        new_out = tmp;
      }
      /* If last_in != new_in or last_out != new_out, set is_all_same to false*/
      // liveness_log("assign value to is_same");
      if (!Equal(last_in, new_in) || !Equal(last_out, new_out)) is_all_same = false;
      /* Update in/out map */
      in_->Set(flow_node_it, new_in);
      out_->Set(flow_node_it, new_out);
      /* Dealloc last_in & last_out */
      delete last_in;
      delete last_out;
    }
    if (is_all_same) break;
  }

}

void LiveGraphFactory::InterfGraph() {
  /* TODO: Put your lab6 code here */
  /* Precolored registers interferce with each other */
  // ShowInOut();
  temp::TempList *all_regs = reg_manager->Registers();
  /* 1. Add machine registers to interf_graph */
  for (auto reg : all_regs->GetList()) {
    INodePtr new_node = live_graph_.interf_graph->NewNode(reg);
    temp_node_map_->Enter(reg, new_node);
  }
  /* 2. Add edges between machine registers */
  auto reg_start = all_regs->GetList().begin();
  auto end_flag = all_regs->GetList().end();
  for (auto reg_it = reg_start; reg_it != end_flag; ++reg_it) {
    for (auto reg_it_next = std::next(reg_it); reg_it_next != end_flag; ++reg_it_next) {
      INodePtr reg_it_node = temp_node_map_->Look(*reg_it);
      INodePtr reg_it_next_node = temp_node_map_->Look(*reg_it_next);
      live_graph_.interf_graph->AddEdge(reg_it_node, reg_it_next_node);
      live_graph_.interf_graph->AddEdge(reg_it_next_node, reg_it_node);
    }
  }
  /* Construct interf_graph among live temps (maybe there are machine registers) */
  std::list<fg::FNodePtr> flow_node_list = flowgraph_->Nodes()->GetList();
  INodePtr rsp_node = temp_node_map_->Look(reg_manager->StackPointer());
  /* 1. Create new node in interf_graph */
  for (auto flow_node : flow_node_list) {
    temp::TempList *out_temp_list = out_->Look(flow_node);
    for (auto out_temp : out_temp_list->GetList()) {
      INodePtr new_node = temp_node_map_->Look(out_temp);
      /* Check if node has been created */
      if (new_node == NULL) {
        new_node = live_graph_.interf_graph->NewNode(out_temp);
        temp_node_map_->Enter(out_temp, new_node);
        // liveness_log("Create INode for temp: %d", new_node->NodeInfo()->Int());
      }
      // else {
      //   liveness_log("INode for temp %d has already existed", new_node->NodeInfo()->Int());
      // }
      /* %rsp node interfereces with every live temps */
      if (out_temp != reg_manager->StackPointer()) {
        live_graph_.interf_graph->AddEdge(new_node, rsp_node);
        live_graph_.interf_graph->AddEdge(rsp_node, new_node);
      }
    }
  }
  /* 2. Add edges to the interf_graph, and push some node pairs to movelist */
  for (auto flow_node : flow_node_list) {
    assem::Instr *flow_node_instr = flow_node->NodeInfo();
    temp::TempList *out_temp_list = out_->Look(flow_node);
    /* Condition: instr_type != assem::MoveInstr */
    if (typeid(*flow_node_instr) != typeid(assem::MoveInstr)) {
      ShowInstr(flow_node_instr);
      temp::TempList *flow_node_def = flow_node_instr->Def();
      if (flow_node_def == NULL) continue;
      for (auto def : flow_node_def->GetList()) {
        for (auto out_temp : out_temp_list->GetList()) {
          INodePtr def_node = temp_node_map_->Look(def);
          INodePtr out_temp_node = temp_node_map_->Look(out_temp);
          // liveness_log("def_node: %d, out_node: %d", def_node->NodeInfo()->Int(), out_temp_node->NodeInfo()->Int());
          live_graph_.interf_graph->AddEdge(def_node, out_temp_node);
          live_graph_.interf_graph->AddEdge(out_temp_node, def_node);
        }
      }
    }
    /* Condition: instr_type == assem::MoveInstr */
    else {
      temp::TempList *flow_node_def = flow_node_instr->Def();
      temp::TempList *flow_node_use = flow_node_instr->Use();
      temp::TempList *out_subtract_use = Subtract(flow_node_use, out_temp_list);
      /* Add conflict edges */
      for (auto def : flow_node_def->GetList()) {
        INodePtr def_node = temp_node_map_->Look(def);
        for (auto temp_it : out_subtract_use->GetList()) {
          INodePtr temp_it_node = temp_node_map_->Look(temp_it);
          live_graph_.interf_graph->AddEdge(def_node, temp_it_node);
          live_graph_.interf_graph->AddEdge(temp_it_node, def_node);
        }
      }
      /* Add move edges */
      for (auto def : flow_node_def->GetList()) {
        INodePtr def_node = temp_node_map_->Look(def);
        for (auto use : flow_node_use->GetList()) {
          INodePtr use_node = temp_node_map_->Look(use);
          live_graph_.moves->Append(use_node, def_node);
        }
      }
    }
  }

}

void LiveGraphFactory::Liveness() {
  LiveMap();
  InterfGraph();
  ShowInOut();
}

} // namespace live
