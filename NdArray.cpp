#include <random>
#include <assert.h>
#include <iostream>
#include "NdArray.h"
#include <string>

using namespace std;

NdArray::NdArray(int ndims, vector<int> cards) {

        this->ndims = ndims;
        assert(ndims == cards.size());
        this->cards = cards;
        int temp = 1;
        for (int i = 0; i < ndims; ++i) {
            temp *= cards[i];
        }
        this->array_size = temp;
        this->array = vector<double>(this->array_size); 
}

int NdArray::computeIndex(vector<int> indices) { //indices---S array, cards---M array
        assert(this->ndims == indices.size());
        vector<int> weights(this->ndims,0);
        int currentProduct=1;
        weights[this->ndims-1]=1;
        for(int i = this->ndims-1; i >= 1; --i){
            currentProduct*=this->cards[i];
            weights[i-1]=currentProduct;
        }
        int key=0;
        for(int j=0;j<this->ndims;++j){
            key+=indices[j]*weights[j];
        }
        return key;
}

double NdArray::get(vector<int> indices) {

        int key = computeIndex(indices); // compute index into 1D array
        if (indices.size() == 1) {
            string temp = "get index: " + std::to_string(key) + " key: " + std::to_string(key)
                 + "value: " + std::to_string(this->array[key]) + "\n";
            //cout << temp;
        }
        return this->array[key];
}

void NdArray::set(vector<int> indices, double value) {
    int key = computeIndex(indices); // compute index into 1D array
    this->array[key] = value;

}

void  NdArray::setData(vector<double> array) {
    assert(this->array_size == array.size());
    this->array = array;     // replace the whole array
}   

string NdArray::to_string() {
    string s = "[";
    for (int i=0; i<this->array.size(); i++) {
        if (i != 0)
            s += ", ";
        s += std::to_string(this->array[i]);
    }
    s += "]";
    return s;
}

double get_random() {
  // Returns a random double between 0 and 1 

  return (double) rand() / (double) RAND_MAX;
}

void init_random_cpt_rec(int ndims, int dim, int card, vector<int> parentsCard, vector<int> indices, NdArray& result) {

    if (dim == ndims - 1) {
        // base case:
        // iterate all parents index
        vector<double> cond(card);
        for (int i = 0; i < card; ++i) {
            cond[i] = get_random(); 
        }
        double sum = 0;
        for (int i = 0; i < card; ++i) {
            sum += cond[i];
        }
        // normalize cond cpt
        for (int i = 0; i < card; ++i) {
            cond[i] /= sum;
        }
        for (int i=0; i<card; ++i) {
            vector<int> temp_indices(indices);
            temp_indices.push_back(i);
            result.set(temp_indices, cond[i]);
        }
        // set cond under parent index

        return;

    }

    for(int i=0; i < parentsCard[dim];++i){
        vector<int> indices2(indices);
        indices2.push_back(i);
        init_random_cpt_rec(ndims, dim+1, card, parentsCard, indices2, result);
    }

}

    



// initialize a random CPT over (parentsCard,card)
NdArray init_random_cpt(int card, vector<int> parentsCard) {

    int ndims = parentsCard.size() + 1;
    vector<int> cards(ndims);
    for (int i = 0; i < parentsCard.size(); i++) {
        cards[i] = parentsCard[i];
    }
    cards[ndims-1] = card;
    NdArray result(ndims,cards);

    vector<int> indices;
    init_random_cpt_rec(ndims, 0, card, parentsCard, indices, result);
    return result;
}

// normalize 1D CPT
NdArray normalize(NdArray CPT) {

    assert(CPT.ndims == 1);
    NdArray result(CPT);
    double sum = 0.0;
    for (int i=0; i<result.array_size; ++i) {
        sum += result.array[i];
    }

    for (int i=0; i<result.array_size; ++i) {
        result.array[i] /= sum;
    }

    return result;
}

void project_rec(NdArray CPT, int p_dim, int dim, int index, vector<int> indices, double& sum) {

    if (dim == CPT.ndims) {
        // base case: add the value at indices to sum
        double value = CPT.get(indices);
        sum += value;
        return;
    }

    if (dim == p_dim) {
        vector<int> indices2(indices);
        indices2.push_back(index);
        project_rec(CPT, p_dim, dim+1, index, indices2, sum);

    }

    for (int i = 0; i < CPT.cards[dim]; ++i) {
        vector<int> indices2(indices);
        indices2.push_back(i);
        project_rec(CPT, p_dim, dim+1, index, indices2, sum);
    }

}


// sum out other dimensions of CPT except given dimension
NdArray project(NdArray CPT, int p_dim) {
    //cout<<"p_dim is "<<p_dim<<" ndims is "<<CPT.ndims<<endl;
    assert(0 <= p_dim && p_dim < CPT.ndims);
    int p_card = CPT.cards[p_dim];
    vector<int> temp(1, p_card);
    // sum out other dimension except p_dim
    NdArray result(1, temp);
    for (int index = 0; index < p_card; ++index) {
        double sum = 0;
        vector<int> indices;
        project_rec(CPT, p_dim, 0, index, indices, sum);
        vector<int> temp2(1, index);
        //cout<<"calling set point 2"<<endl;
        result.set(temp2, sum);
        // sum out other dimensions

    }

    result = normalize(result);
    return result;

}



void dotProduct_rec(NdArray CPT, vector<NdArray> operands, NdArray& result, int dim, vector<int> indices){
    if(dim == CPT.ndims){
        assert(CPT.ndims == indices.size());
        double value = CPT.get(indices);
        for(int i = 0; i < operands.size(); ++i){
            NdArray currOperand = operands[i];
            value *= currOperand.get(vector<int> {indices[i]});
        }  
        result.set(indices, value);
        return;
    }
    for(int i=0; i < CPT.cards[dim];++i){
        vector<int> indices2(indices);
        indices2.push_back(i);
        dotProduct_rec(CPT,operands, result, dim+1,indices2);
    }
}

// compute product of CPTs and messages from parents and children
NdArray dotProduct(NdArray CPT, vector<NdArray> operands) {
    string temp = "start dot product function\n" ;
    //cout << temp;
    assert(CPT.ndims == operands.size());
    for (int i=0;i<operands.size();++i) {
        assert(operands[i].ndims=1);
        assert(operands[i].cards[0] == CPT.cards[i]);
    }
    NdArray result(CPT.ndims, CPT.cards);
    vector<int> indices;
    dotProduct_rec(CPT, operands, result, 0, indices);
    temp = "finish dot product function\n";
    //cout << temp;
    return result;
}




NdArray childProduct(int card, int num_children, vector<NdArray>& operands) {
    assert(operands.size() == num_children);
    for (int i=0; i<num_children; ++i) {
        string temp = "child op " + to_string(i) + ": " + operands[i].to_string() + "\n";
        //cout << temp;
    }

    vector<int> cards(1, card);
    NdArray product = NdArray(1, cards);
    for (int i = 0; i < card; ++i) {
        vector<int> index(1, i);
        double value = 1.0;
        for (int j = 0; j < num_children; ++j) {
            assert(operands[j].ndims == 1);
            assert(operands[j].cards[0] == card);
            value *= operands[j].get(index);
        }
        string temp = "index " + to_string(i) + ": " + to_string(value) + "\n";
        //cout << temp;
        product.set(index, value);
    }
    string temp = "child product: " + product.to_string() + "\n";
    //cout << temp;
    return product;
   
}

