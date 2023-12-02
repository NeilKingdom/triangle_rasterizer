#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct {
   float x;
   float y;
} point_t;

typedef struct {
   uint8_t r;
   uint8_t g;
   uint8_t b;
} pixel_t;

static pixel_t fg_color;
static pixel_t bg_color;

#define SET_FG_COLOR(_r, _g, _b) (fg_color.r) = (_r); \
                                 (fg_color.g) = (_g); \
                                 (fg_color.b) = (_b)

#define SET_BG_COLOR(_r, _g, _b) (bg_color.r) = (_r); \
                                 (bg_color.g) = (_g); \
                                 (bg_color.b) = (_b)

#define BUF_SZ    256
#define FILE_NAME "triangle.ppm"
#define FLINE_SZ  ((sizeof(pixel_t) * 3) + (sizeof(char) * 3))

static void swap_points(point_t *a, point_t *b) {
   point_t c = *a;
   *a = *b;
   *b = c;
}

static void map_point(point_t *p) {
   p->y = BUF_SZ - 1 - (p->y);
}

static char *itoa(int num) {
   uint8_t digits; 
   char *str = NULL;

   if (num == 0) digits = 1;
   else digits = log10(num) + 1;

   str = malloc(sizeof(char) * (digits + 1));

   for (int i = 1; i < (digits + 1); ++i) {
      str[digits - i] = (num % 10) + '0';
      num /= 10;
   }

   str[digits] = '\0';

   return str;
}

static char *pixel_to_str(pixel_t pix) {
   char *str = malloc(FLINE_SZ);
   memset(str, ' ', FLINE_SZ); 

   char *tmp = itoa(pix.r);
   strncpy(str, tmp, strlen(tmp));
   free(tmp);
   tmp = itoa(pix.g);
   strncpy(str + 4, tmp, strlen(tmp));
   free(tmp);
   tmp = itoa(pix.b);
   strncpy(str + 8, tmp, strlen(tmp));
   free(tmp);

   str[FLINE_SZ - 1] = '\n';

   return str;
}

static void fput_pixel(FILE *fp, char *hdr, int y_pos, int x_start, int x_end, char *pix_as_str) {
   int x;

   if (x_start > x_end) {
      float tmp = x_start;
      x_start = x_end;
      x_end = tmp;
   }

   fseek(fp, strlen(hdr) + (y_pos * BUF_SZ * FLINE_SZ) + (x_start * FLINE_SZ), SEEK_SET);
   for (x = x_start; x < x_end && x < BUF_SZ; ++x) {
      fwrite((void*)pix_as_str, strlen(pix_as_str), 1, fp);
   }
}

int main(void) {
   FILE *fp = NULL;
   char *fg_str = NULL;
   char *bg_str = NULL;
   pixel_t buf[BUF_SZ][BUF_SZ];
   int y;

   point_t A = { .x = 188.7f, .y = 132.2f };
   point_t B = { .x = 101.9f, .y = 240.3f };
   point_t C = { .x =  58.5f, .y = 38.9f };
   point_t D = { 0 };

   map_point(&A);
   map_point(&B);
   map_point(&C);

   /*** Sort by y coordinate (ascending from a-c) ***/

   if (A.y > B.y) swap_points(&A, &B);
   if (B.y > C.y) swap_points(&B, &C);
   if (A.y > B.y) swap_points(&A, &B);

   memset(buf, 0, sizeof(buf));

   /*** Create PPM file ***/

   fp = fopen(FILE_NAME, "w+");
   if (fp == NULL) {
      perror("Failed to open raster.ppm for write");
      exit(EXIT_FAILURE);
   }

   /*** Write PPM header ***/

   char hdr[256];
   sprintf(hdr, "P3\n%d %d\n255\n", BUF_SZ, BUF_SZ);
   fwrite((void*)hdr, strlen(hdr), 1, fp);

   /*** Fill backround ***/

   SET_BG_COLOR(0.0f, 0.0f, 0.0f);
   bg_str = pixel_to_str(bg_color);
   for (int i = 0; i < sizeof(buf); ++i) {
      fwrite((void*)bg_str, strlen(bg_str), 1, fp);
   }

   /*
    * The ordering of points is such that the triangle will always
    * be laid out with point A at the top, B in the middle, and C
    * at the bottom. There are 3 possible configuration of triangle 
    * here:
    *
    * Configuration 1 (Ay == By) - Only render bottom half:
    * 
    *    B +------- D
    *      |   _-'
    *      |.-'
    *    C
    *
    * Configuration 2 (Ay > By) - Render top and bottom half:
    *
    *         A             A
    *        /  \          /  \
    *       /---- B =>  C +---- B  +  B +---- D
    *      /  _-'                      /  _-'
    *     /.-'                        /.-'
    *   C                           C
    *
    * Configuration 3 (By == Cy) - Only reder top half:
    *
    *          A
    *        /   \
    *       /     \
    *     B ------- C
    */

   /*** Render top half ***/

   if (A.y != B.y) {
      SET_FG_COLOR(0.0f, 0.0f, 255.0f);
      fg_str = pixel_to_str(fg_color);

      for (y = A.y; y < B.y && y < BUF_SZ; ++y) {
         float AB_dx = A.x - B.x;
         float AB_dy = A.y - B.y;
         float AB_m  = AB_dy / AB_dx;

         float AC_dx = A.x - C.x;
         float AC_dy = A.y - C.y;
         float AC_m  = AC_dy / AC_dx;

         /* 
            Solve for y-intercept (can use either point for y and x):
            y = mx + c
            y - c = mx
            -c = -y + mx
            c = y - mx
         */

         float AB_c = B.y - (AB_m * B.x);
         float AC_c = C.y - (AC_m * C.x);

         /*
            Solve for x:
            y = mx + c
            y - c = mx
            (y - c) / m = x
         */

         int start = (y - AB_c) / AB_m; 
         int end   = (y - AC_c) / AC_m;

         fput_pixel(fp, hdr, y, start, end, fg_str);
      } 
   }

   /*** Let point D be the right-hand mid point ***/

   if (A.y == B.y) {
      D = A;
      if (D.x < B.x) {
         swap_points(&D, &B);
      }
   } else {
      float AC_dx = A.x - C.x;
      float AC_dy = A.y - C.y;
      float AC_m  = AC_dy / AC_dx;

      /* Solve y-intercept */
      float AC_c = C.y - (AC_m * C.x);

      /* Solve for x */
      D.x = (B.y - AC_c) / AC_m; 
      D.y = B.y;
   }

   /*** Render bottom half ***/

   if (B.y != C.y) {
      SET_FG_COLOR(0.0f, 255.0f, 0.0f);
      if (fg_str) free(fg_str);
      fg_str = pixel_to_str(fg_color);

      for (y = B.y; y <= C.y && y < BUF_SZ; ++y) {
         float BC_dx = B.x - C.x;
         float BC_dy = B.y - C.y;
         float BC_m  = BC_dy / BC_dx;

         float DC_dx = D.x - C.x;
         float DC_dy = D.y - C.y;
         float DC_m  = DC_dy / DC_dx;

         /* Solve y-intercept */
         float BC_c = B.y - (BC_m * B.x);
         float DC_c = D.y - (DC_m * D.x);

         /* Solve for x */
         int start = (y - BC_c) / BC_m; 
         int end   = (y - DC_c) / DC_m;

         fput_pixel(fp, hdr, y, start, end, fg_str);
      }
   }

   free(bg_str);
   free(fg_str);
   fclose(fp);

   return EXIT_SUCCESS;
}
