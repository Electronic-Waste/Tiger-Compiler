#include "tiger/absyn/absyn.h"
#include "tiger/semant/semant.h"

namespace absyn {

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  this->root_->SemAnalyze(venv, tenv, 0, errormsg);
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  env::EnvEntry *entry = venv->Look(this->sym_);
  if (entry && typeid(*entry) == typeid(env::VarEntry)) 
    return ((env::VarEntry *) entry)->ty_->ActualTy();
  else {
    errormsg->Error(this->pos_, "undefined variable %s", this->sym_->Name().data());
    return type::IntTy::Instance();
  }
}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *var_type = this->var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  /* Check type */
  if (var_type == NULL || typeid(*var_type) != typeid(type::RecordTy)) {
    errormsg->Error(this->pos_, "not a record type");
    return type::VoidTy::Instance();
  }
  
  /* Check symbol */
  std::list<type::Field *> record_field_list = ((type::RecordTy *) var_type)->fields_->GetList();
  auto start = record_field_list.cbegin();
  auto end = record_field_list.cend();
  for (; start != end; ++start) {
    if ((*start)->name_ == this->sym_)
      return (*start)->ty_->ActualTy();
  }
  errormsg->Error(this->pos_, "field %s doesn't exist", this->sym_->Name().data());
  return type::VoidTy::Instance();
 
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  /* Check var */
  type::Ty *var_type = this->var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (var_type == NULL || typeid(*var_type) != typeid(type::ArrayTy)) {
    errormsg->Error(this->pos_, "array type required");
    return type::VoidTy::Instance();
  }
  /* Check subscript */
  type::Ty *subscript_type = this->subscript_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (subscript_type == NULL || typeid(*subscript_type) != typeid(type::IntTy)) {
    errormsg->Error(this->pos_, "invalid index type");
    return type::VoidTy::Instance();
  }
  return ((type::ArrayTy *) var_type)->ty_->ActualTy();
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return this->var_->SemAnalyze(venv, tenv, labelcount, errormsg);
}

type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::IntTy::Instance();
}

type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::StringTy::Instance();
}

