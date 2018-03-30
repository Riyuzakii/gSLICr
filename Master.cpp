// Copyright 2014-2015 Isis Innovation Limited and the authors of gSLICr

//This file can be used to apply hysteresis thresholding on the image obtained after segmentation and averaging!

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

int blues[7200]={0}, reds[7200]={0}, greens[7200]={0};
void print(float rs[],float bs[], float gs[] ,int c[],int x,int y,int n)
{
   if(c[n*x+y]!=0 && (rs[n*x+y]/c[n*x+y]>16 || bs[n*x+y]/c[n*x+y]>16 || gs[n*x+y]/c[n*x+y]>16)){
        reds[n*x+y]=(int)(rs[n*x+y]/c[n*x+y]);
        blues[n*x+y]=(int)(bs[n*x+y]/c[n*x+y]);
        greens[x*n+y]=(int)(gs[n*x+y]/c[n*x+y]);
    }
    else {
        reds[n*x+y] = rs[n*x+y]=100*c[n*x+y];
        blues[n*x+y] = bs[n*x+y]=50*c[n*x+y]; // why not only 100
        greens[x*n+y] = gs[n*x+y]=150*c[n*x+y];
        //cout<<"100 150 50 ";
    }
}
///
    //Defining here to use everywhere without passing through functions
    int size_x=800;
    int size_y=800;
    int matrix[800*800] = {0};
    int superFlag=-1;
    int superID=-1;

void CallBackFunc(int event, int x, int y, int flags, void* userdata)
{
     
    if  ( event == EVENT_LBUTTONDOWN )
    {
        //cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
        superFlag=1;
        superID =matrix[x+y*size_x];
        //cout<<superFlag<<endl;
    }
    else if  ( event == EVENT_RBUTTONDOWN )
    {
        //cout << "Right button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
        superFlag = 0;
        superID= matrix[x + y*size_x];
        //cout<<superFlag<<endl;
    }
    
}

    float blue_sum[1600*(2)] = {0};
    float green_sum[1600*(2)] = {0};
    float red_sum[1600*(2)] = {0};
    int blue[800][800] = {0};
    int green[800][800] = {0};
    int red[800][800]= {0};
    int sum_x[1600] = {0};
    int sum_y[1600] = {0};

