#include "arducam_mipicamera.h"
#include <linux/v4l2-controls.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define LOG(fmt, args...) fprintf(stderr, fmt "\n", ##args)

FILE *fd;
int frame_count = 0;
int save = 0;
int raw_callback(BUFFER *buffer) {
    // This buffer will be automatically released after the callback function ends.
    // If you do a time-consuming operation here, it will affect other parts,
    // such as preview, video encoding(This issue may be fixed in the future).
    if (buffer->length) {
        if (TIME_UNKNOWN == buffer->pts) {
            LOG("TIME_UNKNOWN");
            // Frame data in the second half
        }
        // LOG("buffer length = %d, pts = %llu, flags = 0x%X", buffer->length, buffer->pts, buffer->flags);
        static int64_t ptsbase;
        if (frame_count==0) {
           fd=fopen("frame.pts","w");
           fprintf(fd,"# timecode format v2\n");
           ptsbase = buffer->pts;
        }
        fprintf(fd,"%lld.%03lld\n",(buffer->pts-ptsbase)/1000,(buffer->pts-ptsbase)%1000);
        frame_count++;

        if (save >= frame_count)
        {
            char buf[99];
            sprintf(buf, "/dev/shm/frame.%04d.raw", frame_count);
            FILE *tgt = fopen(buf, "wb");
            fwrite(buffer->data, buffer->length, 1, tgt);
            fclose(tgt);
        }
    }
    return 0;
}

void printCurrentMode(CAMERA_INSTANCE camera_instance){
    struct format currentFormat;
    char fourcc[5];
    fourcc[4] = '\0';
    arducam_get_format(camera_instance, &currentFormat);
    strncpy(fourcc, (char *)&currentFormat.pixelformat, 4);
     printf("%c[32;40m",0x1b);  
     printf("Current mode: %d, width: %d, height: %d, pixelformat: %s, desc: %s\r\n", 
            currentFormat.mode, currentFormat.width, currentFormat.height, fourcc, 
            currentFormat.description);
}

int main(int argc, char **argv) {
    CAMERA_INSTANCE camera_instance;
    int count = 0;
    int width = 0, height = 0, mode = 0;
    int exp = (argc>1) ? atoi(argv[1]) : 500;
    if (argc > 2)
        save = atoi(argv[2]);
    if (argc > 3)
        mode = atoi(argv[3]);
    
    LOG("Open camera...");
    int res = arducam_init_camera(&camera_instance);
    if (res) {
        LOG("init camera status = %d", res);
        return -1;
    }

    res = arducam_set_mode(camera_instance, mode);
    if (res) {
        LOG("set resolution status = %d", res);
        return -1;
    }
    if (arducam_set_control(camera_instance, V4L2_CID_EXPOSURE, exp)) {
        LOG("Failed to set exposure, the camera may not support this control.");
    }

    printCurrentMode(camera_instance);

    // start raw callback
    LOG("Start raw data callback...");
    res = arducam_set_raw_callback(camera_instance, raw_callback, NULL);
    if (res) {
        LOG("Failed to start raw data callback.");
        return -1;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);
    usleep(1000 * 1000 * 10);
    clock_gettime(CLOCK_REALTIME, &end);

    double timeElapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    LOG("Total frame count = %d", frame_count);
    LOG("TimeElapsed = %f", timeElapsed);
    // stop raw data callback
    LOG("Stop raw data callback...");
    arducam_set_raw_callback(camera_instance, NULL, NULL);
    fclose(fd);

    LOG("Close camera...");
    res = arducam_close_camera(camera_instance);
    if (res) {
        LOG("close camera status = %d", res);
    }
    return 0;
}
