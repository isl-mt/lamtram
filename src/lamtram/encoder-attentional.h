#pragma once

#include <lamtram/sentence.h>
#include <lamtram/ll-stats.h>
#include <lamtram/linear-encoder.h>
#include <lamtram/neural-lm.h>
#include <lamtram/extern-calculator.h>
#include <lamtram/mapping.h>
#include <cnn/cnn.h>
#include <vector>
#include <iostream>
#include <cnpy/cnpy.h>
#include <cnpy/cnpy-utils.h>
#include <lamtram/gru-cond.h>
#include "boost/property_tree/ptree.hpp"

namespace cnn {
class Model;
struct ComputationGraph;
struct Parameter;
struct RNNBuilder;
}

namespace lamtram {

// A class to calculate extern_calcal context
class ExternAttentional : public ExternCalculator {
    friend class CnpyUtils;
public:

    ExternAttentional(const std::vector<LinearEncoderPtr> & encoders,
                      const std::string & attention_type, const std::string & attention_hist,
                      int state_size, const std::string & lex_type,
                      const DictPtr & vocab_src, const DictPtr & vocab_trg,
                      cnn::Model & mod);
    virtual ~ExternAttentional() { }

    // Index the parameters in a computation graph
    void NewGraph(cnn::ComputationGraph & cg);

    // Initialize the sentence with one or more sets of encoded input
    virtual void InitializeSentence(const Sentence & sent, bool train, cnn::ComputationGraph & cg) override;
    virtual void InitializeSentence(const std::vector<Sentence> & sent, bool train, cnn::ComputationGraph & cg) override;

    // Create a variable encoding the context
    virtual cnn::expr::Expression CreateContext(
        // const Sentence & sent, int loc,
        const std::vector<cnn::expr::Expression> & state_in,
        const cnn::expr::Expression & align_sum_in,
        bool train,
        cnn::ComputationGraph & cg,
        std::vector<cnn::expr::Expression> & align_out,
        cnn::expr::Expression & align_sum_out) const override;
        
    virtual cnn::expr::Expression CalcContext(
        const cnn::expr::Expression & state_in
        )const override;

    // Calculate the prior
    cnn::expr::Expression CalcPrior(
        const cnn::expr::Expression & align_vec) const override;

    // Create an empty context
    virtual cnn::expr::Expression GetEmptyContext(cnn::ComputationGraph & cg) const override;

    int GetHiddenSize() const { return hidden_size_; }
    int GetStateSize() const { return state_size_; }
    int GetContextSize() const { return context_size_; }

    cnn::expr::Expression GetState() { return i_h_last_; }

    // Reading/writing functions
    static ExternAttentional* Read(std::istream & in, const DictPtr & vocab_src, const DictPtr & vocab_trg, cnn::Model & model);
    void Write(std::ostream & out);


    // Setters
    void SetDropout(float dropout) {
      for(auto & enc : encoders_) enc->SetDropout(dropout);
    }

protected:
    std::vector<LinearEncoderPtr> encoders_;
    std::string attention_type_, attention_hist_;
    int hidden_size_, state_size_;

    // Lexical type
    std::string lex_type_, lex_file_;
    MultipleIdMappingPtr lex_mapping_;
    float lex_alpha_;
    size_t lex_size_;

    // Parameters
    cnn::Parameter p_ehid_h_W_;
    cnn::Parameter p_ehid_h_b_;
    cnn::Parameter p_ehid_state_W_;
    cnn::Parameter p_e_ehid_W_;
    cnn::Parameter p_e_ehid_b_;
    cnn::Parameter p_align_sum_W_;
    

    // Interned parameters
    cnn::expr::Expression i_ehid_h_W_;
    cnn::expr::Expression i_ehid_h_b_;
    cnn::expr::Expression i_ehid_state_W_;
    cnn::expr::Expression i_e_ehid_W_;
    cnn::expr::Expression i_e_ehid_b_;
    cnn::expr::Expression i_align_sum_W_;

    // Temporary variables
    cnn::expr::Expression i_h_;
    cnn::expr::Expression i_h_last_;
    cnn::expr::Expression i_ehid_hpart_;
    cnn::expr::Expression i_sent_len_;
    cnn::expr::Expression i_lexicon_;

private:
    // A pointer to the current computation graph.
    // This is only used for sanity checking to make sure NewGraph
    // is called before trying to do anything that requires it.
    cnn::ComputationGraph * curr_graph_;

