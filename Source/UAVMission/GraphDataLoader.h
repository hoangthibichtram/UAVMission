// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UAVPawn.h"
#include "GraphTypes.h"
#include "Blueprint/UserWidget.h"
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
	virtual void Tick(float DeltaTime) override;

	//mang chua dinh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Graph")
	TArray<FVertexData> Vertices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graph|Visual")
	TObjectPtr<UStaticMesh> VertexMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graph|Visual")
	float VertexScale = 30000.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Graph")
	TArray<FEdgeData> Edges;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graph|Visual")
	FLinearColor EdgeColor = FLinearColor::Yellow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Graph")
	TArray<FUnitData> Units;

	UPROPERTY(EditAnywhere, Category = "Graph")
	float GraphHeightOffset = -20000.0f;  
	 
	TMap<FString, float> ProbabilityMap;

	//Dữ liệu GA 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GA")
	TArray<FTargetData> Targets;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GA")
	TArray<FUAVData> UAVs;

	//hằng số k
	UPROPERTY(EditAnywhere, Category = "GA")
	float CostCoefK = 0.5f; // USD/km

	TArray<TArray<int32>> BestAssignment;
	float BestFitness = 0.0f;

	TArray<TArray<bool>> Reachable;   // Reachable[i][j] = UAV i có tới được mục tiêu j không?

	// ===== Dashboard =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dashboard")
	TSubclassOf<UUserWidget> DashboardWidgetClass;

	UPROPERTY()
	UUserWidget* DashboardWidget = nullptr;

	bool bDashboardVisible = false;

	// Danh sach tat ca duong bay (luu de UAV bay theo)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight")
	TArray<FFlightPath> FlightPaths;

	//BP
	UPROPERTY(EditAnywhere, BluePrintReadWrite, Category = "Flight")
	TSubclassOf<AUAVPawn> KamikazeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	TSubclassOf<AUAVPawn> CombatClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	TMap<FString, TSubclassOf<AUAVPawn>> UAVClassByCode;

	//Hệ tọa độ
	UPROPERTY(EditAnywhere, Category = "Georeference")
	double OriginNorthing = 1436888.0;    // X0 (m)

	UPROPERTY(EditAnywhere, Category = "Georeference")
	double OriginEasting = 506661.0;     // Y0 (m)

	// 1 met that = bao nhieu don vi UE(~cm) (100 = ti le 1:1): vì đơn vị trong UE là cm
	UPROPERTY(EditAnywhere, Category = "Georeference")
	float MetersToUE = 100.f;

	// ===== Minimap =====
	// Pham vi ban do (don vi UE)
	UPROPERTY(EditAnywhere, Category = "Minimap")
	float MapExtentNorth = 1880000.f;   // nửa chieu Bac-Nam

	UPROPERTY(EditAnywhere, Category = "Minimap")
	float MapExtentEast = 1875000.f;    //  nửa chieu Dong-Tay

	// Danh sach UAV dang hoat dong
	UPROPERTY(BlueprintReadOnly, Category = "Minimap")
	TArray<AUAVPawn*> ActiveUAVs;

	// Vi tri cac UAV cam tu da no (de ve dau X)
	UPROPERTY(BlueprintReadOnly, Category = "Minimap")
	TArray<FVector> DestroyedPositions;

	// Vi tri cac muc tieu da bi tan cong (de ve icon lua)
	UPROPERTY(BlueprintReadOnly, Category = "Minimap")
	TArray<FVector> BurningTargets;

	// Widget minimap se duoc tao khi bat dau
	UPROPERTY(EditAnywhere, Category = "Minimap")
	TSubclassOf<UUserWidget> MinimapWidgetClass;

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

	TArray<int32> FindShortestPath(int32 StartVertexId, int32 EndVertexId, float* OutDistance = nullptr);// trả về đường đi và độ dài đường đi.

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

	// Tinh chi phi c_ij = cost_i + k * L_ij/1000
	float ComputeCostIJ(int32 i, int32 j);

	// Tinh tong weight (met) cua mot duong di
	float PathLength(const TArray<int32>& Path);
	
	//DashBoard
	void ToggleDashboard();

	UFUNCTION(BlueprintCallable, Category = "Dashboard")
	FDashboardData GetDashboardData() const;

	//BP
	UFUNCTION(BlueprintCallable, Category = "Flight")
	void SpawnUAVs();

	//minimap
	// Ham UMG goi moi khung hinh de lay ky hieu
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	TArray<FMinimapMarker> GetMinimapMarkers();

	// Chuyen toa do the gioi -> toa do minimap 0..1
	UFUNCTION(BlueprintPure, Category = "Minimap")
	FVector2D WorldToMinimap(const FVector& WorldPos) const;

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void MarkTargetHit(const FVector& TargetPos);

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	FMinimapData GetMinimapData();

	float GetGroundZAt(const FVector& Location) const;

	void DrawSavedAssignmentPaths();

private:
	// Tính fitness của 1 nghiệm
	float EvaluateFitness(const TArray<TArray<int32>>& X);

	// Sửa ràng buộc
	void RepairSolution(TArray<TArray<int32>>& X);

	// Khởi tạo 1 cá thể ngẫu nhiên
	TArray<TArray<int32>> RandomIndividual();

	//Tính sẵn ma trận Reachable một lần trước khi chạy GA
	void BuildReachability();

	UFUNCTION(BlueprintCallable, Category = "GA")
	void OptimizeSubsets();

	UPROPERTY()
	TArray<AStaticMeshActor*> VertexActors;

	// Vi tri lan cuoi cua tung UAV, de biet no no o dau
	TMap<AUAVPawn*, FVector> LastKnownPos;

	bool bGraphVisible = true;  // Mặc định Graph hiển thị
	bool bVertexSpheresVisible = true;

	void ToggleGraph();         // Hàm toggle

	void BuildFlightPhases(FFlightPath& Path, const TArray<int32>& Waypoints);
	FVector FindVertexLocation(int32 VertexId) const;

	float CalculatePathDistance(const TArray<int32>& Path) const;
};
