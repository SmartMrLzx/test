
#include "glwidget.h"
#include <math.h>
#include <QGuiApplication>
#include <QDebug>
#include <QOpenGLFunctions>
#include <QImageReader.h>
#include "glut/glut.h"
#include <QTouchEvent>


#define GLWidgetDEBUG 0
#define NO_USE_AITK 0
#define INIT_PARAM_A 3
#define INIT_PARAM_B 2
#define PARAM_D1 4
#define PARAM_D2 6
#define PARAM_D3 10

#define TranThread 1
#define FindToolThread 5


bool GLWidget::m_sbTouchOperateFlag = true;
E_MarkAction GLWidget::m_sCurMarkAction = MARK_DEFAULT;
QColor GLWidget::m_sConColor= QColor(203,42,39);
E_ImageType GLWidget::m_CurROIImgType = ImgNone;
bool GLWidget::mbROIEnale = false;
E_ROI_Status_Mode GLWidget::mCurROIStatusMode = ROI_NONE_MODE;
GLWidget::GLWidget(QWidget *parent,quint8 index)
    : QOpenGLWidget(parent),m_wdtindex(index)
{

    //imagedata ctrl
    this->mpCurQImg    = NULL;
    this->mpCurImgData = NULL;
    memset(&(this->mImgpty),0,sizeof(T_DImgPropety));

    mDVolumpty   = NULL;
    mpData       = NULL;
    m_ImgXYRatio = 1.0;

    //opengl thread
    m_thread   = NULL;
    m_renderer = NULL;

    memset(this->m_arrViewRc,0,sizeof(GLfloat)*4);
    memset(this->m_arrWatchRc,0,sizeof(GLfloat)*4);
    memset(this->m_arrTextRc,0,sizeof(GLfloat)*4);

    mbUpdateImgDataFlag = false;
    mbPaintFlag = false;

    //image sroll pan control
    mHoriOffset = 0;
    mVertOffset = 0;
    mScaleFactor=1;
    mMsgHoriOffset = 0;
    mMsgVertOffset = 0;
    mMsgScale = 1;

    mPreScreenPt = QPointF(0,0);
    m_bMouseTranslate = false;
    mZoomDelta = 0.2;
    m_LastMousePos = QPoint(0,0);
    mbTouching = false;

    //textinfo
    // memset(m_arrTextInfo,0,sizeof(T_TextInfo)*TEXT_INFO_NUM);
    mScreenPt = QPoint(0,0);
    m_pConList = NULL;
    m_bValidAreaFlag = false;
    m_bIsBrushing = false;
    m_BrushRadius = 8;
    mCurBrushState = M_NONE;
    m_bIsBrushing = false;
	mCurROIOperaType = ROI_None;
	
	InitBrushPts(m_BrushRadius);
	
	mpItkSeg = NULL;
    mpImgSegment = NULL;
	mpCalculate = new CalculateClass(this);
	mMoveStyle = MOVE_NONE;
	mCurmoveToolIdx = -1;
	mCurmoveToolPointIdx = -1;

	//zlx
	mIsRenderLine = false;
	mIsMouseMove = false;
	mIsSelectXLine = false;
	mIsSelectYLine = false;
	mIsRenderTool = false;
	//zlx


	mpToolWdts = NULL;
	mCurBtnRectIdx = GL_TC_Rc_Small_Btn;
	mCurToolPressFunType = GL_TC_None;

	mWdtShowStatus = GL_CommonStates;
	mbPaint = false;
	
    InitDrawTextInfo();
	//CreateToolWdt();
    InitRenderThread();

#if NO_USE_AITK
	if (mpImgSegment == NULL)
	{
		mpImgSegment = new ImageSegment(this);
	}
#endif

    setAttribute(Qt::WA_AcceptTouchEvents);
	this->setAttribute(Qt::WA_DeleteOnClose);
}

GLWidget::~GLWidget()
{
    m_pConList= NULL;
    if(!mBrushCirclePtList.isEmpty())
    {
        mBrushCirclePtList.clear();
    }
    if(mpCurImgData!=NULL)
    {
        mpCurImgData=NULL;
    }
    if(mpCurQImg!=NULL)
    {
        delete mpCurQImg;
        mpCurQImg=NULL;
    }
    mDVolumpty = NULL;



    m_renderer->prepareExit();
    disconnect(this, &QOpenGLWidget::aboutToCompose, this, &GLWidget::onAboutToCompose);
    disconnect(this, &QOpenGLWidget::frameSwapped, this, &GLWidget::onFrameSwapped);
    disconnect(this, &QOpenGLWidget::aboutToResize, this, &GLWidget::onAboutToResize);
    disconnect(this, &QOpenGLWidget::resized, this, &GLWidget::onResized);
    disconnect(m_thread, &QThread::finished, m_renderer, &QObject::deleteLater);
    disconnect(this, &GLWidget::renderRequested, m_renderer, &Renderer::render);
    disconnect(m_renderer, &Renderer::contextWanted, this, &GLWidget::grabContext);




    m_thread->quit();
    m_thread->wait();
    delete m_thread;

    m_pConList = NULL;

#if NO_USE_AITK
    if(mpImgSegment != NULL)
    {
        delete mpImgSegment;
    }
#else
	if (mpItkSeg != NULL)
	{
		delete mpItkSeg;
	}
#endif

    while(!mTextInfoLabelList.isEmpty())
    {
        QLabel *pLabel=mTextInfoLabelList.first();
		pLabel->deleteLater();

        pLabel =NULL;
        mTextInfoLabelList.removeFirst();
    }

	qDebug() << "glwidget delete";

}

void GLWidget::paintEvent(QPaintEvent *event)
{

	QWidget::paintEvent(event);
}

bool GLWidget::event(QEvent *event)
{
	

  if(m_sCurMarkAction == E_MarkAction::MARK_SCROLL)
      return QWidget::event(event);

    switch (event->type())
    {
    case QEvent::TouchBegin:
    {

        QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);
        QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->touchPoints();
        const QTouchEvent::TouchPoint &touchPoint0 = touchPoints.first();
        mPreScreenPt = touchPoint0.startPos();
        mbTouching = true;
        return true;
    }
    case QEvent::TouchUpdate:
    {
        if(m_sbTouchOperateFlag == false)return true;

        QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);
        QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->touchPoints();
        if(touchPoints.count() == 1)
        {

            const QTouchEvent::TouchPoint &touchPoint0 = touchPoints.first();

            QPointF endpt = touchPoint0.lastPos();
			T_MSG_Value msgvalue;
			

			switch (m_sCurMarkAction)
			{
			case MARK_WCSET:
			{
				QPointF curpos = endpt - mPreScreenPt;
				QPoint pagepos;
				pagepos.setX(curpos.x());
				pagepos.setY(curpos.y());
				ProcessMarkActionImageWWandWCMouseMoveEvent(pagepos);
			}break;
			case MARK_DEFAULT:
			{
				QPointF curpos = endpt - mPreScreenPt;
				QPoint pagepos;
				pagepos.setX(curpos.x());
				pagepos.setY(curpos.y());
				ProcessMarkActionScrollPageMouseMoveEvent(pagepos);

			}break;
			case MARK_FREE_HAND:
			{
				if ((abs(endpt.y() - mPreScreenPt.y())>10) || (abs(endpt.x() - mPreScreenPt.x())>10))
				{
					QPointF delta = endpt - mPreScreenPt;
					mPreScreenPt = endpt;

					this->mHoriOffset += delta.x() / 2;
					this->mVertOffset += delta.y() / 2;


#if TranThread
					msgvalue.msgtype = E_MSG_Type::ImgTransform;
					msgvalue.yoffset = mVertOffset;
					msgvalue.xoffset = mHoriOffset;
					msgvalue.scaleFactor = mScaleFactor;					
#else
					emit renderRequested();
#endif
				}
			}break;


			default:
				break;
			}

			emit PushMsg(msgvalue);

    


        }else if(touchPoints.count()==2)
        {
            const QTouchEvent::TouchPoint &touchPoint0 = touchPoints.first();
            const QTouchEvent::TouchPoint &touchPoint1 = touchPoints.last();
            if(m_bIsTwoPoint == false)
            {
                mPreScreenPt= touchPoint0.startPos();

                m_bIsTwoPoint =true;
            }
            QPointF endpt = touchPoint0.lastPos();
            qreal scalevalue =QLineF(touchPoint0.pos(), touchPoint1.pos()).length()
                    /QLineF(touchPoint0.startPos(), touchPoint1.startPos()).length();

            if((abs(endpt.y()-mPreScreenPt.y())>5)||(abs(endpt.x()-mPreScreenPt.x())>5))
            {


#if GLWidgetDEBUG
                qDebug()<<"scalevalue is"<<scalevalue;
                QPointF endpt = touchPoint0.lastPos();
                qDebug()<<"endpt y is"<<endpt.y()<<"x is"<<endpt.x();
                qDebug()<<"previouspt y is"<<previouspt.y()<<"x is"<<previouspt.x();
#endif

                if(scalevalue >1)
                {
                    zoom((1+mZoomDelta));
                }else
                {
                    zoom((1-mZoomDelta));
                }
                mPreScreenPt= touchPoint0.pos();


#if TranThread

#else
                emit renderRequested();
#endif

            }

        }
        return true;
    }
    case QEvent::TouchEnd:
    {
        mbTouching = false;
        m_bIsTwoPoint = false;
    }
    default:
        break;
    }

    return QWidget::event(event);



}
void GLWidget::closeEvent(QCloseEvent * event)
{
	event->accept();
}
void GLWidget::mouseDoubleClickEvent(QMouseEvent *)
{
    PushResetImgSizeLocationMsg();
}
void GLWidget::mousePressEvent(QMouseEvent *event)
{

    if (event->button() == Qt::LeftButton)
    {
        m_bMouseTranslate = true;
        m_LastMousePos = event->pos();
        emit PushMousePressMsg();

		if (mpData)
		{
			QList<T_LastGLPressPtCtrl*>* pressptlt = 
				mpData->GetGLWdtLatestPressedPt();

			if (!pressptlt->isEmpty())
			{
				T_LastGLPressPtCtrl* pt = pressptlt->last();
				QPoint oldpt = pt->pos;
				QPoint newpt;
				if ((m_LastMousePos.x()>=(oldpt.x()-3))&& (m_LastMousePos.x() <= (oldpt.x() + 3)))
				{
					mIsSelectXLine = true;
				}
				if ((m_LastMousePos.y() >= (oldpt.y() - 3)) && (m_LastMousePos.y() <= (oldpt.y() + 3)))
				{
					mIsSelectYLine = true;
				}
			}

			T_SelectDragLineCtrl* dragctrl = mpData->GetCurSelectDragCtrl();
			dragctrl->dragImgType = this->mImgpty.imgtype;
			dragctrl->selectXline = mIsSelectXLine;
			dragctrl->selectYline = mIsSelectYLine;
		}
		
		QPoint point;
		if (mIsSelectXLine || mIsSelectYLine)
		{
			if (mIsSelectXLine != mIsSelectYLine)
			{
				QList<T_LastGLPressPtCtrl*>* pressptlt =
					mpData->GetGLWdtLatestPressedPt();
				T_LastGLPressPtCtrl* recordptctrl = pressptlt->last();
				QPoint recordpt = recordptctrl->pos;

				if (mIsSelectXLine)
				{
					point.setY(recordpt.y());
					point.setX(m_LastMousePos.x());
				}
				else
				{
					point.setY(m_LastMousePos.y());
					point.setX(recordpt.x());
				}
			}
			else
			{
				point = event->pos();
			}
		}
		else
		{
			point = event->pos();
		}


	    ProSliceTranslateEvent(point);
	 


		//ProSliceTranslateEvent(m_LastMousePos);

		mIsMouseMove = true;

        switch(m_sCurMarkAction)
        {
            case E_MarkAction::MARK_ROI:
            {
			/*	if (m_pConList == NULL)
				{
					QWidget::mousePressEvent(event);
					return;
				}*/
				switch (GLWidget::mCurROIStatusMode)
				{
				case ROI_CREATE_MODE:
				{
					MakeSureTheROIImageType();
				}
				default:
					break;
				}
                this->ProcessMarkActionROIMousePressEvent(event->pos());

            }break;

	
			case E_MarkAction::MARK_NONE:
			{
				QPoint ImPoint(event->pos());
				QPointF RealPoint(0, 0);
				if (m_renderer->ProcessOpenglCalculation(&ImPoint, &RealPoint, CAL_SCREENTOGL))
				{
					//if (GLToImage(RealPoint, ImPoint))
					//{
						FindCurToolIdx(RealPoint);
						if (mMoveStyle != MOVE_NONE)
						{
							m_sCurMarkAction = MARK_MOVE;
						}
						emit renderRequested();
					//}
				}

				
			}break;
            default:break;
        }
    }

	QWidget::mousePressEvent(event);
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    if(mbTouching)
    {
        QWidget::mouseMoveEvent(event);
        return;
    }

    if (m_bMouseTranslate)
    {
        QPoint curpos = event->pos();
        QPoint mouseDelta = curpos - m_LastMousePos;
        switch(m_sCurMarkAction)
        {
            case E_MarkAction::MARK_FREE_HAND:
            {
                if(m_sbTouchOperateFlag){translate(mouseDelta);}
            } break;
            case E_MarkAction::MARK_ROI:
            {
				m_LastMousePos = event->pos();
				/*if (m_pConList == NULL)
				{
					QWidget::mousePressEvent(event);
					return;
				}*/
              ProcessMarkActionROIMouseMoveEvent(curpos);

            }break;
            case E_MarkAction::MARK_DEFAULT:
            {
				if (!mIsRenderLine)
				{
					ProcessMarkActionScrollPageMouseMoveEvent(curpos);
				}
                
            }break;
            case E_MarkAction::MARK_WCSET:
            {
                ProcessMarkActionImageWWandWCMouseMoveEvent(curpos);
            }break;
			case E_MarkAction::MARK_MOVE:
			{
				ProcessMarkActionMeasureMoveEvent(curpos);
			}break;
            default:break;

        }
        m_LastMousePos = event->pos();		
    }	

	if (mIsMouseMove)
	{
		QPoint point;
		if (mIsSelectXLine || mIsSelectYLine)
		{
			if (mIsSelectXLine != mIsSelectYLine)
			{
				QList<T_LastGLPressPtCtrl*>* pressptlt = 
					mpData->GetGLWdtLatestPressedPt();
				T_LastGLPressPtCtrl* recordptctrl = pressptlt->last();
				QPoint recordpt = recordptctrl->pos;

				if (mIsSelectXLine)
				{
					point.setY(recordpt.y());
					point.setX(m_LastMousePos.x());
				}
				else
				{
					point.setY(m_LastMousePos.y());
					point.setX(recordpt.x());
				}
			}
			else
			{
				point = event->pos();
			}
		}
		else
		{
			point = event->pos();
		}


		ProSliceTranslateEvent(point);
	}

    QWidget::mouseMoveEvent(event);
}
void GLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_bMouseTranslate = false;
        setCursor(Qt::ArrowCursor);

        switch(m_sCurMarkAction)
        {

        case E_MarkAction::MARK_ROI:
        {
			/*if (m_pConList == NULL)
			{
				QWidget::mousePressEvent(event);
				return;
			}
*/
            QPoint ImPoint(event->pos());
            QPointF RealPoint(0,0);
            m_renderer->ProcessOpenglCalculation(&ImPoint,&RealPoint,CAL_SCREENTOGL);

            if(GLToImage(RealPoint, ImPoint))
            {
                if(m_bValidAreaFlag)
                {
                    m_bIsBrushing = false;
                    m_bValidAreaFlag = false;
                    emit renderRequested();
				/*	if(mImgpty.imgtype == Axial)
					{
						
					}*/
					PushROIOperateMsgtoMaindata();

                }
            }
            else
            {
                m_bIsBrushing = false;
            }
        } break;
		case E_MarkAction::MARK_ANGLE:
		case E_MarkAction::MARK_DISTANCE:
		case E_MarkAction::MARK_MARK:
		{
			QPoint ImPoint(event->pos());
			QPointF RealPoint(0, 0);
			m_renderer->ProcessOpenglCalculation(&ImPoint, &RealPoint, CAL_SCREENTOGL);

			//if (GLToImage(RealPoint, ImPoint))
			//{
				ProcessMarkActionMeasurePressEvent(m_sCurMarkAction, RealPoint);

			//}
		}break;
		case E_MarkAction::MARK_MOVE:
		{
			m_sCurMarkAction = MARK_NONE;
			mMoveStyle = MOVE_NONE;
			mCurmoveToolIdx = -1;
			mCurmoveToolPointIdx = -1;
			emit renderRequested();
		}break;
        }

    }



	if (mIsMouseMove)
	{
		QPoint point;
		if (mIsSelectXLine || mIsSelectYLine)
		{
			if (mIsSelectXLine != mIsSelectYLine)
			{
				QList<T_LastGLPressPtCtrl*>* pressptlt =
					mpData->GetGLWdtLatestPressedPt();
				T_LastGLPressPtCtrl* recordptctrl = pressptlt->last();
				QPoint recordpt = recordptctrl->pos;

				if (mIsSelectXLine)
				{
					point.setY(recordpt.y());
					point.setX(m_LastMousePos.x());
				}
				else
				{
					point.setY(m_LastMousePos.y());
					point.setX(recordpt.x());
				}
			}
			else
			{
				point = event->pos();
			}
		}
		else
		{
			point = event->pos();
		}


		ProSliceTranslateEvent(point);
	}




	//ProSliceTranslateEvent(event->pos());
	mIsMouseMove = false;
	mIsSelectXLine = false;
	mIsSelectYLine = false;
    QWidget::mouseReleaseEvent(event);
}

