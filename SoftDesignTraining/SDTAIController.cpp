#include "SDTAIController.h"
#include "SDTCollectible.h"
#include "SoftDesignTraining.h"
#include "Engine/StaticMeshActor.h"
#include "SDTUtils.h"
#include "Engine/OverlapResult.h"

void ASDTAIController::Tick(float deltaTime)
{
	bool hasHit = false;
	if (APawn* const pawn = GetPawn()) {
		FVector castStart = pawn->GetActorLocation();
		FVector castEnd = pawn->GetActorLocation() + pawn->GetActorForwardVector() * castLength;
		ASoftDesignTrainingMainCharacter* PlayerCharacter = Cast<ASoftDesignTrainingMainCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

		
		FHitResult deathHits;
		FHitResult collectibleHits;


		FCollisionObjectQueryParams objectQueryParams;
		objectQueryParams.AddObjectTypesToQuery(COLLISION_DEATH_OBJECT);

		
		bool bCollectibleHit = CastForCollectible(collectibleHits);
		bool bDeathHit = GetWorld()->SweepSingleByObjectType(deathHits, castStart, castEnd, FQuat::Identity, objectQueryParams, FCollisionShape::MakeCapsule(pawn->GetSimpleCollisionRadius(), pawn->GetDefaultHalfHeight()));
		

		if (bDeathHit && deathHits.GetComponent()->GetCollisionObjectType() == COLLISION_DEATH_OBJECT && deathHits.Distance <= 500.f) {
			Deccelerate(0.95);
			FindNewPath(deltaTime, true);

		} 
	
		else if (PlayerCharacter && SphereCastForPlayer(PlayerCharacter) && CanMoveToPlayer(PlayerCharacter)) {
			FVector steeringForce = GetSteeringForce(PlayerCharacter);
			FVector moveDirection = steeringForce * MaxSpeed;
			FRotator desiredRotation;

			
			if (!(PlayerCharacter->IsPoweredUp())) { 
				desiredRotation = FRotationMatrix::MakeFromX(PlayerCharacter->GetActorLocation() - pawn->GetActorLocation()).Rotator();
				RotatePawn(desiredRotation, deltaTime, DefaultRotationSpeed);
			}
			else {
				desiredRotation = FRotationMatrix::MakeFromX(pawn->GetActorLocation() - PlayerCharacter->GetActorLocation()).Rotator();
				RotatePawn(desiredRotation, deltaTime, DefaultRotationSpeed);
				FindNewPath(deltaTime);
			}
			Accelerate(deltaTime);
		}

		else if (bCollectibleHit && collectibleHits.GetComponent()->IsVisible() && CanMoveToCollectible(collectibleHits)) {
			FVector inBetweenVector = FVector(
				collectibleHits.GetActor()->GetActorLocation().X - pawn->GetActorLocation().X,
				collectibleHits.GetActor()->GetActorLocation().Y - pawn->GetActorLocation().Y,
				0
			);

			FVector desiredVelocity = inBetweenVector.GetSafeNormal() * CurrentSpeed;
			FVector steeringForce = (desiredVelocity - pawn->GetVelocity());
			FVector moveDirection = steeringForce * MaxSpeed;
			FRotator desiredRotation = FRotationMatrix::MakeFromX(inBetweenVector).Rotator();
			RotatePawn(desiredRotation, deltaTime, DefaultRotationSpeed);
			Accelerate(deltaTime);
		} 

	
		else {
			TArray<struct FHitResult> hits;
			FCollisionResponseParams responseParams;
			responseParams.CollisionResponse.SetResponse(COLLISION_COLLECTIBLE, ECollisionResponse::ECR_Ignore);
			
	
			GetWorld()->SweepMultiByChannel(
				hits,
				castStart,
				castEnd,
				FQuat::Identity,
				ECC_WorldStatic,
				FCollisionShape::MakeCapsule(
					pawn->GetSimpleCollisionRadius(),
					pawn->GetDefaultHalfHeight() - 1
				),
				FCollisionQueryParams::DefaultQueryParam,
				responseParams
			);

			for (FHitResult& hit : hits) {
				AActor* actor = hit.GetActor();
				bool wallVerification = actor && actor->IsA<AStaticMeshActor>() && hit.Distance < 250.f;
				if (wallVerification) {
					Deccelerate(0.98);
					FindNewPath(deltaTime);
					hasHit = true;
					break;
				}
			}
			if (!hasHit) {
				Accelerate(deltaTime);
			}
		}

		pawn->AddMovementInput(pawn->GetActorForwardVector(), CurrentSpeed / MaxSpeed);
	}
}


