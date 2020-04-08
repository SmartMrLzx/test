/***************************************************
brief Describe:
*GLWidget.h can Render the image using Opengl,and support zoom,
* translate,ROI operation,which should be supported by opengl and
* imagesegment.
*
*
*Create:    wangzhigang   Date:2017-09-10
*Note:       Who:          Date:
*1.
*2.
*Modify :    Who:          Date:
*1.Adding function private:InitBrushPts and					
*  pointer variable mpItkSeg					yangzefu	2018.05.04
*2.Adding member variable mBrushEdgePtList、mBrushInnerPtList、mBrushPtList
*  to replace mBrushCirclePtList				yangzefu	2018.05.04
*3.Modify function PrepareBrushContourWithAlrorithm：modify the coordinates
*  of the brush according to m_ImgXYRatio		yangzefu	2018.05.06
*4.Adding function private:RefineBrushEdge		yangzefu	2018.05.07
*
*************************************************/

#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLPaintDevice>
#include <QOpenGLTexture>

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QMouseEvent>

#include <QLabel>
#include <QPushButton>
#include<QPainter>
#include <QStyleOption>

#include "datatypedef.h"

#include "Algorithm/imagesegment.h"
#include "Algorithm\AItkImgSeg.h"
#include "DCMProcess\maindata.h"
#include "ToolWidget\AimToolCellWidget.h"
#include "ImageWidget\AimGlWidgetConfig.h"
#include "Algorithm\CalculateClass.h"
#include "qpoint.h"
#include "qstring.h"

class GLWidget;//just for render

#define TEXTURE_SIZE 3 //texture handle size


enum E_TEXTINFO_IDX{
    INFO_TYPE =0,
    INFO_WINWIDTH,
    INFO_WINCENTER,
    INFO_ORDER
};
class Renderer : public QObject, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    Renderer(GLWidget *w);
    ~Renderer();

    void lockRenderer() { m_renderMutex.lock(); }
    void unlockRenderer() { m_renderMutex.unlock(); }
    QMutex *grabMutex() { return &m_grabMutex; }
    QWaitCondition *grabCond() { return &m_grabCond; }
    void prepareExit() { m_bExiting = true; m_grabCond.wakeAll(); }

public:
    bool ProcessOpenglCalculation(void *input,void *ouput,E_CalType type);   
    void SetROIMovePt(QPoint spt );
	void InitROIMovePt();

    QPointF ScreenToGL(QPoint ScreenP);

protected:
	//zlx
	void DrawPositionLine();
	void DrawAgsLine(int width, int height, int x, int y,QColor color);
	void DrawPuncturePtList();
	void DrawCoordinateLine(quint16 width, quint16 height, QPointF point, E_ImageType imageType);
	void DrawPuncturePt(T_RobotPuncturePtCtrl* PtCtrl, bool flag);
	QPointF ImageToGL(QPoint point);
	QPointF PuncturePtToGL(T_RobotPuncturePtCtrl * PtCtrl);
	void DrawToolLine();
protected:
    void ProcessMovePtsList();
    void SetupLights();

    void DrawCircle(QColor color, QPointF point);
    void DrawContour();
	void DrawContour(QList<T_Contour> *plist,QColor color);
	//add by xieyangjie
	void DrawMeasureTool();
	void DrawMeasureTool(T_MeasureDataCtrl *pDatactrl,int index);
	void DrawMeasureToolInfo(T_MeasureDataCtrl *pDatactrl);
	void DrawString(char* str, QPointF pt);
	

private:
    void RenderImage();
    void RenderProcess();
    void renderContour();


signals:
    void contextWanted();

public slots:
    void render();

private:

    GLWidget *m_glwidget;
	CalculateClass *mpCalculate;
    QMutex m_renderMutex;
    QElapsedTimer m_elapsed;
    QMutex m_grabMutex;
    QWaitCondition m_grabCond;
    bool m_bExiting;
	

    GLuint m_TextureIndex;
    QPainter m_Painter;
    QList<QPoint>m_ScreenPtList;
    bool m_bRenderFinishFlag;
	//zlx
	QPointF mCurLineCenter;
};

