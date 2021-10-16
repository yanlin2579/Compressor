#include "huffman.h"
#include <QDebug>

/*                                         压缩文件存放格式
 * 压缩文件头部：  <原文件类型：4字节> <原文件大小：8字节> <注释长度：1字节（2个字节的注释，其长度算为1个长度）> <注释：字节数不定（最长256*2）>
 * 压缩文件字典：  <表长：2字节> {<字符1：1字节> <字符编码长：1字节> <编码：位数不定（最多256位）>}*n
 * 压缩文件数据：  <压缩文件数据部分><补零个数：1字节>
 */

std::string Huffman::byte_to_stringStream(char ch)
{
    unsigned char temp = 0X80;  //二进制：1000,0000，用来取出8位比特流的某一位
    unsigned char item = 0;  //
    std::string str;
    for (int i = 0; i < 8; ++i)
    {
        item = ((temp & ch) >> (7 - i)) + '0';
        temp >>= 1;  //右移一位
        str.push_back(item);
    }

    return str;
}

char Huffman::stringStream_to_byte(const std::string& str)
{
    char byte = 0;
    for (int i = 0; i < 8;++i)
    {
        byte += str.at(i) - '0';
        if (i != 7) byte <<= 1;  //左移一位；if防止左移超过7次导致导致高位丢失，低位补零

    }

    return byte;
}

bool Huffman::weightValueCalculate(const std::string& in_file_path)
{
    std::ifstream in_file(in_file_path, std::ios::binary);
    if(in_file.is_open()==false) return false;

    /*
    * 1.边读入数据边计算权值。
    * 2.由于读取到文件末尾遇到文件结束符EOF时会在此处读取两次，所以加一个if语句避免最后一个字符重复读取。
    */
    char data_ch;
    w_table.clear();
    while (!in_file.eof())
    {
        in_file.get(data_ch);
        if (!in_file.eof())
        {                                               //map容器在用[key]方法访问value时，如果括号内
            w_table[data_ch] = w_table[data_ch] + 1;	//的key不存在，则会新建一个pair插入；且新建的
        }                                               //pair的value为0。
    }

    in_file.close();
    return true;
}

void Huffman::creatHuffmanTree()
{
    int leaf_num = w_table.size();
    int node_num= 2 * leaf_num - 1;  //Huffman树的总结点数=叶子结点树*2-1

    h_tree.clear();
    h_tree.resize(node_num, HTNode());	//初始化Huffman树，并扩大vector容器到node_num大

    /*
    * 为Huffman树的叶子结点设置参数
    */
    auto iter = w_table.begin();
    for (int i=0; i < leaf_num; ++iter, ++i)
    {
        h_tree.at(i).data = iter->first;
        h_tree.at(i).weight = iter->second;
    }

    /*
    * 生成完整的Huffman树
    */
    for (int i = leaf_num; i < node_num; ++i)
    {
        int first_min_node=0;
        int second_min_node = 0;
        for (int j = 0; j < i; ++j)
        {
            if (h_tree.at(j).parent == -1)
            {
                first_min_node = j;
                break;
            }
        }
        for (int j = 0; j < i; ++j)
        {
            if (h_tree.at(j).parent == -1
                && h_tree.at(j).weight < h_tree.at(first_min_node).weight)
                first_min_node = j;
        }
        for (int j = 0; j < i; ++j)
        {
            if (h_tree.at(j).parent == -1
                && j != first_min_node)
            {
                second_min_node = j;
                break;
            }
        }
        for (int j = 0; j < i; ++j)
        {
            if (h_tree.at(j).parent == -1
                && h_tree.at(j).weight < h_tree.at(second_min_node).weight
                && j!=first_min_node)
                second_min_node = j;
        }
        h_tree.at(first_min_node).parent = i;
        h_tree.at(second_min_node).parent = i;
        h_tree.at(i).weight = h_tree.at(first_min_node).weight + h_tree.at(second_min_node).weight;
        h_tree.at(i).lchild = first_min_node;
        h_tree.at(i).rchild = second_min_node;
    }
}