// ZOOM
void GLWidget::wheelEvent(QWheelEvent *event)
{
    switch(m_sCurMarkAction)
    {

    case E_MarkAction::MARK_FREE_HAND:
    {
        if(m_sbTouchOperateFlag)
        {
            QPoint scrallAmount = event->angleDelta();

            if(scrallAmount.y() > 0)
            {
                zoomIn();
            }
            else if(scrallAmount.y() < 0){
                zoomOut();
            }

        }
    } break;
    default:break;
    }

    QWidget::wheelEvent(event);
}


//set the data pointer from maindata
void GLWidget::SetMainData(MainData *pdata)
{
    if(pdata!=NULL)
    {
        this->mpData = pdata;

    }
}
void GLWidget::setDCMVolumePty(DICOMPropertyData *pty, T_DImageWindowCtrl *wc)
{
    if(NULL != pty)
        mDVolumpty = pty;
	if (NULL != wc)
		mImgWCCtrl = wc;


}
void GLWidget::setImageProperty(T_DImgPropety *pty , E_IMG_Multi_Show_Mode *pshowmode)
{

    if(NULL != pty)
        this->mImgpty = *pty;

	this->mpImgShowMode = pshowmode;



}
void GLWidget::SetMeasureLists()
{
	if (NULL != this->mpData)
	{
		m_pMeasuresDataCtrlList = this->mpData->GetMeasureDataCtrl();
	}
}
void GLWidget::SetROILists()
{
	if (NULL != this->mpData)
	{
		m_pROIsDataCtrlList = this->mpData->GetROIDataCtrl();
	}
}
void GLWidget::SetCurROIStatusMode(E_ROI_Status_Mode mode)
{
	GLWidget::mCurROIStatusMode = mode;
	GLWidget::m_sConColor = this->mpData->GetCurROIColor();
}
void GLWidget::MakeSureTheROIImageType()
{
	if (NULL != this->mpData)
	{
		E_ImageType curroitype = this->mpData->GetCurROIType();

		if (this->mImgpty.imgtype != curroitype)
		{
			m_pConList = this->mpData->GetConList();
			this->m_CurROIImgType = this->mImgpty.imgtype;

			this->mpData->setCurROIType(this->m_CurROIImgType);
		   GLWidget::mCurROIStatusMode = ROI_NONE_MODE;
		}


	}

}

void GLWidget::SetToolShow(QPoint tip, QPoint mid)
{
	mCurTip = tip;
	mCurMid = mid;
	emit renderRequested();
}

void GLWidget::SetCurROIContourListAndImgType(bool isSelected,bool colorchange)
{
	if (colorchange)
	{
		if (NULL != this->mpData)
		{
			GLWidget::m_sConColor = this->mpData->GetCurROIColor();

		}
		this->mbPaintFlag = true; 
		return;
	}

	if (isSelected)
	{
		if (NULL != this->mpData)
		{
			E_ImageType curroitype = this->mpData->GetCurROIType();
		    m_pConList = this->mpData->GetConList();
		    this->m_CurROIImgType = curroitype;
	
		}
		
	}
	else
	{
		m_pConList = NULL;
		this->m_CurROIImgType = ImgNone;
	}
	

}
void GLWidget::SetTextInfoHide(QList<bool>&hideList)
{
    quint8 size = hideList.size();
    if(size>TEXT_INFO_COUNT)return;
    for(int i =0;i< size;i++)
    {
        T_TextInfo *ptext = &m_arrTextInfo[i];
        if(hideList.at(i))
        {
            ptext->isShow = false;
        }

    }
}
void GLWidget::SetUpdateImgSourceData(bool flag)
{
    mbUpdateImgDataFlag = flag;
}

 void GLWidget::InitMakrActions()
{
	GLWidget::m_sCurMarkAction = MARK_DEFAULT;
}

 void GLWidget::SetRenderToolFlag(bool flag)
 {
	 mIsRenderTool = flag;
 }

//get image data ,view rect ,zoom ,translate,scale facotors
QImage * GLWidget::GetCurImageforRender()
{
    return this->mpCurQImg;
}
GLfloat * GLWidget::GetTextRectforRender()
{
    return this->m_arrTextRc;
}
GLfloat *GLWidget::GetViewPortForRender()
{
    return this->m_arrViewRc;
}
GLfloat *GLWidget::GetWatchPortForRender()
{
    return this->m_arrWatchRc;
}

