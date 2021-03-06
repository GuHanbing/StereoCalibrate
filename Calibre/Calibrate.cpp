#include"stdafx.h"
#include <opencv2/opencv.hpp>  
#include <stdio.h>  
using namespace cv;
using namespace std;

void calDisparity(Rect leftROI, Rect rightROI, Mat _left, Mat _right, Mat & disparity)
{
	cvtColor(_left, _left, COLOR_BGR2GRAY);
	cvtColor(_right, _right, COLOR_BGR2GRAY);
	cv::Ptr<cv::StereoBM> bm = cv::StereoBM::create(16, 9);
	bm->setPreFilterType(CV_STEREO_BM_XSOBEL); 
	bm->setPreFilterSize(9);
	bm->setPreFilterCap(31);
	bm->setBlockSize(15);
	bm->setMinDisparity(0);
	bm->setNumDisparities(64);
	bm->setTextureThreshold(10);
	bm->setUniquenessRatio(10);
	bm->setSpeckleWindowSize(100);
	bm->setSpeckleRange(32);
	bm->setROI1(leftROI);
	bm->setROI2(rightROI);

	copyMakeBorder(_left, _left, 0, 0, 80, 0, IPL_BORDER_REPLICATE); 
	copyMakeBorder(_right, _right, 0, 0, 80, 0, IPL_BORDER_REPLICATE);
	bm->compute(_left, _right, disparity);
	//disparity = disparity.colRange(80, _left.cols);
	disparity.convertTo(disparity, CV_32F, 1.0 / 16);
}

void mergeImg(cv::Mat &dst, cv::Mat &src1, cv::Mat &src2)
{
	int rows = src1.rows>src2.rows ? src1.rows : src2.rows;//合成图像的行数
	int cols = src1.cols + 20 + src2.cols;  //合成图像的列数
	CV_Assert(src1.type() == src2.type());
	cv::Mat zeroMat = cv::Mat::zeros(rows,cols, src1.type());
	zeroMat.copyTo(dst);
	src1.copyTo(dst(cv::Rect(0, 0, src1.cols, src1.rows)));
	src2.copyTo(dst(cv::Rect(src1.cols + 20, 0, src2.cols, src2.rows)));//两张图像之间相隔20个像素
	double score = 89.101;
	char info[256];
	sprintf(info, "score=%.2f", score);
	cv::putText(dst, info, cv::Point(2, 50), CV_FONT_HERSHEY_COMPLEX, 1, cv::Scalar(255, 0, 0));
	cv::imshow("合成图像", dst);
}
int boardWidth, boardHeight;
int boardTotal;
CvSize boardSize;
CvSize squareSize;
CvPoint2D32f * image_points_buf;
Mat srcColorL, srcGrayL, srcColorR, srcGrayR;
IplImage srcIp;
CvMat cvmatSrc;
int nowNumber, found;
char* left_imgN[15] = { "l1.jpg","l2.jpg","l3.jpg","l4.jpg","l5.jpg","l6.jpg","l7.jpg","l8.jpg","l9.jpg","l10.jpg","l11.jpg","l12.jpg","l13.jpg","l14.jpg","l15.jpg" };
char* right_imgN[15] = { "r1.jpg","r2.jpg","r3.jpg","r4.jpg","r5.jpg","r6.jpg","r7.jpg","r8.jpg","r9.jpg","r10.jpg","r11.jpg","r12.jpg","r13.jpg","r14.jpg","r15.jpg" };
vector < vector <Point2f> > all_image_points_buf;
vector < vector <Point2f> > left_image_points_buf;
vector < vector <Point2f> > right_image_points_buf;
vector < vector <Point3f> > object_Points;
void initFindCorner() {
	boardSize = cvSize(9, 6);
	boardWidth = boardSize.width;
	boardHeight = boardSize.height;
	boardTotal = boardWidth*boardHeight;
	squareSize = cvSize(20,20);
	image_points_buf = new CvPoint2D32f[boardTotal];
	vector <Point3f> tempPointSet;
	for (int t = 0; t<10; t++)
	{
		for (int j = 0; j<boardHeight; j++)
		{	
		
			for (int i = 0; i<boardWidth; i++)
			{
				/* 假设定标板放在世界坐标系中z=0的平面上 */
				Point3f tempPoint;
				tempPoint.x = i*squareSize.width;
				tempPoint.y = j*squareSize.height;
				tempPoint.z = 0;
				tempPointSet.push_back(tempPoint);

			}
		}

		object_Points.push_back(tempPointSet);
		tempPointSet.clear();
	}
}

