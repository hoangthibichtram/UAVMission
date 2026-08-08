#pragma once

#include "CoreMinimal.h"
#include "GraphTypes.generated.h"

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
	FVector Location;

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
    float Explosive = 0.0f;   

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
    float MilitaryValue = 0.0f;   // v_j: giá trị mục tiêu

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target")
    int32 VertexId = 0;       // đỉnh trong graph
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
    float Range = 0.0f;       // tầm bắn (cm)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UAV")
    float Speed = 0.0f;     

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UAV")
    float Explosive = 0.0f;  

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UAV")
    float MilitaryValue = 0.0f;

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
    float Speed = 300.f;           // toc do bay (cm/s)
};