#if TranThread
qreal GLWidget::GetWatchHorizontalOffsetForRender()
{
    return this->mMsgHoriOffset;
}
qreal GLWidget::GetWatchVerticalOffsetForRender()
{

    return this->mMsgVertOffset;
}
qreal GLWidget::GetWatchScaleFactorForRender()
{
    return this->mMsgScale;
}
#else
qreal GLWidget::GetWatchHorizontalOffsetForRender()
{

    return this->horizontalOffset;

}
qreal GLWidget::GetWatchVerticalOffsetForRender()
{
    return this->verticalOffset;

}
qreal GLWidget::GetWatchScaleFactorForRender()
{
    return this->mScaleFactor;
}
#endif
//get operation mark action type
E_MarkAction GLWidget::GetCurValidMarkAction()
{
    return this->m_sCurMarkAction;
}
void GLWidget:: GetCurColor(QColor &cr)
{
    cr = this->m_sConColor;
}
//judge the touch or update status
bool GLWidget::IsTouchOperating()
{
    return m_sbTouchOperateFlag;
}
bool GLWidget::IsLeftMousePressed()
{
    return this->m_bMouseTranslate;

}
bool GLWidget::IsUpdateImgSourceData()
{
    return mbUpdateImgDataFlag ;
}
//get ROI contour ,glpt,screempt, Radius
QList<T_Contour> * GLWidget::GetContourList()
{
    return this->m_pConList;
}
float GLWidget::GetCalculatedRadius()
{
    return m_BrushRadius;
}
void GLWidget:: GetScreenPtFromMsg(QPoint &pt)
{
    pt=this->mScreenPt;
}
QList<T_ROIDataCtrl*>* GLWidget::GetROIsList()
{
	return this->m_pROIsDataCtrlList;
}
QList<T_MeasureDataCtrl*>* GLWidget::GetMeasureToolsList()
{
	return this->m_pMeasuresDataCtrlList;
}
void GLWidget::UpdateROIStatus()
{
	emit renderRequested();
}
int GLWidget::GetCurSliceindex()
{
    return mImgpty.order;
}
void GLWidget::GetImageXYfactor(GLfloat &rAxfactor,GLfloat &rAyfactor)
{

    rAxfactor=1.0;
    rAyfactor=1.0;


    switch(this->mImgpty.imgtype)
    {
    case E_ImageType::Axial:
    {

        rAxfactor=mDVolumpty->xSpacing;
        rAyfactor=mDVolumpty->ySpacing;

    }
        break;
    case E_ImageType::Cornal:
    {
        rAxfactor=mDVolumpty->xSpacing;
        rAyfactor=mDVolumpty->zSpacing;
    }
        break;
    case E_ImageType::Sagigtal:
    {
        rAxfactor=mDVolumpty->ySpacing;
        rAyfactor=mDVolumpty->zSpacing;

    }
        break;
    default:break;

    }

}
E_ImageType GLWidget::GetCurImageType()
{
	return   this->mImgpty.imgtype;
}
//update translate zoom textinfo
E_ErrorCode GLWidget::UpdateImage(quint8 *pdata)
{

    if(NULL == pdata )
        return PtrNull;
    SetUpdateImgSourceData(true);
    mpCurImgData=&pdata[0];

    if(mpCurQImg!=NULL)
    {
        //delete mpCurQImg;
        mpCurQImg = NULL;
    }


    mpCurQImg = new QImage(this->mpCurImgData,mImgpty.width,mImgpty.height,QImage::Format_Grayscale8);

#if NO_USE_AITK
    mpImgSegment->SetImageRect(mImgpty.width,mImgpty.height);
    mpImgSegment->SetBrushCirclePt(&mBrushCirclePtList);
    mpImgSegment->SetBrushStatus(&mCurBrushState);
    mpImgSegment->SetImageSliceOrder(mImgpty.order);
#else
	if (NULL == mpItkSeg)
	{
		mpItkSeg = new AItkImgSeg(this);
		mpItkSeg->SetBrushEdgePts(&mBrushEdgePtList);
		mpItkSeg->SetBrushInnerPts(&mBrushInnerPtList);
		mpItkSeg->CreateItkMask(mImgpty.width, mImgpty.height);
	}
	mpItkSeg->SetImageRect(mImgpty.width, mImgpty.height);
	mpItkSeg->SetBrushStatus(&mCurBrushState);
	mpItkSeg->SetImageSliceOrder(mImgpty.order);
#endif

    InitGLRect(this->width(),this->height());

    this->mbPaintFlag = true;
    return Success;
}
void GLWidget::UpdateTextInfo()
{
  //  SetTextsInfo();
	switch (*mpImgShowMode)
	{
	case IMG_SINGLE_SHOW:
	{
		SetTextInfo(GL_Info_Basic);
		SetTextInfo(GL_Info_LocUp);
		SetTextInfo(GL_Info_WWC);
		for (int i = GL_Info_Basic; i < GL_Info_LocImg; i++)
		{
			SetLabelTextInfo(i);
		}
		mTextInfoLabelList.at(GL_Info_LocImg)->setHidden(false);
	}break;
	case IMG_MULTI_SHOW:
	{
		for (int i = GL_Info_Basic; i < GL_Info_LocImg; i++)
		{
			mTextInfoLabelList.at(i)->clear();
		}
		mTextInfoLabelList.at(GL_Info_LocImg)->setHidden(true);
	
		mTextInfoLabelList.at(GL_Info_LocDown)->setText(QString("S:%1").arg(mImgpty.order + 1) + QString("/%1").arg(mImgpty.total));
	}break;
	default:
		break;
	}
	
	
   // SetTextInfLocation();
}
void GLWidget::UpdateTranslationForImage(T_MSG_Value v)
{
    this->mMsgHoriOffset = v.xoffset;
    this->mMsgVertOffset = v.yoffset;
    this->mMsgScale = v.scaleFactor;

    emit renderRequested();

}
void GLWidget::UpdateROIForImage(T_MSG_Value v)
{
    this->mScreenPt.setX(v.xoffset);
    this->mScreenPt.setY(v.yoffset);

    emit SendMovePtRequest(this->mScreenPt);

}
// process msg from toolview btns
void GLWidget::ProcessToolMsgInGlWidget(E_MSG_Type msg)
{

    switch(msg)
    {
    case ZoomIn:
    {
        this->zoomIn();
    }break;
    case ZoomOut:
    {
        this->zoomOut();
    }break;
    case ImgReSet:
    {
        this->PushResetImgSizeLocationMsg();
		this->PushResetImgWCMsg();
    }break;
    case WCreset:
    {
        this->PushResetImgWCMsg();
    }break;
    case PrevPage:
    {
        this->ScrollPreviousImgPage();
    }break;
    case NextPage:
    {
        this->ScrollNextImgPage();
    }break;

    default:break;

    }
}
void GLWidget::ProcessToolMsgInGlWidget(E_MSG_Type msg,bool flag)
{
    if(flag == false)
    {
        m_sCurMarkAction = MARK_NONE;
        m_sbTouchOperateFlag = false;
        return;
    }
    switch(msg)
    {

    case TouchValid:
    {
        m_sCurMarkAction = MARK_FREE_HAND;
    }break;
    case ROI:
    {
        m_sCurMarkAction = MARK_ROI;
       // m_sConColor =QColor(255, 255, 0);
    }break;
    case ScrollPage:
    {
        m_sCurMarkAction = MARK_SCROLL;

    }break;
    case WCSet:
    {
        m_sCurMarkAction = MARK_WCSET;

    }break;
	case ChangePage:
	{
		m_sCurMarkAction = MARK_DEFAULT;
		//m_sCurMarkAction = MARK_NONE;
	}break;
	case MeasureAngel:
	{
		m_sCurMarkAction = MARK_ANGLE;
	}break;
	case MeasureDistance:
	{
		m_sCurMarkAction = MARK_DISTANCE;
	}break;
	case MeasureMark:
	{
		m_sCurMarkAction = MARK_MARK;
	}break;
    default:break;
    }
	m_sbTouchOperateFlag = true;
}
//ROI Calculate
void GLWidget::PrepareBrushContourWithAlrorithm(QPointF point,quint16 radius)
{
    quint16 w = mImgpty.width;
    quint16 h = mImgpty.height;
    E_ImageType type = mImgpty.imgtype;
    int order = mImgpty.order;

    QPoint ImPoint(0,0);

#if NO_USE_AITK
    if(IS_E_TRUE !=mpImgSegment->TextCoorToImCoor(point,ImPoint))
    {
        return;
    }
	mpImgSegment->SetCurrentImPt(ImPoint);
#else
	if (IS_E_TRUE != mpItkSeg->TextCoorToImCoor(point, ImPoint))
	{
		return;
	}
	mpItkSeg->SetCurrentImPt(ImPoint, w, h, m_BrushRadius);
#endif

    mpData->OpenMaskImage(type,order,w,h);
    quint8 **pmark = mpData->GetMaskImage(type,order);

#if NO_USE_AITK
    mpImgSegment->SetMaskImage(pmark);
#else
	mpItkSeg->SetMaskImage(pmark);
#endif

    mCurBrushState = M_IN;
    bool isExist = mpData->IsExistContour(order);

    if(isExist == false)
    {
        // mpImgSegment->InitMaskImage();
        mpData->InitMaskImage(type,order);
		mPtCtnOldRoI = 0;
    }
	else
	{
        // quint8** pMaskImg = mpImgSegment->GetMaskImage();

        quint8 **pMaskImg = mpData->GetMaskImage(type,order);
        T_Contour *pcontour = (T_Contour *)this->GetCurImageContour();
        if(pcontour !=NULL)
        {
            if(pcontour->isfromfile)
            {
				const  QList<QPoint> *plist =&( pcontour->pImgContourPtList);

#if NO_USE_AITK
				mpImgSegment->InitMaskImageAccordingContour(pMaskImg,
                                                            mImgpty.width,
                                                            mImgpty.height,
                                                            plist);
#else
				mpItkSeg->InitMaskImageAccordingContour(pMaskImg,
						mImgpty.width, mImgpty.height,
						plist);
#endif

                pcontour->isfromfile = false;
            }
			mPtCtnOldRoI = pcontour->pTextPtList.size();
        }


        if(pMaskImg[ImPoint.x()][ImPoint.y()]==M_OUT)
        {
            mCurBrushState = M_OUT;
        }
        else
        {
            mCurBrushState = M_IN;
        }

    }

#if NO_USE_AITK
    mBrushCirclePtList.clear();
    float xyratio = m_ImgXYRatio;
    for(int i=-radius; i<=radius; i++)
        for(int j=-radius; j<=radius; j++)
        {
            int ab=i*i+j*j;
            if(ab< radius*radius)
            {
                if(xyratio>1)
                {
                    mBrushCirclePtList.append(QPointF(i, j*xyratio));
                }
                else if(xyratio<1)
                {
                    mBrushCirclePtList.append(QPointF(i*xyratio, j));
                }else
                {
                    mBrushCirclePtList.append(QPointF(i, j));
                }
            }
        }
#else
	/**半径等于4或11时，边界存在多余的点，应该去除 */
	if ((4 == radius) || (11 == radius))
	{
		//法一
		//RefineBrushEdge();
		//法二：直接移除最后四个点
		for (quint8 i = 0; i < 4; i++)
		{
			mBrushEdgePtList.removeLast();
		}
	}

	if (m_ImgXYRatio>1)
	{
		/**更新画刷内部点 */
		for (int i = 0; i<mBrushInnerPtList.count(); ++i)
		{
			mBrushInnerPtList[i].setY(mBrushInnerPtList[i].y()*m_ImgXYRatio);
		}
		/**更新画刷边缘点 */
		for (int i = 0; i<mBrushEdgePtList.count(); ++i)
		{
			mBrushEdgePtList[i].setY(mBrushEdgePtList[i].y()*m_ImgXYRatio);
		}
	}
	else if (m_ImgXYRatio<1)
	{
		for (int i = 0; i<mBrushInnerPtList.count(); ++i)
		{
			mBrushInnerPtList[i].setX(mBrushInnerPtList[i].x()*m_ImgXYRatio);
		}
		for (int i = 0; i<mBrushEdgePtList.count(); ++i)
		{
			mBrushEdgePtList[i].setX(mBrushEdgePtList[i].x()*m_ImgXYRatio);
		}
	}
#endif


    m_bIsBrushing = true;
}
void GLWidget::CombineBrushContour(QList<QPointF> *point)
{
    if(!m_bIsBrushing)
        return;

#if NO_USE_AITK
    mpImgSegment->CombineBrushContourAlgorithm(m_pConList,point,m_bIsBrushing,m_BrushRadius);
#else
	mpItkSeg->CombineBrushContourAlgorithm(m_pConList, point, m_bIsBrushing, m_BrushRadius);
#endif

    if(m_pConList->isEmpty())
    {
        this->m_CurROIImgType = ImgNone;
        this->mbROIEnale = false;
    }

}
bool GLWidget::GLToImage(QPointF RealP, QPoint &ImPoint)
{
    ImPoint.setX(int(mImgpty.width*(RealP.x()+m_arrTextRc[1])/(m_arrTextRc[1]-m_arrTextRc[0])));
    ImPoint.setY(mImgpty.height-int(mImgpty.height*(RealP.y()+m_arrTextRc[3])/(m_arrTextRc[3]-m_arrTextRc[2])));
    if((ImPoint.x()>0)&&(ImPoint.y()<mImgpty.height)&&(ImPoint.x()>0)&&(ImPoint.x()<mImgpty.width))
        return true;
    else
        return false;
}
E_ImageType GLWidget::CurROIImgType()
{
    return m_CurROIImgType;
}
const T_Contour *GLWidget::GetCurImageContour()
{
    QList<T_Contour> *pConList = this->m_pConList;
    if(pConList == NULL)
    {
        return NULL;
    }

    const T_Contour *pcontour = NULL;

    if(!pConList->isEmpty())
    {
        int sliceindex = this->GetCurSliceindex();
        for(int i=0; i<pConList->count(); i++)
        {
            pcontour = &(pConList->at(i));

            if(pcontour->SliceIndex == sliceindex)
            {

                break;
			}
			else
			{
				pcontour = NULL;
			}
        }
    }
    return pcontour;
}
/**
*Function Describe: Useing Bresenham algorithm to Init the brush edges and interior points.
*Name: InitBrushPts
*Parameter: quint8 radius
*Input: radius of brush
*Output: brush edges and interior points
*Create:    yangzefu                Date: 2018-05-04
*Note:       Who:          Date:
*1.
*2.
*Modify :    Who:          Date:
*1.
*2.
*/
void GLWidget::InitBrushPts(quint8 radius)
{
	mBrushEdgePtList.clear();
	mBrushInnerPtList.clear();
	mBrushPtList.clear();

	int xs = 0, ys = radius;

	/**d: initial value of decision parameter 
	 **    xs = 0; ys = R;
	 **    d = (xs+1)^2 + (ys-0.5)^2 - R^2 = 1.25 - R
	 ** Double the calculation results and avoid floating-point calculations: 
	 **    (1.25-R) -> (1.25-R)*2 -> (2.5-2*R) -> (3-2*R) 
	 **	   INIT_PARAM_A = 3, INIT_PARAM_B = 2
	 */
	int d = INIT_PARAM_A - INIT_PARAM_B * radius;

	/**quartered symmetry transformation */
	mBrushEdgePtList.append(QPoint(xs, ys));
	mBrushEdgePtList.append(QPoint(xs, -ys));
	mBrushEdgePtList.append(QPoint(ys, xs));
	mBrushEdgePtList.append(QPoint(-ys, xs));

	/**mBrushEdgePtList stores the points on the edge,
	 **mBrushPtList stores all the points on the bursh */
	while (xs<ys)
	{
		if ((0 != xs) || (radius != ys))
		{
			/**Octave symmetry transformation */
			mBrushEdgePtList.append(QPoint(xs, ys));
			mBrushEdgePtList.append(QPoint(xs, -ys));
			mBrushEdgePtList.append(QPoint(-xs, ys));
			mBrushEdgePtList.append(QPoint(-xs, -ys));
			mBrushEdgePtList.append(QPoint(ys, xs));
			mBrushEdgePtList.append(QPoint(ys, -xs));
			mBrushEdgePtList.append(QPoint(-ys, xs));
			mBrushEdgePtList.append(QPoint(-ys, -xs));
		}

		for (int i = -ys; i <= ys; ++i)
		{
			mBrushPtList.append(QPoint(i, -xs));
			if (0 != xs)
			{
				mBrushPtList.append(QPoint(i, xs));
			}
		}

		/**if d<0, the point outside the circle; if d>0, the point in the circle */
		if (d<0)
		{
			/**d = (xs+2)^2 + (ys-0.5)^2 -R^2 = 2*xs + 3 
			 **then you should double result: d = 2*d = 4*sx + 6 */
			d += PARAM_D1 * xs + PARAM_D2;

		}// Take an above point
		else
		{
			for (int i = -xs; i <= xs; ++i)
			{
				mBrushPtList.append(QPoint(i, -ys));
				mBrushPtList.append(QPoint(i, ys));
			}

			/**d = (xs+2)^2 + (ys-1.5)^2 - R^2 = 2*(xs-ys) + 5 
			 **then you should double result: d = 2*d = 4*(xs-ys) + 10 */
			d += PARAM_D1 * (xs - ys) + PARAM_D3;
			ys--;

		}// Take a following point
		xs++;
	}

	/**Diagonal points */
	if (xs == ys)
	{
		for (int i = -ys; i <= ys; ++i)
		{
			mBrushPtList.append(QPoint(i, -xs));
			mBrushPtList.append(QPoint(i, xs));
		}
		/**quartered symmetry transformation */
		mBrushEdgePtList.append(QPoint(xs, ys));
		mBrushEdgePtList.append(QPoint(xs, -ys));
		mBrushEdgePtList.append(QPoint(-xs, ys));
		mBrushEdgePtList.append(QPoint(-xs, -ys));
	}

	/**Find the difference between mBrushPtList and mBrushEdgePtList  */
	for (int i = 0; i < mBrushPtList.size(); i++)
	{
		/**First, append the element*/
		mBrushInnerPtList.append(mBrushPtList.at(i));
		/**Then determine if the element exists in mBrushEdgePtList */
		for (int j = 0; j < mBrushEdgePtList.size(); j++)
		{
			/**If it exists, remove this element */
			if ((mBrushPtList[i].x() == mBrushEdgePtList[j].x()) &&
				  (mBrushPtList[i].y() == mBrushEdgePtList[j].y()) )
			{
				mBrushInnerPtList.removeLast();
				break;
			}
		}
	}
}
/**
*Function Describe: To Get standard brush edge pts.
*Name: RefineBrushEdge
*Parameter: 
*Input: QList<QPoint>mBrushEdgePts: origin brush edge pts
*Output: standard brush edge pts 
*Create:    yangzefu                Date: 2018-05-07
*Note:       Who:          Date:
*1.
*2.
*Modify :    Who:          Date:
*1.
*2.
*/
void GLWidget::RefineBrushEdge()
{
	QList<QPointF> temp_list;
	temp_list.clear();

	/*
	**The order of eight neighborhoods (x positive: right, y positive: up)
	 —————
	| 8 | 1 | 2 |
	|—————
	| 7 |   | 3 |
	|—————
	| 6 | 5 | 4 |
	 —————
	**			          1       2       3        4        5         6        7        8 */
	int Os8[8][2] = { {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1} };


	/**1.Sort clockwise in the mBrushEdgePtList */
	temp_list.append(mBrushEdgePtList.takeFirst());
	while ( !mBrushEdgePtList.isEmpty() )
	{
		QPointF pt = temp_list.last();
		/**Find the eight-neighborhood of the current point pt clockwise */
		for (int i = 0; i < 8; i++)
		{
			QPoint neigh_pt(0, 0);
			neigh_pt.setX(pt.x() + Os8[i][0]);
			neigh_pt.setY(pt.y() + Os8[i][1]);

			/**Extract the first neighborhood point found in mBrushEdgePtList */
			int idx = mBrushEdgePtList.indexOf(neigh_pt);
			if (-1 != idx)
			{
				temp_list.append(mBrushEdgePtList.takeAt(idx));
				break;
			}
		}
	}

	/**2.Determine the number of eight-neighborhood points for each point in order and
	 **handle points greater than two */
	for (int i = 0; i < temp_list.size(); i++)
	{
		int cnt = 0;
		QPointF pt = temp_list.at(i);

		for (int j = 0; j < 8; j++)
		{
			QPoint neigh_pt(0, 0);
			neigh_pt.setX(pt.x() + Os8[j][0]);
			neigh_pt.setY(pt.y() + Os8[j][1]);
			if (temp_list.contains(neigh_pt))
			{
				cnt++;
			}
		}

		/**If the number of neighbors in the list in this point is greater than two, then 
		 **the next point in the clockwise order is the point that needs to be removed. */
		if (cnt > 2)
		{
			/**Extra points to remove */
			QPointF p = temp_list.at(i + 1);
			/**The point is quartered symmetric */
			int sign[4][2] = { {1,1}, {1,-1}, {-1,1}, {-1,-1} };			
			for (int k = 0; k < 4; k++)
			{
				QPoint extra_pt(p.x()*sign[k][0], p.y()*sign[k][1]);
				int idx = temp_list.indexOf(extra_pt);
				temp_list.removeAt(idx);
			}

			break;
		}
	}

	mBrushEdgePtList.append(temp_list);
}
void GLWidget::SetToolcellWdtHiden(bool flag)
{
//	this->mpToolWdts->setHidden(flag);
}
void GLWidget::SetToolCellWdtCommStatus(int idx)
{
//	this->mpToolWdts->SetToolCellCommImage(idx);
}
void GLWidget::SetTextInfoLocationMode(E_GL_WdtShowStats showstatus)
{
	mWdtShowStatus = showstatus;
}
E_GL_WdtShowStats GLWidget::GetTextInfoLactionMode()
{
	return this->mWdtShowStatus;
}
GLfloat * GLWidget::GetCurArrTextRc()
{
	return m_arrTextRc;
}
QPoint GLWidget::GetLastPressPos()
{
	return m_LastMousePos;
}

T_DImgPropety GLWidget::GetImageSize()
{
	return this->mImgpty;
}


MainData * GLWidget::GetMainData()
{
	return this->mpData;
}

