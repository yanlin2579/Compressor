#include "mainwindow.h"

/*                                         压缩文件存放格式
 * 压缩文件头部：  <原文件类型：4字节> <原文件大小：8字节> <注释长度：1字节（2个字节的注释，其长度算为1个长度）> <注释：字节数不定（最长256*2）>
 * 压缩文件字典：  <表长：2字节> {<字符1：1字节> <字符编码长：1字节> <编码：位数不定（最多256位）>}*n
 * 压缩文件数据：  <压缩文件数据部分><补零个数：1字节>
 */

/*********************************************************************************************
 *                                  构造与析构
 *********************************************************************************************/
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) , ui(new Ui::MainWindow) ,
      m_hf(new Huffman) , m_cmpfile_headsize(13),
      m_act_recent_files_empty(nullptr)
{
    this->setAcceptDrops(true);  //启用放下操作，用于拖拽动作触发
    ui->setupUi(this);  
    readResources();  //加载资源
    initUi();  //初始化UI
}

MainWindow::~MainWindow()
{
    delete m_hf;  //删除Huffman对象
    m_hf=nullptr;

    delete ui;  //删除ui对象
    ui=nullptr;

    delete m_act_recent_files_empty;  //删除Action对象
    m_act_recent_files_empty=nullptr;

    m_act_recent_files.clear();  //删除多个Action对象
}



/*********************************************************************************************
 *                                 UI设置函数
 *********************************************************************************************/
inline void MainWindow::initUi()
{
    resetMainWindow();
    resetButtons();
    resetActions();
    resetLabels();
    resetTextEdit();
    resetTable(false);
    refreshRecentFileActions();
}

void MainWindow::connectSignalSlots()
{
    for(auto &act_rf : m_act_recent_files)
    {
        connect(act_rf,SIGNAL(triggered()),this,SLOT(openActRecentFile()));
    }
}

//将频繁会被调用的函数声明成inline（内联函数）可提高执行速度，但不能用于过长的、有for、while、递归的、构造、析构函数，且只声明不定义不起作用，只定义或者定义加声明才有效
inline void MainWindow::resetMainWindow()
{
    this->setFixedHeight(235);
}

inline void MainWindow::resetButtons()
{
    ui->btn_compress->setHidden(true);
    ui->btn_decompress->setHidden(true);
}

inline void MainWindow::resetActions()
{
    ui->act_close_file->setDisabled(true);
    ui->act_save_file_as->setDisabled(true);
    ui->act_decode->setDisabled(true);
    ui->act_encode->setDisabled(true);
    ui->act_add_cmpfile_comment->setDisabled(true);
    ui->act_delete_cmpfile_comment->setDisabled(true);
}

inline void MainWindow::resetLabels()
{
    ui->label_file_comment->setText("请“文件”菜单中点击“打开文件”，或将文件拖动至此窗口");
    ui->label_file_comment->setHidden(false);
}

inline void MainWindow::resetTextEdit()
{
    ui->text_file_comment->setHidden(true);
    ui->text_file_comment->clear();
    ui->text_file_comment->setReadOnly(true);
}

void MainWindow::resetTable(bool visible)
{
    QTableWidgetItem *item;
    QStringList table_headers_text;
    table_headers_text<<"文件名称"<<"文件类型"<<"文件大小"<<"文件路径";

    ui->table_file_info->clear();  //清除整个表格，回收空间
    ui->table_file_info->setColumnCount(table_headers_text.size());  //设置列数
    for(int i=0; i<table_headers_text.count(); ++i)  //初始化表头
    {
        item=new QTableWidgetItem;
        item->setText(table_headers_text.at(i));
        ui->table_file_info->setHorizontalHeaderItem(i,item);
    }
    ui->table_file_info->setFixedHeight(110);
    ui->table_file_info->horizontalHeader()->setVisible(true);
    ui->table_file_info->setVisible(visible);
}

