# waifu2x - Snowshell [![apm](https://img.shields.io/apm/l/vim-mode.svg)]( [![apm](https://img.shields.io/apm/dm/vim-mode.svg)](https://github.com/YukihoAA/waifu2x_snowshell)) [![Github Releases](https://img.shields.io/github/downloads/YukihoAA/waifu2x_snowshell/total.svg)](https://github.com/YukihoAA/waifu2x_snowshell/releases)

This is GUI Shell for [waifu2x-converter-cpp](https://github.com/DeadSix27/waifu2x-converter-cpp), [waifu2x-caffe](https://github.com/lltcggie/waifu2x-caffe) and [waifu2x-ncnn-vulkan](https://github.com/nihui/waifu2x-ncnn-vulkan) <br/> <br/>
![Preview](https://github.com/YukihoAA/waifu2x_snowshell/blob/master/Preview.PNG) ![Example](https://github.com/YukihoAA/waifu2x_snowshell/blob/master/Example.PNG) <br /> <br/>
You can use Snowshell to make your image better. <br/>
Snowshell uses C++ and Win32API. 

# Usage
Just drag & drop your image files (or folder) in to Snowshell! (multiple image supports) <br/>
To change your conversion setting, select option at menubar. <br/>

## To Use waifu2x-ncnn-vulkan (Supported GPU Only)
> 1. Install latest Graphics Driver. [AMD](https://www.amd.com/en/support) [NVIDIA](https://www.nvidia.co.kr/Download/index.aspx) [INTEL](https://downloadcenter.intel.com/product/80939/Graphics-Drivers) <br/>
> 2. Put [waifu2x-ncnn-vulkan](https://github.com/nihui/waifu2x-ncnn-vulkan/releases) to waifu2x-ncnn-vulkan folder. <br/>
> 3. Select Converter to waifu2x-ncnn-vulkan. <br/>
> 4. Set conversion mode on TTA menu. <br/>
> 4-1. TTA convert mode is slower but higher quality conversion. <br/>
> 5. Snowshell will start conversion using waifu2x-ncnn-vulkan.

## To Use waifu2x-caffe (NVIDIA GPU Only)
> 1. Install latest [NVIDIA Graphics Driver](https://www.nvidia.co.kr/Download/index.aspx). <br/>
> 2. Put [waifu2x-caffe](https://github.com/lltcggie/waifu2x-caffe/releases) to waifu2x-caffe folder. <br/>
> 3. Select Converter to waifu2x-caffe. <br/>
> 4. Set conversion mode on GPU(CPU) menu. <br/>
> 4-1. TTA convert mode is slower but higher quality conversion. <br/>
> 5. Snowshell will start conversion using waifu2x-caffe.

## To Change Model File
> 1. Open config.ini file (if there is no config.ini, launch Snowshell and close it) <br/>
> 2. Set your model file directory with absolute path or relative path. <br/>
> 2-1 You have to use \\\\ instead of \ <br/>
> 2-2. When you use relative path, path is relative to converter's directory. <br/>
> 3. Converters default model directory (relative) <br/>
> 3-1. waifu2x_converter_cpp = models_rgb <br/>
> 3-2. waifu2x_caffe = models\\\\upconv_7_anime_style_art_rgb <br/>
> 3-3. waifu2x_ncnn-vulkan = models-cunet <br/>

## To Use Advanced Option in Converter
> 1. Open config.ini file (if there is no config.ini, launch Snowshell and close it) <br/>
> 2. Check your converter and fill OptionString with currect option. <br/>
> 3. To find option for your converter, please visit converter's HP <br/>
> 3-1. waifu2x_converter_cpp: [waifu2x-converter-cpp](https://github.com/DeadSix27/waifu2x-converter-cpp) <br/>
> 3-2. waifu2x_caffe: [waifu2x-caffe](https://github.com/lltcggie/waifu2x-caffe) <br/>
> 3-3. waifu2x_ncnn-vulkan: [waifu2x-ncnn-vulkan](https://github.com/nihui/waifu2x-ncnn-vulkan) <br/>
> 4. If you want to ignore/override settings that Snowshell supports, add -- to end of optionstring to ignore all settings.<br/>
> 4-1. you cannot override input file name because it is nessesory to make Snowshell works.
