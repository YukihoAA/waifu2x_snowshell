# waifu2x - Snowshell [![apm](https://img.shields.io/apm/l/vim-mode.svg)]( [![apm](https://img.shields.io/apm/dm/vim-mode.svg)](https://github.com/YukihoAA/waifu2x_snowshell)) [![Github Releases](https://img.shields.io/github/downloads/YukihoAA/waifu2x_snowshell/total.svg)](https://github.com/YukihoAA/waifu2x_snowshell/releases)

This is GUI Shell for [waifu2x-converter-cpp](https://github.com/DeadSix27/waifu2x-converter-cpp) and [waifu2x-caffe](https://github.com/lltcggie/waifu2x-caffe) <br/> <br/>
![Preview](https://github.com/YukihoAA/waifu2x_snowshell/blob/master/Preview.PNG) ![Example](https://github.com/YukihoAA/waifu2x_snowshell/blob/master/Example.PNG) <br /> <br/>
You can use Snowshell to make your image better. <br/>
Snowshell uses C++ and Win32API. 

# Usage
Just drag & drop your image files (or folder) in to Snowshell! (multiple image supports) <br/>
To change your conversion setting, select option at menubar. <br/>

## To Use waifu2x-caffe (NVIDIA GPU Only)
> 1. Install latest [NVIDIA Graphics Driver](https://www.nvidia.co.kr/Download/index.aspx).
> 2. Put [waifu2x-caffe](https://github.com/lltcggie/waifu2x-caffe/releases) to waifu2x-caffe folder
> 3. Set GPU to Caffe or Caffe(TTA).
> 3-1. TTA convert mode is slower but higher quality conversion.
> 4. Snowshell will start conversion using waifu2x-caffe. (with TTA option)

## To Use waifu2x-caffe (CPU Mode)
> 1. Put [waifu2x-caffe](https://github.com/lltcggie/waifu2x-caffe/releases) to waifu2x-caffe folder
> 2. Run waifu2x_snowshell.exe and close it ones.
> 3. Open config.ini and set waifu2x_converter_cpp_x64 value to false in [Converter] section.
![image](https://user-images.githubusercontent.com/20253131/53020935-61fc2300-349b-11e9-90ca-127af2bd272c.png)
> 4. Set GPU to Caffe or Caffe(TTA).
> 4-1. TTA convert mode is slower but higher quality conversion.
> 5. Snowshell will start conversion using waifu2x-caffe. (with TTA option)
- For more option, please check [README.md](https://github.com/YukihoAA/waifu2x_snowshell)

## To Change Model File
> 1. Open config.ini file (if there is no config.ini, launch Snowshell and close it) <br/>
> 2. Set your model file directory with absolute path or relative path. <br/>
> 2-1 You have to use \\\\ instead of \ <br/>
> 2-2. When you use relative path, path is relative to converter's directory. <br/>
> 3. Converters default model directory (relative) <br/>
> 3-1. waifu2x_converter_cpp_x64 = models_rgb <br/>
> 3-2. waifu2x_converter_cpp_x86 = models_rgb <br/>
> 3-3. waifu2x_caffe = models\\\\upconv_7_anime_style_art_rgb <br/>

## To Use Advanced Option in Converter
> 1. Open config.ini file (if there is no config.ini, launch Snowshell and close it) <br/>
> 2. Check your converter and fill OptionString with currect option. <br/>
> 3. To find option for your converter, please visit converter's HP <br/>
> 3-1. waifu2x_converter_cpp_x64 (Default): [waifu2x-converter-cpp](https://github.com/DeadSix27/waifu2x-converter-cpp) <br/>
> 3-2. waifu2x_converter_cpp_x86 (For x86 User): [waifu2x-converter-cpp](https://github.com/DeadSix27/waifu2x-converter-cpp) <br/>
> 3-3. waifu2x_caffe (For NVIDIA user): [waifu2x-caffe](https://github.com/lltcggie/waifu2x-caffe) <br/>
> 4. If you want to ignore/override settings that Snowshell supports, add -- to end of optionstring to ignore all settings.<br/>
> 4-1. you cannot override input file name because it is nessesory to make Snowshell works.