void MainWindow::refreshRecentFileActions()
{
    QAction *act_recent_file;

    for(int i=0; i<m_act_recent_files.count(); ++i)  //去掉m_recent_files_path中已经有的文件路径
    {
        m_recent_files_path.removeOne(m_act_recent_files.at(i)->text());
    }

    for(QString rfp : m_recent_files_path)  //对新读入的文件建立Action
    {
        act_recent_file=new QAction;
        act_recent_file->setText(rfp);
        act_recent_file->setCheckable(true);
        m_act_recent_files.push_back(act_recent_file);  //将建立的Action记录在m_act_recent_files中
        ui->menu_recent_files->addActions(m_act_recent_files);  //并添加到UI界面

        connect(act_recent_file,SIGNAL(triggered()),this,SLOT(actOpenRecentFile()));  //连接Action信号与槽函数actOpenRecentFile()
    }

    if(m_act_recent_files.isEmpty())  //如果最近没有打开过的文件，则新建一个无法点击的空Action通知用户
    {
        if(m_act_recent_files_empty==nullptr)
        {
            m_act_recent_files_empty=new QAction;
            m_act_recent_files_empty->setText("（空）");
            m_act_recent_files_empty->setDisabled(true);
            ui->menu_recent_files->addAction(m_act_recent_files_empty);
        }
    }
    else
    {
        if(m_act_recent_files_empty!=nullptr)  //如果最近有了打开过的文件，则删除空Action
        {
            ui->menu_recent_files->removeAction(m_act_recent_files_empty);
            delete m_act_recent_files_empty;
            m_act_recent_files_empty=nullptr;
        }
    }
}

void MainWindow::refreshFileInfoTable()
{
    if(!m_in_file_info.filePath().isEmpty())
    {
        QTableWidgetItem *item;

        /*更新表格*/
        item=new QTableWidgetItem;
        item->setText(m_in_file_info.completeBaseName());
        item->setTextAlignment(Qt::AlignCenter);
        ui->table_file_info->setItem(0,0,item);  //在表格中设置当前文件名

        item=new QTableWidgetItem;
        if(m_in_file_info.suffix()=="hfm")
        {
            item->setText(m_in_file_info.suffix().toUpper()+"压缩文件");
        }
        else
            item->setText(m_in_file_info.suffix().toUpper());
        item->setTextAlignment(Qt::AlignCenter);
        ui->table_file_info->setItem(0,1,item);  //在表格中设置当前文件类型

        qint64 in_file_size=m_in_file_info.size();
        item=new QTableWidgetItem;
        if(in_file_size<1024)
            item->setText(QString::number(in_file_size)+"b");
        else if(in_file_size<1024*1024)
            item->setText(QString::number(in_file_size/1024.,'d',2)+"kb");
        else if(in_file_size<1024*1024*1024)
            item->setText(QString::number(in_file_size/(1024.*1024.),'d',2)+"Mb");
        else
            item->setText(QString::number(in_file_size/(1024.*1024.*1024.),'d',2)+"Gb");
        item->setTextAlignment(Qt::AlignCenter);
        ui->table_file_info->setItem(0,2,item);  //在表格中设置当前文件大小

        item=new QTableWidgetItem;
        item->setText(m_in_file_info.path());
        item->setTextAlignment(Qt::AlignCenter);
        ui->table_file_info->setItem(0,3,item);  //在表格中设置当前文件路径

        if(m_in_file_info.suffix()=="hfm")
        {
            ui->table_file_info->insertColumn(3);  //在表格的第3列前，插入新的列
            item=new QTableWidgetItem;
            item->setText("原文件类型");
            item->setTextAlignment(Qt::AlignCenter);
            ui->table_file_info->setHorizontalHeaderItem(3,item);  //设置新列的header为"原文件类型"
            item=new QTableWidgetItem;
            item->setText(m_cmpfile_original_type.toUpper()+"文件");
            item->setTextAlignment(Qt::AlignCenter);
            ui->table_file_info->setItem(0,3,item);  //在表格中设置原文件类型

            ui->table_file_info->insertColumn(4);  //在表格的第4列前，插入新的列
            item=new QTableWidgetItem;
            item->setText("原文件大小");
            item->setTextAlignment(Qt::AlignCenter);
            ui->table_file_info->setHorizontalHeaderItem(4,item);  //设置新列的header为"原文件大小"
            item=new QTableWidgetItem;
            if(m_cmpfile_original_size<1024)
                item->setText(QString::number(m_cmpfile_original_size)+"b");
            else if(m_cmpfile_original_size<1024*1024)
                item->setText(QString::number(m_cmpfile_original_size/1024.,'d',2)+"kb");
            else if(m_cmpfile_original_size<1024*1024*1024)
                item->setText(QString::number(m_cmpfile_original_size/(1024.*1024.),'d',2)+"Mb");
            else
                item->setText(QString::number(m_cmpfile_original_size/(1024.*1024.*1024.),'d',2)+"Gb");
            item->setTextAlignment(Qt::AlignCenter);
            ui->table_file_info->setItem(0,4,item);  //在表格中设置原文件大小

            ui->table_file_info->insertColumn(5);  //在表格的第5列前，插入新的列
            item=new QTableWidgetItem;
            item->setText("压缩率");
            item->setTextAlignment(Qt::AlignCenter);
            ui->table_file_info->setHorizontalHeaderItem(5,item);  //设置新列的header为"压缩率"
            item=new QTableWidgetItem;
            item->setText(QString::number((m_in_file_info.size()*100.0)/m_cmpfile_original_size,'d',1)+"%");
            item->setTextAlignment(Qt::AlignCenter);
            ui->table_file_info->setItem(0,5,item);  //在表格中设置压缩率
        }
    }
}

