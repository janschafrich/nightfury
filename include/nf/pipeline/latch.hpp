#ifndef NF_PIPELINE_LATCH_HPP_
# define NF_PIPELINE_LATCH_HPP_

#include <type_traits>
#include <utility>

namespace nf::pipeline {

// Two phase tick (Evaluate, Commit) discipline (see docs/design-decisions.md)
// - producers call SetNext during Evaluate()
// - consumers read Output during Evaluate()
// - the pipeline calls Commit() on each latch, after all stages are evaluated.
// No reference of Output may be held after Commit().

template <typename T>
requires    std::is_default_constructible_v<T> &&
            std::is_move_assignable_v<T>
class PipelineLatch {
    public:
        const T &Output() const { return q_; }      // use as this cycles input
        void    SetNext(T next) { d_ = std::move(next); }    // propagate from producer to consumer
        void    Commit() { q_ = std::move(d_); }      // clock edge d_ = q_
    
    private:
        T q_;   // value served to consumers this cycle
        T d_;   // value being produced this cycle
};


} // namespace nf::pipeline


#endif // NF_PIPELINE_LATCH_HPP_