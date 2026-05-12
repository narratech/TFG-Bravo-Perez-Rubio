// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"


class  FCosmicFoliageCollectionActions : public FAssetTypeActions_Base
{
public:
    // El nombre que aparecerá en el menú
    virtual FText GetName() const override { return FText::FromString("Foliage Collection"); }

    // El color de la franja en la parte inferior del Asset
    virtual FColor GetTypeColor() const override { return FColor(45, 175, 255); } // Azul

    // A qué clase representa esta acción
    virtual UClass* GetSupportedClass() const override;

    // En qué categoría del menú aparecerá
    virtual uint32 GetCategories() override;

    // ID de la categoría personalizada
    EAssetTypeCategories::Type MyAssetCategory;

};
