#include "AimRobotPunctureWidget.h"
#include <QStyleOption>
#include <QPainter>

AimRobotPunctureWidget::AimRobotPunctureWidget(QWidget *parent)
	: QWidget(parent)
{
	InitMenbers();           //初始化成员
	CreateCtrls();           //创建控制
}

AimRobotPunctureWidget::~AimRobotPunctureWidget()
{

}

void AimRobotPunctureWidget::UpdatePathInfoLabelText(int pathidx, float pathdis)         //更新路径信息标签文字
{
	this->mCurPathIdx = pathidx;       //获取路径索引
	QString pathtext = QString("路径 ") + QString::number(pathidx);                     //路径文字=“路径”+路径索引
	QString path_detail = pathtext + QString("：") + QString("%1").arg(pathdis);        //路径细节=路径文字：路径距离
	if (!mTextLabelLt.isEmpty())           //判断文字label列表是否为空，不为空则执行下面代码
	{
		if (mTextLabelLt.at(RPW_PathTitleDetail_Lbl))
		{
			mTextLabelLt.at(RPW_PathTitleDetail_Lbl)->setText(pathtext);      //设置路径文字
		}
		if (mTextLabelLt.at(RPW_PathMsg_Lbl))
		{
			mTextLabelLt.at(RPW_PathMsg_Lbl)->setText(path_detail);           //设置路径细节
		}
	}
}

void AimRobotPunctureWidget::SetPathListPt(QList<T_RobotPuncturePathCtrl*>* pathlist)          //设置路径列表   
{
	this->m_pPathList = pathlist;         //获取路径列表
	if (m_pPathList)                      
	{
		InitTableWdtShow();           //如果路径列表不为空，则初始化表格部件并显示
	}
}

void AimRobotPunctureWidget::resizeEvent(QResizeEvent* event) 
{
	LayoutCtrls();            //布局控制
	minit = false;            //未初始化
	QWidget::resizeEvent(event);    //
}

void AimRobotPunctureWidget::paintEvent(QPaintEvent* event)
{
	RPW_ImgLabel_Type type[] = { RPW_SetTargetPt_Lbl,RPW_SetIntoPt_Lbl };        //图像label类型：设置目标点，设置靶点
	RPW_StrImgOrder img[] = { RPW_Target_Image,RPW_Into_Image };                 //图像次序：目标点图像，靶点图像
	if (!minit)
	{
		minit = true;   //如果没有初始化，则进行初始化并将初始化判据设置为true
		
		int size = mImgLabelLt.size();        //获取图像label列表的大小
		for (int i = 0; i < size; i++)
		{
			this->SetLabelImage(mImgLabelLt.at(i), img[i]);     //遍历图像label列表，设置图像label的图像
		}
	}

	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);    //设置背景颜色
	QWidget::paintEvent(event);
}

void AimRobotPunctureWidget::LayoutWdts(QWidget * pwdt, double * pRectRate)          //摆放部件，需传递部件及位置信息
{
	if (pwdt == 0)return;      //无部件则返回
	int width = this->width();     //获取宽度
	int height = this->height();   //获取高度
	QRect rc(pRectRate[0] * width, pRectRate[1] * height,
		pRectRate[2] * width, pRectRate[3] * height);
	pwdt->setGeometry(rc);            //摆放窗口部件
}

void AimRobotPunctureWidget::LayoutCtrls()         //布局控制
{
	if (!mTextLabelLt.isEmpty())            //判断文字label列表是否为空
	{
		int size = mTextLabelLt.size();      //获取文字label列表的数量
		for (int i = 0; i < size; i++)
		{
			LayoutWdts(mTextLabelLt.at(i), &RPW_TextLbl_Rect[i][0]);         //摆放文字label
		}
	}

	if (!mImgLabelLt.isEmpty())
	{
		int size = mImgLabelLt.size();
		for (int i = 0; i < size; i++)
		{
			LayoutWdts(mImgLabelLt.at(i), &RPW_ImgLbl_Rect[i][0]);
		}
	}

	if (!mTextBtnLt.isEmpty())
	{
		int size = mTextBtnLt.size();
		for (int i = 0; i < size; i++)
		{
			LayoutWdts(mTextBtnLt.at(i), &RPW_TextBtn_Rect[i][0]);
		}
	}

	if (!mImgBtnLt.isEmpty())
	{
		int size = mImgBtnLt.size();
		for (int i = 0; i < size; i++)
		{
			LayoutWdts(mImgBtnLt.at(i), &RPW_ImgBtn_Rect[i][0]);
		}
	}

	if (mpTable)
	{
		int width = this->width();
		int height = this->height();
		mpTable->setGeometry(RPW_Table_X*width, RPW_Table_Y*height,
			RPW_Table_Width, RPW_Table_Height);
	}

}