class GLWidget : public QOpenGLWidget
{
    //class life control functions,etc.
    Q_OBJECT
public:

    explicit GLWidget(QWidget *parent = 0,quint8 index =0);
    ~GLWidget();

protected:

    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *)Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event)Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event)Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event)Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *event)Q_DECL_OVERRIDE;
    bool event(QEvent *event);
	void closeEvent(QCloseEvent *event);

    //custom functions
public:

    //set the data pointer from maindata
    void SetMainData(MainData*pdata = 0);
    void setImageProperty(T_DImgPropety *pty,E_IMG_Multi_Show_Mode *pshowmode);
	void SetMeasureLists();
	void SetROILists();
	void SetCurROIStatusMode(E_ROI_Status_Mode mode);
	void MakeSureTheROIImageType();
	void SetCurROIContourListAndImgType(bool isSelected, bool colorchange=false);
    void setDCMVolumePty(DICOMPropertyData *pty,T_DImageWindowCtrl *wc);

    //set textinfo status and flag
    void SetTextInfoHide(QList<bool>&hideList);
    //set image data status
    void SetUpdateImgSourceData(bool flag);
	static void InitMakrActions();

	void SetRenderToolFlag(bool flag);
	void SetToolShow(QPoint tip, QPoint mid);
public:
    //get image data ,view rect ,zoom ,translate,scale facotors
    QImage *GetCurImageforRender();

    GLfloat *GetTextRectforRender();
    GLfloat *GetViewPortForRender();
    GLfloat *GetWatchPortForRender();

    qreal GetWatchHorizontalOffsetForRender();
    qreal GetWatchVerticalOffsetForRender();
    qreal GetWatchScaleFactorForRender();

    //get operation mark action type
    E_MarkAction GetCurValidMarkAction();
    void GetCurColor(QColor &cr);

    //judge the touch or update status
    static bool IsTouchOperating();
    bool IsUpdateImgSourceData();
    bool IsLeftMousePressed();

    //get ROI contour ,glpt,screempt, Radius
    QList<T_Contour> *GetContourList();
    float GetCalculatedRadius();
    void GetScreenPtFromMsg(QPoint &pt);
	QList<T_ROIDataCtrl*> * GetROIsList();
	QList<T_MeasureDataCtrl*>* GetMeasureToolsList();
	void UpdateROIStatus();

    //get Image info
    int GetCurSliceindex();
    void GetImageXYfactor(GLfloat &rAxfactor,GLfloat &rAyfactor);
	E_ImageType GetCurImageType();


    //update image data and text
    E_ErrorCode UpdateImage(quint8 *pdata);
    void UpdateTextInfo();

    //update translate zoom
    void UpdateTranslationForImage(T_MSG_Value v);
    void UpdateROIForImage(T_MSG_Value v);

    // process msg from toolview btns
    void ProcessToolMsgInGlWidget(E_MSG_Type msg);
    void ProcessToolMsgInGlWidget(E_MSG_Type msg,bool flag);

    //ROI Calculate
    void PrepareBrushContourWithAlrorithm(QPointF point,quint16 radius);
    void CombineBrushContour(QList<QPointF> *point);
    bool GLToImage(QPointF RealP, QPoint &ImPoint);
    static E_ImageType CurROIImgType();
    const T_Contour*GetCurImageContour();
	void InitBrushPts(quint8 radius);
	void RefineBrushEdge();

	//toolcell
	void SetToolcellWdtHiden(bool flag);
	void SetToolCellWdtCommStatus(int idx);
	void SetTextInfoLocationMode(E_GL_WdtShowStats showstatus);
	E_GL_WdtShowStats GetTextInfoLactionMode();

	//zlx
	GLfloat* GetCurArrTextRc();
	QPoint GetLastPressPos();
	T_DImgPropety GetImageSize();
	MainData* GetMainData();
	void ProSliceTranslateEvent(QPoint point);
	void DrawSuccessPuncturePt();

