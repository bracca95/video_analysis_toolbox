#include "frame.h"

Frame::Frame()
{
	this->frame_gpu = cv::cuda::GpuMat();
	this->count = -1;
}

Frame::Frame(cv::cuda::GpuMat& _gpu_mat, int _count)
{
	_gpu_mat.copyTo(this->frame_gpu);
	this->count = _count; 
	this->exposure_level = 0.0f;
	this->entropy_level = 0.0f;
	this->motion = 0.0f;
}

/**
* Compute blurriness of the whole frame
* 
* Use Laplacian filter to compute the gradient of the image.
* The blurriness is defines as the variance of the retrieved matrix:
* the lower the value (variance), the higher the blurriness.
* The function could work on all channels, yet grayscale only is faster.
* 
* @param _lap (cuda Filter): laplacian filter
* @param _roi (cv Rect): user-defined region of interest of the image
* @param _patch_info (int*): nx, ny, patch w, patch h
* 
* @see [original code](https://stackoverflow.com/questions/63508517/opencv-cuda-laplacian-filter-on-3-channel-image)
* @see [built-in function](https://docs.opencv.org/3.4/dc/d66/group__cudafilters.html#ga53126e88bb7e6185dcd5628e28e42cd2)
*/
void Frame::computeBlur(cv::Ptr<cv::cuda::Filter>& _lap, const cv::Rect& _roi, const int _patch_info[])
{
	cv::cuda::GpuMat gray_mat, patch_mat;
	cv::cuda::cvtColor(this->frame_gpu, gray_mat, cv::COLOR_BGRA2GRAY, 1);
	cv::Scalar mean_blur, mean_pix, std_blur, std_pix;
	
	// reduce matrix to ROI (defined in input)
	gray_mat = gray_mat(_roi);

	// work on patches
	for (int y = 0; y < _patch_info[1]; y++)
	{
		for (int x = 0; x < _patch_info[0]; x++)
		{
			cv::Rect patch_roi = cv::Rect(x * _patch_info[2], y * _patch_info[3], _patch_info[2], _patch_info[3]);
			patch_mat = gray_mat(patch_roi);

			//pixel variance
			cv::cuda::meanStdDev(patch_mat, mean_pix, std_pix);
			float variance_pixel = static_cast<float>(std_pix[0] * std_pix[0]); // variance is the squared of std

			// blur variance
			_lap->apply(patch_mat, patch_mat);
			cv::cuda::meanStdDev(patch_mat, mean_blur, std_blur);
			float variance_blur = static_cast<float>(std_blur[0] * std_blur[0]);
			
			this->blur_level.push_back(pair(variance_blur, variance_pixel));
		}
	}
}

/**
* Compute exposure value
* 
* The function firstly computes the histogram for the required number of bins (3).
* Afterwards, the exposure is seen as the difference between the highest and lowest bin (among the 3).
* Since it is only meant to catch high distortion, it implements the formula:
* (high - min) * (1 - high - mid/2)
* If the returned value is negative, it means the low value component are the predominant ones.
* The multiplied component is used to enhance strong difference between high and low.
* 
* @param ch_number: (int) the channel of the considered histogram
* @param bin_number: (int) the number of bins of the histogram
* @param area: (double) the number of pixels in the original matrix (normalization)
*/
void Frame::computeExposure(const int& ch_number, const int& _bin_number, const double& _area)
{
	cv::Mat hist_cpu = computeHist(ch_number, _bin_number);

	// init
	int lst = MIN_BIN_NUMBER - 1;
	int mid = static_cast<int>(ceil(lst / 2));
	double hist_bin[MIN_BIN_NUMBER] = { 0. };
	double min = 0;
	
	// normalization
	for (int y = 0; y < hist_cpu.rows; y++)
		for (int x = 0; x < hist_cpu.cols; x++)
			hist_bin[y] = static_cast<double>(hist_cpu.at<int>(y, x)) / _area;

	if (hist_bin[lst] >= hist_bin[0])
		min = hist_bin[0];
	else
		min = hist_bin[lst];

	// formula: negative if hist_bin[0] is max
	this->exposure_level = static_cast<float>((hist_bin[lst] - hist_bin[0]) * abs(1 - hist_bin[mid] - min));
}

