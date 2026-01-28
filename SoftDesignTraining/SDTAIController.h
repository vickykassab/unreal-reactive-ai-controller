// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SoftDesignTrainingMainCharacter.h"

#include "SDTAIController.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = AI, config = Game)
class SOFTDESIGNTRAINING_API ASDTAIController : public AAIController
{
    GENERATED_BODY()
public:
    virtual void Tick(float deltaTime) override;
    //UPROPERTY angle/sec

protected:
    UPROPERTY(EditAnywhere, Category = AIProperty)
    int castLength = 1000;
    UPROPERTY(EditAnywhere, Category = AIProperty)
    float CurrentSpeed = 0.f;
    UPROPERTY(EditAnywhere, Category = AIProperty)
    float MaxSpeed = 1000.0f;
    UPROPERTY(EditAnywhere, Category = AIProperty)
    float Acceleration = 200.0f;
    UPROPERTY(EditAnywhere, Category = AIProperty)
    float DefaultRotationSpeed = 10.f;
    UPROPERTY(EditAnywhere, Category = AIProperty)
    float EmergencyRotationSpeed = 180.f;

    FVector GetSteeringForce(ASoftDesignTrainingMainCharacter* PlayerCharacter);
    bool foundCollectible = false;

    void Accelerate(float deltaTime);
    void Deccelerate(float scale);
    bool SphereCastForPlayer(ASoftDesignTrainingMainCharacter* PlayerCharacter);
    void FindNewPath(float deltaTime, bool exitBehind = false);
    void RotatePawn(FRotator targetRotation, float deltaTime, float rotationSpeed);
    float GetAngle(FVector v1, FVector v2);

    bool CanMoveToPlayer(ASoftDesignTrainingMainCharacter* PlayerCharacter);
    
    bool CastForCollectible(FHitResult& hit);
    bool CanMoveToCollectible(FHitResult hit);
};
