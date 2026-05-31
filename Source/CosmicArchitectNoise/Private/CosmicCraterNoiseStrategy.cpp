

#include "CosmicCraterNoiseStrategy.h"
#include "CosmicEarthLikeNoiseStrategy.h"

void FCosmicCraterNoiseStrategy::Initialize(int32 InSeed, FCosmicNoiseLayer InLayerParameters, FCosmicNoiseBiomeParameters InBiomeParameters, FCosmicNoiseCraterParameters InCraterParameters)
{
    Seed = InSeed;
    LayerParameters = InLayerParameters;
    BiomeParameters = InBiomeParameters;
    CraterParameters = InCraterParameters;

    Noise.SetSeed(Seed);
    switch (LayerParameters.NoiseType) {
    case ECosmicNoiseType::Perlin: Noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin); break;
    case ECosmicNoiseType::Simplex:
    case ECosmicNoiseType::Ridged: Noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2); break;
    case ECosmicNoiseType::Cellular: Noise.SetNoiseType(FastNoiseLite::NoiseType_Cellular); break;
    case ECosmicNoiseType::Value: Noise.SetNoiseType(FastNoiseLite::NoiseType_Value); break; 
    }
    switch (LayerParameters.FractalType) {
    case ECosmicFractalType::None: Noise.SetFractalType(FastNoiseLite::FractalType_None); break;
    case ECosmicFractalType::FBM: Noise.SetFractalType(FastNoiseLite::FractalType_FBm); break;
    case ECosmicFractalType::Ridged: Noise.SetFractalType(FastNoiseLite::FractalType_Ridged); break;
    case ECosmicFractalType::PingPong: Noise.SetFractalType(FastNoiseLite::FractalType_PingPong); break;
    }
    Noise.SetFrequency(LayerParameters.Frequency);
    Noise.SetFractalOctaves(LayerParameters.Octaves);
    Noise.SetFractalLacunarity(LayerParameters.Lacunarity);
    Noise.SetFractalGain(LayerParameters.Persistence);

    HumidityNoise.SetSeed(Seed + 128);
    HumidityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    HumidityNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    HumidityNoise.SetFrequency(BiomeParameters.HumidityFrequency);
    HumidityNoise.SetFractalOctaves(BiomeParameters.HumidityOctaves);

    TempNoise.SetSeed(Seed + 256);
    TempNoise.SetFrequency(BiomeParameters.TemperatureFrequency);

    //Ruido para los cráteres
    CraterNoise.SetSeed(Seed + 512);
    CraterNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);

    CraterNoise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
    CraterNoise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance);
    CraterNoise.SetFrequency(CraterParameters.CraterFrequency);

    /*if (CraterParameters.CraterOctaves > 1)
    {
        CraterNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
        CraterNoise.SetFractalOctaves(CraterParameters.CraterOctaves);
        CraterNoise.SetFractalLacunarity(CraterParameters.CraterLacunarity);
        CraterNoise.SetFractalGain(CraterParameters.CraterPersistence);
    }*/
    CraterNoise.SetFractalType(FastNoiseLite::FractalType_None);
}

