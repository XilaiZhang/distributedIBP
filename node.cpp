#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>      
#include <stdlib.h> 
#include <string.h>
#include <random>
#include <iostream>
#include <string>
#include "NdArray.h"

using namespace std;

const int MAX_NUM_NEIGHBORS = 10;
//TODO: 
//math for sum out
//default constructor of NdArray

#include "message.cpp"

class Node{
public:
    int id; 
    int card; // cardinality
    vector<int> parents; // parents id
    vector<int> children; // children id
    vector<int> parentsCard; //parents cardinality
    vector<int> childrenCard; // children cardinality
    int num_parents;
    int num_children;
    int num_iter=0;
    NdArray CPT; // probability table of this node

    // store pipes to neighbor nodes
    vector<pthread_mutex_t*> lock_from_neighbors;
    vector<pthread_mutex_t*> lock_to_neighbors;
    vector<Message> msg_from_neighbors_cache;
    vector<Message> msg_to_neighbors_cache;
    vector<Message*> msg_from_neighbors;
    vector<Message*> msg_to_neighbors;
    //pointers, actual physical obeject not set y

    Node(){
        //vector<pthread_mutex_t*> v1 (10, nullptr);
        //vector<pthread_mutex_t*> v2(10, nullptr);
        lock_from_neighbors= vector<pthread_mutex_t*>(MAX_NUM_NEIGHBORS);
        lock_to_neighbors= vector<pthread_mutex_t*>(MAX_NUM_NEIGHBORS);
        msg_from_neighbors = vector<Message*>(MAX_NUM_NEIGHBORS);
        msg_to_neighbors = vector<Message*>(MAX_NUM_NEIGHBORS);
        msg_from_neighbors_cache = vector<Message>(MAX_NUM_NEIGHBORS);
        msg_to_neighbors_cache = vector<Message>(MAX_NUM_NEIGHBORS);
    }

    /*
    Node(int id, vector<int> parents, vector<int> parentsCard, int num_parents, vector<int> children, vector<int> childrenCard, int num_children){
        this->id = id;
        this->num_parents = num_parents;
        this->parents = parents;
        this->parentsCard = parentsCard;

        this->num_children = num_children;
        this->children = children;
        this->childrenCard = childrenCard;
        //default deep copy
            
    } 
    */
   void sanity_check() {

       string s1 = "Node <" + to_string(this->id) + ">: Start sanity check...";
       //cout << s1 << endl;
       int num_neighbors = num_parents + num_children;
       for (int i = 0; i < num_neighbors; ++i) {

           assert(lock_from_neighbors[i] != NULL && lock_to_neighbors[i] != NULL);
           assert(msg_from_neighbors[i]->dst == this->id && msg_to_neighbors[i]->src == this->id);
       }
       string s2 = "Node <" + to_string(this->id) + ">: finish sanity check";
       //cout << s2 << endl;
   }


