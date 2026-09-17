// Fill out your copyright notice in the Description page of Project Settings.


#include "UAVPawn.h"
#include "Algo/Reverse.h"
#include "GraphDataLoader.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AUAVPawn::AUAVPawn()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// body chính
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

	// ===== PHASE SYSTEM =====
	if (bUsePhaseSystem && bFlying)
	{
		UpdateFlightPhase(DeltaTime * TimeScale);
		return;  // không chạy waypoint system
	}

	// ===== WAYPOINT SYSTEM (cũ) =====
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

	// Toi gan waypoint (< Threshold) -> chuyen sang waypoint ke tiep
	// Nguong = quang duong 1 frame x 2, toi thieu 5000 (50m)
	float Threshold = FMath::Max(MovementSpeed * DeltaTime * 2.0f, 5000.0f);
	if (FVector::Dist(GetActorLocation(), TargetLoc) <= Threshold)
		CurrentWaypointIndex++;
}

FString AUAVPawn::GetPhaseTypeName(EFlightPhaseType Type) const
{
	switch (Type)
	{
	case EFlightPhaseType::Climb:   return TEXT("CLIMB");
	case EFlightPhaseType::Cruise:  return TEXT("CRUISE");
	case EFlightPhaseType::Descent: return TEXT("DESCENT");
	case EFlightPhaseType::Attack:  return TEXT("ATTACK");
	case EFlightPhaseType::Return:  return TEXT("RETURN");
	default:                         return TEXT("UNKNOWN");
	}
}