void FCosmicCraterNoiseStrategy::EvaluatePoint(const FVector& NoiseDir, float& OutHeight, FLinearColor& OutColor) const
{
    const float X = NoiseDir.X;
    const float Y = NoiseDir.Y;
    const float Z = NoiseDir.Z;

    // --- ALTURA BASE ---
    float BaseNoise = Noise.GetNoise(X, Y, Z); // [-1, 1]
    float Height = BaseNoise * LayerParameters.Amplitude;

    // --- CRÁTERES ---
    float FinalCraterHeight = 0.0f;
    float FreqScale = 1.0f;
    float AmpScale = 1.0f;

    for (int32 i = 0; i < CraterParameters.CraterOctaves; ++i)
    {
        const float OffX = (X * FreqScale) + (i * 133.7f);
        const float OffY = (Y * FreqScale) + (i * 133.7f);
        const float OffZ = (Z * FreqScale) + (i * 133.7f);

        // Remapear distancia celular de [-1,1] a [0,1]
        float CellDistance = CraterNoise.GetNoise(OffX, OffY, OffZ);
        CellDistance = (CellDistance + 1.0f) * 0.5f;

        const float SizeVariation = FMath::Lerp(0.6f, 1.4f, (BaseNoise + 1.0f) * 0.5f);
        const float DynamicRadius = FMath::Max(CraterParameters.CraterRadiusMultiplier * SizeVariation, 0.01f);
        const float CurrentDepth = CraterParameters.CraterDepth * AmpScale;

        // Solo procesar puntos dentro del area de influencia del cráter
        if (CellDistance < DynamicRadius * 1.3f)
        {
            const float t = CellDistance / DynamicRadius; // 0=centro, 1=borde
            float CraterShape = 0.0f;

            // --- CAVIDAD (interior, t < 1) ---
            if (t < 1.0f)
            {
                // FloorHeight=0  bowl completo sin suelo plano
                // FloorHeight=0.5  suelo plano hasta la mitad del radio
                // FloorHeight=1  cráter completamente plano
                const float FloorStart = FMath::Clamp(CraterParameters.CraterFloorHeight, 0.0f, 0.99f);

                float Bowl = 0.0f;
                if (t < FloorStart)
                {
                    // Zona plana del suelo
                    Bowl = 1.0f;
                }
                else
                {
                    // Transición suave del suelo hacia el borde: parábola invertida
                    const float tNorm = (t - FloorStart) / (1.0f - FloorStart); // [0,1] desde el suelo hasta el borde
                    const float S = FMath::SmoothStep(0.0f, 1.0f, tNorm);
                    Bowl = FMath::Pow(1.0f - S, 2.0f);
                }

                CraterShape -= Bowl * CurrentDepth; // negativo = se hunde
            }

            // --- RIM (borde, campana gaussiana centrada en t=1) ---
            // CraterRimSharpness controla cuán estrecho es el borde
            // CraterRimHeight escala su altura relativa a la profundidad
            const float RimExponent = FMath::Pow((t - 1.0f) / 0.15f, 2.0f) * CraterParameters.CraterRimSharpness;
            const float Rim = FMath::Exp(-RimExponent);
            CraterShape += Rim * CurrentDepth * CraterParameters.CraterRimHeight;

            FinalCraterHeight += CraterShape;
        }

        FreqScale *= CraterParameters.CraterLacunarity;
        AmpScale *= CraterParameters.CraterPersistence;
    }

    Height += FinalCraterHeight;

    // --- HUMEDAD ---
    const float RawHum = HumidityNoise.GetNoise(X, Y, Z);
    float Humidity = (RawHum + 1.0f) * 0.5f; // [0, 1]
    Humidity = FMath::Clamp(
        (Humidity + BiomeParameters.HumidityOffset - 0.5f) * BiomeParameters.HumidityContrast + 0.5f,
        0.0f, 1.0f
    );

    // --- TEMPERATURA ---
    const float Latitude = FMath::Abs(Z);
    const float BaseTemp = 1.0f - (Latitude * BiomeParameters.LatitudeEffect);
    const float TempNoisVal = TempNoise.GetNoise(X, Y, Z) * 0.2f;
    const float Temperature = FMath::Clamp(BaseTemp + TempNoisVal, 0.0f, 1.0f);

    // --- MODIFICACIÓN POR BIOMA ---
    const float BiomeInfluence = FMath::Lerp(0.8f, 1.2f, Humidity);
    Height *= BiomeInfluence;

    // --- SALIDA ---
    const float AltitudeNormalized = FMath::Clamp(
        Height / FMath::Max(LayerParameters.Amplitude, 1.0f),
        0.0f, 1.0f
    );
    const float VisualTemp = FMath::Clamp(
        Temperature - (AltitudeNormalized * BiomeParameters.AltitudeTemperaturePenalty),
        0.0f, 1.0f
    );

    OutHeight = Height;
    OutColor = FLinearColor(AltitudeNormalized, VisualTemp, Humidity, 0.0f);
}
