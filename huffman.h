#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "htnode.h"

/*                                         压缩文件存放格式
 * 压缩文件头部：  <原文件类型：4字节> <原文件大小：8字节> <注释长度：1字节（2个字节的注释，其长度算为1个长度）> <注释：字节数不定（最长256*2）>
 * 压缩文件字典：  <表长：2字节> {<字符1：1字节> <字符编码长：1字节> <编码：位数不定（最多256位）>}*n
 * 压缩文件数据：  <压缩文件数据部分><补零个数：1字节>
 */

class Huffman {

public:
    bool compressFile(const std::string&, const std::string&);
    bool decompressFile(const std::string&, const std::string&, const int);

private:
    std::string byte_to_stringStream(char);
    char stringStream_to_byte(const std::string&);

    bool weightValueCalculate(const std::string&);  //权值计算函数
    void creatHuffmanTree();
    void creatHuffmanCodeTable();
    bool compressData_and_createCompressFile(const std::string&, const std::string&);
    bool decompressData_and_createPrimaryFile(const std::string&, const std::string&, const int);

private:
    std::vector<HTNode> h_tree;  //哈夫曼树
    std::map<char, unsigned int> w_table; //权值表
    std::map<char, std::string> hc_table;  //编码时哈夫曼编码表
    std::map<std::string, char> rhc_table;  //解码时哈夫曼编码表

};

#endif // HUFFMAN_H