void ASDTAIController::Accelerate(float deltaTime) {
	CurrentSpeed = FMath::Min(CurrentSpeed + Acceleration * deltaTime, MaxSpeed);
}


void ASDTAIController::Deccelerate(float scale) {
	CurrentSpeed = FMath::Max(CurrentSpeed * scale, MaxSpeed / 5);
}


bool ASDTAIController::SphereCastForPlayer(ASoftDesignTrainingMainCharacter* PlayerCharacter) {
	if (APawn* const pawn = GetPawn()) {
		TArray<FOverlapResult> playerOverlaps;

		FVector pawnLocation = pawn->GetActorLocation();

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(pawn);

		bool bHit = GetWorld()->OverlapMultiByChannel(
			playerOverlaps,
			pawnLocation,
			FQuat::Identity,
			ECC_Pawn,
			FCollisionShape::MakeSphere(750.f),
			QueryParams
		);
		for (const FOverlapResult& overlap : playerOverlaps) {
			if (Cast<ASoftDesignTrainingMainCharacter>(overlap.GetActor()) == PlayerCharacter) {
				return true;
			}
		}
	}
	return false;
}


void ASDTAIController::FindNewPath(float deltaTime, bool exitBehind) {
	if (APawn* const pawn = GetPawn()) {
		FVector actorLocation = pawn->GetActorLocation();
		FVector actorFowardVector = pawn->GetActorForwardVector();
		
		TArray<float> noHitAngles;
		float furthestHitAngle;
		bool hasFurthestHitAngle = false;
		float maxHitDistance = -1.f;
		

		FCollisionObjectQueryParams queryParams;
		queryParams.AddObjectTypesToQuery(ECC_WorldStatic);
		queryParams.AddObjectTypesToQuery(COLLISION_DEATH_OBJECT);

	
		float verificationAngle = 90.f;
		for (float angle = -verificationAngle; angle <= verificationAngle; angle += 5.f) {
			FHitResult hitResult;
			bool bHit = GetWorld()->SweepSingleByObjectType(
				hitResult,
				actorLocation,
				actorLocation + actorFowardVector.RotateAngleAxis(angle, FVector::UpVector) * castLength,
				FQuat::Identity,
				queryParams,
				FCollisionShape::MakeCapsule(
					pawn->GetSimpleCollisionRadius(),
					pawn->GetDefaultHalfHeight() - 1
				)
			);
			
			if (bHit) {
				if (hitResult.GetComponent()->GetCollisionObjectType() != COLLISION_DEATH_OBJECT &&
					hitResult.GetActor()->IsA<AStaticMeshActor>() && 
					FVector::Dist(hitResult.ImpactPoint, actorLocation) > maxHitDistance
				) {
					furthestHitAngle = angle;
					hasFurthestHitAngle = true;
					maxHitDistance = FVector::Dist(hitResult.ImpactPoint, actorLocation);
				}
			}
			else {
				noHitAngles.Add(angle);
			}
		}
		
	
		if (!noHitAngles.IsEmpty()) {
			float angle = noHitAngles[FMath::RandRange(0, noHitAngles.Num() - 1)];
			RotatePawn(pawn->GetActorRotation() + FRotator(0.f, angle, 0.f), deltaTime, DefaultRotationSpeed);
		}
		
		else if (hasFurthestHitAngle) {
			RotatePawn(pawn->GetActorRotation() + FRotator(0.f, furthestHitAngle, 0.f), deltaTime, DefaultRotationSpeed);
		}
		else if (exitBehind) {
			RotatePawn(pawn->GetActorRotation() + FRotator(0.f, 180, 0.f), deltaTime, EmergencyRotationSpeed);
		}
	}
}



