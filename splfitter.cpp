// Copyright 2014-2015 Isis Innovation Limited and the authors of gSLICr
/*
        creates a mask as per given values of 0s and 1s from train.py  and then fits the curve with a spline to connect discrete parts of the lane
        
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
#include <vector>
#include<iomanip>
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
vector <int > splx1;
vector<int > splx2;
vector <pair<int,int> > sply1;
vector<pair<int,int> > sply2;
static int mask2d[24][24];
void spline(int N, vector<pair<int,int> >& eqn, vector<int>& loop)
{
    int i,j,k,n;
    cout.precision(4);                        //set precision
    cout.setf(ios::fixed);
    double x[N],y[N];
    for(int i=0;i<N;i++){
        x[i]=eqn[i].first;
    }
    for(int i=0;i<N;i++){
        y[i]=eqn[i].second;
    }
    n=3;                                // n is the degree of Polynomial 
    double X[2*n+1];                        //Array that will store the values of sigma(xi),sigma(xi^2),sigma(xi^3)....sigma(xi^2n)
    for (i=0;i<2*n+1;i++)
    {
        X[i]=0;
        for (j=0;j<N;j++)
            X[i]=X[i]+pow(x[j],i);        //consecutive positions of the array will store N,sigma(xi),sigma(xi^2),sigma(xi^3)....sigma(xi^2n)
    }
    double B[n+1][n+2],a[n+1];            //B is the Normal matrix(augmented) that will store the equations, 'a' is for value of the final coefficients
    for (i=0;i<=n;i++)
        for (j=0;j<=n;j++)
            B[i][j]=X[i+j];            //Build the Normal matrix by storing the corresponding coefficients at the right positions except the last column of the matrix
    double Y[n+1];                    //Array to store the values of sigma(yi),sigma(xi*yi),sigma(xi^2*yi)...sigma(xi^n*yi)
    for (i=0;i<n+1;i++)
    {    
        Y[i]=0;
        for (j=0;j<N;j++)
        Y[i]=Y[i]+pow(x[j],i)*y[j];        //consecutive positions will store sigma(yi),sigma(xi*yi),sigma(xi^2*yi)...sigma(xi^n*yi)
    }
    for (i=0;i<=n;i++)
        B[i][n+1]=Y[i];                //load the values of Y as the last column of B(Normal Matrix but augmented)
    n=n+1; 
    for (i=0;i<n;i++)                    //From now Gaussian Elimination starts(can be ignored) to solve the set of linear equations (Pivotisation)
        for (k=i+1;k<n;k++)
            if (B[i][i]<B[k][i])
                for (j=0;j<=n;j++)
                {
                    double temp=B[i][j];
                    B[i][j]=B[k][j];
                    B[k][j]=temp;
                }
    
    for (i=0;i<n-1;i++)            //loop to perform the gauss elimination
        for (k=i+1;k<n;k++)
            {
                double t=B[k][i]/B[i][i];
                for (j=0;j<=n;j++)
                    B[k][j]=B[k][j]-t*B[i][j];    //make the elements below the pivot elements equal to zero or elimnate the variables
            }
    for (i=n-1;i>=0;i--)                //back-substitution
    {                        //x is an array whose values correspond to the values of x,y,z..
        a[i]=B[i][n];                //make the variable to be calculated equal to the rhs of the last equation
        for (j=0;j<n;j++)
            if (j!=i)            //then subtract all the lhs values except the coefficient of the variable whose value                                   is being calculated
                a[i]=a[i]-B[i][j]*a[j];
        a[i]=a[i]/B[i][i];            //now finally divide the rhs by the coefficient of the variable to be calculated
    }
    for(int k=0;k<loop.size();k++){
        int j,i;
        i=loop[k];
        double jj= a[0] + a[1]*i + a[2]*i*i + a[3]*i*i*i;
        j=int(jj);
        if(mask2d[i][j]==0 && j<24){
            mask2d[i][j]=1;
        }
    }
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
    // =====================================================================================
    
    // ============ Declaration of a few arrays to be used further     
        
        float blue_sum[my_settings.no_segs*(2)] = {0};
        float green_sum[my_settings.no_segs*(2)] = {0};
        float red_sum[my_settings.no_segs*(2)] = {0};
        int blue[my_settings.img_size.x][my_settings.img_size.y] = {0};
        int green[my_settings.img_size.x][my_settings.img_size.y] = {0};
        int red[my_settings.img_size.x][my_settings.img_size.y] = {0};
        int sum_x[my_settings.no_segs] = {0};
        int sum_y[my_settings.no_segs] = {0};
        int matrix[250000] = {0};
        int count[250]={0};
        int lable;
        int prev_lable[my_settings.no_segs]={0};

    // =====================================================================================   

    // ============= processing and segmenting image 

        resize(oldFrame, frame, s);      
        load_image(frame, in_img);
        gSLICr_engine->Process_Frame(in_img);
        gSLICr_engine->Draw_Segmentation_Result(out_img);
        load_image(out_img, boundry_draw_frame);
        gSLICr_engine->Write_Seg_Res_To_PGM("abc",matrix);
        Mat M = frame;
        Mat M2,Mimg;

    //======================================================================================    

    // ============  Retrieving colour channels from the image and storing them in repective arrays

        for(int i=0;i<my_settings.img_size.x;i++)
        {
            for(int j=0;j<my_settings.img_size.y;j++)
            {
                blue[i][j] = M.at<cv::Vec3b>(i,j)[0]; // b
                green[i][j] = M.at<cv::Vec3b>(i,j)[1]; // g
                red[i][j] = M.at<cv::Vec3b>(i,j)[2]; // r
            }
        }
    // =====================================================================================

    //================ Summing over the pixel Lvalues of all segments to repective sum arrays

        for(int i=0;i<my_settings.img_size.x*my_settings.img_size.y;i++)
        {
            blue_sum[matrix[i]]+= blue[i/my_settings.img_size.x][i%my_settings.img_size.x];
            red_sum[matrix[i]]+= red[i/my_settings.img_size.x][i%my_settings.img_size.x];
            green_sum[matrix[i]]+= green[i/my_settings.img_size.x][i%my_settings.img_size.x];
            sum_x[matrix[i]]+=  ( i/my_settings.img_size.x);
            sum_y[matrix[i]]+= (i%my_settings.img_size.x);
            count[matrix[i]]++;
        }
       
       n--;                 // ============ since the matrix of superpixels is of the size (n-1) X (n-1)

    // =========== Masking the final Image for the hardcoded mask array 
        int k=0;
        static int mask[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        
        for(int i=0;i<24;i++){
            for(int j=0;j<24;j++){
                mask2d[i][j]=mask[i*24+j];
                cout<<mask2d[i][j]<<", ";
            }
           cout<<endl;
        }
        int N,flag1,flag2;
        cout<<endl;
        for(int i=0;i<24;i++){
            flag1=0;
            for(int j=0;j<12;j++){
                if(mask2d[i][j]==1){
                    sply1.push_back(make_pair(i,j));
                    flag1=1;
                }
                if(flag1==1)
                    break;
            }
            if(flag1==0)
                splx1.push_back(i);
        }
        for(int i=0;i<24;i++){
            flag2=0;
            for(int j=12;j<24;j++){
                if(mask2d[i][j]==1){
                    sply2.push_back(make_pair(i,j));
                    flag2=1;
                }
                if(flag2==1)
                    break;
            }
            if(flag2==0)
                splx2.push_back(i);
        }
        cout << flush;
        spline(sply1.size(), sply1, splx1);
        spline(sply2.size(), sply2, splx2);

        
        for(int i=0;i<24;i++){
            for(int j=0;j<24;j++){
                mask2d[i][j]=mask[i*24+j];
                cout<<mask2d[i][j]<<", ";
            }
            cout<<endl;
        }
        for(int i=0;i<24;i++){
            for(int j=0;j<24;j++){
                mask[i*24+j]=mask2d[i][j];
            }
        }
        for(int i=0;i<729;i++)
        {
            if(i/n==0 || i%n==0 || i/n==n-1 || i%n==n-1){
                red_sum[i] =0;
                green_sum[i] =0;
                blue_sum[i] =0;
            }
            else {
                red_sum[i] =mask[k]*255;
                green_sum[i] =mask[k]*255;
                blue_sum[i] =mask[k]*255;
                k++;
            }

        }
    // ==================================================================================== 

    // ============ Re-inserting the average Lvalues back into the image
        for(int i=0;i<my_settings.img_size.y;i++)
        {
            for(int j=0;j<my_settings.img_size.x;j++)
            {
                M.at<cv::Vec3b>(i,j)[0] = blue_sum[matrix[i*my_settings.img_size.x + j ]];// b
                M.at<cv::Vec3b>(i,j)[1] = green_sum[matrix[i*my_settings.img_size.x + j ]];// g
                M.at<cv::Vec3b>(i,j)[2] = red_sum[matrix[i*my_settings.img_size.x + j ]];// r
            }
        }
    // ===================================================================================
    
    // ====================== Displaying the image and/or writing it.    
        Mat M3;
        resize(M, M3, s1);
        //cv::imshow("FinalImg",M2);
        cv::namedWindow("Binary",5);
        cv::imshow("Binary",M3);
        std::string newname(".bmp");
        string newest;
        newest = mid + newname;
        cv::imwrite(newest,M3);
    // ====================================================================================    

    destroyAllWindows();
    return 0;
}