type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  env::EnvEntry *func_entry = venv->Look(this->func_);
  if (func_entry == NULL || typeid(*func_entry) != typeid(env::FunEntry)) {
    errormsg->Error(this->pos_, "undefined function %s", this->func_->Name().data());
    return type::VoidTy::Instance();
  }
  /* Check actuals with formals declared before */
  std::list<type::Ty *> formals_ty_list = ((env::FunEntry *) func_entry)->formals_->GetList();
  std::list<Exp *> actuals_list = this->args_->GetList();
  /* Type check */
  auto formal_ptr_begin = formals_ty_list.cbegin();
  auto actual_ptr_begin = actuals_list.cbegin();
  auto formal_ptr_end = formals_ty_list.cend();
  auto acutal_ptr_end = actuals_list.cend();
  for (; formal_ptr_begin != formal_ptr_end && actual_ptr_begin != acutal_ptr_end; ++formal_ptr_begin, ++actual_ptr_begin) {
    type::Ty *actal_ty = (*actual_ptr_begin)->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (!(*formal_ptr_begin)->IsSameType(actal_ty)) {
      errormsg->Error(this->pos_, "para type mismatch");
      return ((env::FunEntry *) func_entry)->result_;
    }
  }
  /* Num check */
  if (formals_ty_list.size() < actuals_list.size()) {
    errormsg->Error(this->pos_, "too many params in function %s", this->func_->Name().data());
  }
  else if (formals_ty_list.size() > actuals_list.size()) {
    errormsg->Error(this->pos_, "too few params in function %s", this->func_->Name().data());
  }
  return ((env::FunEntry *) func_entry)->result_;
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *left_type = left_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  type::Ty *right_type = right_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if (this->oper_ == absyn::PLUS_OP || this->oper_ == absyn::MINUS_OP ||
      this->oper_ == absyn::TIMES_OP || this->oper_ == absyn::DIVIDE_OP) {
    if (typeid(*left_type) != typeid(type::IntTy)) 
      errormsg->Error(this->pos_, "integer required");
    if (typeid(*right_type) != typeid(type::IntTy)) 
      errormsg->Error(this->pos_, "integer required");
  }
  else {
    if (!left_type->IsSameType(right_type))
      errormsg->Error(this->pos_, "same type required");
  }
  return type::IntTy::Instance();
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *record_type = tenv->Look(this->typ_);
  /* Check if defined */
  if (record_type == NULL) {
    errormsg->Error(this->pos_, "undefined type %s", this->typ_->Name().data());
    return type::VoidTy::Instance();
  }
  record_type = record_type->ActualTy();
  if (typeid(*record_type) != typeid(type::RecordTy)) {
    errormsg->Error(this->pos_, "not a record type");
    return type::VoidTy::Instance();
  }
  std::list<type::Field *> record_formals_list =((type::RecordTy *) record_type)->fields_->GetList();
  std::list<EField *> record_actuals_list = this->fields_->GetList();
  /* Check number */
  if (record_formals_list.size() != record_actuals_list.size()) {
    errormsg->Error(this->pos_, "the number of actuals in record dismatch");
    return type::VoidTy::Instance();
  }
  /* Check each type in actuals list */
  auto actual_start = record_actuals_list.cbegin();
  auto actual_end = record_actuals_list.cend();
  auto formal_start = record_formals_list.cbegin();
  auto formal_end = record_formals_list.cend();
  for (; actual_start != actual_end && formal_start != formal_end; ++actual_start, ++formal_start) {
    if ((*formal_start)->name_ != (*actual_start)->name_) {
      errormsg->Error(this->pos_, "field name dismatches");
      return type::VoidTy::Instance();
    }
    int isnull = (((type::NameTy *) (*formal_start)->ty_)->ActualTy() == NULL) ? 0 : 1;
    printf("name: %s, ty: %d\n", (*formal_start)->name_->Name().data(), isnull);
    type::Ty *exp_type = (*actual_start)->exp_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (!(*formal_start)->ty_->IsSameType(exp_type)) {
      errormsg->Error(this->pos_, "actual type dismatches with formal type declared");
      return type::VoidTy::Instance();
    }
  }
  return record_type->ActualTy();
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<Exp *> seqexp_list = this->seq_->GetList();
  type::Ty *ret_type = type::VoidTy::Instance();
  auto start = seqexp_list.cbegin();
  auto end = seqexp_list.cend();
  for (; start != end; ++start) {
    ret_type = (*start)->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  return ret_type;  
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (typeid(*var_) == typeid(SimpleVar)) {
    sym::Symbol *var_sym = ((SimpleVar *) this->var_)->sym_;
    env::EnvEntry *var_entry = venv->Look(var_sym);
    if (typeid(*var_entry) == typeid(env::VarEntry)) {
      bool flag = ((env::VarEntry *) var_entry)->readonly_;
      if (flag) errormsg->Error(this->pos_, "loop variable can't be assigned");
    }
  }
  type::Ty *assign_type = this->var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *exp_type = this->exp_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typeid(*assign_type) == typeid(type::VoidTy)) {
    return type::VoidTy::Instance();
  }
  else if (typeid(*assign_type) != typeid(*exp_type)) {
    errormsg->Error(this->pos_, "unmatched assign exp");
    return type::VoidTy::Instance();
  }
  return type::VoidTy::Instance();
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *test_type = this->test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *then_type = this->then_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *result_type = type::VoidTy::Instance();
  if (typeid(*test_type) != typeid(type::IntTy)) {
    errormsg->Error(this->pos_, "invalid test exp type");
  }
  if (this->elsee_ != NULL) {
    type::Ty *else_type = this->elsee_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (!then_type->IsSameType(else_type)) {
      errormsg->Error(this->pos_, "then exp and else exp type mismatch");
    }
  }
  else {
    if (typeid(*test_type) != typeid(type::VoidTy)) {
      errormsg->Error(this->pos_, "if-then exp's body must produce no value");
    }
  }

  return type::VoidTy::Instance();
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *test_type = this->test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *body_type = this->body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);
  if (typeid(*test_type) != typeid(type::IntTy)) {
    errormsg->Error(this->pos_, "invalid test exp type");
  }
  if (typeid(*body_type) != typeid(type::VoidTy)) {
    errormsg->Error(this->pos_, "while body must produce no value");
  }
  return type::VoidTy::Instance();
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  venv->BeginScope();
  type::Ty *lo_type = lo_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *hi_type = hi_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typeid(*lo_type) != typeid(type::IntTy) || typeid(*hi_type) != typeid(type::IntTy)) {
    errormsg->Error(this->pos_, "for exp's range type is not integer");
  }
  venv->Enter(this->var_, new env::VarEntry(type::IntTy::Instance(), true));
  type::Ty *body_type = body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);
  if (typeid(*body_type) != typeid(type::VoidTy)) {
    errormsg->Error(this->pos_, "for body must produce no value");
  }
  venv->EndScope();
  return type::VoidTy::Instance();

}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (labelcount == 0) {
    errormsg->Error(this->pos_, "break is not inside any loop");
  }
  return type::VoidTy::Instance();
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  venv->BeginScope();
  tenv->BeginScope();
  for (Dec *dec : this->decs_->GetList()) 
    dec->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *body_type;
  if (!this->body_) 
    body_type = type::VoidTy::Instance();
  else 
    body_type = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  tenv->EndScope();
  venv->EndScope();
  return body_type;
}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *array_type = tenv->Look(this->typ_)->ActualTy();
  if (array_type == NULL || typeid(*array_type) != typeid(type::ArrayTy)) {
    errormsg->Error(this->pos_, "undefined type");
  }
  type::Ty *element_type = ((type::ArrayTy *) array_type)->ty_;
  type::Ty *size_type = size_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *init_type = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typeid(*size_type) != typeid(type::IntTy)) {
    errormsg->Error(this->pos_, "invalid size type");
  }
  if (typeid(*element_type) != typeid(*init_type)) {
    errormsg->Error(this->pos_, "type mismatch");
  }
  return array_type;
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<FunDec *> fundec_list = this->functions_->GetList();
  /* First: input function head (function name, formals list, result type) */
  for (FunDec *fundec : fundec_list) {
    type::Ty *result_type = type::VoidTy::Instance();
    type::TyList *formalTys = fundec->params_->MakeFormalTyList(tenv, errormsg);
    if (fundec->result_ != NULL)
      result_type = tenv->Look(fundec->result_);
    if (venv->Look(fundec->name_) != NULL) {
      errormsg->Error(this->pos_, "two functions have the same name");
      continue;
    }
    venv->Enter(fundec->name_, new env::FunEntry(formalTys, result_type));
  }
  /* Second: Address the body of function */
  for (FunDec *fundec : fundec_list) {
    venv->BeginScope();
    std::list<type::Field *> param_list = fundec->params_->MakeFieldList(tenv, errormsg)->GetList();
    for (type::Field *param_it : param_list) 
      venv->Enter(param_it->name_, new env::VarEntry(param_it->ty_));
    type::Ty *body_type = fundec->body_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (fundec->result_ == NULL && typeid(*body_type) != typeid(type::VoidTy::Instance())) {
      errormsg->Error(this->pos_, "procedure returns value");
    }
    else if (!tenv->Look(fundec->result_)->IsSameType(body_type)) {
      errormsg->Error(this->pos_, "the return value dismatches with result type!");
    }
    venv->EndScope();
  }
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *init_type = this->init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(typ_ != NULL && !tenv->Look(this->typ_)->IsSameType(init_type)){
    errormsg->Error(this->pos_, "type mismatch");
  } 
  else if(typ_ == NULL && typeid(*init_type) == typeid(type::NilTy)){
    errormsg->Error(this->pos_, "init should not be nil without type specified");
  }
  venv->Enter(this->var_, new env::VarEntry(init_type));
}

