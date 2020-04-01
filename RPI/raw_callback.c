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
            sprintf(buf, "/dev/shm/frame.%04d.raw10", frame_count);
            FILE *tgt = fopen(buf, "wb");
            fwrite(buffer->data, buffer->length, 1, tgt);
            fclose(tgt);
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    CAMERA_INSTANCE camera_instance;
    int count = 0;
    int width = 0, height = 0;
    int exp = (argc>1) ? atoi(argv[1]) : 500;
    if (argc > 2)
        save = atoi(argv[2]);
    
    LOG("Open camera...");
    int res = arducam_init_camera(&camera_instance);
    if (res) {
        LOG("init camera status = %d", res);
        return -1;
    }

    width = 1920;
    height = 1080;
    LOG("Setting the resolution...");
    res = arducam_set_resolution(camera_instance, &width, &height);
    if (res) {
        LOG("set resolution status = %d", res);
        return -1;
    } else {
        LOG("Current resolution is %dx%d", width, height);
        LOG("Notice:You can use the list_format sample program to see the resolution and control supported by the camera.");
    }
    if (arducam_set_control(camera_instance, V4L2_CID_EXPOSURE, exp)) {
        LOG("Failed to set exposure, the camera may not support this control.");
    }

    /* https://cdn.datasheetspdf.com/pdf-down/O/V/7/OV7750-OmniVision.pdf#page=84 */
    uint16_t reg_val = 0;
    if (arducam_read_sensor_reg(camera_instance, 0x3662, &reg_val)) {
        LOG("Failed to read sensor register.");
    } else {
        LOG("Read 0x3662 value = 0x%02X", reg_val);
    }
/*
    LOG("Set RAW8 mode ...");
    if (arducam_write_sensor_reg(camera_instance, 0x3662, reg_val|(1<<1))) {
        LOG("Failed to write sensor register.");
    }
    if (arducam_read_sensor_reg(camera_instance, 0x3662, &reg_val)) {
        LOG("Failed to read sensor register.");
    } else {
        LOG("Read 0x3662 value = 0x%02X", reg_val);
    }
*/
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
