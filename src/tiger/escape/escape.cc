#include "tiger/escape/escape.h"
#include "tiger/absyn/absyn.h"

namespace esc {
void EscFinder::FindEscape() { absyn_tree_->Traverse(env_.get()); }
} // namespace esc

namespace absyn {

void AbsynTree::Traverse(esc::EscEnvPtr env) {
  /* TODO: Put your lab5 code here */
  this->root_->Traverse(env, 0);
}

void SimpleVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  esc::EscapeEntry *entry = env->Look(this->sym_);
  /* Not found: Can't find definition */
  if (entry == NULL) return;
  /* If depth > entry's depth, set escape to true */
  else if (depth > entry->depth_) 
    *entry->escape_ = true;
  
}

void FieldVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  esc::EscapeEntry *entry = env->Look(this->sym_);
  /* Not found: Can't find definition */
  if (entry == NULL) return;
  /* If depth > entry's depth, set escape to true */
  else if (depth > entry->depth_) 
    *entry->escape_ = true;
}

void SubscriptVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env, depth);
  this->subscript_->Traverse(env, depth);
}

void VarExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env, depth);
}

void NilExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  /* Do nothing */
  return;
}

void IntExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  /* Do nothing */
  return;
}

void StringExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  /* Do nothing */
  return;
}

void CallExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  std::list<Exp *> arglist = this->args_->GetList();
  for (Exp *e : arglist)
    e->Traverse(env, depth);
}

void OpExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->left_->Traverse(env, depth);
  this->right_->Traverse(env, depth);
}

void RecordExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  /* Do nothing */
  std::list<EField *> fieldList = this->fields_->GetList();
  for (EField *e : fieldList)
    e->exp_->Traverse(env, depth);
  return;
}

void SeqExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  std::list<Exp *> expList = this->seq_->GetList();
  for (Exp *e : expList)
    e->Traverse(env, depth);
}

void AssignExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env, depth);
  this->exp_->Traverse(env, depth);
}

void IfExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->test_->Traverse(env, depth);
  this->then_->Traverse(env, depth);
  if (this->elsee_ != NULL)
    this->elsee_->Traverse(env, depth);
}

void WhileExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->test_->Traverse(env, depth);
  this->body_->Traverse(env, depth);
}

void ForExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  env->BeginScope();
  this->escape_ = false;
  env->Enter(this->var_, new esc::EscapeEntry(depth, &this->escape_));
  this->lo_->Traverse(env, depth);
  this->hi_->Traverse(env, depth);
  this->body_->Traverse(env, depth);
  env->EndScope();
}

void BreakExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  /* Do nothing */
  return;
}

void LetExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  env->BeginScope();
  std::list<Dec *> decList = this->decs_->GetList();
  for (Dec *d : decList) 
    d->Traverse(env, depth);
  this->body_->Traverse(env, depth);
  env->EndScope();
}

void ArrayExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->size_->Traverse(env, depth);
  this->init_->Traverse(env, depth);
}

void VoidExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  /* Do nothing */
  return;
}

void FunctionDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  std::list<FunDec *> funDecList = this->functions_->GetList();
  for (FunDec *dec : funDecList) {
    env->BeginScope();
    std::list<Field *> fieldList = dec->params_->GetList();
    for (Field *f : fieldList) {
      f->escape_ = false;
      env->Enter(f->name_, new esc::EscapeEntry(depth + 1, &f->escape_));
    }
    dec->body_->Traverse(env, depth + 1);
    env->EndScope();
  }
}

void VarDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->escape_ = false;
  env->Enter(this->var_, new esc::EscapeEntry(depth, &this->escape_));
  this->init_->Traverse(env, depth);
}

void TypeDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  /* Do nothing */
  return;
}

} // namespace absyn
