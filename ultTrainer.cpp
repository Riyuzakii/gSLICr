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

typedef struct {
    double r;       // a fraction between 0 and 1
    double g;       // a fraction between 0 and 1
    double b;       // a fraction between 0 and 1
} rgb;

typedef struct {
    double h;       // angle in degrees
    double s;       // a fraction between 0 and 1
    double v;       // a fraction between 0 and 1
} hsv;

static hsv   rgb2hsv(rgb in);
static rgb   hsv2rgb(hsv in);

hsv rgb2hsv(rgb in)
{
    hsv         out;
    double      min, max, delta;

    min = in.r < in.g ? in.r : in.g;
    min = min  < in.b ? min  : in.b;

    max = in.r > in.g ? in.r : in.g;
    max = max  > in.b ? max  : in.b;

    out.v = max;                                // v
    delta = max - min;
    if (delta < 0.00001)
    {
        out.s = 0;
        out.h = 0; // undefined, maybe nan?
        return out;
    }
    if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        out.s = (delta / max);                  // s
    } else {
        // if max is 0, then r = g = b = 0              
        // s = 0, h is undefined
        out.s = 0.0;
        out.h = NAN;                            // its now undefined
        return out;
    }
    if( in.r >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( in.g - in.b ) / delta;        // between yellow & magenta
    else
    if( in.g >= max )
        out.h = 2.0 + ( in.b - in.r ) / delta;  // between cyan & yellow
    else
        out.h = 4.0 + ( in.r - in.g ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if( out.h < 0.0 )
        out.h += 360.0;

    return out;
}
struct pub_info
{
    float centre_x;
    float centre_y;
    float avg_colors[3];
    float size;
};

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
void print(float rs[],float bs[], float gs[] ,int c[],int x,int y,int n)
{
   if(c[n*x+y]!=0 && rs[n*x+y]/c[n*x+y]>16 && bs[n*x+y]/c[n*x+y]>16 && gs[n*x+y]/c[n*x+y]>16 ){
        cout<<(int)(rs[n*x+y]/c[n*x+y])<<" ";
        cout<<(int)(bs[n*x+y]/c[n*x+y])<<" ";
        cout<<(int)(gs[n*x+y]/c[n*x+y])<<" ";
    }
    else {
        rs[n*x+y]=100*c[n*x+y];
        bs[n*x+y]=150*c[n*x+y];
        gs[n*x+y]=50*c[n*x+y];
        cout<<"100 150 50 ";
    }
}
///
    //Defining here to use everywhere without passing through functions
    int matrix[800*800] = {0};
    int superFlag=1;
    int superID;

void CallBackFunc(int event, int x, int y, int flags, void* userdata)
{
     
    if  ( event == EVENT_LBUTTONDOWN )
    {
        //cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
        superFlag=1;
        superID =matrix[x+y*800];
        //cout<<superFlag<<endl;
    }
    else if  ( event == EVENT_RBUTTONDOWN )
    {
        //cout << "Right button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
        superFlag = 0;
        superID= matrix[x + y*800];
        //cout<<superFlag<<endl;
    }
    
}



int main()
{
    gSLICr::objects::settings my_settings;

    my_settings.img_size.x = 800;
    my_settings.img_size.y = 800;
    my_settings.no_segs = 1600;
    my_settings.spixel_size = 100;
    my_settings.coh_weight = 0.8f;
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
    Size s1(800,800);
    Mat oldFrame, frame;
    Mat boundry_draw_frame; boundry_draw_frame.create(s, CV_8UC3);
    int key,h=0;
    cin>>h;
        //for(int i=0;i<=h;i++)
        //{
        //h++;
        std::string first ("../top_view_dataset/test_0/frame000");
        std::string sec (".jpg");
        std::string mid = ToString(h);
        std::string name;
        name=first+mid+sec;
        oldFrame = cv::imread(name);
        //cout<<Lvalue<<"+"<<Hvalue<<"-\n";
        
        float blue_sum[my_settings.no_segs*(2)] = {0};
        float green_sum[my_settings.no_segs*(2)] = {0};
        float red_sum[my_settings.no_segs*(2)] = {0};
        int blue[my_settings.img_size.x][my_settings.img_size.y] = {0};
        int green[my_settings.img_size.x][my_settings.img_size.y] = {0};
        int red[my_settings.img_size.x][my_settings.img_size.y] = {0};
        int sum_x[my_settings.no_segs] = {0};
        int sum_y[my_settings.no_segs] = {0};

        int count[1600]={0};
        resize(oldFrame, frame, s);
        
        load_image(frame, in_img);
        gSLICr_engine->Process_Frame(in_img);
        gSLICr_engine->Draw_Segmentation_Result(out_img);
        load_image(out_img, boundry_draw_frame);
        cv::namedWindow("InitialImg",0);
        cv::imshow("InitialImg", frame);
        //cv::namedWindow("FinalImg",0);
        

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
        for(int x=1;x<=40;x++)
            for(int y=1;y<=40;y++){
            print(red_sum, green_sum, blue_sum, count, x-1,y-1,n);
            print(red_sum, green_sum, blue_sum, count, x-1,y,n);
            print(red_sum, green_sum, blue_sum, count, x-1,y+1,n);
            print(red_sum, green_sum, blue_sum, count, x,y-1,n);
            print(red_sum, green_sum, blue_sum, count, x,y,n);
            print(red_sum, green_sum, blue_sum, count, x,y+1,n);
            print(red_sum, green_sum, blue_sum, count, x+1,y-1,n);
            print(red_sum, green_sum, blue_sum, count, x+1,y,n);
            print(red_sum, green_sum, blue_sum, count, x+1,y+1,n);
            cout<<endl;
        }

         for(int x=1;x<=40;x++)
            for(int y=1;y<=40;y++){
            print(red_sum, green_sum, blue_sum, count, x,y,n);
            cout<<endl;
        }

        for(int i=0;i<my_settings.img_size.y;i++)
        {
            for(int j=0;j<my_settings.img_size.x;j++)
            {
                M.at<cv::Vec3b>(i,j)[0] = blue_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// b
                M.at<cv::Vec3b>(i,j)[1] = green_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// g
                M.at<cv::Vec3b>(i,j)[2] = red_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// r
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
        //cout<<superFlag<<endl;
        blue_sum[superID]= 255*superFlag*count[superID];
        red_sum[superID]= 255*superFlag*count[superID];
        green_sum[superID]= 255*superFlag*count[superID];
        ///Re-inserting the average Lvalues back into the image
        for(int i=0;i<my_settings.img_size.y;i++)
        {
            for(int j=0;j<my_settings.img_size.x;j++)
            {
                M.at<cv::Vec3b>(i,j)[0] = blue_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// b
                M.at<cv::Vec3b>(i,j)[1] = green_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// g
                M.at<cv::Vec3b>(i,j)[2] = red_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// r
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
                int bs = blue_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// b
                int gs = green_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// g
                int rs = red_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// r

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
    n=40;
    for(int i=0;i<40*40;i++)
    {            
        if(i/n==0 || i%n==0 || i/n==n-1 || i%n==n-1){
                red_sum[i] =0;
                green_sum[i] =0;
                blue_sum[i] =0;
                cout<<"0 ";
        }
        else cout<<new_lable[i]<<" ";
    }
    cout<<endl;
    //for(int i=0;i<800*800;i++)
      //  cout<<matrix[i]<<" ";
    Mat M4;
    resize(M, M4, s1);
    
    cv::imwrite(newest,M4);
    destroyAllWindows();
    return 0;
}