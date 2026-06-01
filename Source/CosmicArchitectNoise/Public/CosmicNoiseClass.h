// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ICosmicNoiseStrategy.h"
#include "CosmicNoiseClass.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnNoiseSettingsChanged);

/**
 * Base abstracta para assets de configuración de ruido procedural.
 *
 * Permite crear estrategias de ruido a partir de datos editables en Unreal Engine
 * y notificar cambios en tiempo de edición.
 */
UCLASS(Abstract, BlueprintType)
class COSMICARCHITECTNOISE_API UCosmicNoiseClass : public UDataAsset
{
	GENERATED_BODY()
	
public: 
     /**
     * Evento que se dispara cuando se modifican propiedades del asset en el editor.
     */
	FOnNoiseSettingsChanged OnNoiseSettingsChanged;

    /**
     * Crea una instancia de la estrategia de ruido asociada a este asset.
     */
	virtual TSharedPtr<ICosmicNoiseStrategy> CreateStrategy() const;
protected:
#if WITH_EDITOR

    /**
     * Se ejecuta cuando una propiedad del asset es modificada en el editor.
     */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
