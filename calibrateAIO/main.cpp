﻿

#include "CalibrationTool.hpp"
#include "ParameterBase.hpp"
#include <ctime>

//using namespace std;
#include "point.h"
#include "lof.h"
#include <algorithm>
#include <map>
#include"puttext.h"
#include<iostream>
#include<filesystem>

using namespace LOF;
//using namespace matplot;


using namespace UcitCalibrate;

//#define readcalib
//#define Readcalibratexml 
#define  calibrateradar
#define writecalibratexml
#define point_10


#define project2pixeltest 1
#define project2gpstest 0

#define ManMade	0


enum pixelDirection
{
	topleft = 1,
	topright,
	bottomleft,
	bottomright,
	Nonbias
};

struct dircs
{
	pixelDirection e_dirs;
	std::string m_drirs;
};

namespace fs = std::filesystem;


dircs judgedirection(cv::Point2d& srcpoint, cv::Point2d& basepoint, double threashold)
{
	dircs ret{Nonbias,"雷达返投点位不偏"};
	double xx = srcpoint.x - basepoint.x;
	double yy = srcpoint.y - basepoint.y;

	double offset = std::max(abs(xx), abs(yy));
	if (offset > threashold)
	{
		if (xx < 0 && yy < 0)
		{
			ret.e_dirs = topleft;
			ret.m_drirs = "雷达返投偏左上";
		}
		if (xx > 0 && yy < 0)
		{
			ret.e_dirs = topright;
			ret.m_drirs = "雷达返投偏右上";
		}
		if (xx > 0 && yy > 0)
		{
			ret.e_dirs = bottomright;
			ret.m_drirs = "雷达返投偏右下";
		}
		if (xx < 0 && yy>0)
		{
			ret.e_dirs = bottomleft;
			ret.m_drirs = "雷达返投偏左下";
		}
	}
	else
	{
		return ret;
	}
	
	return ret;
}


typedef std::pair<string, double> PAIR;
struct cmpsort {
	bool operator()(const PAIR& lhs, const PAIR& rhs) {
		return lhs.second > rhs.second;
	}
};


string dou2str(double num,int precision = 16)   //num也可以是int类型
{
	stringstream ss;      //stringstream需要sstream头文件
	ss.precision(precision);
	string str;
	ss << num;
	ss >> str;
	return str;
}

void Gps2WorldCoord4test(double earthR,double handleheight, \
	longandlat m_longandlat, std::map<int, double> P1_lo, \
	std::map<int, double> P1_la, vector<cv::Point3d> &m_worldBoxPoints)
{

	if (P1_la.size() != P1_lo.size())
	{
		printf("input the longitude and latitude can't be paired!!! return");
		return;
	}
	double val = CV_PI / 180.0;
	for (int i = 1; i < P1_la.size()+1; i++)
	{
		cv::Point3d temp3d;
		temp3d.x = 2 * CV_PI * (earthR * cos(m_longandlat.latitude * val)) * ((P1_lo[i] - m_longandlat.longtitude) / 360);
		temp3d.y = 2 * CV_PI * earthR * ((P1_la[i] - m_longandlat.latitude) / 360);
		temp3d.z = handleheight;
		m_worldBoxPoints.push_back(temp3d);
	}
	printf("******************* \n");
	
}

// 创建xml文件

