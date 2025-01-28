#pragma once

#include <SFML/Audio.hpp>
#include <iostream>
#include <cmath>

class VolumeManager : public sf::SoundRecorder
{
public:
	float currentVolume = 0;
	double talking = 0;
	int talkingLevel = 0;
	int minVolume = 25;
	sf::VertexArray soundWaveform = sf::VertexArray(sf::LineStrip, 100);
	sf::Color waveformColors [3] = { sf::Color::Cyan, sf::Color::Green, sf::Color::Red };
	float baseHeight = 112.5f;
	double amplitudeFactor = 0.01;


	virtual bool onStart()
	{
		for (int i = 0; i < soundWaveform.getVertexCount(); i++)
		{
			soundWaveform[i].position = sf::Vector2f((800 / (soundWaveform.getVertexCount()-2)) * i, baseHeight);
		}

		setProcessingInterval(sf::milliseconds(5));

		return true;
	}

	virtual bool onProcessSamples(const sf::Int16* samples, std::size_t sampleCount)
	{
		// std::cout << *samples << std::endl;

		for (int i = 0; i < soundWaveform.getVertexCount() - 1; i++)
		{
			soundWaveform[(soundWaveform.getVertexCount() - 1) - i].position.y = soundWaveform[(soundWaveform.getVertexCount() - 2) - i].position.y;
			soundWaveform[(soundWaveform.getVertexCount() - 1) - i].color = soundWaveform[(soundWaveform.getVertexCount() - 2) - i].color;
		}
		soundWaveform[0] = (sf::Vertex(sf::Vector2f(0, baseHeight + (amplitudeFactor/2 * *samples))));

		currentVolume = abs(amplitudeFactor * *samples);

		if (currentVolume > minVolume) 
		{ 
			talking = 1; 
			if(talkingLevel < 0) talkingLevel = 0; 
		}

		// std::cout << talkingLevel << " // " << currentVolume << std::endl;

		if (currentVolume > 4 * minVolume && talkingLevel < 1) { talkingLevel = 1; }
		if (currentVolume > 9 * minVolume) { talkingLevel = 2; }
		if (talking > 0) { talking -= 0.025; soundWaveform[0].color = waveformColors[talkingLevel];  }
		if (talking < 0) { talking = 0; talkingLevel = -1; }

		return true;
	}
};