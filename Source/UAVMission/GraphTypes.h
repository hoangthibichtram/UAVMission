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



