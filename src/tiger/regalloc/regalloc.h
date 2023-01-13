#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/codegen/codegen.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/regalloc/color.h"
#include "tiger/util/graph.h"

#include <set>
#include <map>
namespace ra {

class Result {
public:
  temp::Map *coloring_;
  assem::InstrList *il_;

  Result() : coloring_(nullptr), il_(nullptr) {}
  Result(temp::Map *coloring, assem::InstrList *il)
      : coloring_(coloring), il_(il) {}
  Result(const Result &result) = delete;
  Result(Result &&result) = delete;
  Result &operator=(const Result &result) = delete;
  Result &operator=(Result &&result) = delete;
  ~Result() {
    delete coloring_;
    delete il_;
  }
};

class RegAllocator {
  /* TODO: Put your lab6 code here */
public:
  RegAllocator(frame::Frame *frame_, std::unique_ptr<cg::AssemInstr> assem_instr_);
  RegAllocator() = default;
  ~RegAllocator() = default;
  std::unique_ptr<Result> TransferResult();
  void RegAlloc();
  void Build();
  void AddEdge(live::INodePtr u, live::INodePtr v);
  void MakeWorkList();
  live::INodeListPtr Adjacent(live::INodePtr n);
  live::MoveList *NodeMoves(live::INodePtr n);
  bool MoveRelated(live::INodePtr n);
  void Simplify();
  void DecrementDegree(live::INodePtr m);
  void EnableMoves(live::INodeListPtr nodes);
  void Coalesce();
  void AddWorkList(live::INodePtr u);
  bool OK(live::INodePtr t, live::INodePtr r);
  bool Conservative(live::INodeListPtr nodes);
  live::INodePtr GetAilas(live::INodePtr n);
  void Combine(live::INodePtr u, live::INodePtr v);
  void Freeze();
  void FreezeMoves(live::INodePtr n);
  void SelectSpill();
  void AssignColors();
  void RewriteProgram();

  void ShowStatus();

private:
  const int K = 16;
  std::unique_ptr<Result> result;
  frame::Frame *frame;
  std::unique_ptr<cg::AssemInstr> assem_instr;

  live::LiveGraphFactory *live_graph_factory;
  live::LiveGraph *live_graph;

  /* low degree & non-move-related nodes */
  live::INodeListPtr simplifyWorklist;
  /* low degree & move-related nodes */
  live::INodeListPtr freezeWorklist;
  /* high degree nodes */
  live::INodeListPtr spillWorklist;
  /* nodes to be spilled in this round */
  live::INodeListPtr spilledNodes;
  /* nodes that have been coalesced */
  live::INodeListPtr coalescedNodes;
  /* nodes that have benn colored*/
  live::INodeListPtr coloredNodes;
  /* a stack containing nodes deleted from interf_graph */
  live::INodeListPtr selectStack;

  /* move instrs that have been coalesced */
  live::MoveList *coalescedMoves;
  /* constrained move instrs*/
  live::MoveList *constrainedMoves;
  /* frozen move instrs */
  live::MoveList *frozenMoves;
  /* move instrs likely to be coalesced */  
  live::MoveList *worklistMoves;
  /* move instr not prepared for coalescing */
  live::MoveList *activeMoves;

  /* map: node -> its degree */
  std::map<live::INodePtr, int> degree;
  /* map: node -> its move instrs */
  std::map<live::INodePtr, live::MoveList *> moveList;
  /* map: node -> the node coalescing this node */
  std::map<live::INodePtr, live::INodePtr> alias;
  /* map: node -> its color */
  std::map<live::INodePtr, std::string *> color;
  
};

} // namespace ra

#endif