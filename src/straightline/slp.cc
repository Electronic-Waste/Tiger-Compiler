#include "straightline/slp.h"

#include <iostream>

namespace A {
/* *********** This part is for Stm ************* */
int A::CompoundStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return stm1->MaxArgs() > stm2->MaxArgs() ? stm1->MaxArgs() : stm2->MaxArgs();   // Get max args in stm1 & stm2
}

Table *A::CompoundStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  t = stm1->Interp(t);    // Execute in sequence
  t = stm2->Interp(t);    // Execute in sequence
  return t;
}

int A::AssignStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return exp->MaxArgs();  // Get max args in exp (which may contain 'print')
}

Table *A::AssignStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable *result = exp->Interp(t);   // Execute exp first
  Table *newT = result->t;                // Get new Table
  return newT->Update(id, result->i);     // Assign value and return new Table
}

int A::PrintStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return exps->MaxArgs();                 // Max args in exps (ExpList)
}

Table *A::PrintStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable *result = exps->Interp(t);
  return result->t;              // Use Interp fucntion in ExpList to get result
}

/* *********** This part is for Exp ************* */
int A::IdExp::MaxArgs() const {
  return 0;                               // There is no args in this type of Exp
}

IntAndTable *A::IdExp::Interp(Table *t) const {
  // std::cout << "id: " << id << std::endl;
  return new IntAndTable(t->Lookup(id), t);     // return the value of id; do not change Table t
}

int A::NumExp::MaxArgs() const {
  return 0;                               // There is no args in this type of Exp
}

IntAndTable *A::NumExp::Interp(Table *t) const {
  return new IntAndTable(num, t);         // return num; do not change Table t
}

int A::OpExp::MaxArgs() const {
  return left->MaxArgs() > right->MaxArgs() ? left->MaxArgs() : right->MaxArgs();       // Get max args in two Exps
}

IntAndTable *A::OpExp::Interp(Table *t) const {
  IntAndTable *leftResult = left->Interp(t);                    // Execute left exp
  int leftValue = leftResult->i;                                // Get left value
  IntAndTable *rightResult = right->Interp(leftResult->t);      // Execute right exp using new table returned from left exp
  int rightValue = rightResult->i;                              // Get right value
  Table *newT = rightResult->t;                                 // Get newest table
  switch (oper) {
    case PLUS:
      return new IntAndTable(leftValue + rightValue, newT);
    case MINUS:
      return new IntAndTable(leftValue - rightValue, newT);
    case TIMES:
      return new IntAndTable(leftValue * rightValue, newT);
    case DIV:
      return new IntAndTable(leftValue / rightValue, newT);
  }
}

int A::EseqExp::MaxArgs() const {
  return stm->MaxArgs() > exp->MaxArgs() ? stm->MaxArgs() : exp->MaxArgs();             // Get max args in two item
}

IntAndTable *A::EseqExp::Interp(Table *t) const {
  Table *newT = stm->Interp(t);             // Execute stm and get new table
  return exp->Interp(newT);                 // Execute exp and get return value
}

/* *********** This part is for ExpList ************* */
int A::PairExpList::MaxArgs() const {
  int maxArgs = exp->MaxArgs() > tail->MaxArgs() ? exp->MaxArgs() : tail->MaxArgs();    // Get max args in sub-items(exp & tail)
  int maxNums = this->NumExps();            // Arg nums in PairExpList
  return maxArgs > maxNums ? maxArgs : maxNums;
}

int A::PairExpList::NumExps() const {
  return tail->NumExps() + 1;               // Compute recursively
}

IntAndTable *A::PairExpList::Interp(Table *t) const {
  IntAndTable *result = exp->Interp(t);     // Execute exp and get return value
  std::cout << result->i << " ";            // Print result->i to terminal
  return tail->Interp(result->t);           // Execute recursively
}


int A::LastExpList::MaxArgs() const {
  return exp->MaxArgs() > 1 ? exp->MaxArgs() : 1;
}

int A::LastExpList::NumExps() const {
  return 1;                                 // Recursion end
}

IntAndTable *A::LastExpList::Interp(Table *t) const {
  IntAndTable *result = exp->Interp(t);     
  std::cout << result->i << std::endl;      // Recursion end: execute exp.
  return result;               
}

/* *********** This part is for Table ************* */
int Table::Lookup(const std::string &key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(const std::string &key, int val) const {
  return new Table(key, val, this);
}

void Table::Show() const {
  std::cout << id << ": " << value << "->";
  if (tail != nullptr) tail->Show();
  else std::cout << std::endl;
}
}  // namespace A
