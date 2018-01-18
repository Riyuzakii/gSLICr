// Copyright 2014-2015 Isis Innovation Limited and the authors of gSLICr
/*
        creates a mask as per given values of 0s and 1s from train.py  and displaqys the image 
*/
#include <iostream>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include "gSLICr_Lib/gSLICr.h"
#include "NVTimer.h"
#include <queue>
#include <cmath>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/opencv.hpp"
#include "./gSLICr_Lib/objects/gSLICr_spixel_info.h"
#include "./gSLICr_Lib/engines/gSLICr_seg_engine_shared.h"

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


std::string ToString(int val)
{
    stringstream ss;
    ss<<val;
    return ss.str();
}

int main()
{
    // ========= Declaration of constant parameters
    gSLICr::objects::settings my_settings;
    my_settings.img_size.x = 500;
    my_settings.img_size.y = 500;
    my_settings.no_segs = 750;
    my_settings.spixel_size = 100;
    my_settings.coh_weight = 1.0f;
    my_settings.no_iters = 50;
    my_settings.color_space = gSLICr::CIELAB; // gSLICr::CIELAB for Lab, or gSLICr::RGB for RGB
    my_settings.seg_method = gSLICr::GIVEN_NUM; // or gSLICr::GIVEN_NUM for given number
    my_settings.do_enforce_connectivity = true; // whether or not run the enforce connectivity step
    int n = int(sqrt(my_settings.no_segs));    // instantiate a core_engine
    gSLICr::engines::core_engine* gSLICr_engine = new gSLICr::engines::core_engine(my_settings);
    gSLICr::UChar4Image* in_img = new gSLICr::UChar4Image(my_settings.img_size, true, true);
    gSLICr::UChar4Image* out_img = new gSLICr::UChar4Image(my_settings.img_size, true, true);
    Size s(my_settings.img_size.x, my_settings.img_size.y);
    Size s1(640, 480);

    //======================================================================================

    // ============ reading file name of the original unsegmented image 

        Mat oldFrame, frame;
        Mat boundry_draw_frame; boundry_draw_frame.create(s, CV_8UC3);
        int key, h=0;
        cin>>h;
        std::string first ("../dr/");
        std::string sec (".png");
        std::string mid = ToString(h);
        std::string name;
        name=first+mid+sec;
        oldFrame = cv::imread(name);

        int matrix[250000] = {0};

        resize(oldFrame, frame, s);      
        load_image(frame, in_img);
        gSLICr_engine->Process_Frame(in_img);
        gSLICr_engine->Draw_Segmentation_Result(out_img);
        load_image(out_img, boundry_draw_frame);
        gSLICr_engine->Write_Seg_Res_To_PGM("abc",matrix);

        for(int i=0;i<250000;i++){
                cout<<matrix[i]<<", ";
            }
    destroyAllWindows();
    return 0;
}