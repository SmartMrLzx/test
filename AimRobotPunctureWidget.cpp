#include "AimRobotPunctureWidget.h"
#include <QStyleOption>
#include <QPainter>

AimRobotPunctureWidget::AimRobotPunctureWidget(QWidget *parent)
	: QWidget(parent)
{
	InitMenbers();
	CreateCtrls();
}

AimRobotPunctureWidget::~AimRobotPunctureWidget()
{

}

void AimRobotPunctureWidget::UpdatePathInfoLabelText(int pathidx, float pathdis)
{
	this->mCurPathIdx = pathidx;
	QString pathtext = QString("Â·¾¶  ") + QString::number(pathidx);
	QString path_detail = pathtext + QString("£º") + QString("%1").arg(pathdis);
	if (!mTextLabelLt.isEmpty())
	{
		if (mTextLabelLt.at(RPW_PathTitleDetail_Lbl))
		{
			mTextLabelLt.at(RPW_PathTitleDetail_Lbl)->setText(pathtext);
		}
		if (mTextLabelLt.at(RPW_PathMsg_Lbl))
		{
			mTextLabelLt.at(RPW_PathMsg_Lbl)->setText(path_detail);
		}
	}
}

void AimRobotPunctureWidget::SetPathListPt(QList<T_RobotPuncturePathCtrl*>* pathlist)
{
	this->m_pPathList = pathlist;
	if (m_pPathList)
	{
		InitTableWdtShow();
	}
}

void AimRobotPunctureWidget::resizeEvent(QResizeEvent* event)
{
	LayoutCtrls();
	minit = false;
	QWidget::resizeEvent(event);
}

void AimRobotPunctureWidget::paintEvent(QPaintEvent* event)
{
	RPW_ImgLabel_Type type[] = { RPW_SetTargetPt_Lbl,RPW_SetIntoPt_Lbl };
	RPW_StrImgOrder img[] = { RPW_Target_Image,RPW_Into_Image };
	if (!minit)
	{
		minit = true;
		
		int size = mImgLabelLt.size();
		for (int i = 0; i < size; i++)
		{
			this->SetLabelImage(mImgLabelLt.at(i), img[i]);
		}
	}

	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
	QWidget::paintEvent(event);
}

void AimRobotPunctureWidget::LayoutWdts(QWidget * pwdt, double * pRectRate)
{
	if (pwdt == 0)return;
	int width = this->width();
	int height = this->height();
	QRect rc(pRectRate[0] * width, pRectRate[1] * height,
		pRectRate[2] * width, pRectRate[3] * height);
	pwdt->setGeometry(rc);
}

