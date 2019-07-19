#include <iostream>
#include <random>
#include <time.h>
#include "dlink.h"

struct dlinklist_t *lst;

struct num_node_t : public dlinklist_node_t{
    int val = 0;
};

void P(void* node){
    if (!node){
        return;
    }
    std::cout << ((num_node_t*)node)->val <<",";
}

int main(int argc, char const *argv[])
{
    time_t t;
    time(&t);
    std::default_random_engine e(t);

    dlinklist_t *lst = new dlinklist_t;
    num_node_t * rand_node = nullptr;

    for (auto n =0 ;n < 300 ; n++){
        num_node_t *node = new num_node_t;
        node->val = n;
        //std::cout << "new: node val -> " << ((num_node_t*)node)->val << std::endl;
        //PrintLink(lst,P);
        switch (e()%5){
            case 0:
                //std::cout << "AddHead: node val -> " << ((num_node_t*)node)->val << std::endl;
                AddHead<num_node_t,dlinklist_t>(lst,node);
                rand_node = node;
                break;
            case 1:
                //std::cout << "AddTail: node val -> " << ((num_node_t*)node)->val << std::endl;
                AddTail<num_node_t,dlinklist_t>(lst,node);
                rand_node = node;
                break;
            case 2:
                if (rand_node){
                    //std::cout << "InsertBefore:" << ((num_node_t*)node)->val <<"->"<<rand_node->val<< std::endl;
                    InsertBefore<num_node_t,dlinklist_t>(rand_node,node);
                }else{
                    //std::cout << "AddHead: node val -> " << ((num_node_t*)node)->val << std::endl;
                    AddHead<num_node_t,dlinklist_t>(lst,node);
                }
                rand_node = node;
                break;
            case 3:
                if (rand_node){
                    //std::cout << "InsertAfter: node val -> " << ((num_node_t*)node)->val << std::endl;
                    InsertAfter<num_node_t,dlinklist_t>(rand_node,node);
                }else{
                    //std::cout << "AddTail: node val -> " << ((num_node_t*)node)->val << std::endl;
                    AddTail<num_node_t,dlinklist_t>(lst,node);
                }
                rand_node = node;
                break;
            case 5:
                if(rand_node){
                    RemoveFromLink<num_node_t,dlinklist_t>(rand_node);
                    delete rand_node;
                    rand_node = nullptr;
                    delete node;
                }
                break;
        }
    }
    PrintLink(lst,P);
    delete lst;
    lst = nullptr;
    return 0;
}
