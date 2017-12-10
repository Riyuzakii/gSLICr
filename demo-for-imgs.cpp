// Copyright 2014-2015 Isis Innovation Limited and the authors of gSLICr
#include <iostream>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include "gSLICr_Lib/gSLICr.h"
#include "NVTimer.h"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/opencv.hpp"
#include "./gSLICr_Lib/objects/gSLICr_spixel_info.h"
#include "./gSLICr_Lib/engines/gSLICr_seg_engine_shared.h"

using namespace std;
using namespace cv;

typedef struct RgbColor
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
} RgbColor;

typedef struct HsvColor
{
    unsigned char h;
    unsigned char s;
    unsigned char v;
} HsvColor;

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

HsvColor RgbToHsv(RgbColor rgb)
{
    HsvColor hsv;
    unsigned char rgbMin, rgbMax;

    rgbMin = rgb.r < rgb.g ? (rgb.r < rgb.b ? rgb.r : rgb.b) : (rgb.g < rgb.b ? rgb.g : rgb.b);
    rgbMax = rgb.r > rgb.g ? (rgb.r > rgb.b ? rgb.r : rgb.b) : (rgb.g > rgb.b ? rgb.g : rgb.b);

    hsv.v = rgbMax;
    if (hsv.v == 0)
    {
        hsv.h = 0;
        hsv.s = 0;
        return hsv;
    }

    hsv.s = 255 * long(rgbMax - rgbMin) / hsv.v;
    if (hsv.s == 0)
    {
        hsv.h = 0;
        return hsv;
    }

    if (rgbMax == rgb.r)
        hsv.h = 0 + 43 * (rgb.g - rgb.b) / (rgbMax - rgbMin);
    else if (rgbMax == rgb.g)
        hsv.h = 85 + 43 * (rgb.b - rgb.r) / (rgbMax - rgbMin);
    else
        hsv.h = 171 + 43 * (rgb.r - rgb.g) / (rgbMax - rgbMin);

    return hsv;
}


const int Slider_max = 100;
int VMH_slider, VML_slider, Sat_slider;
double VMH,VML,Sat;
void on_trackbar( int, void* ){
    VMH = (double) VMH_slider/Slider_max ;
}
void on_trackbar2( int, void* ){
    VML = (double) VML_slider/Slider_max ;
}
void on_trackbar3( int, void* ){
    VML = (double) Sat_slider/Slider_max ;
}
std::string ToString(int val)
{
    stringstream ss;
    ss<<val;
    return ss.str();
}



