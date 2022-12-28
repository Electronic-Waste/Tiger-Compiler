#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */

RegAllocator::RegAllocator(frame::Frame *frame, std::unique_ptr<cg::AssemInstr> assem_instr)
    : frame_(frame), assem_instr_(std::move(assem_instr)) {
    result_ = std::make_unique<Result>();
}

void RegAllocator::RegAlloc() {
    
}

} // namespace ra