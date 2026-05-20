#include "pico8sim.h"
#include "string.h"
#include "stdio.h"
extern const unsigned char sprite[];
extern const unsigned char tilemap[];

const uint16_t font_data[54] =
{
    0x7BED,0x7BAF,0x3923,0x6B6F,0x79A7,0x79A4,0x392F,0x5BED,
    0x7497,0x7496,0x5BAD,0x4927,0x7F6D,0x6B6D,0x3B6E,0x7BE4,
    0x2B73,0x7BAD,0x39CE,0x7492,0x5B6B,0x5B7A,0x5B7F,0x5AAD,
    0x5BCF,0x72A7,0x7B6F,0x6497,0x73E7,0x72CF,0x5BC9,0x79CF,
    0x49EF,0x7249,0x7BEF,0x7BC9,0x03E0,0x2482,0x2B63,0x5F7D,
    0x7CFA,0x52A5,0x2A00,0x6DAF,0x55D5,0x2922,0x224A,0x0007,
    0x05D0,0x01C0,0x0E38,0x72C2,0x0410,0x0002
};
const char fontmap[] = "abcdefghijklmnopqrstuvwxyz0123456789~!@#$%^&*()_+-=?:.";


static const uint8_t mask[] = {
    0, 0, 0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 2, 0,  0,  0,  0,  0, 0, 0, 0, 0, 2, 0, 0, 0, 0,
    3, 3, 3,  3,  3,  3,  3, 3, 4, 4, 4, 2, 2, 0, 0, 0,
    3, 3, 3,  3,  3,  3,  3, 3, 4, 4, 4, 2, 2, 2, 2, 2,
    0, 0, 19, 19, 19, 19, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2,
    0, 0, 19, 19, 19, 19, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2,
    0, 0, 19, 19, 19, 19, 0, 4, 4, 2, 2, 2, 2, 2, 2, 2,
    0, 0, 19, 19, 19, 19, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2
};

static const uint16_t col_list[16] = {

    0x0000,0x4A19,0x2A79,0x2A04,
    0x86AA,0xA95A,0x18C6,0x9DFF,
    0x09F8,0x00FD,0x64FF,0x2607,
    0x7F2D,0xB383,0xB5FB,0x75FE,
};

uint8_t pal_list[16]=
{
    0,1,2,3,
    4,5,6,7,
    8,9,10,11,
    12,13,14,15
};
int16_t camera_x=0;
int16_t camera_y=0;

uint8_t p8framebuf[128*128]={0};
void p8draw_pixel(int16_t x,int16_t y,uint8_t col)
{
    if((uint16_t)x>=128 ||(uint16_t)y>=128)return;
    p8framebuf[y*128+x]=pal_list[col%16];
}
void p8draw_hline(int16_t x,int16_t y,int16_t w,uint8_t col)
{
    if((uint16_t)y>=128 ||x>=128) return;
    if(x<0)
    {
        w+=x;
        x=0;
    }
    if(x+w>128)w=128-x;
    for(int i=0;i<w;i++)
    {
        p8framebuf[y*128+x+i]=pal_list[col%16];
    }
}
void p8blit_to_rgb565(uint16_t *buf)
{
    int dx=0,dy=0,sx=0,sy=0;
    if(camera_x || camera_y)
    {
        memset(buf,0,128*128*2);
        if(camera_x>0)dx=camera_x;
        else sx=-camera_x;
        if(camera_y>0)dy=camera_y;
        else sy=-camera_y;
    }

    while(sy<128 && dy<128)
    {
        uint8_t* src=&p8framebuf[sy*128];
        uint16_t* dest=&buf[dy*128];
        int sx1=sx,dx1=dx;
        while(sx1<128 && dx1<128)
        {
            dest[dx1++]=col_list[src[sx1++]];
        }
        sy++;dy++;
    }
}