void AUAVPawn::StartFlight(const FFlightPath& InPath)
{
	if (InPath.Phases.Num() > 0)
	{
		bUsePhaseSystem = true;
		CurrentFlightPath = InPath;
		CurrentPhaseIdx = 0;
		PhaseElapsedTime = 0.0f;

		// Đặt UAV ở vị trí xuất phát (căn cứ)
		SetActorLocation(InPath.StartUnitPos);
		PhaseStartPos = InPath.StartUnitPos;
		PhaseStartAltitude = 0.0f;

		bFlying = true;
		MissionState = EUAVMissionState::Flying;

		UE_LOG(LogTemp, Warning,
			TEXT("UAV %s START FLIGHT (PHASES) | %d phases"),
			*UAVCode, InPath.Phases.Num());
		return;
	}
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
		if (AGraphDataLoader* Loader = Cast<AGraphDataLoader>(
			GetWorld()->SpawnActor<AGraphDataLoader>()))
		{
			Loader->MarkTargetHit(GetActorLocation());  // Vẽ dấu X trên minimap
		}
		OnKamikazeExplode();
	}
	else {
		MissionState = EUAVMissionState::Attacking;
		AttackTimer = AttackDuration;// default=3.0
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

void AUAVPawn::UpdateFlightPhase(float DeltaTime)
{
	if (!bUsePhaseSystem || CurrentFlightPath.Phases.Num() == 0)
	{
		bFlying = false;
		return;
	}

	// Vượt quá phase cuối → kết thúc
	if (CurrentPhaseIdx >= CurrentFlightPath.Phases.Num())
	{
		bFlying = false;
		MissionState = EUAVMissionState::Finished;
		OnMissionFinished();
		UE_LOG(LogTemp, Warning, TEXT("UAV %s FLIGHT COMPLETE"), *UAVCode);
		return;
	}

	const FFlightPhase& Phase = CurrentFlightPath.Phases[CurrentPhaseIdx];
	PhaseElapsedTime += DeltaTime;

	// ========== ATTACK PHASE: Không di chuyển, chỉ bay qua + tấn công ==========
	if (Phase.PhaseType == EFlightPhaseType::Attack)
	{
		if (PhaseElapsedTime < DeltaTime * 1.1f)
		{
			MissionState = EUAVMissionState::Attacking;
			const FVector P = GetActorLocation();
			const float DatThat = GetGroundAltitudeAtLocation(P);
			UE_LOG(LogTemp, Warning,
				TEXT("[%s] ATTACK | UAV_Z=%.0f | Dat_duoi_UAV=%.0f | Chenh=%.0f | TargetPos_Z=%.0f"),
				*UAVCode, P.Z, DatThat, P.Z - DatThat, Phase.TargetPos.Z);
			if (bIsKamikaze)
			{
				// Vẽ dấu X trên minimap tại vị trí nổ (giống logic cũ trong OnReachDestination)
				if (AGraphDataLoader* Loader = Cast<AGraphDataLoader>(
					UGameplayStatics::GetActorOfClass(GetWorld(), AGraphDataLoader::StaticClass())))
				{
					Loader->MarkTargetHit(GetActorLocation());
				}
				OnKamikazeExplode();   // FIX: cảm tử phải nổ, không "attack" như chiến đấu
			}
			else
			{
				OnStartAttack();
			}
			UE_LOG(LogTemp, Warning, TEXT("[%s] ===== ATTACK START ====="), *UAVCode);
		}

		// ===== CHỜ ATTACK DURATION HẾT =====
		if (PhaseElapsedTime >= Phase.Duration)
		{
			// Attack xong → chuyển phase tiếp theo
			CurrentPhaseIdx++;
			PhaseElapsedTime = 0.0f;
			PhaseStartPos = GetActorLocation();
			PhaseStartAltitude = Phase.AltitudeAboveGround;
		}
		return;  // không di chuyển trong attack
	}

	// ========== CÁC PHASE KHÁC: Lerp từ vị trí hiện tại → TargetPos ==========
	FVector CurrentPos = GetActorLocation();
	float Progress = FMath::Clamp(PhaseElapsedTime / Phase.Duration, 0.0f, 1.0f);

	// Di chuyển smooth từ vị trí cũ → target
	FVector NextPos = FMath::Lerp(PhaseStartPos, Phase.TargetPos, Progress);

	// Điều chỉnh Z 
	float GroundAlt = NextPos.Z;
	float BaseAlt = FMath::Lerp(PhaseStartAltitude, Phase.AltitudeAboveGround, Progress);
	float ArcAlt = Phase.ArcHeight * 4.0f * Progress * (1.0f - Progress);
	NextPos.Z = GroundAlt + BaseAlt + ArcAlt;

	SetActorLocation(NextPos);

	// Xoay UAV hướng về phía mục tiêu của phase
	FRotator TargetRot;

	if (bIsKamikaze)
	{
		FVector ToTarget = (NextPos - CurrentPos).GetSafeNormal();
		if (!ToTarget.IsNearlyZero())
		{
			TargetRot = ToTarget.Rotation();
		}
		else
		{
			TargetRot = GetActorRotation();
		}
	}
	else
	{
		FVector FlightDir = (NextPos - CurrentPos);
		FlightDir.Z = 0;  
		FlightDir = FlightDir.GetSafeNormal();

		if (!FlightDir.IsNearlyZero())
		{
			TargetRot = FlightDir.Rotation();
		}
		else
		{
			TargetRot = GetActorRotation();
		}
	}

	// ===== SMOOTH ROTATION =====
	FRotator SmoothRot = FMath::RInterpTo(
		GetActorRotation(), TargetRot, DeltaTime, 3.0f);
	SetActorRotation(SmoothRot);

	// Phase hoàn thành → chuyển sang phase tiếp theo
	if (Progress >= 1.0f)
	{
		CurrentPhaseIdx++;
		PhaseElapsedTime = 0.0f;
		PhaseStartPos = Phase.TargetPos;
		PhaseStartAltitude = Phase.AltitudeAboveGround;
		UE_LOG(LogTemp, Warning, TEXT("[%s] Phase %d complete → %d"),
			*UAVCode, CurrentPhaseIdx - 1, CurrentPhaseIdx);
	}
}

float AUAVPawn::GetGroundAltitudeAtLocation(const FVector& Location) const
{
	FHitResult HitResult;
	FVector Start = FVector(Location.X, Location.Y, 10000000.0f);   // 100km lên trời
	FVector End = FVector(Location.X, Location.Y, -10000000.0f);  // 100km xuống đất

	//FVector Start = Location + FVector(0, 0, 200000.0f);  // Bắt đầu từ cao
	//FVector End = Location - FVector(0, 0, 200000.0f);    // Bắn xuống

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(const_cast<AUAVPawn*>(this));

	if (GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_WorldStatic,
		QueryParams))
	{
		LastGoodGroundAlt = HitResult.ImpactPoint.Z;
		return LastGoodGroundAlt;
	}
	return LastGoodGroundAlt;
}
