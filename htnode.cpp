#include "htnode.h"

HTNode::HTNode():weight(0), parent(-1),lchild(-1),rchild(-1){}

HTNode::HTNode(char d, int w, int p,int lcd, int rcd):
    data(d),weight(w),parent(p),lchild(lcd),rchild(rcd){}