int writeXmlFile(cv::Mat *raderRT44,
	cv::Mat *cameraRT44,
	cv::Mat *cameraRT33,
	cv::Mat *cameraRT31,
	double m_longtitude,
	double m_latititude,
	cv::Mat *cameraDist,
	cv::Mat *camerainstrinic,
	double handheight,
	double radarinstallheight,
	std::string place,
	const char * file)
{
	if (raderRT44==nullptr || cameraRT44==nullptr)
	{
		printf("input rt matrix error \n");
		return -1;
	}
	TiXmlDocument* writeDoc = new TiXmlDocument; //xml文档指针

	//文档格式声明
	TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "UTF-8", "yes");
	writeDoc->LinkEndChild(decl); //写入文档

	TiXmlElement* RootElement = new TiXmlElement("CalibInfo");//根元素
	writeDoc->LinkEndChild(RootElement);

	TiXmlElement* placeElement = new TiXmlElement("place");
	RootElement->LinkEndChild(placeElement);
	TiXmlText* placetext = new TiXmlText(place.c_str());
	placeElement->LinkEndChild(placetext);

	// 雷达rt矩阵写入
	if (raderRT44->empty())
	{
		printf("radarrt44 is empty,and not create radarRT44 xml \n");
	}
	else

	{
		TiXmlElement* RadElement = new TiXmlElement("radarRT44");
		RootElement->LinkEndChild(RadElement);
		for (int r = 0; r < raderRT44->rows; r++)
		{
			for (int c = 0; c < raderRT44->cols; c++)
			{
				TiXmlElement* index = new TiXmlElement("index");
				RadElement->LinkEndChild(index);
				std::string valuestring = dou2str(raderRT44->at<double>(r, c));
				TiXmlText* value = new TiXmlText(valuestring.c_str());
				index->LinkEndChild(value);
			}
		}
	}


	// 摄像头rt矩阵写入
	TiXmlElement* CameraElement = new TiXmlElement("cameraRT44");
	RootElement->LinkEndChild(CameraElement);
	for (int r = 0; r < cameraRT44->rows; r++)
	{
		for (int c = 0; c < cameraRT44->cols; c++)
		{
			TiXmlElement* index = new TiXmlElement("indexcamera");
			CameraElement->LinkEndChild(index);
			std::string valuestring = dou2str(cameraRT44->at<double>(r, c));
			TiXmlText* value = new TiXmlText(valuestring.c_str());
			index->LinkEndChild(value);
		}
	}

	for (int i = 0; i < 4; i++)
	{
		TiXmlElement* index = new TiXmlElement("indexcamera");
		CameraElement->LinkEndChild(index);
		if (i!=3)
		{
			std::string value = "0";
			TiXmlText* values = new TiXmlText(value.c_str());
			index->LinkEndChild(values);
		}
		else
		{
			std::string value = "1";
			TiXmlText* values = new TiXmlText(value.c_str());
			index->LinkEndChild(values);
		}
	}

	TiXmlElement* CameraElement33 = new TiXmlElement("cameraRT33");
	RootElement->LinkEndChild(CameraElement33);
	for (int r = 0; r < cameraRT33->rows; r++)
	{
		for (int c = 0; c < cameraRT33->cols; c++)
		{
			TiXmlElement* index = new TiXmlElement("indexcamera33");
			CameraElement33->LinkEndChild(index);
			std::string valuestring = dou2str(cameraRT33->at<double>(r, c));
			TiXmlText* value = new TiXmlText(valuestring.c_str());
			index->LinkEndChild(value);
		}
	}

	TiXmlElement* CameraElement31 = new TiXmlElement("cameraRT31");
	RootElement->LinkEndChild(CameraElement31);
	for (int r = 0; r < cameraRT31->rows; r++)
	{
		for (int c = 0; c < cameraRT31->cols; c++)
		{
			TiXmlElement* index = new TiXmlElement("indexcamera31");
			CameraElement31->LinkEndChild(index);
			std::string valuestring = dou2str(cameraRT31->at<double>(r, c));
			TiXmlText* value = new TiXmlText(valuestring.c_str());
			index->LinkEndChild(value);
		}
	}


	TiXmlElement* originpoll = new TiXmlElement("originpoll");
	RootElement->LinkEndChild(originpoll);
	for (int r = 0; r < 2; r++)
	{
		if (r ==0 )
		{
			TiXmlElement* lng = new TiXmlElement("lng");
			originpoll->LinkEndChild(lng);
			std::string value = dou2str(m_longtitude);
			TiXmlText* values = new TiXmlText(value.c_str());
			lng->LinkEndChild(values);
		}
		else
		{
			TiXmlElement* lat = new TiXmlElement("lat");
			originpoll->LinkEndChild(lat);
			std::string value = dou2str(m_latititude);
			TiXmlText* values = new TiXmlText(value.c_str());
			lat->LinkEndChild(values);
		}
	}


	TiXmlElement* distort = new TiXmlElement("distort");
	RootElement->LinkEndChild(distort);
	char zifu[256];
	for (int i=0; i < 5;i++)
	{
		sprintf(zifu, "value%d", i);
		TiXmlElement* index = new TiXmlElement(zifu);
		distort->LinkEndChild(index);
		std::string value = dou2str(cameraDist->at<double>(i, 0));
		TiXmlText* values = new TiXmlText(value.c_str());
		index->LinkEndChild(values);
	}

	TiXmlElement* camerainstri = new TiXmlElement("camerainstrinic");
	RootElement->LinkEndChild(camerainstri);

	for (int i=0;i < 4;i++)
	{
		std::string value = "";
		
			if (i == 0)
			{
				value = dou2str(camerainstrinic->at<double>(0, 0));
				TiXmlElement* fx = new TiXmlElement("fx");
				camerainstri->LinkEndChild(fx);
				TiXmlText* values = new TiXmlText(value.c_str());
				fx->LinkEndChild(values);
			}
			else if (i == 1)
			{
				value = dou2str(camerainstrinic->at<double>(1, 1));
				TiXmlElement* fy = new TiXmlElement("fy");
				camerainstri->LinkEndChild(fy);
				TiXmlText* values = new TiXmlText(value.c_str());
				fy->LinkEndChild(values);
			}
			else if (i ==2)
			{
				value = dou2str(camerainstrinic->at<double>(0, 2));
				TiXmlElement* cx = new TiXmlElement("cx");
				camerainstri->LinkEndChild(cx);
				TiXmlText* values = new TiXmlText(value.c_str());
				cx->LinkEndChild(values);
			}
			else
			{
				value = dou2str(camerainstrinic->at<double>(1, 2));
				TiXmlElement* cy = new TiXmlElement("cy");
				camerainstri->LinkEndChild(cy);
				TiXmlText* values = new TiXmlText(value.c_str());
				cy->LinkEndChild(values);
			}
		}

	TiXmlElement* handradarheight = new TiXmlElement("handradarheight");
	RootElement->LinkEndChild(handradarheight);
	TiXmlElement* height = new TiXmlElement("height");
	handradarheight->LinkEndChild(height);
	std::string handheight_str = dou2str(handheight);
	TiXmlText* values = new TiXmlText(handheight_str.c_str());
	height->LinkEndChild(values);

	TiXmlElement* radarinstall = new TiXmlElement("radarinstallheight");
	RootElement->LinkEndChild(radarinstall);
	TiXmlElement* heightrader = new TiXmlElement("height");
	radarinstall->LinkEndChild(heightrader);
	std::string installheights = dou2str(radarinstallheight);
	TiXmlText* isntallvalues = new TiXmlText(installheights.c_str());
	heightrader->LinkEndChild(isntallvalues);


		writeDoc->SaveFile(file);
		delete writeDoc;
		return 1;
}