void AimRobotPunctureWidget::InitMenbers()        //初始化成员
{
	minit = false;                     //初始化判据设置为false
	mpTable = NULL;                    //表格指针置为空指针
	mTableCtrl.TableRowCount = 0;      //表格行数置零
	mCurPathIdx = 0;                   //当前路径索引置零
	m_pPathList = NULL;                //路径列表指针置零
}

void AimRobotPunctureWidget::CreateCtrls()    //创建控制
{
	CreateTextLabel();      /
	CreateImgLabel();
	CreateTextBtns();
	CreateImgBtns();
	CreateTableWdt();
}

void AimRobotPunctureWidget::CreateTextLabel()
{
	RPW_TextLabel_Type type[] = { RPW_PathTitle_Lbl ,RPW_PathTitleDetail_Lbl ,RPW_PathMsg_Lbl };          //文字label类型：路径标题label，路径标题细节label，路径信息label
	RPW_TextType text[] = { RPW_Title_Text ,RPW_TitleDetail_Text,RPW_TitleDetail_Text };                  //文本类型：标题，标题细节，？？
	RPW_StyleSheet_Type stylesheet[] = { RPW_Common_Style ,RPW_FrameLbl_Style ,RPW_FrameLbl_Style };      //样式类型：普通类型，
	QColor textcolor(RPW_TextColor[RPW_Text_Color][Aim_RED],      //文字颜色
		RPW_TextColor[RPW_Text_Color][Aim_GREEN],
		RPW_TextColor[RPW_Text_Color][Aim_BLUE]);

	int fontorder = 0;
#if LANGUAGE
	fontorder = 1;
#endif

	QFont font(FontTypeStr[fontorder], RPW_Text_fsize, QFont::Normal);    //设置字体

	int size = sizeof(type) / sizeof(RPW_TextLabel_Type);      //textlabel类型总数
	for (int i = 0; i < size; i++)                             //遍历
	{
		QLabel* plabel = new QLabel(this);                 //创建新label
		plabel->setObjectName("plabel");                   //设置对象名：plabel
		QPalette pe;                                       //创建调色板对象
		pe.setColor(QPalette::WindowText, textcolor);      //设置调色板颜色
		plabel->setFont(font);                             //设置所用字体
		plabel->setPalette(pe);                            //设置所用调色板
		plabel->setStyleSheet(QString("#plabel") + RPW_StyleSheet_TextStr[stylesheet[i]]);        //设置样式
		plabel->setText(QString(RPW_TextStr[text[i] + fontorder]));        //设置文本
		mTextLabelLt.append(plabel);              //将新label添加到文字label列表
	}
}

void AimRobotPunctureWidget::CreateImgLabel()              
{
	RPW_ImgLabel_Type type[] = { RPW_SetTargetPt_Lbl,RPW_SetIntoPt_Lbl };         //图像label类型：靶点label，入针点label

	int size = sizeof(type) / sizeof(RPW_ImgLabel_Type);        //遍历
	for (int i = 0; i < size; i++)
	{
		QLabel* imglabel = new QLabel(this);          //创建新的imglabel
		mImgLabelLt.append(imglabel);                 //将新的imglabel添加到图像label列表
	}

}