Size image_size;
int findCornersWork(int i) {
	Mat lChess, rChess, comChess;
	vector <Point2f>  per_image_points_buf;
	//left
	srcColorL = imread(left_imgN[i], CV_LOAD_IMAGE_COLOR);
	//imshow("源图像", srcColorL);
	image_size = srcColorL.size();
	cvtColor(srcColorL, srcGrayL, COLOR_BGR2GRAY);
	//imshow("灰阶图", srcGray);
	
	
	found = findChessboardCorners(srcGrayL, boardSize, per_image_points_buf,
		CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS);
	//printf("捕获角点数量:%d\n", nowNumber);
	if (found == 0)
	{
		cout << "f:" << left_imgN[i] << endl;
		return 0;
	}
	cornerSubPix(srcGrayL,  per_image_points_buf,  Size(11, 11),Size(-1, -1),
		TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
	drawChessboardCorners(srcColorL, boardSize, per_image_points_buf, found);
	//imshow(left_imgN[i], srcColorL);
	left_image_points_buf.push_back(per_image_points_buf);
	per_image_points_buf.clear();


	//right
	srcColorR = imread(right_imgN[i]);
	//imshow("源图像", srcColor);
	image_size = srcColorR.size();
	cvtColor(srcColorR, srcGrayR, COLOR_BGR2GRAY);
	//imshow("灰阶图", srcGray);

	
	found = findChessboardCorners(srcGrayR, boardSize, per_image_points_buf,
		CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS);
	//printf("捕获角点数量:%d\n", nowNumber);
	if (found == 0)
	{
		cout << "f:" << right_imgN[i] << endl;
		return 0;
	}
	cornerSubPix(srcGrayR, per_image_points_buf, Size(11, 11), Size(-1, -1),
		TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
	drawChessboardCorners(srcColorR, boardSize, per_image_points_buf, found);
	//imshow(right_imgN[i], srcColorR);
	right_image_points_buf.push_back(per_image_points_buf);
	per_image_points_buf.clear();
	return 0;
}

Mat intrinsic_matrix;
Mat distortion_coeffs;
vector<Mat> rotation_vectors;
vector<Mat> translation_vectors;
int main(int argc, char* argv[]) {
	Mat K1, K2, R, F, E;
	Vec3d T;
	Mat D1, D2;
	cv::Mat R1, R2, P1, P2, Q;

	cv::Mat lmapx, lmapy, rmapx, rmapy;
	cv::Mat imgU1, imgU2, img1, img2;
	initFindCorner();
	for(int i=0;i<10;++i)
	  findCornersWork(i);
	double err1 = calibrateCamera(object_Points, left_image_points_buf, image_size, K1, D1, rotation_vectors, translation_vectors, CALIB_ZERO_TANGENT_DIST);
	double err2 = calibrateCamera(object_Points, right_image_points_buf, image_size, K2, D2, rotation_vectors, translation_vectors, CALIB_ZERO_TANGENT_DIST);
	//cout << "monoculor calibration" << endl;
	//cout << err1 << "   " << err2 << endl;
	//cout << K1 << endl;
	//cout << D1 << endl;
	//cout << K2 << endl;
	//cout << D2 << endl;


	////Mat src= imread("l1.jpg");
	////Mat dst;
	////undistort(src, dst, K1, D1);
	////imshow("畸变图像", src);
	////imshow("校正图像",dst);

	//
	Rect RoiL,RoiR;
	stereoCalibrate(object_Points, left_image_points_buf, right_image_points_buf, K1, D1, K2, D2, image_size, R, T, E, F, CALIB_FIX_INTRINSIC);

	//cout << "monoculor calibration" << endl;
	//cout << "K1:" << K1 << endl;
	//cout << "D1:" << D1 << endl;
	//cout << "K2:" << K2 << endl;
	//cout << "D2:" << D2 << endl;
	//cout << "R:" << R << endl;
	//cout << "T:" << T << endl;


	stereoRectify(K1, D1, K2, D2, image_size, R, T, R1, R2, P1, P2, Q, 0/*CALIB_ZERO_DISPARITY*/, 0, image_size,&RoiL,&RoiR);

	cout << "stereoRectify:" << endl;
	P2.at<double>(0, 3) = 0;
	//cout << "P1:" << P1 << endl;
	//cout << "P2:" << P2 << endl;
	//cout << "ROI:" << RoiL << endl;
	//cout << "ROI:" << RoiR << endl;

	//FileStorage fs1;
	//fs1.open("CameraData.xml", FileStorage::WRITE);
	//fs1 << "K1" << K1 << "D1" << D1 << "R1" << R1 << "P1" << P1;
	//fs1 << "K2" << K2 << "D2" << D2 << "R2" << R2 << "P2" << P2;
	//fs1.release();
	//FileStorage fs2;
	//fs2.open("CameraData.xml", FileStorage::READ);
	//fs2["K1"] >> K1;
	//fs2["D1"] >> D1;
	//fs2["R1"] >> R1;
	//fs2["P1"] >> P1;
	//fs2["K2"] >> K2;
	//fs2["D2"] >> D2;
	//fs2["R2"] >> R2;
	//fs2["P2"] >> P2;
	//fs2.release();
	img1 = imread("ltest.jpg");
	img2 = imread("rtest.jpg");

	//cout << "rectify:" << endl;
	//cout << "K1:" << K1 << endl;
	//cout << "D1:" << D1 << endl;
	//cout << "R1:" << R1 << endl;
	//cout << "P1:" << P1 << endl;
	//cout << "K2:" << K2 << endl;
	//cout << "D2:" << D2 << endl;
	//cout << "R2:" << R2 << endl;
	//cout << "P2:" << P2 << endl;

	cv::initUndistortRectifyMap(K1, D1, R1, P1, image_size, CV_32F, lmapx, lmapy);
	cv::initUndistortRectifyMap(K2, D2, R2, P2, image_size, CV_32F, rmapx, rmapy);
	cv::remap(img1, imgU1, lmapx, lmapy, cv::INTER_LINEAR);
	cv::remap(img2, imgU2, rmapx, rmapy, cv::INTER_LINEAR);
	imshow("原图-左", img1);
	imshow("原图-右", img2);
	imshow("校正-左", imgU1);
	imshow("校正-右", imgU2);
	Mat imgdis;
	calDisparity(RoiL, RoiR, imgU1, imgU2, imgdis);
	imshow("视差图", imgdis);

	waitKey();

	return 0;
}