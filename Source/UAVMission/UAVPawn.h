// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GraphTypes.h"
#include "UAVPawn.generated.h"

UENUM(BlueprintType)
enum class EUAVMissionState : uint8
{
	Idle       UMETA(DisplayName = "Idle"),
	Flying     UMETA(DisplayName = "Flying"),
	Attacking  UMETA(DisplayName = "Attacking"),
	Returning  UMETA(DisplayName = "Returning"),
	Finished   UMETA(DisplayName = "Finished")
};

UCLASS()
class UAVMISSION_API AUAVPawn : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AUAVPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	float AttackTimer = 0.f;//bộ đếm thời gian
	void OnReachDestination(); 
	void StartReturn();

public:
	// rỗng trong mesh — Blueprint se gan hinh UAV vao
	UPROPERTY(VisibleAnywhere, Category = "UAV")
	UStaticMeshComponent* UAVMesh;

	// Trang thai nhiem vu
	UPROPERTY(BlueprintReadOnly, Category = "UAV")
	EUAVMissionState MissionState = EUAVMissionState::Idle;

	// true = cam tu (B), false = chien dau (C)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UAV")
	bool bIsKamikaze = false;

	// Thoi gian danh muc tieu (giay)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UAV")
	float AttackDuration = 3.0f;

	// He so tang toc thoi gian mo phong (1 = thoi gian thuc)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UAV")
	float TimeScale = 20.f;

	// Nhan duong bay va bat dau bay
	void StartFlight(const FFlightPath& InPath);

	// Cac su kien de Blueprint hook hieu ung (Lop D)
	UFUNCTION(BlueprintImplementableEvent, Category = "UAV")
	void OnStartAttack();

	UFUNCTION(BlueprintImplementableEvent, Category = "UAV")
	void OnKamikazeExplode();

	UFUNCTION(BlueprintImplementableEvent, Category = "UAV")
	void OnMissionFinished();

private:
	TArray<FVector> Waypoints;      
	int32 CurrentWaypointIndex = 0; 
	float MovementSpeed = 500.f;     
	UPROPERTY(BlueprintReadOnly, Category = "UAV", meta = (AllowPrivateAccess = "true"))
	bool bFlying = false;           // dang bay hay da dung




};
