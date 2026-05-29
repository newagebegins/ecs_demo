#if !defined(MY_MATH_H)
#define MY_MATH_H

union v2
{
    struct
    {
        real32 x;
        real32 y;
    };
    struct
    {
        real32 u;
        real32 v;
    };
    real32 E[2];
};

union v3
{
    struct
    {
        real32 x, y, z;
    };
    struct
    {
        real32 r, g, b;
    };
    real32 E[3];
};

union v4
{
    struct
    {
        real32 x, y, z, w;
    };
    struct
    {
        real32 r, g, b, a;
    };
    real32 E[4];
};

struct rectangle2i
{
    int32 MinX, MinY;
    int32 MaxX, MaxY;
};

//
// NOTE: Scalar operations
//

inline real32
SignOf(real32 Value)
{
    real32 Result = (Value >= 0.0f) ? 1.0f : -1.0f;
    return(Result);
}

inline real32
Square(real32 A)
{
    real32 Result = A*A;
    return(Result);
}

inline real32
Clamp(real32 Min, real32 Val, real32 Max)
{
    real32 Result = Val;

    if(Result < Min)
    {
        Result = Min;
    }
    else if(Result > Max)
    {
        Result = Max;
    }

    return(Result);
}

inline real32
Clamp01(real32 Val)
{
    real32 Result = Clamp(0.0f, Val, 1.0f);

    return(Result);
}

inline real32
Lerp(real32 A, real32 t, real32 B)
{
    real32 Result = (1.0f - t)*A + t*B;

    return(Result);
}

inline real32
InverseLerp01(real32 Min, real32 t, real32 Max)
{
    real32 Result = 0.0f;
    real32 Range = Max - Min;
    if(Range != 0.0f)
    {
        Result = Clamp01((t - Min) / Range);
    }
    return(Result);
}

inline real32
SafeRatioN(real32 Numerator, real32 Divisor, real32 N)
{
    real32 Result = N;

    if(Divisor != 0.0f)
    {
        Result = Numerator / Divisor;
    }

    return(Result);
}

inline real32
SafeRatio0(real32 Numerator, real32 Divisor)
{
    real32 Result = SafeRatioN(Numerator, Divisor, 0.0f);

    return(Result);
}

inline real32
SafeRatio1(real32 Numerator, real32 Divisor)
{
    real32 Result = SafeRatioN(Numerator, Divisor, 1.0f);

    return(Result);
}

//
// NOTE: v2 operations
//

inline v2
V2(real32 X, real32 Y)
{
    v2 Result;

    Result.x = X;
    Result.y = Y;

    return(Result);
}

inline v2
V2i(int32 X, int32 Y)
{
    v2 Result = V2((real32)X, (real32)Y);

    return(Result);
}

inline v2
V2i(uint32 X, uint32 Y)
{
    v2 Result = V2((real32)X, (real32)Y);

    return(Result);
}

inline v2
operator-(v2 V)
{
    v2 Result;
    Result.x = -V.x;
    Result.y = -V.y;
    return(Result);
}

inline v2
operator+(v2 A, v2 B)
{
    v2 Result;
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    return(Result);
}

inline v2
operator-(v2 A, v2 B)
{
    v2 Result;
    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    return(Result);
}

inline v2 &
operator+=(v2 &A, v2 B)
{
    A = A + B;
    return(A);
}

inline v2 &
operator-=(v2 &A, v2 B)
{
    A = A - B;
    return(A);
}

inline v2
operator*(v2 V, real32 S)
{
    v2 Result;
    Result.x = V.x*S;
    Result.y = V.y*S;
    return(Result);
}

inline v2
operator*(real32 S, v2 V)
{
    v2 Result = V*S;
    return(Result);
}

inline v2 &
operator*=(v2 &V, real32 S)
{
    V = V*S;
    return(V);
}

inline v2
operator/(v2 V, real32 S)
{
    v2 Result;
    Result.x = V.x/S;
    Result.y = V.y/S;
    return(Result);
}

inline v2
operator/(real32 S, v2 V)
{
    v2 Result = V/S;
    return(Result);
}

inline v2 &
operator/=(v2 &V, real32 S)
{
    V = V/S;
    return(V);
}

inline real32
Inner(v2 A, v2 B)
{
    real32 Result = A.x*B.x + A.y*B.y;
    return(Result);
}

inline v2
Hadamard(v2 A, v2 B)
{
    v2 Result = {A.x*B.x, A.y*B.y};
    return(Result);
}

inline real32
LengthSq(v2 V)
{
    real32 Result = Inner(V, V);

    return(Result);
}

inline real32
Length(v2 V)
{
    real32 Result = SquareRoot(LengthSq(V));

    return(Result);
}

inline v2
Normalize(v2 A)
{
    v2 Result = A / Length(A);

    return(Result);
}

inline v2
Clamp(real32 Min, v2 V, real32 Max)
{
    v2 Result;

    Result.x = Clamp(Min, V.x, Max);
    Result.y = Clamp(Min, V.y, Max);

    return(Result);
}

inline v2
Clamp01(v2 V)
{
    v2 Result = Clamp(0.0f, V, 1.0f);

    return(Result);
}

