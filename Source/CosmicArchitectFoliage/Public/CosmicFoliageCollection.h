// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CosmicFoliageTypes.h"
#include "CosmicFoliageCollection.generated.h"

/**
 * Delegado que se ejecuta cuando la colección de foliage es modificada.
 */
DECLARE_MULTICAST_DELEGATE(FOnFoliageCollectionChanged);

/**
 * DataAsset que define una colección de foliage utilizada en la generación de planetas.
 *
 * Permite agrupar múltiples entradas de foliage y notificar cambios en editor.
 */
UCLASS(BlueprintType)
class COSMICARCHITECTFOLIAGE_API UCosmicFoliageCollection : public UDataAsset
{ 
	GENERATED_BODY()

public:

	/**
	 * Lista de entradas de foliage que componen esta colección.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foliage")
	TArray<FCosmicFoliageCollectionEntry> FoliageEntries;

	/**
	 * Evento que se dispara cuando la colección cambia.
	 */
	FOnFoliageCollectionChanged OnFoliageCollectionChanged;

protected:

#if WITH_EDITOR

	/**
	 * Callback del editor cuando una propiedad del asset es modificada.
	 *
	 * Se usa para detectar cambios en la colección y notificar a sistemas dependientes.
	 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif
};