    // Collect messages from all neighbors for each iteration
    void collect() {
        string s= "Node <"+ to_string(this->id) + ">: Start collect message for iteration " + to_string(this->num_iter) + "...\n";
        //cout<<s<<endl;
        int is_message_pushed[MAX_NUM_NEIGHBORS];
        memset(is_message_pushed, 0, sizeof(is_message_pushed));
        // keep track of whether an updated message has been collected from this neighbor
        int count = 0;
        int num_neighbors = num_parents + num_children;
        const int NUM_TRY_LOCKS = 3;
        for (int i = 0; i < num_neighbors; i = (i+1) % num_neighbors) { 

            if(count == num_neighbors) { 
                // if all messages collected
                break; 
            }

            if (is_message_pushed[i]){
                // if message collected from this neighbor
                continue;
            }
        
            for (int j = 0; j < NUM_TRY_LOCKS; j++) {
                // Try collecting message from this neighbor
                // Try NUM_TRY_LOCKS times
                string s1 = "Node <" + to_string(this->id) + ">: try lock index " + to_string(i) + "...";
                //cout << s1 << endl;
                if (pthread_mutex_trylock(lock_from_neighbors[i]) == 0) {
                    string s2 = "Node <" + to_string(this->id) + ">: get lock "  + to_string(i) + ".";
                    //cout << s2 << endl;
                    // if acquired lock
                    Message* msg = msg_from_neighbors[i];
                    //cout << "Node <" << this->id << ">: finish retrieve message." << endl;
                    if (msg->num_iter == num_iter) {
                        string s3 = "Node <" + to_string(this->id) + ">: get message "  + to_string(i) + ".";
                        //cout << s3 << endl;
                        // if msg has been updated
                        msg_from_neighbors_cache[i] = *msg;
                        msg->is_collected = 1;
                        // copy msg to the cache for computation
                        is_message_pushed[i] = 1;
                        count++;
                    }
                    
                    // if msg from old iteration
                    int err = pthread_mutex_unlock(lock_from_neighbors[i]);
                    assert(err == 0);
                    break;
                }
                
                string s3 = "Node <" + to_string(this->id) + ">:  fail to get lock " + to_string(i) + ".";
                //cout << s3 << endl;
                usleep(1);
            
            }
            // otherwise check message from next neighbor
        }

        s= "Node <"+ to_string(this->id) +">: finish collect message for iteration " + to_string(this->num_iter) + ".\n";
        //cout<<s<<endl;
    }

//TODO: check if from last iteration
//TODO: increment number of iteration
    void computeMsg(){

        string temp= "Node <"+ to_string(this->id) + ">: Start compute message for iter " + to_string(num_iter) + "...\n" ;
        //cout<<temp;
        for(int receiver = 0; receiver < num_parents+num_children; ++receiver) {
            string temp= "Node <"+ to_string(this->id) + ">: for reciever " + to_string(receiver) + "...\n" ;
            //cout<<temp;
            // for each neighbor as receiver
            vector<int> cards(num_parents+1);
            for (int i = 0; i < num_parents; ++i)
                cards[i] = parentsCard[i];
            cards[num_parents] = this->card;
            // the result CPT is over parents and this node           
            vector<NdArray> operands(num_parents+1);
            // for each parent
            for (int i = 0; i < num_parents; ++i) {
                if (i != receiver){
                    // from other neighbors
                    operands[i] = msg_from_neighbors_cache[i].getCPT();//TODO: getCPT(): convert msg to NDarray
                }
                else {
                    // from this receiver, use all ones message as placeholder
                    int card_i = cards[i];
                    NdArray ones = NdArray(1, vector<int> {card_i});
                    vector<double> data(card_i, 1.0);
                    ones.setData(data);
                    operands[i] = ones;
                }

            }


            vector<NdArray> childrenOperands(num_children);
            // for each children
            for (int i = num_parents; i < num_parents + num_children; ++i) {
                if (i != receiver){
                    childrenOperands[i-num_parents] = msg_from_neighbors_cache[i].getCPT();//TODO: getCPT(): convert msg to NDarray
                }
                else {
                    NdArray ones = NdArray(1, vector<int> {this->card});
                    vector<double> data(this->card, 1.0);
                    ones.setData(data);
                    childrenOperands[i-num_parents] = ones;
                }

            }

            temp= "Node <"+ to_string(this->id) + ">: for reciever " + to_string(receiver) + " start child product\n" ;
            //cout<<temp;

            NdArray childProductResult = childProduct(this->card, num_children, childrenOperands);
            temp= "Node <"+ to_string(this->id) + ">: for reciever " + to_string(receiver) + " finish child product\n" ;
            //cout <<temp;
            operands[num_parents] = childProductResult;
            temp= "Node <"+ to_string(this->id) + ">: for reciever " + to_string(receiver) + " start dot product\n" ;
            //cout<<temp;
            NdArray result = dotProduct(this->CPT, operands);
            temp= "Node <"+ to_string(this->id) + ">: for reciever " + to_string(receiver) + " finish dot product\n" ;
            //cout<<temp;
            Message myMsg(id,receiver,this->num_iter+1);
            // if receiver is a parent
            if (receiver < num_parents) {

                result = project(result, receiver);
                // sum out result to the receiver dim
            }
            else {
                result = project(result, num_parents);
                // sum out result to the last dim
            }

            myMsg.setCPT(result);
            msg_to_neighbors_cache[receiver]= myMsg;
        }

        temp= "Node <"+ to_string(this->id) + ">: finish compute message for iter " + to_string(num_iter) + ".\n" ;
        //cout<<temp;
    }

