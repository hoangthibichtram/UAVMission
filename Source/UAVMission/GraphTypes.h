#pragma once

#include "CoreMinimal.h"

#include "GraphTypes.generated.h"

class AUAVPawn;

USTRUCT(BlueprintType)
struct FVertexData
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vertex")
	int32 Id = 0;

	//vtri(x,y,z)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vertex")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vertex")
	FString TypeVertex;
};

USTRUCT (BlueprintType)
struct FEdgeData {
	GENERATED_BODY()

	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Edge");
	int32 StartId = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Edge")
	int32 EndId = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Edge")
	float Weight = 0.0f;
};

USTRUCT (BlueprintType)
struct FUnitData {
	GENERATED_BODY();
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = "Unit")
	FString UnitId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
	FString UnitName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    int32 VertexId = -1;   
};

USTRUCT(BlueprintType)
struct FTargetData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
    int32 TargetId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
    FString Code;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
    int32 Priority = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
    float ExplosiveReq = 0.0f;   

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
    float ValueUSD = 0.0f;   // v_j: giá trị mục tiêu

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
    int32 VertexId = 0;       // đỉnh trong graph

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
    float GroundAltitude = 0.0f;   // cao do mat dat (m)
};

USTRUCT(BlueprintType)
struct FUAVData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UAV")
    int32 UavId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UAV")
    FString UavCode;          // B11, C11... — khóa tra Probability

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UAV")
    FString UavType;   

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UAV")
    int32 Quantity = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UAV")
    float Range = 0.0f;       // tầm bay (m)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UAV")
    float Speed = 0.0f;     

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UAV")
    float Explosive = 0.0f;  

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UAV")
    float CostUSD = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UAV")
    float Budget = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UAV")
    FString UnitId;          
};

//DashBoard

// ===== Panel 2: một dòng phân công (UAV nào → mục tiêu nào) =====
USTRUCT(BlueprintType)
struct FAssignmentRow
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Dashboard")
    FString UAVCode;          // vd "C32"

    UPROPERTY(BlueprintReadOnly, Category = "Dashboard")
    FString TargetName;       // vd "So chi huy"

    UPROPERTY(BlueprintReadOnly, Category = "Dashboard")
    float KillProb = 0.f;     // xac suat tieu diet cua cap nay (0..1)
};

// ===== Panel 3: một mục tiêu + % tiêu diệt (cho biểu đồ 100%) =====
USTRUCT(BlueprintType)
struct FTargetResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Dashboard")
    FString TargetName;

    UPROPERTY(BlueprintReadOnly, Category = "Dashboard")
    float DestroyPercent = 0.f;  // % tieu diet (0..100)  -> phan MAU DO

    UPROPERTY(BlueprintReadOnly, Category = "Dashboard")
    int32 Priority = 1;          // do uu tien, de sau nay to mau
};

// ===== Gói tổng: gom hết số liệu 4 panel vào một chỗ =====
USTRUCT(BlueprintType)
struct FDashboardData
{
    GENERATED_BODY()

    // -- Panel 1: Tong quan --
    UPROPERTY(BlueprintReadOnly, Category = "Dashboard")
    float FitnessPercent = 0.f;       // vd 86.2

    UPROPERTY(BlueprintReadOnly, Category = "Dashboard")
    int32 TargetsCovered = 0;         // so muc tieu đủ nổ

    UPROPERTY(BlueprintReadOnly, Category = "Dashboard")
    int32 TargetsTotal = 0;

    // -- Panel 2: Bang phan cong --
    UPROPERTY(BlueprintReadOnly, Category = "Dashboard")
    TArray<FAssignmentRow> Assignments;

    // -- Panel 3: Muc tieu diet tung muc tieu --
    UPROPERTY(BlueprintReadOnly, Category = "Dashboard")
    TArray<FTargetResult> TargetResults;

    // -- Panel 4: Chi phi / loi ich --
    UPROPERTY(BlueprintReadOnly, Category = "Dashboard")
    float DestroyedValue = 0.f;       // tong gia tri dich bi tieu diet

    UPROPERTY(BlueprintReadOnly, Category = "Dashboard")
    float OurCost = 0.f;              // tong chi phi UAV ta dung

    UPROPERTY(BlueprintReadOnly, Category = "Dashboard")
    float NetBenefit = 0.f;           // DestroyedValue - OurCost: lợi ích ròng.
};