void GLWidget::ProSliceTranslateEvent(QPoint point)
{
	if (mIsRenderLine)
	{
		T_LastGLPressPtCtrl* ptctrl = new T_LastGLPressPtCtrl;
		ptctrl->imagetype = this->mImgpty.imgtype;
		ptctrl->pos = point;
		QList<T_LastGLPressPtCtrl*>* pressptlt = mpData->GetGLWdtLatestPressedPt();
		T_SelectDragLineCtrl* draglinectrl = mpData->GetCurSelectDragCtrl();

		switch (mImgpty.imgtype)
		{
		case Axial:
		{
			int width = this->width();
			int height = this->height();

			if (draglinectrl->selectXline || draglinectrl->selectYline)
			{
				if (draglinectrl->selectXline != draglinectrl->selectYline)
				{
					int* idx;
					if (draglinectrl->selectXline)
					{
						idx = GetSelectDragLineSliceIdx(draglinectrl->dragImgType, 0);
					}
					else
					{
						idx = GetSelectDragLineSliceIdx(draglinectrl->dragImgType, 1);
					}
					ptctrl->coronalorder = idx[0];
					ptctrl->sagittalorder = idx[1];
					ptctrl->axialorder = mImgpty.order;
					break;
				}
			}


			ptctrl->coronalorder = mImgpty.height*m_LastMousePos.y() / height;
			if (ptctrl->coronalorder >= mImgpty.height)
			{
				ptctrl->coronalorder = mImgpty.height-1;
			}
			else if (ptctrl->coronalorder <= 0)
			{
				ptctrl->coronalorder = 0;
			}

			ptctrl->sagittalorder = mImgpty.width*m_LastMousePos.x() / width;
			if (ptctrl->sagittalorder >= mImgpty.width)
			{
				ptctrl->sagittalorder = mImgpty.width-1;
			}
			else if (ptctrl->sagittalorder <= 0)
			{
				ptctrl->sagittalorder = 0;
			}

			ptctrl->axialorder = mImgpty.order;

		}break;
		case Cornal:
		{

			int width = this->width();
			int height = this->height();


			if (draglinectrl->selectXline || draglinectrl->selectYline)
			{
				if (draglinectrl->selectXline != draglinectrl->selectYline)
				{
					int* idx;
					if (draglinectrl->selectXline)
					{
						idx = GetSelectDragLineSliceIdx(draglinectrl->dragImgType, 0);
					}
					else
					{
						idx = GetSelectDragLineSliceIdx(draglinectrl->dragImgType, 1);
					}
					ptctrl->axialorder = idx[0];
					ptctrl->sagittalorder = idx[1];
					ptctrl->coronalorder = mImgpty.order;
					break;
				}
			}


			//select line
			ptctrl->axialorder = mImgpty.height - mImgpty.height*m_LastMousePos.y() / height;
			if (ptctrl->axialorder >= mImgpty.height)
			{
				ptctrl->axialorder = mImgpty.height-1;
			}
			else if (ptctrl->axialorder <= 0)
			{
				ptctrl->axialorder = 0;
			}

			ptctrl->sagittalorder = mImgpty.width*m_LastMousePos.x() / width;
			if (ptctrl->sagittalorder >= mImgpty.width)
			{
				ptctrl->sagittalorder = mImgpty.width-1;
			}
			else if (ptctrl->sagittalorder <= 0)
			{
				ptctrl->sagittalorder = 0;
			}

			ptctrl->coronalorder = mImgpty.order;
		}break;
		case Sagigtal:
		{

			int width = this->width();
			int height = this->height();

			if (draglinectrl->selectXline || draglinectrl->selectYline)
			{
				if (draglinectrl->selectXline != draglinectrl->selectYline)
				{
					int* idx;
					if (draglinectrl->selectXline)
					{
						idx = GetSelectDragLineSliceIdx(draglinectrl->dragImgType, 1);
					}
					else
					{
						idx = GetSelectDragLineSliceIdx(draglinectrl->dragImgType, 0);
					}
					ptctrl->axialorder = idx[0];
					ptctrl->coronalorder = idx[1];
					ptctrl->sagittalorder = mImgpty.order;
					break;
				}
			}


			//select
			ptctrl->axialorder = mImgpty.height - mImgpty.height*m_LastMousePos.y() / height;
			if (ptctrl->axialorder >= mImgpty.height)
			{
				ptctrl->axialorder = mImgpty.height-1;
			}
			else if (ptctrl->axialorder <= 0)
			{
				ptctrl->axialorder = 0;
			}

			ptctrl->coronalorder = mImgpty.width*m_LastMousePos.x() / width;
			if (ptctrl->coronalorder >= mImgpty.height)
			{
				ptctrl->coronalorder = mImgpty.height-1;
			}
			else if (ptctrl->coronalorder <= 0)
			{
				ptctrl->coronalorder = 0;
			}

			ptctrl->sagittalorder = mImgpty.order;
		}break;
		default:
			break;
		}

		pressptlt->clear();
		mpData->AddRobotSurgicalPathCtrl(ptctrl);
		emit SendToUpdateSliceChange();
	}
}
void GLWidget::DrawSuccessPuncturePt()
{
	emit renderRequested();
}


int * GLWidget::GetSelectDragLineSliceIdx(E_ImageType type,int idx)
{
	int sliceidx[2] = { 0,0 };
	QList<T_LastGLPressPtCtrl*>* pressptlt = mpData->GetGLWdtLatestPressedPt();
	if (pressptlt->isEmpty())return sliceidx;

	int width = this->width();
	int height = this->height();

	T_LastGLPressPtCtrl* lastpt = pressptlt->last();
	switch (type)
	{
	case Axial:
	{
		if (idx == 0)
		{
			sliceidx[0] = lastpt->coronalorder;
			sliceidx[1] = mImgpty.width*m_LastMousePos.x() / width;
		}
		else
		{
			sliceidx[0] = mImgpty.height*m_LastMousePos.y() / height;
			sliceidx[1] = lastpt->sagittalorder;
		}

	}break;
	case Cornal:
	{
		if (idx == 0)
		{
			sliceidx[0] = lastpt->axialorder;
			sliceidx[1] = mImgpty.width*m_LastMousePos.x() / width;
		}
		else
		{
			sliceidx[0] = mImgpty.height - mImgpty.height*m_LastMousePos.y() / height;
			sliceidx[1] = lastpt->sagittalorder;
		}
	}break;
	case Sagigtal:
	{
		if (idx == 0)
		{
			sliceidx[0] = mImgpty.height - mImgpty.height*m_LastMousePos.y() / height;
			sliceidx[1] = lastpt->coronalorder;
		}
		else
		{
			sliceidx[0] = lastpt->axialorder;
			sliceidx[1] = mImgpty.width*m_LastMousePos.x() / width;
		}
	}break;
	default:
		break;
	}
	return sliceidx;

}

/**********************PROTECTED******************************/
void GLWidget::InitRenderThread()
{
    connect(this, &QOpenGLWidget::aboutToCompose, this, &GLWidget::onAboutToCompose);
    connect(this, &QOpenGLWidget::frameSwapped, this, &GLWidget::onFrameSwapped);
    connect(this, &QOpenGLWidget::aboutToResize, this, &GLWidget::onAboutToResize);
    connect(this, &QOpenGLWidget::resized, this, &GLWidget::onResized);

    m_thread = new QThread;
    m_renderer = new Renderer(this);
    m_renderer->moveToThread(m_thread);

    QSurfaceFormat  format;

    format.setProfile(QSurfaceFormat::OpenGLContextProfile::CompatibilityProfile);
    QSurfaceFormat::setDefaultFormat(format);

    this->setFormat(format);


    connect(m_thread, &QThread::finished, m_renderer, &QObject::deleteLater);
    connect(this, &GLWidget::renderRequested, m_renderer, &Renderer::render);
    connect(m_renderer, &Renderer::contextWanted, this, &GLWidget::grabContext);

    m_thread->start();
}
void GLWidget::InitDrawTextInfo()
{
    for(int i =0;i< TEXT_INFO_COUNT;i++)
    {
       T_TextInfo *ptext = &m_arrTextInfo[i];
        ptext->isShow = true;
        ptext->opacity = 1;
        ptext->str1 = QString("");
        ptext->icolor = QColor(255, 170, 0);

        QLabel *label = CreateTextlabel(GL_InfoText_fsize,EN_Type, GL_Info_Basic_Textcolor);
		label->setText("text");
        mTextInfoLabelList.append(label);

    }
}
QLabel* GLWidget::CreateTextlabel(int fontsize, E_FontType ftype, E_GL_Info_TextColorOrder color)
{
	QColor textcolor(GL_Info_TextColor[color][Aim_RED],
		GL_Info_TextColor[color][Aim_GREEN],
		GL_Info_TextColor[color][Aim_BLUE]);
	QPalette pe;
	pe.setColor(QPalette::WindowText, textcolor);
	pe.setColor(QPalette::Background, QColor(255, 255, 255));
	
	int fontorder = ftype;
#if LANGUAGE
	fontorder = 1;
#endif
	QFont font(FontTypeStr[fontorder], fontsize, QFont::Normal);

	QLabel *label = new QLabel(this);
	label->setFont(font);
	label->setPalette(pe);
	label->setAlignment(Qt::AlignLeft);
	

	return label;

}
void GLWidget::SetTextsInfo()
{
    SetTextInfoImagetype();
    SetTextInfoOrder();
    SetTextInfoWindowWidth();
    SetTextInfoWindowCenter();
}
void GLWidget::SetTextInfLocation()
{
    int width = this->width();
    int height = this->height();

    T_TextInfo *ptext = &m_arrTextInfo[0];
    ptext->LocationX = 10;
    ptext->LocationY = 20;

    ptext = &m_arrTextInfo[1];
    ptext->LocationX =width-50;
    ptext->LocationY = 20;

    ptext = &m_arrTextInfo[2];
    ptext->LocationX =width-50;
    ptext->LocationY = 35;

    ptext = &m_arrTextInfo[3];
    ptext->LocationX =(width-50)/2;
    ptext->LocationY =height-25;

}
void GLWidget::SetTextInfoImagetype()
{
    T_TextInfo * pText = m_arrTextInfo;
    switch(mImgpty.imgtype)
    {
    case E_ImageType::Axial:
    {

        pText[E_TEXTINFO_IDX::INFO_TYPE].str1 = QString("Axial");
    }
        break;
    case E_ImageType::Cornal:
    {

        pText[E_TEXTINFO_IDX::INFO_TYPE].str1 = QString("Cornal");
    }
        break;
    case E_ImageType::Sagigtal:
    {
        pText[E_TEXTINFO_IDX::INFO_TYPE].str1 = QString("Sagigtal");
    }
        break;
    default:break;

    }


}
void GLWidget::SetTextInfoOrder()
{
    m_arrTextInfo[E_TEXTINFO_IDX::INFO_ORDER].str1=QString("S:%1").arg(mImgpty.order+1)+QString("/%1").arg(mImgpty.total);
}
void GLWidget::SetTextInfoWindowWidth()
{
    m_arrTextInfo[E_TEXTINFO_IDX::INFO_WINWIDTH].str1 = QString("WW:%1").arg(mImgWCCtrl->windowWidth);
}
void GLWidget::SetTextInfoWindowCenter()
{
    m_arrTextInfo[E_TEXTINFO_IDX::INFO_WINCENTER].str1 = QString("WC:%1").arg(mImgWCCtrl->windowCenter);
}

/***************************************************
*Function Describe:show imagetext information
*Name:  void GLWidget::ShowImageTextInfo()
*Parameter:
*Input:
*Output:
*Create:                   Date:
*Note:       Who:  xyj        Date:170929
*1.
*2.
*Modify :    Who:          Date:
*1.change the way to show the text (before use opengl;now use QLabel)
*2.modify the methord of layout textinfo and the function name .wangzhigang 2018-03-08
*************************************************/
void GLWidget::LayoutTextInfoLabels()
{

	switch (*mpImgShowMode)
	{
	case IMG_SINGLE_SHOW:
	{
		this->LayoutTextInfoInSingleViewMode();
	}break;
	case IMG_MULTI_SHOW:
	{
		this->LayouTextInfoInMultiViewMode();
	}break;
	default:
		break;
	}
		

}
void GLWidget::LayoutTextInfoInSingleViewMode()
{
	int width = this->width();
	int height = this->height();

	for (int i = 0; i < TEXT_INFO_COUNT; i++)
	{
		double * pRectRate = NULL;
		switch (mWdtShowStatus)
		{
		case GL_CommonStates:
		{
			pRectRate = GL_TextInfo_CommRect[i];
		}
		break;
		case GL_SmallStates:
		{
			pRectRate = GL_TextInfo_SmallRect[i];
		}break;
		case GL_ZoominStates:
		{
			pRectRate = GL_TextInfo_ZoominRect[i];
		}
		break;
		default:
			break;
		}

		QRect rc(pRectRate[0] * width, pRectRate[1] * height,
			pRectRate[2] * width, pRectRate[3] * height);

		QLabel*plabel = mTextInfoLabelList.at(i);

		plabel->setGeometry(rc);

	}


}
void GLWidget::LayouTextInfoInMultiViewMode()
{
	int width = this->width();
	int height = this->height();

	int i = GL_Info_LocDown;
	double * pRectRate = GL_MultiShow_TxtInfo_Rect[mWdtShowStatus];
		
	QRect rc(pRectRate[0] * width, pRectRate[1] * height,
			pRectRate[2] * width, pRectRate[3] * height);

	QLabel*plabel = mTextInfoLabelList.at(i);
	plabel->setGeometry(rc);

	
}
void GLWidget::SetLabelTextInfo(int idx)
{
	mTextInfoLabelList.at(idx)->setText(m_arrTextInfo[idx].str1);
	
}
void GLWidget::SetTextInfo(E_GL_TextInfo idx)
{

	switch (idx)
	{
	case GL_Info_Basic: { SetTextImagetypeAndOrder();}break;
	case GL_Info_LocUp: { SetTextWindowWidthCenter();} break;
	case GL_Info_WWC: { SetTextDirectionInfo();}break;
	case GL_Info_LocImg: {}break;
	default:
		break;
	}
}
void GLWidget::SetTextImagetypeAndOrder()
{

	switch (mImgpty.imgtype)
	{
	case E_ImageType::Axial:		
	case E_ImageType::Cornal:		
	case E_ImageType::Sagigtal:
	{
		
		m_arrTextInfo[GL_Info_Basic].str1 = QString(GL_Info_DirectionType[mImgpty.imgtype])+ "\n"+ 
			QString("Im:%1").arg(mImgpty.order + 1) + QString("/%1").arg(mImgpty.total);
	
	}	
	break;
	default:break;

	}

}
void GLWidget::SetTextWindowWidthCenter()
{
	m_arrTextInfo[GL_Info_WWC].str1 = QString("WW:%1").arg(mDVolumpty->windowWidth)+"\n"+ 
		QString("WC:%1").arg(mDVolumpty->windowCenter);

}
void GLWidget::SetTextDirectionInfo()
{

	switch (mImgpty.imgtype)
	{
	case E_ImageType::Axial:
	case E_ImageType::Cornal:	
	case E_ImageType::Sagigtal:
	{
		SetTextInfoDirectin(GL_Info_DirectionInfo[mImgpty.imgtype]);
	}
	break;
	default:break;

	}

}
void GLWidget::SetTextInfoDirectin(const char ** dirction)
{

	for (int i = GL_Info_Location_Up; i < GL_Info_WWC_Textcolor; i++)
	{
		int idx = i - GL_Info_Location_Up;
		m_arrTextInfo[i].str1 = QString(dirction[idx]);
	}
	

}
void GLWidget::SetTextDirectinImg()
{
	int idx = 0;
	switch (mImgpty.imgtype)
	{
	case E_ImageType::Axial:
	{
		idx = GL_Direction_Axial;
	}break;
	case E_ImageType::Cornal:
	{
		idx = GL_Direction_Cornal;
	}break;
	case E_ImageType::Sagigtal:
	{
		idx = GL_Direction_Sagigtal;
			
	}
	break;
	default:break;

	}
	QLabel*plabel = mTextInfoLabelList.at(GL_Info_LocImg_Rc);
	SetLabelImage(plabel, GL_Info_ImgShowStr, idx);
	
}
void GLWidget::SetLabelImage(QLabel *plabel, const char **pImg, int idx)
{
	if (plabel)
	{

		QPixmap pixmap(pImg[idx]);
		if (!pixmap.isNull())
		{
		//	KeepAspectRatioExpending
			QPixmap fitpixmap = pixmap.scaled(plabel->width(), plabel->height(),
				Qt::KeepAspectRatio, Qt::SmoothTransformation);
			plabel->setPixmap(fitpixmap);
		}

	}

}
/***************************************************
*Function Describe:goto previous image and show it when press previous page btn.
*Name:ScrollPreviousImgPage
*Parameter:
*Input:
*Output:
*Create:        wangzhigang           Date: 2017-10-10
*Note:       Who:          Date:
*1.for signle view,goto previous image
*2.
*Modify :    Who:          Date:
*1.
*2.
*************************************************/
void GLWidget::ScrollPreviousImgPage()
{
    if(mImgpty.order == 0)return;
    T_MSG_Value msgvalue;
    msgvalue.msgtype = E_MSG_Type::PrevPage;
    msgvalue.ScrollPage =1;
    PushViewMsgForUpDateImageData(msgvalue);
	QList<T_LastGLPressPtCtrl*> *ptctrllt = mpData->GetGLWdtLatestPressedPt();
	if (!ptctrllt->isEmpty())
	{
		T_LastGLPressPtCtrl* ctrl = ptctrllt->at(0);
		ProSliceTranslateEvent(ctrl->pos);
	}
}
void GLWidget::ScrollNextImgPage()
{
	if (mImgpty.order == mImgpty.total - 1)return;
	T_MSG_Value msgvalue;
	msgvalue.msgtype = E_MSG_Type::NextPage;
	msgvalue.ScrollPage = -1;
	PushViewMsgForUpDateImageData(msgvalue);  
	QList<T_LastGLPressPtCtrl*> *ptctrllt = mpData->GetGLWdtLatestPressedPt();
	if (!ptctrllt->isEmpty())
	{
		T_LastGLPressPtCtrl* ctrl = ptctrllt->at(0);
		ProSliceTranslateEvent(ctrl->pos);
	}
}
//msg push interface
void GLWidget::PushResetImgWCMsg()
{
    T_MSG_Value msgvalue;
    msgvalue.msgtype = E_MSG_Type::WCreset;
    PushViewMsgForUpDateImageData(msgvalue);
}
void GLWidget::PushViewMsgForUpDateImageData(T_MSG_Value v)
{

    emit pushmsgtoimagemultiwidget( v);

}

