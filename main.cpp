#include "node.cpp"
#include <stdlib.h>
#include <pthread.h>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sys/time.h>
#include <time.h>


using namespace std;

///// Experimental settings /////////////////////////
int NUM_NODES = 20;
int NUM_ITERS  = 50;
int NUM_THREADS = 10;
/////////////////////////////////////////////////////

using namespace std;

struct myArgs {
    Node* nodes;
    int size;
    int tid;
    int n_threads;
};

void* do_work(void* args) {

    struct myArgs* myArgs = (struct myArgs*) args;
    Node* nodes = myArgs->nodes; 
    int size = myArgs->size;
    int tid = myArgs->tid;
    int n_threads = myArgs->n_threads;

    for(int i=0;i<NUM_ITERS;++i) {
        // for each iteration
        // for each thread is responsible for doing BP for assigned nodes j
        for (int j=tid; j<size; j+=n_threads) {
            // first pass: collect message
            Node* n = &nodes[j]; // pointer to the jth Node
            string temp = "Node <" + to_string(n->id) + ">: start iteration " + to_string(i) + "...\n";
            //cout << temp << endl;
            n->collect();
        }
        
        for (int j=tid; j<size; j+=n_threads) {
            Node* n = &nodes[j];
            // second pass: compute message for next iteration
            n->computeMsg(); // run BN once
        }

        for (int j=tid; j<size; j+=n_threads) {
            Node* n = &nodes[j];
            // third pass: push message
            n->push();
            string temp = "Node <" + to_string(n->id) + ">: finish iteration " + to_string(i) + ".\n";
            cout << temp;
        }

    }

    string temp = "Thread " + to_string(tid) + " Done!\n";
    //cout << temp;
    return (void *) nodes;
}

void init_locks(Node* myNodes);
void init_messages(Node* myNodes);
// initialize random network with NUM_NODES nodes
// initialize locks and message pipes between two adjacent nodes
// return: array of intialized nodes in network
Node* init_random_BN () {

    cout << "Start initialize network of size " << NUM_NODES << "..." << endl;
    Node* myNodes = new Node[NUM_NODES];
    for (int i=0; i<NUM_NODES;++i) {
        myNodes[i] = Node();
    }

    for(int i=0;i<NUM_NODES;++i){
        //myNodes[i] = Node();
        myNodes[i].id=i;
        myNodes[i].card=rand()%3+2; //a card between 2 and 4
        for(int j=i+1;j<min(i+6,NUM_NODES);++j){
            if(j==i+1){
                myNodes[j].parents.push_back(i);
                myNodes[i].children.push_back(j);
            }
            else{
                int randNumber=rand()%2;
                if(randNumber==0){
                    myNodes[j].parents.push_back(i);
                    myNodes[i].children.push_back(j);
                }
            }
        }
    }

    for(int i=0;i<NUM_NODES;++i){
        vector<int> parentsCard;
        for(int j=0;j<myNodes[i].parents.size();j++){
            parentsCard.push_back(myNodes[ myNodes[i].parents[j] ].card);
        }
        myNodes[i].parentsCard= parentsCard;
        myNodes[i].CPT = init_random_cpt(myNodes[i].card, parentsCard);

        vector<int> childrenCard;
        for(int j=0;j<myNodes[i].children.size();j++){
            childrenCard.push_back(myNodes[ myNodes[i].children[j] ].card);
        }
        myNodes[i].childrenCard= childrenCard;
    }

    for(int i=0;i<NUM_NODES;++i){
        for(int x:myNodes[i].parents){
            myNodes[i].parentsCard.push_back(myNodes[x].card);
        }
        myNodes[i].num_parents=myNodes[i].parents.size();
        for(int y:myNodes[i].children){
            myNodes[i].childrenCard.push_back(myNodes[y].card);
        }
        myNodes[i].num_children=myNodes[i].children.size();
    }


    //initialize mutex lock between each pair of parent and child
    //int lcount = 0;
    init_locks(myNodes);
    // initialize shared message pipes between each pair of parent and child
    init_messages(myNodes);


    for(int i=0;i<NUM_NODES;++i) {
        myNodes[i].sanity_check();
    }

    cout << "Finish initialize network of size " << NUM_NODES << "." << endl;
    return myNodes;

}

void init_locks(Node* myNodes) {

    for(int i=0;i<NUM_NODES;++i){
        //cout << "Initialize Node " << i << " with parents: ";
        //for (int x: myNodes[i].parents) { cout << x << ", "; }
        //cout << endl;
        for(int index=0;index<myNodes[i].num_parents;++index){
            //cout << "Node " << i << ": start initialize locks..." << endl;
            pthread_mutex_t* myMutex = new pthread_mutex_t[1];
            pthread_mutex_init(myMutex,NULL);
            myNodes[i].lock_from_neighbors[index]=myMutex;
            int parentId=myNodes[i].parents[index];
            int childPos=find(myNodes[parentId].children.begin(),myNodes[parentId].children.end(),i)-myNodes[parentId].children.begin();
            childPos=childPos+myNodes[parentId].num_parents;
            myNodes[parentId].lock_to_neighbors[childPos]=myMutex;
            pthread_mutex_t* otherMutex = new pthread_mutex_t[1];
            pthread_mutex_init(otherMutex,NULL);
            myNodes[parentId].lock_from_neighbors[childPos]=otherMutex;
            myNodes[i].lock_to_neighbors[index]=otherMutex;
            
        }
    }

}

