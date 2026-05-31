

#include "CosmicNoiseDSettingsFactory.h"

#include "CosmicDefaultNoiseSettings.h"

UCosmicNoiseDSettingsFactory::UCosmicNoiseDSettingsFactory()
{
    SupportedClass = UCosmicDefaultNoiseSettings::StaticClass();

    // Permitimos crear objetos nuevos desde cero (no solo importando)
    bCreateNew = true;

    //Abre automáticamente el panel de detalles al crearlo
    bEditAfterNew = true;
}

UObject* UCosmicNoiseDSettingsFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    // Usamos NewObject para crear la instancia real en la memoria de Unreal
    UCosmicDefaultNoiseSettings* NewNoiseSettings = NewObject<UCosmicDefaultNoiseSettings>(InParent, InClass, InName, Flags | RF_Transactional);

    // Inicializar valores por defecto
    // ej: NewNoiseSettings->Params.Seed = FMath::RandRange(1000, 9999);
     
    return NewNoiseSettings;
}

bool UCosmicNoiseDSettingsFactory::ShouldShowInNewMenu() const
{
    return true; // Para que aparezca en el menú de "Miscellaneous" por defecto
}