int main(int argc, char ** argv)
{
	// 命令行输入

	if (argc < 5)
	{
		std::cout << "please input the mode of calibrate! for MEC or AIO" << std::endl;
		printf("please input the mode, tupianpath, xmlfile, savepath respectively \n");
		return 0;
	}

	std::string mode = "";
	
	mode = argv[1];
	
	printf("mode is %s \n", mode.c_str());

	std::string savepath = argv[4];
	

	if (!fs::is_directory(savepath)) {
		fs::create_directories(savepath);
	}
	else {
		std::cout << "文件夹已存在" << std::endl;
	}

	

	////// 首先通过标定板的图像像素坐标以及对应的世界坐标，通过PnP求解相机的R&T//////
	// 准备的是图像上的像素点
	// 创建输出参数文件，ios：：trunc含义是有文件先删除
	ofstream outfile("../results/CalibrateLog.txt", ios::trunc);
	// record GPS files for display

	ofstream gpsfile("../results/gpsGenerator.txt", ios::trunc);


	std::string m_xmlpath = "";   //原来的标定
	m_xmlpath = argv[3];
	
	// 基于当前系统的当前日期/时间
	time_t now = time(0);
	// 把 now 转换为字符串形式
	


	
	CalibrationTool  &m_Calibrations = CalibrationTool::getInstance();
	vector<unsigned int> pickPointindex;
	bool rasac = false;

	
	double reflectorheight,raderheight;
	std::map<int, cv::Point3d> m_Measures;
	vector<cv::Point2d> boxPoints, validPoints;
	


	struct meandistance
	{
		double pixeldistance;
		double radardistance;
		double gpsdistance;
	};

	



	vector<meandistance> error_means;

	std::map<int, cv::Point2d> mp_images;
	std::map<int, double> mp_Gpslong, mp_Gpslat;

	longandlat originallpoll;
	cv::Mat  cameradistort1,camrainst;
	vector<double> m_ghostdis;
	std::vector<double> gpsheight;
	std::string calibrateplace = "";
	// read from xml config


	// 人工补点之类
	vector<cv::Point2d> ManMadePixels;
	std::map<int, cv::Point3d> ManMadeRadars;

#ifndef  readcalib

		m_Calibrations.ReadPickpointXml(m_xmlpath,
		pickPointindex,
		boxPoints,
		mp_images,
		mp_Gpslong,
		mp_Gpslat,
		reflectorheight,
		raderheight,
		m_Measures,
		originallpoll, m_ghostdis, camrainst,gpsheight,calibrateplace);

		int manPointsNum = 0;


#if ManMade
		cv::Point2d pixels1(1403.632, 375.81);
		cv::Point2d pixels2(1516.587, 367.152);
		cv::Point2d pixels3(1862.1045, 434.562);

		cv::Point3d radars1(-3.600006, 67, reflectorheight);
		cv::Point3d radars2(-7, 65.600037, reflectorheight);
		cv::Point3d radars3(-13.199997, 52.600037, reflectorheight);



		ManMadePixels.push_back(pixels1);
		ManMadePixels.push_back(pixels2);
		ManMadePixels.push_back(pixels3);

		ManMadeRadars[0] = radars1;
		ManMadeRadars[1] = radars2;
		ManMadeRadars[2] = radars3;

		manPointsNum = 3;

#endif


		//处理measures
		std::map<int, cv::Point3d>::iterator iter_begin = m_Measures.begin();
		std::map<int, cv::Point3d>::iterator iter_end = m_Measures.end();
		/*for (; iter_begin != iter_end; iter_begin++)
		{
			double ydist = iter_begin->second.y;
			double ynew = sqrt(ydist * ydist - pow(raderheight - reflectorheight, 2));
			iter_begin->second.y = ynew;
			cout << ynew << "newy of iter:" << iter_begin->second.y << endl;

		}*/

		m_Calibrations.SetCoordinateOriginPoint(originallpoll.longtitude, originallpoll.latitude);
		m_Calibrations.SetRadarHeight(reflectorheight);

#endif
	



	//setpi
	m_Calibrations.SetPi(CV_PI);

	std::vector<CalibrationTool::BoxSize> mv_results;
	m_Calibrations.PickImagePixelPoints4PnPsolve(pickPointindex, mp_images);

	// Step one Loading image
	std::string tupath = argv[2];
	cv::Mat sourceImage = cv::imread(tupath);

	cv::Mat xiezitu(cv::Size(800, 1440),CV_8UC3);
    // 写字
	cv::Mat hints(cv::Size(800, 1440), CV_8UC3);
    
    char textbuf[256];

	// draw 原始的取点
	vector<cv::Point2d>::iterator iter_origpixel = boxPoints.begin();
	vector<cv::Point2d>::iterator iter_origpixel_e = boxPoints.end();


	for (int i = 0; i < boxPoints.size(); ++i)
	{
		circle(sourceImage, boxPoints[i], 8, cv::Scalar(100, 100, 0), -1, cv::LINE_AA);
        int text_x = (int)(boxPoints[i].x - 30);
        int text_y = (int)(boxPoints[i].y - 10);
        sprintf(textbuf, "p:%d",i+1);
		//putText::putTextZH(sourceImage, textbuf, cv::Point(text_x - 15, text_y - 8), cv::Scalar(0, 100, 0), 30, "宋体");
        cv::putText(sourceImage, textbuf, cv::Point(text_x-5, text_y-8), 0,1, cv::Scalar(0,0,255),2,1);
	}

	putText::putTextZH(hints, "深蓝色点是采集的原始像素点", cv::Point(100, 100), cv::Scalar(100, 100, 0), 40, "隶书");

#if ManMade

	for (size_t i = 0; i < manPointsNum; i++)
	{
		cv::circle(sourceImage, ManMadePixels[i], 8, cv::Scalar(0, 100, 200),-2, cv::LINE_AA);
		int text_x = (int)(ManMadePixels[i].x - 30);
		int text_y = (int)(ManMadePixels[i].y - 10);
		sprintf(textbuf, "ManP:%d", i + 1);
		cv::putText(sourceImage, textbuf, cv::Point(text_x - 5, text_y - 8), 0, 1.5, cv::Scalar(0, 100, 200), 2, 1);
	}

#endif

	cv::namedWindow("raw image", cv::WINDOW_NORMAL);
	
    // Step two Calculate the GPS 2 World
	
	
	// pick points for calibration
	m_Calibrations.PickRawGPSPoints4VectorPairs(pickPointindex, mp_Gpslong, mp_Gpslat);

	// gps convert to the world coordination


	// 最终选定的高程的点
	std::vector<double> gpsheights;
	for (int j = 0; j < pickPointindex.size(); j++)
	{
		gpsheights.push_back(gpsheight[pickPointindex[j] - 1]);
	}



	if (strcmp(mode.c_str(), "MEC")==0)
	{
		printf("calibrate MEC now \n");
		// 预备标定camera的世界坐标系的点
		m_Calibrations.Gps2WorldCoord(m_Calibrations.gps_longPick, m_Calibrations.gps_latiPick, gpsheights);
		// 增加手持gps
	}
	else if (strcmp(mode.c_str(), "AIO") == 0)
	{
		printf("calibrate AIO now \n");
		
	}
	else
	{
		printf("please input the create mode, be carefull capital written!");
		return 0;
	}



	
	 // 这个函数不知道干啥用的
	/*std::vector<cv::Point3d> Gpsworld4radarcalibrate;
	m_Calibrations.Generategps2world(m_Calibrations.gps_longPick,
		m_Calibrations.gps_latiPick,
		gpsheights,
		Gpsworld4radarcalibrate);*/


	m_Calibrations.SetWorldBoxPoints();



	// 计算雷达的rt矩阵
	// 用于反算雷达是否标定准确,以下的点是实测距离值，单位为米
	vector<cv::Point3d> srcPoints;




// 标定雷达的rt矩阵
	// 

	if (strcmp("AIO",mode.c_str())==0)
	{
		// 为标定AIO选定雷达原始测量值
		m_Calibrations.PickMeasureMentValue4AIOcalib(pickPointindex, m_Measures);
	}
	else
	{
		CalibrationTool::getInstance().PickRadarMeasure4MECcalibRadarRT(pickPointindex, m_Measures);
		// 计算雷达的rt矩阵
		cv::Mat RT = m_Calibrations.Get3DR_TransMatrix(m_Calibrations.GetMeasureMentPoint(), \
			m_Calibrations.GetWorldBoxPoints());

		char* dt = ctime(&now);
		cout << "本地日期和时间：" << dt << endl;
		outfile << "Current Generate time: \n" << dt << endl;
		for (int i = 0; i < pickPointindex.size(); ++i)
		{
			outfile << "Pick the Points:" << pickPointindex[i] << " for calibration" << endl;
		}
		outfile << "\n" << endl;
		outfile << "it is a radar RT matrix first! \n" << endl;
		outfile << RT << "\n\n" << endl;

	}

	

	// camera relevant 
	// camera inside parameters
   

	// 设置相机的内参和畸变系数
	m_Calibrations.SetCameraInstrinic(camrainst.at<double>(0,0)
		,camrainst.at<double>(1,1),
		camrainst.at<double>(0,2)
		,camrainst.at<double>(1,2));

	cv::Mat cameradistort = cv::Mat::eye(5, 1, cv::DataType<double>::type);
	for (int i = 0; i < 5; i++)
	{
		cameradistort.at<double>(i, 0) = m_ghostdis[i];
	}

	m_Calibrations.SetCameraDiff(
		cameradistort.at<double>(0,0),
		cameradistort.at<double>(1, 0),
		cameradistort.at<double>(2, 0),
		cameradistort.at<double>(3, 0),
		cameradistort.at<double>(4, 0)
	);
	


	m_Calibrations.SetRadarHeight(reflectorheight);
	m_Calibrations.SetCoordinateOriginPoint(originallpoll.longtitude, originallpoll.latitude);
	m_Calibrations.CalibrateCamera(rasac, mode, pickPointindex);

		double poll_lon, poll_lat;
		poll_lon = originallpoll.longtitude;
		poll_lat = originallpoll.latitude;
		// generate xml files 


		cv::Mat tmpRadarRT = CalibrationTool::getInstance().GetRadarRTMatrix();
		cv::Mat tmpCameraRT = CalibrationTool::getInstance().GetCameraRT44Matrix();
		
		std::string files = savepath + "/" + "calibration.xml";

		int flag = writeXmlFile(&tmpRadarRT, \
			& tmpCameraRT, &m_Calibrations.m_cameraRMatrix33, \
			& m_Calibrations.m_cameraTMatrix, \
			poll_lon,\
			poll_lat,\
			&cameradistort,\
			&camrainst,\
			reflectorheight,\
			raderheight,\
			calibrateplace,\
			files.c_str());




		// gps 转化成世界坐标之后的点

		cv::Mat m_gps2world = cv::Mat::ones(4, 1, cv::DataType<double>::type);
		cv::Mat m_AIOradarWorld = cv::Mat::ones(4, 1, cv::DataType<double>::type);



		//3D to 2D////////////////////////////
		cv::Mat image_points = cv::Mat::ones(3, 1, cv::DataType<double>::type);

		meandistance recorddistance{ 0,0,0 };

if(strcmp("MEC",mode.c_str())==0)
{ 

#if project2pixeltest
		
		// 反推12个点投影到pixel坐标,以下用新构造的点来计算,像素的值 红色
		vector<cv::Point3d> m_fantuiceshi;
		Gps2WorldCoord4test(6378245, reflectorheight, originallpoll, mp_Gpslong, mp_Gpslat, m_fantuiceshi);

		vector<cv::Point3d>::iterator iter = m_fantuiceshi.begin();
		for (; iter != m_fantuiceshi.end(); iter++)
		{
			m_gps2world.at<double>(0, 0) = iter->x;
			m_gps2world.at<double>(1, 0) = iter->y;
			// 预估gps测量时的z轴高度为1.2f
			m_gps2world.at<double>(2, 0) = iter->z;
			image_points = camrainst * CalibrationTool::getInstance().GetCameraRT44Matrix() * m_gps2world;
			cv::Mat D_Points = cv::Mat::ones(3, 1, cv::DataType<double>::type);
			D_Points.at<double>(0, 0) = image_points.at<double>(0, 0) / image_points.at<double>(2, 0);
			D_Points.at<double>(1, 0) = image_points.at<double>(1, 0) / image_points.at<double>(2, 0);

			cout << "3D to 2D:   " << D_Points << endl;

			cv::Point2d raderpixelPoints;
			raderpixelPoints.x = D_Points.at<double>(0, 0);
			raderpixelPoints.y = D_Points.at<double>(1, 0);
			validPoints.push_back(raderpixelPoints); 
			std::string raders = "radarPoints";

			cv::circle(sourceImage, raderpixelPoints, 6, cv::Scalar(0, 0, 255), -1, cv::LINE_AA);
		}
		for (int i = 0; i < boxPoints.size(); i++)
		{
			recorddistance.pixeldistance = sqrt(pow((validPoints[i].x - boxPoints[i].x), 2) + pow((validPoints[i].y - boxPoints[i].y), 2));

			error_means.push_back(recorddistance);
			printf("distancepixle[%d]:%f \n", i, recorddistance.pixeldistance);
		}
#endif
		putText::putTextZH(hints, "红色点是像素点经过矩阵反投影的值", cv::Point(100, 200), cv::Scalar(0, 0, 255), 40, "隶书");



		// 计算标定误差
		outfile << "\n" << endl;
		outfile << "Error X not exceeding 20, while Error Y not exceeding 15 at least!!\n" << endl;
		vector<int> index;
		int count = 0;
		for (int i=0; i < validPoints.size(); ++i)
		{
			count++;
			printf("count:%d \n", count);
			double error_pixel = std::abs(validPoints[i].x - boxPoints[i].x) / 2560;
			double error_x = std::abs(validPoints[i].x - boxPoints[i].x);
			double error_y = std::abs(validPoints[i].y - boxPoints[i].y);
			double error_pixel_y = std::abs(validPoints[i].y - boxPoints[i].y) / 1440;
			if (error_x>20 || error_y>14)
			{
				index.push_back(i);
			}
			std::cout <<"cv::Point:"<<i<<"\t"<< "error:X\t"<< error_x<<"\t"<< "error:Y\t" <<error_y<< std::endl;
			outfile << "cv::Point:" << i << "\t" << "error:X\t" << error_x << "\t" << "error:Y\t" << error_y << std::endl;
		}
		outfile << "\n" << endl;
		for (int k=0;k<index.size();++k)
		{
			outfile << "cv::Point:" << index[k]<<"\t's\t" << "error exceeds requirement!!" << endl;
		}
		outfile << "\n" << endl;
		if (index.size()==3)
		{
			outfile << "there are  three points exceeding the requirement,calibration has just passed!!\n  " << endl;
			outfile << "passed!" << endl;
		}
		else if (index.size()>3)
		{
			outfile << "calibration's accuracy didn't satisfy requirement!!!\n" << endl;
			outfile << "calibrate failed!<<" << endl;
		}
		else if(index.size() < 3)
		{
			outfile << "calibration's accuracy  satisfy requirement,good\n" << endl;
			outfile << "calibrate good enough!" << endl;
		}


#if ManMade
	
		for (int i = 0; i < manPointsNum; i++)
		{
			GpsWorldCoord radar_input;
			longandlat gpsresult;
			radar_input.X = ManMadeRadars[i].x;
			radar_input.Y = ManMadeRadars[i].y;
			// input distance 有可能要修改
			radar_input.Distance = 0;
			// 反算雷达到gps
			m_Calibrations.radarworld2Gps(radar_input, gpsresult);
			printf("GPS:[%3.8f,%3.8f] \n", gpsresult.longtitude, gpsresult.latitude);
			printf("GPS:[%3.8f,%3.8f] \n", gpsresult.longtitude, gpsresult.latitude);
			printf("GPS:[%3.8f,%3.8f] \n", gpsresult.longtitude, gpsresult.latitude);
			outfile <<fixed << setprecision(10) << gpsresult.longtitude << "\t" << gpsresult.latitude << std::endl;

		}


#endif
}
else
{
		// 验证AIO 投影反算
		std::map<int, cv::Point3d>::iterator iter_radarM_begin = m_Measures.begin();
		std::map<int, cv::Point3d>::iterator iter_radarM_end = m_Measures.end();
		for (; iter_radarM_begin!=iter_radarM_end; iter_radarM_begin++)
		{
			// old functions
			/*m_AIOradarWorld.at<double>(0, 0) = iter_radarM_begin->second.x;
			m_AIOradarWorld.at<double>(1, 0) = iter_radarM_begin->second.y;
			m_AIOradarWorld.at<double>(2, 0) = iter_radarM_begin->second.z;
			image_points = camrainst * CalibrationTool::getInstance().GetCameraRT44Matrix() * m_AIOradarWorld;
			cv::Mat D_Points = cv::Mat::ones(3, 1, cv::DataType<double>::type);
			D_Points.at<double>(0, 0) = image_points.at<double>(0, 0) / image_points.at<double>(2, 0);
			D_Points.at<double>(1, 0) = image_points.at<double>(1, 0) / image_points.at<double>(2, 0);
			raderpixelPoints.x = D_Points.at<double>(0, 0);
			raderpixelPoints.y = D_Points.at<double>(1, 0);*/

			AIOradarCoord radarworld;
			radarworld.X = iter_radarM_begin->second.x;
			radarworld.Y = iter_radarM_begin->second.y;
			radarworld.Z = iter_radarM_begin->second.z;
			cv::Point2d raderpixelPoints;
			CalibrationTool::getInstance().AIORadar2camerapixel(radarworld, raderpixelPoints);
			validPoints.push_back(raderpixelPoints);
			std::string raders = "radarPoints";
			cv::circle(sourceImage, raderpixelPoints, 6, cv::Scalar(0, 0, 255), -1, cv::LINE_AA);

		}
		for (int i = 0; i < boxPoints.size(); i++)
		{
			recorddistance.pixeldistance = sqrt(pow((validPoints[i].x - boxPoints[i].x), 2) + pow((validPoints[i].y - boxPoints[i].y), 2));

			error_means.push_back(recorddistance);
			printf("distancepixle[%d]:%f \n", i, recorddistance.pixeldistance);
		}
		putText::putTextZH(hints, "红色点是原始雷达值反投影的值", cv::Point(100, 200), cv::Scalar(0, 0, 255), 40, "隶书");


}

if (strcmp("MEC",mode.c_str())==0)
{
#if  project2pixeltest 

	// 雷达反投影,应该也要先乘以雷达的逆矩阵
	//  雷达坐标系返回的点是左正右负, 反投影雷达要加高度z，记住
	std::vector<cv::Point2d> radarprojectpixle;
	for (; iter_begin != iter_end; iter_begin++)
	{
		cv::Point3d radartmp;
		radartmp.x = iter_begin->second.x;
		radartmp.y = iter_begin->second.y;
		radartmp.z = iter_begin->second.z;
		cv::Point2d radpixe;

		longandlat gpsresult;
		GpsWorldCoord radar_input, radarworld;
		radar_input.X = radartmp.x;
		radar_input.Y = radartmp.y;
		// input distance 有可能要修改
		radar_input.Distance = 0;
		// 反算雷达到gps
		m_Calibrations.radarworld2Gps(radar_input, gpsresult);
		printf("GPS:[%3.8f,%3.8f] \n", gpsresult.longtitude, gpsresult.latitude);
		printf("GPS:[%3.8f,%3.8f] \n", gpsresult.longtitude, gpsresult.latitude);
		printf("GPS:[%3.8f,%3.8f] \n", gpsresult.longtitude, gpsresult.latitude);

		// 其实是为了得到雷达的Z，这绕了一步
		m_Calibrations.Gps2radarworld(gpsresult, radarworld);

		UcitCalibrate::WorldDistance radardistance;
		radardistance.X = radarworld.X;
		radardistance.Y = radarworld.Y;
		radardistance.Height = radarworld.Distance;

		m_Calibrations.Distance312Pixel(radardistance, radpixe);
		cv::circle(sourceImage, radpixe, 8, cv::Scalar(0, 0, 100), -1, cv::LINE_8);
		radarprojectpixle.push_back(radpixe);

	}

	char dayindir[256];
	char prezifu[50];
	//计算与真值的偏差
	int Nonbiasnum = 0;
	for (int i = 0; i < boxPoints.size(); i++)
	{
		dircs m_dirs = judgedirection(radarprojectpixle[i], boxPoints[i], 110);
		if (strcmp(m_dirs.m_drirs.c_str(), "雷达返投点位不偏") != 0)
		{
			Nonbiasnum++;
		}
		recorddistance.radardistance = sqrt(pow((radarprojectpixle[i].x - boxPoints[i].x), 2) + pow((radarprojectpixle[i].y - boxPoints[i].y), 2));
		error_means[i].radardistance = recorddistance.radardistance;
		printf("distance radarproject[%d]:%f \n", i, recorddistance.radardistance);
		strcpy(dayindir, m_dirs.m_drirs.c_str());
		int textnum = sizeof(m_dirs.m_drirs);
		sprintf(prezifu, "点:%d", i + 1);
		//strcpy(dayindir+textnum, prezifu);
		strcat(dayindir, "----->");
		strcat(dayindir, prezifu);
		putText::putTextZH(xiezitu, dayindir, cv::Point(100, 100 + i * 50), cv::Scalar(255, 0, 0), 30, "微软雅黑");
		//cv::putText(xiezitu, dayindir, cv::Point(100, 100 + i * 50), 0, 1, cv::Scalar(255, 0, 0), 3);
	}

	putText::putTextZH(hints, "棕色是雷达值投影到图像的值", cv::Point(100, 300), cv::Scalar(0, 0, 100), 40, "隶书");

	if ((float)Nonbiasnum / (float)boxPoints.size() > 0.2)
	{
		putText::putTextZH(xiezitu, "雷达返投过多的偏离点，需要重新采点", \
			cv::Point(10, 700), \
			cv::Scalar(0, 0, 255), 30, \
			"微软雅黑");
		putText::putTextZH(xiezitu, ",具体看下图提示，同时采集相应的gps值", \
			cv::Point(10, 800), \
			cv::Scalar(0, 0, 255), 30, \
			"微软雅黑");
		//cv::putText(xiezitu, "Too much Nonbias points", cv::Point(100,700), 0, 1.5, cv::Scalar(255, 0, 0), 3);
	}
	else
	{
		putText::putTextZH(xiezitu, "雷达基本满足标定要求，点的偏移个数在接受范围内", \
			cv::Point(10, 700), \
			cv::Scalar(255, 0, 0), 30, \
			"微软雅黑");
		putText::putTextZH(xiezitu, "，下面具体查看3组数据整体偏差图", \
			cv::Point(10, 800), \
			cv::Scalar(255, 0, 0), 30, \
			"微软雅黑");
	}
	cv::namedWindow("debugoffset", 0);
	cv::imshow("debugoffset", xiezitu);

	cv::waitKey(0);
	cv::destroyWindow("debugoffset");

#endif


}
	
	


if (strcmp("MEC", mode.c_str()) == 0)
{
	// 反投影为了验证 gps到pixel 显示
#if project2pixeltest
	std::vector<cv::Point2d> gpsprojects;
	//测试之前采的rtk  gps绝对坐标准不准,GPS反投影,绿色点
	for (int i = 1; i < mp_Gpslat.size() + 1; i++)
	{
		GpsWorldCoord  m_gps;
		longandlat m_longandtemp;
		m_longandtemp.longtitude = mp_Gpslong[i];
		m_longandtemp.latitude = mp_Gpslat[i];

		m_Calibrations.Gps2radarworld(m_longandtemp, m_gps);
		cv::Point3d radartemp;
		radartemp.x = m_gps.X;
		radartemp.y = m_gps.Y;
		radartemp.z = m_gps.Distance;
		cv::Point2d radpixell;
		UcitCalibrate::WorldDistance worldDistance;
		worldDistance.X = radartemp.x;
		worldDistance.Y = radartemp.y;
		worldDistance.Height = radartemp.z;
		m_Calibrations.Distance312Pixel(worldDistance, radpixell);
		std::string raders = "radarPoints";
		sprintf(textbuf, "GPS%d[%3.8f,%3.8f]", i, m_longandtemp.longtitude, m_longandtemp.latitude);
		//putText(sourceImage, textbuf, cv::Point((int)radpixell.x - 100, (int)radpixell.y - 30), 0, 1, cv::Scalar(0, 0, 0), 2);
		std::cout << "GPS:\t" << i << "'s pixels:" << radpixell.x << "\t" << radpixell.y << endl;
		cv::circle(sourceImage, radpixell, 7, cv::Scalar(0, 255, 0), -1, cv::LINE_AA);
		gpsprojects.push_back(radpixell);
	}

	for (int i = 0; i < boxPoints.size(); i++)
	{
		recorddistance.gpsdistance = sqrt(pow((gpsprojects[i].x - boxPoints[i].x), 2) + pow((gpsprojects[i].y - boxPoints[i].y), 2));
		error_means[i].gpsdistance = recorddistance.gpsdistance;
		printf("distance GPSproject[%d]:%f \n", i, recorddistance.gpsdistance);
	}

	putText::putTextZH(hints, "绿色是GPS投影到图像的值", cv::Point(100, 400), cv::Scalar(0, 255, 0), 40, "隶书");
	// sum up overall errors 
	std::map<string, double> anverage_distance;
	vector<meandistance>::iterator iter_mean_b = error_means.begin();
	vector<meandistance>::iterator iter_mean_e = error_means.end();
	int counts = 1;
	char strpoint[256];
	for (; iter_mean_b != iter_mean_e; iter_mean_b++)
	{
		double anveragedistance = (iter_mean_b->pixeldistance + iter_mean_b->radardistance + iter_mean_b->gpsdistance) / 3;
		sprintf(strpoint, "Point:%d", counts);
		anverage_distance.insert(make_pair(strpoint, anveragedistance));
		counts++;
	}


	// 将误差结果排序
	vector<PAIR> name_score_vec(anverage_distance.begin(), anverage_distance.end());
	//对vector排序  
	sort(name_score_vec.begin(), name_score_vec.end(), cmpsort());

	//排序前  
	map<string, double>::iterator iter_map;
	cout << "排序前:" << endl;
	for (iter_map = anverage_distance.begin(); iter_map != anverage_distance.end(); iter_map++)
		cout << iter_map->first << "\t" << iter_map->second << endl;


	char resultsdrawing[256];
	cout << "排序后:" << endl;
	for (int i = 0; i != name_score_vec.size(); ++i) {
		//可在此对按value排完序之后进行操作  
		cout << name_score_vec[i].first << "\t" << name_score_vec[i].second << endl;

		std::ostringstream stream;
		stream << name_score_vec[i].first;
		// 补充一个空格
		stream << setw(10) << setfill(' ');
		stream << name_score_vec[i].second;
		std::string display = stream.str();
		if (name_score_vec[i].second > 100)
		{
			cv::putText(sourceImage, display, cv::Point(50, 200 + i * 50), 0, 1.6, cv::Scalar(0, 0, 255), 3);
			putText::putTextZH(sourceImage, "误差太大", cv::Point(600, 150 + i * 50), cv::Scalar(0, 200, 255), 40, "微软雅黑");
		}
		else
		{
			cv::putText(sourceImage, display, cv::Point(50, 200 + i * 50), 0, 1.5, cv::Scalar(100, 255, 0), 3);
		}
	}
#endif
}



if (strcmp("MEC",mode.c_str())==0)
{
	#define project2gpstest 1
}
else
{
	#define project2gpstest 0
}

#if project2gpstest

		// 先搞三大组的gps
		int pointsize = m_Measures.size();
		inputgps* mp_inputs =(inputgps*) malloc(sizeof(inputgps) * 3);
		for (int i =0; i<3;i++)
		{
			mp_inputs[i].v_lonandlat = (longandlat*)malloc(sizeof(longandlat) * pointsize);
		}
		longandlat tmp;

		mp_Gpslong, mp_Gpslat;
		std::map<int, double>::iterator iter_gpslon = mp_Gpslong.begin();
		std::map<int, double>::iterator iter_gpslat = mp_Gpslat.begin();

	
		gpsfile << "This is RawGPS showing as follows!" << "\n" << endl;
		int counterr = 0;
		for (; iter_gpslon != mp_Gpslong.end() && iter_gpslat != mp_Gpslat.end(); iter_gpslon++, iter_gpslat++)
		{
			printf("RawGPS[%.8f,%.8f] \n", iter_gpslon->second, iter_gpslat->second);
			gpsfile << std::setprecision(12) << iter_gpslat->second << "," << iter_gpslon->second << endl;
			strcpy(mp_inputs[0].m_color, "blue");
			mp_inputs[0].m_type = 0;
			mp_inputs[0].v_lonandlat[counterr].latitude = iter_gpslat->second;
			mp_inputs[0].v_lonandlat[counterr].longtitude = iter_gpslon->second;

			counterr++;
		}


		//  先计算图像到世界
		cv::Point3d m_worldcoord;
		longandlat pixel_gps,radar_gps;
		gpsfile << "This is Pixel2Gps showing as follows!" << "\n" << endl;
		int counterp = 0;
		for (; iter_origpixel != iter_origpixel_e; iter_origpixel++)
		{
			//m_Calibrations.CameraPixel2World(*iter_origpixel,m_worldcoord);
			//printf("m_worldcoord[%.5f,%.5f,%.5f] \n", m_worldcoord.x, m_worldcoord.y,m_worldcoord.z);
			// 再转到GPS 坐标
			m_Calibrations.CameraPixel2Gps(*iter_origpixel, pixel_gps);
			printf("pixel2GPS[%.8f,%.8f] \n", pixel_gps.longtitude, pixel_gps.latitude);
			gpsfile << std::setprecision(12) << pixel_gps.latitude << "," << pixel_gps.longtitude << endl;
			tmp.latitude = pixel_gps.latitude;
			tmp.longtitude = pixel_gps.longtitude;
			strcpy(mp_inputs[1].m_color, "green");
			mp_inputs[1].m_type = 1;
			mp_inputs[1].v_lonandlat[counterp].latitude = pixel_gps.latitude;
			mp_inputs[1].v_lonandlat[counterp].longtitude = pixel_gps.longtitude;
			counterp++;
		}


		gpsfile << "This is radar2GPS showing as follows!" << "\n" << endl;
		// 计算从雷达原始到GPS
		int counteradar = 0;

		std::map<int, cv::Point3d>::iterator iter_begin1 = m_Measures.begin();
		std::map<int, cv::Point3d>::iterator iter_end1 = m_Measures.end();
		for (; iter_begin1 != iter_end1; iter_begin1++)
		{
			cv::Point3d radartmp;
			radartmp.x = iter_begin1->second.x;
			radartmp.y = iter_begin1->second.y;
			radartmp.z = iter_begin1->second.z;
			cv::Point2d radpixe;
			GpsWorldCoord radar_input, radarworld;
			radar_input.X = radartmp.x;
			radar_input.Y = radartmp.y;
			// input distance 有可能要修改
			radar_input.Distance = 0;
			// 反算雷达到gps
			m_Calibrations.radarworld2Gps(radar_input, radar_gps);
			printf("radar2GPS:[%3.8f,%3.8f] \n", radar_gps.longtitude, radar_gps.latitude);

			strcpy(mp_inputs[2].m_color, "red");
			mp_inputs[2].m_type = 2;
			mp_inputs[2].v_lonandlat[counteradar].longtitude = radar_gps.longtitude;
			mp_inputs[2].v_lonandlat[counteradar].latitude = radar_gps.latitude;
			counteradar++;
			gpsfile << std::setprecision(12) << radar_gps.latitude << "," << radar_gps.longtitude << endl;
		}

		// 写入KML files
		std::string m_filepath = "../results/gps.KML";
		GpsKmlGenerator::Instance().writegpskml(mp_inputs,3, pointsize,m_filepath);

		for (int j=0; j<3;j++)
		{
			if (mp_inputs[j].v_lonandlat!=nullptr)
			{
				free(mp_inputs[j].v_lonandlat);
				mp_inputs[j].v_lonandlat == nullptr;
			}
		}
		if (mp_inputs!=nullptr)
		{
			free(mp_inputs);
			mp_inputs == nullptr;
		}
		
		std::cout << "gps thing process done!!" << std::endl;

#endif

#if 0
		cv::Point2d  m_Distancepixel;
		// 计算边界值，y = 0的对应的区域
		cv::Point2d raderpixelPoints, point1, pointend;
		for (float i = -100; i < 100; i += 0.5)
		{
			UcitCalibrate::WorldDistance worldDistance;
			worldDistance.X = i;
			worldDistance.Y = 0;
			worldDistance.Height = 1.2;
			if (i == -9)
			{
				m_Calibrations.Distance312Pixel(worldDistance, point1);
				raderpixelPoints = point1;
			}
			if (i == 8.0)
			{
				m_Calibrations.Distance312Pixel(worldDistance, pointend);
				raderpixelPoints = pointend;
			}
			m_Calibrations.Distance312Pixel(worldDistance, raderpixelPoints);
			std::string raders = "radarPoints";

			cv::circle(sourceImage, raderpixelPoints, 9, cv::Scalar(0, 100, 0), -1, cv::LINE_AA);
			sprintf(textbuf, "%3.1f", i);
			cv::putText(sourceImage, textbuf, cv::Point((int)raderpixelPoints.x - 80, (int)raderpixelPoints.y - 30), 0, 1, cv::Scalar(0, 0, 255), 2);

		}
		BlindArea results;
		cv::line(sourceImage, pointend, point1, cv::Scalar(100, 0, 200), 3);


#endif
		
	std::string savep = savepath + "/" + "save.jpg";
	cv::imshow("hints", hints);
	cv::imwrite(savep, sourceImage);
	cv::imshow("raw image", sourceImage);
	cv::waitKey(0);
	cv::destroyAllWindows();
	outfile.close();
	gpsfile.close();
    std::system("PAUSE");
    return 0;
}

