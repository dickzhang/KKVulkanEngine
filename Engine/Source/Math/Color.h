﻿#pragma once

#include "Common/Common.h"
#include "Common/Log.h"
#include "Math/Math.h"
#include "Math/Vector3.h"
#include "Utils/StringUtils.h"

struct Color;

enum
class GammaSpace
{
    Linear,
    Pow22,
    sRGB,
};

struct LinearColor
{

public:
    float r;
    float g;
    float b;
    float a;

    static const LinearColor White;
    static const LinearColor Gray;
    static const LinearColor Black;
    static const LinearColor Transparent;
    static const LinearColor Red;
    static const LinearColor Green;
    static const LinearColor Blue;
    static const LinearColor Yellow;

    static double pow22OneOver255Table[256];
    static double sRGBToLinearTable[256];

public:

    explicit LinearColor(const Color& Color);

    explicit LinearColor()
        : r(0)
        , g(0)
        , b(0)
        , a(0)
    {

    }

    constexpr LinearColor(float inR, float inG, float inB, float inA = 1.0f)
        : r(inR)
        , g(inG)
        , b(inB)
        , a(inA)
    {

    }

    FORCE_INLINE Color ToRGBE() const;

    FORCE_INLINE LinearColor LinearRGBToHSV() const;

    FORCE_INLINE LinearColor HSVToLinearRGB() const;

    FORCE_INLINE Color Quantize() const;

    FORCE_INLINE Color QuantizeRound() const;

    FORCE_INLINE Color ToFColor(const bool sRGB) const;

    FORCE_INLINE LinearColor Desaturate(float desaturation) const;

    FORCE_INLINE float& Component(int32 index)
    {
        return (&r)[index];
    }

    FORCE_INLINE const float& Component(int32 index) const
    {
        return (&r)[index];
    }

    FORCE_INLINE LinearColor operator+(const LinearColor& rhs) const
    {
        return LinearColor(
            this->r + rhs.r,
            this->g + rhs.g,
            this->b + rhs.b,
            this->a + rhs.a
        );
    }

    FORCE_INLINE LinearColor& operator+=(const LinearColor& rhs)
    {
        r += rhs.r;
        g += rhs.g;
        b += rhs.b;
        a += rhs.a;

        return *this;
    }

    FORCE_INLINE LinearColor operator-(const LinearColor& rhs) const
    {
        return LinearColor(
            this->r - rhs.r,
            this->g - rhs.g,
            this->b - rhs.b,
            this->a - rhs.a
        );
    }

    FORCE_INLINE LinearColor& operator-=(const LinearColor& rhs)
    {
        r -= rhs.r;
        g -= rhs.g;
        b -= rhs.b;
        a -= rhs.a;

        return *this;
    }

    FORCE_INLINE LinearColor operator*(const LinearColor& rhs) const
    {
        return LinearColor(
            this->r * rhs.r,
            this->g * rhs.g,
            this->b * rhs.b,
            this->a * rhs.a
        );
    }

    FORCE_INLINE LinearColor& operator*=(const LinearColor& rhs)
    {
        r *= rhs.r;
        g *= rhs.g;
        b *= rhs.b;
        a *= rhs.a;

        return *this;
    }

    FORCE_INLINE LinearColor operator*(float scalar) const
    {
        return LinearColor(
            this->r * scalar,
            this->g * scalar,
            this->b * scalar,
            this->a * scalar
        );
    }

    FORCE_INLINE LinearColor& operator*=(float scalar)
    {
        r *= scalar;
        g *= scalar;
        b *= scalar;
        a *= scalar;

        return *this;
    }

    FORCE_INLINE LinearColor operator/(const LinearColor& rhs) const
    {
        return LinearColor(
            this->r / rhs.r,
            this->g / rhs.g,
            this->b / rhs.b,
            this->a / rhs.a
        );
    }

    FORCE_INLINE LinearColor& operator/=(const LinearColor& rhs)
    {
        r /= rhs.r;
        g /= rhs.g;
        b /= rhs.b;
        a /= rhs.a;

        return *this;
    }

    FORCE_INLINE LinearColor operator/(float scalar) const
    {
        const float invScalar = 1.0f / scalar;

        return LinearColor(
            this->r * invScalar,
            this->g * invScalar,
            this->b * invScalar,
            this->a * invScalar
        );
    }

    FORCE_INLINE LinearColor& operator/=(float scalar)
    {
        const float invScalar = 1.0f / scalar;

        r *= invScalar;
        g *= invScalar;
        b *= invScalar;
        a *= invScalar;

        return *this;
    }

    FORCE_INLINE LinearColor GetClamped(float inMin = 0.0f, float inMax = 1.0f) const
    {
        LinearColor ret;

        ret.r = MMath::Clamp(r, inMin, inMax);
        ret.g = MMath::Clamp(g, inMin, inMax);
        ret.b = MMath::Clamp(b, inMin, inMax);
        ret.a = MMath::Clamp(a, inMin, inMax);

        return ret;
    }

    FORCE_INLINE bool operator==(const LinearColor& other) const
    {
        return this->r == other.r && this->g == other.g && this->b == other.b && this->a == other.a;
    }

    FORCE_INLINE bool operator!=(const LinearColor& Other) const
    {
        return this->r != Other.r || this->g != Other.g || this->b != Other.b || this->a != Other.a;
    }

