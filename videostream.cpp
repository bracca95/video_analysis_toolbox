#include "videostream.hpp"

Videostream::Videostream(const Tpath& _tpath)
{
	this->source = _tpath;

	// init frame buffer
	for (int i = 0; i < BATCH_SIZE; i++)
		this->frames_batch.push_back(Frame());

	// video info: use non-GPU library because cuda does not hold all these informations
	cv::VideoCapture cap = cv::VideoCapture(this->source.full_filename);
		
	this->width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
	this->height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
	this->area = width * height;
	this->fps = ceil(cap.get(cv::CAP_PROP_FPS));
	this->tot_fps = cap.get(cv::CAP_PROP_FRAME_COUNT);
	this->duration = cap.get(cv::CAP_PROP_FRAME_COUNT) / this->fps;

	cap.release();

	// init roi as full image and patch to -1
	this->blur_roi = cv::Rect(0, 0, static_cast<int>(this->width), static_cast<int>(this->height));
	for (int i = 0; i < 4; i++)
		this->patch_info[i] = -1;
}


/**
 * Convert array to cv::Rect.
 * 
 * Convert the input array[4] for ROI into a cv:Rect to determine a ROI during the analysis.
 * The function checks if all array values are the same (no ROI applied, so full image),
 * or if the values are not correct. If any errors occur, the program exits. If correction
 * is possible e.g. width or height are larger than the image itself, default values are used.
 * 
 * @param _roi (std::vecot): vector parsed from json defining the four points of the ROI
 * @param _debug (bool): perform action if debug is true
 */
void Videostream::vec2CVRect(const vector<int>& _roi, const bool& _debug)
{
	int tempW = _roi[2], tempH = _roi[3];
	int first = _roi[0];
	bool all_equal = true;

	for (int i = 1; i < ROI_LEN; i++)
	{
		if (_roi[i] != first)
		{
			all_equal = false;
			break;
		}
	}
	
	// if all elements are equal, change the ROI. Otherwise: full image
	if (all_equal == false)
	{
		// if some negative numbers
		for (int i = 0; i < ROI_LEN; i++)
		{
			if (_roi[i] < 0)
			{
				cout << "WARNING::negative number in ROI. Reverting ROI to full image!" << endl;
				break;
			}
		}

		// check x < this->width
		if (_roi[0] > this->width)
		{
			cout << "ERR::x must be < video width. Quitting..." << endl;
			exit(-1);
		}
		
		// check w < this->width
		if (_roi[2] > this->width)
		{
			cout << "ERR::w must be < video width. Quitting..." << endl;
			exit(-1);
		}
		
		// check x < this->width
		if (_roi[1] > this->height)
		{
			cout << "ERR::y must be < video height. Quitting..." << endl;
			exit(-1);
		}

		// check w < this->width
		if (_roi[3] > this->height)
		{
			cout << "ERR::h must be < video height. Quitting..." << endl;
			exit(-1);
		}

		// check x < w
		if (_roi[0] >= _roi[2])
		{
			cout << "ERR::position 1 (x) must be <= position 3 (width). Quitting..." << endl;
			exit(-1);
		}

		// check y < h
		if (_roi[1] >= _roi[3])
		{
			cout << "ERR::position 2 (y) must be <= position 4 (height). Quitting..." << endl;
			exit(-1);
		}

		// check x+w < width
		if (tempW > static_cast<int>(this->width) - _roi[0])
		{
			cout << "WARNING::width wider than frame! Reverting to original!" << endl;
			tempW = static_cast<int>(this->width) - _roi[0];
		}

		// check y+h < height
		if (tempH > static_cast<int>(this->height) - _roi[1])
		{
			cout << "WARNING::height heigher than frame! Reverting to original!" << endl;
			tempH = static_cast<int>(this->height) - _roi[1];
		}

		// update ROI value
		this->blur_roi = cv::Rect(_roi[0], _roi[1], tempW, tempH);
	}
	else
	{
		cout << "DEBUG::values are all equal. No ROI applied to images." << endl;
	}

	if (_debug)
	{
		cout << "DEBUG::ROI " << this->blur_roi << endl;
	}
}

/**
* Array to patch.
* 
* Given the array of patch parsed from the input json, which contains two elements:
* number of patches on x, number of patches on y, this function stores in memory
* those values together with the width and the height referred to the 
* previously defined ROI, for each regular patch.
* If values are negative or 0,0 or 1,1, the whole ROI is kept
* 
* @param _grid (std::vector): parsed values for patch
* @param _debug (bool): display debug info
*/
void Videostream::vec2Patch(const vector<int>& _grid, const bool& _debug)
{
	int reject_val[] = { 0, 1 };
	int nx, ny, patch_w, patch_h;
	
	// check values
	for (int i = 0; i < 2; i++)
	{
		if ( (reject_val[i] == _grid[0] && reject_val[i] == _grid[1]) || (_grid[i] < 0) )
		{
			nx = 1;
			ny = 1;
			break;
		}
		else
		{
			nx = _grid[0];
			ny = _grid[1];
		}
	}
	
	patch_w = static_cast<int>(floor((double)this->blur_roi.width / (double)nx));
	patch_h = static_cast<int>(floor((double)this->blur_roi.height / (double)ny));

	this->patch_info[0] = nx;
	this->patch_info[1] = ny;
	this->patch_info[2] = patch_w;
	this->patch_info[3] = patch_h;

	if (_debug)
	{
		cout << "DEBUG::patch = [";
		for (int i = 0; i < 4; i++)
			cout << this->patch_info[i] << ",";
		cout << "]" << endl;
	}
}

