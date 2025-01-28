#include <sstream>
#include <vector>
#include <string>
#include "FaceDetector.h"
#include <opencv2\opencv.hpp>

FaceDetector::FaceDetector() :
	ConfidenceThreshold(0.5),
	InputWidth(300),
	InputHeight(300),
	ScaleFactor(0.25),
	MeanValues({ 104.,177.,123. })
{
	Network = cv::dnn::readNetFromCaffe("D:/Projects/Visual Studio Projects/C++/ByteCamera/x64/Debug/assets/deploy.prototxt.txt", "D:/Projects/Visual Studio Projects/C++/ByteCamera/x64/Debug/assets/res10_300x300_ssd_iter_140000_fp16.caffemodel");

	if (Network.empty())
	{
		std::ostringstream ss;
		ss << "Failed to load network with the following settings:\nConfiguration: assets/deploy.prototxt.txt\nBinary: assets/res10_300x300_ssd_iter_140000_fp16.caffemodel\n";
		throw std::invalid_argument(ss.str());
	}
}

std::vector<cv::Rect> FaceDetector::DetectFaceRectangles(const cv::Mat& frame)
{
	cv::Mat InputBlob = cv::dnn::blobFromImage(frame, ScaleFactor, cv::Size(InputWidth, InputHeight), MeanValues, false, false);
	
	Network.setInput(InputBlob, "data");

	cv::Mat Detection = Network.forward("detection_out");
	cv::Mat DetectionMatrix(Detection.size[2], Detection.size[3], CV_32F, Detection.ptr<float>());

	std::vector<cv::Rect> faces;

	for (int Index = 0; Index < DetectionMatrix.rows; Index++)
	{
		float Confidence = DetectionMatrix.at<float>(Index, 2);

		if (Confidence < ConfidenceThreshold)
		{
			continue;
		}

		int BottomLeftX = static_cast<int>(DetectionMatrix.at<float>(Index, 3) * frame.cols);
		int BottomLeftY = static_cast<int>(DetectionMatrix.at<float>(Index, 4) * frame.rows);
		int UpperRightX = static_cast<int>(DetectionMatrix.at<float>(Index, 5) * frame.cols);
		int UpperRightY = static_cast<int>(DetectionMatrix.at<float>(Index, 6) * frame.rows);

		faces.emplace_back(BottomLeftX, BottomLeftY, UpperRightX - BottomLeftX, UpperRightY - BottomLeftY);
	}

	return faces;
}