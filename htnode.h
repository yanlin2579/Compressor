#ifndef HTNODE_H
#define HTNODE_H

#include <string>

class HTNode
{
public:
    char data; //字符
    int weight; //权值
    int parent; //父结点下标
    int lchild; //左孩子结点下标
    int rchild; //右孩子结点下标

public:
    HTNode();  //默认构造函数
    HTNode(char, int, int, int, int);  //构造函数
};

#endif // HTNODE_H