void AimRobotPunctureWidget::CreateTextBtns()
{
	int fontorder = 0;
#if LANGUAGE
	fontorder = 1;
#endif

	RPW_TextBtn_Type type[] =                  //文本按钮类型
	{
		RPW_SetTargetPt_Btn,               //设置靶点按钮
		RPW_SetIntoPt_Btn,                 //设置入针点按钮
		RPW_Clear_Btn,                     //清空按钮
		RPW_Done_Btn                       //完成按钮
	};

	RPW_TextType text[] =                      //文本类型
	{
		RPW_SetTar_BtnText,                //设置靶点按钮的文本
		RPW_SetInto_BtnText,               //设置入针点按钮的文本
		RPW_Clear_BtnText,                 //清空按钮的文本
		RPW_Done_BtnText                   //完成按钮的文本
	};
	QColor color(RPW_TextColor[RPW_Text_Color][Aim_RED],     //创建新颜色
		RPW_TextColor[RPW_Text_Color][Aim_GREEN],
		RPW_TextColor[RPW_Text_Color][Aim_BLUE]);
	QFont font(FontTypeStr[fontorder], RPW_Text_fsize, QFont::Normal);       //创建新字体
	int size = sizeof(type) / sizeof(RPW_TextBtn_Type);        //遍历
	for (int i = 0; i < size; i++)
	{
		AimPushButton *btn = new AimPushButton(type[i], this);        //创建新按钮
		btn->setObjectName("textbtn");                                //设置对象名：textbtn
		btn->setStyleSheet(QString("#textbtn") + RPW_StyleSheet_TextStr[RPW_FrameLbl_Style]);     //设置样式
		btn->SetTextAlignFlag(Qt::AlignCenter);             //设置对齐方式
		btn->SetTextConfig(&QString(RPW_TextStr[text[i] + fontorder]), &font, &font, &color, &color);      //
		connect(btn, &AimPushButton::AimBtnClick, this, &AimRobotPunctureWidget::AimPushBtnClick);      //连接槽函数
		mTextBtnLt.append(btn);      //将按钮添加到列表
	}
}

void AimRobotPunctureWidget::CreateImgBtns()
{
	RPW_ImgBtn_Type type[] = { RPW_AddPath_Btn,RPW_DeletePath_Btn };         //imglabel类型：添加路径按钮，删除路径按钮
	RPW_StrImgOrder img[] = { RPW_AddPath_Image,RPW_Delete_Image };          //图像次序：添加路径图像，删除路径图像
	int size = sizeof(type) / sizeof(RPW_ImgBtn_Type);          //遍历
	for (int i = 0; i < size; i++)
	{ 
		AimPushButton* pbtn = new AimPushButton(type[i], this);          //创建新按钮
		QPixmap normalpix(RPW_ImgPathStr[img[i]]);                       //..
		pbtn->SetIconPixmap(normalpix, ICON_Normal);
		pbtn->ShowIconPixmap(ICON_Normal);
		connect(pbtn, &AimPushButton::AimBtnClick, this, &AimRobotPunctureWidget::AimPushBtnClick);     //连接槽函数
		mImgBtnLt.append(pbtn);           //将按钮添加到列表
	}
}

void AimRobotPunctureWidget::CreateTableWdt()
{
	int fontorder = 0;
#if LANGUAGE
	fontorder = 1;
#endif
	if (mpTable == NULL)
	{
		QTableWidget* ptable = new QTableWidget(this);       //创建新的QTableWidget对象：ptable
		ptable->setColumnCount(RPW_Table_Column);            //设置table列数
		ptable->setSelectionBehavior(QAbstractItemView::SelectRows);    //设置选择行为
		ptable->setEditTriggers(QAbstractItemView::NoEditTriggers);     //设置编辑触发
		QHeaderView *verticalhead = ptable->verticalHeader();           //表头管理
		verticalhead->setHidden(true);                                  //隐藏垂直方向表头
		QHeaderView *horizontalhead = ptable->horizontalHeader();       
		horizontalhead->setHidden(true);                                //隐藏水平方向表头

		ptable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);     //关闭水平方向scrollbar
		ptable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);       //关闭竖直方向scrollbar
		ptable->setFocusPolicy(Qt::NoFocus);                              //不能通过tab建或被单击获得焦点
		ptable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel); //设置鼠标滚轮行为：每像素一步
		ptable->setSelectionMode(QAbstractItemView::SingleSelection);     //设置选择模式：单选
		ptable->setRowCount(mTableCtrl.TableRowCount);       //设置列数
		ptable->setObjectName("table");                      //设置对象名：table
		ptable->setStyleSheet(QString("#table") + QString(RPW_StyleSheet_TextStr[RPW_Table_Style]));   //设置样式

		QFont font(FontTypeStr[fontorder], RPW_TableText_fsize, QFont::Normal);   //创建新字体
		ptable->setFont(font);                            //设置所用字体
		ptable->viewport()->installEventFilter(this);     //？？
		this->installEventFilter(this);
		mpTable = ptable;
		mpTable->show();
	}

}

