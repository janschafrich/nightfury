#ifndef NF_PIPELINE_LATCH_HPP_
#define NF_PIPELINE_LATCH_HPP_

#include <type_traits>
#include <utility>
#include <cassert>

namespace nf::pipeline {

// Pipeline register between producer and consumer stage with a ready/valid handshake.
// Skid buffer functionality: the latch retains the offered valid packet, until the consumer accepts.

// Two phase tick (Evaluate control and data, then Advance) discipline (see docs/design/decisions.md):
// 1. Control: Consumers assert Stall() in their input latch when not ready; silence means ready.
// 2. Data:    Producer skips Evaluate while !Ready(); otherwise drives exactly once Set/SetNop.
// 3. Advance: The pipeline calls Advance() on every latch, every cycle. It serves as the clock edge.

// No reference returned by Q() may be held after Advance().

// PipelineLatch is a class template. C++ compilation requires the full template 
// definition to be visble in every translation unit. 
// Put all definitions in this file. 

template <typename T>
requires    std::is_default_constructible_v<T> &&
            std::is_move_assignable_v<T>
class PipelineLatch {
    public:
        // ---- producer stage side ----
        // Keep stalling until consumer accepted q
        bool Ready() const { return ready_; }     
        // Drive payload into latch
        void Set(T next) { 
            assert(Ready()); 
            d_              = std::move(next); 
            d_valid_        = true; 
        }

        void SetNop() { 
            assert(Ready()); 
            d_              = T{}; 
            d_valid_        = false; 
        }
        // Remove a mispredicted instruction from the pipeline
        void Squash() {
            squash_         = true; 
        }
        
        // ---- consumer stage side ----
        // Called by consumer stage during Evaluate()
        bool Valid() const { return q_valid_; }
        const T &Q() const { return q_; }      // use as this cycles input
        void Stall()       { ready_ = false; } // set each stalled cycle, consumer not ready
        
        // ---- pipeline ----
        void Advance() { 
            if (squash_) {
                d_valid_    = false; 
                q_valid_    = false;
                ready_      = true; 
                squash_     = false; 
                return;
            }
            if (!ready_) {
                ready_      = true; 
                return; 
            } // freeze d (Skid buffer)
            
            q_ = std::move(d_);     // clock edge q_ = d_
            q_valid_ = d_valid_;
        }      
    
    private:
        T q_{};   // value served to the consumer this cycle
        T d_{};   // value offered by the producer this cycle
        bool d_valid_{false};   // whether producer has new valid data 
        bool q_valid_{false};   // whether the data currently stored is valid  
        bool ready_{true};      // whether consumer is ready this cycle (restored at edge)
        bool squash_{false};   // whether control requested a squash this cycle
};


} // namespace nf::pipeline


#endif // NF_PIPELINE_LATCH_HPP_