protected:
	int* GetSelectDragLineSliceIdx(E_ImageType type, int idx);

protected:
    //init opengl thread and render
     void InitRenderThread();
    //setting,location,show text info.
     void InitDrawTextInfo();
	 QLabel* CreateTextlabel(int fontsize, E_FontType ftype, E_GL_Info_TextColorOrder color);
    
	 void SetTextsInfo();
     void SetTextInfoImagetype();
     void SetTextInfoOrder();
     void SetTextInfoWindowWidth();
     void SetTextInfoWindowCenter();
     void SetTextInfLocation();

     void LayoutTextInfoLabels();
	 void LayoutTextInfoInSingleViewMode();
	 void LayouTextInfoInMultiViewMode();
	 void SetLabelTextInfo(int idx);
	 void SetTextInfo(E_GL_TextInfo idx);
	 void SetTextImagetypeAndOrder();//imagetype and series order
	 void SetTextWindowWidthCenter();//window center and width
	 void SetTextDirectionInfo(); //image direction
	 void SetTextInfoDirectin(const char** dirction);
	 void SetTextDirectinImg();
	 void SetLabelImage(QLabel *plabel, const char **pImg, int idx);
	 // void SetTextInfoLocatinInfo();
     //image basic operation
     void ScrollPreviousImgPage();
     void ScrollNextImgPage();
     void PushResetImgWCMsg();

     //msg push interface
     void PushMsgToThreadOremitToRender();
     void PushViewMsgForUpDateImageData(T_MSG_Value v);

      //ROI Calculate
     bool IsROIOperattionEnable();
	 void PushROIOperateMsgtoMaindata();
	 //measure process
	 void PushMeasureOperationMsgtoMaindata(E_MeasureOperateType type);
	
	 
private:
     void InitGLRect(int wid,int hgt);
     void CreateImage();
     void PushResetImgSizeLocationMsg();
     void zoomIn();  // to big
     void zoomOut();  // to small
     void zoom(float scale); //  scaleFactor
     void translate(QPointF delta);  // translate

    //ROI MouseEvent
     void ProcessMarkActionROIMousePressEvent(QPoint screempt);
     void ProcessMarkActionROIMouseMoveEvent(QPoint screempt);
    // scrollpage, window width and center process
     void ProcessMarkActionScrollPageMouseMoveEvent(QPoint screempt);
     void ProcessMarkActionImageWWandWCMouseMoveEvent(QPoint screempt);
	 
	 //measure mouseevent
	 void ProcessMarkActionMeasurePressEvent(E_MarkAction msg, QPointF pt);
	 void ProcessMarkActionMeasureMoveEvent(QPoint screempt);
	 QList<QPointF>*  SetMeasureToolPoints(E_MarkAction msg, QPointF pt);
	 void FindCurToolIdx(QPointF pt);
public:
	 E_MarkMoveStyle mMoveStyle;
	 qreal mCurmoveToolIdx;
	 int mCurmoveToolPointIdx;

	 //zlx robot path
	 bool mIsRenderLine;
	 bool mIsSelectXLine;
	 bool mIsSelectYLine;
	 bool mIsRenderTool;

	 QPoint mCurTip;
	 QPoint mCurMid;
protected:
	 void CreateToolWdt();
	 void SetToolCellCtrls(T_ToolCellCtrl & cellctrl, int * content);
	 void LayoutToolWdt();
	 void SetToolCellRectCtrl(T_ToolRcCtrl & rc, double * rcrate);

	 void ProAimToolNormalTypeBtn(int idx);
	 void ProAimToolPressedTypeBtn(int idx);
	 void ProAimToolSwitchTypeBtn(int idx);
