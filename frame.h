#ifndef __FRAME_H__
#define __FRAME_H__

#include "generica.hpp"
#include <opencv2/core.hpp>
#include <opencv2/cudacodec.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/cudafilters.hpp>	// sobel
#include <opencv2/cudaarithm.hpp>	// cuda::norm
#include <opencv2/cudaimgproc.hpp>	// cuda::cvtcolor
#include <opencv2/cudaoptflow.hpp>
#include <opencv2/core/cvstd.hpp>

#define MIN_BIN_NUMBER 5
#define MAX_BIN_NUMBER 256

using namespace std;

class Frame
{
private:
	cv::cuda::GpuMat frame_gpu;		// if cudacoded is used --> BGRA (4 channels)
	int count;
	vector<pair<float, float>> blur_level;				// [0:inf), [0:2] normalized post acquisition
	float exposure_level;			// [-1:1]
	float entropy_level;			// [0:inf)
	float motion;					// [0:inf)

public:
	// Constructors
	Frame();
	Frame(cv::cuda::GpuMat& _gpu_mat, int _count);

	// methods::setters
	void computeBlur(cv::Ptr<cv::cuda::Filter>& _lap, const cv::Rect& _roi, const int _patch_info[]);
	void computeExposure(const int& ch_number, const int& _bin_number, const double& _area);
	void computeEntropy(const int& ch_number, const int& _bin_number, const double& _area);
	void computeMotion(const vector<Frame>& _buf, cv::Ptr<cv::cuda::CornersDetector>& _cd, cv::Ptr<cv::cuda::SparsePyrLKOpticalFlow>& _of);

	// methods::getter
	int getFrameCounter() const;
	vector<pair<float, float>> getBlurLevel() const;
	float getExposureLevel() const;
	float getEntropyLevel() const;
	float getMotionLevel() const;
	cv::cuda::GpuMat getCurrentMat() const;

	// methods::other
	cv::Mat computeHist(const int& ch_number, const int& _bin_number);
};

#endif
