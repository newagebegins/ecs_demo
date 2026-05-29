#if !defined(RENDER_LIST_H)
#define RENDER_LIST_H

enum render_entry_id
{
    RenderEntry_Clear,
    RenderEntry_Bitmap,
    RenderEntry_Rect,
};

struct render_entry_base
{
    render_entry_id ID;
};

struct render_entry_clear
{
    v4 Color;
};

struct render_entry_rect
{
    int32 MinX, MinY;
    int32 MaxX, MaxY;
    v4 Color;
};

struct render_entry_bitmap
{
    v2 Scale;
    v2 Offset;
    v2 UVScale;
    v2 UVOffset;
    v4 Color;
};

#endif
