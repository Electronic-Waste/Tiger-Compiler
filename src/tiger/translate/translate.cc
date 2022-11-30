#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/x64frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/frame.h"

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;

namespace tr {

Access *Access::AllocLocal(Level *level, bool escape) {
  /* TODO: Put your lab5 code here */
  frame::Access *new_access = level->frame_->AllocLocal(escape);
  return new tr::Access(level, new_access);
}

class Cx {
public:
  PatchList trues_;
  PatchList falses_;
  tree::Stm *stm_;

  Cx(PatchList trues, PatchList falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) {}
};

class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() = 0;
  [[nodiscard]] virtual Cx UnCx(err::ErrorMsg *errormsg) = 0;
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() override { 
    /* TODO: Put your lab5 code here */
    return this->exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(this->exp_);
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    PatchList true_list, false_list;
    tree::Stm *test_stm = new tree::CjumpStm(tree::RelOp::NE_OP, this->exp_, new tree::ConstExp(0), NULL, NULL);
    return Cx(true_list, false_list, test_stm);
  }
};

class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    return new tree::EseqExp(this->stm_, new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() override { 
    /* TODO: Put your lab5 code here */
    return this->stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    errormsg->Error(0, "can't convert NxExp to Cx");
    PatchList true_list, false_list;
    return Cx(true_list, false_list, NULL);
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(PatchList trues, PatchList falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}
  
  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    temp::Temp *r = temp::TempFactory::NewTemp();
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();
    cx_.trues_.DoPatch(t);
    cx_.falses_.DoPatch(f);
    return new tree::EseqExp(
      new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(1)),
      new tree::EseqExp(
        cx_.stm_,
        new tree::EseqExp(
          new tree::LabelStm(f),
          new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r),
                              new tree::ConstExp(0)),
                      new tree::EseqExp(new tree::LabelStm(t),
                                  new tree::TempExp(r)))
        )
      )
    );
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(UnEx());
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override { 
    /* TODO: Put your lab5 code here */
    return this->cx_;
  }
};

tree::Exp *GetStaticLink(tr::Level *current_level, tr::Level *target_level) {
  tree::Exp *frame_ptr = new tree::TempExp(reg_manager->FramePointer());
  while (current_level != target_level) {
    frame_ptr = current_level->frame_->formals_->front()->ToExp(frame_ptr);
    current_level = current_level->parent_;
  }
  return frame_ptr;
}

tree::Stm *InitRecordField(temp::Temp *record_temp, int offset, tree::Exp *src) {
  return new tree::MoveStm(
    new tree::MemExp(
      new tree::BinopExp(
        tree::BinOp::PLUS_OP,
        new tree::TempExp(record_temp),
        new tree::ConstExp(offset * reg_manager->WordSize())
      )
    ),
    src
  );
}

void ProgTr::Translate() {
  /* TODO: Put your lab5 code here */
  ExpAndTy *result = this->absyn_tree_->Translate(venv_.get(), tenv_.get(), main_level_.get(), NULL, errormsg_.get());
  frags->PushBack(new frame::ProcFrag(frame::ProcEntryExit1(this->main_level_->frame_, result->exp_->UnNx()), this->main_level_->frame_));
}

} // namespace tr

