#pragma once
#include "ConfigCtrl\CommonConfig.h"

#define RPW_Text_fsize 10
#define RPW_Table_Column 1
#define RPW_TableText_fsize 10
enum RPW_TextLabel_Type
{
	RPW_PathTitle_Lbl = 0,
	RPW_PathTitleDetail_Lbl,
	RPW_PathMsg_Lbl,
};

enum RPW_ImgLabel_Type
{
	RPW_SetTargetPt_Lbl = 0,
	RPW_SetIntoPt_Lbl,
};

enum RPW_TextBtn_Type
{
	RPW_SetTargetPt_Btn = 0,
	RPW_SetIntoPt_Btn,
	RPW_Clear_Btn,
	RPW_Done_Btn,
};

enum RPW_ImgBtn_Type
{
	RPW_AddPath_Btn = 4,
	RPW_DeletePath_Btn,
};

static double RPW_TextLbl_Rect[][4] =
{
	{ 0.0458,0.0408,0.1526,0.12 }, //title
	{ 0.2061,0.0408,0.7557,0.12 },//title detail
	{ 0.0382,0.2853,0.8244,0.12 },//path msg
};

enum RPW_TextType
{
	RPW_Title_Text = 0,
	RPW_TitleDetail_Text = 2,
	RPW_SetTar_BtnText = 4,
	RPW_SetInto_BtnText = 6,
	RPW_Clear_BtnText = 8,
	RPW_Done_BtnText = 10,
};

static const char* RPW_TextStr[] =
{
	"名称:","Title:",
	"路径","Path",
	"设置靶点","",
	"设置进针点","",
	"清除","clear",
	"完成","finish",
};

enum RPW_Color_Type
{
	RPW_Text_Color = 0,
};

static const int  RPW_TextColor[][3] =
{
	{ 255,255,255 },
};

enum RPW_StyleSheet_Type
{
	RPW_Common_Style = 0,
	RPW_FrameLbl_Style,
	RPW_Table_Style,
};

static const char *RPW_StyleSheet_TextStr[] =
{
	"{background-color: transparent;}",
	"{background-color: transparent;border:1px solid rgb(131, 130,129);border-radius: 5px;}",
	"{background-color: rgb(50,52,53);selection-background-color:rgb(94, 104, 110);gridline-color : rgb(81, 81, 81);}",
};

static double RPW_ImgLbl_Rect[][4] =
{
	{ 0.17,0.19,0.0573,0.0612 },//target
	{ 0.56,0.19,0.0573,0.0612 },//into
};

static double RPW_TextBtn_Rect[][4] =
{
	{ 0.23,0.17,0.3,0.1 },//target
	{ 0.62,0.17,0.3,0.1 },//into
	{ 0.1,0.87,0.38,0.1 },//clear
	{ 0.55,0.87,0.38,0.1 },//done
};

static double RPW_ImgBtn_Rect[][4] =
{
	{ 0.87,0.2853,0.122,0.1036 },//add path
	{ 0.87,0.41,0.122,0.1036 },//delete path
};

enum RPW_StrImgOrder
{
	RPW_Target_Image = 0,
	RPW_Into_Image = 1,
	RPW_AddPath_Image = 2,
	RPW_Delete_Image = 3,
};

static const char *RPW_ImgPathStr[]
{
	"./image/robot/setpoint.png",
	"./image/robot/setpoint2.png",
	"./image/robot/add.png",
	"./image/robot/delete.png",
};

#define RPW_Table_X 0.0382
#define RPW_Table_Y 0.41
#define RPW_Table_Width 210
#define RPW_Table_Height 100

enum RPW_Event_Type
{
	RPW_SetIntoPt = 0,
	RPW_SetTargetPt = 1,

};
