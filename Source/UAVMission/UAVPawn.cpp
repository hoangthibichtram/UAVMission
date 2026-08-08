// Fill out your copyright notice in the Description page of Project Settings.


#include "UAVPawn.h"
#include "Algo/Reverse.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AUAVPawn::AUAVPawn()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Tao "o trong" mesh — chua gan hinh, Blueprint gan sau
	UAVMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UAVMesh"));
	RootComponent = UAVMesh;

	// Chua can va cham
	UAVMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

}

// Called when the game starts or when spawned
void AUAVPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame: Bộ điều khiển bay
void AUAVPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MissionState == EUAVMissionState::Attacking)
	{
		AttackTimer -= DeltaTime;
		if (AttackTimer <= 0.f)
			StartReturn();
		return;   // khong di chuyen trong luc danh
	}

	if (!bFlying) return;

	// Da di het cac waypoint -> dừng
	if (CurrentWaypointIndex >= Waypoints.Num())
	{
		OnReachDestination();
		return;
	}

	FVector CurrentLoc = GetActorLocation();//vtri hiện tại của UAv
	FVector TargetLoc = Waypoints[CurrentWaypointIndex];//điểm kế tiếp

	// Hướng từ chỗ đang đứng -> waypoint dang tiến đến
	FVector Dir = (TargetLoc - CurrentLoc).GetSafeNormal();

	// Xoay mui UAV ve huong bay
	if (!Dir.IsNearlyZero())
	{
		FRotator Smooth = FMath::RInterpTo(
			GetActorRotation(), Dir.Rotation(), DeltaTime, 5.0f);
		SetActorRotation(Smooth);
	}

	// Tien thang ve phia truoc
	FVector NewLoc = CurrentLoc + Dir * (MovementSpeed * DeltaTime);
	SetActorLocation(NewLoc);

	// Toi gan waypoint (< 300) -> chuyen sang waypoint ke tiep
	if (FVector::Dist(GetActorLocation(), TargetLoc) <= 300.0f)
		CurrentWaypointIndex++;
}

void AUAVPawn::StartFlight(const FFlightPath& InPath)
{
	Waypoints = InPath.Points;
	float SpeedKmh = (InPath.Speed > 0.f ? InPath.Speed : 100.f);
	MovementSpeed = SpeedKmh / 3.6f * 100.f * TimeScale;

	if (Waypoints.Num() < 2)
	{
		bFlying = false;
		return;
	}

	SetActorLocation(Waypoints[0]);//đặt UAV ở điểm XP(0)
	CurrentWaypointIndex = 1;//bắt đầu đi đến điểm 1
	bFlying = true;//Start
}

void AUAVPawn::OnReachDestination() {
	if (MissionState == EUAVMissionState::Returning) {
		bFlying = false;
		MissionState = EUAVMissionState::Finished;
		OnMissionFinished();
		return;
	}

	if (bIsKamikaze) {
		bFlying = false;
		MissionState = EUAVMissionState::Finished;
		OnKamikazeExplode();
	}
	else {
		MissionState = EUAVMissionState::Attacking;
		AttackTimer = AttackDuration;
		OnStartAttack();
	}
}

void AUAVPawn::StartReturn()
{
	Algo::Reverse(Waypoints);      // dao thu tu waypoint
	CurrentWaypointIndex = 1;
	MissionState = EUAVMissionState::Returning;
	bFlying = true;
}


