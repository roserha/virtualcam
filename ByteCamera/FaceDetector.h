#pragma once
#ifndef VISUALS_FACEDETECTOR_H
#define VISUALS_FACEDETECTOR_H
#include <opencv2\dnn.hpp>

class FaceDetector 
{
public:
	explicit FaceDetector();
	/// Detect faces in an image frame
	/// \param frame Image to detect faces in
	/// \return Vector of detected faces
	std::vector<cv::Rect> DetectFaceRectangles(const cv::Mat& frame);

private:
	/// Face detection network
	cv::dnn::Net Network;

	/// Input image's width
	const int InputWidth;

	/// Input image's height
	const int InputHeight;

	/// Image's scaling factor
	const double ScaleFactor;

	/// Mean normalization values network was trained with
	const cv::Scalar MeanValues;

	///Face detection confidence threshold
	const float ConfidenceThreshold;
};
#endif // VISUALS_FACEDETECTOR_H