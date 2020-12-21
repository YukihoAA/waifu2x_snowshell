#pragma once

#include <string>

#undef MAX_PATH
#define MAX_PATH SHRT_MAX

class ConvertOption {
private:
	int NoiseLevel;	// -1:none, 0:low, 1:mid, 2:high, 3:very high
	int CoreNum;	// Enable Only in CPU
	int DebugMode;	// 0:Disabled, 1:Snow Converting Log
	bool NoLabel;
	bool ForceCPU;
	bool IsTTAEnabled;
	std::wstring ScaleRatio;
	std::wstring InputFilePath;
	std::wstring OutputFolderName;

public:
	const static int	CO_NOISE_NONE = -1;
	const static int	CO_NOISE_LOW = 0;
	const static int	CO_NOISE_MID = 1;
	const static int	CO_NOISE_HIGH = 2;
	const static int	CO_NOISE_VERY_HIGH = 3;

public:
	ConvertOption();
	ConvertOption(std::wstring inputFile, int noiseLevel = ConvertOption::CO_NOISE_NONE, std::wstring scaleRatio = L"1.0", bool ttaEnabled = false, std::wstring OutputFolderName = L"");
	void setNoiseLevel(int noiseLevel);
	void setCoreNum(int coreNum);
	void setDebugMode(int debugMode);
	void setNoLabel(bool noLabel);
	void setForceCPU(bool forceCPU);
	void setTTAEnabled(bool ttaEnabled);
	void setScaleRatio(std::wstring scaleRatio);
	void setInputFilePath(std::wstring inputFile);
	void setOutputFolderName(std::wstring OutputFolderName);

	int getNoiseLevel();
	int getCoreNum();
	int getDebugMode();
	bool getNoLabel();
	bool getForceCPU();
	bool getTTAEnabled();
	std::wstring getScaleRatio();
	std::wstring getInputFilePath();
	std::wstring getOutputFolderName();
};