    // Push messages to all neighbors for the next iteration
    void push() {
        string temp= "Node <"+ to_string(this->id) + ">: Start push message for iteration " + to_string(num_iter) + "...\n" ;
        //cout<<temp;
        int is_message_pushed[MAX_NUM_NEIGHBORS];
        memset(is_message_pushed, 0, sizeof(is_message_pushed));
        // keep track of whether an updated message has been collected from this neighbor
        int count = 0;
        int num_neighbors = num_parents + num_children;
        const int NUM_TRY_LOCKS = 3;
        for (int i = 0; i < num_neighbors; i = (i+1) % num_neighbors) { 

            if(count == num_neighbors) { 
                // if all messages pushed
                break; 
            }

            if (is_message_pushed[i]){
                // if message pushed to this neighbor
                continue;
            }

            for (int j = 0; j < NUM_TRY_LOCKS; j++) {
                // Try push message from this neighbor
                // Try NUM_TRY_LOCKS times
                if (pthread_mutex_trylock(lock_to_neighbors[i]) == 0) {
                    // if acquired lock
                    Message msg = msg_to_neighbors_cache[i];
                    if (! msg_to_neighbors[i]->is_collected) {

                        int nid;
                        if (i < num_parents)
                            nid = parents[i];
                        else
                        {
                            nid = children[i-num_parents];
                        }
                        // if message from last iter has not been collected, cannot override
                        temp = "Node <"+ to_string(this->id) + ">: cannot push message to node " + to_string(nid) 
                            + " since message for iteration " + to_string(msg_to_neighbors[i]->num_iter) + " has not been collected yet\n";
                        //cout << temp;
                        pthread_mutex_unlock(lock_to_neighbors[i]);
                        break;
                    }
                    // if message from last iter has been collected
                    *(msg_to_neighbors[i]) = msg;
                    // copy msg to the cache for computation
                    is_message_pushed[i] = 1;
                    count++;
                    pthread_mutex_unlock(lock_to_neighbors[i]);
                    break;
                }

                usleep(1); // wait bewteen two try_locks
            }
            // otherwise check message from next neighbor

        }

        this->num_iter++;
        temp= "Node <"+ to_string(this->id) + ">: finish push message for iteration " + to_string(num_iter) + "...\n" ;
        //cout<<temp;
    }

    NdArray marginals() {

        this->collect(); 
        // collect the messages from final iteration
        vector<NdArray> operands(num_parents+1);
        // for each parent
        for (int i = 0; i < num_parents; ++i) {
            // for each parent
            operands[i] = msg_from_neighbors_cache[i].getCPT();//TODO: getCPT(): convert msg to NDarray
        }

        vector<NdArray> childrenOperands(num_children);
        // for each children
        for (int i = num_parents; i < num_parents + num_children; ++i) {
            childrenOperands[i-num_parents] = msg_from_neighbors_cache[i].getCPT();//TODO: getCPT(): convert msg to NDarray 
        }

        NdArray childProductResult = childProduct(this->card, num_children, childrenOperands);
        string temp = "child product: " + childProductResult.to_string() + "\n";
        //cout << temp;
        operands[num_parents] = childProductResult;
        NdArray result = dotProduct(this->CPT, operands);
        //temp = "product: " + result.to_string() + "\n";
        //cout << temp;
        result = project(result,num_parents);
        return result;

    }

    void clear() {
        this->num_iter = 0;
        lock_from_neighbors= vector<pthread_mutex_t*>(MAX_NUM_NEIGHBORS);
        lock_to_neighbors= vector<pthread_mutex_t*>(MAX_NUM_NEIGHBORS);
        msg_from_neighbors = vector<Message*>(MAX_NUM_NEIGHBORS);
        msg_to_neighbors = vector<Message*>(MAX_NUM_NEIGHBORS);
        msg_from_neighbors_cache = vector<Message>(MAX_NUM_NEIGHBORS);
        msg_to_neighbors_cache = vector<Message>(MAX_NUM_NEIGHBORS);
    }


};
