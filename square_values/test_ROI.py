#!/usr/bin/env python

# for(int x = 0; x < 4; x++){
#   for(int y = 0; y < 4; y++){
#     //An ROI is a top left x, top left y, bottom left x, bottom left y
#     ROIConfigs[(x*4) + y] = {4*x, (15-4*y), (4*x+3), (15-4*y-3)};
#   }
# }

for y in range(4):
	for x in range(4):
		idx = (x*4)+y
		tl_x = (x*4)
		tl_y = 15-(y*4)
		br_x = (x*4) + 3
		br_y = (15-(y*4)) - 3
		print("{0:2}: tl X:{1:2} Y:{2:2} br X:{3:2} Y:{4}".format(idx, tl_x, tl_y, br_x, br_y))