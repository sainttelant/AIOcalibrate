#pragma once
#include <opencv2/opencv.hpp>
#include <Eigen/Dense>
#include <opencv2/core/eigen.hpp>
#include"GpsKmlGenerator.hpp"




//using namespace cv;


namespace UcitCalibrate
{
	struct GpsWorldCoord
	{
		double X;
		double Y;
		// distacne 有时候也代表1.2m，注意区分
		double Distance;
	};

	struct WorldDistance
	{
		double X;
		double Y;
		double Height;
	};

	struct AIOradarCoord
	{
		double X;
		double Y;
		double Z;
	};

	

	struct RadarSpeed
	{
		double vx;
		double vy;
		double vz;
	};

	struct RadarHeading
	{
		double speed_value;
		double theta;
		std::string direction;
	};   

	// y =kx+b
	struct BlindArea
	{
		double k;
		double b;
	};

	enum Movingdirection
	{
		North,
		East,
		South,
		West,
		NorthEast,
		SouthEast,
		SorhtWest,
		NorthWest,
		Unkown
	};



	class CalibrationTool
	{

	public:
		static CalibrationTool &getInstance()
		{
			static CalibrationTool wl_UcitCalibration;
			return wl_UcitCalibration;
		};


		struct BoxSize
		{
			int xMin;
			int yMin;
			int xMax;
			int yMax;
		};  


		
		bool ReadParaXml(std::string m_strXmlPath, std::vector<BoxSize>& vecNode);

		bool ReadPickpointXml(std::string m_xmlpath, 
			std::vector<unsigned int>& pickpoints, 
			std::vector<cv::Point2d> &rawpoint, 
			std::map<int, cv::Point2d> &map_points, 
			std::map<int, double> &map_long, 
			std::map<int, double> &map_lan,
			double &reflectheight,
			double &installheight,
			std::map<int, cv::Point3d> &map_Measures,
			longandlat &originpoll,
			std::vector<double> &ghostdistort,
			cv::Mat &camerainstrinic,
			std::vector<double> &v_height,
			std::string &place);

		bool ReadCalibrateParam(std::string m_xmlpath,
			cv::Mat &raderRT44, 
			cv::Mat &cameraRT44,
			cv::Mat &cameraRT34,
			cv::Mat &cameraRT33, 
			cv::Mat &cameraRT31,
			longandlat& originpoll,
			std::vector<double>& ghostdistort,
			cv::Mat& camerainstrinic,
			double &m_handradarheight,
			double &m_radarinstallheight
			);

		void Generategps2world(std::vector<double>& P1_lo, \
			std::vector<double>& P1_la, \
			std::vector<double>& height, \
			std::vector<cv::Point3d> &Gpsworld4radar);

		// gps坐标转世界坐标系，不计算高度的，存到calibratetool 成员里面，为标camera做准备
		void Gps2WorldCoord(std::vector<double> P1_lo, std::vector<double> P1_la, std::vector<double>& m_gpsheights);
		void WorldCoord2Gps(std::vector<longandlat> &m_longandlat,std::vector<GpsWorldCoord> &m_gpsworld);



		void radarworld2Gps(GpsWorldCoord &m_gpsworldcoord, longandlat &m_gpslongandlat); 
		void Gps2radarworld(longandlat& m_gpslongandlat, GpsWorldCoord& m_gpsworldcoord);
		void CameraPixel2World(cv::Point2d m_pixels, cv::Point3d &m_world);
		void CameraPixel2Gps(cv::Point2d m_pixels, longandlat& m_longandlat);

		// AIO 像素到雷达
		void AIOcamerapixel2Radar(cv::Point2d m_pixels, AIOradarCoord &Radarvalue);
		void AIORadar2camerapixel(AIOradarCoord Radarvalue, cv::Point2d &m_pixels);

		// pixel 折算到3*1的距离值, 1: camerapixel points 2：输出x,y,z值，计算像素到雷达坐标系
		void Pixel2Distance31(cv::Point2d pixels, WorldDistance &Distances);
		void Distance312Pixel(WorldDistance Distances, cv::Point2d& pixels);

		// 


		void Gps2Pixel(GpsWorldCoord &m_gpscoord, cv::Point2d &pixels);