int main()
{
    
    /*VideoCapture cap("../sam.webm");
    if (!cap.isOpened()) 
    {
        cerr << "unable to open camera!\n";
        return -1;
    }*/
    

    // gSLICr settings
    gSLICr::objects::settings my_settings;
    my_settings.img_size.x = 500;
    my_settings.img_size.y = 500;
    my_settings.no_segs = 250;
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


    //Initiating trackbars
    VMH_slider = 0;
    VML_slider = 0;
    Sat_slider = 0;
    //Create Windows
    namedWindow("ValueMeterHigh", 1);
    namedWindow("ValueMeterLow", 2);
    namedWindow("Saturation", 3);
    /// Create Trackbars
    char vmh[50],vml[50],sat[50];
    sprintf( vmh, "VMH x %d", Slider_max );
    sprintf( vml, "VML x %d", Slider_max );
    sprintf( sat, "Sat x %d", Slider_max );

    createTrackbar( vmh, "ValueMeterHigh", &VMH_slider, Slider_max, on_trackbar );
    createTrackbar( vml, "ValueMeterLow", &VML_slider, Slider_max, on_trackbar2 );
    createTrackbar( sat, "Saturation", &VML_slider, Slider_max, on_trackbar3 );

    int key, save_count = 0,no_imgs;
    cin>>no_imgs;
//    while(1)
    for(int i=1;i<=no_imgs;i++)    
    {

        std::string first ("../dr2/");
        std::string sec (".png");
        std::string mid = ToString(i);
        std::string name;
        name=first+mid+sec;
        VMH=0.56;
        VML=0.22;
        Sat=0.50;
        oldFrame = cv::imread(name);
       

        on_trackbar( VMH_slider, 0 );
        on_trackbar2(VML_slider,0);
        on_trackbar3(Sat_slider,0);


        float blue_sum[my_settings.no_segs*(2)]={0},green_sum[my_settings.no_segs*(2)]={0},red_sum[my_settings.no_segs*(2)]={0};
        int blue[my_settings.img_size.x][my_settings.img_size.y]={0};
        int green[my_settings.img_size.x][my_settings.img_size.y]={0};
        int red[my_settings.img_size.x][my_settings.img_size.y]={0};
        int sum_x[my_settings.no_segs]={0},sum_y[my_settings.no_segs]={0};
        int matrix[250000]={0};
        int count[200]={0};
        pub_info *obj=(pub_info*)malloc(2*my_settings.no_segs*sizeof(pub_info));

        resize(oldFrame, frame, s);
        load_image(frame, in_img);
        gSLICr_engine->Process_Frame(in_img);
        gSLICr_engine->Draw_Segmentation_Result(out_img);
        load_image(out_img, boundry_draw_frame);
        //cv::namedWindow("Segmented Image(No averaging)",0);
        //cv::imshow("Segmented Image(No averaging)", boundry_draw_frame);
        
        //RGB-value matrix retrieval
        gSLICr_engine->Write_Seg_Res_To_PGM("abc",matrix);
        Mat M = frame;
        Mat M2, M4;
        Mat M3=frame;
        RgbColor rgb_M3;
        HsvColor hsv_M3;
        HsvColor hsv_obj;
        int lable;

        //Extracting bgr-channels
        for(int i=0;i<my_settings.img_size.x;i++)
        {
            for(int j=0;j<my_settings.img_size.y;j++)
            {
                blue[i][j]=M.at<cv::Vec3b>(i,j)[0]; // b
                green[i][j]=M.at<cv::Vec3b>(i,j)[1]; // g
                red[i][j]=M.at<cv::Vec3b>(i,j)[2]; // r
            }
        }

        //Pixel-wise thresholding, without SLIC segmentation
        for(int i=0;i<my_settings.img_size.x;i++)
        {
            for(int j=0;j<my_settings.img_size.y;j++)
            {
                rgb_M3.b=M3.at<cv::Vec3b>(i,j)[0]; // b
                rgb_M3.g=M3.at<cv::Vec3b>(i,j)[1]; // g
                rgb_M3.r=M3.at<cv::Vec3b>(i,j)[2]; // r
                hsv_M3=RgbToHsv(rgb_M3);

                if((hsv_M3.v>VMH*300 && hsv_M3.s<Sat*300))
                    {
                        M3.at<cv::Vec3b>(i,j)[0]=255*count[i];
                            M3.at<cv::Vec3b>(i,j)[1]=255*count[i];
                        M3.at<cv::Vec3b>(i,j)[2]=255*count[i];
                        
                    }                           
                else
                {
                    M3.at<cv::Vec3b>(i,j)[0]=0;
                        M3.at<cv::Vec3b>(i,j)[1]=0;
                    M3.at<cv::Vec3b>(i,j)[2]=0;
                }  
            }
        }
        
        //calculating the sum of color values of a segment in the image
        for(int i=0;i<my_settings.img_size.x*my_settings.img_size.y;i++)
        {
            blue_sum[matrix[i]]+=blue[i/my_settings.img_size.x][i%my_settings.img_size.x];
            red_sum[matrix[i]]+=red[i/my_settings.img_size.x][i%my_settings.img_size.x];
            green_sum[matrix[i]]+=green[i/my_settings.img_size.x][i%my_settings.img_size.x];
            sum_x[matrix[i]]+= (i/my_settings.img_size.x);
            sum_y[matrix[i]]+= (i%my_settings.img_size.x);
            count[matrix[i]]++;
        }

        //Segment-wise thresholding, after SLIC and averaging of segment values
        for(int i=0;i<my_settings.no_segs;i++)
        {

                RgbColor rgb_obj;
                rgb_obj.r=red_sum[i]/count[i] ;// r
                rgb_obj.g=green_sum[i]/count[i] ;// r
                rgb_obj.b=blue_sum[i]/count[i] ;// r
                HsvColor hsv_obj= RgbToHsv(rgb_obj);
                if((hsv_obj.v>VMH*300 && hsv_obj.s<Sat*300))
                    {
                        red_sum[i]=255*count[i];
                        green_sum[i]=255*count[i];
                        blue_sum[i]=255*count[i];
                        lable=1;
                    }                           
                else
                {
                    red_sum[i]=0;
                    green_sum[i]=0;
                    blue_sum[i]=0;
                    lable=0;
                }   
                if(int(rgb_obj.r)!=0)
                    cout<<int(rgb_obj.r)<<" "<<int(rgb_obj.g)<<" "<<int(rgb_obj.b)<<" "<<lable<<"\n";
        }

        //Updating for Prediction values recieved after ML--comment out otherwise
/*        for(int i=0;i<my_settings.no_segs;i++)
        {
            red_sum[i]=newmat[i]*255*count[i];
            green_sum[i]=newmat[i]*255*count[i];
            blue_sum[i]=newmat[i]*255*count[i];
        }
*/


        for(int i=0;i<my_settings.img_size.y;i++)
        {
            for(int j=0;j<my_settings.img_size.x;j++)
            {
                M.at<cv::Vec3b>(i,j)[0]=blue_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// b
                M.at<cv::Vec3b>(i,j)[1]=green_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// g
                M.at<cv::Vec3b>(i,j)[2]=red_sum[matrix[i*my_settings.img_size.x + j ]]/count[matrix[i*my_settings.img_size.x + j]] ;// r
            }
        }
        // now compute all properties of pub_info obj[no_segs]
        for(int i=0;i<=my_settings.no_segs;i++)
        {
            if(count[i]!=0){
                        obj[i].size=count[i];
                        obj[i].avg_colors[0]=(int)(blue_sum[i]/count[i]);
                        obj[i].avg_colors[1]=(int)(green_sum[i]/count[i]);
                        obj[i].avg_colors[2]=(int)(red_sum[i]/count[i]);
                        obj[i].centre_x=(int)(sum_x[i]/count[i]);
                        obj[i].centre_y=(int)(sum_y[i]/count[i]);
                    }
        }
        
        resize(M, M2, s1);
        resize(M3, M4, s1);
//      cv::imshow("SegAveImg",0);
//      cv::imshow("SegAveImg", M2);
        cv::namedWindow("ThreshImg",0);
        cv::imshow("ThreshImg",M2);

        cv::imwrite(name,M4);
        key = (char)waitKey(1);
        if (key == 27) break;
        free(obj);
        //cout<<"This is the end of the line for UUUUUUUUUUUUUUUUUUUUUUUUUUUU......................."<<endl;
      
      }
    destroyAllWindows();
    return 0;
}