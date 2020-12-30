#ifndef NDARRAY_HEADER
#define NDARRAY_HEADER


#include <vector>
#include <string>
using namespace std;

class NdArray {
public:
    int ndims; 
    vector<int> cards; //M array
    vector<double> array; // 1D representation of ND array
    int array_size; // size of this 1D array
    
    NdArray() {};
    NdArray(int ndims, vector<int> cards);
    int computeIndex(vector<int> indices); //indices---S array, cards---M array
    double get(vector<int> indices);
    void set(vector<int> indices, double value);
    void setData(vector<double> array);
    string to_string();

};

// initialize random cpt of size (parentsCard, card)
NdArray init_random_cpt(int card, vector<int> parentsCard);
// project ndarray CPT to the given dim 
NdArray project(NdArray CPT, int p_dim);
// compute product of CPTs and messages from parents and children
NdArray dotProduct(NdArray CPT, vector<NdArray> operands);
// compute product of messages from children
NdArray childProduct(int card, int num_children, vector<NdArray>& operands);

#endif
