// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.


#include "CosmicRealisticNoiseStrategy.h"

void FCosmicRealisticNoiseStrategy::Initialize(
    int32 InSeed,
    float InHeightNormalizationScale,
    FCosmicNoiseBiomeParameters InBiomeParameters,
    FCosmicNoiseDataLayer InContinental,
    FCosmicNoiseDataLayer InMountain,
    FCosmicNoiseDataLayer InHill,
    FCosmicNoiseDataLayer InDiffer,
    FCosmicNoiseDataLayer InRiver)
{
    Seed = InSeed;
    HeightNormalizationScale = InHeightNormalizationScale;
    BiomeParameters = InBiomeParameters;

    ContinentalLayer = InContinental;
    MountainLayer = InMountain;
    HillLayer = InHill;
    DifferLayer = InDiffer;

    auto SetupSimplexFBM = [&](FastNoiseLite& Noise, const FCosmicNoiseDataLayer& Layer, int32 Offset)
        {
            Noise.SetSeed(Seed + Offset);
            Noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
            Noise.SetFractalType(FastNoiseLite::FractalType_FBm);
            Noise.SetFrequency(Layer.Frequency);
            Noise.SetFractalOctaves(Layer.Octaves);
            Noise.SetFractalLacunarity(Layer.Lacunarity);
            Noise.SetFractalGain(Layer.Persistence);
        };

    // Continentes (base)
    SetupSimplexFBM(ContinentalNoise, ContinentalLayer, 0);

    // Colinas (FBM suave)
    SetupSimplexFBM(HillNoise, HillLayer, 200);

    // Detalle (ValueCubic FBM)
    DifferNoise.SetSeed(Seed + 300);
    DifferNoise.SetNoiseType(FastNoiseLite::NoiseType_ValueCubic);
    DifferNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    DifferNoise.SetFrequency(DifferLayer.Frequency);
    DifferNoise.SetFractalOctaves(DifferLayer.Octaves);
    DifferNoise.SetFractalLacunarity(DifferLayer.Lacunarity);
    DifferNoise.SetFractalGain(DifferLayer.Persistence);

    // Montañas (RIDGED)
    MountainNoise.SetSeed(Seed + 100);
    MountainNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    MountainNoise.SetFractalType(FastNoiseLite::FractalType_Ridged);
    MountainNoise.SetFrequency(MountainLayer.Frequency);
    MountainNoise.SetFractalOctaves(MountainLayer.Octaves);
    MountainNoise.SetFractalLacunarity(MountainLayer.Lacunarity);
    MountainNoise.SetFractalGain(MountainLayer.Persistence);

    // (Opcional) Clima – si los necesitas para el color, mantenlos
    SetupSimplexFBM(HumidityNoise, { BiomeParameters.HumidityFrequency, BiomeParameters.HumidityOctaves, 2.0f, 0.5f, 1.0f }, 500);
    SetupSimplexFBM(TempNoise, { BiomeParameters.TemperatureFrequency, 3, 2.0f, 0.5f, 1.0f }, 600);
}

void FCosmicRealisticNoiseStrategy::EvaluatePoint(
    const FVector& NoiseDir,
    float& OutHeight,
    FLinearColor& OutColor) const
{
    const float X = NoiseDir.X;
    const float Y = NoiseDir.Y;
    const float Z = NoiseDir.Z;

    // ------------------ CAPA CONTINENTAL (océano vs tierra) ------------------
    float ContinentRaw = ContinentalNoise.GetNoise(X, Y, Z);
    float Continent = (ContinentRaw + 1.0f) * 0.5f;          // [0,1]
    float BaseHeight = (Continent - 0.5f) * ContinentalLayer.Amplitude;

    float OceanMask = FMath::SmoothStep(0.4f, 0.5f, Continent);
    float LandMask = 1.0f - OceanMask;

    // ------------------ MÁSCARA DE RELIEVE (montaña vs colina) ------------------
    // Obtenemos un valor de máscara entre 0 y 1 desde DifferNoise
    float DiffRaw = DifferNoise.GetNoise(X, Y, Z);
    // Normalizamos de [-1,1] o [0,1] según el ruido. ValueCubic suele dar [-1,1].
    float DiffMask = (DiffRaw + 1.0f) * 0.5f;   // ahora [0,1]
    // Opcional: aplicar contraste para zonas más definidas
    DiffMask = FMath::Clamp((DiffMask - 0.5f) * 2.0f + 0.5f, 0.0f, 1.0f); // contraste opcional

    // ------------------ GENERACIÓN DE MONTAÑAS Y COLINAS ------------------
    // Montañas (Ridged)
    float MountainRaw = MountainNoise.GetNoise(X, Y, Z);
    float Mountain = FMath::Max(0.0f, MountainRaw);    // Ridged ya da [0,1] aprox
    float MountainHeight = Mountain * MountainLayer.Amplitude;

    // Colinas (FBM suave)
    float HillRaw = HillNoise.GetNoise(X, Y, Z);
    float Hill = (HillRaw + 1.0f) * 0.5f;               // [0,1]
    float HillHeight = Hill * HillLayer.Amplitude;

    // Mezcla según DiffMask: donde DiffMask es alto -> montañas, bajo -> colinas
    float ReliefHeight = FMath::Lerp(HillHeight, MountainHeight, DiffMask);

    // ------------------ COMBINACIÓN FINAL ------------------
    // Solo aplicamos relieve sobre tierra firme
    float Height = BaseHeight + (ReliefHeight * LandMask);

    // Hundir océanos (opcional)
    Height = FMath::Lerp(Height, -ContinentalLayer.Amplitude * 0.6f, OceanMask);

    // (Opcional) Añadir un pequeño ruido de detalle extra si quieres, pero ya no usamos DifferLayer para eso.
    // Si deseas microdetalle, puedes añadir otra capa aparte, pero con esto es suficiente.

    // ------------------ NORMALIZACIÓN Y COLOR ------------------
    float TrueMinHeight = -0.6f * ContinentalLayer.Amplitude;
    float TrueMaxHeight = 0.5f * ContinentalLayer.Amplitude
        + FMath::Max(MountainLayer.Amplitude, HillLayer.Amplitude); // el mayor de los dos
    float NormalizedMin = TrueMinHeight * HeightNormalizationScale;
    float NormalizedMax = TrueMaxHeight * HeightNormalizationScale;
    float AltitudeNormalized = (Height - NormalizedMin) / (NormalizedMax - NormalizedMin);
    AltitudeNormalized = FMath::Clamp(AltitudeNormalized, 0.0f, 1.0f);

    // Cálculo de clima (si lo necesitas para color)
    float RawHum = HumidityNoise.GetNoise(X, Y, Z);
    float Humidity = (RawHum + 1.0f) * 0.5f;
    Humidity = FMath::Clamp((Humidity + BiomeParameters.HumidityOffset - 0.5f) * BiomeParameters.HumidityContrast + 0.5f, 0.0f, 1.0f);

    float Latitude = FMath::Abs(Z);
    float BaseTemp = 1.0f - (Latitude * BiomeParameters.LatitudeEffect);
    float TempVar = TempNoise.GetNoise(X, Y, Z) * 0.2f;
    float Temperature = FMath::Clamp(BaseTemp + TempVar, 0.0f, 1.0f);
    float VisualTemp = FMath::Clamp(Temperature - (AltitudeNormalized * BiomeParameters.AltitudeTemperaturePenalty), 0.0f, 1.0f);

    OutHeight = Height;
    OutColor = FLinearColor(AltitudeNormalized, VisualTemp, Humidity, 1.0f);
}
