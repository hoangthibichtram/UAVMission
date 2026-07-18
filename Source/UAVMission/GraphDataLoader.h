// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GraphTypes.h"
#include "GraphDataLoader.generated.h"

UCLASS()
class UAVMISSION_API AGraphDataLoader : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGraphDataLoader();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	//virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Graph")
	TArray<FVertexData> Vertices;

	UFUNCTION(BlueprintCallable, Category = "Graph")
	bool LoadVerticesFromCSV(const FString& FileName);
};
