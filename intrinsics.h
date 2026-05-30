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

inline s32
ModuloN(s32 Value, s32 N)
{
    s32 Result = Value % N;
    Result = (Result < 0) ? (Result + N) : Result;
    return(Result);
}

inline r32
ModuloN(r32 Value, r32 N)
{
#if WASM_BUILD
    r32 Result = __builtin_fmodf(Value, N);
#else
    r32 Result = fmodf(Value, N);
#endif
    if(Result < 0.0f)
    {
        Result += N;
    }
    return(Result);
}

#endif
