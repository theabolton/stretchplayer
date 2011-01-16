#!/usr/bin/python
#
# Test program to generate data that simulates RubberBand's
# processing... for the purpose of modelling the synchronization of
# song position pointer sync. points.

INPUT_BLOCK = 512
OUTPUT_BLOCK = 128
RATIO = 2.0

input_song_pos = 0
output_song_pos = 0.0
output_frame = 0
buf_frames = 0

print "%5s %3s %3s %4s %5s %5s" % ("INPOS", "WRI", "REA", "BUF", "OUTPT", "OTPOS")
while input_song_pos < 5000:
    read = 0
    if buf_frames >= float(OUTPUT_BLOCK)/RATIO:
        read = OUTPUT_BLOCK
    written = 0
    if buf_frames <= INPUT_BLOCK:
        written = INPUT_BLOCK
    print "%5d %3d %3d %4d %5d %5d" % (input_song_pos, written, read, buf_frames, output_frame, output_song_pos)
    buf_frames = buf_frames - float(read)/RATIO + written
    input_song_pos += written
    if read > 0:
        output_frame += read
        output_song_pos += float(OUTPUT_BLOCK)/RATIO
