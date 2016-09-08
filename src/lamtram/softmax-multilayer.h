#pragma once

#include <cnn/expr.h>
#include <lamtram/softmax-base.h>
#include <lamtram/softmax-factory.h>
#include <boost/algorithm/string.hpp>

namespace cnn { struct Parameter; }

namespace lamtram {

// An interface to a class that takes a vector as input
// (potentially batched) and calculates a probability distribution
// over words
class SoftmaxMultiLayer : public SoftmaxBase {
    friend class CnpyUtils;

public:
  SoftmaxMultiLayer(const std::string & sig, int input_size, const DictPtr & vocab, cnn::Model & mod);
  ~SoftmaxMultiLayer() { };

  // Create a new graph
  virtual void NewGraph(cnn::ComputationGraph & cg) override;

  // Calculate training loss for one word
  virtual cnn::expr::Expression CalcLoss(cnn::expr::Expression & in, cnn::expr::Expression & prior, const Sentence & ngram, bool train) override;
  // Calculate training loss for multiple words
  virtual cnn::expr::Expression CalcLoss(cnn::expr::Expression & in, cnn::expr::Expression & prior, const std::vector<Sentence> & ngrams, bool train) override;
  
  // Calculate the full probability distribution
  virtual cnn::expr::Expression CalcProb(cnn::expr::Expression & in, cnn::expr::Expression & prior, const Sentence & ctxt, bool train) override;
  virtual cnn::expr::Expression CalcProb(cnn::expr::Expression & in, cnn::expr::Expression & prior, const std::vector<Sentence> & ctxt, bool train) override;
  virtual cnn::expr::Expression CalcLogProb(cnn::expr::Expression & in, cnn::expr::Expression & prior, const Sentence & ctxt, bool train) override;
  virtual cnn::expr::Expression CalcLogProb(cnn::expr::Expression & in, cnn::expr::Expression & prior, const std::vector<Sentence> & ctxt, bool train) override;

protected:
  cnn::Parameter p_sm_W_; // Softmax weights
  cnn::Parameter p_sm_b_; // Softmax bias

  SoftmaxPtr softmax_;

  cnn::expr::Expression i_sm_W_;
  cnn::expr::Expression i_sm_b_;

};

}
