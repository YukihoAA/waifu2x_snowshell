#pragma once

#include <string>

class ConvertOption {
private:
	int NoiseLevel;	// 0:none, 1:low, 2:high, 3:very high
	bool IsTTAEnabled;
	std::wstring ScaleRatio;
	std::wstring InputFilePath;
	std::wstring OutputFolderName;

public:
	const static int	NOISE_NONE = 0;
	const static int	NOISE_LOW = 1;
	const static int	NOISE_HIGH = 2;
	const static int	NOISE_VERY_HIGH = 3;

public:
	ConvertOption();
	ConvertOption(std::wstring inputFile, int noiseLevel = ConvertOption::NOISE_NONE, std::wstring scaleRatio = L"1.0", bool ttaEnabled = true, std::wstring OutputFolderName = L"");
	void setNoiseLevel(int noiseLevel);
	void setScaleRatio(std::wstring scaleRatio);
	void setTTAEnabled(bool ttaEnabled);
	void setInputFilePath(std::wstring inputFile);
	void setOutputFolderName(std::wstring OutputFolderName);
	int getNoiseLevel();
	std::wstring getScaleRatio();
	bool getTTAEnabled();
	std::wstring getInputFilePath();
	std::wstring getOutputFolderName();

};