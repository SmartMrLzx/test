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

	void UpdatePathInfoLabelText(int pathidx, float pathdis);
	void SetPathListPt(QList<T_RobotPuncturePathCtrl*>*pathlist);

protected:
	void resizeEvent(QResizeEvent* event);
	void paintEvent(QPaintEvent* event);

protected:
	void LayoutWdts(QWidget * pwdt, double * pRectRate);
	void LayoutCtrls();

protected:
	void InitMenbers();
	void CreateCtrls();
	void CreateTextLabel();
	void CreateImgLabel();
	void CreateTextBtns();
	void CreateImgBtns();
	void CreateTableWdt();
	void SetLabelImage(QLabel *plabel, RPW_StrImgOrder order);
	void InitTableWdtShow();

protected:
	bool eventFilter(QObject *object, QEvent *event);
	void FastScroll(int mdis);
	void ScrollUp();
	void ScrollDown();
	void TableMouseButtonPressEvent(QEvent *event);
	void TableMouseMoveEvent(QEvent *event);
	void TableMouseReleaseEvent(QEvent *event);
	void ProScrollUpAndDown();

protected:
	void ProPathAddBtnClickedEvent();
	void ProPathClearBtnClickedEvent();
	void ProPathDeleteBtnClickedEvent();
	void AddOnePathToTableWdt(QString *pathinfo);

public slots:
	void AimPushBtnClick(int index, E_BTN_SATUS_TYPE Type);

signals:
	void SendPathAddEventType(RPW_Event_Type type);
	void SendPathClearMsg();
	void SendPathDeleteMsg(int deleteIdx);
	void SendDoneMsg();

private:
	QList<QLabel*> mTextLabelLt;
	QList<QLabel*> mImgLabelLt;
	QList<AimPushButton*> mTextBtnLt;
	QList<AimPushButton*> mImgBtnLt;
	QTableWidget* mpTable;
	TableWdtScrollCtrl mTableCtrl;
	bool minit;
	QList<QString> mTableInfoList;
	int mCurPathIdx;
	QList<T_RobotPuncturePathCtrl*>* m_pPathList;
};
