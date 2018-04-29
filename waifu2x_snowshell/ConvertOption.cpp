#include "ConvertOption.h"

ConvertOption::ConvertOption() {
	NoiseLevel = CO_NOISE_NONE;
	ScaleRatio = L"1.0";
	IsTTAEnabled = false;
	InputFilePath = L"";
	OutputFolderName = L"";
	CoreNum = 0;
	NoLabel = false;
}

ConvertOption::ConvertOption(std::wstring inputFile, int noiseLevel, std::wstring scaleRatio, bool ttaEnabled, std::wstring OutputFolderName) {
	this->NoiseLevel = noiseLevel;
	this->ScaleRatio = scaleRatio;
	this->IsTTAEnabled = ttaEnabled;
	this->InputFilePath = inputFile;
	this->OutputFolderName = OutputFolderName;
	this->CoreNum = 0;
	this->NoLabel = false;
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

void ConvertOption::setCoreNum(int coreNum) {
	this->CoreNum = coreNum;
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

int ConvertOption::getCoreNum() {
	return this->CoreNum;
}

bool ConvertOption::getNoLabel() {
	return this->NoLabel;
}

void ConvertOption::setNoLabel(bool noLabel) {
	this->NoLabel = NoLabel;
}