void Huffman::creatHuffmanCodeTable()
{
    int leaf_num = w_table.size();
    int child;
    int parent;
    std::string hc_str;  //存放Huffman编码

    /*
    * 从Huffman树根结点向上，解析出每个字符的Huffman编码，存放在string中，最后逆置string
    */
    hc_table.clear();
    for (int i = 0; i < leaf_num; i++)
    {
        child = i;
        parent = h_tree.at(i).parent;
        while (parent != -1)
        {
            if (h_tree.at(parent).lchild == child)
                hc_str.push_back('0');  //左标0
            else
                hc_str.push_back('1');  //右标1
            child = parent;
            parent = h_tree.at(parent).parent;
        }
        //while结束后hc_str字符串中存放的是逆Huffman编码

        std::reverse(hc_str.begin(), hc_str.end()); //逆置hc_str字符串
        hc_table[h_tree[i].data] = hc_str;  //存入Huffman编码表
        hc_str.clear();  //清空hc_str字符串
    }
}

bool Huffman::compressData_and_createCompressFile(const std::string& in_file_path,
                                                  const std::string& out_file_path)
{
    std::ifstream in_file(in_file_path, std::ios::binary);
    std::ofstream out_file(out_file_path, std::ios::app | std::ios::binary);  //此处以追加方式打开文件，指针指向文件尾
    if(in_file.is_open()==false || out_file.is_open()==false)
        return false;

    char byte;
    std::string hc_substr; //用来存放8个Huffman编码
    int hc_length; //huffman码长度
    int hc_table_length = hc_table.size(); //huffman编码表长度，即字符个数
    auto hc_table_temp(hc_table);

    out_file.write((char*)&hc_table_length, 2); //写入表长
    for (auto& item : hc_table_temp)
    {
        out_file.write((char*)&item.first, 1);  //写入字符
        hc_length = item.second.length();
        if (hc_length % 8 != 0)
        { //不足8个字符补‘0’
            for (int i = hc_length % 8; i < 8; ++i)
                item.second.push_back('0');
        }
        out_file.write((char*)&hc_length, 1);  //写入字符对应的Huffman编码长度
        for (int i = 0; i < item.second.length(); i += 8)
        {  //写入补0后的Huffman编码，以8位为单位写
            hc_substr = item.second.substr(i, 8);
            byte = stringStream_to_byte(hc_substr);
            out_file.put(byte);
        }
    }

    /*
    * 1.读入文件数据；
    * 2.根据Huffman编码表，将数据压缩成二进制比特流。每转换8位，输出一次，最后部分不足8位的补0，并在文件末尾写上补0个数。
    * 3.存放形式为：{<数据><补0个数>}
    */
    char data_ch;
    std::string data_str;
    std::string data_substr;
    int data_str_length=0;
    int zero_count=0;

    while(!in_file.eof())
    {
        for(int i=0; i<8 && !in_file.eof(); ++i)
        {
            in_file.get(data_ch);
            if(!in_file.eof())
                data_str+=hc_table.at(data_ch);
        }



    }

//    while (!in_file.eof()) {  //读入数据并通过Huffman编码表求得数据对应的‘0’‘1’字符串；if语句避免最后读两次
//        in_file.get(data_ch);
//        if (!in_file.eof()) {
//            data_str += hc_table.at(data_ch);
//        }
//    }
//    data_str_length = data_str.size();

//    int i;
//    for (i = 0; i < data_str_length - 8; i += 8) {  //将前8*(n-1)个‘0’‘1’字符串翻译为n-1个字节的二进制比特流，并写入到文件中
//        data_substr = data_str.substr(i, 8);
//        char byte = stringStream_to_byte(data_substr);
//        out_file.put(byte);
//    }
//    if (i > data_str_length - 8) {  //补0，写入最后一个字节的数据，最后再写入补0位数
//        zero_count = i - (data_str_length - 8);
//        for (int j = 0; j < zero_count; ++j) {
//            data_str.push_back('0');
//        }
//        data_substr = data_str.substr(i, 8);
//        char byte = stringStream_to_byte(data_substr);
//        out_file.put(byte).put(zero_count);
//    }
//    else
//        out_file.put(0);


    in_file.close();
    out_file.close();
    return true;
}