// {name->absyn::Ty *}->{name->type::Ty *}
void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<NameAndTy *> name_and_ty_list = this->types_->GetList();
  bool is_cycle = false;
  for (NameAndTy *name_and_ty : name_and_ty_list) {
    if (tenv->Look(name_and_ty->name_) != NULL) {
      errormsg->Error(this->pos_, "two types have the same name");
      continue;
    }
    tenv->Enter(name_and_ty->name_, new type::NameTy(name_and_ty->name_, NULL));
  }
  for (NameAndTy *name_and_ty : name_and_ty_list) {
    type::NameTy *name_ty = (type::NameTy *) tenv->Look(name_and_ty->name_);
    name_ty->ty_ = name_and_ty->ty_->SemAnalyze(tenv, errormsg);
  }
  /* Detecting type cycle */
  bool iscycle = false;
  std::vector<sym::Symbol *> sym_vec;
  for (NameAndTy *name_and_ty : name_and_ty_list) {
    sym::Symbol *sym = name_and_ty->name_;
    sym_vec.push_back(sym);
    type::Ty *ty = tenv->Look(sym);
    while (true) {
      type::Ty *name_ty = ((type::NameTy *) ty)->ty_;
      if (typeid(*name_ty) == typeid(type::NameTy)) {
        sym::Symbol *next_sym = ((type::NameTy *) name_ty)->sym_;
        for (sym::Symbol *s : sym_vec) {
          if (s == next_sym) {
            is_cycle = true;
            break;
          }
        }
        if (is_cycle) break;
        else ty = name_ty;
      }
      else {
        break;
      }
    }
    if (is_cycle) {
      errormsg->Error(pos_, "illegal type cycle");
      break;
    }
    sym_vec.clear();
  }
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *name_type = tenv->Look(this->name_);
  if (name_type == NULL) {
    errormsg->Error(this->pos_, "undefined type %s", this->name_);
    return type::VoidTy::Instance();
  }
  return new type::NameTy(this->name_, name_type);
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return new type::RecordTy(this->record_->MakeFieldList(tenv, errormsg));
}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *array_type = tenv->Look(this->array_);
  if (array_type == NULL) {
    errormsg->Error(this->pos_, "undefined type %s", this->array_);
    return type::VoidTy::Instance();
  }
  return new type::ArrayTy(array_type);
}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}

} // namespace tr
