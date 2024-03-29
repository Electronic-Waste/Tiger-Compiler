#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>

#define DEBUGx

#ifdef DEBUG
  #define codegen_log(fmt, args...) \
    do { \
      printf("[CODEGEN_LOG][%s:%d->%s]" fmt "\n", \
              __FILE__, __LINE__, __FUNCTION__, ##args); \
    } while (0);
#else
  #define codegen_log(fmt, args...) \
    do {} while (0);
#endif

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;


} // namespace

namespace cg {

void CodeGen::PushReg(assem::InstrList &instr_list, temp::Temp *dst_addr, temp::Temp *reg) {
  instr_list.Append(
    new assem::OperInstr(
      "movq `s0,(`d0)",
      new temp::TempList(dst_addr), 
      new temp::TempList(reg),
      NULL
    )
  );
  instr_list.Append(
    new assem::OperInstr(
      "subq $" + std::to_string(reg_manager->WordSize()) + ",`d0",
      new temp::TempList(dst_addr), NULL, 
      NULL
    )
  );
}

void CodeGen::PopReg(assem::InstrList &instr_list, temp::Temp *src_addr, temp::Temp *reg) {
  instr_list.Append(
    new assem::OperInstr(
      "movq (`s0),`d0",
      new temp::TempList(reg),
      new temp::TempList(src_addr),
      NULL
    )
  );
  instr_list.Append(
    new assem::OperInstr(
      "subq $" + std::to_string(reg_manager->WordSize()) + ",`d0",
      new temp::TempList(src_addr), NULL, 
      NULL
    )
  );
}

void CodeGen::StoreCalleeRegisters(assem::InstrList &instr_list, std::string_view fs) {
  temp::Temp *ptr_reg = temp::TempFactory::NewTemp();
  codegen_log("StoreCalleeRegister: %d", ptr_reg->Int());
  instr_list.Append(
    new assem::OperInstr(
      "leaq " + std::string(fs) + "_framesize(%rsp),`d0",
      new temp::TempList(ptr_reg),
      NULL, NULL
    )
  );
  instr_list.Append(
    new assem::OperInstr(
      "addq $" + std::to_string(this->frame_->current_stack_pos) + ",`d0",
      new temp::TempList(ptr_reg),
      new temp::TempList(ptr_reg),
      NULL
    )
  );

  temp::TempList *calleesaves = reg_manager->CalleeSaves();
  for (temp::Temp *t : calleesaves->GetList()) 
    PushReg(instr_list, ptr_reg, t);
  
}

void CodeGen::RestoreCalleeRegisters(assem::InstrList &instr_list, std::string_view fs) {
  temp::Temp *ptr_reg = temp::TempFactory::NewTemp();
  codegen_log("RestoreCalleeRegisters: %d", ptr_reg->Int());
  instr_list.Append(
    new assem::OperInstr(
      "leaq " + std::string(fs) + "_framesize(%rsp),`d0",
      new temp::TempList(ptr_reg),
      NULL, NULL
    )
  );
  instr_list.Append(
    new assem::OperInstr(
      "addq $" + std::to_string(this->frame_->current_stack_pos) + ",`d0",
      new temp::TempList(ptr_reg),
      new temp::TempList(ptr_reg),
      NULL
    )
  );

  temp::TempList *calleesaves = reg_manager->CalleeSaves();
  for (temp::Temp *t : calleesaves->GetList())
    PopReg(instr_list, ptr_reg, t);
}

void CodeGen::Codegen() {
  /* TODO: Put your lab5 code here */
  assem::InstrList *instr_list = new assem::InstrList;
  std::list<tree::Stm *> stm_list = this->traces_->GetStmList()->GetList();
  this->fs_ = this->frame_->name_->Name();

  /* Store callee-saved registers */
  StoreCalleeRegisters(*instr_list, this->fs_);

  /* Generate code */
  for (tree::Stm *stm : stm_list)
    stm->Munch(*instr_list, this->fs_);

  /* Restore callee-saved registers */
  RestoreCalleeRegisters(*instr_list, this->fs_);
  
  assem_instr_ = std::make_unique<AssemInstr>(frame::ProcEntryExit2(instr_list));
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {
/* TODO: Put your lab5 code here */

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  this->left_->Munch(instr_list, fs);
  this->right_->Munch(instr_list, fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(
    new assem::LabelInstr(
      temp::LabelFactory::LabelString(this->label_),
      this->label_
    )
  );
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(
    new assem::OperInstr(
      "jmp `j0",
      NULL, NULL,
      new assem::Targets(this->jumps_)
    )
  );
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *left_temp = this->left_->Munch(instr_list, fs);
  temp::Temp *right_temp = this->right_->Munch(instr_list, fs);

  std::string jump_instr = "";
  switch (this->op_) {
    case EQ_OP: {
      jump_instr = "je ";
      break;
    }
    case NE_OP: {
      jump_instr = "jne ";
      break;
    }
    case LT_OP: {
      jump_instr = "jl ";
      break;
    }
    case LE_OP: {
      jump_instr = "jle ";
      break;
    }
    case GT_OP: {
      jump_instr = "jg ";
      break;
    }
    case GE_OP: {
      jump_instr = "jge ";
      break;
    }
  }

  /* Due to cmpq's calling convention, we need to reverse the order of left_temp & right_temp */
  instr_list.Append(
    new assem::OperInstr(
      "cmpq `s0,`s1",
      NULL, new temp::TempList{right_temp, left_temp},
      NULL
    )
  );
  instr_list.Append(
    new assem::OperInstr(
      jump_instr + "`j0",
      NULL, NULL,
      new assem::Targets(new std::vector<temp::Label *>{true_label_})
    )
  );
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  if (typeid(*this->dst_) == typeid(tree::TempExp)) {
    temp::Temp *src_temp = this->src_->Munch(instr_list, fs);
    temp::Temp *dst_temp = this->dst_->Munch(instr_list, fs);
    instr_list.Append(
      new assem::MoveInstr(
        "movq `s0,`d0",
        new temp::TempList(dst_temp),
        new temp::TempList(src_temp)
      )
    );
  }
  else if (typeid(*this->dst_) == typeid(tree::MemExp)) {
    temp::Temp *src_temp = this->src_->Munch(instr_list, fs);
    temp::Temp *dst_temp = ((tree::MemExp *) this->dst_)->exp_->Munch(instr_list, fs);
    instr_list.Append(
      new assem::OperInstr(
        "movq `s0,(`s1)",
        NULL, new temp::TempList{src_temp, dst_temp},
        NULL
      )
    );
  }
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  this->exp_->Munch(instr_list, fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *left_temp = this->left_->Munch(instr_list, fs);
  temp::Temp *right_temp = this->right_->Munch(instr_list, fs);
  temp::Temp *result_temp = temp::TempFactory::NewTemp();
  switch (this->op_) {
    case PLUS_OP: {
      instr_list.Append(
        new assem::MoveInstr(
          "movq `s0,`d0",
          new temp::TempList(result_temp),
          new temp::TempList(left_temp)
        )
      );
      instr_list.Append(
        new assem::OperInstr(
          "addq `s0,`d0",
          new temp::TempList(result_temp),
          new temp::TempList{right_temp, result_temp},
          NULL
        )
      );
      return result_temp;
    }
    case MINUS_OP: {
      instr_list.Append(
        new assem::MoveInstr(
          "movq `s0,`d0",
          new temp::TempList(result_temp),
          new temp::TempList(left_temp)
        )
      );
      instr_list.Append(
        new assem::OperInstr(
          "sub `s0,`d0",
          new temp::TempList(result_temp),
          new temp::TempList{right_temp, result_temp},
          NULL
        )
      );
      return result_temp;
    }
    case MUL_OP: {
      temp::Temp *rax = reg_manager->GetRegister(0);
      temp::Temp *rdx = reg_manager->GetRegister(3);
      instr_list.Append(
        new assem::MoveInstr(
          "movq `s0,`d0",
          new temp::TempList(rax),
          new temp::TempList(left_temp)
        )
      );
      instr_list.Append(
        new assem::OperInstr(
          "imulq `s0",
          new temp::TempList{rax, rdx},
          new temp::TempList{right_temp, rax},
          NULL
        )
      );
      return rax;
    }
    case DIV_OP: {
      temp::Temp *rax = reg_manager->GetRegister(0);
      temp::Temp *rdx = reg_manager->GetRegister(3);
      instr_list.Append(
        new assem::MoveInstr(
          "movq `s0,`d0",
          new temp::TempList(rax),
          new temp::TempList(left_temp)
        )
      );
      instr_list.Append(
        new assem::OperInstr(
          "cqto",
          new temp::TempList(rdx),
          new temp::TempList(),
          NULL
        )
      );
      instr_list.Append(
        new assem::OperInstr(
          "idivq `s0",
          new temp::TempList{rax, rdx},
          new temp::TempList{right_temp, rax, rdx},
          NULL
        )
      );
      return rax;
    }
  }
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *result_temp = temp::TempFactory::NewTemp();
  result_temp = this->exp_->Munch(instr_list, fs);
  instr_list.Append(
      new assem::OperInstr(
        "movq (`s0),`d0",
        new temp::TempList(result_temp),
        new temp::TempList(this->exp_->Munch(instr_list, fs)),
        NULL
      )
    );
  return result_temp;

  /* ************************ Note ************************* */
  /* This part of code is unnecessary, because exp_->Munch will pass a temp
     to result_temp. We just need to "move (`s0),`d0" */
  // if (typeid(*this->exp_) == typeid(tree::BinopExp)) {
  //   tree::BinopExp *binop_exp = (tree::BinopExp *) this->exp_;
  //   /* movq Imm(r), d */
  //   if (typeid(*binop_exp->left_) == typeid(tree::ConstExp)) {
  //     temp::Temp *right_temp = binop_exp->right_->Munch(instr_list, fs);
  //     int left_consti = ((tree::ConstExp *) binop_exp->left_)->consti_;
  //     instr_list.Append(
  //       new assem::OperInstr(
  //         "movq " + std::to_string(left_consti) + "(`s0),`d0.",
  //         new temp::TempList(result_temp),
  //         new temp::TempList(right_temp),
  //         NULL
  //       )
  //     );
  //   }
  //   /* movq Imm(r), d */
  //   else if (typeid(*binop_exp->right_) == typeid(tree::ConstExp)) {
  //     temp::Temp *left_temp = binop_exp->left_->Munch(instr_list, fs);
  //     int right_consti = ((tree::ConstExp *) binop_exp->right_)->consti_;
  //     instr_list.Append(
  //       new assem::OperInstr(
  //         "movq " + std::to_string(right_consti) + "(`s0),`d0",
  //         new temp::TempList(result_temp),
  //         new temp::TempList(left_temp),
  //         NULL
  //       )
  //     );
  //   }
  // }
  // /* movq (r), d */
  // else {
  //   instr_list.Append(
  //     new assem::OperInstr(
  //       "movq (`s0),`d0",
  //       new temp::TempList(result_temp),
  //       new temp::TempList(this->exp_->Munch(instr_list, fs)),
  //       NULL
  //     )
  //   );
  // }
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  if (this->temp_ != reg_manager->FramePointer())
    return this->temp_;
  
  temp::Temp *result_temp = temp::TempFactory::NewTemp();
  codegen_log("TempExp Munch: %d", result_temp->Int());
  instr_list.Append(
    new assem::OperInstr(
      "leaq " + std::string(fs) + "_framesize(%rsp),`d0",
      new temp::TempList(result_temp), NULL,
      NULL
    )
  );
  return result_temp;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  this->stm_->Munch(instr_list, fs);
  temp::Temp *result_temp = this->exp_->Munch(instr_list, fs);
  return result_temp;
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *string_addr = temp::TempFactory::NewTemp();
  instr_list.Append(
    new assem::OperInstr(
      "leaq " + this->name_->Name() + "(%rip),`d0",
      new temp::TempList(string_addr),
      new temp::TempList(),
      NULL
    )
  );
  return string_addr;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *return_temp = temp::TempFactory::NewTemp();
  instr_list.Append(
    new assem::OperInstr(
      "movq $" + std::to_string(consti_) + ",`d0",
      new temp::TempList(return_temp),
      new temp::TempList(),
      NULL
    )
  );
  return return_temp;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  /* We do not need to get static link there since all args(<=6) will be moved to
    registers and we treat static link as a normal arg. Meanwhile, external call does
    not have static link, we could simply ignore it */
  temp::Temp *result_temp = temp::TempFactory::NewTemp();
  std::list<tree::Exp *> arg_exp_list = this->args_->GetList();

  /* Generate args */
  tree::ExpList *new_arg_exp_list = new tree::ExpList();
  for (tree::Exp *e : arg_exp_list)
    new_arg_exp_list->Append(e);
  temp::TempList *arg_list = new_arg_exp_list->MunchArgs(instr_list, fs);

  /* Update stack pointer (between ArgGen & ArgMov because we need to get correct fp) */
  int stack_offset = (arg_exp_list.size() >= 6) ? arg_exp_list.size() - 6 : 0;
  SetSP(instr_list, stack_offset);

  /* Move args to registers/stack */
  temp::TempList *arg_temp_list = MoveArgs(instr_list, arg_list, fs);
  
  /* To be destroyed */
  temp::TempList *calldef = reg_manager->CallerSaves();
  calldef->Append(reg_manager->ReturnValue());

  /* Call function */
  instr_list.Append(
    new assem::OperInstr(
      "call " + ((tree::NameExp *) this->fun_)->name_->Name(),
      calldef, arg_temp_list,
      NULL
    )
  );

  /* Reset stack pointer */
  ResetSP(instr_list, stack_offset);

  return reg_manager->ReturnValue();

}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::TempList *result_temp_list = new temp::TempList();
  for (tree::Exp *e : this->GetList()) {
    if (e == NULL) result_temp_list->Append(NULL);
    else result_temp_list->Append(e->Munch(instr_list, fs));
  }
  return result_temp_list;
}

temp::TempList *MoveArgs(assem::InstrList &instr_list, temp::TempList *arg_list, std::string_view fs) {
  temp::TempList *result_list = new temp::TempList();
  int params_passed = 0;
  for (temp::Temp *arg : arg_list->GetList()) {
    if (params_passed < 6) {
      instr_list.Append(
        new assem::MoveInstr(
          "movq `s0,`d0",
          new temp::TempList(reg_manager->ArgRegs()->NthTemp(params_passed)),
          new temp::TempList(arg)
        )
      );
      result_list->Append(arg);
    }
    else {
      /* Return address is stored in (%rsp), but is automatically processed by 'call' */
      instr_list.Append(
        new assem::OperInstr(
          "movq `s0," + std::to_string((params_passed - 6) * reg_manager->WordSize()) + "(%rsp)",
          NULL, new temp::TempList(arg),
          NULL
        )
      );
    }
    ++params_passed;
  }
  return result_list;
}

void SetSP(assem::InstrList &instr_list, int offset) {
  /* Check */
  if (offset == 0) return;

  instr_list.Append(
    new assem::OperInstr(
      "subq $" + std::to_string(offset * reg_manager->WordSize()) + ",%rsp",
      NULL, NULL,
      NULL
    )
  );
}

void ResetSP(assem::InstrList &instr_list, int offset) {
  /* Check */
  if (offset == 0) return;

  instr_list.Append(
    new assem::OperInstr(
      "addq $" + std::to_string(offset * reg_manager->WordSize()) + ",%rsp",
      NULL, NULL,
      NULL
    )
  );
}

} // namespace tree
