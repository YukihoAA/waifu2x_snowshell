#include "LocaleString.h"


int INT_SETTING_VER = 0;

wstring STRING_MENU_FILE = L"파일";
wstring STRING_MENU_NOISE = L"노이즈 감소";
wstring STRING_MENU_SCALE = L"비율";
wstring STRING_MENU_EXPORT = L"출력";
wstring STRING_MENU_CONFIRM = L"확인창";

wstring STRING_MENU_FILE_IMAGE_SEL = L"이미지 선택 및 실행";
wstring STRING_MENU_FILE_CREDIT = L"정보";
wstring STRING_MENU_FILE_QUIT = L"종료";

wstring STRING_MENU_NOISE_NONE = L"없음";
wstring STRING_MENU_NOISE_LOW = L"낮음";
wstring STRING_MENU_NOISE_MID = L"중간";
wstring STRING_MENU_NOISE_HIGH = L"높음";
wstring STRING_MENU_NOISE_VERY_HIGH = L"최대";

wstring STRING_MENU_SCALE_CUSTOM = L"사용자 설정";

wstring STRING_MENU_GPU_CPU = L"CPU 사용";
wstring STRING_MENU_GPU_GPU = L"GPU 사용";

wstring STRING_MENU_TTA_DISABLED = L"비활성화";
wstring STRING_MENU_TTA_ENABLED = L"활성화";

wstring STRING_MENU_EXPORT_SAME = L"원본 이미지와 같은 폴더";
wstring STRING_MENU_EXPORT_NEW = L"output 폴더";

wstring STRING_MENU_CONFIRM_SHOW = L"보여주기";
wstring STRING_MENU_CONFIRM_SKIP = L"보여주지 않기";
wstring STRING_MENU_CONFIRM_DEBUG = L"변환과정 출력";


int INT_TEXT_TAB = 11;

wstring STRING_TEXT_NOISE = L"노이즈 감소";
wstring STRING_TEXT_NOISE_NONE = L"없음";
wstring STRING_TEXT_NOISE_LOW = L"낮음";
wstring STRING_TEXT_NOISE_MID = L"중간";
wstring STRING_TEXT_NOISE_HIGH = L"높음";
wstring STRING_TEXT_NOISE_VERY_HIGH = L"최대";

wstring STRING_TEXT_SCALE = L"확대 비율";
wstring STRING_TEXT_SCALE_x1_0 = L"x1.0";
wstring STRING_TEXT_SCALE_x1_5 = L"x1.5";
wstring STRING_TEXT_SCALE_x1_6 = L"x1.6";
wstring STRING_TEXT_SCALE_x2_0 = L"x2.0";
wstring STRING_TEXT_SCALE_PREFIX = L"x";
wstring STRING_TEXT_SCALE_POSTFIX = L"";

wstring STRING_TEXT_GPU = L"변환방식";
wstring STRING_TEXT_GPU_CPU = L"CPU";
wstring STRING_TEXT_GPU_CPU_TTA = L"CPU (TTA)";
wstring STRING_TEXT_GPU_GPU = L"GPU";
wstring STRING_TEXT_GPU_GPU_TTA = L"GPU (TTA)";
wstring STRING_TEXT_GPU_VULKAN = L"VULKAN";
wstring STRING_TEXT_GPU_VULKAN_TTA = L"VULKAN (TTA)";

wstring STRING_TEXT_EXPORT = L"출력 폴더";
wstring STRING_TEXT_EXPORT_SAME = L"같은 폴더";
wstring STRING_TEXT_EXPORT_NEW = L"output 폴더";

wstring STRING_TEXT_CONFIRM = L"확인";
wstring STRING_TEXT_CONFIRM_SHOW = L"보여주기";
wstring STRING_TEXT_CONFIRM_SKIP = L"보여주지 않기";
wstring STRING_TEXT_CONFIRM_TITLE = L"확인";
wstring STRING_TEXT_CONFIRM_MESSAGE = L"큰 이미지 파일은\n작업에 많은 시간이 소요될 수 있습니다.\n작업을 진행하시겠습니까?";
wstring STRING_TEXT_CONFIRM_CUSTOM_SCALE_TITLE = L"경고";
wstring STRING_TEXT_CONFIRM_CUSTOM_SCALE_MESSAGE = L"메모리가 부족하면 작업 도중 오류가 발생할 수 있습니다.\n작업을 진행하시겠습니까?";

wstring STRING_TEXT_NOCONVERTER_TITLE = L"오류";
wstring STRING_TEXT_NOCONVERTER_MESSAGE = L"사용 가능한 컨버터가 없습니다.\n윈도우 버전을 확인해주시기 바랍니다.";

wstring STRING_TEXT_TOO_LONG_PATH_TITLE = L"오류";
wstring STRING_TEXT_TOO_LONG_PATH_MESSAGE = L"다음 파일의 파일 경로가 너무 길기 때문에 변환이 불가능합니다.";
wstring STRING_TEXT_TOO_LONG_PATH_MESSAGE_COUNT = L"자";

wstring STRING_TEXT_ABORT_CONVERT_TITLE = L"확인";
wstring STRING_TEXT_ABORT_CONVERT_MESSAGE = L"변환을 중단하시겠습니까?";