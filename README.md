A C++ implementation of distributed Iterative Belief Propagation (IBP) <br />
wrote everything from scratch (without using any pre-existing code). probably the first distributed C++ implementation of IBP on the internet.

usage:
g++ -lpthread -std=c++11 node.cpp message.cpp NdArray.cpp main.cpp -o main
./main

In main.cpp, we have implemented a basic BP pipeline in which we randomly set up a network with NUM_NODES nodes and
run belief propagation using NUM_THREADS threads up to NUM_ITERS iterations. The marginal probabilities 
from the last iteration will be saved in marginals_size_NUM_NODES_threads_NUM_THREADS.txt

You can modify the experiment setting, i.e. NUM_NODES and NUM_THREADS at the top of main.cpp AND RECOMPILE; 

We have also implemented a scalability experiment and a speedup experiment which you can uncomment and run
in main.cpp



