/* gcc -O3 -Wno-variadic-macros -Wall -Wextra -pedantic braking_eol.c -o braking_eol  -larducam_mipicamera -lpthread -lpigpio
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

#include <pigpio.h>

#define LOG(fmt, args...) fprintf(stderr, fmt "\n", ##args)

#define gpioLeftFwd 25
#define gpioLeftRew  9
#define gpioLeftPwm 24
#define gpioRghtFwd 22
#define gpioRghtRew 23
#define gpioRghtPwm 27

#define gpioBulb 4

#define size 8
#define min 20

int line(unsigned char *p, int width)
{
  unsigned char *q, *r;
  for(q=p+width-1; q>=p; --q)
  {
    if (*q)  continue;

    for(r=q; q>=p && !*q; --q)  {}

    if (r-q>=min)
      return 1;
  }

  return 0;
}

void motor_stop(int r)
{
  LOG("motor_stop (%d)\n", r);
  gpioWrite(gpioLeftFwd, 1);
  gpioWrite(gpioLeftRew, 1);
  gpioPWM(gpioLeftPwm, (r<3) ? 0 : 255);
  gpioWrite(gpioRghtFwd, 1);
  gpioWrite(gpioRghtRew, 1);
  gpioPWM(gpioRghtPwm, (r<3) ? 0 : 255);
}

FILE *fd;
int frame_count = 0;
int save = 1600;
int thr;
int raw_callback(BUFFER *buffer) {
    // This buffer will be automatically released after the callback function ends.
    // If you do a time-consuming operation here, it will affect other parts,
    // such as preview, video encoding(This issue may be fixed in the future).
    if (buffer->length) {
        uint8_t *buf,*p,*q, *a, *b, *c, m=0;
        int width=320, height=240, stop=0;
        double sc;

  buf = buffer->data;

  for(p=buf; p<buf+width*height; ++p)
    if (*p > m)
      m = *p;

  sc = 254.0 / m;
/*
  for(p=buf; p<buf+width*(height-3); ++p)
   if ((p-buf)%width<width/2)
    *p = sc * *p;
   else
    *p = (sc * *p) > thr ? 255 : 0;
*/
  for(p=buf+width*(height-3); p<buf+width*height; ++p)
    *p = (sc * *p) > thr ? 255 : 0;


  a=buf+width*(height-1);
  b=a-width;
  c=b-width;

  stop = line(a, width) + line(b, width) + line(c, width) >= 2 ? 0x00 : 0xff;

  if (stop)
  {
    motor_stop(3); 
  }

  for(q=buf; q<buf+size; ++q)
    *q=(stop ^0xff);
  for(p=buf+width; p<buf+size*width; p+=width)
  {
    *p=(stop ^0xff);
    for(q=p+1; q<p+size-1; ++q)
      *q=stop;
    *q=(stop ^0xff);
  }
  for(q=p; q<p+size; ++q)
    *q=(stop ^0xff);


        if (TIME_UNKNOWN == buffer->pts) {
            LOG("TIME_UNKNOWN%s", "");
            // Frame data in the second half
        }
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

int main(int argc, char **argv) {
    CAMERA_INSTANCE camera_instance;
    int mode = 5;
    int exp = 20, res;

    assert(argc == 4 || !"braking_eol lft rgt thr");

    int lft = atoi(argv[1]);
    int rgt = atoi(argv[2]);
    thr= atoi(argv[3]);

    assert(arducam_init_camera(&camera_instance) == 0);

    assert(arducam_set_mode(camera_instance, mode) == 0);
    assert(arducam_set_control(camera_instance, V4L2_CID_EXPOSURE, exp) == 0);

    assert(gpioInitialise()>=0);

    gpioSetPWMfrequency(4, 100000);
    gpioPWM(gpioBulb, 255);
    usleep(200000);
    gpioPWM(gpioBulb, 0);
    usleep(200000);
    gpioPWM(gpioBulb, 255);

    usleep(1000 * 1000);

    gpioSetMode(gpioLeftFwd, PI_OUTPUT);
    gpioSetMode(gpioLeftRew, PI_OUTPUT);
    gpioSetMode(gpioRghtFwd, PI_OUTPUT);
    gpioSetMode(gpioRghtRew, PI_OUTPUT);

    gpioWrite(gpioLeftFwd, 0);
    gpioWrite(gpioLeftRew, 1);
    gpioPWM(gpioLeftPwm, lft);
    gpioWrite(gpioRghtFwd, 0);
    gpioWrite(gpioRghtRew, 1);
    gpioPWM(gpioRghtPwm, rgt);

    res = arducam_set_raw_callback(camera_instance, raw_callback, NULL);
    if (res) {
        LOG("Failed to start raw data callback%s", "");
        motor_stop(1); 
        gpioPWM(gpioBulb, 0);
        gpioTerminate();
        return -1;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

    usleep(1000 * 1000 * 5/2);

    clock_gettime(CLOCK_REALTIME, &end);

    double timeElapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    LOG("Total frame count = %d", frame_count);
    LOG("TimeElapsed = %f", timeElapsed);
    arducam_set_raw_callback(camera_instance, NULL, NULL);
    usleep(10000);
    motor_stop(2);
    fclose(fd);

    res = arducam_close_camera(camera_instance);
    if (res) {
        LOG("close camera status = %d", res);
    }

    gpioPWM(gpioBulb, 0);
    gpioTerminate();

    return 0;
}