void MainWindow::refreshMainWindow()
{
    if(!m_in_file_info.filePath().isEmpty())
    {
        refreshFileInfoTable();  //更新表格
        ui->table_file_info->setHidden(false);  //更新完成后显示表格

        ui->act_close_file->setDisabled(false);  //启用关闭文件按钮

        ui->act_save_file_as->setDisabled(false);  //启用另存为按键
        ui->act_add_cmpfile_comment->setDisabled(false);  //启用添加注释按钮

        if(m_in_file_info.suffix()=="hfm" )  //如果是压缩文件，则显示压缩界面
        {
            ui->btn_decompress->setHidden(false);
            ui->label_file_comment->setHidden(true);
            ui->act_delete_cmpfile_comment->setDisabled(true);
            ui->act_add_cmpfile_comment->setDisabled(true);

            if(m_cmpfile_comment_length!=0)  //如果压缩文件有注释，则显示注释
            {
                this->setFixedHeight(400);  //重置主窗口大小
                ui->label_file_comment->setText("压缩文件注释");
                ui->label_file_comment->setHidden(false);
                ui->text_file_comment->setPlainText(m_cmpfile_comment);  //将压缩文件注释写入输入框
                ui->text_file_comment->setReadOnly(true);  //设置输入框只读模式
                ui->text_file_comment->setHidden(false);  //显示输入框
            }
        }
        else
        {
            ui->label_file_comment->setHidden(true);  //隐藏标签
            ui->btn_compress->setHidden(false);  //显示压缩按钮
        }
    }
}



/*********************************************************************************************
 *                                   槽函数
 *********************************************************************************************/
void MainWindow::on_act_open_file_triggered()
{
    readResources();  //加载资源
    initUi();  //初始化UI界面
    resetMemberVariable();  //重置成员变量

    QString current_file_path=QDir::currentPath();  //获取当前目录
    QString dialog_title="打开文件";  //对话窗口标题
    QString file_filter="所有文件(*.*);;压缩文件(*.hfm);;文本文件(*.txt *.doc *.pptx *.xls);;图片(*.bmp *.png)";  //文件过滤器
    m_in_file_info=QFileDialog::getOpenFileName(this,dialog_title,current_file_path,file_filter);  //打开文件，返回文件路径来更新文件信息

    if(!m_in_file_info.filePath().isEmpty())  //防止用户未选择文件而返回了空路径
    {
        if(m_in_file_info.size()!=0)  //防止用户打开空文件
        {
            writeResources();  //写入资源
            if(m_in_file_info.suffix()=="hfm")   //若当前文件为压缩文件，则读入压缩文件头部信息
                readCmpfileHeader();
            refreshMainWindow();  //刷新主窗体界面
        }
        else
        {
            initUi();
            QMessageBox::information(this,"提示","该文件为空文件，请重试",QMessageBox::Ok,QMessageBox::NoButton);
            return;
        }
    }
}