bool GLWidget::IsROIOperattionEnable()
{
    bool enable =false;
    if(this->m_CurROIImgType == ImgNone)
    {
        this->m_CurROIImgType = this->mImgpty.imgtype;
        this->mbROIEnale = true;
		this->mpData->setCurROIType(m_CurROIImgType);
		this->mpData->UpdateROICtrlData(m_CurROIImgType);
        enable =true;
    }else
    {
        if (this->m_CurROIImgType == this->mImgpty.imgtype)
        {
            enable =true;
        }
    }
    return enable;
}
void GLWidget::PushROIOperateMsgtoMaindata()
{
	E_ROIOperateType type = ROI_None;
	T_Contour *pcontour = (T_Contour *)this->GetCurImageContour();
	if (pcontour != NULL)
	{
		quint32 newsize = pcontour->pTextPtList.size();
		int result = mPtCtnOldRoI - newsize;
		if (abs(result)<2)
		{
			type = ROI_None;
			return;
		}
		else
		{			
			type = ROI_Modify;
			if (mPtCtnOldRoI == 0)
			{
				type = ROI_Add;
			}
		}
	}
	else
	{
		if (mPtCtnOldRoI != 0)
		{
			type = ROI_Delete;
			if (this->m_pConList->isEmpty())
			{
				GLWidget::mCurROIStatusMode = ROI_CREATE_MODE;
			}
		}

	}
	if (type == ROI_None)return;
	this->mpData->setCurROIType(this->m_CurROIImgType);
	this->mpData->StartROIOperateEventForVolumeChange(type, mImgpty.order);
}

void GLWidget::PushMeasureOperationMsgtoMaindata(E_MeasureOperateType type)
{
	if (type == Measure_None)return;
	this->mpData->setCurROIType(this->m_CurROIImgType);
	this->mpData->StartMeasureOperateEventForChange(type, mImgpty.order);
}



void GLWidget::PushMsgToThreadOremitToRender()
{

#if TranThread
    T_MSG_Value msgvalue;
    msgvalue.msgtype = E_MSG_Type::ImgTransform;
    msgvalue.yoffset = mVertOffset;
    msgvalue.xoffset = mHoriOffset;
    msgvalue.scaleFactor = mScaleFactor;
    emit PushMsg(msgvalue);
#else
    emit renderRequested();
#endif
}

/**********************PRIVATE******************************/
void GLWidget::InitGLRect(int wid,int hgt)
{
    m_arrViewRc[0]=0;
    m_arrViewRc[1]=0;
    m_arrViewRc[2]=wid;
    m_arrViewRc[3]=hgt;

    GLfloat ax,ay;
    float xyRatio =1.0;
    switch(this->mImgpty.imgtype)
    {
    case E_ImageType::Axial:
    {
        ax=mImgpty.width*mDVolumpty->xSpacing;
        ay=mImgpty.height*mDVolumpty->ySpacing;
        xyRatio =mDVolumpty->xSpacing/mDVolumpty->ySpacing;
    }
        break;
    case E_ImageType::Cornal:
    {
        ax=mImgpty.width*mDVolumpty->xSpacing;
        ay=mImgpty.height*mDVolumpty->zSpacing;
        xyRatio =mDVolumpty->xSpacing/mDVolumpty->zSpacing;
    }
        break;
    case E_ImageType::Sagigtal:
    {
        ax=mImgpty.width*mDVolumpty->ySpacing;
        ay=mImgpty.height*mDVolumpty->zSpacing;
        xyRatio =mDVolumpty->ySpacing/mDVolumpty->zSpacing;

    }
        break;
    default:break;

    }
    m_ImgXYRatio =xyRatio;
    GLfloat ViewAspect=((float)m_arrViewRc[bottom])/m_arrViewRc[top];
    GLfloat ImAspect=ax/ay;
    if(ImAspect>ViewAspect)
        ay=ax/ViewAspect;
    else
        ax=ay*ViewAspect;

    m_arrWatchRc[0]=-ax/2;
    m_arrWatchRc[1]=ax/2;
    m_arrWatchRc[2]=-ay/2;
    m_arrWatchRc[3]=ay/2;


    if (ImAspect>ViewAspect)
    {

        m_arrTextRc[0]=-ax/2;
        m_arrTextRc[1]=ax/2;
        m_arrTextRc[2]=-ax/(ImAspect*2);
        m_arrTextRc[3]=ax/(ImAspect*2);
    }
    else
    {

        m_arrTextRc[0]=-ay*ImAspect/2;
        m_arrTextRc[1]=ay*ImAspect/2;
        m_arrTextRc[2]=-ay/2;
        m_arrTextRc[3]=ay/2;
    }

#if NO_USE_AITK
    if(mpImgSegment)
    {
        mpImgSegment->SetTextureRect(m_arrTextRc,4);
    }
#else
	if (mpItkSeg)
	{
		mpItkSeg->SetTextureRect(m_arrTextRc, 4);
	}
#endif
}
void GLWidget::CreateImage()
{

    QString filename =QString("C:/Users/WZG/Pictures/Camera Roll/slice.jpg");
    QImageReader reader(filename);
    reader.setAutoTransform(true);
    if (!reader.canRead()) {
        return ;
    }
    QImage *image= new QImage();
    reader.read(image);


    this->mpCurQImg = image;


}
void GLWidget::PushResetImgSizeLocationMsg()
{

    this->mScaleFactor = 1;
    this->mVertOffset = 0;
    this->mHoriOffset = 0;

    PushMsgToThreadOremitToRender();

}
void GLWidget::zoomIn()
{
    zoom(1 + mZoomDelta);
}
void GLWidget::zoomOut()
{
    zoom(1 - mZoomDelta);
}
void GLWidget::zoom(float scale)
{
    mScaleFactor *= scale;
    PushMsgToThreadOremitToRender();

}
void GLWidget::translate(QPointF delta)
{
    mHoriOffset += delta.x()/2;
    mVertOffset += delta.y()/2;
    PushMsgToThreadOremitToRender();
}
/***************************************************
*Function Describe: process sth like isRoIEnable,isValide Area when mouse press the image,
* and calculate ROI result.
*Name:ProcessMarkActionROIMousePressEvent
*Parameter:QPoint screempt
*Input:screempt,touch pt or mouse press point.
*Output:
*Create:   wangzhigang                Date:2017-10-16
*Note:       Who:          Date:
*1.
*2.
*Modify :    Who:          Date:
*1.
*2.
*************************************************/
void GLWidget::ProcessMarkActionROIMousePressEvent(QPoint screempt)
{

    //enalbe one section like axial,cornal or saggital
    mbROIEnale =  IsROIOperattionEnable();
    if(false == mbROIEnale)
        return;

    //calculate gl point
    QPoint ImPoint(screempt);
    QPointF RealPoint(0,0);
    m_renderer->ProcessOpenglCalculation(&ImPoint,&RealPoint,CAL_SCREENTOGL);

    //evaluate the validArea and do sth to be ready for contour.
    if(GLToImage(RealPoint, ImPoint))
    {
        m_bValidAreaFlag = true;
        PrepareBrushContourWithAlrorithm( RealPoint,m_BrushRadius);
        this->mScreenPt = screempt;
		mCurROIOperaType = ROI_None;
		m_renderer->InitROIMovePt();
        m_renderer->SetROIMovePt(screempt);
        emit renderRequested();
    }
}

void GLWidget::ProcessMarkActionROIMouseMoveEvent(QPoint screempt)
{
    if( false == mbROIEnale)
        return;
    //send pt to thread to process contour
    this->mScreenPt = screempt;
    if(m_bValidAreaFlag ==true)
    {
        m_renderer->SetROIMovePt(screempt);
    }
    emit renderRequested();

}
/***************************************************
*Function Describe: process scroll the image event when mouve move,up to increase,down to descrease
*Name:ProcessMarkActionScrollPageMouseMoveEvent
*Parameter:QPoint screempt
*Input:screempt,touch pt or mouse move point.
*Output:
*Create:   wangzhigang                Date:2017-10-16
*Note:       Who:          Date:
*1.
*2.
*Modify :    Who:          Date:
*1.
*2.
*************************************************/
void GLWidget::ProcessMarkActionScrollPageMouseMoveEvent(QPoint screempt)
{
    T_MSG_Value msgvalue;
    msgvalue.msgtype = E_MSG_Type::ScrollPage;

    QPoint mouseDelta = screempt - m_LastMousePos;
    float dis=mouseDelta.y();

    //construct msgvalue for scroll page
    if(abs(dis)>2)
    {
        m_LastMousePos = screempt;
        if(dis<0)
        {
            msgvalue.ScrollPage = 1;//down
        }
        else
        {
            msgvalue.ScrollPage = -1;//up
        }
        PushViewMsgForUpDateImageData(msgvalue);
    }
}
/***************************************************
*Function Describe: process adjust Window width and Center for image,
* X-axis presents width,y-axis presents center.
*Name:ProcessMarkActionImageWWandWCMouseMoveEvent
*Parameter:QPoint screempt
*Input:screempt,touch pt or mouse move point.
*Output:
*Create:   wangzhigang                Date:2017-10-16
*Note:       Who:          Date:
*1.
*2.
*Modify :    Who:          Date:
*1.
*2.
*************************************************/
void GLWidget::ProcessMarkActionImageWWandWCMouseMoveEvent(QPoint screempt)
{
    T_MSG_Value msgvalue;
    memset(&msgvalue,0,sizeof(T_MSG_Value));
    msgvalue.msgtype = E_MSG_Type::WCSet;

    QPoint mouseDelta = screempt - m_LastMousePos;
    bool needupdate = false;

    float dis=mouseDelta.y();
    if(abs(dis)>2)
    {
        m_LastMousePos = screempt;
        if(dis<0)
        {
            msgvalue.ScrollPage = 1;//present window width adjusting.
        }
        else
        {
           msgvalue.ScrollPage = -1;
        }
       needupdate= true;
    }

    float disx=mouseDelta.x();
    if(abs(disx)>2)
    {
        m_LastMousePos = screempt;
        if(disx<0)
        {
            msgvalue.HorizonScroll = 1;//present window center adjusting.
        }
        else
        {
            msgvalue.HorizonScroll = -1;
        }
        needupdate= true;

    }
    if(true == needupdate)
    {
        PushViewMsgForUpDateImageData(msgvalue);
    }
}

/***************************************************
*Function Describe: in the measure style the mousetouch event
*Name:ProcessMarkActionMeasureToolPressEvent
*Create:   xieyangjie                Date:2018-4-20
*Note:       Who:          Date:
*1.judge wheather to select or to build a measuretool
*2.
*Modify :    Who:          Date:
*1.
*2.
*************************************************/
void GLWidget::ProcessMarkActionMeasurePressEvent(E_MarkAction msg, QPointF pt)
{	
	T_MeasureDataCtrl *tool = m_pMeasuresDataCtrlList->last();
		tool->measureType = msg;
		tool->ImgType = this->mImgpty.imgtype;
		tool->coordlist = SetMeasureToolPoints(msg, pt);//save the glcoors
		tool->sliceIdx = mImgpty.order;
		tool->isShow = true;
		mCurSelectMeasureIdx = m_pMeasuresDataCtrlList->count() - 1;

		//since the measure tool is built,emit msg to make the tool btn up
		T_MSG_Value msgvalue;
		switch (msg)
		{
		case MARK_ANGLE:
			msgvalue.msgtype = MeasureAngel;
			break;
		case MARK_DISTANCE:
			msgvalue.msgtype = MeasureDistance;
			break;
		case MARK_MARK:
			msgvalue.msgtype = MeasureMark;
			break;
		default:
			break;
		}		
		m_sCurMarkAction = MARK_NONE;
		emit PushMsg(msgvalue);
		emit renderRequested();
}

void GLWidget::ProcessMarkActionMeasureMoveEvent(QPoint screempt)
{
	QPointF glpt(0, 0);
	if (m_renderer->ProcessOpenglCalculation(&screempt, &glpt, CAL_SCREENTOGL) == false)
		return;
	//if (GLToImage(glpt, screempt))
	//{
		QList<QPointF>* ptlist = m_pMeasuresDataCtrlList->at(mCurmoveToolIdx)->coordlist;
		if (mMoveStyle == MOVE_TOOL)
		{
			QPointF glLastMousePos(0, 0);
			m_renderer->ProcessOpenglCalculation(&m_LastMousePos, &glLastMousePos, CAL_SCREENTOGL);
			QPointF mouseDelta = glpt - glLastMousePos;
			QPointF curpt;
			for (int i = 0; i < ptlist->count(); i++)
			{
				curpt = ptlist->at(i);
				curpt += mouseDelta;
				ptlist->replace(i, curpt);
			}
		}
		else if (mMoveStyle == MOVE_POINT)
		{
			ptlist->replace(mCurmoveToolPointIdx, glpt);
		}
		emit renderRequested();
	//}
}

QList<QPointF>*  GLWidget::SetMeasureToolPoints(E_MarkAction msg, QPointF pt)
{
	QList<QPointF>* plist = new QList<QPointF>();
	plist->append(pt);
	switch (msg)
	{
	case MARK_ANGLE:
		plist->append(QPointF(pt.x() + 50, pt.y()));
		plist->append(QPointF(pt.x() + 35, pt.y()+35));
		break;
	case MARK_DISTANCE:
		plist->append(QPointF(pt.x() + 50, pt.y()));
		break;
	case MARK_MARK:
		break;
	default:
		break;
	}
	return plist;
}

