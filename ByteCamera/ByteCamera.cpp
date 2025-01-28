#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <string>
#include <map>
#include <fstream>
#include <ctime>
#include <time.h>
#include <chrono>

#include "MicVolumeManager.h"
#include "Stremdeck.h"
#include "FaceTrackingThread.h"

const float mapValue(const float& n, const float& start1, const float& end1, const float& start2, const float& end2)
{
	return (((n - start1) / (end1 - start1)) * (end2 - start2)) + start2;
}

const double lerp(const double& x, const double& y, const double& t, const bool& bEnableOutOfBounds=false)
{
	if (!bEnableOutOfBounds && (t > 1 || t < 0)) { return t > 1 ? y : x; }

	return x + t * (y - x);
	// return t * y + (1 - t) * x;
}

double iEuclidianDistance(const sf::Vector2i& a, const sf::Vector2i& b)
{
	return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}

int main()
{
	// FPS Management
	sf::Clock clock = sf::Clock::Clock();
	sf::Time lastTime = clock.getElapsedTime();
	sf::Time currentTime;

	// File Path
	const std::string path = "D:/Projects/Visual Studio Projects/C++/ByteCamera/x64/Release";

	// Loading LastTouch and LastAccessoryTime
	int lastTouchId = -1;
	std::string lastAccessoryTime = "";

	std::cout << "Loading Last Touch Id: ";

	std::ifstream lastTouchIdFile;
	lastTouchIdFile.open("lt.file");

	while (lastTouchId == -1)
	{
		if (lastTouchIdFile.is_open())
		{
			std::string lastNum;
			lastTouchIdFile >> lastNum;
			lastTouchId = std::stoi(lastNum);
			//lastTouchIdFile.close();
		}
	}
	
	lastTouchIdFile.close();

	std::cout << lastTouchId << std::endl;

	std::cout << "Loading Last Accessory Time: ";
	std::ifstream lastAccessoryFile;
	lastAccessoryFile.open("la.file");

	if (lastAccessoryFile.is_open())
	{
		lastAccessoryFile >> lastAccessoryTime;
		lastAccessoryFile.close();
	}

	std::cout << lastAccessoryTime << std::endl;

	// Fonts
	sf::Font interFont;
	std::cout << "Loading font asset" << std::endl;
	if (!interFont.loadFromFile(path + "/intermed.ttf"))
	{
		std::cout << "Error when loading font asset!" << std::endl;
	}

	// List available sound devices
	std::vector<std::string> soundDevices = sf::SoundRecorder::getAvailableDevices();

	for (std::string device : soundDevices)
	{
		std::cout << device << std::endl;
	}

	// Create VolumeManager instance, and begin recording
	VolumeManager volManager;
	std::cout << "Setting Volume Manager input device" << std::endl;
	if (!volManager.setDevice(soundDevices[0]))
	{
		std::cout << "Error on setting Volume Manager input device!" << std::endl;
	}
	volManager.start();

	// Create FaceTrackingThread instance (after creating the necessary variables), and start thread
	sf::Vector2<double> lastFacePositionRaw; sf::Rect<double> lastFaceRectRaw;
	sf::Vector2<double> facePositionRaw; sf::Rect<double> faceRectRaw;
	sf::Vector2<double> facePosition; sf::Rect<double> faceRect;
	double eyeOpenLevelRaw[2], browLiftLevelRaw[2], lastEyeOpenLevelRaw[2], lastBrowLiftLevelRaw[2];
	double eyeOpenLevel[2], browLiftLevel[2];
	double lerpAmount = 0;
	float headPitch, headRoll, headYaw;
	FaceTracker faceTrackerManager;

	faceTrackerManager.beginTrackingThread(&facePositionRaw, &faceRectRaw, &lastFacePositionRaw, &lastFaceRectRaw, &lerpAmount, &headPitch, &headRoll, &headYaw, lastBrowLiftLevelRaw);

	// Debug Text
	sf::Text debugText;
	debugText.setFont(interFont);
	debugText.setCharacterSize(16);
	debugText.setString("Hello, World!");

	// Textures
	sf::RenderTexture outputTexture;
	std::cout << "Creating output texture" << std::endl;
	if (!outputTexture.create(3840, 2160))
	{
		std::cout << "Error when creating output texture!" << std::endl;
	}

	sf::Texture backgroundTexture;
	std::cout << "Loading backgroundTexture" << std::endl;
	if (!backgroundTexture.loadFromFile(path + "/bg/reg.png"))
	{
		std::cout << "Error when loading backgroundTexture!" << std::endl;
	}

	sf::Texture microphoneTexture;
	std::cout << "Loading microphoneTexture" << std::endl;
	if (!microphoneTexture.loadFromFile(path + "/fg/mic.png"))
	{
		std::cout << "Error when loading microphoneTexture!" << std::endl;
	}

	sf::Texture cloudTexture;
	std::cout << "Loading cloudTexture" << std::endl;
	if (!cloudTexture.loadFromFile(path + "/bg/clouds.png"))
	{
		std::cout << "Error when loading cloudTexture!" << std::endl;
	}
	int cloudOffset = rand() % 3600;

	sf::Texture byteTextures[2] = { };
	std::cout << "Loading byte 0 texture" << std::endl;
	if (!byteTextures[0].loadFromFile(path + "/fg/byte base 0.png"))
	{
		std::cout << "Error when loading byte 0 texture!" << std::endl;
	}
	std::cout << "Loading byte 1 texture" << std::endl;
	if (!byteTextures[1].loadFromFile(path + "/fg/byte base 1.png"))
	{
		std::cout << "Error when loading byte 1 texture!" << std::endl;
	}

	sf::Texture emotionTextures[7][3]; // Index 0 is reserved for eye expressions with no voice + blinking

	std::cout << "Loading byte's eye" << std::endl;
	if (!emotionTextures[0][0].loadFromFile(path + "/fg/byte eyes.png"))
	{
		std::cout << "Error when loading byte's eye!" << std::endl;
	}
	std::cout << "Loading byte's blinking eye" << std::endl;
	if (!emotionTextures[0][1].loadFromFile(path + "/fg/byte eyes closed updated.png"))
	{
		std::cout << "Error when loading byte's blinking eye!" << std::endl;
	}
	std::cout << "Loading byte's blinking eye mask" << std::endl;
	if (!emotionTextures[0][2].loadFromFile(path + "/fg/byte eyes closed mask.png"))
	{
		std::cout << "Error when loading byte's blinking eye mask!" << std::endl;
	}

	{
		std::string emotions[7] = { "", "nt", "hp", "ag", "sd", "sc", "eb" };
		for (int i = 1; i < 7; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				std::string fileName = emotions[i] + std::to_string(j) + ".png";
				std::cout << "Loading Brow: " << "/fg/brows/" + fileName << std::endl;
				if (!emotionTextures[i][j].loadFromFile(path + "/fg/brows/" + fileName))
				{
					std::cout << "Error when loading emotion texture " << fileName << std::endl;
				}
			}
		}
	}

	sf::Texture eyeTextures[7][3];
	for (int i = 1; i < 7; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			std::cout << "Filling empty eyes: " << std::to_string(int((double(3*i + j)/21.)*100)) + "%" << std::endl;
			if (!eyeTextures[i][j].loadFromFile(path + "/fg/eyes/empty.png"))
			{
				std::cout << "Error when loading empty eye # " << (3 * i + j) << std::endl;
			}
		}
	}
	{
		std::map<std::vector<int>, std::string> eyes;
		eyes[{2, 2}] = "hp2";
		eyes[{3, 2}] = "ag2";
		eyes[{4, 2}] = "sd2";
		eyes[{5, 1}] = "sc1";
		eyes[{5, 2}] = "sc2";
		eyes[{6, 0}] = "eb0";
		eyes[{6, 1}] = "eb1";
		eyes[{6, 2}] = "eb2";

		for (const auto& pair : eyes)
		{
			std::string fileName = pair.second + ".png";
			std::cout << "Loading Eye: " << "/fg/eyes/" + fileName << " on index [" << pair.first[0] << "][" << pair.first[1] << "]" << std::endl;
			if (!eyeTextures[pair.first[0]][pair.first[1]].loadFromFile(path + "/fg/eyes/" + fileName))
			{
				std::cout << "Error when loading eye texture " << fileName << std::endl;
			}
		}
	}

	std::string accessory = "empty";
	std::map<std::string, sf::Texture> accessoryTextures;
	std::cout << "Loading empty accessory" << std::endl;
	if (!accessoryTextures["empty"].loadFromFile(path + "/fg/accessories/empty.png"))
	{
		std::cout << "Error when loading empty accessory" << std::endl;
	}
	std::cout << "Loading star glasses" << std::endl;
	if (!accessoryTextures["star-glasses"].loadFromFile(path + "/fg/accessories/star-glasses.png"))
	{
		std::cout << "Error when loading star glasses" << std::endl;
	}


	sf::Texture skyRingTexture;
	std::cout << "Loading skyRingTexture" << std::endl;
	if (!skyRingTexture.loadFromFile(path + "/bg/skyring.png"))
	{
		std::cout << "Error when loading skyRingTexture!" << std::endl;
	}
	sf::Image skyRing = skyRingTexture.copyToImage();

	sf::Texture cloudRingTexture;
	std::cout << "Loading cloudRingTexture" << std::endl;
	if (!skyRingTexture.loadFromFile(path + "/bg/cloudring.png"))
	{
		std::cout << "Error when loading cloudRingTexture!" << std::endl;
	}
	sf::Image cloudRing = skyRingTexture.copyToImage();

	// Sprites
	sf::Sprite backgroundSprite;
	backgroundSprite.setTexture(backgroundTexture);

	sf::Sprite microphoneSprite;
	microphoneSprite.setTexture(microphoneTexture);

	sf::Sprite cloudSprite;
	cloudSprite.setTexture(cloudTexture);

	sf::Sprite byteSprite;
	byteSprite.setTexture(byteTextures[0]);

	sf::Sprite eyeSprite;
	eyeSprite.setTexture(emotionTextures[0][0]);

	sf::Sprite eyeCloseSprite;
	eyeCloseSprite.setTexture(emotionTextures[0][1]);

	sf::Sprite eyeMaskSprite;
	eyeMaskSprite.setTexture(emotionTextures[0][2]);

	sf::Sprite browSprites;
	browSprites.setTexture(emotionTextures[1][0]);

	sf::Sprite eyeDetailSprites;
	eyeDetailSprites.setTexture(eyeTextures[1][0]);

	sf::Sprite accessorySprite;
	accessorySprite.setTexture(accessoryTextures[accessory]);

	// Variables
	int emotion = 0, touchId = lastTouchId;
	bool debug = true, eyeSwitch = false;
	double emotionCooldown = 0;
	std::string accessoryTime = lastAccessoryTime;

	// Last Variables (To avoid changing to the same value)
	int lastCloudPos = 0;
	sf::Vector2f lastEyePos = sf::Vector2f();
	int lastMonitor = 0;
	int lastTalkingLevel = 0;
	int lastEmotion = 0;
	bool lastDebug = false, lastEyeSwitch = false;
	sf::Vector2i lastMousePos = sf::Mouse::getPosition();

	// Create Streamdeck instance, and start thread
	Streamdeck streamdeckManager;
	streamdeckManager.runStreamdeckThread(&emotion, &eyeSwitch, &volManager.minVolume, &debug, &touchId, &lastTouchId, &accessory, &accessoryTime, &lastAccessoryTime);

	// Main window creation
	sf::VideoMode renderWindowVideoMode = sf::VideoMode(1920, 1080);
	sf::ContextSettings settings;
	settings.antialiasingLevel = 0;
	sf::RenderWindow window(renderWindowVideoMode, "Byte Webcam - C++ Edition", sf::Style::None/*, 7U, settings*/);
	window.setFramerateLimit(60);

	// Create debug window
	sf::VideoMode debugVideoMode = sf::VideoMode(800, 450);
	sf::RenderWindow debugWindow(debugVideoMode, "Debug Window");

	volManager.baseHeight = debugWindow.getSize().y / 2;

	float eyeOpenTime = 0;

	// Main Loop
	while (window.isOpen())
	{
		// FPS Calculations
		lastTime = currentTime;
		currentTime = clock.getElapsedTime();
		double deltaTime = double(currentTime.asSeconds()) - double(lastTime.asSeconds());
		int fps = 1. / deltaTime;
		

		if (emotionCooldown > 0) { emotionCooldown -= deltaTime; }

		// Event Pooling
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
			{
				window.close();
			}
		}

		// Event Pooling for debug window
		sf::Event debugEvent;
		debugWindow.pollEvent(debugEvent);

		// Face Rect and Position Smoothing

		lerpAmount += 15 * deltaTime; 
		
		faceRect.left = lerp(1 - lastFaceRectRaw.left, 1 - faceRectRaw.left, lerpAmount);
		faceRect.top = lerp(lastFaceRectRaw.top, faceRectRaw.top, lerpAmount);
		faceRect.width = lerp(lastFaceRectRaw.width, faceRectRaw.width, lerpAmount);
		faceRect.height = lerp(lastFaceRectRaw.height, faceRectRaw.height, lerpAmount);

		facePosition.x = lerp(1 - lastFacePositionRaw.x, 1 - facePositionRaw.x, lerpAmount);
		facePosition.y = lerp(lastFacePositionRaw.y, facePositionRaw.y, lerpAmount);


		/*eyeOpenLevel[0] = mapValue(lerp(lastEyeOpenLevelRaw[0], eyeOpenLevelRaw[0], lerpAmount), .45, 0.65, 0, 1);
		eyeOpenLevel[1] = mapValue(lerp(lastEyeOpenLevelRaw[1], eyeOpenLevelRaw[1], lerpAmount), .45, 0.65, 0, 1);*/

		// Determining eye openness level with mouse velocity
		sf::Vector2i mousePos = sf::Mouse::getPosition();
		double velocity = iEuclidianDistance(mousePos, lastMousePos) / deltaTime;
		lastMousePos = mousePos;

		eyeOpenLevel[0] = lerp(eyeOpenLevel[0], 
			mapValue(volManager.talkingLevel, -1, 2, 0.65, 1) + (velocity / 16000), 
			3*lerpAmount/4);
		eyeOpenLevel[1] = mapValue(lerp(lastEyeOpenLevelRaw[1], eyeOpenLevelRaw[1], lerpAmount), .45, 0.65, 0, 1);

		if (eyeOpenLevel[0] > 1) { eyeOpenLevel[0] = 1; } if (eyeOpenLevel[0] < 0) { eyeOpenLevel[0] = 0; }
		if (eyeOpenLevel[1] > 1) { eyeOpenLevel[1] = 1; } if (eyeOpenLevel[1] < 0) { eyeOpenLevel[1] = 0; }

		browLiftLevel[0] = lerp(lastBrowLiftLevelRaw[0], browLiftLevelRaw[0], lerpAmount);
		browLiftLevel[1] = lerp(lastBrowLiftLevelRaw[1], browLiftLevelRaw[1], lerpAmount);

		// Cloud positioning
		int cloudPosition = (int(currentTime.asSeconds() + cloudOffset) % 3600) * -1.0669630452903584328980272297861;
		if (cloudPosition != lastCloudPos) { cloudSprite.setPosition(sf::Vector2f(cloudPosition, 0)); lastCloudPos = cloudPosition; }

		// Byte positioning based on head position
		sf::Vector2<double> anchorPosition = sf::Vector2<double>(renderWindowVideoMode.width / 2, renderWindowVideoMode.height / 2);
		const sf::Vector2f faceOffset = sf::Vector2f((facePosition.x * renderWindowVideoMode.width) - anchorPosition.x, (facePosition.y * renderWindowVideoMode.height) - anchorPosition.y);
		byteSprite.setPosition(faceOffset);

		// Change Byte's head based on MousePosition
		// int currentMonitor = floor(sf::Mouse::getPosition().x / 1920.)/* + 1*/;
		// if (currentMonitor != lastMonitor) { byteSprite.setTexture(byteTextures[currentMonitor]); lastMonitor = currentMonitor; }

		// Change Byte's head based on headRoll
		int currentMonitor = headRoll > 20 ? 0 : 1;
		if (currentMonitor != lastMonitor) { byteSprite.setTexture(byteTextures[currentMonitor]); lastMonitor = currentMonitor; }

		// Eye position based on Mouse Position and currentMonitor
		sf::Vector2f eyePos = sf::Vector2f(sf::Mouse::getPosition() + sf::Vector2i(1920, 0));
		if (currentMonitor == 1 && eyePos.x < 1920) eyePos.x = 960;
		else if (currentMonitor == 0 && eyePos.x > 1920) eyePos.x = 960;
		eyePos.x = mapValue(int(eyePos.x) % 1920, 0, 1919, -351, 63) + (171 * currentMonitor);
		eyePos.y = mapValue(eyePos.y, 0, 1079, -144, 102);
		
		// Subtle eye movement + eye opening depending on volume & mouse movement
		const sf::Vector2f newEyePos = eyePos + sf::Vector2f(faceOffset.x / 1.0625, faceOffset.y / 1.0625); // Changes eye position slightly when moving Byte's head
		eyeSprite.setPosition(newEyePos); eyeDetailSprites.setPosition(newEyePos); eyeMaskSprite.setPosition(newEyePos); lastEyePos = eyePos;
		browSprites.setPosition(newEyePos);

		// Moving sprite with eye
		accessorySprite.setPosition(newEyePos);

		double eyeOpenness = (1 - eyeOpenLevel[0]);
		if (eyeOpenness > 1) { eyeOpenness = 1; }

		// Super ugly eyeOpenness fix to change the old look to a more dynamic one

		if (eyeOpenness < 0.225) eyeOpenTime = currentTime.asSeconds();

		eyeCloseSprite.setPosition(newEyePos + sf::Vector2f(0, (currentTime.asSeconds() - eyeOpenTime) > 60 ? eyeOpenness * 234 : 0));

		// Changing emotion information when talking
		if ((lastTalkingLevel != volManager.talkingLevel && volManager.talkingLevel >= 0) || // talking louder
			((emotion != lastEmotion || touchId != lastTouchId)) || // changed emotion
			(emotionCooldown < 0 && browSprites.getTexture() != emotionTextures[1])) // cooldown's over
		{
			// Reset emotion if cooldown's over
			if (emotionCooldown < 0 && browSprites.getTexture() != emotionTextures[1]) { emotion = 0; emotionCooldown = 0; }

			// Reset cooldown if new touch
			if (touchId != lastTouchId) { emotionCooldown = 5; }

			// Change sprite
			browSprites.setTexture(emotionTextures[emotion + 1][volManager.talkingLevel >= 0 ? volManager.talkingLevel : 0]);
			eyeDetailSprites.setTexture(eyeTextures[emotion + 1][volManager.talkingLevel >= 0 ? volManager.talkingLevel : 0]);

			// std::cout << "Emotion: " << emotion << "// Touch ID: " << touchId << "// Talking Level: " << volManager.talkingLevel << " // Cooldown: " << emotionCooldown << std::endl;
			// std::cout << "Last Em: " << lastEmotion << "// Last TID: " << lastTouchId << "// Last Talk Lvl: " << lastTalkingLevel << " // Cooldown: " << emotionCooldown << std::endl;

			lastTalkingLevel = volManager.talkingLevel;
			lastEmotion = emotion;
			lastTouchId = touchId;
		}

		// Change accessory
		if (accessoryTime != lastAccessoryTime)
		{
			std::cout << "CHANGED ACCESSORY CHANGED ACCESSORY CHANGED ACCESSORY CHANGED ACCESSORY CHANGED ACCESSORY CHANGED ACCESSORY CHANGED ACCESSORY CHANGED ACCESSORY CHANGED ACCESSORY CHANGED ACCESSORY " << std::endl;
			accessorySprite.setTexture(accessoryTextures[accessory]);
			lastAccessoryTime = accessoryTime;
		}

		// Draw sprites on outputTexture

		sf::Color SkyColor, CloudColor;

		std::time_t currentClockTimeRaw;
		std::time(&currentClockTimeRaw);
		struct std::tm currentClockTime;
		localtime_s(&currentClockTime, &currentClockTimeRaw);
		double CurrentHour = currentClockTime.tm_hour + ((double)currentClockTime.tm_min / 60.0) + ((double)currentClockTime.tm_sec / 3600.0);
		// double CurrentHour = ((double)currentTime.asMilliseconds()/1000.0)/60.0 * 24.0 * 5;
		double TimeToAngle = (CurrentHour / 24.0) * 6.2831853071795864769;

		SkyColor = skyRing.getPixel((int)floor(250 + 225 * cos(TimeToAngle)), (int)floor(250 - 225 * sin(TimeToAngle)));
		CloudColor = cloudRing.getPixel((int)floor(250 + 225 * cos(TimeToAngle)), (int)floor(250 - 225 * sin(TimeToAngle)));

		cloudSprite.setColor(CloudColor);

		outputTexture.clear(SkyColor/*sf::Color(179, 239, 255)*/); // Draw sky
		outputTexture.draw(cloudSprite);               // Draw clouds
		outputTexture.draw(backgroundSprite);          // Draw background
		outputTexture.draw(byteSprite);                // Draw Byte
		outputTexture.draw(eyeSprite);                 // Draw eyes
		outputTexture.draw(eyeCloseSprite);            // Draw eye closing
		outputTexture.draw(eyeMaskSprite);             // "Masks" eye closing
		outputTexture.draw(browSprites);               // Draw Emotion's eyebrow
		outputTexture.draw(eyeDetailSprites);          // Draws the eye's details depending on emotion
		outputTexture.draw(accessorySprite);           // Draws accessory sprites
		outputTexture.draw(microphoneSprite);          // Draws the mic on top
		outputTexture.display();

		// Make sprite with outputTexture
		sf::Sprite canvasSprite;
		canvasSprite.setTexture(outputTexture.getTexture());

		// outputTexture Resizing
		canvasSprite.setScale(sf::Vector2f((float)renderWindowVideoMode.width / 3840.f, (float)renderWindowVideoMode.height / 2160.f));

		// Draw canvas onto window
		window.clear();
		window.draw(canvasSprite);
		//window.draw(volManager.soundWaveform);
		window.display();

		if (debug)
		{
			if (!debugWindow.isOpen()) { debugWindow.create(debugVideoMode, "Debug Window"); }
			// Update Debug Text
			debugText.setString("FPS: " + std::to_string(fps) +
				"\nCurrent Emotion: " + std::to_string(emotion) +
				"\nTouch ID: " + std::to_string(touchId) +
				"\nEmotion Cooldown: " + std::to_string(emotionCooldown) +
				"\nCurrent Hour: " + std::to_string(CurrentHour) + " // Angle: " + std::to_string(360 * TimeToAngle / 6.2831853071795864769) + "°" +
				"\n(" + std::to_string(headPitch) + "," + std::to_string(headRoll) + "," + std::to_string(headYaw) + ")" +
				"\nEye Open Level: " + std::to_string(eyeOpenness) + " // Eye Open Time: " + std::to_string(eyeOpenTime));

			// Creates Face Rectangle
			sf::RectangleShape faceOutline;
			sf::Vector2u windowDimensions = debugWindow.getSize();
			faceOutline.setOutlineThickness(5);
			faceOutline.setOutlineColor(sf::Color(255, 127, 0));
			faceOutline.setFillColor(sf::Color().Transparent);
			faceOutline.setSize(sf::Vector2f(faceRect.width * debugVideoMode.width, faceRect.height * debugVideoMode.height));
			faceOutline.setPosition((facePosition.x * debugVideoMode.width) - (faceOutline.getSize().x/2), faceRect.top * debugVideoMode.height);

			sf::VertexArray horizontalLine(sf::LineStrip, 2);
			horizontalLine[0].position = sf::Vector2f(0, facePosition.y * debugVideoMode.height);
			horizontalLine[1].position = sf::Vector2f(debugVideoMode.width, facePosition.y * debugVideoMode.height);

			sf::VertexArray verticalLine(sf::LineStrip, 2);
			verticalLine[0].position = sf::Vector2f(facePosition.x * debugVideoMode.width, 0);
			verticalLine[1].position = sf::Vector2f(facePosition.x * debugVideoMode.width, debugVideoMode.height);

			sf::CircleShape facePoint;
			facePoint.setRadius(5);
			facePoint.setFillColor(sf::Color().Red);
			facePoint.setPosition(sf::Vector2f((facePosition.x * debugVideoMode.width) - 5, (facePosition.y * debugVideoMode.height) - 5));

			// Draw Debug Window Info
			debugWindow.clear();
			debugWindow.draw(faceOutline);
			debugWindow.draw(horizontalLine);
			debugWindow.draw(verticalLine);
			debugWindow.draw(facePoint);
			debugWindow.draw(volManager.soundWaveform);
			debugWindow.draw(debugText);
			debugWindow.display();
		}
		else
		{
			if (debugWindow.isOpen()) { debugWindow.close(); }
		}
	}

	if (debugWindow.isOpen()) { debugWindow.close(); }

	volManager.stop();
	streamdeckManager.stopStreamdeckThread();
	faceTrackerManager.endTrackingThread();

	std::cout << "Saving accessory time" << std::endl;

	std::ofstream lastAccessoryFileOut;
	lastAccessoryFileOut.open("la.file");
	lastAccessoryFileOut << accessoryTime;
	lastAccessoryFileOut.close();

	std::cout << "Saving emotion TouchId" << std::endl;

	std::ofstream lastTouchFileOut;
	lastTouchFileOut.open("lt.file");
	lastTouchFileOut << touchId;
	lastTouchFileOut.close();

	return 0;
}