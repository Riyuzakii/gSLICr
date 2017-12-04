// Copyright 2014-2015 Isis Innovation Limited and the authors of gSLICr
<<<<<<< HEAD
#include <iostream>
=======

>>>>>>> 7de441a3b2efd988d3937b80b53f0603d0d54400
#pragma once
#include "gSLICr_seg_engine.h"

using namespace std;
using namespace gSLICr;
using namespace gSLICr::objects;
using namespace gSLICr::engines;


seg_engine::seg_engine(const objects::settings& in_settings)
{
	gSLICr_settings = in_settings;
}


seg_engine::~seg_engine()
{
	if (source_img != NULL) delete source_img;
	if (cvt_img != NULL) delete cvt_img;
	if (idx_img != NULL) delete idx_img;
	if (spixel_map != NULL) delete spixel_map;
}

void seg_engine::Perform_Segmentation(UChar4Image* in_img)
{
	source_img->SetFrom(in_img, ORUtils::MemoryBlock<Vector4u>::CPU_TO_CUDA);
	Cvt_Img_Space(source_img, cvt_img, gSLICr_settings.color_space);

	Init_Cluster_Centers();
	Find_Center_Association();

	for (int i = 0; i < gSLICr_settings.no_iters; i++)
	{
		Update_Cluster_Center();
		Find_Center_Association();
	}

<<<<<<< HEAD
	idx_img->UpdateHostFromDevice();
		
		//cout<<sizeof(*idx_img) << "   " << spixel_size <<" "<< sizeof(*spixel_map); 
	

=======
>>>>>>> 7de441a3b2efd988d3937b80b53f0603d0d54400
	if(gSLICr_settings.do_enforce_connectivity) Enforce_Connectivity();
	cudaThreadSynchronize();
}



