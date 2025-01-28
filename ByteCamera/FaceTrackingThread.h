#pragma once

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <string>
#include <sstream>

#include <SFML\System.hpp>
#include <SFML\Graphics.hpp>

#define USE_FACE
#include "NtKinect.h"

// #include "FaceDetector.h"


using namespace cv;

class FaceTracker
{
private:
	std::thread TrackingThread;
	std::atomic<bool> bThreadRunning = false;

	const int landmarkCoordsArray[10] = { 19, 24, 37, 38, 40, 41, 43, 44, 46, 47 };

	double eyeOpen[2];
	double pupilWidth[2];
	double browLift[2];

	bool depth = false;

	string faceProp[8] = {
		"happy", "engaged", "glass", "leftEyeClosed", "rightEyeClosed", "mouthOpen", "mouthMoved", "lookingAway"
	};

	string state[4] = {
		"unknown", "no", "maybe", "yes"
	};

	bool hideYourself = false;

public:
	void DrawString(cv::Mat img, string s, cv::Point p)
	{
		cv::putText(img, s, p, cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(0, 230, 0), 1, cv::LINE_AA);
	}

	string HexString(int n) { stringstream ss; ss << hex << n; return ss.str(); };

	double verticalDistance(const Point2f& a, const Point2f& b)
	{
		return abs(a.y - b.y);
	}

	double euclidianDistance(const Point2f& a, const Point2f& b)
	{
		return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
	}

	Point2f midpoint(const Point2f& a, const Point2f& b)
	{
		return Point2f((a.x + b.x) / 2, (a.y + b.y) / 2);
	}

	cv::Rect doJob()
	{
		NtKinect kinect;
		cv::Mat img8;
		cv::Mat img;

		kinect.setHDFaceModelFlag(true);

		while (true)
		{
			// Enable RGB image
			if (!depth) kinect.setRGB();

			// Enable Depth image
			if (depth)
			{
				kinect.setDepth();
				kinect.depthImage.convertTo(img8, CV_8UC1, 255.0 / 4500);
				cv::cvtColor(img8, img, cv::COLOR_GRAY2BGR);
			}

			// Track skeleton
			kinect.setSkeleton();

			// Track face
			if (!depth) kinect.setFace(); // RGB Image
			else kinect.setFace(false); // Depth Image

			// Get HD Face!
			//kinect.setHDFace();

			// Hide your body
			if (hideYourself) kinect.rgbImage.setTo(cv::Scalar(0, 0, 0));

			// Plot points for skeletons
			for (auto person : kinect.skeleton)
			{
				for (auto joint : person)
				{
					if (joint.TrackingState == TrackingState_NotTracked) continue;
					if (!depth)
					{
						ColorSpacePoint CSP; // RGB
						kinect.coordinateMapper->MapCameraPointToColorSpace(joint.Position, &CSP); // RGB
						cv::rectangle(kinect.rgbImage, cv::Rect((int)CSP.X - 5, (int)CSP.Y - 5, 10, 10), cv::Scalar(200, 0, 0), 2); // RGB
					}
					else
					{
						DepthSpacePoint DSP; // Depth
						kinect.coordinateMapper->MapCameraPointToDepthSpace(joint.Position, &DSP); // Depth
						cv::rectangle(img, cv::Rect((int)DSP.X - 5, (int)DSP.Y - 5, 10, 10), cv::Scalar(200, 0, 0), 2); // Depth
					}
				}
			}

			// Draw rectangle around face
			for (cv::Rect faceRect : kinect.faceRect)
			{
				cv::rectangle(!depth ? kinect.rgbImage : img, faceRect, cv::Scalar(255, 255, 0), 2);
			}

			// Draw face points
			for (vector<PointF> pVector : kinect.facePoint)
			{
				for (PointF facePoint : pVector)
				{
					cv::rectangle(!depth ? kinect.rgbImage : img, cv::Rect((int)facePoint.X - 5, (int)facePoint.Y - 5, 10, 10), cv::Scalar(0, 255, 255), 2);
				}
			}

			// Get face rotation
			for (int i = 0; i < kinect.faceDirection.size(); i++)
			{
				cv::Vec3f dir = kinect.faceDirection[i];
				DrawString(!depth ? kinect.rgbImage : img, "pitch: " + to_string(dir[0]), cv::Point(200 * i + 50, 30));
				DrawString(!depth ? kinect.rgbImage : img, "roll: " + to_string(dir[1]), cv::Point(200 * i + 50, 60));
				DrawString(!depth ? kinect.rgbImage : img, "yaw: " + to_string(dir[2]), cv::Point(200 * i + 50, 90));
			}

			// Get face props
			for (int i = 0; i < kinect.faceProperty.size(); i++)
			{
				for (int j = 0; j < FaceProperty_Count; j++)
				{
					int v = kinect.faceProperty[i][j];
					cv::putText(!depth ? kinect.rgbImage : img, faceProp[j] + ": " + state[v], cv::Point(200 * i + 50, 30 * j + 120), cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(232, 232, 0), 1, cv::LINE_AA); // RGB
				}
			}
			/*
			DrawString(kinect.rgbImage, "TrackingID: ", cv::Point(0, 730));
			DrawString(kinect.rgbImage, "Collection: ", cv::Point(0, 760));
			DrawString(kinect.rgbImage, "Capture: ", cv::Point(0, 790));
			DrawString(kinect.rgbImage, "Collection: ", cv::Point(0, 820));
			DrawString(kinect.rgbImage, "Capture: ", cv::Point(0, 850));

			// Draw HD Face points
			for (int i = 0; i < kinect.hdfaceVertices.size(); i++)
			{
				for (CameraSpacePoint SP : kinect.hdfaceVertices[i])
				{
					ColorSpacePoint CSP;
					kinect.coordinateMapper->MapCameraPointToColorSpace(SP, &CSP); // RGB
					cv::rectangle(kinect.rgbImage, cv::Rect((int)CSP.X - 1, (int)CSP.Y - 1, 2, 2), cv::Scalar(192,192,0)); // RGB
				}
				int x = 200 * i + 200;
				auto status = kinect.hdfaceStatus[i];
				auto statusS = kinect.hdfaceStatusToString(status);
				DrawString(kinect.rgbImage, HexString(kinect.hdfaceTrackingId[i]), cv::Point(x, 730));
				DrawString(kinect.rgbImage, HexString(status.first), cv::Point(x, 760));
				DrawString(kinect.rgbImage, HexString(status.second), cv::Point(x, 790));
				DrawString(kinect.rgbImage, statusS.first, cv::Point(x, 820));
				DrawString(kinect.rgbImage, statusS.second, cv::Point(x, 850));
			}*/

			/*if (!depth) cv::imshow("rgb", kinect.rgbImage); // RGB
			else cv::imshow("depth", img); // Depth

			auto key = cv::waitKey(1);
			if (key == 'p') hideYourself = !hideYourself;
			if (key == 'q') break;*/
		}

		cv::destroyAllWindows();
	}