uint8_t fget(uint8_t n, uint8_t f)
{
    return n<sizeof(mask) && (mask[n]&(1<<f));
}
void pal(uint8_t c0, uint8_t c1)
{
    pal_list[c0]=c1;
}
void camera(fix16_t x, fix16_t y)
{
    camera_x=fix16_to_int(x);
    camera_y=fix16_to_int(y);
}
void rectfill(fix16_t x0, fix16_t y0, fix16_t x1, fix16_t y1, uint8_t col)
{
    int16_t left=fix16_to_int(fix16_min(x0,x1));
    int16_t top=fix16_to_int(fix16_min(y0,y1));
    int16_t bottom=fix16_to_int(fix16_max(y0,y1));
    int16_t width=fix16_to_int(fix16_max(x0,x1))-left+1;
    for(int16_t i=top;i<=bottom;i++)
    {
        p8draw_hline(left,i,width,col);
    }
}
void circfill(fix16_t x, fix16_t y, fix16_t r, uint8_t col)
{
    int16_t ix=fix16_to_int(x);
    int16_t iy=fix16_to_int(y);
    if (r<=F16(1))
    {
        p8draw_pixel(ix,iy-1,col);
        p8draw_hline(ix-1,iy,3,col);
        p8draw_pixel(ix,iy+1,col);      
    }
    else if (r<=F16(2))
    {
        p8draw_hline(ix-1,iy-2,3,col);
        for (int16_t row=iy-1;row<=iy+1;row++)
        {
            p8draw_hline(ix-2,row,5,col);
        }
        p8draw_hline(ix-1,iy+2,3,col);
    }
    else if (r<=F16(3))
    {
        p8draw_hline(ix-1,iy-3,3,col);
        p8draw_hline(ix-2,iy-2,5,col);
        for (int16_t row=iy-1;row<=iy+1;row++)
        {
            p8draw_hline(ix-3,row,7,col);
        }
        p8draw_hline(ix-2,iy+2,5,col);
        p8draw_hline(ix-1,iy+3,3,col);
    }
}
void spr(uint8_t n, fix16_t x0, fix16_t y0, uint8_t flip_x, uint8_t flip_y)
{
    int16_t ix=fix16_to_int(x0);
    int16_t iy=fix16_to_int(y0);
    uint16_t base = n * 64;

    for(int16_t dy=0;dy<8;dy++)
    {
        int16_t y=iy+dy;
        if(y<0 || y>=128) continue;
        int16_t src_y = flip_y?(7-dy):dy;
        for(int16_t dx=0;dx<8;dx++)
        {
            int16_t x=ix+dx;
            if(x<0 || x>=128) continue;
            int16_t src_x=flip_x?(7-dx):dx;
            uint8_t col=sprite[base+src_y*8+src_x];
            if(col==0) continue;
            
            p8framebuf[y*128+x]=pal_list[col%16];
        }
    }
}
uint8_t mget(int16_t x, int16_t y)
{
    return tilemap[x+y*128];
}
void map(int16_t tile_x, int16_t tile_y, int16_t sx, int16_t sy, int16_t tile_w, int16_t tile_h, uint8_t layers)
{
    for (int16_t x=0;x<tile_w;x++)
    {
        for (int16_t y=0;y<tile_h;y++)
        {
            uint8_t tile=tilemap[x+tile_x+(y+tile_y)*128];
            if(tile<128)
            {

                if(layers==0||fget(tile,layers))
                {
                    for(int16_t dy=0;dy<8;dy++)
                    {
                        int16_t y2=y*8+sy+dy;
                        if(y2<0||y2>=128)continue;
                        for(int16_t dx=0;dx<8;dx++)
                        {
                            int16_t x2=x*8+sx+dx;
                            if(x<0||x>=128)continue;
                            uint8_t col=sprite[tile*64+dy*8+dx];
                            if(col==0) continue;
                            p8framebuf[y2*128+x2]=pal_list[col%16];
                        }
                    }
                }
            }
        }
    }
}

void print(const char* fmt, fix16_t x, fix16_t y, uint8_t col, ...)
{
    int16_t left=fix16_to_int(x);
    int16_t top=fix16_to_int(y);
    uint8_t color=col%16;
    char buf[40];
    
    va_list args;
    va_start(args, col);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    int len=strlen(buf);
    for(int i=0;i<len;i++)
    {
        char ch=buf[i];
        int index=-1;
        for(int j=0;j<sizeof(fontmap)-1;j++)
        {
            if(fontmap[j]==ch)
            {
                index=j;
                break;
            }
        }
        if(index>=0)
        {
            for(int k=0;k<15;k++)
            {
                if(font_data[index]&(0x4000U>>k))
                {
                    int16_t sy=top+k/3;
                    if((uint16_t)sy>=128)continue;
                    int16_t sx=k%3+left;
                    if((uint16_t)sx>=128)continue;
                    p8framebuf[sy*128+sx]=color;
                }
            }
        }
        left+=4;
    }
}