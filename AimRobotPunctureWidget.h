#pragma once

#include <QWidget>
#include <qobject.h>
#include <QLabel>
#include <QHeaderView>
#include <QEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QTime>
#include <qtablewidget.h>
#include "Robot\PathSelect\RobotPunctrueConfig.h"
#include "aimpushbutton.h"
#include "datatypedef.h"

class AimRobotPunctureWidget : public QWidget
{
	Q_OBJECT

public:
	AimRobotPunctureWidget(QWidget *parent);
	~AimRobotPunctureWidget();

	void UpdatePathInfoLabelText(int pathidx, float pathdis);                 //更新路径信息
	void SetPathListPt(QList<T_RobotPuncturePathCtrl*>*pathlist);             //设置路径列表

protected:
	void resizeEvent(QResizeEvent* event);            
	void paintEvent(QPaintEvent* event);

protected:
	void LayoutWdts(QWidget * pwdt, double * pRectRate);                //部件布局
	void LayoutCtrls();                                                 //布局控制

protected:
	void InitMenbers();          //初始化成员
	void CreateCtrls();          //创建控制
	void CreateTextLabel();      //创建文字label
	void CreateImgLabel();       //创建图像label
	void CreateTextBtns();       //创建文字button
	void CreateImgBtns();        //创建图像button
	void CreateTableWdt();       //创建表格部件
	void SetLabelImage(QLabel *plabel, RPW_StrImgOrder order);      //设置label图像
	void InitTableWdtShow();     //显示表格部件

protected:
	bool eventFilter(QObject *object, QEvent *event);        //事件过滤器
	void FastScroll(int mdis);                               //快速滚动
	void ScrollUp();                                         //向上
	void ScrollDown();                                       //向下
	void TableMouseButtonPressEvent(QEvent *event);          //表格中鼠标按下事件
	void TableMouseMoveEvent(QEvent *event);                 //表格中鼠标移动事件
	void TableMouseReleaseEvent(QEvent *event);              //表格中鼠标释放事件
	void ProScrollUpAndDown();                               //？？

protected:
	void ProPathAddBtnClickedEvent();                  //路径添加按钮的点击事件
	void ProPathClearBtnClickedEvent();                //路径清除按钮的点击事件
	void ProPathDeleteBtnClickedEvent();               //路径删除按钮的点击事件
	void AddOnePathToTableWdt(QString *pathinfo);      //添加一条路径到表格部件事件

public slots:
	void AimPushBtnClick(int index, E_BTN_SATUS_TYPE Type);      //定位按钮点击槽函数

signals:
	void SendPathAddEventType(RPW_Event_Type type);        //发送路径添加事件类型信号
	void SendPathClearMsg();                               //发送路径清除信息信号
	void SendPathDeleteMsg(int deleteIdx);                 //发送路径删除信息信号
	void SendDoneMsg();                                    //发送完成信息信号

private:
	QList<QLabel*> mTextLabelLt;                  //文字label列表
	QList<QLabel*> mImgLabelLt;                   //图像label列表
	QList<AimPushButton*> mTextBtnLt;             //文字按钮列表
	QList<AimPushButton*> mImgBtnLt;              //图像按钮列表
	QTableWidget* mpTable;                        //表格部件指针
	TableWdtScrollCtrl mTableCtrl;                //表格部件滚动控制
	bool minit;                                   //我的初始化判据
	QList<QString> mTableInfoList;                //表格信息列表
	int mCurPathIdx;                              //当前路径索引
	QList<T_RobotPuncturePathCtrl*>* m_pPathList; //路径列表
};
