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









