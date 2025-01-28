#pragma once
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <fstream>
#include <string>
#include <regex>

class Streamdeck
{
private:
	std::thread streamdeckThread;
	std::atomic<bool> threadRunning = false;

public :
	std::map<char, int> streamdeckData;
	std::map<std::string, std::string> accessoryData;

	std::map<char, int> processJSON(std::string fileLocation)
	{
		std::map<char, int> returnJSON;
		
		std::string rawJSON;

		std::ifstream jsonFile;
		jsonFile.open(fileLocation);

		if (jsonFile.is_open())
		{
			jsonFile >> rawJSON;
			jsonFile.close();
		}

		if (rawJSON.length() < 2) { return {}; }

		std::cout << "Emotion File: " << rawJSON << std::endl;

		rawJSON = rawJSON.substr(1, rawJSON.length() - 2);
		if (std::regex_match(rawJSON, std::regex("\"e\":\\d,\"s\":\\d,\"v\":\\d\\d,\"d\":\\d,\"t\":\\d\\d\\d\\d\\d\\d")))
		{
			while (rawJSON.find(":") != std::string::npos)
			{
				int colonPos = rawJSON.find(":");

				char key = rawJSON.substr(colonPos - 2, 1)[0];
				rawJSON = rawJSON.substr(colonPos + 1, rawJSON.length() - colonPos - 1);

				int data = 0;

				if (rawJSON.find(",") != std::string::npos) { data = std::stoi(rawJSON.substr(0, rawJSON.find(","))); }
				else { data = std::stoi(rawJSON); }

				returnJSON.emplace(key, data);
			}
		}

		return returnJSON;
	}

	std::map<std::string, std::string> processAcc(std::string fileLocation)
	{
		std::map<std::string, std::string> returnJSON;

		std::string rawJSON;

		std::ifstream jsonFile;
		jsonFile.open(fileLocation);

		if (jsonFile.is_open())
		{
			jsonFile >> rawJSON;
			jsonFile.close();
		}

		std::cout << "Accessory File: " << rawJSON << std::endl;

		if (rawJSON.length() < 2) { return {}; }

		std::smatch m;

		// std::regex r("\"\\w + \":\\s\"[^ \\s, ] + ");

		while (std::regex_search(rawJSON, m, std::regex("\"(\\w+)\":\\s*\"([^\\s,\"]+)\"")))
		{
			std::string key = "";
			std::string val = "";

			int i = 1;

			if (m.size() >= 1) { key = m[1]; }
			if (m.size() >= 2) { val = m[2]; }

			rawJSON = m.suffix();
			returnJSON.insert(std::make_pair(key, val));
		}

		return returnJSON;
	}

	void processStreamdeckData(int* emotion, bool* eyeShift, int* minVolume, bool* debug, int* touchId, const int* lastTouchId, std::string* accessory, std::string* aId, const std::string* lastAId)
	{
		while (threadRunning.load())
		{
			// system("\"D:/Apps/Touch Portal/plugins/adb/platform-tools/adb\" pull sdcard/Stremdeck/emotion.json \"D:/Projects/Visual Studio Projects/C++/ByteCamera/x64/Debug/streamdeckdata.json\"");
			system("adb -d pull sdcard/Stremdeck/emotion.json ./streamdeckdata.json");
			// system("adb pull sdcard/Stremdeck/emotion.json \"D:/Projects/Visual Studio Projects/C++/ByteCamera/x64/Debug/streamdeckdata.json\"");

			streamdeckData = processJSON("streamdeckdata.json");

			accessoryData = processAcc("accessories.json");

			if (streamdeckData.count('e') == 0) { continue; }

			for (const auto& dataValue : streamdeckData)
			{
				switch (dataValue.first)
				{
					case 'e':
						if (streamdeckData['t'] != *lastTouchId) { *emotion = dataValue.second; }
						break;

					case 's':
						if (*eyeShift != dataValue.second) { *eyeShift = dataValue.second; }
						break;

					case 'v':
						if (*minVolume != dataValue.second) { *minVolume = dataValue.second; }
						break;

					case 'd':
						if (*debug != dataValue.second) { *debug = dataValue.second; }
						break;

					case 't':
						if (*touchId != dataValue.second) { *touchId = dataValue.second; }
						break;
				}
			}

			*accessory = accessoryData["accessory"];
			*aId = accessoryData["purchaseTime"];

		}

		std::cout << "thread closing // " << threadRunning.load() << std::endl;
	}

	void runStreamdeckThread(int* emotion, bool* eyeShift, int* minVolume, bool* debug, int* touchId, const int* lastTouchId, std::string* accessory, std::string* aId, const std::string* lastAId)
	{
		threadRunning = true;
		streamdeckThread = std::thread([this, emotion, eyeShift, minVolume, debug, touchId, lastTouchId, accessory, aId, lastAId]
		{this->processStreamdeckData(emotion, eyeShift, minVolume, debug, touchId, lastTouchId, accessory, aId, lastAId);});
	}

	void stopStreamdeckThread()
	{
		threadRunning = false;
		streamdeckThread.join();
	}
};