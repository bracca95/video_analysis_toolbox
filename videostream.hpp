#ifndef __VIDEOSTREAM_H__
#define __VIDEOSTREAM_H__

#include <opencv2/cudawarping.hpp>	// cuda::resize
#include <opencv2/cudacodec.hpp>
#include <opencv2/core/opengl.hpp>

#include "frame.h"
#include "generica.hpp"

#define BATCH_SIZE 10
#define NEW_H 360
#define V_CHANNEL 2

using namespace std;

class Videostream
{
private:
	Tpath source;
	vector<Frame> frames_batch;
	double width, height, area;
	double fps;
	double tot_fps;
	double duration;
	
	cv::Rect blur_roi;		// x, y, w, h
	int patch_info[4];		// nx, ny, w, h

public:
	// Constructors
	Videostream(const Tpath& _tpath);

	// methods
	void vec2CVRect(const vector<int>& _roi, const bool& _debug);
	void vec2Patch(const vector<int>& _grid, const bool& _debug);
	void processing(Targuments _args);
};

#endif