void AimRobotPunctureWidget::LayoutCtrls()
{
	if (!mTextLabelLt.isEmpty())
	{
		int size = mTextLabelLt.size();
		for (int i = 0; i < size; i++)
		{
			LayoutWdts(mTextLabelLt.at(i), &RPW_TextLbl_Rect[i][0]);
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

void AimRobotPunctureWidget::InitMenbers()
{
	minit = false;
	mpTable = NULL;
	mTableCtrl.TableRowCount = 0;
	mCurPathIdx = 0;
	m_pPathList = NULL;
}

void AimRobotPunctureWidget::CreateCtrls()
{
	CreateTextLabel();
	CreateImgLabel();
	CreateTextBtns();
	CreateImgBtns();
	CreateTableWdt();
}

void AimRobotPunctureWidget::CreateTextLabel()
{
	RPW_TextLabel_Type type[] = { RPW_PathTitle_Lbl ,RPW_PathTitleDetail_Lbl ,RPW_PathMsg_Lbl };
	RPW_TextType text[] = { RPW_Title_Text ,RPW_TitleDetail_Text,RPW_TitleDetail_Text };
	RPW_StyleSheet_Type stylesheet[] = { RPW_Common_Style ,RPW_FrameLbl_Style ,RPW_FrameLbl_Style };
	QColor textcolor(RPW_TextColor[RPW_Text_Color][Aim_RED],
		RPW_TextColor[RPW_Text_Color][Aim_GREEN],
		RPW_TextColor[RPW_Text_Color][Aim_BLUE]);

	int fontorder = 0;
#if LANGUAGE
	fontorder = 1;
#endif

	QFont font(FontTypeStr[fontorder], RPW_Text_fsize, QFont::Normal);

	int size = sizeof(type) / sizeof(RPW_TextLabel_Type);
	for (int i = 0; i < size; i++)
	{
		QLabel* plabel = new QLabel(this);
		plabel->setObjectName("plabel");
		QPalette pe;
		pe.setColor(QPalette::WindowText, textcolor);
		plabel->setFont(font);
		plabel->setPalette(pe);
		plabel->setStyleSheet(QString("#plabel") + RPW_StyleSheet_TextStr[stylesheet[i]]);
		plabel->setText(QString(RPW_TextStr[text[i] + fontorder]));
		mTextLabelLt.append(plabel);
	}
}

void AimRobotPunctureWidget::CreateImgLabel()
{
	RPW_ImgLabel_Type type[] = { RPW_SetTargetPt_Lbl,RPW_SetIntoPt_Lbl };

	int size = sizeof(type) / sizeof(RPW_ImgLabel_Type);
	for (int i = 0; i < size; i++)
	{
		QLabel* imglabel = new QLabel(this);
		mImgLabelLt.append(imglabel);
	}

}

void AimRobotPunctureWidget::CreateTextBtns()
{
	int fontorder = 0;
#if LANGUAGE
	fontorder = 1;
#endif

	RPW_TextBtn_Type type[] = 
	{
		RPW_SetTargetPt_Btn,
		RPW_SetIntoPt_Btn,
		RPW_Clear_Btn,
		RPW_Done_Btn
	};

	RPW_TextType text[] =
	{
		RPW_SetTar_BtnText,
		RPW_SetInto_BtnText,
		RPW_Clear_BtnText,
		RPW_Done_BtnText 
	};
	QColor color(RPW_TextColor[RPW_Text_Color][Aim_RED],
		RPW_TextColor[RPW_Text_Color][Aim_GREEN],
		RPW_TextColor[RPW_Text_Color][Aim_BLUE]);
	QFont font(FontTypeStr[fontorder], RPW_Text_fsize, QFont::Normal);
	int size = sizeof(type) / sizeof(RPW_TextBtn_Type);
	for (int i = 0; i < size; i++)
	{
		AimPushButton *btn = new AimPushButton(type[i], this);
		btn->setObjectName("textbtn");
		btn->setStyleSheet(QString("#textbtn") + RPW_StyleSheet_TextStr[RPW_FrameLbl_Style]);
		btn->SetTextAlignFlag(Qt::AlignCenter);
		btn->SetTextConfig(&QString(RPW_TextStr[text[i] + fontorder]), &font, &font, &color, &color);
		connect(btn, &AimPushButton::AimBtnClick, this, &AimRobotPunctureWidget::AimPushBtnClick);
		mTextBtnLt.append(btn);
	}
}

void AimRobotPunctureWidget::CreateImgBtns()
{
	RPW_ImgBtn_Type type[] = { RPW_AddPath_Btn,RPW_DeletePath_Btn };
	RPW_StrImgOrder img[] = { RPW_AddPath_Image,RPW_Delete_Image };
	int size = sizeof(type) / sizeof(RPW_ImgBtn_Type);
	for (int i = 0; i < size; i++)
	{
		AimPushButton* pbtn = new AimPushButton(type[i], this);
		QPixmap normalpix(RPW_ImgPathStr[img[i]]);
		pbtn->SetIconPixmap(normalpix, ICON_Normal);
		pbtn->ShowIconPixmap(ICON_Normal);
		connect(pbtn, &AimPushButton::AimBtnClick, this, &AimRobotPunctureWidget::AimPushBtnClick);
		mImgBtnLt.append(pbtn);
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
		QTableWidget* ptable = new QTableWidget(this);
		ptable->setColumnCount(RPW_Table_Column);
		ptable->setSelectionBehavior(QAbstractItemView::SelectRows);
		ptable->setEditTriggers(QAbstractItemView::NoEditTriggers);
		QHeaderView *verticalhead = ptable->verticalHeader();
		verticalhead->setHidden(true);
		QHeaderView *horizontalhead = ptable->horizontalHeader();
		horizontalhead->setHidden(true);

		ptable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		ptable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		ptable->setFocusPolicy(Qt::NoFocus);
		ptable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
		ptable->setSelectionMode(QAbstractItemView::SingleSelection);
		ptable->setRowCount(mTableCtrl.TableRowCount);
		ptable->setObjectName("table");
		ptable->setStyleSheet(QString("#table") + QString(RPW_StyleSheet_TextStr[RPW_Table_Style]));

		QFont font(FontTypeStr[fontorder], RPW_TableText_fsize, QFont::Normal);
		ptable->setFont(font);
		ptable->viewport()->installEventFilter(this);
		this->installEventFilter(this);
		mpTable = ptable;
		mpTable->show();
	}

}

void AimRobotPunctureWidget::SetLabelImage(QLabel * plabel, RPW_StrImgOrder order)
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
		mTableCtrl.TableRowCount = m_pPathList->size();
		mpTable->setRowCount(mTableCtrl.TableRowCount);
		for (int i = 0; i < mTableCtrl.TableRowCount; i++)
		{
			mpTable->setRowHeight(i, 30);
			QTableWidgetItem *pitm = new QTableWidgetItem();
			pitm->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
			mpTable->setItem(i, 0, pitm);
			mpTable->setColumnWidth(0, RPW_Table_Width);
			mpTable->item(i, 0)->setTextColor(QColor(255, 255, 255));

			float dis = m_pPathList->at(i)->pathDis;
			QString pathtext = QString("Â·¾¶  ") + QString::number(i + 1);
			QString path_detail = pathtext + QString("£º") + QString("%1").arg(dis);
			mTableInfoList.append(path_detail);
			mpTable->item(i, 0)->setText(path_detail);
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

void AimRobotPunctureWidget::ProPathAddBtnClickedEvent()
{
	if (!mTextLabelLt.isEmpty())
	{
		QString pathinfo;

		if (mTextLabelLt.at(RPW_PathTitleDetail_Lbl))
		{
			mTextLabelLt.at(RPW_PathTitleDetail_Lbl)->clear();
		}
		if (mTextLabelLt.at(RPW_PathMsg_Lbl))
		{
			if (mTextLabelLt.at(RPW_PathMsg_Lbl)->text().isEmpty())return;
			pathinfo = mTextLabelLt.at(RPW_PathMsg_Lbl)->text();
			mTextLabelLt.at(RPW_PathMsg_Lbl)->clear();
		}
		if (mpTable)
		{
			this->AddOnePathToTableWdt(&pathinfo);
			mTableInfoList.append(pathinfo);
		}
	}
}

void AimRobotPunctureWidget::ProPathClearBtnClickedEvent()
{
	if (mpTable)
	{
		mpTable->setRowCount(0);
		emit SendPathClearMsg();
	}
}

void AimRobotPunctureWidget::ProPathDeleteBtnClickedEvent()
{
	if (mpTable)
	{
		int curSelectedRow = mpTable->currentRow();
		if (curSelectedRow != -1)
		{
			mTableInfoList.removeAt(curSelectedRow);
			mTableCtrl.TableRowCount--;
			mpTable->setRowCount(mTableCtrl.TableRowCount);

			for (int i = 0; i < mTableCtrl.TableRowCount; i++)
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

			emit SendPathDeleteMsg(curSelectedRow);
		}
	}
	
}

void AimRobotPunctureWidget::AddOnePathToTableWdt(QString * pathinfo)
{
	mTableCtrl.TableRowCount = mpTable->rowCount();
	mTableCtrl.TableRowCount++;
	mpTable->setRowCount(mTableCtrl.TableRowCount);
	
	int curRowIdx = mTableCtrl.TableRowCount - 1;
	mpTable->setRowHeight(curRowIdx, 30);
	QTableWidgetItem *pitm = new QTableWidgetItem();
	pitm->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	mpTable->setItem(curRowIdx, 0, pitm);
	mpTable->setColumnWidth(0, RPW_Table_Width);
	
	mpTable->item(curRowIdx, 0)->setTextColor(QColor(255, 255, 255));
	mpTable->item(curRowIdx, 0)->setText(*pathinfo);

	mpTable->update();

}

void AimRobotPunctureWidget::AimPushBtnClick(int index, E_BTN_SATUS_TYPE Type)
{
	switch (index)
	{
	case RPW_SetTargetPt_Btn:
	{
		emit SendPathAddEventType(RPW_SetTargetPt);
	}break;
	case RPW_SetIntoPt_Btn:
	{
		emit SendPathAddEventType(RPW_SetIntoPt);
	}break;
	case RPW_Clear_Btn:
	{
		ProPathClearBtnClickedEvent();
	}break;
	case RPW_Done_Btn:
	{
		emit SendDoneMsg();
	}break;
	case RPW_AddPath_Btn:
	{
		ProPathAddBtnClickedEvent();
	}break;
	case RPW_DeletePath_Btn:
	{
		ProPathDeleteBtnClickedEvent();
	}break;
	default:
		break;
	}
}