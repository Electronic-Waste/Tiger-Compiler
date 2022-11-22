//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {
class X64RegManager : public RegManager {
  /* TODO: Put your lab5 code here */
  [[nodiscard]] temp::TempList *Registers() override {
    return NULL;
  }

  [[nodiscard]] temp::TempList *ArgRegs() override {
    return NULL;
  }

  [[nodiscard]] temp::TempList *CallerSaves() override {
    return NULL;
  }

  [[nodiscard]] temp::TempList *CalleeSaves() override {
    return NULL;
  }

  [[nodiscard]] temp::TempList *ReturnSink() override {
    return NULL;
  }

  [[nodiscard]] int WordSize() override {
    return NULL;
  }

  [[nodiscard]] temp::Temp *FramePointer() override {
    return NULL;
  }

  [[nodiscard]] temp::Temp *StackPointer() override {
    return NULL;
  }

  [[nodiscard]] temp::Temp *ReturnValue() override {
    return NULL;
  }
};

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
