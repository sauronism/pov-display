#include <constants.h>

class VideoFrame
{
public:
    uint32_t frameImgae[STRIP_NUM_LEDS * IMAGE_WIDTH]; 
    uint32_t currentRow[STRIP_NUM_LEDS];
};