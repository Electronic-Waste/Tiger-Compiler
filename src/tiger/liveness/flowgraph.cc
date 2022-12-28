#include "tiger/liveness/flowgraph.h"

namespace fg {

void FlowGraphFactory::AssemFlowGraph() {
  /* TODO: Put your lab6 code here */
  auto instr_list = this->instr_list_->GetList();
  FNodePtr last_node = NULL;
  std::list<std::pair<FNodePtr, assem::Targets *>> jump_list;
  for (assem::Instr *instr : instr_list) {
    FNodePtr new_node = flowgraph_->NewNode(instr);
    if (last_node)
      flowgraph_->AddEdge(last_node, new_node);
    /* If it's a label instr, add to label-Node map*/
    if (typeid(*instr) == typeid(assem::LabelInstr)) {
      last_node = new_node;
      label_map_->Enter(((assem::LabelInstr *) instr)->label_, new_node);
    }
    /* If it's a jump/Cjump instr, add to tempory jump_list */
    else if (typeid(*instr) == typeid(assem::OperInstr) && ((assem::OperInstr *) instr)->jumps_) {
      last_node = NULL;
      assem::Targets *jump_targets = ((assem::OperInstr *) instr)->jumps_;
      jump_list.emplace_back(new_node, jump_targets);
    }
    /* Else just update last_node */
    else {
      last_node = new_node;
    }
  }
  /* Find label's node, and addEdge from jump's node to label's node */
  for (auto jump_node : jump_list) {
    std::vector<temp::Label *> *labels = jump_node.second->labels_;
    for (auto label : *labels) {
      FNodePtr target_node = label_map_->Look(label);
      flowgraph_->AddEdge(jump_node.first, target_node);
    }
  }
}

} // namespace fg

namespace assem {

temp::TempList *LabelInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return NULL;
}

temp::TempList *MoveInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return this->dst_;
}

temp::TempList *OperInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return this->dst_;
}

temp::TempList *LabelInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return NULL;
}

temp::TempList *MoveInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return this->src_;
}

temp::TempList *OperInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return this->src_;
}
} // namespace assem