void MainWindow::on_btn_compress_clicked()
{
    QString current_file_path=QDir::currentPath();  //获取当前目录
    QString dialog_title="保存压缩文件";  //对话窗口标题
    QString file_filter="HFM压缩文件(*.hfm)";  //文件过滤器
    m_out_file_path=QFileDialog::getSaveFileName(this,dialog_title,current_file_path,file_filter);  //打开文件，返回文件路径

    if(!m_out_file_path.isEmpty())
    {
        writeCmpfileHeader();  //写入压缩文件头部信息

        QMessageBox::information(this,"提示","正在压缩...",QMessageBox::Ok,QMessageBox::NoButton);
        if(!m_hf->compressFile(m_in_file_info.filePath().toLocal8Bit().constData(),m_out_file_path.toLocal8Bit().constData()))  //压缩
        {
            initUi();
            QMessageBox::information(this,"提示","文件打开错误，请重试",QMessageBox::Ok,QMessageBox::NoButton);
            return;
        }
        QMessageBox::information(this,"提示","压缩成功",QMessageBox::Ok,QMessageBox::NoButton);
    }
}

void MainWindow::on_btn_decompress_clicked()
{
    QString current_file_path=QDir::currentPath();  //获取当前目录
    QString dialog_title="保存解压文件";  //对话窗口标题
    QString file_filter=m_cmpfile_original_type.toUpper()+ "文件(*."+m_cmpfile_original_type+")";  //文件过滤器
    m_out_file_path=QFileDialog::getSaveFileName(this,dialog_title,current_file_path,file_filter);  //打开文件，返回文件路径


    if(!m_out_file_path.isEmpty())
    {
        QMessageBox::information(this,"提示","正在解压...",QMessageBox::Ok,QMessageBox::NoButton);
        if(!m_hf->decompressFile(m_in_file_info.filePath().toLocal8Bit().constData(),m_out_file_path.toLocal8Bit().constData(),m_cmpfile_headsize))
        {
            initUi();
            QMessageBox::information(this,"提示","文件打开错误，请重试",QMessageBox::Ok,QMessageBox::NoButton);
            return;
        }
        QMessageBox::information(this,"提示","解压成功",QMessageBox::Ok,QMessageBox::NoButton);
    }
}

void MainWindow::on_act_exit_triggered()
{
    this->destroy(true,true);  //关闭当前应用
}

void MainWindow::on_act_close_file_triggered()
{
    readResources();  //加载资源
    initUi();
    resetMemberVariable();
}

void MainWindow::on_act_save_file_as_triggered()
{
    QString current_file_path=QDir::currentPath();  //获取当前目录
    QString dialog_title="打开文件";  //对话窗口标题
    QString file_filter="."+m_in_file_info.suffix()+";;所有文件(*.*)";  //文件过滤器
    m_out_file_path=QFileDialog::getSaveFileName(this,dialog_title,current_file_path,file_filter);
    QFile::copy(m_in_file_info.filePath(),m_out_file_path);
}

void MainWindow::on_text_file_comment_textChanged()
{
    m_cmpfile_comment=ui->text_file_comment->toPlainText();

    int length = m_cmpfile_comment.count();
    int max_length = 256; // 最大字符数

    if(length > max_length)
    {
        int position = ui->text_file_comment->textCursor().position();
        QTextCursor textCursor = ui->text_file_comment->textCursor();
        m_cmpfile_comment.remove(position - (length - max_length), length - max_length);
        ui->text_file_comment->setPlainText(m_cmpfile_comment);
        textCursor.setPosition(position - (length - max_length));
        ui->text_file_comment->setTextCursor(textCursor);
    }

    if(length > 0)
    {
        if(m_cmpfile_comment.back()==' '
                ||m_cmpfile_comment.back()=='\n'
                ||m_cmpfile_comment.back()=='\r'
                ||m_cmpfile_comment.back()=='\t'
                ||m_cmpfile_comment.back()=='\v'
                ||m_cmpfile_comment.back()=='\f')
        {
            int position = ui->text_file_comment->textCursor().position();
            QTextCursor textCursor = ui->text_file_comment->textCursor();
            m_cmpfile_comment.remove(position - 1, 1);
            ui->text_file_comment->setPlainText(m_cmpfile_comment);
            textCursor.setPosition(position - 1);
            ui->text_file_comment->setTextCursor(textCursor);
        }
    }
}