int main()
{
    gSLICr::objects::settings my_settings;
    int height=40;
    int length=40;
    my_settings.img_size.x = size_x;
    my_settings.img_size.y = size_y;
    my_settings.no_segs = length*height;
    my_settings.spixel_size = 8;
    my_settings.coh_weight = 0.1f;
    my_settings.no_iters = 5;
    my_settings.color_space = gSLICr::CIELAB; // gSLICr::CIELAB for Lab, or gSLICr::RGB for RGB
    my_settings.seg_method = gSLICr::GIVEN_NUM; // or gSLICr::GIVEN_NUM for given number
    my_settings.do_enforce_connectivity = true; // whether or not run the enforce connectivity step
    int n = int(sqrt(my_settings.no_segs));

    // instantiate a core_engine
    gSLICr::engines::core_engine* gSLICr_engine = new gSLICr::engines::core_engine(my_settings);
    gSLICr::UChar4Image* in_img = new gSLICr::UChar4Image(my_settings.img_size, true, true);
    gSLICr::UChar4Image* out_img = new gSLICr::UChar4Image(my_settings.img_size, true, true);


    Size s(my_settings.img_size.x, my_settings.img_size.y);
    //Size s1(640, 480);
    Size s1(size_x,size_y);
    Mat oldFrame, frame;
    Mat boundry_draw_frame; boundry_draw_frame.create(s, CV_8UC3);
    int key,h=0;
    cin>>h;
    std::string first ("./5.33pm_20deg/frame000");
    std::string sec (".jpg");
    std::string mid = ToString(h);
    std::string name;
    name=first+mid+sec;
    oldFrame = cv::imread(name);
    //cout<<Lvalue<<"+"<<Hvalue<<"-\n";
    
   
    
    int count[my_settings.no_segs*3]={0};
    resize(oldFrame, frame, s);
    
    load_image(frame, in_img);
    gSLICr_engine->Process_Frame(in_img);
    gSLICr_engine->Draw_Segmentation_Result(out_img);
    load_image(out_img, boundry_draw_frame);
    cv::namedWindow("InitialImg",0);
    cv::imshow("InitialImg", frame);

    gSLICr_engine->Write_Seg_Res_To_PGM("abc",matrix);
    Mat M = frame;
    Mat M2,Mimg;
    int lable;
    int prev_lable[my_settings.no_segs]={0};
    ///Retrieving colour channels from the jimage
    for(int i=0;i<my_settings.img_size.x;i++)
    {
        for(int j=0;j<my_settings.img_size.y;j++)
        {
            blue[i][j] = M.at<cv::Vec3b>(i,j)[0]; // b
            green[i][j] = M.at<cv::Vec3b>(i,j)[1]; // g
            red[i][j] = M.at<cv::Vec3b>(i,j)[2]; // r
        }
    }
    
    ///Summing over the pixel Lvalues of all segments
    for(int i=0;i<my_settings.img_size.x*my_settings.img_size.y;i++)
    {
        blue_sum[matrix[i]]+= blue[i/my_settings.img_size.x][i%my_settings.img_size.x];
        red_sum[matrix[i]]+= red[i/my_settings.img_size.x][i%my_settings.img_size.x];
        green_sum[matrix[i]]+= green[i/my_settings.img_size.x][i%my_settings.img_size.x];
        sum_x[matrix[i]]+=  ( i/my_settings.img_size.x);
        sum_y[matrix[i]]+= (i%my_settings.img_size.x);
        count[matrix[i]]++;
    }
     for(int x=0;x<length;x++)
        for(int y=0;y<height;y++){
        print(red_sum, blue_sum,green_sum, count, x,y,n);
    }
    
    for(int i=0;i<my_settings.img_size.y;i++)
    {
        for(int j=0;j<my_settings.img_size.x;j++)
        {
            if(count[matrix[i*my_settings.img_size.x + j]]==0){
                M.at<cv::Vec3b>(i,j)[0] = 0;//blue_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// b
                M.at<cv::Vec3b>(i,j)[1] = 0;//green_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// g
                M.at<cv::Vec3b>(i,j)[2] = 0;//red_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// r

            }
            else{
                M.at<cv::Vec3b>(i,j)[0] = blue_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// b
                M.at<cv::Vec3b>(i,j)[1] = green_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// g
                M.at<cv::Vec3b>(i,j)[2] = red_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// r
            }
        }
    }
    std::string newname(".png");
    std::string newname2("_.png");
    string newest,newest2;
    newest = mid + newname;
    newest2 = mid + newname2;
    cv::imwrite(newest2,M);
    while(1){
        cv::namedWindow("Binary",5);
        setMouseCallback("Binary", CallBackFunc, NULL);
        
        blue_sum[superID]= 255*superFlag*count[superID];
        red_sum[superID]= 255*superFlag*count[superID];
        green_sum[superID]= 255*superFlag*count[superID];
        
        ///Re-inserting the average Lvalues back into the image
        for(int i=0;i<my_settings.img_size.y;i++)
    {
        for(int j=0;j<my_settings.img_size.x;j++)
        {
            if(count[matrix[i*my_settings.img_size.x + j]]==0){
                M.at<cv::Vec3b>(i,j)[0] = 0;//blue_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// b
                M.at<cv::Vec3b>(i,j)[1] = 0;//green_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// g
                M.at<cv::Vec3b>(i,j)[2] = 0;//red_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// r

            }
            else{
                M.at<cv::Vec3b>(i,j)[0] = blue_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// b
                M.at<cv::Vec3b>(i,j)[1] = green_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// g
                M.at<cv::Vec3b>(i,j)[2] = red_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// r
            }
        }
    }
        Mat M3;
        resize(M, M3, s1);
        //cv::imshow("FinalImg",M2);
        cv::imshow("Binary",M3);
        key = (char)waitKey(1);
        if (key == 27) break;
        
    }
    
    int new_lable[my_settings.no_segs]={0};
    
    for(int i=0;i<my_settings.img_size.y;i++)
        {
            for(int j=0;j<my_settings.img_size.x;j++)
            {
                int bs,gs,rs;
                if(count[matrix[i*my_settings.img_size.x + j]]==0){
                    bs = 0;
                    gs = 0;
                    rs = 0;
                }
                else{
                    bs = blue_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// b
                    gs = green_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// g
                    rs = red_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// r
                }

                if(bs!=255 && gs!=255 && rs!=255){
                    bs=gs=rs=0;
                    M.at<cv::Vec3b>(i,j)[0] = bs;// b
                    M.at<cv::Vec3b>(i,j)[1] = gs;// g
                    M.at<cv::Vec3b>(i,j)[2] = rs;// r
                    new_lable[matrix[i*my_settings.img_size.x + j ]]=0;
                }
                else new_lable[matrix[i*my_settings.img_size.x + j ]]=1;
        }
    }
    pair<int, int> Max_x[my_settings.no_segs], Max_y[my_settings.no_segs], Min_x[my_settings.no_segs], Min_y[my_settings.no_segs]; 
    
    for (int i=0;i<my_settings.no_segs;i++)
    {
        Max_x[i].first=0;
        Max_x[i].second=0;

        Max_y[i].first=0;
        Max_y[i].second=0;
        
        Min_x[i].first=2000;
        Min_x[i].second=2000;
        
        Min_y[i].first=2000;
        Min_y[i].second=2000;
    }

    for(int i=0;i<my_settings.img_size.y;i++)
        {
            for(int j=0;j<my_settings.img_size.x;j++)
            {
                int l=matrix[i*my_settings.img_size.x + j];
                int xl = j;
                int yl = i;
                if(Max_x[l].first < xl)
                    Max_x[l] = make_pair(xl, yl);
                if(Max_y[l].second < yl)
                    Max_y[l] = make_pair(xl, yl);
                if(Min_x[l].first > xl)
                    Min_x[l] = make_pair(xl, yl);
                if(Min_y[l].second > yl)
                    Min_y[l] = make_pair(xl, yl);
            }
        }
    for(int i=0;i<my_settings.no_segs;i++)
    {
        //double dx = sqrt(pow((Max_x[i].first-Min_x[i].first),2) + pow((Max_x[i].second - Min_x[i].second), 2));
        //double dy = sqrt(pow((Max_y[i].first-Min_y[i].first),2) + pow((Max_y[i].second - Min_y[i].second), 2));
        //double d2x = sqrt(pow((Max_x[i].first-Min_y[i].first),2) + pow((Max_x[i].second - Min_y[i].second), 2));
        //double d2y = sqrt(pow((Max_y[i].first-Min_x[i].first),2) + pow((Max_y[i].second - Min_x[i].second), 2));
        //cout<<new_lable[i]<<" "<<dy/dx<<" "<<d2y/d2x<<" "<<d2y/dx<<" "<<dy/d2x<<endl;
        double ctrod_x = (Min_x[i].first + Min_y[i].first + Max_x[i].first + Max_y[i].first)/4;
        double ctrod_y = (Min_x[i].second + Min_y[i].second + Max_x[i].second + Max_y[i].second)/4;
        double d1 = sqrt(pow((Max_x[i].first-ctrod_x),2) + pow((Max_x[i].second - ctrod_y), 2));
        double d2 = sqrt(pow((ctrod_x-Min_x[i].first),2) + pow((ctrod_y - Min_x[i].second), 2));
        double d3 = sqrt(pow((ctrod_x-Min_y[i].first),2) + pow((ctrod_y - Min_y[i].second), 2));
        double d4 = sqrt(pow((Max_y[i].first-ctrod_x),2) + pow((Max_y[i].second - ctrod_y), 2));
        cout<<new_lable[i]<<" "<<d1<<" "<<d2<<" "<<d3<<" "<<d4<<endl;
        
        
    }
    n=40;/*
    for(int i=0;i<length*height;i++){
        if(blues[i]>255){
            cout<<"50 150 100 "<<new_lable[i]<<endl;
        }
        else
            cout<<" "<<blues[i]<<" "<<greens[i]<<" "<<reds[i]<<" "<<new_lable[i]<<endl;
    }

    /*int pp;
    for(pp=0;pp<my_settings.no_segs*3;pp++){
        cout<<count[pp]<<"("<<pp<<")"<<" ";
    }
    cout<<pp;
    */cout<<endl;
    Mat M4;
    resize(M, M4, s1);
    
    cv::imwrite(newest,M4);
    destroyAllWindows();
    return 0;
}