	void trackFace(sf::Vector2<double>* FacePosition, sf::Rect<double>* FaceRect, sf::Vector2<double>* LastFacePosition, sf::Rect<double>* LastFaceRect, double* LerpAmount, float* headPitch, float* headRoll, float* headYaw, double lastBrowLiftLevel[])
	{
		NtKinect kinect;
		cv::Mat img8;
		cv::Mat img;

		// kinect.setHDFaceModelFlag(true);

		int smoothNum = 5;
		std::vector<cv::Rect> faceRects;
		std::vector<float> facePitchValues;
		std::vector<float> faceRollValues;
		std::vector<float> faceYawValues;
		cv::Rect smoothedFace;
		float smoothedFacePitch = 0, smoothedFaceRoll = 0, smoothedFaceYaw = 0;

		while (bThreadRunning.load())
		{
			// Enable RGB image
			if (!depth) kinect.setRGB();

			// Enable Depth image
			if (depth)
			{
				kinect.setDepth();
				kinect.depthImage.convertTo(img8, CV_8UC1, 255.0 / 4500);
				cv::cvtColor(img8, img, cv::COLOR_GRAY2BGR);
			}

			// Track skeleton
			kinect.setSkeleton();

			// Track face
			if (!depth) kinect.setFace(); // RGB Image
			else kinect.setFace(false); // Depth Image

			// Get HD Face!
			//kinect.setHDFace();

			// Hide your body
			if (hideYourself) kinect.rgbImage.setTo(cv::Scalar(0, 0, 0));

			// Plot points for skeletons
			for (auto person : kinect.skeleton)
			{
				for (auto joint : person)
				{
					if (joint.TrackingState == TrackingState_NotTracked) continue;
					if (!depth)
					{
						ColorSpacePoint CSP; // RGB
						kinect.coordinateMapper->MapCameraPointToColorSpace(joint.Position, &CSP); // RGB
						cv::rectangle(kinect.rgbImage, cv::Rect((int)CSP.X - 5, (int)CSP.Y - 5, 10, 10), cv::Scalar(200, 0, 0), 2); // RGB
					}
					else
					{
						DepthSpacePoint DSP; // Depth
						kinect.coordinateMapper->MapCameraPointToDepthSpace(joint.Position, &DSP); // Depth
						cv::rectangle(img, cv::Rect((int)DSP.X - 5, (int)DSP.Y - 5, 10, 10), cv::Scalar(200, 0, 0), 2); // Depth
					}
				}
			}

			// Draw rectangle around face
			for (cv::Rect faceRect : kinect.faceRect)
			{
				if (faceRects.size() <= smoothNum) faceRects.insert(faceRects.begin(), kinect.faceRect[0]);
				if (faceRects.size() > smoothNum) faceRects.pop_back();

				std::cout << faceRects.size() << " - ";

				smoothedFace = cv::Rect();
				for (cv::Rect fR : faceRects)
				{
					std::cout << "(" << fR.x << "," << fR.y << "," << fR.width << "," << fR.height << ") ";
					smoothedFace.x += fR.x;
					smoothedFace.y += fR.y;
					smoothedFace.width += fR.width;
					smoothedFace.height += fR.height;
				}

				smoothedFace.x /= smoothNum;
				smoothedFace.y /= smoothNum;
				smoothedFace.width /= smoothNum;
				smoothedFace.height /= smoothNum;

				cv::rectangle(!depth ? kinect.rgbImage : img, faceRect, cv::Scalar(255, 255, 0), 2);
			}

			if (kinect.faceRect.size() == 0)
			{
				smoothedFace.y = -1;
			}

			// Draw face points
			for (vector<PointF> pVector : kinect.facePoint)
			{
				for (PointF facePoint : pVector)
				{
					cv::rectangle(!depth ? kinect.rgbImage : img, cv::Rect((int)facePoint.X - 5, (int)facePoint.Y - 5, 10, 10), cv::Scalar(0, 255, 255), 2);
				}
			}

			// Get face rotation
			for (int i = 0; i < kinect.faceDirection.size(); i++)
			{
				smoothedFacePitch = 0; smoothedFaceRoll = 0; smoothedFaceYaw = 0;
				if (facePitchValues.size() <= smoothNum) facePitchValues.insert(facePitchValues.begin(), kinect.faceDirection[0][0]);
				if (facePitchValues.size() > smoothNum) facePitchValues.pop_back();
				if (faceRollValues.size() <= smoothNum) faceRollValues.insert(faceRollValues.begin(), kinect.faceDirection[0][1]);
				if (faceRollValues.size() > smoothNum) faceRollValues.pop_back();
				if (faceYawValues.size() <= smoothNum) faceYawValues.insert(faceYawValues.begin(), kinect.faceDirection[0][2]);
				if (faceYawValues.size() > smoothNum) faceYawValues.pop_back();
				for (float sN : facePitchValues)
				{
					smoothedFacePitch += sN;
				}
				smoothedFacePitch /= smoothNum;
				for (float sN : faceRollValues)
				{
					smoothedFaceRoll += sN;
				}
				smoothedFaceRoll /= smoothNum;
				for (float sN : faceYawValues)
				{
					smoothedFaceYaw += sN;
				}
				smoothedFaceYaw /= smoothNum;
				cv::Vec3f dir = kinect.faceDirection[i];
				DrawString(!depth ? kinect.rgbImage : img, "pitch: " + to_string(dir[0]), cv::Point(200 * i + 50, 30));
				DrawString(!depth ? kinect.rgbImage : img, "roll: " + to_string(dir[1]), cv::Point(200 * i + 50, 60));
				DrawString(!depth ? kinect.rgbImage : img, "yaw: " + to_string(dir[2]), cv::Point(200 * i + 50, 90));
			}

			// Get face props
			for (int i = 0; i < kinect.faceProperty.size(); i++)
			{
				for (int j = 0; j < FaceProperty_Count; j++)
				{
					int v = kinect.faceProperty[i][j];
					cv::putText(!depth ? kinect.rgbImage : img, faceProp[j] + ": " + state[v], cv::Point(200 * i + 50, 30 * j + 120), cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(232, 232, 0), 1, cv::LINE_AA); // RGB
				}
			}

			if (!kinect.faceRect.empty())
			{
				*LastFaceRect = *FaceRect;
				*LastFacePosition = *FacePosition;
				*LerpAmount = 0;

				FaceRect->left = (double)(1920 - smoothedFace.x) / 1920.;
				FaceRect->top = (double)(smoothedFace.y+200) / 1080.;
				FaceRect->width = (double)(smoothedFace.width) / 1920.;
				FaceRect->height = (double)(smoothedFace.height) / 1080.;

				*FacePosition = sf::Vector2<double>(FaceRect->left + (FaceRect->width / 2.), FaceRect->top + (FaceRect->height / 2.));

				*headPitch = smoothedFacePitch;
				*headRoll = smoothedFaceRoll;
				*headYaw = smoothedFaceYaw;
			}
			else if (kinect.faceRect.empty())
			{
				// FaceRect->top = -1;
			}

			/*Camera >> CameraFrame;

			std::vector<Rect2i> Rectangles = MyFace.DetectFaceRectangles(CameraFrame);

			if (!Rectangles.empty())
			{
				*LastFaceRect = *FaceRect;
				*LastFacePosition = *FacePosition;
				*LerpAmount = 0;

				FaceRect->left = (double)(Rectangles[0].x)/640.;
				FaceRect->top = (double)(Rectangles[0].y)/480.;
				FaceRect->width = (double)(Rectangles[0].width)/640.;
				FaceRect->height = (double)(Rectangles[0].height)/480.;

				*FacePosition = sf::Vector2<double>(FaceRect->left + (FaceRect->width / 2.), FaceRect->top + (FaceRect->height / 2.));
			}
			/*
			std::vector<Rect> landmarkRects = Rectangles;
			for (Rect& face : landmarkRects)
			{
				Rect newFace = face;

				newFace.y -= 32;
				newFace.height += 64;
				newFace.width = newFace.height;
				newFace.x -= (newFace.width - face.width) / 2;

				face = newFace;
			}

			bool landmarkSuccessful = false;
			std::vector<std::vector<Point2f>> partialLandmarks;

			{
				std::vector<std::vector<Point2f>> landmarks;
				landmarkSuccessful = facemark->fit(CameraFrame, landmarkRects, landmarks);

				if (landmarkSuccessful)
				{
					for (int Index = 0; Index < landmarks.size(); Index++)
					{
						partialLandmarks.push_back(std::vector<Point2f>());

						for (int Jdex = 0; Jdex < landmarks[Index].size(); Jdex++)
						{
							if (std::find(std::begin(landmarkCoordsArray), std::end(landmarkCoordsArray), Jdex) != std::end(landmarkCoordsArray))
							{
								partialLandmarks[Index].push_back(landmarks[Index][Jdex]);
							}
						}
					}
				}
			}

			if (landmarkSuccessful)
			{
				*lastEyeOpenLevel = *eyeOpenLevel;
				*lastBrowLiftLevel = *browLiftLevel;

				double eyeOpen[2];
				double pupilWidth[2];
				double browLift[2];

				std::vector<Point2f> trackerPoints = partialLandmarks[0];

				pupilWidth[0] = ((euclidianDistance(trackerPoints[4], trackerPoints[5]) + euclidianDistance(trackerPoints[2], trackerPoints[3])) / 2);

				pupilWidth[1] = ((euclidianDistance(trackerPoints[6], trackerPoints[7]) + euclidianDistance(trackerPoints[8], trackerPoints[9])) / 2);

				eyeOpenLevel[0] = ((euclidianDistance(trackerPoints[2], trackerPoints[5]) + euclidianDistance(trackerPoints[4], trackerPoints[3])) / 2) / pupilWidth[0];

				eyeOpenLevel[1] = ((euclidianDistance(trackerPoints[6], trackerPoints[9]) + euclidianDistance(trackerPoints[7], trackerPoints[8])) / 2) / pupilWidth[1];

				browLiftLevel[0] = verticalDistance(trackerPoints[0], midpoint(trackerPoints[5], trackerPoints[4])) / pupilWidth[0];

				browLiftLevel[1] = verticalDistance(trackerPoints[1], midpoint(trackerPoints[9], trackerPoints[8])) / pupilWidth[1];

			}*/
		}
	}

	void beginTrackingThread(sf::Vector2<double>* FacePosition, sf::Rect<double>* FaceRect, sf::Vector2<double>* LastFacePosition, sf::Rect<double>* LastFaceRect, double* LerpAmount, float* headPitch, float* headRoll, float* headYaw, double lastBrowLiftLevel[])
	{
		/*if (!Camera.open(0))
		{
			throw "Unable to open camera.";
		}*/

		bThreadRunning = true;
		TrackingThread = std::thread([this, FacePosition, FaceRect, LastFacePosition, LastFaceRect, LerpAmount, headPitch, headRoll, headYaw, lastBrowLiftLevel]
		{this->trackFace(FacePosition, FaceRect, LastFacePosition, LastFaceRect, LerpAmount, headPitch, headRoll, headYaw, lastBrowLiftLevel);});
	}

	void endTrackingThread()
	{
		bThreadRunning = false;
		TrackingThread.join();
	}
};