inline v2
Perp(v2 A)
{
    v2 Result = {-A.y, A.x};

    return(Result);
}

inline v2
Lerp(v2 A, real32 t, v2 B)
{
    v2 Result = (1.0f - t)*A + t*B;

    return(Result);
}

//
// NOTE(slava): v3 operations
//

inline v3
V3(real32 X, real32 Y, real32 Z)
{
    v3 Result;

    Result.x = X;
    Result.y = Y;
    Result.z = Z;

    return(Result);
}

inline v3
V3i(int32 X, int32 Y, int32 Z)
{
    v3 Result = V3((real32)X, (real32)Y, (real32)Z);

    return(Result);
}

inline v3
V3(v2 XY, real32 Z)
{
    v3 Result;

    Result.x = XY.x;
    Result.y = XY.y;
    Result.z = Z;

    return(Result);
}

inline v3
operator-(v3 V)
{
    v3 Result;

    Result.x = -V.x;
    Result.y = -V.y;
    Result.z = -V.z;

    return(Result);
}

inline v3
operator+(v3 A, v3 B)
{
    v3 Result;
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;
    return(Result);
}

inline v3
operator-(v3 A, v3 B)
{
    v3 Result;
    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    Result.z = A.z - B.z;
    return(Result);
}

inline v3 &
operator+=(v3 &A, v3 B)
{
    A = A + B;
    return(A);
}

inline v3 &
operator-=(v3 &A, v3 B)
{
    A = A - B;
    return(A);
}

inline v3
operator*(v3 V, real32 S)
{
    v3 Result;
    Result.x = V.x*S;
    Result.y = V.y*S;
    Result.z = V.z*S;
    return(Result);
}

inline v3
operator/(v3 V, real32 S)
{
    v3 Result;
    Result.x = V.x/S;
    Result.y = V.y/S;
    Result.z = V.z/S;
    return(Result);
}

inline v3
operator*(real32 S, v3 V)
{
    v3 Result = V*S;
    return(Result);
}

inline v3
operator/(real32 S, v3 V)
{
    v3 Result = V/S;
    return(Result);
}

inline v3 &
operator*=(v3 &V, real32 S)
{
    V = V*S;
    return(V);
}

inline real32
Inner(v3 A, v3 B)
{
    real32 Result = A.x*B.x + A.y*B.y + A.z*B.z;
    return(Result);
}

inline v3
Hadamard(v3 A, v3 B)
{
    v3 Result = {A.x*B.x, A.y*B.y, A.z*B.z};
    return(Result);
}

inline real32
LengthSq(v3 V)
{
    real32 Result = Inner(V, V);

    return(Result);
}

inline real32
Length(v3 V)
{
    real32 Result = SquareRoot(LengthSq(V));

    return(Result);
}

inline v3
Normalize(v3 A)
{
    v3 Result = A / Length(A);

    return(Result);
}

inline v3
Clamp(real32 Min, v3 V, real32 Max)
{
    v3 Result;

    Result.x = Clamp(Min, V.x, Max);
    Result.y = Clamp(Min, V.y, Max);
    Result.z = Clamp(Min, V.z, Max);

    return(Result);
}

inline v3
Clamp01(v3 V)
{
    v3 Result = Clamp(0.0f, V, 1.0f);

    return(Result);
}

inline v3
Lerp(v3 A, real32 t, v3 B)
{
    v3 Result = (1.0f - t)*A + t*B;

    return(Result);
}

//
// NOTE(slava): Intervals
//

inline bool32
HalfOpenIntervalsOverlap(int32 Start0, int32 End0, int32 Start1, int32 End1)
{
    bool32 Result = ((End0 > Start1) && (End1 > Start0));
    return(Result);
}

inline bool32
HalfOpenIntervalsOverlapModN(int32 Start0, int32 End0, int32 Start1, int32 End1, int32 N)
{
    int32 W0 = ModuloN(End0 - Start0, N);
    int32 W1 = ModuloN(End1 - Start1, N);
    bool32 Result = ((W1 && ModuloN(Start1 - Start0, N) < W0) ||
                     (W0 && ModuloN(Start0 - Start1, N) < W1));
    return(Result);
}


//
// NOTE(slava): rectangle2i operations
//

inline bool32
RectsOverlap(rectangle2i A, rectangle2i B)
{
    bool32 Result = (HalfOpenIntervalsOverlap(A.MinX, A.MaxX, B.MinX, B.MaxX) &&
                     HalfOpenIntervalsOverlap(A.MinY, A.MaxY, B.MinY, B.MaxY));
    return(Result);
}

inline bool32
RectsOverlapWrapX(rectangle2i A, rectangle2i B, int32 WrapX)
{
    bool32 Result = (HalfOpenIntervalsOverlapModN(A.MinX, A.MaxX, B.MinX, B.MaxX, WrapX) &&
                     HalfOpenIntervalsOverlap(A.MinY, A.MaxY, B.MinY, B.MaxY));
    return(Result);
}

inline s32
GetWidth(rectangle2i A)
{
    s32 Result = A.MaxX - A.MinX;
    Assert(Result >= 0);
    return(Result);
}

#endif
