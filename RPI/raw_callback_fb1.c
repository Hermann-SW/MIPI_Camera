/* gcc -O3 -Wno-variadic-macros -Wall -Wextra -pedantic raw_callback_fb1.c -o raw_callback_fb1  -larducam_mipicamera
*/
#include "arducam_mipicamera.h"
#include <linux/v4l2-controls.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <stdint.h>
#include <assert.h>
#include <sys/types.h> 

#define LOG(fmt, args...) fprintf(stderr, fmt "\n", ##args)

uint16_t buf2[320*240], map[256];

FILE *fd;
int frame_count = 0;
int thr;
int raw_callback(BUFFER *buffer) {
    // This buffer will be automatically released after the callback function ends.
    // If you do a time-consuming operation here, it will affect other parts,
    // such as preview, video encoding(This issue may be fixed in the future).
    if (buffer->length) {
        uint8_t *buf,*p, m=0, cmp;
        uint16_t *q;
        int width=320, height=240;
        double sc;

        buf = buffer->data;

        if (thr >= 0)
        {
            for(p=buf; p<buf+width*height; ++p)
                if (*p > m)
                    m = *p;
      
            sc = 254.0 / m;

            if (thr > 0)
            { 
                cmp = thr / sc;

                for(p=buf, q=buf2; p<buf+width*height; )
                    *q++ = *p++ > cmp ? 0xffff : 0;
            }
            else
                for(p=buf, q=buf2; p<buf+width*height; )
                    *q++ = map[(int)(sc * *p++)];
        }
        else
            for(p=buf, q=buf2; p<buf+width*height; )
                *q++ = map[*p++];

        if (TIME_UNKNOWN == buffer->pts) {
            LOG("TIME_UNKNOWN%s", "");
            // Frame data in the second half
        }

        if (frame_count==0) {
           fd=fopen("/dev/fb1","w");
        }
        frame_count++;

        rewind(fd);
        fwrite(buf2, 320*240*2, 1, fd);
        fflush(fd);
    }
    return 0;
}

int main(int argc, char **argv) {
    CAMERA_INSTANCE camera_instance;
    int mode = 5;
    int exp = 20, res;

    assert(argc <= 2 || !"braking_eol [thr]");

    thr= (argc>1) ? atoi(argv[1]) : -1;

    assert(arducam_init_camera(&camera_instance) == 0);

    assert(arducam_set_mode(camera_instance, mode) == 0);
    assert(arducam_set_control(camera_instance, V4L2_CID_EXPOSURE, exp) == 0);

    for(int i=0; i<256; ++i)
    {
        map[i] = (i >> 3) << 11 | (i >> 2) << 5 | (i >> 3);
    }

    res = arducam_set_raw_callback(camera_instance, raw_callback, NULL);
    if (res) {
        LOG("Failed to start raw data callback%s", "");
        return -1;
    }

    for(;;)  usleep(10000);

    arducam_set_raw_callback(camera_instance, NULL, NULL);
    fclose(fd);

    res = arducam_close_camera(camera_instance);
    if (res) {
        LOG("close camera status = %d", res);
    }

    return 0;
}
