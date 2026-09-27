#ifndef NF_PIPELINE_LATCH_HPP_
# define NF_PIPELINE_LATCH_HPP_

namespace nf::pipeline {


template <typename T>
class PipelineLatch {
    public:
        const T &Output() const;    // serve q to consumer stage reads output this cycle
        void    SetNext(T next);    // accept d stage from producer
        void    Commit(); // clock edge d_ = q_
    
    private:
        T d_;
        T q_;
};


}


#endif // NF_PIPELINE_LATCH_HPP_