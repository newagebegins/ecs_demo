#if !defined(RANDOM_H)
#define RANDOM_H

struct random_series
{
    uint32 A;
};

inline random_series
RandomSeries(uint32 Seed)
{
    random_series Result = {Seed};
    return(Result);
}

inline uint32
NextRandomNumber(random_series *Series)
{
    uint32 X = Series->A;
    X ^= X << 13;
    X ^= X >> 17;
    X ^= X << 5;
    Series->A = X;
    return(X);
}

inline uint32
RandomChoice(random_series *Series, uint32 Count)
{
    uint32 Result = NextRandomNumber(Series) % Count;
    return(Result);
}

#define RandomPick(Series, Array) (Array[RandomChoice(Series, ArrayCount(Array))])

inline int32
RandomBetween(random_series *Series, int32 Min, int32 Max)
{
    int32 Result = Min + RandomChoice(Series, Max - Min + 1);
    return(Result);
}

inline real32
RandomUnilateral(random_series *Series)
{
    real32 Divisor = 1.0f / (real32)UINT32_MAX;
    real32 Result = (real32)NextRandomNumber(Series)*Divisor;
    return(Result);
}

inline real32
RandomBiilateral(random_series *Series)
{
    real32 Result = 2.0f*RandomUnilateral(Series) - 1.0f;
    return(Result);
}

inline real32
RandomBetween(random_series *Series, real32 Min, real32 Max)
{
    real32 Result = Lerp(Min, RandomUnilateral(Series), Max);
    return(Result);
}

#endif