void GLWidget::FindCurToolIdx(QPointF pt)
{
	if (!m_pMeasuresDataCtrlList->isEmpty())
	{
		double cur_mindis = 10000;
		double temp_dis, temp_dis1;
		for (int i = 0; i < m_pMeasuresDataCtrlList->count(); i++)
		{
			QList<QPointF> * ptlist = m_pMeasuresDataCtrlList->at(i)->coordlist;
			for (int j = 0; j < ptlist->count(); j++)
			{
				temp_dis = mpCalculate->ComputeTwoPointDis(pt, ptlist->at(j));
				if (temp_dis < cur_mindis)
				{
					mCurmoveToolPointIdx = j;
					mCurmoveToolIdx = i;
					cur_mindis = temp_dis;
				}
			}
		}
		
		if (cur_mindis < FindToolThread)
		{
			mMoveStyle = MOVE_POINT;
			emit PushSelectOneToolMsg(mCurmoveToolIdx);
		}
		else
		{	
			mCurmoveToolPointIdx = -1;
			cur_mindis = 10000;
			E_MarkAction curmarkAction;
			for (int i = 0; i < m_pMeasuresDataCtrlList->count(); i++)
			{
				QList<QPointF> * ptlist = m_pMeasuresDataCtrlList->at(i)->coordlist;
				curmarkAction = m_pMeasuresDataCtrlList->at(i)->measureType;
				if (curmarkAction == MARK_ANGLE)
				{
					temp_dis = mpCalculate->ComputePointToLine(pt, ptlist->at(0),ptlist->at(1));
					temp_dis1 = mpCalculate->ComputePointToLine(pt, ptlist->at(0), ptlist->at(2));
					temp_dis = qMin(temp_dis, temp_dis1);
					
				}
				else if (curmarkAction == MARK_DISTANCE)
				{
					temp_dis = mpCalculate->ComputePointToLine(pt, ptlist->at(0), ptlist->at(1));
				}					
				if (temp_dis < cur_mindis)
				{
					mCurmoveToolIdx = i;
					cur_mindis = temp_dis;
				}
			}
			if (cur_mindis < FindToolThread)
			{
				mMoveStyle = MOVE_TOOL;
				emit PushSelectOneToolMsg(mCurmoveToolIdx);
			}else
			{
				mMoveStyle = MOVE_NONE;
			}
		}
	}
}



/***************************************************
*Function Describe: Create toolwdts of glwidget operator
*Name:CreateToolWdt
*Create:   wangzhigang                Date:2018-3-7
*Note:       Who:          Date:
*1.
*2.
*Modify :    Who:          Date:
*1.
*2.
*************************************************/
void GLWidget::CreateToolWdt()
{

	
	E_BTN_SATUS_TYPE btntype[] = { BTN_Pressed ,BTN_Normal ,BTN_Pressed ,BTN_SWitch ,BTN_SWitch };
	T_ToolCellCtrl cellctrl;

	this->SetToolCellCtrls(cellctrl, GL_PlanToolCellCtrls[0]);

	AimToolCellWidget *toolcellWdt = new AimToolCellWidget(this, cellctrl,true);
	toolcellWdt->setObjectName("ToolCellWdt");
	toolcellWdt->setStyleSheet(QString("#ToolCellWdt") + GL_TC_StyleSheet_TextStr[GL_ToolCell_Bg_Style]);
	int size = sizeof(btntype) / sizeof(E_BTN_SATUS_TYPE);
	//void AimToolBtnClick(int idx, E_BTN_SATUS_TYPE btntype);
//	connect(toolcellWdt, &AimToolCellWidget::AimToolBtnClick, this, &GLWidget::ProAimToolBtnClickEvent);
	for (int i = 0; i < size; i++)
		{
			toolcellWdt->SetToolCellBtnType(i, btntype[i]);
		}
		
	toolcellWdt->SetToolBtnsIcon(GL_Tools_CommImgStr, GL_Tools_PressImgStr);
	toolcellWdt->setHidden(true);
	this->mpToolWdts = toolcellWdt;




}
void GLWidget::SetToolCellCtrls(T_ToolCellCtrl & cellctrl, int * content)
{
	cellctrl.cnt = content[TCC_Cnt];
	cellctrl.columns = content[TCC_Cols];
	cellctrl.rows = content[TCC_Rows];

	cellctrl.hastitle = content[TCC_HasTitle];
	cellctrl.index = content[TCC_IDX];
	cellctrl.btnfontsize = content[TCC_BtnFont];

}
void GLWidget::LayoutToolWdt()
{
	int width = this->width();
	int height = this->height();


	const double *plantoolboxsize = GL_ToolCell_Rect[mWdtShowStatus];

	QRect rc(plantoolboxsize[0] * width, plantoolboxsize[1] * height,
		plantoolboxsize[2] * width, plantoolboxsize[3] * height);

	this->mpToolWdts->setGeometry(rc);


}
void GLWidget::SetToolCellRectCtrl(T_ToolRcCtrl & rc, double * rcrate)
{

	int width = this->width();
	int height = this->height();

	rc.offset_x = rcrate[TCRC_Off_X] * width;
	rc.offset_y = rcrate[TCRC_Off_Y] * height;
	rc.space_x = rcrate[TCRC_SP_X] * width;
	rc.space_y = rcrate[TCRC_SP_X] * height;
	rc.width = rcrate[TCRC_Width] * width;
	rc.height = rcrate[TCRC_Height] * height;
}
//*****************************PRIVATE SLOTS***********
void GLWidget::onAboutToResize()
{
    m_renderer->lockRenderer();
    //  qDebug()<<"onAboutToResize\n";
}
void GLWidget::onResized()
{
    m_renderer->unlockRenderer();
    if(mpCurQImg)
    {
        InitGLRect(this->width(),this->height());
        LayoutTextInfoLabels();
		qDebug() << "width " << this->width();
		qDebug() << "height " << this->height();
        mbPaintFlag = true;
    }
}




void GLWidget::onAboutToCompose()
{
    // We are on the gui thread here. Composition is about to
    // begin. Wait until the render thread finishes.
    m_renderer->lockRenderer();
    //  qDebug()<<"onAboutToCompose\n";
}
void GLWidget::onFrameSwapped()
{
    m_renderer->unlockRenderer();
    // Assuming a blocking swap, our animation is driven purely by the
    // vsync in this example.

    if(mbPaintFlag)
    {
        mbPaintFlag = false;
        emit renderRequested();
		LayoutTextInfoLabels();
		//LayoutToolWdt();
		SetTextDirectinImg();
    }
	
}
void GLWidget::grabContext()
{
    m_renderer->lockRenderer();
    QMutexLocker lock(m_renderer->grabMutex());
    context()->moveToThread(m_thread);
    m_renderer->grabCond()->wakeAll();
    m_renderer->unlockRenderer();
}
void  GLWidget::ProAimToolBtnClickEvent(int idx, E_BTN_SATUS_TYPE btntype)
{
	switch (btntype)
	{
	case BTN_Normal:
	{
		ProAimToolNormalTypeBtn(idx);
	}
		break;
	case BTN_Pressed:
	{
		ProAimToolPressedTypeBtn(idx);
	}
		break;
	case BTN_SWitch:
	{
		ProAimToolSwitchTypeBtn(idx);
	}
		break;
	default:
		break;
	}
	


}

void GLWidget::ProPathAddEvent(bool flag)
{
	mIsRenderLine = flag;
	emit renderRequested();
}

void GLWidget::UpdateCoordinateLineShow()
{
	emit renderRequested();
}

void GLWidget::ProAimToolNormalTypeBtn(int idx)
{
	E_GL_ToolCellType type = (E_GL_ToolCellType)idx;
	if (type == GL_TC_Reset)
	{
		PushResetImgSizeLocationMsg();
		this->PushResetImgWCMsg();
	}
	
	
}
void GLWidget::ProAimToolPressedTypeBtn(int idx)
{
	E_GL_ToolCellType type = (E_GL_ToolCellType)idx;
	
	bool sendselectstat = false;
	if (mCurToolPressFunType == type)//the same btn self click
	{
		BTN_PRESS vstaus = PRESS_NONE;
		mpToolWdts->GetToolCellBtnValidStatus(idx, vstaus);
		if (vstaus == PRESS_VALID)
			{
				
				sendselectstat = true;
			}
			
		mCurToolPressFunType = GL_TC_None;
	}
	else//the different btn click
	{
		int oldbtnidx = mCurToolPressFunType;
		mpToolWdts->SetToolCellBtnValidStatus(oldbtnidx, PRESS_NONE);
		mpToolWdts->SetToolCellBtnValidImage(oldbtnidx, PRESS_NONE);
		mCurToolPressFunType = type;
		mpToolWdts->SetToolCellBtnValidStatus(oldbtnidx, PRESS_VALID);
		sendselectstat = true;

	}

	switch (mCurToolPressFunType)
	{
		case GL_TC_Move:
		{
			E_MSG_Type msgtype = TouchValid;
			m_sCurMarkAction = MARK_FREE_HAND;
			this->ProcessToolMsgInGlWidget(msgtype, sendselectstat);
		}break;
		case GL_TC_WC:
		{
		//	m_sCurMarkAction = MARK_WCSET;
				E_MSG_Type msgtype = WCSet;
				this->ProcessToolMsgInGlWidget(msgtype, sendselectstat);
		}
		break;
		case GL_TC_None:
		{
			E_MSG_Type msgtype = PrevPage;
		
			this->ProcessToolMsgInGlWidget(msgtype, true);
		}
		break;


		default:
			break;
	}
	
}
void GLWidget::ProAimToolSwitchTypeBtn(int idx) 
{
	E_GL_ToolCellType type = (E_GL_ToolCellType)idx;
	switch (type)
	{
	
		case GL_TC_MultiView:
		{

			emit SendGLWidgetShowStatusRequest(GL_TC_MultiView);
		}
			break;
		case GL_TC_ChangeView:
		{
			if (mWdtShowStatus == GL_ZoominStates )
			{
				mWdtShowStatus = GL_CommonStates;
			}else{
				mWdtShowStatus = GL_ZoominStates;

			}
			emit SendGLWidgetShowStatusRequest(GL_TC_ChangeView);
		}
			break;
		default:
			break;
	}
}
Renderer::Renderer(GLWidget *w)
    : m_glwidget(w),
      m_bExiting(false)
{
	mpCalculate = new CalculateClass(this);
    m_bRenderFinishFlag = true;
    this->m_ScreenPtList.clear();

}
Renderer::~Renderer()
{
    m_glwidget = NULL;

}
// Some OpenGL implementations have serious issues with compiling and linking
// shaders on multiple threads concurrently. Avoid this.
Q_GLOBAL_STATIC(QMutex, initMutex)
void Renderer::SetupLights(void)
{
    glEnable(GL_BLEND);//active GL_BLEND
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    //::glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    GLfloat ambientLight[] = { 0.0f, 0.0f, 0.0f, 1.0f/*0.0f, 0.0f, 0.0f, 1.0f*/};
    GLfloat diffuseLight[] = { 1.0f, 1.0f, 1.0f, 1.0f};
    //GLfloat specularLight[]= {0.5f, 0.5f, 0.5f, 1.0f};
    GLfloat lightPos0[]     = {600.0f,600.0f,600.0f, 1.0f};
    GLfloat lightPos1[]     = {600.0f,600.0f,-600.0f, 1.0f};
    GLfloat lmodel_ambient[] = {0.4f,0.4f,0.4f,1.0f};
    GLfloat local_view[] = {0.0f};

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    //glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);

    glLightfv(GL_LIGHT1, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuseLight);
    //glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
    glLightfv(GL_LIGHT1, GL_POSITION, lightPos1);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER,local_view);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    GLfloat mat_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat mat_diffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
    GLfloat mat_specular[] = { 0.5f, 0.5f, 0.1f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 100);
}
void Renderer::renderContour()
{

    
    if(m_glwidget->GetCurValidMarkAction() == MARK_ROI)
    {
        if(m_glwidget->IsLeftMousePressed())
        {
			ProcessMovePtsList();
            QPointF glpoint(0,0);
            if( m_ScreenPtList.size()==0)
            {
                QPoint screenpt;
                m_glwidget->GetScreenPtFromMsg(screenpt);
                glpoint = ScreenToGL(screenpt);
                QColor color;
                m_glwidget->GetCurColor(color);
                DrawCircle(color, glpoint);
            }

        }

    }
   


}
void Renderer::RenderImage()
{
    GLfloat *pViewRect = m_glwidget->GetViewPortForRender();

    glPushMatrix();
    glViewport(pViewRect[0],pViewRect[1],pViewRect[2],pViewRect[3]);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    GLfloat *pWatchRect = m_glwidget->GetWatchPortForRender();
    qreal hoffset = m_glwidget->GetWatchHorizontalOffsetForRender();
    qreal voffset = m_glwidget->GetWatchVerticalOffsetForRender();
    qreal scale = m_glwidget->GetWatchScaleFactorForRender();

    glOrtho((pWatchRect[0]-hoffset)/scale,
            (pWatchRect[1]-hoffset)/scale,
            (pWatchRect[3]-voffset)/scale,
            (pWatchRect[2]-voffset)/scale,
            -3000,3000);
    glPopMatrix();

    initializeOpenGLFunctions();

    if(m_glwidget->IsUpdateImgSourceData())
    {
        QImage *mpCurQImg = m_glwidget->GetCurImageforRender();
		if ((mpCurQImg == NULL) || (mpCurQImg->isNull()))
		{
			return;
		}

        QImage GlImage = mpCurQImg->convertToFormat(QImage::Format_RGBA8888);
        glGenTextures(TEXTURE_SIZE,&m_TextureIndex);
        glBindTexture(GL_TEXTURE_2D,m_TextureIndex);
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, GlImage.width(), GlImage.height(), 0,
                      GL_RGBA, GL_UNSIGNED_BYTE, GlImage.bits() );
        m_glwidget->SetUpdateImgSourceData(false);


    }

    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_SMOOTH);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0, 0.0, 0.0, 0.5);
    glClearDepth(1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
    glEnable(GL_LINE_STIPPLE);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    SetupLights();

    glColor4f(1.0f,1.0f,1.0f,1.0f);
    glLineWidth(1.0);

    GLfloat *pTextRect = m_glwidget->GetTextRectforRender();

    glPushMatrix();

    glBindTexture(GL_TEXTURE_2D,m_TextureIndex);
    glBegin( GL_QUADS );

    glTexCoord2f( 0.0, 0.0 ); glVertex3f( pTextRect[0], pTextRect[2],0);
    glTexCoord2f( 1.0, 0.0 ); glVertex3f( pTextRect[1], pTextRect[2],0);
    glTexCoord2f( 1.0, 1.0 ); glVertex3f( pTextRect[1],  pTextRect[3],0);
    glTexCoord2f( 0.0, 1.0 ); glVertex3f( pTextRect[0],  pTextRect[3],0);

    glEnd();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glPopMatrix();
}

