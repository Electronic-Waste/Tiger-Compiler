#include "tiger/frame/x64frame.h"
#include <map>

extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */
X64RegManager::X64RegManager() {
  std::map<int, std::string *> reg_map = std::map<int, std::string *>{
    {0, new std::string("%rax")}, {1, new std::string("%rbx")},
    {2, new std::string("%rcx")}, {3, new std::string("%rdx")},
    {4, new std::string("%rsi")}, {5, new std::string("%rdi")},
    {6, new std::string("%rbp")}, {7, new std::string("%rsp")},
    {8, new std::string("%r8")}, {9, new std::string("%r9")},
    {10, new std::string("%r10")}, {11, new std::string("%r11")},
    {12, new std::string("%r12")}, {13, new std::string("%r13")},
    {14, new std::string("%r14")}, {15, new std::string("%r15")}
  };

  for (int i = 0; i < 16; i++) {
    temp::Temp *new_temp = temp::TempFactory::NewTemp();
    regs_.push_back(new_temp);
    temp_map_->Enter(new_temp, reg_map[i]);
  }
}

temp::TempList *X64RegManager::Registers() {
  temp::TempList *tempList = new temp::TempList();
  for (temp::Temp *temp: regs_)
    tempList->Append(temp);
  return tempList;
}

temp::TempList *X64RegManager::ArgRegs() {
  temp::TempList *tempList = new temp::TempList();
  tempList->Append(regs_[5]);
  tempList->Append(regs_[4]);
  tempList->Append(regs_[3]);
  tempList->Append(regs_[2]);
  tempList->Append(regs_[8]);
  tempList->Append(regs_[9]);
  return tempList;
}

temp::TempList *X64RegManager::CallerSaves() {
  temp::TempList *tempList = new temp::TempList();
  tempList->Append(regs_[0]);
  tempList->Append(regs_[2]);
  tempList->Append(regs_[3]);
  tempList->Append(regs_[4]);
  tempList->Append(regs_[5]);
  tempList->Append(regs_[8]);
  tempList->Append(regs_[9]);
  tempList->Append(regs_[10]);
  tempList->Append(regs_[11]);
  return tempList;
}

temp::TempList *X64RegManager::CalleeSaves() {
  temp::TempList *tempList = new temp::TempList();
  tempList->Append(regs_[1]);
  tempList->Append(regs_[6]);
  tempList->Append(regs_[12]);
  tempList->Append(regs_[13]);
  tempList->Append(regs_[14]);
  tempList->Append(regs_[15]);
  return tempList;
}

temp::TempList *X64RegManager::ReturnSink() {
  temp::TempList *tempList = CalleeSaves();
  tempList->Append(ReturnValue());
  tempList->Append(StackPointer());
  return tempList;
}

int X64RegManager::WordSize() {
  return 8;
}

temp::Temp *X64RegManager::FramePointer() {
  return regs_[6];
}

temp::Temp *X64RegManager::StackPointer() {
  return regs_[7];
}

temp::Temp *X64RegManager::ReturnValue() {
  return regs_[0];
}

tree::Exp *InFrameAccess::ToExp(tree::Exp *framePtr) const {
  return new tree::MemExp(
    new tree::BinopExp(tree::BinOp::PLUS_OP,
    framePtr, new tree::ConstExp(offset))
  );
}

tree::Exp *InRegAccess::ToExp(tree::Exp *framePtr) const {
  return new tree::TempExp(reg);
}

X64Frame::X64Frame(temp::Label *name, std::list<bool> *formals) {
  
  /* Initialize some variables in the class */
  this->current_stack_pos = -8;
  name_ = name;
  moves_ = new std::list<tree::Stm *>;
  formals_ = new std::list<frame::Access *>;

  /* Check formals == NULL ? */
  if (formals == NULL) return;

  /* Alloc local variable for formals */
  for (bool escape : *formals) 
    formals_->push_back(AllocLocal(escape));
    
  /* View Shift: Move params to corresponding pos */
  tree::TempExp *frame_ptr = new tree::TempExp(reg_manager->FramePointer());
  int params_passed = 0;
  for (frame::Access *access : *formals_) {
    tree::MoveStm *move_stm;
    /* Params passed in registers */
    /* ArgRegs: %rdi, %rsi, %rdx, %rcx, %r8, %r9 */
    if (params_passed < 6) {
      move_stm = new tree::MoveStm(
        access->ToExp(frame_ptr),
        new tree::TempExp(reg_manager->ArgRegs()->NthTemp(params_passed))
      );
      params_passed++;
    }
    /* Params passed in stack */
    /* In x86-64, staticlink is pushed to stack, which is adjacent to first param in stack */
    else {
      move_stm = new tree::MoveStm(
        access->ToExp(frame_ptr),
        new tree::MemExp(
          new tree::BinopExp(
            tree::BinOp::PLUS_OP,
            frame_ptr,
            new tree::ConstExp(reg_manager->WordSize() * (params_passed - 6 + 1))
          )
        )
      );
      params_passed++;
    }
    moves_->push_back(move_stm);
  }
}

frame::Access *X64Frame::AllocLocal(bool isEscape) {
  frame::Access *access;
  if (isEscape) {
    access = new InFrameAccess(this->current_stack_pos);
    this->current_stack_pos -= 8;
  }
  else {
    access = new InRegAccess(temp::TempFactory::NewTemp());
  }
  return access;
}

tree::Exp *ExternelCall(std::string func_name, tree::ExpList *args) {
  temp::Label *func_label = temp::LabelFactory::NamedLabel(func_name);
  return new tree::CallExp(new tree::NameExp(func_label), args);
}

tree::Stm *ProcEntryExit1(frame::Frame *frame, tree::Stm *stm) {
  /* Empty moves_ means that we have reached the "tigermain" level.
     There is no view shift and we just need to address return stm */
  if (frame->moves_->size() == 0) return stm;

  auto stm_it = frame->moves_->begin();
  auto end_it = frame->moves_->end();
  auto seq_stm = *stm_it;
  stm_it++;
  for (; stm_it != end_it; ++stm_it)
    seq_stm = new tree::SeqStm(seq_stm, *stm_it);
  return new tree::SeqStm(seq_stm, stm);
}

assem::InstrList *ProcEntryExit2(assem::InstrList *body) {
  body->Append(new assem::OperInstr("", new temp::TempList(), reg_manager->ReturnSink(), NULL));
  return body;
}

assem::Proc *ProcEntryExit3(frame::Frame *frame, assem::InstrList *body) {
  char buf[100];
  sprintf(
    buf,
    "PROCEDURE %s\n",
    temp::LabelFactory::LabelString(frame->name_).data()
  );
  return new assem::Proc(std::string(buf), body, "END\n");
}

} // namespace frame