void MainWindow::on_act_add_cmpfile_comment_triggered()
{
    this->setFixedHeight(400);
    ui->act_add_cmpfile_comment->setDisabled(true);  //屏蔽添加注释按钮
    ui->act_delete_cmpfile_comment->setDisabled(false);
    ui->label_file_comment->setText("压缩文件注释");
    ui->label_file_comment->setHidden(false);
    ui->text_file_comment->setHidden(false);
    ui->text_file_comment->setReadOnly(false);
}

void MainWindow::on_act_delete_cmpfile_comment_triggered()
{
    this->setFixedHeight(235);
    ui->act_add_cmpfile_comment->setText("添加注释");
    ui->act_add_cmpfile_comment->setDisabled(false);
    ui->act_delete_cmpfile_comment->setDisabled(true);
    ui->label_file_comment->setHidden(true);
    ui->text_file_comment->clear();
    ui->text_file_comment->setHidden(true);
    ui->text_file_comment->setReadOnly(true);
}

void MainWindow::actOpenRecentFile()
{
    initUi();  //初始化UI界面
    resetMemberVariable();  //重置成员变量

    for(int i=0; i<m_act_recent_files.count(); ++i)
    {
        if(m_act_recent_files.at(i)->isChecked())
        {
            m_in_file_info=m_act_recent_files.at(i)->text();
            m_act_recent_files.at(i)->setChecked(false);

            if(!m_in_file_info.filePath().isEmpty())  //防止用户未选择文件而返回了空路径
            {
                if(m_in_file_info.size()!=0)  //防止用户打开空文件
                {
                    writeResources();  //写入资源
                    if(m_in_file_info.suffix()=="hfm")   //若当前文件为压缩文件，则读入压缩文件头部信息
                        readCmpfileHeader();
                    refreshMainWindow();
                }
                else
                {
                    initUi();
                    QMessageBox::information(this,"提示","该文件为空文件，请重试",QMessageBox::Ok,QMessageBox::NoButton);
                    return;
                }
            }
        }
    }

}



/*********************************************************************************************
 *                                   拖拽事件函数
 *********************************************************************************************/
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    readResources();  //加载资源
    initUi();  //初始化UI界面
    resetMemberVariable();  //重置成员变量

    QList<QUrl> urls = event->mimeData()->urls();  //获取文件URL
    if(urls.size()>1)
    {
        QMessageBox::information(this,"提示","只能选择一个文件，请重试",QMessageBox::Ok,QMessageBox::NoButton);
        return;
    }
    m_in_file_info = urls.at(0).toLocalFile();

    if(!m_in_file_info.filePath().isEmpty())  //防止用户未选择文件而返回了空路径
    {
        if(m_in_file_info.size()!=0)  //防止用户打开空文件
        {
            writeResources();  //写入资源
            if(m_in_file_info.suffix()=="hfm")   //若当前文件为压缩文件，则读入压缩文件头部信息
                readCmpfileHeader();
            refreshMainWindow();  //刷新主窗体界面
        }
        else
        {
            initUi();
            QMessageBox::information(this,"提示","该文件为空文件，请重试",QMessageBox::Ok,QMessageBox::NoButton);
            return;
        }
    }
}



/*********************************************************************************************
 *                       功能函数（用于其他的函数（槽，事件）调用，实现某一具体功能）
 *********************************************************************************************/
inline void MainWindow::resetMemberVariable()
{
    QString null_str;
    m_in_file_info=null_str;
    m_out_file_path.clear();
    m_recent_files_path.clear();
    m_cmpfile_comment.clear();
    m_cmpfile_original_size=0;
    m_cmpfile_original_type.clear();
    m_cmpfile_comment_length=0;
    m_cmpfile_headsize=13;
}

