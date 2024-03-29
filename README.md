# waifu2x - Snowshell ![apm](https://img.shields.io/github/license/YukihoAA/waifu2x_snowshell) [![Github Releases](https://img.shields.io/github/downloads/YukihoAA/waifu2x_snowshell/total.svg)](https://github.com/YukihoAA/waifu2x_snowshell/releases)

This is GUI Shell for [waifu2x-converter-cpp](https://github.com/DeadSix27/waifu2x-converter-cpp), [waifu2x-caffe](https://github.com/lltcggie/waifu2x-caffe), [waifu2x-ncnn-vulkan](https://github.com/nihui/waifu2x-ncnn-vulkan), [Real-CUGAN](https://github.com/nihui/realcugan-ncnn-vulkan) and [Real-ESRGAN](https://github.com/xinntao/Real-ESRGAN-ncnn-vulkan) <br/> <br/>
![Preview](https://github.com/YukihoAA/waifu2x_snowshell/blob/master/Preview.PNG) ![Example](https://github.com/YukihoAA/waifu2x_snowshell/blob/master/Example.PNG) <br /> <br/>
| Original |
| :---: |
| ![inazuma_(kancolle), blush, smile, blue archive(style), highres, classroom, xd s-4008249648](https://user-images.githubusercontent.com/20253131/196036227-386859a3-7a47-4224-ba46-a6e084a22fb6.png) |

| Original-Magnified | Waifu2x |
| :---: | :---: |
|  ![origin](https://user-images.githubusercontent.com/20253131/196035794-a5da8b46-7fc1-498a-a35b-12fdb92c60ae.PNG)  | ![waifu2x](https://user-images.githubusercontent.com/20253131/196035799-86897dbe-5535-4b87-a246-3720913644e4.PNG) | <br/>

| Real-CUGAN | Real-ESRGAN |
| :---: | :---: |
| ![cugan](https://user-images.githubusercontent.com/20253131/196035811-a5194833-f572-43ac-b03f-52b4626c3121.PNG) | ![esrgan](https://user-images.githubusercontent.com/20253131/196035813-31796abe-0bfe-4824-a406-60e01de5c8fd.PNG) |

 <br/>
You can use Snowshell to make your image better. <br/>
Snowshell uses C++ and Win32API. 

# Usage
Just drag & drop your image files (or folder) in to Snowshell! (multiple image supports) <br/>
To change your conversion setting, select option at menu bar. <br/>

## To Use Vulkan Based Converters (Supported GPU Only)
> 1. Install latest Graphics Driver. [AMD](https://www.amd.com/en/support) / [NVIDIA](https://www.nvidia.co.kr/Download/index.aspx) / [INTEL](https://www.intel.com/content/www/us/en/support/detect.html) <br/>
> 2. Choose Converter to download <br/>
> 2-1. Download [waifu2x-ncnn-vulkan](https://github.com/nihui/waifu2x-ncnn-vulkan/releases) to waifu2x-ncnn-vulkan folder. <br/>
> 2-2. Download [Real-CUGAN](https://github.com/nihui/realcugan-ncnn-vulkan) to realcugan-vulkan folder. <br/>
> 2-3. Download [Real-ESRGAN](https://github.com/xinntao/Real-ESRGAN-ncnn-vulkan) to realesrgan-vulkan folder. <br/>
> 3. Select Converter to waifu2x-ncnn-vulkan/realcugan-vulkan. <br/>
> 4. Set conversion mode on TTA menu. <br/>
> 4-1. TTA convert mode is slower but higher quality conversion. <br/>
> 5. Snowshell will start conversion using waifu2x-ncnn-vulkan / realcuan-vulkan.

### Notice For waifu2x-ncnn-vulkan:
> You cannot use upconv_7 models for x1 mode. please use cunet instead (default)

## To Use waifu2x-caffe (NVIDIA GPU Only)
> 1. Install latest [NVIDIA Graphics Driver](https://www.nvidia.co.kr/Download/index.aspx). <br/>
> 2. Put [waifu2x-caffe](https://github.com/lltcggie/waifu2x-caffe/releases) to waifu2x-caffe folder. <br/>
> 3. Select Converter to waifu2x-caffe. <br/>
> 4. Set conversion mode on GPU(CPU) menu. <br/>
> 4-1. TTA convert mode is slower but higher quality conversion. <br/>
> 5. Snowshell will start conversion using waifu2x-caffe.

### To Use cunet model with included waifu2x-caffe:
> 1. set model folder to models/cunet
> 2. set Custom Option to -p cudnn <br/>
> ![image](https://user-images.githubusercontent.com/20253131/71813700-aeede580-30bd-11ea-84a7-63d49a9fe6b6.png)

## To Change Model File
> 1. Open config.ini file (if there is no config.ini, launch Snowshell once and close it) <br/>
> 2. Set your model file directory with absolute path or relative path. <br/>
> 2-1 You have to use \\\\ instead of \ <br/>
> 2-2. When you use relative path, path is relative to converter's directory. <br/>
> 3. Converters default model directory (relative) <br/>
> 3-1. waifu2x_converter_cpp = models_rgb <br/>
> 3-2. waifu2x_caffe = models\\\\upconv_7_anime_style_art_rgb <br/>
> 3-3. waifu2x_ncnn-vulkan = models-cunet <br/>
> 3-4. realcugan-vulkan = model-se <br/>
> 3-5. realesrgan-vulkan = realesrgan-x4plus-anime <br/>

## To Change Output Extension
> 1. Open config.ini file (if there is no config.ini, launch Snowshell once and close it) <br/>
> 2. Change extension to prefer file format. <br/>
> 2-1. Check converter's HP to find which file format will be supported. <br/>

## To Use Advanced Option in Converter
> 1. Open config.ini file (if there is no config.ini, launch Snowshell and close it) <br/>
> 2. Check your converter and fill OptionString with currect option. <br/>
> 3. To find option for your converter, please visit converter's HP <br/>
> 3-1. waifu2x_converter_cpp: [waifu2x-converter-cpp](https://github.com/DeadSix27/waifu2x-converter-cpp) <br/>
> 3-2. waifu2x_caffe: [waifu2x-caffe](https://github.com/lltcggie/waifu2x-caffe) <br/>
> 3-3. waifu2x_ncnn-vulkan: [waifu2x-ncnn-vulkan](https://github.com/nihui/waifu2x-ncnn-vulkan) <br/>
> 3-4. realcugan-vulkan: [Real=CUGAN](https://github.com/nihui/realcugan-ncnn-vulkan) <br/>
> 3-5. realesrgan-vulkan: [Real-ESRGAN](https://github.com/xinntao/Real-ESRGAN-ncnn-vulkan) <br/>
> 4. If you want to ignore/override settings that Snowshell supports, add -- to end of optionstring to ignore all settings.<br/>
> 4-1. you cannot override input file name because it is nessesory to make Snowshell works.