namespace absyn {

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return this->root_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::VarEntry *var_entry = (env::VarEntry *) venv->Look(this->sym_);
  tree::Exp *frame_ptr = tr::GetStaticLink(level, var_entry->access_->level_);
  return new tr::ExpAndTy(
    new tr::ExExp(var_entry->access_->access_->ToExp(frame_ptr)),
    var_entry->ty_->ActualTy()
  );
}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *var_exp_ty = this->var_->Translate(venv, tenv, level, label, errormsg);
  std::list<type::Field *> field_list = ((type::RecordTy *) var_exp_ty->ty_)->fields_->GetList();
  int offset = 0;
  type::Ty *type = NULL;
  for (type::Field *f : field_list) {
    if (f->name_ == this->sym_) {
      type = f->ty_;
      break;
    }
    ++offset;
  }
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::MemExp(
        new tree::BinopExp(
          tree::BinOp::PLUS_OP,
          var_exp_ty->exp_->UnEx(),
          new tree::ConstExp(offset * reg_manager->WordSize())
        )
      )
    ),
    type
  );
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *var_exp_ty = this->var_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *subscript_exp_ty =this->subscript_->Translate(venv, tenv, level, label, errormsg);
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::MemExp(
        new tree::BinopExp(
          tree::BinOp::PLUS_OP,
          var_exp_ty->exp_->UnEx(),
          new tree::BinopExp(
            tree::BinOp::MINUS_OP,
            subscript_exp_ty->exp_->UnEx(),
            new tree::ConstExp(reg_manager->WordSize())
          )
        )
      )
    ),
    ((type::ArrayTy *) subscript_exp_ty->ty_)->ty_
  );
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return this->var_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(
    new tr::ExExp(new tree::ConstExp(0)),
    type::NilTy::Instance()
  );
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(
    new tr::ExExp(new tree::ConstExp(this->val_)),
    type::IntTy::Instance()
  );
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  temp::Label *str_label = temp::LabelFactory::NewLabel();
  frags->PushBack(new frame::StringFrag(str_label, this->str_));
  return new tr::ExpAndTy(
    new tr::ExExp(new tree::NameExp(str_label)),
    type::StringTy::Instance()
  );
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::FunEntry *func_entry = (env::FunEntry *) tenv->Look(this->func_);
  std::list<absyn::Exp *> arg_list = this->args_->GetList();
  tree::ExpList *tree_arg_list = new tree::ExpList();
  tree::Exp *call_exp;
  for (absyn::Exp *e : arg_list) {
    tr::ExpAndTy *arg_exp_ty = e->Translate(venv, tenv, level, label, errormsg);
    tree_arg_list->Append(arg_exp_ty->exp_->UnEx());
  }
  /* External function call */
  if (func_entry->level_ == NULL) {
    call_exp = frame::ExternelCall(this->func_->Name(), tree_arg_list);
  }
  /* Need to find static link */
  else {
    tree::Exp *frame_ptr = tr::GetStaticLink(level, func_entry->level_->parent_);
    tree_arg_list->Insert(frame_ptr);
    call_exp = new tree::CallExp(
      new tree::NameExp(func_entry->label_),
      tree_arg_list
    );
  }
  return new tr::ExpAndTy(
    new tr::ExExp(call_exp),
    func_entry->result_
  );
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *left_exp_ty = this->left_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *right_exp_ty = this->right_->Translate(venv, tenv, level, label, errormsg);
  switch (this->oper_) {
    case absyn::AND_OP:
    case absyn::OR_OP:
    case absyn::PLUS_OP:
    case absyn::MINUS_OP:
    case absyn::TIMES_OP:
    case absyn::DIVIDE_OP:
      return new tr::ExpAndTy(
        new tr::ExExp(
          new tree::BinopExp(
            (tree::BinOp) this->oper_,
            left_exp_ty->exp_->UnEx(),
            right_exp_ty->exp_->UnEx()
          )
        ),
        type::IntTy::Instance()
      );
    case absyn::LT_OP:
    case absyn::LE_OP:
    case absyn::GT_OP:
    case absyn::GE_OP: {
      tree::CjumpStm *stm = new tree::CjumpStm(
        (tree::RelOp) (this->oper_ - 6),
        left_exp_ty->exp_->UnEx(),
        right_exp_ty->exp_->UnEx(),
        NULL,
        NULL
      );
      return new tr::ExpAndTy(
        new tr::CxExp(
          tr::PatchList(std::list<temp::Label **>()),
          tr::PatchList(std::list<temp::Label **>()),
          stm
        ),
        type::IntTy::Instance()
      );
    }
    case absyn::EQ_OP:
    case absyn::NEQ_OP: {
      /* String */
      if (left_exp_ty->ty_->IsSameType(type::StringTy::Instance())) {
        tree::ExpList *arg_list = new tree::ExpList{left_exp_ty->exp_->UnEx(), right_exp_ty->exp_->UnEx()};
        tree::CjumpStm *stm = new tree::CjumpStm(
          (tree::RelOp) (this->oper_ - 6),
          frame::ExternelCall("string_equal", arg_list),
          new tree::ConstExp(1),
          NULL,
          NULL
        );
        return new tr::ExpAndTy(
          new tr::CxExp(
            tr::PatchList(std::list<temp::Label **>()),
            tr::PatchList(std::list<temp::Label **>()),
            stm
          ),
          type::IntTy::Instance()
        );
      }
      /* Normal Form: int*/
      else {
        tree::CjumpStm *stm = new tree::CjumpStm(
          (tree::RelOp) (this->oper_ - 6),
          left_exp_ty->exp_->UnEx(),
          right_exp_ty->exp_->UnEx(),
          NULL,
          NULL
        );
        return new tr::ExpAndTy(
          new tr::CxExp(
            tr::PatchList(std::list<temp::Label **>()),
            tr::PatchList(std::list<temp::Label **>()),
            stm
          ),
          type::IntTy::Instance()
        );
      }
    }
  }
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,      
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty *record_type = tenv->Look(this->typ_);
  std::list<type::Field *> record_formals_list = ((type::RecordTy *) record_type)->fields_->GetList();
  std::list<EField *> record_actuals_list = this->fields_->GetList();
  /* Alloc space for record */
  temp::Temp *record_temp = temp::TempFactory::NewTemp();
  tree::Stm *alloc_stm = new tree::MoveStm(
    new tree::TempExp(record_temp),
    frame::ExternelCall(
      "alloc_record",
      new tree::ExpList{
        new tree::ConstExp(record_actuals_list.size() * reg_manager->WordSize())
      }
    )
  );
  /* Assign value to each field (in reverse order) */
  tree::Stm *assign_stm = NULL;
  int actual_list_size = record_actuals_list.size();
  auto record_actuals_start = record_actuals_list.begin();
  auto record_actuals_end = record_actuals_list.end();
  if (record_actuals_start != record_actuals_end) {
    int offset = actual_list_size - 1;
    tr::ExpAndTy *assign_exp_ty = (*record_actuals_end)->exp_->Translate(venv, tenv, level, label, errormsg);
    assign_stm = tr::InitRecordField(record_temp, offset--, assign_exp_ty->exp_->UnEx());
    record_actuals_end--;
    for (; record_actuals_end != record_actuals_start; --record_actuals_end) {
      assign_exp_ty = (*record_actuals_end)->exp_->Translate(venv, tenv, level, label, errormsg);
      assign_stm = new tree::SeqStm(tr::InitRecordField(record_temp, offset--, assign_exp_ty->exp_->UnEx()), assign_stm);
    }
  }
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::EseqExp(
        new tree::SeqStm(alloc_stm, assign_stm),
        new tree::TempExp(record_temp)
      )
    ),
    record_type
  );
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<absyn::Exp *> seqexp_list = this->seq_->GetList();
  if (seqexp_list.size() == 0) {
    return new tr::ExpAndTy(NULL, type::VoidTy::Instance());
  }
  else if (seqexp_list.size() == 1) {
    return seqexp_list.front()->Translate(venv, tenv, level, label, errormsg);
  }
  else {
    tree::Stm *seq_stm = NULL;
    int cur_pos = 0;
    for (absyn::Exp *e : seqexp_list) {
      tr::ExpAndTy *e_exp_ty = e->Translate(venv, tenv, level, label, errormsg);
      if (seq_stm == NULL)
        seq_stm = e_exp_ty->exp_->UnNx();
      else if (++cur_pos < seqexp_list.size())
        seq_stm = new tree::SeqStm(
          seq_stm, e_exp_ty->exp_->UnNx()
        );
      else {
        return new tr::ExpAndTy(
          new tr::ExExp(
            new tree::EseqExp(
              seq_stm, e_exp_ty->exp_->UnEx()
            )
          ),
          e_exp_ty->ty_
        );
      }
    }
  }
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,                       
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *var_exp_ty = this->var_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *exp_exp_ty = this->exp_->Translate(venv, tenv, level, label, errormsg);
  return new tr::ExpAndTy(
    new tr::NxExp(
      new tree::MoveStm(
        var_exp_ty->exp_->UnEx(),
        exp_exp_ty->exp_->UnEx()
      )
    ),
    type::VoidTy::Instance()
  );
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *test_exp_ty = this->test_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *then_exp_ty = this->then_->Translate(venv, tenv, level, label, errormsg);
  temp::Label *true_label = temp::LabelFactory::NewLabel();
  temp::Label *false_label = temp::LabelFactory::NewLabel();
  temp::Label *end_label = temp::LabelFactory::NewLabel();
  tr::Cx test_cx = test_exp_ty->exp_->UnCx(errormsg);
  test_cx.trues_.DoPatch(true_label);
  test_cx.falses_.DoPatch(false_label);
  temp::Temp *result_temp = temp::TempFactory::NewTemp();
  if (!this->elsee_) {
    tr::ExpAndTy *else_exp_ty = this->elsee_->Translate(venv, tenv, level, label, errormsg);
    tree::Stm *seq_stm = new tree::SeqStm(
      test_cx.stm_,
      new tree::SeqStm(
        new tree::LabelStm(true_label),
        new tree::SeqStm(
          new tree::MoveStm(
            new tree::TempExp(result_temp),
            then_exp_ty->exp_->UnEx()
          ),
          new tree::SeqStm(
            new tree::JumpStm(new tree::NameExp(end_label),new std::vector<temp::Label *>{end_label}),
            new tree::SeqStm(
              new tree::LabelStm(false_label),
              new tree::SeqStm(
                new tree::MoveStm(
                  new tree::TempExp(result_temp),
                  else_exp_ty->exp_->UnEx()
                ),
                new tree::LabelStm(end_label)
              )
            )
          )
        )
      )
    );
    return new tr::ExpAndTy(
      new tr::ExExp(
        new tree::EseqExp(
          seq_stm,
          new tree::TempExp(result_temp)
        )
      ),
      then_exp_ty->ty_
    );
  }
  else {
    tree::Stm *seq_stm = new tree::SeqStm(
      new tree::LabelStm(true_label),
      new tree::SeqStm(
        then_exp_ty->exp_->UnNx(),
        new tree::LabelStm(false_label)
      )
    );
    return new tr::ExpAndTy(
      new tr::NxExp(seq_stm),
      type::VoidTy::Instance()
    );
  }
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,            
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* test_exp_ty = this->test_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy* body_exp_ty = this->body_->Translate(venv, tenv, level, label, errormsg);
  temp::Label *test_label = temp::LabelFactory::NewLabel();
  temp::Label *body_label = temp::LabelFactory::NewLabel();
  temp::Label *done_label = temp::LabelFactory::NewLabel();
  tr::Cx test_cx = test_exp_ty->exp_->UnCx(errormsg);
  test_cx.trues_.DoPatch(body_label);
  test_cx.falses_.DoPatch(done_label);
  tree::Stm *seq_stm = new tree::SeqStm(
    new tree::LabelStm(test_label),
    new tree::SeqStm(
      test_cx.stm_,
      new tree::SeqStm(
        new tree::LabelStm(body_label),
        new tree::SeqStm(
          body_exp_ty->exp_->UnNx(),
          new tree::SeqStm(
            new tree::JumpStm(new tree::NameExp(test_label), new std::vector<temp::Label *>{test_label}),
            new tree::LabelStm(done_label)
          )
        )
      )
    )
  );
  return new tr::ExpAndTy(
    new tr::NxExp(seq_stm),
    type::VoidTy::Instance()
  );
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  venv->BeginScope();
  tr::Access *it_access = tr::Access::AllocLocal(level, this->escape_);
  tr::ExpAndTy *low_exp_ty = this->lo_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *high_exp_ty = this->hi_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *body_exp_ty = this->body_->Translate(venv, tenv, level, label, errormsg);
  temp::Label *test_label = temp::LabelFactory::NewLabel();
  temp::Label *body_label = temp::LabelFactory::NewLabel();
  temp::Label *done_label = temp::LabelFactory::NewLabel();
  venv->Enter(this->var_, new env::VarEntry(it_access, low_exp_ty->ty_));
  tree::Exp *it_exp = it_access->access_->ToExp(new tree::TempExp(reg_manager->FramePointer()));
  tree::Stm *seq_stm = new tree::SeqStm(
    new tree::MoveStm(it_exp, low_exp_ty->exp_->UnEx()),
    new tree::SeqStm(
      new tree::LabelStm(test_label),
      new tree::SeqStm(
        new tree::CjumpStm(
          tree::RelOp::LE_OP,
          it_exp,
          high_exp_ty->exp_->UnEx(),
          body_label,
          done_label
        ),
        new tree::SeqStm(
          new tree::LabelStm(body_label),
          new tree::SeqStm(
            body_exp_ty->exp_->UnNx(),
            new tree::SeqStm(
              new tree::MoveStm(
                it_exp,
                new tree::BinopExp(
                  tree::BinOp::PLUS_OP,
                  it_exp,
                  new tree::ConstExp(1)
                )
              ),
              new tree::SeqStm(
                new tree::JumpStm(new tree::NameExp(test_label), new std::vector<temp::Label *>{test_label}),
                new tree::LabelStm(done_label)
              )
            )
          )
        )
      )
    )
  );
  venv->EndScope();
  return new tr::ExpAndTy(
    new tr::NxExp(seq_stm),
    type::VoidTy::Instance()
  );
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(
    new tr::NxExp(
      new tree::JumpStm(
        new tree::NameExp(label),
        new std::vector<temp::Label *>{label}
      )
    ),
    type::VoidTy::Instance()
  );
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  venv->BeginScope();
  tenv->BeginScope();
  tree::Stm *seq_stm = NULL;
  /* Declaration part */
  for (Dec *dec : this->decs_->GetList()) {
    if (seq_stm == NULL)
      seq_stm = dec->Translate(venv, tenv, level, label, errormsg)->UnNx();
    else {
      seq_stm = new tree::SeqStm(
        seq_stm,
        dec->Translate(venv, tenv, level, label, errormsg)->UnNx()
      );
    }
  }
  /* Body part */
  tr::ExpAndTy *body_exp_ty;
  if (!this->body_) {
    body_exp_ty = new tr::ExpAndTy(
      new tr::ExExp(new tree::ConstExp(0)),
      type::VoidTy::Instance()
    );
  }
  else {
    body_exp_ty = this->body_->Translate(venv, tenv, level, label, errormsg);
  }
  /* Return part */
  tr::ExpAndTy *ret_exp_ty;
  if (!seq_stm) {
    ret_exp_ty = body_exp_ty;
  }
  else {
    ret_exp_ty = new tr::ExpAndTy(
      new tr::ExExp(
        new tree::EseqExp(
          seq_stm, body_exp_ty->exp_->UnEx()
        )
      ),
      body_exp_ty->ty_
    );
  }
  tenv->EndScope();
  venv->EndScope();
  return ret_exp_ty;
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,                    
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty *array_ty = tenv->Look(this->typ_)->ActualTy();
  type::Ty *element_ty = ((type::ArrayTy *) array_ty)->ty_;
  tr::ExpAndTy *size_exp_ty = this->size_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *init_exp_ty = this->init_->Translate(venv, tenv, level, label, errormsg);
  tree::ExpList *arg_list = new tree::ExpList();
  arg_list->Append(size_exp_ty->exp_->UnEx());
  arg_list->Append(init_exp_ty->exp_->UnEx());
  return new tr::ExpAndTy(
    new tr::ExExp(frame::ExternelCall("init_array", arg_list)),
    array_ty
  );
  return NULL;
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(
    new tr::ExExp(new tree::ConstExp(0)),
    type::VoidTy::Instance()
  );
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<FunDec *> fundec_list = this->functions_->GetList();
  for (FunDec *fundec : fundec_list) {
    /* Get Result type */
    type::Ty *result_type = type::VoidTy::Instance();
    if (fundec->result_ != NULL)
      result_type = tenv->Look(fundec->result_);
    /* Get escape list */
    std::list<bool> *escape_list = new std::list<bool>;
    for (Field *field : fundec->params_->GetList())
      escape_list->push_back(field->escape_);
    /* Create function label */
    temp::Label *func_label = temp::LabelFactory::NamedLabel(fundec->name_->Name());
    /* Get formals' type list */
    type::TyList *formalTys = fundec->params_->MakeFormalTyList(tenv, errormsg);
    /* Create level */
    tr::Level *func_level = new tr::Level(level, func_label, escape_list);
    /* Store function entry in venv */
    venv->Enter(fundec->name_, new env::FunEntry(func_level, func_label, formalTys, result_type));
  }

  for (FunDec *fundec : fundec_list) {
    venv->BeginScope();
    env::FunEntry *func_entry = (env::FunEntry *) tenv->Look(fundec->name_);
    std::list<type::Field *> param_list = fundec->params_->MakeFieldList(tenv, errormsg)->GetList();
    auto formal_it = func_entry->level_->frame_->formals_->begin();
    for (type::Field *param_it : param_list) {
      tr::Access *param_it_access = new tr::Access(func_entry->level_, *formal_it);
      venv->Enter(param_it->name_, new env::VarEntry(param_it_access, param_it->ty_->ActualTy()));
      ++formal_it;
    }
    tr::ExpAndTy *body_exp_ty = fundec->body_->Translate(venv, tenv, level, label, errormsg);
    tree::MoveStm *move_return = new tree::MoveStm(
      new tree::TempExp(reg_manager->ReturnValue()),
      body_exp_ty->exp_->UnEx()
    );
    frags->PushBack(new frame::ProcFrag(frame::ProcEntryExit1(func_entry->level_->frame_, move_return), func_entry->level_->frame_));
    venv->EndScope();
  }
  return new tr::ExExp(new tree::ConstExp(0));
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::Access *var_access = tr::Access::AllocLocal(level, this->escape_);
  tr::ExpAndTy *init_exp_ty = this->init_->Translate(venv, tenv, level, label, errormsg);
  venv->Enter(this->var_, new env::VarEntry(var_access, init_exp_ty->ty_));
  return new tr::NxExp(
    new tree::MoveStm(
      var_access->access_->ToExp(
        new tree::TempExp(reg_manager->FramePointer())
      ),
      init_exp_ty->exp_->UnEx()
    )
  );
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<NameAndTy *> types_list = this->types_->GetList();
  for (NameAndTy *type_dec : types_list) 
    tenv->Enter(type_dec->name_, new type::NameTy(type_dec->name_, NULL));
  for (NameAndTy *type_dec : types_list) {
    type::NameTy *name_ty = (type::NameTy *) tenv->Look(type_dec->name_);
    name_ty->ty_ = type_dec->ty_->Translate(tenv, errormsg);
  }
  return new tr::ExExp(new tree::ConstExp(0));
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty *name_type = tenv->Look(this->name_);
  return new type::NameTy(this->name_, name_type);
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new type::RecordTy(this->record_->MakeFieldList(tenv, errormsg));
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new type::ArrayTy(tenv->Look(this->array_));
}

} // namespace absyn
