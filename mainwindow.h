#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAction>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>
#include <QMimeData>
#include "ui_mainwindow.h"
#include "huffman.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/*                                         压缩文件存放格式
 * 压缩文件头部：  <原文件类型：4字节> <原文件大小：8字节> <注释长度：1字节（2个字节的注释，其长度算为1个长度）> <注释：字节数不定（最长256*2）>
 * 压缩文件字典：  <表长：2字节> {<字符1：1字节> <字符编码长：1字节> <编码：位数不定（最多256位）>}*n
 * 压缩文件数据：  <压缩文件数据部分><补零个数：1字节>
 */

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);  //构造函数
    ~MainWindow();  //析构函数

private:
    void initUi();  //UI界面初始化函数
    void connectSignalSlots();  //关联函数：关联信号与槽函数

    void resetMainWindow();  //UI函数：重置主窗口
    void resetTable(bool);  //UI函数：重置表
    void resetButtons();  //UI函数：重置按钮
    void resetLabels();  //UI函数：重置标签
    void resetTextEdit();  //UI函数：重置文本编辑框
    void resetActions();  //UI函数：重置Action部件

    void refreshRecentFileActions();  //UI函数：刷新最近文件Action部件
    void refreshFileInfoTable();  //UI函数：刷新文件表
    void refreshMainWindow();  //UI函数：刷新主窗口（在打开文件文件后）

    void dragEnterEvent(QDragEnterEvent *event);  //拖拽事件函数：捕捉文件拖到主窗体的事件
    void dropEvent(QDropEvent *event);  //拖拽事件函数：处理并打开捕捉到的文件

    void resetMemberVariable();  //功能函数：重置对象所有成员变量

    void writeResources();  //功能函数：写入应用资源
    inline void readResources();  //功能函数：读取应用资源
    void writeCmpfileHeader();  //功能函数：写入压缩文件头部相关信息
    void readCmpfileHeader();  //功能函数：读取压缩文件头部相关信息

private slots:
    void on_act_open_file_triggered();  //槽函数：文件打开
    void on_btn_compress_clicked();  //槽函数：文件压缩
    void on_btn_decompress_clicked();  //槽函数：文件解压
    void on_act_exit_triggered();  //槽函数：关闭程序
    void on_act_close_file_triggered();  //槽函数：关闭当前文件
    void on_act_save_file_as_triggered();  //槽函数：当前文件另存为
    void on_act_add_cmpfile_comment_triggered();  //槽函数：添加压缩文件注释
    void on_text_file_comment_textChanged();  //槽函数：实时监测文本编辑框的输入字符，限制字符输入个数，限制非法输入
    void on_act_delete_cmpfile_comment_triggered();  //槽函数：删除压缩文件注释

    void actOpenRecentFile(); //自定义槽函数：打开最近文件



private:
    Ui::MainWindow *ui;  //UI部件：该指针指向UI设计器设计的部件
    QList<QAction*> m_act_recent_files;  //UI部件：最近文件的Action
    QAction *m_act_recent_files_empty;  //UI部件：最近文件为空时的Action

    Huffman *m_hf;  //Huffman

    QFileInfo m_in_file_info;  //输入文件信息
    QString m_out_file_path;  //输出文件路径
    QStringList m_recent_files_path;  //最近打开过的多个文件路径

    QString m_cmpfile_original_type;  //压缩文件的原文件类型
    qint64 m_cmpfile_original_size;  //压缩文件的原文件大小
    qint8 m_cmpfile_comment_length;  //压缩文件注释长度
    QString m_cmpfile_comment;  //压缩文件注释
    qint16 m_cmpfile_headsize;  //压缩文件头部大小，单位为字节
};
#endif // MAINWINDOW_H