void AimRobotPunctureWidget::SetLabelImage(QLabel * plabel, RPW_StrImgOrder order)    //设置imglabel的图像
{
	if (plabel)
	{

		QPixmap pixmap(RPW_ImgPathStr[order]);
		if (!pixmap.isNull())
		{
			QPixmap fitpixmap = pixmap.scaled(plabel->width(), plabel->height(),
				Qt::KeepAspectRatio, Qt::SmoothTransformation);
			plabel->setPixmap(fitpixmap);
		}

	}
}

void AimRobotPunctureWidget::InitTableWdtShow()
{
	if (mpTable)
	{
		mTableCtrl.TableRowCount = m_pPathList->size();          //根据路径列表中路径个数决定表格行数
		mpTable->setRowCount(mTableCtrl.TableRowCount);          
		for (int i = 0; i < mTableCtrl.TableRowCount; i++)       //遍历
		{
			mpTable->setRowHeight(i, 30);                                    //设置行高
			QTableWidgetItem *pitm = new QTableWidgetItem();                 //创建新的表格项
			pitm->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);        //文本对齐方式：左对齐，垂直方向上居中
			mpTable->setItem(i, 0, pitm);                                    //将新项添加到表格
			mpTable->setColumnWidth(0, RPW_Table_Width);                     //设置列宽
			mpTable->item(i, 0)->setTextColor(QColor(255, 255, 255));        //设置表格项的文字颜色

			float dis = m_pPathList->at(i)->pathDis;                         //路径距离
			QString pathtext = QString("路径  ") + QString::number(i + 1);   //路径标号文字
			QString path_detail = pathtext + QString("：") + QString("%1").arg(dis);    //路径详情
			mTableInfoList.append(path_detail);                              //将路径详情添加到表格信息列表
			mpTable->item(i, 0)->setText(path_detail);                       //设置表格项文本为路径详情
		}
	}
	
}

bool AimRobotPunctureWidget::eventFilter(QObject * object, QEvent * event)
{
	return false;
}

void AimRobotPunctureWidget::FastScroll(int mdis)
{

}

void AimRobotPunctureWidget::ScrollUp()
{

}

void AimRobotPunctureWidget::ScrollDown()
{

}

void AimRobotPunctureWidget::TableMouseButtonPressEvent(QEvent * event)
{

}

void AimRobotPunctureWidget::TableMouseMoveEvent(QEvent * event)
{

}

void AimRobotPunctureWidget::TableMouseReleaseEvent(QEvent * event)
{

}

void AimRobotPunctureWidget::ProScrollUpAndDown()
{

}

void AimRobotPunctureWidget::ProPathAddBtnClickedEvent()           //“路径添加”按钮点击事件
{
	if (!mTextLabelLt.isEmpty())                               //如果文字label列表不为空
	{
		QString pathinfo;                                  //定义路径信息变量：pathinfo

		if (mTextLabelLt.at(RPW_PathTitleDetail_Lbl))      //如果路径已存在，则将旧路径清除
		{
			mTextLabelLt.at(RPW_PathTitleDetail_Lbl)->clear();
		}
		if (mTextLabelLt.at(RPW_PathMsg_Lbl))              //如果路径信息label存在
		{
			if (mTextLabelLt.at(RPW_PathMsg_Lbl)->text().isEmpty())return;         //如果路径信息的文本为空则返回
			pathinfo = mTextLabelLt.at(RPW_PathMsg_Lbl)->text();             //否则将路径信息文本读入pathinfo
			mTextLabelLt.at(RPW_PathMsg_Lbl)->clear();                       //清除路径信息label
		}
		if (mpTable)                                      //如果有表格
		{
			this->AddOnePathToTableWdt(&pathinfo);    //添加一条路径到表格部件
			mTableInfoList.append(pathinfo);          //将路径信息添加到表格信息列表
		}
	}
}