void MainWindow::readResources()
{
    QFile file("recentfiles.txt");  //打开应用资源，recentfiles.txt里按行存放的最近文件路径

    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream txt_stream(&file);  //用QTextStream处理txt文本更方便，可消除格式问题
        m_recent_files_path.clear();  //初始化QStringList
        while(!txt_stream.atEnd())
        {
            m_recent_files_path<<txt_stream.readLine();  //按行读入最近文件路径
        }
        file.close();
    }
}

void MainWindow::writeCmpfileHeader()
{
    std::string cmpfile_comment=m_cmpfile_comment.toLocal8Bit().data();
    qint8 cmpfile_comment_size=cmpfile_comment.length();
    m_cmpfile_headsize=13+cmpfile_comment_size;

    std::ofstream out_file(m_out_file_path.toLocal8Bit().constData(),std::ios::binary);  //打开并创建压缩文件
    if(out_file.is_open())
    {
        qint64 in_file_size;

        out_file<<m_in_file_info.suffix().toStdString();  //将原文件后缀存入压缩文件头部
        if(m_in_file_info.suffix().size()==3)  //如果原文件后缀只有三位，则追加一个空格
            out_file.put(' ');

        in_file_size=m_in_file_info.size();
        out_file.write((char*)&in_file_size,8);  //将原文件大小存入压缩文件头部，以二进制方式写入

        out_file.write((char*)&cmpfile_comment_size,1);  //存入注释长度，000 0001代表2个字节

        if(cmpfile_comment_size!=0)
        {
            for(int i=0; i<cmpfile_comment_size; ++i)
            {
                out_file.write((char*)&cmpfile_comment.at(i),1);
            }
        }

        out_file.close();  //关闭压缩文件
    }
    else
    {
        initUi();
        QMessageBox::information(this,"提示","文件打开错误，请重试",QMessageBox::Ok,QMessageBox::NoButton);
        return;
    }
}

void MainWindow::writeResources()
{
    /*检查当前打开的文件是否已添加到了最近文件里*/
    int flag=0;
    QString in_file_path=m_in_file_info.filePath();
    for(auto &arf : m_act_recent_files)
    {
        if(in_file_path==arf->text())
        {
            flag=1;
            break;
        }
    }
    if(flag==0)  //如果未添加，则将当前文件路径写入recentfiles.txt保存
    {
        QFile file("recentfiles.txt");  //打开应用资源，recentfiles.txt里按行存放的最近文件路径
        if(file.open(QIODevice::Append | QIODevice::Text))
        {
            QTextStream txt_stream(&file);
            txt_stream<<m_in_file_info.filePath()<<"\r\n";  //文件路径+换行符
            file.close();
        }
    }
}

void MainWindow::readCmpfileHeader()
{
    std::ifstream in_file(m_in_file_info.filePath().toLocal8Bit().constData(),std::ios::binary);  //打开压缩文件
    if(in_file.is_open())
    {
        char cmpfile_original_type_ch[4];  //存放压缩文件的原文件类型,字符数组

        in_file.read(cmpfile_original_type_ch,4);  //获取原文件类型
        in_file.read((char*)&m_cmpfile_original_size,8);  //获取原文件大小
        in_file.read((char*)&m_cmpfile_comment_length,1);  //获取压缩文件注释长度
        m_cmpfile_headsize=13+m_cmpfile_comment_length;  //计算压缩文件头部大小

        if(m_cmpfile_comment_length!=0)  //压缩文件注释长度不为0，则获取压缩文件注释
        {
            char cmpfile_comment_ch;
            std::string cmpfile_comment_std_str;
            for(int i=0; i<m_cmpfile_comment_length; ++i)
            {
                in_file.read((char*)&cmpfile_comment_ch,1);
                cmpfile_comment_std_str.push_back(cmpfile_comment_ch);
            }
            m_cmpfile_comment=QString::fromLocal8Bit(QByteArray::fromRawData(cmpfile_comment_std_str.c_str(),cmpfile_comment_std_str.length()));
        }

        for(int i=0; cmpfile_original_type_ch[i]!=' ' && i<4; ++i)  //将去除空格的字符数组转换为字符串
        {
            m_cmpfile_original_type+=cmpfile_original_type_ch[i];
        }

        in_file.close();  //关闭压缩文件
    }
}
