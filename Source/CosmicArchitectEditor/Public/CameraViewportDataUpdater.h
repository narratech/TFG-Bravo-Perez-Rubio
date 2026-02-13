// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TickableEditorObject.h"
#include "Templates/UniquePtr.h"

class FCameraViewportDataUpdater : public FTickableEditorObject
{
public:
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override;

private:
    void UpdateCameraViewport();
};
