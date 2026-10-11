#ifndef NF_PIPELINE_LATCH_HPP_
# define NF_PIPELINE_LATCH_HPP_

#include <type_traits>
#include <utility>
#include <cassert>

namespace nf::pipeline {

// Two phase tick (Evaluate, Commit) discipline (see docs/design/decisions.md)
// - producers call SetNext during Evaluate()
// - consumers read Output during Evaluate()
// - the pipeline calls Commit() on each latch, after all stages are evaluated.
// No reference of Output may be held after Commit().

template <typename T>
requires    std::is_default_constructible_v<T> &&
            std::is_move_assignable_v<T>
class PipelineLatch {
    public:
        bool    IsValid() const { return q_valid_; }
        const T &Output() const { return q_; }      // use as this cycles input
        void    SetNext(T next) { assert(!hold_); d_ = std::move(next); d_valid_ = true; }    // propagate, from producer to consumer
        void    SetNextBubble() { assert(!hold_); d_ = T{}; d_valid_ = false; }
        void    SetHold()       { hold_ = true; }       // set each stalled cycle
        void    Commit() { 
            if (hold_) {hold_ = false; return; }
            q_ = std::move(d_);     // clock edge q_ = d_
            q_valid_ = d_valid_;
        }      
    
    private:
        T q_{};   // value served to consumers this cycle
        T d_{};   // value being produced this cycle
        bool d_valid_{false};   
        bool q_valid_{false};       
        bool hold_{false};       // when consumer not ready
};


} // namespace nf::pipeline


#endif // NF_PIPELINE_LATCH_HPP_