internal void
PushClear(render_group *Group, color Color)
{
    render_entry_base *Base = PushStruct(Group->Arena, render_entry_base);
    Base->ID = RenderEntry_Clear;

    render_entry_clear *Entry = PushStruct(Group->Arena, render_entry_clear);
    Entry->Color = Group->Palette[Color];
}

internal void
PushRect(render_group *Group, int32 MinX, int32 MinY, int32 MaxX, int32 MaxY,
         color Color = Color_White, bool32 WrapX = true)
{
    render_entry_base *Base = PushStruct(Group->Arena, render_entry_base);
    Base->ID = RenderEntry_Rect;

    render_entry_rect *Entry = PushStruct(Group->Arena, render_entry_rect);
    Entry->MinX = MinX;
    Entry->MinY = MinY;
    Entry->MaxX = MaxX;
    Entry->MaxY = MaxY;
    Entry->Color = Group->Palette[Color];

    if(WrapX)
    {
        if(MinX < 0)
        {
            MinX += BACKBUFFER_WIDTH;
            MaxX += BACKBUFFER_WIDTH;
            PushRect(Group, MinX, MinY, MaxX, MaxY, Color, false);
        }
        else if(MaxX > BACKBUFFER_WIDTH)
        {
            MaxX -= BACKBUFFER_WIDTH;
            MinX -= BACKBUFFER_WIDTH;
            PushRect(Group, MinX, MinY, MaxX, MaxY, Color, false);
        }
    }
}

internal void
PushRect(render_group *Group, rectangle2i Rect, color Color = Color_White, bool32 WrapX = true)
{
    PushRect(Group, Rect.MinX, Rect.MinY, Rect.MaxX, Rect.MaxY, Color, WrapX);
}

internal void
PushBitmap(render_group *Group, bitmap_id ID, int32 MinX, int32 MinY,
           u32 FrameIndex = 0, bool32 MirrorX = false, color Color = Color_White,
           bool32 WrapX = true, s32 FrameOffsetX = 0, s32 VisibleWidthArg = -1)
{
    ++Group->BitmapCount;

    render_entry_base *Base = PushStruct(Group->Arena, render_entry_base);
    Base->ID = RenderEntry_Bitmap;

    bitmap_info *Info = Group->BitmapInfos + ID;
    r32 InvWidth = 1.0f / ATLAS_WIDTH;
    r32 InvHeight = 1.0f / ATLAS_HEIGHT;

    Assert(FrameOffsetX >= 0);
    Assert(FrameOffsetX <= Info->FrameWidth);

    s32 VisibleWidth = VisibleWidthArg;
    if(VisibleWidth < 0)
    {
        VisibleWidth = Info->FrameWidth - FrameOffsetX;
    }
    Assert(VisibleWidth >= 0);
    Assert(VisibleWidth <= (Info->FrameWidth - FrameOffsetX));

    s32 OffsetX;
    s32 OffsetY = Info->OffsetY;

    v2 UVScale = V2((r32)VisibleWidth*InvWidth,
                    (r32)Info->FrameHeight*InvHeight);

    if(MirrorX)
    {
        OffsetX = (FrameIndex + 1)*Info->FrameWidth - FrameOffsetX;
        UVScale.x = -UVScale.x;
    }
    else
    {
        OffsetX = FrameIndex*Info->FrameWidth + FrameOffsetX;
    }

    while(OffsetX >= ATLAS_WIDTH)
    {
        OffsetX -= ATLAS_WIDTH;
        OffsetY += Info->FrameHeight;
    }

    v2 UVOffset = {(r32)OffsetX*InvWidth,
                   (r32)OffsetY*InvHeight};

    render_entry_bitmap *Entry = PushStruct(Group->Arena, render_entry_bitmap);
    Entry->Scale = V2i(VisibleWidth, Info->FrameHeight);
    Entry->Offset = V2i(MinX, MinY);
    Entry->Color = Group->Palette[Color];
    Entry->UVOffset = UVOffset;
    Entry->UVScale = UVScale;

    if(WrapX)
    {
        int32 MaxX = MinX + VisibleWidth;
        if(MinX < 0)
        {
            MinX += BACKBUFFER_WIDTH;
            PushBitmap(Group, ID, MinX, MinY, FrameIndex, MirrorX, Color, false, FrameOffsetX, VisibleWidthArg);
        }
        else if(MaxX > BACKBUFFER_WIDTH)
        {
            MinX -= BACKBUFFER_WIDTH;
            PushBitmap(Group, ID, MinX, MinY, FrameIndex, MirrorX, Color, false, FrameOffsetX, VisibleWidthArg);
        }
    }
}

internal void
PushString(render_group *Group, char *String, int32 X, int32 Y, color Color)
{
    int32 DestX = X;
    for(char *Letter = String;
        *Letter;
        ++Letter)
    {
        u32 FrameIndex = *Letter - ' ';
        PushBitmap(Group, Bitmap_Font, DestX, Y, FrameIndex,
                   false, Color, false);
        DestX += 8;
    }
}

inline v4
ColorUInt32ToV4(uint32 Color)
{
    v4 Result;
    real32 Inv255 = 1.0f / 255.0f;
    Result.r = (real32)((Color >> 16) & 0xFF)*Inv255;
    Result.g = (real32)((Color >> 8) & 0xFF)*Inv255;
    Result.b = (real32)((Color >> 0) & 0xFF)*Inv255;
    Result.a = 1.0f;
    return(Result);
}

internal void
InitializeRenderGroup(render_group *Group, memory_arena *Arena, bitmap_info *BitmapInfos)
{
    Group->Arena = Arena;

    Group->Palette[Color_Black] = ColorUInt32ToV4(0x000000);
    Group->Palette[Color_Blue] = ColorUInt32ToV4(0x0000C0);
    Group->Palette[Color_Red] = ColorUInt32ToV4(0xC00000);
    Group->Palette[Color_Magenta] = ColorUInt32ToV4(0xC000C0);
    Group->Palette[Color_Green] = ColorUInt32ToV4(0x00C000);
    Group->Palette[Color_Cyan] = ColorUInt32ToV4(0x00C0C0);
    Group->Palette[Color_Yellow] = ColorUInt32ToV4(0xC0C000);
    Group->Palette[Color_White] = ColorUInt32ToV4(0xFFFFFF);
    Group->Palette[Color_BrightBlue] = ColorUInt32ToV4(0x0000FF);
    Group->Palette[Color_BrightRed] = ColorUInt32ToV4(0xFF0000);
    Group->Palette[Color_BrightMagenta] = ColorUInt32ToV4(0xFF00FF);
    Group->Palette[Color_BrightGreen] = ColorUInt32ToV4(0x00FF00);
    Group->Palette[Color_BrightCyan] = ColorUInt32ToV4(0x00FFFF);
    Group->Palette[Color_BrightYellow] = ColorUInt32ToV4(0xFFFF00);

    Group->BitmapInfos = BitmapInfos;
    Group->BitmapCount = 0;
}