//modified by xieyangjie,change the return style from void to bool, if false, the output should not be used
bool Renderer::ProcessOpenglCalculation(void *input,void *ouput,E_CalType type)
{
    if (m_bExiting)
        return false;
    QOpenGLContext *ctx = m_glwidget->context();
    if (!ctx) // QOpenGLWidget not yet initialized
        return false;
    if(ctx->thread() != QThread::currentThread())
    {
        qDebug()<<"ctx->thread()"<<ctx->thread();
        return false;
    }

    m_glwidget->makeCurrent();
    initializeOpenGLFunctions();

    switch(type)
    {
    case CAL_SCREENTOGL:
    {
        QPoint screenpt;
        screenpt.setX(((QPoint*)input)->x());
        screenpt.setY(((QPoint*)input)->y());
        QPointF *pt=(QPointF*)ouput;
        QPointF result =ScreenToGL(screenpt);
        pt->setX(result.x());
        pt->setY(result.y());
    }

        break;
    }

    m_glwidget->doneCurrent();
    ctx->moveToThread(qGuiApp->thread());

    // Schedule composition. Note that this will use QueuedConnection, meaning
    // that update() will be invoked on the gui thread.
    QMetaObject::invokeMethod(m_glwidget, "update");
	return true;
}

void Renderer::render()
{
    if (m_bExiting)
        return;
    if(m_bRenderFinishFlag == false)
        return;

    QOpenGLContext *ctx = m_glwidget->context();
    if (!ctx) // QOpenGLWidget not yet initialized
        return;
    if(ctx->thread() == QThread::currentThread())
    {
        qDebug()<<"ctx->thread()"<<ctx->thread();
        return;
    }

    // Grab the context.
    m_grabMutex.lock();
    emit contextWanted();
    m_grabCond.wait(&m_grabMutex);
    QMutexLocker lock(&m_renderMutex);
    m_grabMutex.unlock();

    if (m_bExiting)
        return;

    Q_ASSERT(ctx->thread() == QThread::currentThread());
    qDebug()<<"ctx->thread()"<<ctx->thread();
    qDebug()<<"QThread::currentThread"<<QThread::currentThread();
    // Make the context (and an offscreen surface) current for this thread. The
    // QOpenGLWidget's fbo is bound in the context.
    m_glwidget->makeCurrent();
    initializeOpenGLFunctions();
    RenderProcess();
    m_glwidget->doneCurrent();
    ctx->moveToThread(qGuiApp->thread());

    // Schedule composition. Note that this will use QueuedConnection, meaning
    // that update() will be invoked on the gui thread.
    QMetaObject::invokeMethod(m_glwidget, "update");
}
void Renderer::RenderProcess()
{
	RenderImage();		
	renderContour();
	DrawContour(); 
	DrawMeasureTool();
	if (m_glwidget->mIsRenderLine)
	{
		DrawPositionLine();
		DrawPuncturePtList();
	}
	DrawToolLine();
}



QPointF Renderer::ScreenToGL(QPoint ScreenP)
{

    GLdouble projMatrix[16];
    GLdouble pos1[3];

    GLint viewport[4];
    GLdouble modelMatrix[16];

    glPushMatrix();
    glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
    glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
    glGetIntegerv(GL_VIEWPORT, viewport);
    glPopMatrix();

    GLfloat  winX,winY1,winZ1;
    winX=ScreenP.x();

    winY1=(float)viewport[3] - (float)(ScreenP.y());

    glReadPixels(int(winX), int(winY1), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ1);
    gluUnProject( (GLdouble)winX, (GLdouble)winY1, (GLdouble)winZ1, modelMatrix,
                  projMatrix, viewport, &pos1[0], &pos1[1], &pos1[2] );  //compute coordinate in this function

    QPointF RealP=QPointF(pos1[0], pos1[1]);

    return RealP;
}

void Renderer::DrawPositionLine()
{

	if (m_glwidget->mIsRenderLine)
	{
		E_ImageType type = m_glwidget->GetCurImageType();
		MainData* pdata = m_glwidget->GetMainData();
		QList<T_LastGLPressPtCtrl*>* pressptlt = pdata->GetGLWdtLatestPressedPt();
		QPoint screenpos;
		quint16 Pos[3] = { 0,0,0 };
		QPointF glp;
		GLfloat* rc = m_glwidget->GetCurArrTextRc();
		GLfloat gwidth = rc[1];
		GLfloat gheight = rc[3];

		if (!pressptlt->isEmpty())
		{
			T_LastGLPressPtCtrl* ptctrl = pressptlt->at(0);
			if (!ptctrl)return;
			Pos[0] = ptctrl->axialorder;
			Pos[1] = ptctrl->coronalorder;
			Pos[2] = ptctrl->sagittalorder;	
		}
		
		quint16 axialpos = Pos[0];
		quint16 coronalpos = Pos[1];
		quint16 sagittalpos = Pos[2];
		if ((axialpos == 0) && (coronalpos == 0) && (sagittalpos == 0))
		{
			glp.setX(0);
			glp.setY(0);
		}

	
		switch (type)
		{
		case Axial:
		{
			if (!pressptlt->isEmpty())
			{

				if (pressptlt->at(0)->imagetype == m_glwidget->GetCurImageType())
				{
					screenpos = pressptlt->at(0)->pos;
					glp = ScreenToGL(screenpos);
				}
				else
				{
					T_DImgPropety pty = m_glwidget->GetImageSize();
					quint16 width = pty.width;
					quint16 height = pty.height;

					GLfloat c_rate = coronalpos*1.0 / height;
					GLfloat s_rate = sagittalpos*1.0 / width;

					glp.setX((s_rate - 0.5)*gwidth * 2);
					glp.setY((c_rate - 0.5)*gheight * 2);
				}

			}

		}break;
		case Cornal:
		{
			if (!pressptlt->isEmpty())
			{
				if (pressptlt->at(0)->imagetype == m_glwidget->GetCurImageType())
				{
					screenpos = pressptlt->at(0)->pos;
					glp = ScreenToGL(screenpos);
				}
				else
				{
					T_DImgPropety pty = m_glwidget->GetImageSize();
					quint16 width = pty.width;
					quint16 height = pty.height;

					GLfloat a_rate = (height - axialpos)*1.0 / height;
					GLfloat s_rate = sagittalpos*1.0 / width;

					glp.setX((s_rate - 0.5)*gwidth * 2);
					glp.setY((a_rate - 0.5)*gheight * 2);
				}
			}

		}break;
		case Sagigtal:
		{
			if (!pressptlt->isEmpty())
			{
				if (pressptlt->at(0)->imagetype == m_glwidget->GetCurImageType())
				{
					screenpos = pressptlt->at(0)->pos;
					glp = ScreenToGL(screenpos);
				}
				else
				{
					T_DImgPropety pty = m_glwidget->GetImageSize();
					quint16 width = pty.width;
					quint16 height = pty.height;

					GLfloat a_rate = (height - axialpos)*1.0 / height;
					GLfloat c_rate = (coronalpos)*1.0 / width;

					glp.setX((c_rate - 0.5)*gwidth * 2);
					glp.setY((a_rate - 0.5)*gheight * 2);
				}
			}
		

		}break;
		
		default:
			break;
		}

		QColor mainlinecolor = QColor(0, 125, 194);
		DrawCircle(mainlinecolor, glp);
		DrawCoordinateLine(m_glwidget->width(), m_glwidget->height(), glp, type);
		mCurLineCenter = glp;
		
	}

}

void Renderer::DrawAgsLine(int width, int height, int x, int y, QColor color) //not use
{
	QColor markColor = color;
	markColor.setAlphaF(0.3);
	glColor3f(markColor.red(), markColor.green(), markColor.blue());
	glLineWidth(1);
	glLineStipple(1, 0xFFFF);

	glBegin(GL_LINES);
	glVertex2f(-width, y);
	glVertex2f(width, y);
	glEnd();

	glBegin(GL_LINES);
	glVertex2f(x, height);
	glVertex2f(x, -height);
	glEnd();
}


void Renderer::DrawToolLine()
{
	if (m_glwidget->mIsRenderTool == true)
	{
		double ax = m_glwidget->mCurTip.x();
		double ay = m_glwidget->mCurTip.y();
		double bx = m_glwidget->mCurMid.x();
		double by = m_glwidget->mCurMid.y();

		double DistOfab = sqrt((ax - bx)*(ax - bx) + (ay - by)*(ay - by));
		double DistOfbc = 50.0;
		double cx = ax + DistOfbc / DistOfab*(ax - bx);
		double cy = ay + DistOfbc / DistOfab*(ay - by);


		QColor ToolColor = QColor(255, 0, 0);
		QColor ExtensionColor = QColor(0, 255, 0);
		ToolColor.setAlphaF(1);
		glColor3f(ToolColor.red(), ToolColor.green(), ToolColor.blue());
		glLineWidth(2);
		glLineStipple(1, 0xFFFF);
		glBegin(GL_LINES);
		glVertex2f(m_glwidget->mCurTip.x(), m_glwidget->mCurTip.y());
		glVertex2f(m_glwidget->mCurMid.x(), m_glwidget->mCurMid.y());
		glEnd();

		ExtensionColor.setAlphaF(1);
		glColor3f(ExtensionColor.red(), ExtensionColor.green(), ExtensionColor.blue());
		glLineWidth(2);
		glLineStipple(1, 0xF0F0);
		glBegin(GL_LINES);
		glVertex2f(m_glwidget->mCurTip.x(), m_glwidget->mCurTip.y());
		glVertex2f(cx, cy);
		glEnd();
	}

}

void Renderer::DrawPuncturePtList()
{
	MainData* pdata = m_glwidget->GetMainData();
	QList<T_RobotPuncturePathCtrl*>* pathctrllist = pdata->GetRobotPuncturePath();
	if (pathctrllist->isEmpty())return;
	int size = pathctrllist->size();
	for (int i = 0; i < size; i++)
	{
		T_RobotPuncturePathCtrl* pathctrl = pathctrllist->at(i);
		if (pathctrl->isSetStart)
		{
			DrawPuncturePt(pathctrl->pStartPoint, 0);
		}
		if (pathctrl->isSetTarget)
		{
			DrawPuncturePt(pathctrl->pTargetPoint, 1);
		}
		if ((pathctrl->isSetStart) && (pathctrl->isSetTarget))
		{
			//连成穿刺路径
			QPointF start_gl_pt = PuncturePtToGL(pathctrl->pStartPoint);
			QPointF target_gl_pt = PuncturePtToGL(pathctrl->pTargetPoint);
			glBegin(GL_LINES);
			glVertex2f(start_gl_pt.x(), start_gl_pt.y());
			glVertex2f(target_gl_pt.x(), target_gl_pt.y());
			glEnd();
		}

	}
}



void Renderer::DrawCoordinateLine(quint16 width, quint16 height, QPointF point, E_ImageType imageType)
{
	QColor markColor;
	QColor nextColor;
	switch (imageType)
	{
	case Axial:
	{
		markColor = QColor(255, 0, 0); //red
		nextColor = QColor(0, 255, 0); //blue
	}break;
	case Cornal:
	{
		markColor = QColor(255, 215, 0); //yellow
		nextColor = QColor(0, 255, 0); //blue
	}break;
	case Sagigtal:
	{
		markColor = QColor(255, 215, 0); //yellow
		nextColor = QColor(255, 0, 0); //red
	}break;
	default:
		break;
	}

	markColor.setAlphaF(0.6);
	glColor3f(markColor.red(), markColor.green(), markColor.blue());
	glLineWidth(1.1);
	glLineStipple(1, 0xFFFF);
	glBegin(GL_LINES);
	glVertex2f(point.x() - width, point.y());
	glVertex2f(point.x() + width, point.y());
	glEnd();

	nextColor.setAlphaF(0.6);
	glColor3f(nextColor.red(), nextColor.green(), nextColor.blue());
	glLineWidth(1.1);
	glLineStipple(1, 0xFFFF);
	glBegin(GL_LINES);
	glVertex2f(point.x(), point.y() - height);
	glVertex2f(point.x(), point.y() + height);
	glEnd();
	
}

void Renderer::DrawPuncturePt(T_RobotPuncturePtCtrl * PtCtrl,bool flag)
{
	QColor color;
	if (flag)
	{
		color = QColor(255, 0, 0);
	}
	else
	{
		color = QColor(0, 255, 0);
	}
	QPointF glp = PuncturePtToGL(PtCtrl);
	
	DrawCircle(color, glp);
}

QPointF Renderer::ImageToGL(QPoint point)
{
	QPointF glp;
	T_DImgPropety pty = m_glwidget->GetImageSize();
	quint16 width = pty.width;
	quint16 height = pty.height;
	GLfloat* rc = m_glwidget->GetCurArrTextRc();
	GLfloat gwidth = rc[1];
	GLfloat gheight = rc[3];
	E_ImageType imgtype = m_glwidget->GetCurImageType();
	switch (imgtype)
	{
	case Axial:
	{
		GLfloat c_rate = point.x()*1.0 / height;
		GLfloat s_rate = point.y()*1.0 / width;
		glp.setX((s_rate - 0.5)*gwidth * 2);
		glp.setY((c_rate - 0.5)*gheight * 2);
	}break;
	case Cornal:
	{
		GLfloat a_rate = 1 - point.x()*1.0 / height;
		GLfloat s_rate = point.y()*1.0 / width;

		glp.setX((s_rate - 0.5)*gwidth * 2);
		glp.setY((a_rate - 0.5)*gheight * 2);
	}break;
	case Sagigtal:
	{
		GLfloat a_rate = 1 - point.x()*1.0 / height;
		GLfloat c_rate = point.y()*1.0 / width;

		glp.setX((c_rate - 0.5)*gwidth * 2);
		glp.setY((a_rate - 0.5)*gheight * 2);
	}break;
	default:
		break;
	}
	
	return glp;
}

QPointF Renderer::PuncturePtToGL(T_RobotPuncturePtCtrl * PtCtrl)
{
	QList<int>imgpos = PtCtrl->mpoint;
	E_ImageType type = m_glwidget->GetCurImageType();
	QPoint imgp;
	QPointF glp;
	QPoint screenpos = PtCtrl->screenPt;
	T_DImgPropety pty = m_glwidget->GetImageSize();
	switch (type)
	{
	case Axial:
	{
		if (PtCtrl->imageType == Axial)
		{
			glp = ScreenToGL(screenpos);
		}
		else
		{
			imgp.setX(imgpos[1]);
			imgp.setY(imgpos[0]);
			glp = ImageToGL(imgp);
		}
	}
	break;
	case Cornal:
	{
		if (PtCtrl->imageType == Cornal)
		{
			glp = ScreenToGL(screenpos);
		}
		else
		{
			imgp.setX(imgpos[2]);
			imgp.setY(imgpos[0]);
			glp = ImageToGL(imgp);
		}

	}
	break;
	case Sagigtal:
	{
		if (PtCtrl->imageType == Sagigtal)
		{

			glp = ScreenToGL(screenpos);

		}
		else
		{
			imgp.setX(imgpos[2]);
			imgp.setY(imgpos[1]);
			glp = ImageToGL(imgp);
		}

	}
	break;

	default:
		break;
	}
	return glp;
}


