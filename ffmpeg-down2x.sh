#!/bin/sh -v
#
# ffmpeg-down2x [input] [output] - simple script which takes a video file as input and
# uses ffmpeg to reduce it to half the vertical and horizontal
# size (usually producing a file about 30% of the size of original).
# The original file is not modified or deleted.
#

INPUT_FILE=$1
OUTPUT_FILE=$2

if [ ! -f ${INPUT_FILE} ]
then
   echo Input ${INPUT_FILE} does not exist!
   exit 2
fi

if [ -e ${OUTPUT_FILE} ]
then
   echo Output ${OUTPUT_FILE} already exists!
   exit 2
fi

ffmpeg -i ${INPUT_FILE} -vf "scale=w=iw/2:h=ih/2" ${OUTPUT_FILE}
