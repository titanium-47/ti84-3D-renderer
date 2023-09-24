#include <graphx.h>
#include <tice.h>
#include <sys/timers.h>
#include <stdlib.h>
#include <keypadc.h>
#include <string.h>
#include <math.h>

#define BUFFER_1 0xD40000
#define BUFFER_2 0xD52C00
#define WIDTH 320
#define HEIGHT 240

uint8_t* VRAM = (uint8_t*)BUFFER_1;
uint8_t* buffer2 = (uint8_t*)BUFFER_2;

struct cube {
    float x;
    float y;
    float z;
    float size;
    float xrot;
    float yrot;
    float zrot;
};

struct point2d {
    int x;
    int y;
};

struct point3d {
    float x;
    float y;
    float z;
};

void clearBuffer() {
    memset(buffer2, 0xff, WIDTH*HEIGHT);
}

void render() {
    memcpy(VRAM, buffer2, WIDTH*HEIGHT);
}

float inverse_rsqrt( float number ) {
    const float threehalfs = 1.5F;

    float x2 = number * 0.5F;
    float y = number;

    long i = * ( long * ) &y;

    i = 0x5f3759df - ( i >> 1 );
    y = * ( float * ) &i;

    y = y * ( threehalfs - ( x2 * y * y ) );

    return y;
}

void drawRect(uint8_t color, int x, int y, int width, int height) {
    for (int i = x; i<x+width; i++) {
        for (int j = y; j<y+height; j++) {
            if(i+j*WIDTH < WIDTH*HEIGHT && i+j*WIDTH >= 0){
                buffer2[i+j*WIDTH] = color;
            }
        }
    }
}

float fabsf(float x) {
    if (x < 0) {
        return -x;
    }
    return x;
}


void drawLine(uint8_t color, struct point2d point1, struct point2d point2) {
    int xdiff = point2.x - point1.x;
    int ydiff = point2.y - point1.y;
    int steps = 0;
    
    if(xdiff == 0 && ydiff == 0) {
        return;
    }


    if (abs(xdiff) > abs(ydiff)) {
        steps = abs(xdiff);
    } else {
        steps = abs(ydiff);
    }
    for (int i = 0; i<steps; i++) {
        int x = point1.x + (xdiff * i) / steps;
        int y = point1.y + (ydiff * i) / steps;
        if (x<WIDTH && x>=0 && y<HEIGHT && y>=0){
            buffer2[x+y*WIDTH] = color;
        }
    }
    
}

void drawPolygon(uint8_t color, struct point2d* points, int numPoints) {
    if (numPoints < 3) {
        return;
    }
    for (int i = 0; i < numPoints; i++) {
        drawLine(color, points[i], points[(i+1)%numPoints]);
    }
}

struct point2d projectPoint(struct point3d point) {
    float fov = 256;
    float z = point.z;
    float x = point.x - WIDTH/2;
    float y = point.y - HEIGHT/2;
    float x2 = x * fov / z;
    float y2 = y * fov / z;
    return (struct point2d){(int)x2 + WIDTH/2, (int)y2 + HEIGHT/2};
}

void cubeToPoints(struct cube cube, struct point3d points[8]) {
    float halfSize = cube.size/2;
    
    points[0] = (struct point3d){0 - halfSize, 0 - halfSize, halfSize};
    points[1] = (struct point3d){0 - halfSize, 0 - halfSize, 0 - halfSize};
    points[2] = (struct point3d){halfSize, 0 - halfSize, 0 - halfSize};
    points[3] = (struct point3d){halfSize, 0 - halfSize, halfSize};
    points[4] = (struct point3d){0 - halfSize, halfSize, halfSize};
    points[5] = (struct point3d){0 - halfSize, halfSize, 0 - halfSize};
    points[6] = (struct point3d){halfSize, halfSize, 0 - halfSize};
    points[7] = (struct point3d){halfSize, halfSize, halfSize};
}

void translate3D(float x, float y, float z, struct point3d nodes[8]) {
    for (int n=0; n<8; n++) {
        struct point3d* node = &nodes[n];
        node->x += x;
        node->y += y;
        node->z += z;
    }
}

void rotate3D(float xrot, float yrot, float zrot, struct point3d nodes[8]) {
    float cxrot = cosf(xrot);
    float sxrot = sinf(xrot);
    float cyrot = cosf(yrot);
    float syrot = sinf(yrot);
    float czrot = cosf(zrot);
    float szrot = sinf(zrot);
    
    for (int n=0; n<8; n++) {
        struct point3d* node = &nodes[n];
        node->x = node->x * czrot - node->y * szrot + node->x * cyrot + node->z * syrot;
        node->y = node->y * czrot + node->x * szrot + node->y * cxrot - node->z * sxrot;
        node->z = node->z * cyrot - node->x * syrot + node->z * cxrot + node->y * sxrot;
    }
}

void drawCube(uint8_t color, struct cube cube) {
    struct point3d points[8];
    cubeToPoints(cube, points);
    rotate3D(cube.xrot, cube.yrot, cube.zrot, points);
    translate3D(cube.x, cube.y, cube.z, points);

    struct point2d projectedPoints[8];
    for (int i = 0; i < 8; i++) {
        projectedPoints[i] = projectPoint(points[i]);
    }
    drawPolygon(color, (struct point2d[]){projectedPoints[0], projectedPoints[1], projectedPoints[2], projectedPoints[3]}, 4);
    drawPolygon(0xE0, (struct point2d[]){projectedPoints[4], projectedPoints[5], projectedPoints[6], projectedPoints[7]}, 4);
    drawLine(0x07, projectedPoints[0], projectedPoints[4]);
    drawLine(0x07, projectedPoints[1], projectedPoints[5]);
    drawLine(0x07, projectedPoints[2], projectedPoints[6]);
    drawLine(0x07, projectedPoints[3], projectedPoints[7]);
}
 

int main(void) {
    gfx_Begin();
    struct cube cube = (struct cube){WIDTH/2, HEIGHT/2, 200, 50, 0, 0, 0};
    drawCube(0x00, cube);
    render();
    clearBuffer();
    do {
        for (int i = 0; i<8; i++) {
            if (kb_Data[i]>0) {
                drawCube(0x00, cube);
                render();
                clearBuffer();
            }
        }
        
        kb_Scan();
        if (kb_Data[7] & kb_Left) {
            cube.yrot -= 0.05;
        }
        if (kb_Data[7] & kb_Right) {
            cube.yrot += 0.05;
        }
        if (kb_Data[7] & kb_Up) {
            cube.xrot -= 0.05;
        }
        if (kb_Data[7] & kb_Down) {
            cube.xrot += 0.05;
        }
        if (kb_Data[1] & kb_Del) {
            cube.zrot -= 0.05;
        }
        if (kb_Data[4] & kb_Stat) {
            cube.zrot += 0.05;
        }
    } while (!(kb_Data[6] & kb_Enter));
    gfx_End();
}