# ImageLOL
simple program to store a file into a PNG image.

# USAGE
To extract a file from the image, simply specify the file.

To embed a file into an image, specify the image to store the file into, and the file, make sure that the file is in the current directory.
You can also add -bn where n is in the range 1-8 to use the n lowest bits of each byte to store the file, uses 3 by default.
The image with the embedded file will be the original file with .png appended.

# BUILDING
Use cmake :) 
