#ifndef ONEFLOW_CORE_GRAPH_INPLACE_LBI_GRAPH_H_
#define ONEFLOW_CORE_GRAPH_INPLACE_LBI_GRAPH_H_

#include "oneflow/core/common/util.h"
#include "oneflow/core/operator/operator.h"
#include "oneflow/core/graph/graph.h"

namespace oneflow {

class InplaceLbiEdge;

class InplaceLbiNode : public Node<InplaceLbiNode, InplaceLbiEdge> {
 public:
  virtual ~InplaceLbiNode() = default;

  const LogicalBlobId& lbi() const { return lbi_; }
  const InplaceLbiEdge* GetValidInEdge(
      const std::function<bool(const InplaceLbiEdge*)>& IsValidEdge) const;
  const InplaceLbiEdge* GetSoleValidInEdge(
      const std::function<bool(const InplaceLbiEdge*)>& IsValidEdge) const;
  void ForEachNodeOnValidOutEdge(const std::function<bool(const InplaceLbiEdge*)>& IsValidEdge,
                                 const std::function<void(const InplaceLbiNode*)>& Handler) const;
  virtual bool IsMutRef(const std::function<bool(const InplaceLbiEdge*)>& IsValidEdge) const;
  bool IsConstRef(const std::function<bool(const InplaceLbiEdge*)>& IsValidEdge) const;

  std::string VisualStr() const override { return GenLogicalBlobName(lbi_); }

 protected:
  OF_DISALLOW_COPY_AND_MOVE(InplaceLbiNode);
  explicit InplaceLbiNode(const LogicalBlobId& lbi) : lbi_(lbi) {}

 private:
  LogicalBlobId lbi_;
};

class NormalInplaceLbiNode final : public InplaceLbiNode {
 public:
  OF_DISALLOW_COPY_AND_MOVE(NormalInplaceLbiNode);
  explicit NormalInplaceLbiNode(const LogicalBlobId& lbi) : InplaceLbiNode(lbi) {}
  ~NormalInplaceLbiNode() override = default;

  bool IsMutRef(const std::function<bool(const InplaceLbiEdge*)>& IsValidEdge) const override;
};

class SourceOpInplaceLbiNode final : public InplaceLbiNode {
 public:
  OF_DISALLOW_COPY_AND_MOVE(SourceOpInplaceLbiNode);
  explicit SourceOpInplaceLbiNode(const LogicalBlobId& lbi) : InplaceLbiNode(lbi) {}
  ~SourceOpInplaceLbiNode() = default;
};

class UpdateInplaceLbiNode final : public InplaceLbiNode {
 public:
  OF_DISALLOW_COPY_AND_MOVE(UpdateInplaceLbiNode);
  explicit UpdateInplaceLbiNode(const LogicalBlobId& lbi) : InplaceLbiNode(lbi) {}
  ~UpdateInplaceLbiNode() = default;
};

class InplaceLbiEdge final : public Edge<InplaceLbiNode, InplaceLbiEdge> {
 public:
  OF_DISALLOW_COPY_AND_MOVE(InplaceLbiEdge);
  InplaceLbiEdge(const Operator* op, const std::string& ibn, const std::string& obn)
      : op_(op), ibn_(ibn), obn_(obn) {}
  ~InplaceLbiEdge() = default;

  const Operator& op() const { return *op_; }
  const std::string& ibn() const { return ibn_; }
  const std::string& obn() const { return obn_; }
  bool IsMutRef() const;
  bool IsConstRef() const { return !IsMutRef(); }

  std::string VisualStr() const override {
    return std::string(op_->op_name() + "/" + ibn_ + ":" + obn_);
  }

 private:
  const Operator* op_;
  const std::string ibn_;
  const std::string obn_;
};

class InplaceLbiGraph final : public Graph<const InplaceLbiNode, const InplaceLbiEdge> {
 public:
  OF_DISALLOW_COPY_AND_MOVE(InplaceLbiGraph);
  InplaceLbiGraph(const OpBlobArgList& obas,
                  const std::function<const Operator*(const std::string&)>& Op4OpName) {
    Init(obas, Op4OpName);
  }
  ~InplaceLbiGraph() = default;
  const char* TypeName() const override { return "InplaceLbiGraph"; }

  void ComputeSafeInplaceObns(OpBlobArgList* obas,
                              const std::function<bool(const LogicalBlobId&, const std::string&)>&
                                  IsLbiAllConsumerReachableToOpName) const;

