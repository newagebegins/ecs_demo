#include "platform.h"
#include "atlas.h"
#include <stdio.h>
#include <stdlib.h>

#pragma pack(push, 1)
struct bitmap_file_header
{
    uint16 Id;
    uint32 FileSize;
    uint32 Reserved;
    uint32 PixelsOffset;
};

struct dib_header
{
    uint32 HeaderSize;
    int32 BitmapWidth;
    int32 BitmapHeight;
    uint16 ColorPlanesCount;
    uint16 BitsPerPixel;
    uint32 Compression;
    uint32 ImageSize;
    int32 HorizontalResolution;
    int32 VerticalResolution;
    uint32 PaletteColorsCount;
    uint32 ImportantColorsCount;
    uint32 RedMask;
    uint32 GreenMask;
    uint32 BlueMask;
    uint32 AlphaMask;
};
#pragma pack(pop)

internal void *
ReadEntireFile(char *FilePath)
{
    FILE *File = fopen(FilePath, "rb");
    Assert(File);
    fseek(File, 0, SEEK_END);
    u32 FileSize = ftell(File);
    void *Result = malloc(FileSize);
    Assert(Result);
    fseek(File, 0, SEEK_SET);
    size_t ReadCount = fread(Result, FileSize, 1, File);
    Assert(ReadCount == 1);
    fclose(File);
    return(Result);
}

internal game_bitmap
LoadBMP(char *FilePath)
{
    game_bitmap Result;
    void *FileContents = ReadEntireFile(FilePath);
    bitmap_file_header *FileHeader = (bitmap_file_header *)FileContents;
    Assert(FileHeader->Id == 0x4D42);
    dib_header *DIBHeader = (dib_header *)((uint8 *)FileContents + sizeof(bitmap_file_header));
    Assert(DIBHeader->Compression == 3);
    Assert(DIBHeader->BitsPerPixel == BITMAP_BYTES_PER_PIXEL*8);
    Result.Width = DIBHeader->BitmapWidth;
    Result.Height = DIBHeader->BitmapHeight;
    Assert(Result.Height > 0);
    Result.Pitch = DIBHeader->BitmapWidth*BITMAP_BYTES_PER_PIXEL;
    Result.Pixels = (uint8 *)FileContents + FileHeader->PixelsOffset;
    return(Result);
}

internal void
CopyBitmap(atlas *Atlas, game_bitmap *Bitmap, s32 DestX, s32 DestY)
{
    u8 *DestRow = (u8 *)Atlas->Pixels + DestY*ATLAS_PITCH + DestX*BITMAP_BYTES_PER_PIXEL;
    u8 *SourceRow = (u8 *)Bitmap->Pixels;

    for(s32 Y = 0;
        Y < Bitmap->Height;
        ++Y)
    {
        u32 *Dest = (u32 *)DestRow;
        u32 *Source = (u32 *)SourceRow;

        for(s32 X = 0;
            X < Bitmap->Width;
            ++X)
        {
            *Dest++ = *Source++;
        }

        DestRow += ATLAS_PITCH;
        SourceRow += Bitmap->Pitch;
    }
}

inline void
CopyBitmaps(atlas *Atlas, bitmap_id ID, game_bitmap *Bitmaps, s32 BitmapCount, s32 *DestY, s32 FrameWidth = 0, s32 FrameHeight = 0)
{
    bitmap_info *Info = Atlas->Infos + ID;
    Info->FrameWidth = FrameWidth ? FrameWidth : Bitmaps[0].Width;
    Info->FrameHeight = FrameHeight ? FrameHeight : Bitmaps[0].Height;
    Info->OffsetY = *DestY;

    u32 FrameCountX = (BitmapCount * Bitmaps[0].Width) / Info->FrameWidth;
    u32 FrameCountY = Bitmaps[0].Height / Info->FrameHeight;
    Info->FrameCount = FrameCountX * FrameCountY;

    for(s32 BitmapIndex = 0;
        BitmapIndex < BitmapCount;
        ++BitmapIndex)
    {
        s32 DestX = BitmapIndex*Info->FrameWidth;
        CopyBitmap(Atlas, Bitmaps + BitmapIndex, DestX, *DestY);
    }

    *DestY += Bitmaps[0].Height;
}

#if OUTPUT_BMP
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#endif

int main(void)
{
    game_bitmap FontBitmap = LoadBMP("assets/font.bmp");
    game_bitmap GuyBitmap = LoadBMP("assets/guy.bmp");
    game_bitmap BombBitmap = LoadBMP("assets/bomb.bmp");

    atlas Atlas_ = {};
    atlas *Atlas = &Atlas_;
    Atlas->Infos = (bitmap_info *)malloc(ATLAS_INFOS_SIZE);
    Atlas->Pixels = malloc(ATLAS_PIXELS_SIZE);
    Assert(Atlas->Infos);
    Assert(Atlas->Pixels);

    ZeroStruct(Atlas->Infos[0]);

    s32 DestY = 0;

    CopyBitmaps(Atlas, Bitmap_Font, &FontBitmap, 1, &DestY, 8, 8);
    CopyBitmaps(Atlas, Bitmap_Guy, &GuyBitmap, 1, &DestY);
    CopyBitmaps(Atlas, Bitmap_Bomb, &BombBitmap, 1, &DestY);

    Assert(DestY <= ATLAS_HEIGHT);

#if OUTPUT_BMP
    stbi_flip_vertically_on_write(1); 
    stbi_write_bmp("atlas.bmp", ATLAS_WIDTH, ATLAS_HEIGHT, 4, Atlas->Pixels);
#endif

    FILE *File = fopen("atlas.bin", "wb");
    Assert(File);

    atlas_header Header = {};
    Header.Width = ATLAS_WIDTH;
    Header.Height = ATLAS_HEIGHT;
    Header.InfosSize = ATLAS_INFOS_SIZE;
    Header.PixelsSize = ATLAS_PIXELS_SIZE;
    Header.InfosOffset = sizeof(Header);
    Header.PixelsOffset = Header.InfosOffset + ATLAS_INFOS_SIZE;

    size_t WrittenCount;

    WrittenCount = fwrite(&Header, sizeof(Header), 1, File);
    Assert(WrittenCount == 1);

    WrittenCount = fwrite(Atlas->Infos, ATLAS_INFOS_SIZE, 1, File);
    Assert(WrittenCount == 1);

    WrittenCount = fwrite(Atlas->Pixels, ATLAS_PIXELS_SIZE, 1, File);
    Assert(WrittenCount == 1);

    return(0);
}
