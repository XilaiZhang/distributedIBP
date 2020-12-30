#include "NdArray.h"

class Message{
public:

    int src;
    int dst;
    int num_iter = 0;
    int is_collected = 0;
    NdArray CPT;
    // constructor
    Message() {}
    Message (int src, int dst, int num_iter) {
        this->src = src;
        this->dst = dst;
        this->num_iter=num_iter;
    }
    
    NdArray getCPT() {
        return this->CPT;
    }
    void setCPT(NdArray CPT) {
        this-> CPT = CPT;
    }
};
    
    



