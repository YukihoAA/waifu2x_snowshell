# waifu2x - SnowShell [![apm](https://img.shields.io/apm/l/vim-mode.svg)]( [![apm](https://img.shields.io/apm/dm/vim-mode.svg)](https://github.com/YukihoAA/waifu2x_snowshell)) [![Github Releases](https://img.shields.io/github/downloads/YukihoAA/waifu2x_snowshell/total.svg)](https://github.com/YukihoAA/waifu2x_snowshell/releases)

This is GUI Shell for [waifu2x-converter-cpp](https://github.com/tanakamura/waifu2x-converter-cpp) and [waifu2x-caffe](https://github.com/lltcggie/waifu2x-caffe) <br/> <br/>
![Preview](https://github.com/YukihoAA/waifu2x_snowshell/blob/master/Preview.PNG) ![Example](https://github.com/YukihoAA/waifu2x_snowshell/blob/master/Example.PNG) <br /> <br/>
You can use SnowShell to make your image better. <br/>
SnowShell uses C++ and Win32API. 

# Usage
Just drag & drop your image files (or folder) in to SnowShell! (multiple image supports) <br/>
To change your conversion setting, select option at menubar. <br/>

## To Use waifu2x-chaffe (NVIDIA GPU Only)
> 1. Put latest [waifu2x-caffe](https://github.com/lltcggie/waifu2x-caffe/releases) to waifu2x-caffe folder <br/>
> 2. Set GPU to Caffe or Caffe(TTA). <br/>
> 2-1. TTA convert mode is slower but higher quality conversion. <br/>
> 3. SnowShell will start conversion using waifu2x-caffe. (with TTA option) <br/>

## To Change Model File
> 1. Open config.ini file (if there is no config.ini, launch snowshell and close it) <br/>
> 2. Set your model file directory with absolute path or relative path. <br/>
> 2-1 You have to use \\\\ instead of \ <br/>
> 2-2. When you use relative path, path is relative to converter's directory. <br/>
> 3. Converters default model directory (relative) <br/>
> 3-1. waifu2x_converter_cpp_x64 = models_rgb <br/>
> 3-2. waifu2x_converter_cpp_x86 = models_rgb <br/>
> 3-3. waifu2x_caffe = models\\\\upconv_7_anime_style_art_rgb <br/>

## To Use Advanced Option in Converter
> 1. Open config.ini file (if there is no config.ini, launch snowshell and close it) <br/>
> 2. Check your converter and fill OptionString with currect option. <br/>
> 3. To find option for your converter, please visit converter's HP <br/>
> 3-1. waifu2x_converter_cpp_x64 (Default): [waifu2x-converter-cpp](https://github.com/DeadSix27/waifu2x-converter-cpp) <br/>
> 3-2. waifu2x_converter_cpp_x86 (For x86 User): [waifu2x-converter-cpp](https://github.com/DeadSix27/waifu2x-converter-cpp) <br/>
> 3-3. waifu2x_caffe (For NVIDIA user): [waifu2x-caffe](https://github.com/lltcggie/waifu2x-caffe) <br/>
> 4. If you want to ignore/override settings that snowshell supports, add -- to end of option string for ignoreing all of setting's.<br/>
> 4-1. you cannot override input file name because it is nessesory to make snowshell works.