void init_messages(Node* myNodes) {

    // initialize shared message pipes from/to between each pair of parent and child
    for(int i=0;i<NUM_NODES;++i){
        //cout << "Node " << i << ": start initialize message..." << endl;
        for(int index=0;index<myNodes[i].num_parents;++index){

            int parentId = myNodes[i].parents[index];
            int parentCard = myNodes[parentId].card;
            Message* myMsg = new Message(parentId,i,0);
            int childPos=find(myNodes[parentId].children.begin(),myNodes[parentId].children.end(),i)-myNodes[parentId].children.begin();
            childPos=childPos+myNodes[parentId].num_parents;

            myNodes[parentId].msg_to_neighbors[childPos] = myMsg;
            myNodes[i].msg_from_neighbors[index] = myMsg;

            NdArray CPT(1,vector<int>(1,parentCard));
            CPT.setData(vector<double>(parentCard, 1.0/parentCard));
            myMsg->setCPT(CPT);

            Message* otherMsg = new Message(i,parentId,0);

            myNodes[parentId].msg_from_neighbors[childPos] = otherMsg;
            myNodes[i].msg_to_neighbors[index] = otherMsg;

            NdArray CPT2(1,vector<int>(1,parentCard));
            CPT2.setData(vector<double>(parentCard, 1.0/parentCard));
            otherMsg->setCPT(CPT2);
        }
        //cout << "Node " << i << ": finish initialize message." << endl;
    }

}

// run parallel belief propagation over network with myNodes
// use NUM_THREADS threads
void belief_propagation(Node* myNodes,int n_threads) {

    pthread_t threads[n_threads];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(& attr, PTHREAD_CREATE_JOINABLE );

    //cout << "Start belief propagation..." << endl;
    for(int i = 0; i < n_threads; ++i ) {

        struct myArgs* args = (struct myArgs*) malloc(sizeof(myArgs));
        args->nodes = myNodes;
        args->size = NUM_NODES;
        args->tid = i;
        args->n_threads = n_threads;
        // starting threads
        int res = pthread_create(&threads[i], &attr, do_work, args);
        if (res) {
            cout << "Error:unable to create thread," << res << endl;
            exit(-1);
        }
    }

    pthread_attr_destroy(&attr);
    // joinning threads
    for( int i = 0; i < n_threads; ++i ) {
        int res = pthread_join(threads[i], NULL);
        
        if (res) {
            cout << "Error:unable to join thread," << res << endl;
            exit(-1);
        }
    }
}

void report(Node* myNodes, int n_threads) {

    string fname = "marginals_size_" + to_string(NUM_NODES) + "_threads_" + to_string(n_threads) + ".txt";
    ofstream out;
    out.open(fname);
    for (int i=0; i<NUM_NODES; ++i) {
        Node n = myNodes[i];
        NdArray marginals = n.marginals();
        // compute the marginals from final iteration
        out << "Node " << n.id << ": " << marginals.to_string() << endl;
    }
    out.close();

}


////////////////////////// Experiments //////////////////////////////////////////////////////////////

// do single experiment of distributed IBP
// randomly generate network of NUM_NODES size
// do parallel BP using NUM_THREADS threads, output belief states to marginals_size_NUM_NODES_threads_NUM_THREADS.txt
// report running time
void do_BP_experiment() {

    cout<<" ----- experiment ----- "<<endl;
    cout<<" Run parallel IBP with " << NUM_THREADS << " threads" <<endl;
    Node* myNodes = init_random_BN();
    struct timeval t1, t2;
    double elapseTime;
    // start timer
    gettimeofday(&t1, NULL);
    // run parallel belief propagation
    belief_propagation(myNodes,NUM_THREADS);
    report(myNodes,NUM_THREADS);
    gettimeofday(&t2, NULL);

    // compute and print the elapsed time in millisec
    elapseTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      
    elapseTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   
    cout << "\n\n";
    cout << "Finish Parallel IBP nodes: " << NUM_NODES << " threads: " << NUM_THREADS << " exec time: " << elapseTime << " s"<<endl;
    cout << "Results are saved in marginals_size_" << NUM_NODES << "_threads_" << NUM_THREADS << ".txt" << endl;
    cout << endl;
}