/**
* Compute entropy value for a given channel
*
* The function firstly computes the histogram for the required number of bins (256).
* Afterwards, it implements the Shannon entropy on the specified channel
*
* @param ch_number: (int) the channel of the considered histogram
* @param bin_number: (int) the number of bins of the histogram
* @param area: (double) the number of pixels in the original matrix (normalization)
* 
* @see[implementation](https://stackoverflow.com/a/24930922)
* @see[theory](https://stackoverflow.com/a/40660371)
*/
void Frame::computeEntropy(const int& ch_number, const int& _bin_number, const double& _area)
{
	cv::Mat hist_cpu, logP;

	hist_cpu = computeHist(ch_number, _bin_number);
	hist_cpu.convertTo(hist_cpu, CV_64FC1);
	hist_cpu /= _area;
	hist_cpu += 1e-4; //prevent 0

	cv::log(hist_cpu, logP);
	this->entropy_level = -1 * static_cast<float>(cv::sum(hist_cpu.mul(logP)).val[0]);
}

/**
* Motion estimation
* 
* Estimate quantity of motion over consecutive frames. It uses sparse optical flow because faster
* and generally more accurate than full pixels analysis. The final result is computed as
* the norm of the difference between the position of the points in the previous frame, and those
* in the current frame (next).
* 
* @param (vector<Frame>) _buf: the frame buffer from which the last and second to last frames are extracted
* @see [theory](https://docs.opencv.org/4.4.0/d4/dee/tutorial_optical_flow.html)
* @see [cuda demo](https://github1s.com/opencv/opencv/blob/master/samples/gpu/pyrlk_optical_flow.cpp)
*/
void Frame::computeMotion(const vector<Frame>& _buf, cv::Ptr<cv::cuda::CornersDetector>& _cd, cv::Ptr<cv::cuda::SparsePyrLKOpticalFlow>& _of)
{
	// second to last element: matrix are init at count -1, so skip the first two frames
	if (_buf.at(_buf.size() -2).count > -1)
	{
		// init
		cv::cuda::GpuMat frame_gray_prev, frame_gray_next;
		cv::cuda::GpuMat prevPts, nextPts, diffPts, status;

		cv::cuda::cvtColor(_buf.at(_buf.size() -2).frame_gpu, frame_gray_prev, cv::COLOR_BGRA2GRAY, 1);
		cv::cuda::cvtColor(_buf.back().frame_gpu, frame_gray_next, cv::COLOR_BGRA2GRAY, 1);

		// good features to track
		_cd->detect(frame_gray_prev, prevPts);		// the third, optional, element is mask: reduce area of interest

		// sparse optical flow
		_of->calc(frame_gray_prev, frame_gray_next, prevPts, nextPts, status);

		try
		{
			// compute amount of shift
			cv::cuda::subtract(prevPts, nextPts, diffPts);
			double norm = cv::cuda::norm(diffPts, cv::NORM_L2);
			this->motion = static_cast<float>(norm * norm);
		}
		catch (const exception& msg)
		{
			cout << "ERR::" << msg.what() << endl;
		}
	}
	else
	{
		this->motion = 0.0f;
	}
}

/**
* Compute histogram of the required number of bins
*
* The function computes the histogram for the required number of bins.
* The matrix must be downloaded to access the values.
*
* @param ch_number: (int) the channel of the considered histogram
* @param bin_number: (int) the number of bins of the histogram
* @param area: (double) the number of pixels in the original matrix (normalization)
*/
cv::Mat Frame::computeHist(const int& ch_number, const int& _bin_number)
{
	// init
	cv::cuda::GpuMat temp_mat, hsv_mat, v_hist, v_hist_t;
	cv::Mat hist_cpu;

	// colorspace translation: cuda has not direct transformation rgba -> hsv
	cv::cuda::cvtColor(this->frame_gpu, temp_mat, cv::COLOR_BGRA2BGR, 3);
	cv::cuda::cvtColor(temp_mat, hsv_mat, cv::COLOR_BGR2HSV, 3);

	// split HSV channels
	vector<cv::cuda::GpuMat> channels;
	cv::cuda::split(hsv_mat, channels);

	// compute histogram and download on cpu to further elaboration
	cv::cuda::histEven(channels[ch_number], v_hist, _bin_number, 0, 256);		// 256(3) for full histogram
	cv::cuda::transpose(v_hist, v_hist_t);		// cpu and gpu mat are transposed wtf!! // type 4

	// set
	v_hist_t.download(hist_cpu);	// download or will not be able to access matrix values wtf pt2 // 3r, 1c
	return hist_cpu;
}

/* GETTERS */
/// count
int Frame::getFrameCounter() const
{
	return this->count;
}

/// blur level
vector<pair<float, float>> Frame::getBlurLevel() const
{
	return this->blur_level;
}

/// exposure level
float Frame::getExposureLevel() const
{
	return this->exposure_level;
}

/// entropy level
float Frame::getEntropyLevel() const
{
	return this->entropy_level;
}

/// motion level
float Frame::getMotionLevel() const
{
	return this->motion;
}

/// gpu frame
cv::cuda::GpuMat Frame::getCurrentMat() const
{
	return this->frame_gpu;
}