    FORCE_INLINE bool Equals(const LinearColor& other, float tolerance = KINDA_SMALL_NUMBER) const
    {
        return MMath::Abs(this->r - other.r) < tolerance && MMath::Abs(this->g - other.g) < tolerance && MMath::Abs(this->b - other.b) < tolerance && MMath::Abs(this->a - other.a) < tolerance;
    }

    FORCE_INLINE LinearColor CopyWithNewOpacity(float newOpacicty) const
    {
        LinearColor newCopy = *this;
        newCopy.a = newOpacicty;
        return newCopy;
    }

    FORCE_INLINE float ComputeLuminance() const
    {
        return r * 0.3f + g * 0.59f + b * 0.11f;
    }

    FORCE_INLINE float GetMax() const
    {
        return MMath::Max(MMath::Max(MMath::Max(r, g), b), a);
    }

    FORCE_INLINE bool IsAlmostBlack() const
    {
        return MMath::Square(r) < DELTA && MMath::Square(g) < DELTA && MMath::Square(b) < DELTA;
    }

    FORCE_INLINE float GetMin() const
    {
        return MMath::Min(MMath::Min(MMath::Min(r, g), b), a);
    }

    FORCE_INLINE float GetLuminance() const
    {
        return r * 0.3f + g * 0.59f + b * 0.11f;
    }

    FORCE_INLINE std::string ToString() const
    {
        return StringUtils::Printf("(r=%f,g=%f,b=%f,a=%f)", r, g, b, a);
    }

    static FORCE_INLINE float Dist(const LinearColor &v1, const LinearColor &v2)
    {
        return MMath::Sqrt(MMath::Square(v2.r - v1.r) + MMath::Square(v2.g - v1.g) + MMath::Square(v2.b - v1.b) + MMath::Square(v2.a - v1.a));
    }

    static LinearColor GetHSV(uint8 h, uint8 s, uint8 v);

    static LinearColor MakeRandomColor();

    static LinearColor MakeFromColorTemperature(float temp);

    static LinearColor FromSRGBColor(const Color& Color);

    static LinearColor FromPow22Color(const Color& Color);

    static LinearColor LerpUsingHSV(const LinearColor& from, const LinearColor& to, const float progress);

};

struct Color
{
public:

    uint8 b;
    uint8 g;
    uint8 r;
    uint8 a;

    static const Color White;
    static const Color Black;
    static const Color Transparent;
    static const Color Red;
    static const Color Green;
    static const Color Blue;
    static const Color Yellow;
    static const Color Cyan;
    static const Color Magenta;
    static const Color Orange;
    static const Color Purple;
    static const Color Turquoise;
    static const Color Silver;
    static const Color Emerald;

public:

    Color()
    {

    }

    constexpr Color(uint8 inR, uint8 inG, uint8 inB, uint8 inA = 255)
        : b(inB)
        , g(inG)
        , r(inR)
        , a(inA)
    {

    }

    explicit Color(uint32 inColor)
    {
        DWColor() = inColor;
    }

    FORCE_INLINE uint32& DWColor()
    {
        return *((uint32*)this);
    }

    FORCE_INLINE const uint32& DWColor() const
    {
        return *((uint32*)this);
    }

    FORCE_INLINE bool operator==(const Color &C) const
    {
        return DWColor() == C.DWColor();
    }

    FORCE_INLINE bool operator!=(const Color& C) const
    {
        return DWColor() != C.DWColor();
    }

    FORCE_INLINE void operator+=(const Color& C)
    {
        r = (uint8)MMath::Min((int32)r + (int32)C.r, 255);
        g = (uint8)MMath::Min((int32)g + (int32)C.g, 255);
        b = (uint8)MMath::Min((int32)b + (int32)C.b, 255);
        a = (uint8)MMath::Min((int32)a + (int32)C.a, 255);
    }

    FORCE_INLINE Color WithAlpha(uint8 alpha) const
    {
        return Color(r, g, b, alpha);
    }

    FORCE_INLINE LinearColor ReinterpretAsLinear() const
    {
        return LinearColor(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
    }

    FORCE_INLINE std::string ToHex() const
    {
        return StringUtils::Printf("%02X%02X%02X%02X", r, g, b, a);
    }

    FORCE_INLINE std::string ToString() const
    {
        return StringUtils::Printf("(r=%i,g=%i,b=%i,a=%i)", r, g, b, a);
    }

    FORCE_INLINE uint32 ToPackedARGB() const
    {
        return (a << 24) | (r << 16) | (g << 8) | (b << 0);
    }

    FORCE_INLINE uint32 ToPackedABGR() const
    {
        return (a << 24) | (b << 16) | (g << 8) | (r << 0);
    }

    FORCE_INLINE uint32 ToPackedRGBA() const
    {
        return (r << 24) | (g << 16) | (b << 8) | (a << 0);
    }

    FORCE_INLINE uint32 ToPackedBGRA() const
    {
        return (b << 24) | (g << 16) | (r << 8) | (a << 0);
    }

    LinearColor FromRGBE() const;

    static Color MakeRandomColor();

    static Color MakeRedToGreenColorFromScalar(float scalar);

    static Color MakeFromColorTemperature(float temp);

private:
    explicit Color(const LinearColor& linearColor);

};

FORCE_INLINE LinearColor operator*(float scalar, const LinearColor& Color)
{
    return Color.operator*(scalar);

}