//#include "CosmicArchitectNoiseGenerator.h"
//
//void FCosmicArchitectNoiseGenerator::DoWork() {
//	FastNoiseLite Noise;
//    Noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
//    Noise.SetSeed(1337); // O un seed que pases por el constructor
//
//    for (int32 y = 0; y < Size; y++)
//    {
//        for (int32 x = 0; x < Size; x++)
//        {
//            // Lógica de muestreo esférico para evitar distorsión en planetas
//            FVector LocalPos = Offset + FVector(x, y, 0);
//            FVector NormalPos = LocalPos.GetSafeNormal();
//
//            // Guardamos el ruido en el array referenciado
//            OutHeights[x + y * Size] = Noise.GetNoise(
//                (float)(NormalPos.X * Scale),
//                (float)(NormalPos.Y * Scale),
//                (float)(NormalPos.Z * Scale)
//            );
//        }
//    }
//    
//}