void ASDTAIController::RotatePawn(FRotator targetRotation, float deltaTime, float rotationSpeed) {
	if (APawn* const pawn = GetPawn()) {
		pawn->SetActorRotation(FMath::RInterpTo(pawn->GetActorRotation(), targetRotation, deltaTime, rotationSpeed));
	}
}


float ASDTAIController::GetAngle(FVector v1, FVector v2) {
	return FMath::RadiansToDegrees(acos(FVector::DotProduct(v1.GetSafeNormal(), v2.GetSafeNormal())));
}


FVector ASDTAIController::GetSteeringForce(ASoftDesignTrainingMainCharacter* PlayerCharacter) {
	if (APawn* const pawn = GetPawn()) {
		FVector desiredVelocity = (PlayerCharacter->GetActorLocation() - pawn->GetActorLocation()).GetSafeNormal() * CurrentSpeed;
		FVector steeringForce = (desiredVelocity - pawn->GetVelocity());

		return steeringForce;
	}
	return FVector::ZeroVector;
}

bool ASDTAIController::CanMoveToPlayer(ASoftDesignTrainingMainCharacter* PlayerCharacter) {
	if (APawn* const pawn = GetPawn()) {
		FVector playerLocation = PlayerCharacter->GetActorLocation();
		FVector pawnLocation = pawn->GetActorLocation();

		FHitResult hitResult;
		
		FCollisionObjectQueryParams queryParams;
		queryParams.AddObjectTypesToQuery(ECC_WorldStatic);
		queryParams.AddObjectTypesToQuery(COLLISION_DEATH_OBJECT);

		bool isThereObstacle = GetWorld()->SweepSingleByObjectType(
			hitResult,
			pawnLocation,
			playerLocation,
			FQuat::Identity,
			queryParams,
			FCollisionShape::MakeCapsule(
				pawn->GetSimpleCollisionRadius(),
				pawn->GetDefaultHalfHeight() - 1
			)
		);

		return !isThereObstacle;

	}
	return false;
}

bool ASDTAIController::CastForCollectible(FHitResult& hit) {
	if (APawn* pawn = GetPawn()) {
		FVector pawnLocation = pawn->GetActorLocation();
		FCollisionObjectQueryParams queryParams;
		queryParams.AddObjectTypesToQuery(COLLISION_COLLECTIBLE);
		return GetWorld()->SweepSingleByObjectType(
			hit,
			pawnLocation,
			pawnLocation,
			FQuat::Identity,
			queryParams,
			FCollisionShape::MakeSphere(350.f)
		);
	}
	return false;
}

bool ASDTAIController::CanMoveToCollectible(FHitResult hit)
{
	if (APawn* pawn = GetPawn()) {

		FHitResult outHit;
		FCollisionObjectQueryParams queryParams;
		queryParams.AddObjectTypesToQuery(ECC_WorldStatic);
		queryParams.AddObjectTypesToQuery(COLLISION_COLLECTIBLE);
		bool bHit = GetWorld()->SweepSingleByObjectType(
			outHit,
			pawn->GetActorLocation(),
			hit.GetActor()->GetActorLocation(),
			FQuat::Identity,
			queryParams,
			FCollisionShape::MakeCapsule(
				pawn->GetSimpleCollisionRadius(),
				pawn->GetDefaultHalfHeight() - 1
			)
		);
		if (bHit) {
			return outHit.GetComponent()->GetCollisionObjectType() == COLLISION_COLLECTIBLE;
		}
		return false;
	}
	return false;
}