void AimRobotPunctureWidget::ProPathClearBtnClickedEvent()       //“路径清除”按钮点击事件
{
	if (mpTable)                             //如果表格存在
	{
		mpTable->setRowCount(0);         //将表格行数置零
		emit SendPathClearMsg();         //发送路径清除信息
	}
}

void AimRobotPunctureWidget::ProPathDeleteBtnClickedEvent()       //“路径删除”按钮点击事件
{
	if (mpTable)                                              //如果表格存在
	{
		int curSelectedRow = mpTable->currentRow();       //读取当前所选行
		if (curSelectedRow != -1)                         //如果当前所选行不为-1，即表格存在项
		{
			mTableInfoList.removeAt(curSelectedRow);  //将所选行从表格信息列表中移除
			mTableCtrl.TableRowCount--;               //表格列数减1
			mpTable->setRowCount(mTableCtrl.TableRowCount);    

			for (int i = 0; i < mTableCtrl.TableRowCount; i++)               //重写表格并更新
			{
				mpTable->setRowHeight(i, 30);
				QTableWidgetItem *pitm = new QTableWidgetItem();
				pitm->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
				mpTable->setItem(i, 0, pitm);
				mpTable->setColumnWidth(0, RPW_Table_Width);
				mpTable->item(i, 0)->setTextColor(QColor(255, 255, 255));
				mpTable->item(i, 0)->setText(mTableInfoList.at(i));
			}
			mpTable->update();

			emit SendPathDeleteMsg(curSelectedRow);                    //发送路径删除信息
		}
	}
	
}

void AimRobotPunctureWidget::AddOnePathToTableWdt(QString * pathinfo)             //添加一条路径到表格
{
	mTableCtrl.TableRowCount = mpTable->rowCount();          //读取表格当前行数
	mTableCtrl.TableRowCount++;                              //行数加1
	mpTable->setRowCount(mTableCtrl.TableRowCount);          //设置表格行数
	
	int curRowIdx = mTableCtrl.TableRowCount - 1;            //当前行索引=表格行数-1
	mpTable->setRowHeight(curRowIdx, 30);                    //设置行高
	QTableWidgetItem *pitm = new QTableWidgetItem();         //创建新表格项
	pitm->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);//设置文本对齐方式
	mpTable->setItem(curRowIdx, 0, pitm);                    //添加新项
	mpTable->setColumnWidth(0, RPW_Table_Width);             //设置列宽
	
	mpTable->item(curRowIdx, 0)->setTextColor(QColor(255, 255, 255));     //设置新项文本颜色
	mpTable->item(curRowIdx, 0)->setText(*pathinfo);                      //设置文本

	mpTable->update();           //更新表格

}

void AimRobotPunctureWidget::AimPushBtnClick(int index, E_BTN_SATUS_TYPE Type)      //目标按钮点击：索引，类型
{
	switch (index)
	{
	case RPW_SetTargetPt_Btn:                                       //如果是设置靶点按钮
	{
		emit SendPathAddEventType(RPW_SetTargetPt);             //发送路径添加事件类型（设置靶点）
	}break;
	case RPW_SetIntoPt_Btn:                                         //如果是设置入针点按钮
	{
		emit SendPathAddEventType(RPW_SetIntoPt);               //发送路径添加事件类型（设置入针点）
	}break;
	case RPW_Clear_Btn:                                             //如果是清除按钮
	{
		ProPathClearBtnClickedEvent();                          //发送路径清除信息
	}break;
	case RPW_Done_Btn:                                              //如果是完成按钮
	{
		emit SendDoneMsg();                                     //发送完成信息
	}break;
	case RPW_AddPath_Btn:                                           //如果是添加路径按钮
	{
		ProPathAddBtnClickedEvent();                            //执行路径添加点击事件
	}break;
	case RPW_DeletePath_Btn:                                        //如果是删除路径按钮
	{
		ProPathDeleteBtnClickedEvent();                         //执行路径删除点击事件
	}break;
	default:
		break;
	}
}