signals:
    void renderRequested();
    void PushMsg(T_MSG_Value value);
    void PushMousePressMsg();
	void PushSelectOneToolMsg(qreal index);
    void pushmsgtoimagemultiwidget(T_MSG_Value v);
    void SendMovePtRequest(QPoint spt );
	void SendGLWidgetShowStatusRequest(E_GL_ToolCellType type);
	//void SendSliceChange(quint16 axial, quint16 coronal, quint16 sagittal);
	void SendToUpdateSliceChange();

public slots:
    void grabContext();
	void ProAimToolBtnClickEvent(int idx, E_BTN_SATUS_TYPE btntype);
	void ProPathAddEvent(bool flag);
	void UpdateCoordinateLineShow();
private slots:
    void onAboutToCompose();
    void onFrameSwapped();
    void onAboutToResize();
    void onResized();


protected:
    //image data ctrl
    quint8 *mpCurImgData;
    QImage *mpCurQImg;

    T_DImgPropety mImgpty;
    DICOMPropertyData *mDVolumpty;
	T_DImageWindowCtrl *mImgWCCtrl;
    MainData *mpData;
    float m_ImgXYRatio;


private:
    //opengl thread
    QThread *m_thread;
    Renderer *m_renderer;
    quint8 m_wdtindex;

    //opengl draw control
    GLfloat m_arrViewRc[4];
    GLfloat m_arrWatchRc[4];
    GLfloat m_arrTextRc[4];
    bool mbUpdateImgDataFlag;
    bool mbPaintFlag;

    //image sroll pan control
    qreal mScaleFactor;
    qreal mHoriOffset;
    qreal mVertOffset;
    qreal mMsgHoriOffset;//just from msg value
    qreal mMsgVertOffset;
    qreal mMsgScale;


    //*******image translate when touch event or mouse event*******//
   QPointF mPreScreenPt;

   bool m_bIsTwoPoint;
   bool m_bMouseTranslate;
   qreal mZoomDelta;  // zooming factor
   QPoint m_LastMousePos;  // mouse press last position
   bool mbTouching;//when touch, mouse move unvalid


   //textinfo
   struct T_TextInfo m_arrTextInfo[TEXT_INFO_COUNT];

   //gestures oparation control
   static bool m_sbTouchOperateFlag;//true  valid
   static E_MarkAction m_sCurMarkAction;

   //ROI
   QPoint mScreenPt;
   static QColor m_sConColor;//accoording to tool type define
   quint8 m_BrushRadius;
   bool m_bValidAreaFlag;
   QList<T_Contour> *m_pConList;
   E_MaskLabel mCurBrushState;
   QList<QPointF> mBrushCirclePtList;
   bool m_bIsBrushing;
   static E_ImageType m_CurROIImgType;
   static bool mbROIEnale;
   E_ROIOperateType mCurROIOperaType;
   quint16 mPtCtnOldRoI;
   QList<QPointF> mBrushEdgePtList;
   QList<QPointF> mBrushInnerPtList;
   QList<QPointF> mBrushPtList;

  //ui define
   QList<QLabel*> mTextInfoLabelList;
   AimToolCellWidget *mpToolWdts;
   E_GL_ToolCell_Rect_Idx mCurBtnRectIdx;
   E_GL_ToolCellType mCurToolPressFunType;
   bool mbPaint;
   //argorithm class
   ImageSegment*mpImgSegment;
   CalculateClass *mpCalculate;
   AItkImgSeg *mpItkSeg;

   E_GL_WdtShowStats mWdtShowStatus;
   E_IMG_Multi_Show_Mode *mpImgShowMode;

   static E_ROI_Status_Mode mCurROIStatusMode;
   QList<T_ROIDataCtrl*>*m_pROIsDataCtrlList;
   QList<T_MeasureDataCtrl*>*m_pMeasuresDataCtrlList;
   qreal mCurSelectMeasureIdx;
   bool mIsMouseMove;
   
};

#endif
