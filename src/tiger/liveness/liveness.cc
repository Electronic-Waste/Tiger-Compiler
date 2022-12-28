#include "tiger/liveness/liveness.h"

extern frame::RegManager *reg_manager;

#define DEBUG
#ifdef DEBUG
  #define liveness_log(fmt, args...) \
    do { \
      printf("[LIVENESS_LOG][%s:%d->%s]" fmt "\n", \
              __FILE__, __LINE__, __FUCNTION__, ##args); \
    } while (0);
#else
  # define liveness_log(fmt, args...) \
    do {} while (0);
#endif

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
  for (auto t_dst : dst->GetList()) {
    if (!Contain(t_dst, src))
      result->Append(t_dst);
  }
  return result;
}

temp::TempList *Union(temp::TempList *list1, temp::TempList *list2) {
  temp::TempList *result;
  for (auto t : list2->GetList())
    result->Append(t);
  for (auto t : list1->GetList()) {
    if (!Contain(t, list2))
      result->Append(t);
  }
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
    for (auto flow_node_it : flowgraph_->Nodes()->GetList()) {
      temp::TempList *last_in = in_->Look(flow_node_it);
      temp::TempList *last_out = out_->Look(flow_node_it);
      /* Compute new_in */
      temp::TempList *new_in_def = flow_node_it->NodeInfo()->Def();
      temp::TempList *new_in_use = flow_node_it->NodeInfo()->Use();
      temp::TempList *new_in = Union(new_in_use, Subtract(new_in_def, last_out));
      /* Compute new_out */
      temp::TempList *new_out = new temp::TempList();
      for (auto succ_node : (*flow_node_it).Succ()->GetList()) {
        temp::TempList *succ_node_in = in_->Look(succ_node);
        temp::TempList *tmp = Union(new_out, succ_node_in);
        delete new_out;
        new_out = tmp;
      }
      /* If last_in != new_in or last_out != new_out, set is_all_same to false*/
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
  /* Construct interf_graph with live temps (maybe there are machine registers) */
  std::list<fg::FNodePtr> flow_node_list = flowgraph_->Nodes()->GetList();
  INodePtr rsp_node = temp_node_map_->Look(reg_manager->StackPointer());
  /* 1. Create new node in interf_graph */
  for (auto flow_node : flow_node_list) {
    temp::TempList *out_temp_list = out_->Look(flow_node);
    for (auto out_temp : out_temp_list->GetList()) {
      INodePtr new_node = temp_node_map_->Look(out_temp);
      /* Check if node has been created */
      if (new_node == NULL) 
        new_node = live_graph_.interf_graph->NewNode(out_temp);
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
      temp::TempList *flow_node_def = flow_node_instr->Def();
      if (flow_node_def == NULL) continue;
      for (auto def : flow_node_def->GetList()) {
        for (auto out_temp : out_temp_list->GetList()) {
          INodePtr def_node = temp_node_map_->Look(def);
          INodePtr out_temp_node = temp_node_map_->Look(out_temp);
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
}

} // namespace live