    // Internal storage of a vector full of ones
    std::vector<cnn::real> sent_values_;

    int sent_len_;
    bool use_bias_;

};

typedef std::shared_ptr<ExternAttentional> ExternAttentionalPtr;

// A class for feed-forward neural network LMs
class EncoderAttentional {

public:

    // Create a new EncoderAttentional and add it to the existing model
    EncoderAttentional(const ExternAttentionalPtr & extern_calc,
                       const NeuralLMPtr & decoder,
                       cnn::Model & model);
    ~EncoderAttentional() { }

    // Build the computation graph for the sentence including loss
    template <class SentData>
    cnn::expr::Expression BuildSentGraph(const SentData & sent_src, const SentData & sent_trg, const SentData & sent_cache,
                                         float samp_percent,
                                         bool train,
                                         cnn::ComputationGraph & cg, LLStats & ll);

    // Sample sentences and return an expression of the vector of probabilities
    cnn::expr::Expression SampleTrgSentences(const Sentence & sent_src,
                                             const Sentence * sent_trg,
                                             int num_samples,
                                             int max_len,
                                             bool train,
                                             cnn::ComputationGraph & cg,
                                             std::vector<Sentence> & samples);    

    template <class SentData>
    std::vector<cnn::expr::Expression> GetEncodedState(
                                    const SentData & sent_src, bool train, cnn::ComputationGraph & cg);

    // Reading/writing functions
    static EncoderAttentional* Read(const DictPtr & vocab_src, const DictPtr & vocab_trg, std::istream & in, cnn::Model & model);
    void Write(std::ostream & out);

    //Read from pkz
    static EncoderAttentional* Convert(const DictPtr & vocab_src, const DictPtr & vocab_trg, const std::string & file, const boost::property_tree::ptree & json, cnn::Model & model);

    // Index the parameters in a computation graph
    void NewGraph(cnn::ComputationGraph & cg);

    // Information functions
    static bool HasSrcVocab() { return true; }
    static std::string ModelID() { return "encatt"; }

    // Accessors
    const NeuralLM & GetDecoder() const { return *decoder_; }
    const NeuralLMPtr & GetDecoderPtr() const { return decoder_; }
    const ExternAttentional & GetExternAttentional() const { return *extern_calc_; }
    const ExternAttentionalPtr & GetExternAttentionalPtr() const { return extern_calc_; }
    const ExternCalculator & GetExternCalc() const { return (ExternCalculator&)*extern_calc_; }
    const ExternCalculatorPtr GetExternCalcPtr() const { return std::dynamic_pointer_cast<ExternCalculator>(extern_calc_); }
    ExternCalculatorPtr GetExternCalcPtr() { return std::dynamic_pointer_cast<ExternCalculator>(extern_calc_); }
    int GetVocabSrc() const { return vocab_src_; }
    int GetVocabTrg() const { return vocab_trg_; }
    int GetNgramContext() const { return ngram_context_; }
    int GetWordrepSize() const { return wordrep_size_; }
    int GetUnkSrc() const { return unk_src_; }
    int GetUnkTrg() const { return unk_trg_; }

    // Setters
    void SetDropout(float dropout) {
      extern_calc_->SetDropout(dropout);
      decoder_->SetDropout(dropout);
    }

protected:

    // Variables
    int vocab_src_, vocab_trg_;
    int ngram_context_, wordrep_size_;
    int unk_src_, unk_trg_;

    // Vectors
    ExternAttentionalPtr extern_calc_;
    NeuralLMPtr decoder_;

    // Parameters
    cnn::Parameter p_enc2dec_W_; // Encoder to decoder weights
    cnn::Parameter p_enc2dec_b_; // Encoder to decoder bias

    // Interned Parameters
    cnn::expr::Expression i_enc2dec_W_;
    cnn::expr::Expression i_enc2dec_b_;

private:
    // A pointer to the current computation graph.
    // This is only used for sanity checking to make sure NewGraph
    // is called before trying to do anything that requires it.
    cnn::ComputationGraph * curr_graph_;

};

typedef std::shared_ptr<EncoderAttentional> EncoderAttentionalPtr;

}
