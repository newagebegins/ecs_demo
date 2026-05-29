#if !defined(RENDER_GROUP_H)
#define RENDER_GROUP_H

enum color
{
    Color_Black,
    Color_Blue,
    Color_Red,
    Color_Magenta,
    Color_Green,
    Color_Cyan,
    Color_Yellow,
    Color_White,

    Color_BrightBlue,
    Color_BrightRed,
    Color_BrightMagenta,
    Color_BrightGreen,
    Color_BrightCyan,
    Color_BrightYellow,

    Color_Count,
};

struct render_group
{
    memory_arena *Arena;
    v4 Palette[Color_Count];
    bitmap_info *BitmapInfos;
    u32 BitmapCount;
};

#endif
