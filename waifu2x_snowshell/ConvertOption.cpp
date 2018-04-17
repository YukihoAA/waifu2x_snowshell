#include "ConvertOption.h"

ConvertOption::ConvertOption() {
	NoiseLevel = 0;
	ScaleRatio = 1.0;
	IsTTAEnabled = false;
	InputFilePath = L"";
	OutputFolderName = L"";
}

ConvertOption::ConvertOption(std::wstring inputFile, int noiseLevel, std::wstring scaleRatio, bool ttaEnabled, std::wstring OutputFolderName) {
	this->NoiseLevel = noiseLevel;
	this->ScaleRatio = scaleRatio;
	this->IsTTAEnabled = ttaEnabled;
	this->InputFilePath = inputFile;
	this->OutputFolderName = OutputFolderName;
}

void ConvertOption::setNoiseLevel(int noiseLevel) {
	this->NoiseLevel = noiseLevel;
}

void ConvertOption::setScaleRatio(std::wstring scaleRatio) {
	this->ScaleRatio = scaleRatio;
}

void ConvertOption::setTTAEnabled(bool ttaEnabled) {
	this->IsTTAEnabled = ttaEnabled;
}

void ConvertOption::setInputFilePath(std::wstring inputFile) {
	this->InputFilePath = inputFile;
}

void ConvertOption::setOutputFolderName(std::wstring OutputFolderName) {
	this->OutputFolderName = OutputFolderName;
}

int ConvertOption::getNoiseLevel() {
	return this->NoiseLevel;
}

std::wstring ConvertOption::getScaleRatio() {
	return this->ScaleRatio;
}

bool ConvertOption::getTTAEnabled() {
	return this->IsTTAEnabled;
}

std::wstring ConvertOption::getInputFilePath() {
	return this->InputFilePath;
}

std::wstring ConvertOption::getOutputFolderName() {
	return this->OutputFolderName;
}
