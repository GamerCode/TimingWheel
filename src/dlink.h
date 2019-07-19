
#ifndef __DLINK_H__
#define __DLINK_H__
#include <pthread.h>

/*
简单双向链表实现
 */

struct dlinklist_t;
struct dlinklist_node_t{
    dlinklist_t *lst=nullptr;;
    dlinklist_node_t *prev=nullptr;;
    dlinklist_node_t *next=nullptr;;
};

struct dlinklist_t{
    dlinklist_node_t *head=nullptr;
    dlinklist_node_t *tail=nullptr;;
    pthread_mutex_t locker = PTHREAD_MUTEX_INITIALIZER;
    
    void lock(){
        pthread_mutex_lock(&locker);
    }

    void unlock(){
        pthread_mutex_unlock(&locker);
    }
};

template<class Node = dlinklist_node_t,class Link = dlinklist_t>
void RemoveFromLink(Node* node){

    if(!node || !node->lst){
        return;
    }
    
    Link *lst = node->lst;
    //只有一个元素的时候，是头也是尾
    // 如果是头
    lst->lock();
    if(lst->head == node){
        lst->head = node->next;
        if(lst->head){
            lst->head->prev = nullptr;
        }
    }else{
        node->prev->next = node->next;
    }

    // 如果是尾
    if(lst->tail == node){
        lst->tail = node->prev;
        if (lst->tail){
            lst->tail->next = nullptr;
        }
    }else{
        node->next->prev = node->prev;
    }
    node->prev = node->next = nullptr;
    node->lst = nullptr;
    lst->unlock();
}

template<class Node = dlinklist_node_t,class Link = dlinklist_t>
void AddHead(Link *lst, Node *node){
    if (node->lst){
        RemoveFromLink<Node,Link>(node);
    }
    lst->lock();
    if(lst->head){
        node->next = lst->head;
        lst->head->prev = node;
        lst->head = node;
        lst->head->prev = nullptr;
    }else{
        lst->head = lst->tail = node;
        node->prev = node->next = nullptr;
    }
    node->lst = lst;
    lst->unlock();

}

template<class Node = dlinklist_node_t,class Link = dlinklist_t>
void AddTail(Link *lst, Node *node){
    if (node->lst){
        RemoveFromLink<Node,Link>(node);
    }
    if(lst != node->lst){
        lst->lock();
        if(lst->tail){
            node->prev = lst->tail;
            lst->tail->next = node;
            lst->tail = node;
            lst->tail->next = nullptr;
        }else{
            lst->head = lst->tail = node;
            node->prev = node->next = nullptr;
        }
        node->lst = lst;
        lst->unlock(); 
    }
}

template<class Node = dlinklist_node_t,class Link = dlinklist_t>
void InsertBefore(Node *pos, Node *node){
    if(pos == pos->lst->head){
        AddHead(pos->lst,node);
    }else{
        pos->lst->lock();
        node->next = pos;
        node->prev = pos->prev;
        node->prev->next = node;
        pos->prev = node;
        node->lst = pos->lst;
        pos->lst->unlock();
    }
}

template<class Node = dlinklist_node_t,class Link = dlinklist_t>
void InsertAfter(Node *pos, Node *node){
    if(pos == pos->lst->tail){
        AddTail(pos->lst,node);
    }else{
        pos->lst->lock();
        node->next = pos->next;
        node->prev = pos;
        pos->next = node;
        node->lst = pos->lst;
        pos->lst->unlock();
    }
}

template<class Link = dlinklist_t>
void* PoPHead(Link *lst){
    void *head = nullptr;
    if(lst){
        lst->lock();
        head = lst->head;
        lst->unlock();
    }
    return head;
}

template<class Node = dlinklist_node_t,class Link = dlinklist_t>
Node* NextNode(Node* node){
    Node* next = nullptr;
    if(node && node->lst){
        Link *lst = node->lst;
        lst->lock();
        next = (Node*)node->next;
        lst->unlock();
    }
    return next;
}

using p_func = void(void*);
void PrintLink(dlinklist_t* lst,  p_func* p){
    dlinklist_node_t *node = (dlinklist_node_t *)PoPHead(lst);
    while(node){
        p(node);
        node = (dlinklist_node_t *)NextNode(node);
    }
}
#endif