 private:
  void Init(const OpBlobArgList& obas,
            const std::function<const Operator*(const std::string&)>& Op4OpName);
  std::function<InplaceLbiNode*(const LogicalBlobId&)> MakeMutFindOrCreateNode(
      std::function<const Operator*(const std::string&)> Op4OpName);
  void ComputeSafeInplaceEdges(const std::function<bool(const LogicalBlobId&, const std::string&)>&
                                   IsLbiAllConsumerReachableToOpName,
                               const std::function<void(const InplaceLbiEdge*)>& Handler) const;
  void ComputeSafeInplaceEdges(const HashSet<const InplaceLbiNode*>& nodes,
                               const std::function<bool(const LogicalBlobId&, const std::string&)>&
                                   IsLbiAllConsumerReachableToOpName,
                               const std::function<void(const InplaceLbiEdge*)>& Handler) const;
  void ForEachSafeInplaceEdgeInSourceOpSubTree(
      const HashSet<const InplaceLbiNode*>& nodes,
      const std::function<bool(const LogicalBlobId&, const std::string&)>&
          IsLbiAllConsumerReachableToOpName,
      const std::function<void(const InplaceLbiEdge*)>& Handler,
      HashSet<const InplaceLbiEdge*>* cur_disabled_edges) const;
  void GetSafeInplaceObnEdges(const HashSet<const InplaceLbiNode*>& nodes,
                              const std::function<bool(const InplaceLbiEdge*)>& IsValidEdge,
                              const std::function<bool(const LogicalBlobId&, const std::string&)>&
                                  IsLbiAllConsumerReachableToOpName,
                              HashSet<const InplaceLbiEdge*>* cur_disabled_edges) const;
  const InplaceLbiEdge* FindFirstConstRefConflictMutRefEdge(
      const HashSet<const InplaceLbiNode*>& nodes,
      const std::function<bool(const InplaceLbiEdge*)>& IsValidEdge,
      const std::function<bool(const LogicalBlobId&, const std::string&)>&
          IsLbiAllConsumerReachableToOpName) const;

  const InplaceLbiEdge* FindFirstIntraOpRefConflictMutRefEdge(
      const HashSet<const InplaceLbiNode*>& nodes,
      const std::function<bool(const InplaceLbiEdge*)>& IsValidEdge) const;

  const InplaceLbiEdge* FindFirstInterOpRefConflictMutRefEdge(
      const HashSet<const InplaceLbiNode*>& nodes,
      const std::function<bool(const InplaceLbiEdge*)>& IsValidEdge,
      const std::function<bool(const LogicalBlobId&, const std::string&)>&
          IsLbiAllConsumerReachableToOpName) const;

  bool IsConstRefConflictMutRefNode(
      const InplaceLbiNode* mut_ref_node, const HashSet<const InplaceLbiNode*>& nodes,
      const std::function<bool(const InplaceLbiEdge*)>& IsValidEdge,
      const std::function<bool(const LogicalBlobId&, const std::string&)>&
          IsLbiAllConsumerReachableToOpName) const;

  void FixConstRefOrMutRefConflictsToUpdtNode(
      const HashSet<const InplaceLbiNode*>& nodes,
      const std::function<bool(const LogicalBlobId&, const std::string&)>&
          IsLbiAllConsumerReachableToOpName,
      HashSet<const InplaceLbiEdge*>* cur_disabled_edges) const;

  void FixMutRefConflictsFromSourceOpNode(
      const SourceOpInplaceLbiNode* root,
      const std::function<bool(const InplaceLbiEdge*)>& IsValidEdge,
      HashSet<const InplaceLbiEdge*>* cur_disabled_edges) const;

  void ForEachTree(const HashSet<const InplaceLbiNode*>& nodes,
                   const std::function<bool(const InplaceLbiEdge*)>& IsValidEdge,
                   const std::function<void(const HashSet<const InplaceLbiNode*>&)>& Handler) const;
  void FindAllEdges(const HashSet<const InplaceLbiNode*>& nodes,
                    const std::function<bool(const InplaceLbiEdge*)>& IsValidEdge,
                    HashSet<const InplaceLbiEdge*>* cur_disabled_edges) const;
};

}  // namespace oneflow

#endif  // ONEFLOW_CORE_GRAPH_INPLACE_LBI_GRAPH_H_
