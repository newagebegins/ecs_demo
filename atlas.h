#if !defined(ATLAS_H)
#define ATLAS_H

#pragma pack(push, 1)

enum bitmap_id
{
    Bitmap_None,
    Bitmap_Font,
    Bitmap_Guy,
    Bitmap_Bomb,

    Bitmap_Count,
};

struct bitmap_info
{
    s32 FrameCount;
    s32 FrameWidth;
    s32 FrameHeight;
    s32 OffsetY;
};

struct atlas_header
{
    u32 Width;
    u32 Height;
    u32 InfosSize;
    u32 PixelsSize;
    u32 InfosOffset;
    u32 PixelsOffset;
};

#pragma pack(pop)

#define ATLAS_WIDTH 256
#define ATLAS_HEIGHT ATLAS_WIDTH
#define ATLAS_PITCH (ATLAS_WIDTH*BITMAP_BYTES_PER_PIXEL)

#define ATLAS_INFOS_SIZE (sizeof(bitmap_info)*Bitmap_Count)
#define ATLAS_PIXELS_SIZE (ATLAS_HEIGHT*ATLAS_PITCH)

struct atlas
{
    bitmap_info *Infos;
    void *Pixels;
};

#endif
