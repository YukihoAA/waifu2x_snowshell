#pragma once

#include <string>

#undef MAX_PATH
#define MAX_PATH SHRT_MAX

class ConvertOption {
private:
	int NoiseLevel;	// 0:none, 1:low, 2:high, 3:very high
	int CoreNum;	// Enable Only in CPU
	bool NoLabel;
	bool IsTTAEnabled;
	std::wstring ScaleRatio;
	std::wstring InputFilePath;
	std::wstring OutputFolderName;

public:
	const static int	CO_NOISE_NONE = 0;
	const static int	CO_NOISE_LOW = 1;
	const static int	CO_NOISE_HIGH = 2;
	const static int	CO_NOISE_VERY_HIGH = 3;

public:
	ConvertOption();
	ConvertOption(std::wstring inputFile, int noiseLevel = ConvertOption::CO_NOISE_NONE, std::wstring scaleRatio = L"1.0", bool ttaEnabled = true, std::wstring OutputFolderName = L"");
	void setNoiseLevel(int noiseLevel);
	void setScaleRatio(std::wstring scaleRatio);
	void setTTAEnabled(bool ttaEnabled);
	void setInputFilePath(std::wstring inputFile);
	void setOutputFolderName(std::wstring OutputFolderName);
	void setCoreNum(int coreNum);
	int getNoiseLevel();
	std::wstring getScaleRatio();
	bool getTTAEnabled();
	std::wstring getInputFilePath();
	std::wstring getOutputFolderName();
	int getCoreNum();
	bool getNoLabel();
	void setNoLabel(bool noLabel);
};