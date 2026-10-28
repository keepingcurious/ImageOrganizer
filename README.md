# Image Organizer

This program helps to sort images by date and identify duplicates.

## Description

Starting situation:  
Photos were taken with DSLRs or smartphones, or transferred via USB drives.  
Countless folders with varying structures and naming conventions existed.  
Some images were offloaded to external hard drives, resulting in overall chaos.

Goal:  
All images should be stored centrally, chronologically, and with a unified folder structure.

Implementation:  
Bundle all folders containing images in one place to concentrate the chaos, then let the program create order.  


How it works:  
The program reads the EXIF data of an image, which is generally more accurate than the file modification date.  
It then creates the following folder structure:


```text
2026
| |______ 01
| |______ 02
|  ...
|  ...
| |______ 12
2025
| |______ 01
| |______ 02
|  ...
|  ...
| |______ 12
|
|
... 
  ```

Folders are only created if an image with the respective date exists.

###  Special Feature
The program uses the MD-5 algorithm to identify duplicates.
If an image has already been processed, it will not be sorted again.




## Getting Started

### Dependencies

* This program is optimized for Apple Silicon.
* The EXIV2 library is used.

### Installing

* Code lines for hardware acceleration should be commented out if necessary for other operating systems.

### Executing program

* The program prompts for a source folder containing the images to be sorted.
* The program prompts for a destination folder where the sorted images will be saved.


## Authors

Contributors names and contact info

keepingcurious  
With support from Google Gemini

## Version History

* 0.1
    * Initial Release

## License

This project is licensed under the [**GNU General Public License v3.0 (GPL-3.0)**] License.  
Weitere Details findest du in der Datei [LICENSE](LICENSE).