UENUM(BlueprintType)
enum class EFlightPhaseType : uint8
{
    Climb   UMETA(DisplayName = "Bay len"),
    Cruise  UMETA(DisplayName = "Bay on dinh"),
    Descent UMETA(DisplayName = "Ha xuong"),
    Attack  UMETA(DisplayName = "Tan cong"),
    Return  UMETA(DisplayName = "Quay ve")
};

USTRUCT(BlueprintType)
struct FFlightPhase
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Flight")
    FVector TargetPos = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Flight")
    float Duration = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Flight")
    float AltitudeAboveGround = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Flight")
    float ArcHeight = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Flight")
    EFlightPhaseType PhaseType = EFlightPhaseType::Climb;
};


// ===== Mot duong bay hoan chinh cua 1 UAV =====
USTRUCT(BlueprintType)
struct FFlightPath
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Flight")
    FString UAVCode;               // UAV nao bay

    UPROPERTY(BlueprintReadOnly, Category = "Flight")
    FString TargetName;            // bay toi muc tieu nao

    UPROPERTY(BlueprintReadOnly, Category = "Flight")
    TArray<FVector> Points;        // chuoi diem toa do (duong bay)

    UPROPERTY(BlueprintReadOnly, Category = "Flight")
    FLinearColor Color = FLinearColor::White;   // mau theo loai UAV

    UPROPERTY(BlueprintReadOnly, Category = "Flight")
    float Speed = 300.f;

    UPROPERTY()
    FVector StartUnitPos;       // Vị trí xuất phát từ đơn vị

    UPROPERTY()
    FVector TargetPos;          // Vị trí mục tiêu

    UPROPERTY()
    float CruiseAltitude = 5000.0f;      // Độ cao bay ổn định (cm)

    UPROPERTY()
    float AttackDistance = 10000.0f;     // Cách mục tiêu bao xa khi bắt đầu hạ (cm)

    UPROPERTY()
    float AttackAltitude = 1000.0f;      // Độ cao khi tấn công (cm)

    UPROPERTY()
    bool bIsKamikaze = false;            // Cảm tử đây?

    UPROPERTY()
    TArray<FFlightPhase> Phases;         // Các phase bay

};


UENUM(BlueprintType)
enum class EMarkerType : uint8
{
    Kamikaze   UMETA(DisplayName = "Cam tu"),
    Combat     UMETA(DisplayName = "Chien dau"),
    Fire       UMETA(DisplayName = "Muc tieu bi tan cong"),
    Destroyed  UMETA(DisplayName = "Da pha huy")
};

USTRUCT(BlueprintType)
struct FMinimapMarker
{
    GENERATED_BODY()

    // Vi tri icon tren minimap, chuan hoa 0..1 (X = ngang, Y = doc)
    UPROPERTY(BlueprintReadOnly)
    FVector2D MapPos = FVector2D::ZeroVector;

    // Huong mui UAV, do (0 = Bac)
    UPROPERTY(BlueprintReadOnly)
    float Heading = 0.f;

    UPROPERTY(BlueprintReadOnly)
    EMarkerType Type = EMarkerType::Combat;

    UPROPERTY(BlueprintReadOnly)
    FString UAVCode;

    // Tham chieu de bam theo; null neu la dau X
    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<AUAVPawn> UAVRef = nullptr;
};



// === Dữ liệu vẽ trên minimap ===
USTRUCT(BlueprintType)
struct FMinimapEdge
{
    GENERATED_BODY()

    UPROPERTY()
    FVector2D Start;  // Vị trí đầu (normalized [0,1])

    UPROPERTY()
    FVector2D End;    // Vị trí cuối

    UPROPERTY()
    FLinearColor Color = FLinearColor::White;

    UPROPERTY()
    float Thickness = 2.0f;
};

USTRUCT(BlueprintType)
struct FMinimapObject
{
    GENERATED_BODY()

    UPROPERTY()
    FVector2D Position;    // Vị trí trên minimap [0,1]

    UPROPERTY()
    FString Name;

    UPROPERTY()
    FString Type;          // "Target", "UAV", "Unit"

    UPROPERTY()
    FLinearColor Color;

    UPROPERTY()
    float Size = 10.0f;    // Kích thước icon (pixel)
};

// === Dữ liệu tổng hợp gửi tới minimap ===
USTRUCT(BlueprintType)
struct FMinimapData
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMinimapEdge> GraphEdges;

    UPROPERTY()
    TArray<FMinimapObject> Objects;  // Targets, UAVs, Units
};