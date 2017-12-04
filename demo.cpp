// Copyright 2014-2015 Isis Innovation Limited and the authors of gSLICr

#include <time.h>
#include <stdio.h>

#include "gSLICr_Lib/gSLICr.h"
#include "NVTimer.h"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/opencv.hpp"




using namespace std;
using namespace cv;

void load_image(const Mat& inimg, gSLICr::UChar4Image* outimg)
{
	gSLICr::Vector4u* outimg_ptr = outimg->GetData(MEMORYDEVICE_CPU);

	for (int y = 0; y < outimg->noDims.y;y++)
		for (int x = 0; x < outimg->noDims.x; x++)
		{
			int idx = x + y * outimg->noDims.x;
			outimg_ptr[idx].b = inimg.at<Vec3b>(y, x)[0];
			outimg_ptr[idx].g = inimg.at<Vec3b>(y, x)[1];
			outimg_ptr[idx].r = inimg.at<Vec3b>(y, x)[2];
		}
}

void load_image(const gSLICr::UChar4Image* inimg, Mat& outimg)
{
	const gSLICr::Vector4u* inimg_ptr = inimg->GetData(MEMORYDEVICE_CPU);

	for (int y = 0; y < inimg->noDims.y; y++)
		for (int x = 0; x < inimg->noDims.x; x++)
		{
			int idx = x + y * inimg->noDims.x;
			outimg.at<Vec3b>(y, x)[0] = inimg_ptr[idx].b;
			outimg.at<Vec3b>(y, x)[1] = inimg_ptr[idx].g;
			outimg.at<Vec3b>(y, x)[2] = inimg_ptr[idx].r;
		}
}


int main()
{
	cout << "hell";
	VideoCapture cap(0);


	if (!cap.isOpened()) 
	{
		cerr << "unable to open camera!\n";
		return -1;
	}
	

	// gSLICr settings
	gSLICr::objects::settings my_settings;

	my_settings.img_size.x = 700;
	my_settings.img_size.y = 700;
	my_settings.no_segs = 750;
	my_settings.spixel_size = 200;
	my_settings.coh_weight = 1.0f;
	my_settings.no_iters = 5;
	my_settings.color_space = gSLICr::CIELAB; // gSLICr::CIELAB for Lab, or gSLICr::RGB for RGB
	my_settings.seg_method = gSLICr::GIVEN_NUM; // or gSLICr::GIVEN_NUM for given number

	my_settings.do_enforce_connectivity = true; // whether or not run the enforce connectivity step

	// instantiate a core_engine
	gSLICr::engines::core_engine* gSLICr_engine = new gSLICr::engines::core_engine(my_settings);

	gSLICr::UChar4Image* in_img = new gSLICr::UChar4Image(my_settings.img_size, true, true);
	gSLICr::UChar4Image* out_img = new gSLICr::UChar4Image(my_settings.img_size, true, true);

	Size s(my_settings.img_size.x, my_settings.img_size.y);

	Size s1(640, 480);
	Mat oldFrame, frame;
	Mat boundry_draw_frame; boundry_draw_frame.create(s, CV_8UC3);

    //StopWatchInterface *my_timer; sdkCreateTimer(&my_timer);
   
	int key; int save_count = 0;
	while (cap.read(oldFrame))
	{

	float blue_sum[my_settings.no_segs*(3/2)]={0},green_sum[my_settings.no_segs*(3/2)]={0},red_sum[my_settings.no_segs*(3/2)]={0};
    int blue[my_settings.img_size.x][my_settings.img_size.y]={0};
	int green[my_settings.img_size.x][my_settings.img_size.y]={0};
	int red[my_settings.img_size.x][my_settings.img_size.y]={0};
	int matrix[490000]={0};
	

		resize(oldFrame, frame, s);
		
		load_image(frame, in_img);
        int count[200]={0};
        //sdkResetTimer(&my_timer); sdkStartTimer(&my_timer);
		gSLICr_engine->Process_Frame(in_img);
        //sdkStopTimer(&my_timer); 
        //cout<<"\rsegmentation in:["<<sdkGetTimerValue(&my_timer)<<"]ms"<<flush;
		gSLICr_engine->Draw_Segmentation_Result(out_img);
		cv::namedWindow("tt",0);
		load_image(out_img, boundry_draw_frame);
		cv::imshow("tt", boundry_draw_frame);
		
		gSLICr_engine->Write_Seg_Res_To_PGM("abc",matrix);
		Mat M = frame;
		Mat M2;
		for(int i=0;i<my_settings.img_size.x;i++)
		{
			for(int j=0;j<my_settings.img_size.y;j++)
			{
				blue[i][j]=M.at<cv::Vec3b>(i,j)[0]; // b
				green[i][j]=M.at<cv::Vec3b>(i,j)[1]; // g
				red[i][j]=M.at<cv::Vec3b>(i,j)[2]; // r
			}
		}
		
		for(int i=0;i<my_settings.img_size.x*my_settings.img_size.y;i++)
		{
			blue_sum[matrix[i]]+=blue[i/my_settings.img_size.x][i%my_settings.img_size.x];
			red_sum[matrix[i]]+=red[i/my_settings.img_size.x][i%my_settings.img_size.x];
			green_sum[matrix[i]]+=green[i/my_settings.img_size.x][i%my_settings.img_size.x];
			count[matrix[i]]++;
		}
		//---------------------------------------------------
		for(int i=0;i<my_settings.img_size.y;i++)
		{
			for(int j=0;j<my_settings.img_size.x;j++)
			{
				M.at<cv::Vec3b>(i,j)[0]=blue_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// b
				M.at<cv::Vec3b>(i,j)[1]=green_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// g
				M.at<cv::Vec3b>(i,j)[2]=red_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// r
			}
		}
		resize(M, M2, s1);
		 // M stores the averages
		imshow("test", M2);


		key = waitKey(1);
		if (key == 27) break;
		
	}

	destroyAllWindows();
    return 0;
}