void Renderer::DrawContour(QList<T_Contour> *plist,QColor color)
{	
	if (plist == NULL)return;
	if (!plist->isEmpty())
	{
		//is the roi in current image index;
		int sliceindex = m_glwidget->GetCurSliceindex();
		const T_Contour *pcontour = NULL;
		bool isExist = false;
		for (int i = 0; i<plist->count(); i++)
		{
			pcontour = &(plist->at(i));
			if (pcontour->SliceIndex == sliceindex)
			{
				isExist = true;
				break;
			}
		}
		if (isExist == false)
		{
			return;
		}		
		glColor3f(color.redF(), color.greenF(), color.blueF());
		if (pcontour->pTextPtList.count()>1)
		{
			for (int k = 0; k<pcontour->pTextPtList.count() - 1; k++)
			{
				glLineStipple(1, 0xFFFF);
				glBegin(GL_LINES);
				glVertex2f(pcontour->pTextPtList[k].x(), pcontour->pTextPtList[k].y());
				glVertex2f(pcontour->pTextPtList[k + 1].x(), pcontour->pTextPtList[k + 1].y());
				glEnd();
			}
		}
		// Link the first point and last
		if (pcontour->pTextPtList.count()>2)
		{
			glLineStipple(1, 0xFFFF);
			glBegin(GL_LINES);
			glVertex2f(pcontour->pTextPtList[0].x(), pcontour->pTextPtList[0].y());
			glVertex2f(pcontour->pTextPtList[pcontour->pTextPtList.count() - 1].x(),
				pcontour->pTextPtList[pcontour->pTextPtList.count() - 1].y());
			glEnd();
		}
	}
}

void Renderer::DrawMeasureTool()
{
	QList<T_MeasureDataCtrl*>*pMeasureTools = m_glwidget->GetMeasureToolsList();
	if (pMeasureTools == NULL)
		return;
	int sliceindex = m_glwidget->GetCurSliceindex();
	int size = pMeasureTools->size();
	for (int i = 0; i < size; i++)
	{
		T_MeasureDataCtrl *pDatactrl = pMeasureTools->at(i);
		if (pDatactrl->sliceIdx == sliceindex)
		{
			if (pDatactrl->isShow)
			{
				DrawMeasureTool(pDatactrl,i);
			}
		}
	}
}

void Renderer::DrawMeasureTool(T_MeasureDataCtrl * pDatactrl,int index)
{
	const  int *pcolor = SRW_ColorBar[pDatactrl->colororder];
	QColor color = QColor(pcolor[Aim_RED], pcolor[Aim_GREEN], pcolor[Aim_BLUE]);
	int num = 20;

	//draw tool
	if (m_glwidget->mMoveStyle != MOVE_NONE && index== m_glwidget->mCurmoveToolIdx)
	{
		glColor3f(1 ,1, 1);
	}
	else
	{
		glColor3f(1, 1, 1);//如果有需要用不同的颜色显示，再改
	}
		//glHint(GL_LINE_SMOOTH, GL_NICEST);//告诉opengl以显示效果为重，速度不重要
		glEnable(GL_LINE_SMOOTH);
		if (MARK_ANGLE == pDatactrl->measureType)
		{
			QPointF p1 = pDatactrl->coordlist->at(0);
			QPointF p2 = pDatactrl->coordlist->at(1);
			QPointF p3 = pDatactrl->coordlist->at(2);
			glLineStipple(1, 0xFFFF);
			glBegin(GL_LINE_STRIP);
			glVertex2f(p2.x(), p2.y());
			glVertex2f(p1.x(), p1.y());
			glVertex2f(p3.x(), p3.y());
			glEnd();			
		}
		else if (MARK_DISTANCE == pDatactrl->measureType)
		{
			QPointF p1 = pDatactrl->coordlist->at(0);
			QPointF p2 = pDatactrl->coordlist->at(1);
			glLineStipple(1, 0xFFFF);
			glBegin(GL_LINE_LOOP);
			glVertex2f(p2.x(), p2.y());
			glVertex2f(p1.x(), p1.y());
			glEnd();
		}
 
		//draw point
		int radius;
		for (int i = 0; i < pDatactrl->coordlist->count(); i++)
		{
			QPointF point = pDatactrl->coordlist->at(i);
			glLineStipple(1, 0xFFFF);			
			if(m_glwidget->mCurmoveToolPointIdx==i && index == m_glwidget->mCurmoveToolIdx)
			{
				radius = 6;
				glColor3f(1, 1, 1);
			}else{
				radius = 3;
				glColor3f(0.6, 0.6, 0.6);
			}
			glLineWidth(1);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glBegin(GL_POLYGON);
			for (int i = 0; i<num; i++)
				glVertex2f(radius*sin((float)i / (float)num*2.0*qAcos(-1)) + point.x(),
					radius*cos((float)i / (float)num*2.0*qAcos(-1)) + point.y());
			glEnd();
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		QList<QPointF> plist; 
		plist.append(QPointF(0, 1));
		plist.append(QPointF(0, -1));
		plist.append(QPointF(1, 0));
		plist.append(QPointF(-1, 0));
		if (MARK_MARK == pDatactrl->measureType)
		{			
			QPointF pt = pDatactrl->coordlist->at(0);
			QPointF p1, p2;

			glColor3f(0.46, 0.77, 0.94);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glBegin(GL_POLYGON);
			for (int i = 0; i<4; i++)
				glVertex2f(0.2*sin((float)i / (float)4*2.0*qAcos(-1)) + pt.x(),
					0.2*cos((float)i / (float)4*2.0*qAcos(-1)) + pt.y());
			glEnd();
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			for (int i = 0; i < plist.count(); i++)
			{
				p1 = pt + radius*plist.at(i) / 2;
				p2=pt+ radius*plist.at(i)*3 / 2;
				glColor3f(0.46, 0.77, 0.94);
				glLineStipple(1, 0xFFFF);
				glBegin(GL_LINE_LOOP);
				glVertex2f(p2.x(), p2.y());
				glVertex2f(p1.x(), p1.y());
				glEnd();
			}
		}

		//draw information
		//first need to init
		QString s = QString("");
		char*  ch;
		QByteArray ba = s.toLatin1();
		ch = ba.data();
		DrawString(ch, QPointF(0, 0));
		DrawMeasureToolInfo(pDatactrl);
}

void Renderer::DrawMeasureToolInfo(T_MeasureDataCtrl * pDatactrl)
{
	glColor3f(1, 1, 1);
	switch (pDatactrl->measureType)
	{
	case E_MarkAction::MARK_ANGLE:
	{
		double angle = mpCalculate->ComputeAngle(pDatactrl->coordlist );
		double radius = mpCalculate->ComputeHalfofShorter(pDatactrl->coordlist);
		double pi = qAcos(-1);
		QPointF pt;
		if (qCos(angle / 2) != 0)
		{
			double cosa = qCos(angle / 2);
			double length = double(radius / qAbs(cosa));
			double l1 = mpCalculate->ComputeTwoPointDis(pDatactrl->coordlist->at(0), pDatactrl->coordlist->at(1));
			double l2 = mpCalculate->ComputeTwoPointDis(pDatactrl->coordlist->at(0), pDatactrl->coordlist->at(2));
			QPointF pt1 = QPointF(pDatactrl->coordlist->at(0) + length*(pDatactrl->coordlist->at(1) - pDatactrl->coordlist->at(0)) / l1);
			QPointF pt2 = QPointF(pDatactrl->coordlist->at(0) + length*(pDatactrl->coordlist->at(2) - pDatactrl->coordlist->at(0)) / l2);
			pt = (pt1 + pt2) / 2;
		}
		else
		{
			double l1 = mpCalculate->ComputeTwoPointDis(pDatactrl->coordlist->at(0), pDatactrl->coordlist->at(1));
			QPointF pt1 = QPointF(pDatactrl->coordlist->at(0) + radius*(pDatactrl->coordlist->at(1) - pDatactrl->coordlist->at(0)) / l1);
			pt =QPointF( pDatactrl->coordlist->at(1).y(), -pDatactrl->coordlist->at(1).x());
		}
		QPointF pt3 = QPointF(pDatactrl->coordlist->at(0) + (5 + radius)*(pt - pDatactrl->coordlist->at(0)) / radius);
		QString	s = QString::number(angle * 180 / pi, 'f', 2) + QString("°");
			char*  ch;
			QByteArray ba = s.toLatin1();
			ch = ba.data();
			DrawString(ch, pt3);

			//画弧线
			QList<QPointF> ptlist;
			ptlist.append(pDatactrl->coordlist->at(0));
			ptlist.append(pDatactrl->coordlist->at(0) + QPointF(0, 5));
			ptlist.append(pt);
			double anglez = mpCalculate->ComputeAngle(&ptlist);
			if (pt.x() - pDatactrl->coordlist->at(0).x() < 0)
				anglez = 2 * qAcos(-1) - anglez;
			double anglestart = anglez - angle / 2;
			double angleend = anglez + angle / 2;
			double delta = qAcos(-1) / 60;
			glColor3f(100, 100, 100);
			glLineStipple(1, 0xFFFF);
			glEnable(GL_LINE_SMOOTH);
			glBegin(GL_LINE_STRIP);
			for (double alfa = anglestart; alfa < angleend; alfa += delta)
			{
				glVertex2f(radius*sin(alfa) + pDatactrl->coordlist->at(0).x(),
					radius*cos(alfa) + pDatactrl->coordlist->at(0).y());
			}
			glVertex2f(radius*sin(angleend) + pDatactrl->coordlist->at(0).x(),
				radius*cos(angleend) + pDatactrl->coordlist->at(0).y());
			glEnd();
		
			

	}break;
	case E_MarkAction::MARK_DISTANCE:
	{
		double distance = mpCalculate->ComputeTwoPointDis(pDatactrl->coordlist->at(0), pDatactrl->coordlist->at(1));
		QPointF pt = (pDatactrl->coordlist->at(0) + pDatactrl->coordlist->at(1)) / 2;
		QString s = QString::number(distance,'f',2) + QString("mm");
		char*  ch;
		QByteArray ba = s.toLatin1();
		ch = ba.data();
		//ch = s.data();
		DrawString(ch, QPointF(pt.x(),pt.y()-3));
	}break;
	case E_MarkAction::MARK_MARK:
	{
		//如果需要加上这个点的坐标，再改
	}break;
	default:
		break;
	}
}

void Renderer::DrawString(char * str,QPointF pt)
{
	glPushAttrib(GL_LIST_BIT);
	glRasterPos2f(pt.x(), pt.y());
	wglUseFontBitmaps(wglGetCurrentDC(), 0, 256, 1000);
	glListBase(1000);
	glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);
	glPopAttrib();
}




void Renderer::DrawContour()
{
	QList<T_ROIDataCtrl*>*pROIsDatactrl = m_glwidget->GetROIsList();
	if (pROIsDatactrl == NULL)
		return;
	E_ImageType imgtype = m_glwidget->GetCurImageType();
	int size = pROIsDatactrl->size();
	for (int i = 0; i < size; i++)
	{
		T_ROIDataCtrl *pDatactrl = pROIsDatactrl->at(i);
		if (pDatactrl->ImgType == imgtype)
		{
			if (pDatactrl->isShow)
			{
				QList<T_Contour> *plist = pDatactrl->pContourList;
				const  int *pcolor = SRW_ColorBar[pDatactrl->colororder];

				QColor roicolor = QColor(pcolor[Aim_RED], pcolor[Aim_GREEN], pcolor[Aim_BLUE]);
		
				DrawContour(plist, roicolor);
			}
				
		}
	}

}
//void Renderer::DrawContour(QList<T_Contour> *plist, QColor color)
//{
//	QList<T_Contour> *pConList = m_glwidget->GetContourList();
//	if (pConList == NULL)return;
//
//	if (!pConList->isEmpty())
//	{
//		int sliceindex = m_glwidget->GetCurSliceindex();
//		const T_Contour *pcontour = NULL;
//		bool isExist = false;
//		for (int i = 0; i<pConList->count(); i++)
//		{
//
//			pcontour = &(pConList->at(i));
//
//			if (pcontour->SliceIndex == sliceindex)
//			{
//				isExist = true;
//				break;
//			}
//		}
//
//		if (isExist == false)
//
//			return;
//
//		QColor color;
//		m_glwidget->GetCurColor(color);
//		float red = color.redF();
//		glColor3f(color.redF(), color.greenF(), color.blueF());
//		//	glColor3f(203, 42, 39);
//		if (pcontour->pTextPtList.count()>1)
//		{
//			for (int k = 0; k<pcontour->pTextPtList.count() - 1; k++)
//			{
//				glLineStipple(1, 0xFFFF);
//				glBegin(GL_LINES);
//				glVertex2f(pcontour->pTextPtList[k].x(), pcontour->pTextPtList[k].y());
//				glVertex2f(pcontour->pTextPtList[k + 1].x(), pcontour->pTextPtList[k + 1].y());
//				glEnd();
//
//			}
//		}
//		// Link the first point and last
//		if (pcontour->pTextPtList.count()>2)
//		{
//			glLineStipple(1, 0xFFFF);
//			glBegin(GL_LINES);
//			glVertex2f(pcontour->pTextPtList[0].x(), pcontour->pTextPtList[0].y());
//			glVertex2f(pcontour->pTextPtList[pcontour->pTextPtList.count() - 1].x(),
//				pcontour->pTextPtList[pcontour->pTextPtList.count() - 1].y());
//			glEnd();
//		}
//	}
//
//}
void Renderer::DrawCircle(QColor color, QPointF point)
{
    float radius = m_glwidget->GetCalculatedRadius();
    GLfloat   axfactor=1.0;
    GLfloat   ayfactor=1.0;
    m_glwidget->GetImageXYfactor(axfactor,ayfactor);

    color.setAlphaF(0.3);
    glColor3f(color.red(),color.green(),color.blue());

    glLineWidth(1);
    glLineStipple(1,0xFFFF);
    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);

    glBegin(GL_POLYGON);
    quint8 n=20;
    for (int i=0; i<n; i++)
        glVertex2f( radius*axfactor*sin((float)i/(float)n*2.0*qAcos(-1))+point.x(),
                    radius*axfactor*cos((float)i/(float)n*2.0*qAcos(-1))+point.y());
    glEnd();

    glBegin(GL_LINES);
    glVertex2f(point.x()-radius*axfactor,point.y());
    glVertex2f(point.x()+radius*axfactor,point.y());
    glVertex2f(point.x(),point.y()-radius*axfactor);
    glVertex2f(point.x(),point.y()+radius*axfactor);
    glEnd();
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
}
void Renderer ::ProcessMovePtsList()
{
    if(m_bRenderFinishFlag == true)
    {
        m_bRenderFinishFlag = false;

        //process some pts and delete screenptlist
        int size = m_ScreenPtList.size();

        if(size ==0)
        {
            m_bRenderFinishFlag = true;
            return;
        }
        //qDebug()<<"ScreenPtList size is"<<size;
        QList<QPoint>NeedPtList;
        int i =0;
        for(;i<size;i++)
        {

            NeedPtList<<m_ScreenPtList.at(i);

        }


        //get the gl pts of needptlist
        if (m_bExiting)
            return;
        QPointF RealPoint(0,0);
        QList<QPointF>NeedglPts;

        for(int i=0;i<NeedPtList.size();i++)
        {

            RealPoint= ScreenToGL(NeedPtList.at(i));

            NeedglPts<<RealPoint;
        }

        m_glwidget->CombineBrushContour(&NeedglPts);

        while(size>0)
        {
            if(!m_ScreenPtList.isEmpty())
            {
                m_ScreenPtList.removeFirst();
            }
            size--;
        }
        m_bRenderFinishFlag = true;

    }
}

void Renderer::SetROIMovePt(QPoint spt)
{
    m_ScreenPtList<<spt;
}

void Renderer::InitROIMovePt()
{
	m_ScreenPtList.clear();
}