// Measure the scalability of distributed IBP against sequential IBP on network of increasing sizes
// randomly generate network of size 10,20,...,100
// do sequential and parallel belief propagation using 10 threads
// print execution time in output.txt
void do_speedup_experiment() {

    ofstream out;
    out.open("output.txt");
    out << " ----- experiment ----- "<<endl;
    cout << " ----- experiment ----- "<<endl;
    out << "Speedup of distributed IBP"<<endl;
    cout << "Speedup of distributed IBP"<<endl;

    for (int i=10; i<=100; i+=10) {
        NUM_NODES = i;
        Node* myNodes = init_random_BN();
        struct timeval t1, t2;
        double elapseTime1, elapseTime2;

        cout << "Run sequential IBP"<<endl;
        // start timer
        gettimeofday(&t1, NULL);
        // run sequential belief propagation
        belief_propagation(myNodes,1);
        report(myNodes,1);
        gettimeofday(&t2, NULL);
        // compute and print the elapsed time in millisec
        elapseTime1 = (t2.tv_sec - t1.tv_sec) * 1000.0;      
        elapseTime1 += (t2.tv_usec - t1.tv_usec) / 1000.0;   

        // Important: must clear current states !!!
        for (int i=0; i<NUM_NODES;++i) {
            myNodes[i].clear();
        } 
        init_locks(myNodes);
        init_messages(myNodes);

        cout << "Finish sequential IBP nodes: " << NUM_NODES << " threads: " << 1 << " exec time: " << elapseTime1 << " s"<<endl;
        out << "Finish sequential IBP nodes: " << NUM_NODES << " threads: " << 1 << " exec time: " << elapseTime1 << " s"<<endl;

        // run parallel BP
        int n_threads = NUM_THREADS;
        cout<<" Run parallel IBP with " << n_threads << " threads" <<endl;
        // start timer
        gettimeofday(&t1, NULL);
        // run parallel belief propagation
        belief_propagation(myNodes,n_threads);
        report(myNodes,n_threads);
        gettimeofday(&t2, NULL);
        // compute and print the elapsed time in millisec
        elapseTime2 = (t2.tv_sec - t1.tv_sec) * 1000.0;     
        elapseTime2 += (t2.tv_usec - t1.tv_usec) / 1000.0;  

        double speedup = elapseTime1 / elapseTime2;

        cout << "Finish Parallel IBP nodes: " << NUM_NODES << " threads: " << n_threads << " exec time: " << elapseTime2 << " s "
            << " speedup: " << speedup <<endl;
        out << "Finish Parallel IBP nodes: " << NUM_NODES << " threads: " << n_threads << " exec time: " << elapseTime2 << " s "
            << " speedup: " << speedup <<endl;
 


    }

    cout << "Finish scalability experiment" << endl;
    cout << "Results are saved in output.txt" << endl;
    out.close();
}



// Measure the scalability of distributed IBP on fixed network
// randomly generate network of NUM_NODES size
// do parallel belief propagation using increasing number of threads
// print execution time in output.txt
void do_scalability_experiment() {

    ofstream out;
    out.open("output.txt");
    out << " ----- experiment ----- "<<endl;
    cout << " ----- experiment ----- "<<endl;
    out << "Scalability of distributed IBP"<<endl;
    cout << "Scalability of distributed IBP"<<endl;
    Node* myNodes = init_random_BN();
    double elapseTime_baseline;
    // sample network
    for (int i=1; i<=NUM_THREADS; ++i) {
        int n_threads = i;
        cout<<" Run parallel IBP with " << n_threads << " threads" <<endl;
        struct timeval t1, t2;
        double elapseTime, speedup;
        // start timer
        gettimeofday(&t1, NULL);
        // run parallel belief propagation
        belief_propagation(myNodes,n_threads);
        report(myNodes,n_threads);
        gettimeofday(&t2, NULL);
        // compute and print the elapsed time in millisec
        elapseTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
        elapseTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
        if (i==1) 
            elapseTime_baseline = elapseTime;

        speedup = elapseTime_baseline / elapseTime;
        
        // Important: must clear current states !!!
        for (int i=0; i<NUM_NODES;++i) {
            myNodes[i].clear();
        } 
        init_locks(myNodes);
        init_messages(myNodes);

        cout << "Finish Parallel IBP nodes: " << NUM_NODES << " threads: " << n_threads << " exec time: " << elapseTime << " s "
            << "speed up: " << speedup <<endl;
        out << "Finish Parallel IBP nodes: " << NUM_NODES << " threads: " << n_threads << " exec time: " << elapseTime << " s "
            << "speed up:" << speedup <<endl;
 
    }
    cout << "Finish scalability experiment" << endl;
    cout << "Results are saved in output.txt" << endl;
    out.close();
}



int main() {
    srand(time(NULL));
    do_BP_experiment();
    //do_accuracy_experiment();
    //do_scalability_experiment();
    //do_speedup_experiment();

}


//clean up after everything, destroy all mutexes