/**
* Processing video
* 
* This is the main function of the program: it loads the video capture
* and process it frame by frame. A sliding window is introuced for
* further operations on frames batch.
* 
* @param (Targuments) argument parsed from settings
* @param (bool) activate debug functions
* @see [known-issue videocap](https://answers.opencv.org/question/189349/known-issue-with-gopro-movie-codec-stops-reading-after-26-frames/)
* @see [known-issue](https://answers.opencv.org/question/4120/ffmpeg-crash-with-a-gopro-video/)
* @see [github open issue](https://github.com/opencv/opencv/issues/15352)
*/
void Videostream::processing(Targuments _args)
{
	/* --- INIT --- */
	vec2CVRect(_args.blur_roi, _args.debug); // create ROI for blur
	vec2Patch(_args.patch_grid, _args.debug);
	
	if (_args.debug)
	{
		cout << "DEBUG::video info:" << endl
			 << "w: " << this->width << ", h: " << this->height << ", fps: " << this->fps << ", sec: " << this->duration << endl;
	}

	// window
	int new_w = Generica::getNewW(this->width, this->height, NEW_H);
	const string window_name = "GPU";
	if (_args.show)
	{
		cv::namedWindow(window_name, cv::WINDOW_OPENGL);
		cv::cuda::setGlDevice();
		cv::resizeWindow(window_name, new_w, NEW_H);
	}
		
	// matrix and video
	cv::cuda::GpuMat gpu_mat_bgra;
	cv::Ptr<cv::cudacodec::VideoReader> cap = cv::cudacodec::createVideoReader(this->source.full_filename);
	cv::Ptr<cv::cuda::Filter> lap = cv::cuda::createLaplacianFilter(CV_8U, CV_8U, 3);
	cv::Ptr<cv::cuda::CornersDetector> corner_det = cv::cuda::createGoodFeaturesToTrackDetector(CV_8U);		// grayscale
	cv::Ptr<cv::cuda::SparsePyrLKOpticalFlow> pyrLK_sparse = cv::cuda::SparsePyrLKOpticalFlow::create();	// 1000x1(rxc)
	
	/* file:: */
	string csv_blur_path, csv_exposure_path, csv_entropy_path, csv_motion_path;
	ofstream csv_blur, csv_exposure, csv_entropy, csv_motion;

	if (_args.blur)
	{
		csv_blur_path = Generica::makeCSV(csv_blur, _args.video_path, "blur", this->patch_info);
		csv_blur.open(csv_blur_path, ios_base::app);
	}

	if (_args.exposure)
	{
		csv_exposure_path = Generica::makeCSV(csv_exposure, _args.video_path, "exposure");
		csv_exposure.open(csv_exposure_path, ios_base::app);
	}

	if (_args.entropy)
	{
		csv_entropy_path = Generica::makeCSV(csv_entropy, _args.video_path, "entropy");
		csv_entropy.open(csv_entropy_path, ios_base::app);
	}

	if (_args.motion)
	{
		csv_motion_path = Generica::makeCSV(csv_motion, _args.video_path, "motion");
		csv_motion.open(csv_motion_path, ios_base::app);
	}
	/* EOF::file INIT */
	/* --- EOF::INIT --- */

	int count = 0;
	while (count < static_cast<int>(this->tot_fps))
	{
		try
		{
			if (!cap->nextFrame(gpu_mat_bgra))
			{
				cout << "DEBUG::No frame!" << count << endl;
				count += 1;
				continue;
			}
		}
		catch (const exception& msg)
		{
			cerr << "ERR::" << msg.what() << endl;
			exit(-1);
		}

		// possible pre-processing
		//cv::cuda::resize(gpu_mat_bgra, gpu_mat_bgra, cv::Size(new_w, NEW_H));
		
		// sliding window
		Frame latest_frame = Frame(gpu_mat_bgra, count);
		Generica::bufferize(this->frames_batch, latest_frame);
		
		/* --- ALL FUNCTIONS APPLIED TO THE SINGLE FRAME MUST GO HERE --- */
		if (_args.blur)
		{
			latest_frame.computeBlur(lap, this->blur_roi, this->patch_info);

			// file::
			vector<pair<float, float>> bv = latest_frame.getBlurLevel();
			vector<pair<float, float>>::iterator bvii;

			csv_blur << count;
			for (bvii = bv.begin(); bvii != bv.end(); bvii++)
				csv_blur << "," << bvii->first << "," << bvii->second;
			csv_blur << endl;
		}

		if (_args.exposure)
		{
			latest_frame.computeExposure(V_CHANNEL, MIN_BIN_NUMBER, this->area);
			csv_exposure << count << "," << latest_frame.getExposureLevel() << endl;	// file::
		}

		if (_args.entropy)
		{
			latest_frame.computeEntropy(V_CHANNEL, MAX_BIN_NUMBER, this->area);
			csv_entropy << count << "," << latest_frame.getEntropyLevel() << endl;	// file::
		}

		if (_args.motion)
		{
			latest_frame.computeMotion(this->frames_batch, corner_det, pyrLK_sparse);
			csv_motion << count << "," << latest_frame.getMotionLevel() << endl;	// file::
		}
		/* --- EOF --- */

		// show window if specified
		if (_args.show)
		{
			cv::imshow(window_name, gpu_mat_bgra);

			// Press ESC on keyboard to exit
			char c = (char)cv::waitKey(1);
			if (c == 27)
				break;
		}

		count += 1;
	}

	// close when everything is done
	csv_blur.close();		// file::
	csv_exposure.close();	
	csv_entropy.close();
	csv_motion.close();
	
	cap.release();			// or cap->~VideoReader();
	cv::destroyAllWindows();
}
