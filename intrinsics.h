#if !defined(INTRINSICS_H)
#define INTRINSICS_H

#if !WASM_BUILD
#include <math.h>
#endif

inline real32
SquareRoot(real32 Value)
{
#if WASM_BUILD
    real32 Result = __builtin_sqrtf(Value);
#else
    real32 Result = sqrtf(Value);
#endif
    return(Result);
}

inline int32
RoundReal32ToInt32(real32 X)
{
#if WASM_BUILD
    int32 Result = (int32)__builtin_roundf(X);
#else
    int32 Result = (int32)roundf(X);
#endif
    return(Result);
}

inline real32
Floor(real32 X)
{
#if WASM_BUILD
    real32 Result = __builtin_floorf(X);
#else
    real32 Result = floorf(X);
#endif
    return(Result);
}

inline int32
FloorReal32ToInt32(real32 X)
{
    int32 Result = (int32)Floor(X);
    return(Result);
}

inline real32
AbsoluteValue(real32 Value)
{
#if WASM_BUILD
    real32 Result = __builtin_fabsf(Value);
#else
    real32 Result = fabsf(Value);
#endif
    return(Result);
}

inline int32
ModuloN(int32 Value, int32 N)
{
    int32 Result = Value % N;
    if(Result < 0)
    {
        Result += N;
    }
    return(Result);
}

#if 0
inline real32
Cos(real32 Angle)
{
#if WASM_BUILD
    real32 Result = __builtin_cosf(Angle);
#else
    real32 Result = cosf(Angle);
#endif
    return(Result);
}

inline real32
Sin(real32 Angle)
{
#if WASM_BUILD
    real32 Result = __builtin_sinf(Angle);
#else
    real32 Result = sinf(Angle);
#endif
    return(Result);
}
#endif

#endif
