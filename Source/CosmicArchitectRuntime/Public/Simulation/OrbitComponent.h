// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OrbitComponent.generated.h"



UCLASS( ClassGroup=(Cosmic), meta=(BlueprintSpawnableComponent) )
class COSMICARCHITECTRUNTIME_API UOrbitComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UOrbitComponent();

#if WITH_EDITOR
	// Called when a property is changed in the editor
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* ParentBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orbit State")
	float CurrentOrbitTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params")
	float SemiMajorAxisKm = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params", meta = (ClampMin = "0", ClampMax = "1"))
	float Eccentricity = 0.0f;//0 círculo, 0.99 elipse extrema

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params")
	float OrbitalPeriod = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params", meta = (ClampMin = "0", ClampMax = "360"))
	float Inclination = 0.0f;
protected:
	bool bIsInEditorPreview = false;
	void UpdateInitialOrbitPosition();
private:

    // Función para generar/actualizar la visualización de la órbita
    void UpdateOrbitVisualization();

    // Número de segmentos para dibujar la órbita
    UPROPERTY(EditAnywhere, Category = "Orbit Visualization", meta = (ClampMin = "8", ClampMax = "360"))
    int32 OrbitSegments = 72;

    // Color de la órbita en el editor
    UPROPERTY(EditAnywhere, Category = "Orbit Visualization")
    FColor OrbitColor = FColor::White;

    // Grosor de la línea de la órbita
    UPROPERTY(EditAnywhere, Category = "Orbit Visualization", meta = (ClampMin = "0.1", ClampMax = "100000"))
    float OrbitThickness = 1000.0f;

    // Si mostrar la órbita en el editor
    UPROPERTY(EditAnywhere, Category = "Orbit Visualization")
    bool bShowOrbitInEditor = true;
};