		bool CalculateBlind();
		BlindArea GetBlind();
		void SetWorldBoxPoints();
		std::vector<cv::Point3d> GetWorldBoxPoints(); 
		// choose selected points for calibration
		void PickRawGPSPoints4VectorPairs(std::vector<unsigned int> pointsSet, std::map<int, double>&Gps_longtitude, std::map<int, double>&Gps_latitudes);
		void PickMeasureMentValue4AIOcalib(std::vector<unsigned int> pointsSet, std::map<int, cv::Point3d> &measurements);
		
		void PickRadarMeasure4MECcalibRadarRT(std::vector<unsigned int> pointsSet, \
			std::map<int, cv::Point3d>& measurements);
		
		
		void PickImagePixelPoints4PnPsolve(std::vector<unsigned int>pointsSet, std::map<int, cv::Point2d>& imagesPixel);
		
		std::vector<cv::Point3d> GetMeasureMentPoint();
		void CalibrateCamera(bool rasac, std::string &mode, std::vector<unsigned int> pickPoints);

		void SetPi(double pai);
		double rad(double d);
		double deg(double x);
		// 经纬度之间距离
		double GetDistance(longandlat point1, longandlat point2);

		// 计算航向角,根据经纬度
		double CalculateHeading(longandlat point1, longandlat point2);

		// 雷达坐标系向东（x轴）为正，向北为（Y轴）正，z向上
		// 世界坐标系与雷达坐标系相同，存在一个偏转角，demo大概为60°
		void RadarSpeedHeading(RadarSpeed &m_speed, RadarHeading &m_radarhead); 

		//void RadarHeadings(RadarSpeed& m_speed, RadarHeading& m_radarhead);

		void SetRadarHeight(double radar_height);
		void SetCameraInstrinic(double fx, double fy, double cx, double cy);
		void SetCameraDiff(double df1, double df2, double df3, double df4, double df5);
		void SetCoordinateOriginPoint(double longitude, double latitude);
		void codeRotateByZ(double x, double y, double thetaz, double& outx, double& outy);
		void codeRotateByY(double x, double z, double thetay, double& outx, double& outz);
		void codeRotateByX(double y, double z, double thetax, double& outy, double& outz);
		cv::Point3d RotateByVector(double old_x, double old_y, double old_z, double vx, double vy, double vz, double theta);
		std::vector<GpsWorldCoord> GetGpsworlds();
		// radar2 world
		cv::Mat Get3DR_TransMatrix(const std::vector<cv::Point3d>& srcPoints, const std::vector<cv::Point3d>& dstPoints);
		cv::Mat GetRadarRTMatrix();
		cv::Mat GetCameraRT44Matrix();
		bool SetCameraRT44(cv::Mat CmRT44);
		bool SetRadarRT44(cv::Mat RadRT44);
		bool SetCameraRT33(cv::Mat CmRT33);
		bool SetCameraTMatrix(cv::Mat CTmatrix);

		std::vector<cv::Point3d> m_worldBoxPoints;
		// calibrate for AIO 
		std::vector<cv::Point2d> baseRadarPoints;

		std::vector<double> gps_longPick;
		std::vector<double> gps_latiPick;
		// measures_pick used for AIO calibrate
		std::vector<cv::Point3d> measures_pick;

		std::vector<cv::Point3d> radarmeasure_pick4radarRT;


		std::vector<cv::Point2f> imagePixel_pick;
		cv::Mat  m_cameraRMatrix;
		cv::Mat m_cameraRMatrix33;
		cv::Mat  m_cameraTMatrix;
		cv::Mat  m_cameraintrisic;
	
	private:
		CalibrationTool();
		virtual ~CalibrationTool();
		std::vector<GpsWorldCoord> m_gpsworlds;
		// 用gps计算得到的数据构造以下
		
		double m_PI;
		double m_earthR;
		double m_earthR_Polar;
		double m_earthrL;
		double m_earthrS;
		double m_originlongitude;
		double m_originlatitude;
		double m_radarheight;	
		cv::Mat  m_cameradiff;
		cv::Mat  m_RadarRT;
		cv::Mat  m_cameraRTMatrix44;
		UcitCalibrate::BlindArea m_blind;
		
	};

};