bool Huffman::decompressData_and_createPrimaryFile(const std::string& in_file_path,
                                                   const std::string& out_file_path,
                                                   const int cmpfile_headersize)
{
    std::ifstream in_file(in_file_path, std::ios::binary);
    std::ofstream out_file(out_file_path, std::ios::binary);
    if(in_file.is_open()==false || out_file.is_open()==false)
        return false;

    /*
    * 从压缩文件头部求得Huffman编码表
    */
    int hc_length;  //Huffman编码长度
    int hc_table_length; //Huffman表长度
    int zero_count;  //补0个数
    std::string hc_str;  //一串Huffman编码
    char hc_ch; //Huffman编码表中的一个字符
    char byte;  //一个字节，用来从文件中以字节为单位读取
    char16_t char16_temp;

    rhc_table.clear();
    in_file.seekg(cmpfile_headersize,std::ios::beg);  //跳过压缩文件头部
    in_file.read((char*)&char16_temp, 2);
    hc_table_length= (int)char16_temp;  //求得表长
    for (int i = 0; i < hc_table_length; ++i) {  //求出反Huffman表
        in_file.read((char*)&hc_ch, 1);
        in_file.read((char*)&byte, 1);
        hc_length = (int)byte;
        zero_count = (8 - hc_length % 8) %8;
        for (int j = 0; j < hc_length + zero_count; j += 8) {
            in_file.read((char*)&byte, 1);
            hc_str += byte_to_stringStream(byte);
        }
        for (int j = 0; j < zero_count; ++j) {  //去掉补0部分
            hc_str.pop_back();
        }
        rhc_table[hc_str] = hc_ch;  //反Huffman表即将编码放在first，字符放在second
        hc_str.clear();
    }

    /*
    * 1.读入压缩数据部分；
    * 2.将二进制比特流翻译为‘0’‘1’字符串；
    * 3.根据Huffman编码表译码。
    */
    char data_ch;
    std::string data_str;
    std::string data_substr;

    while (!in_file.eof()) {  //读入数据部分，并将二进制比特流翻译为‘0’‘1’字符流
        in_file.get(data_ch);
        if (!in_file.eof())
            data_str += byte_to_stringStream(data_ch);
    }

    in_file.clear();  //清除in_file异常标志，因为在读到文件末尾遇到EOF异常后就无法对in_file指针进行重定位
    in_file.seekg(-1, std::ios::end);  //in_file指针指向文件倒数第二位（倒数第一位是EOF），此处存放的补0个数
    zero_count= in_file.get();

    data_str.erase(data_str.end() - zero_count - 8, data_str.end());  //去掉补0部分和最后一个字节

    auto iter = rhc_table.begin();
    bool found_flag;

    for (int i = 0; i < data_str.size(); ++i)  //查Huffman编码表解码
    {
        found_flag = true;
        for (int j = 1; found_flag && (i + j) <= data_str.size(); ++j)
        {
            data_substr = data_str.substr(i, j);
            iter = rhc_table.find(data_substr);
            if (iter != rhc_table.end())
            {
                out_file.put(iter->second);
                i += j - 1;
                found_flag = false;
            }
        }
    }

    in_file.close();
    out_file.close();
    return true;
}

bool Huffman::compressFile(const std::string& in_file_path,
                           const std::string& out_file_path)
{
    if(weightValueCalculate(in_file_path))
    {
        creatHuffmanTree();
        creatHuffmanCodeTable();
        if(compressData_and_createCompressFile(in_file_path, out_file_path))
            return true;
    }
    else
        return false;
}

bool Huffman::decompressFile(const std::string& in_file_path,
                             const std::string& out_file_path,
                             const int cmpfile_headersize)
{
    if(decompressData_and_createPrimaryFile(in_file_path, out_file_path, cmpfile_headersize))
        return true;
    else
        return false;
}
