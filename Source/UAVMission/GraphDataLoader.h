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

	//mang chua dinh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Graph")
	TArray<FVertexData> Vertices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graph|Visual")
	TObjectPtr<UStaticMesh> VertexMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graph|Visual")
	float VertexScale = 3000.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Graph")
	TArray<FEdgeData> Edges;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graph|Visual")
	FLinearColor EdgeColor = FLinearColor::Yellow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Graph")
	TArray<FUnitData> Units;
	 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graph|Visual")
	float UnitScale = 5000.0f;

	TMap<FString, float> ProbabilityMap;

	//Dữ liệu GA 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GA")
	TArray<FTargetData> Targets;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GA")
	TArray<FUAVData> UAVs;

	TArray<TArray<int32>> BestAssignment;
	float BestFitness = 0.0f;

	//hàm xử lý 

	UFUNCTION(BlueprintCallable, Category = "Graph")
	bool LoadVerticesFromCSV(const FString& FileName);

	UFUNCTION(BlueprintCallable, Category = "Graph")
	void SpawnVertexActors();

	UFUNCTION(BlueprintCallable, Category = "Graph")
	bool LoadEdgesFromCSV(const FString& FileName);

	UFUNCTION(BlueprintCallable, Category = "Graph")
	void DrawEdges();

	UFUNCTION(BlueprintCallable, Category = "Graph")
	bool LoadUnitsFromCSV(const FString& FileName);

	UFUNCTION(BlueprintCallable, Category = "Graph")
	void SpawnUnitActors();

	UFUNCTION(BlueprintCallable, Category = "Graph")
	int32 FindNearestVertexIndex(const FVector& Position) const;

	UFUNCTION(BlueprintCallable, Category = "Graph")
	void ConnectUnitsToGraph();

	UFUNCTION(BlueprintCallable, Category = "Graph")
	TArray<int32> FindShortestPath(int32 StartVertexId, int32 EndVertexId);

	UFUNCTION(BlueprintCallable, Category = "Graph")
	void DrawPath(const TArray<int32>& PathIds, FLinearColor Color, float Thickness);

	UFUNCTION(BlueprintCallable, Category = "GA")
	bool LoadTargetsFromCSV(const FString& FileName);

	UFUNCTION(BlueprintCallable, Category = "GA")
	bool LoadUAVsFromCSV(const FString& FileName);

	UFUNCTION(BlueprintCallable, Category = "GA")
	bool LoadProbabilityFromCSV(const FString& FileName);

	// Lấy xác suất p_ij
	float GetProbability(const FString& UavCode, int32 TargetId) const;

	UFUNCTION(BlueprintCallable, Category = "GA")
	void RunGA();

	UFUNCTION(BlueprintCallable, Category = "GA")
	void DrawAssignmentPaths();

private:
	// Tính fitness của 1 nghiệm
	float EvaluateFitness(const TArray<TArray<int32>>& X);

	// Sửa ràng buộc
	void RepairSolution(TArray<TArray<int32>>& X);

	// Khởi tạo 1 cá thể ngẫu nhiên
	TArray<TArray<int32>> RandomIndividual();

	UFUNCTION(BlueprintCallable, Category = "GA")
	void OptimizeSubsets();
};
