// Fill out your copyright notice in the Description page of Project Settings.


#include "GraphDataLoader.h"
#include "Misc/FileHelper.h"      
#include "Misc/Paths.h"  
#include "Engine/StaticMeshActor.h"   
#include "Components/StaticMeshComponent.h" 
#include "DrawDebugHelpers.h"

// Sets default values
AGraphDataLoader::AGraphDataLoader()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	//mặc định dùng Sphere mesh 
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		VertexMesh = SphereMesh.Object;
	}
}

// Called when the game starts or when spawned
void AGraphDataLoader::BeginPlay()
{
	Super::BeginPlay();
	//Đỉnh
	if (LoadVerticesFromCSV(TEXT("Vertex1.csv"))) {
		UE_LOG(LogTemp, Warning, TEXT(" === Đã đọc %d đỉnh từ file CSV === "),
			Vertices.Num());

		for (const FVertexData& V : Vertices) {
			UE_LOG(LogTemp, Warning, TEXT("Đỉnh %d tại (%.1f, %.1f, %.1f) loại=%s"),
			V.Id, V.Location.X, V.Location.Y, V.Location.Z, *V.TypeVertex);
		}
		//Spawn quả cầu tại mỗi đỉnh
		SpawnVertexActors();
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("LỖI: Không đọc được file CSV!"));
	}

	//Cạnh
	if (LoadEdgesFromCSV(TEXT("Edge1.csv"))) {
		UE_LOG(LogTemp, Warning, TEXT("=== Đã đọc %d cạnh ==="), Edges.Num());
		DrawEdges();
	}
	
	//Đơn vị
	if (LoadUnitsFromCSV(TEXT("UnitUAV1.csv"))) {
		UE_LOG(LogTemp, Warning, TEXT("=== Đã đọc %d đơn vị ==="), Units.Num());
		SpawnUnitActors();
		ConnectUnitsToGraph();
	}

	LoadTargetsFromCSV(TEXT("Data_target1.csv"));
	LoadUAVsFromCSV(TEXT("Data_uav1.csv"));
	LoadProbabilityFromCSV(TEXT("Probability1.csv"));

	UE_LOG(LogTemp, Warning, TEXT("=== GA Data: %d targets, %d UAVs, %d probs ==="),
		Targets.Num(), UAVs.Num(), ProbabilityMap.Num());
	RunGA();
	OptimizeSubsets();
	DrawAssignmentPaths();
}

//Hàm đọc file ĐỈNH
bool AGraphDataLoader::LoadVerticesFromCSV(const FString& FileName)
{
	//xoa du lieu cu ( neu goi lai)
	Vertices.Empty();
	FString FullPath = FPaths::ProjectContentDir() + TEXT("Data/") + FileName;

	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *FullPath)) {
		UE_LOG(LogTemp, Error, TEXT("Không tìm thấy file: %s"), *FullPath);
		return false;
	}
	//duyet tung dong(tru dong header)
	for (int32 i = 1; i < Lines.Num(); i++) {

		const FString& Line = Lines[i];
		if (Line.IsEmpty()) continue;

		TArray<FString> Cols;

		Line.ParseIntoArray(Cols, TEXT(","), false);

		if (Cols.Num() < 6) continue;
		//tao dinh moi
		FVertexData V;

		float X = FCString::Atof(*Cols[1]);
		float Y = FCString::Atof(*Cols[2]);
		float Z = FCString::Atof(*Cols[3]);
		//ghep X,Y,Z thanh location
		V.Location = FVector(X, Y, Z);
		V.Id = FCString::Atoi(*Cols[4]);
		V.TypeVertex = Cols[5].TrimStartAndEnd();
		Vertices.Add(V);
	}
	//tra ve true neu doc duoc it nhat 1 dinh
	return Vertices.Num() > 0;

}

//Hàm Spawn quả cầu=>duyệt từng vertex, sinnh ra AStaticMeshActor
void AGraphDataLoader::SpawnVertexActors() {
	UWorld* World = GetWorld();
	if (!World || !VertexMesh) return;

	for (const FVertexData& V : Vertices) {
		FTransform SpawnTransform;
		SpawnTransform.SetLocation(V.Location);
		SpawnTransform.SetScale3D(FVector(VertexScale / 100.0f));

		AStaticMeshActor* SphereActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), SpawnTransform);
		if (SphereActor) {
			//gan mesh sphere cho actor vua spawn
			SphereActor->GetStaticMeshComponent()->SetStaticMesh(VertexMesh);
			SphereActor->SetActorLabel(
				FString::Printf(TEXT("Vertex_%d_%s"), V.Id, *V.TypeVertex)
			);
			//tat collision de UAV co the bay xuyen qua dinh
			SphereActor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			UMaterial* BaseMat = LoadObject<UMaterial>(
				nullptr,
				TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")
			);
			if (BaseMat) {
				// Tạo instance động từ material gốc
					UMaterialInstanceDynamic * DynMat =
					UMaterialInstanceDynamic::Create(BaseMat, SphereActor);
				//chọn màu theo loại đỉnh
				FString Type = V.TypeVertex.TrimStartAndEnd().ToLower();
				FLinearColor Color;
				if (Type == (TEXT("target")))
					Color = FLinearColor(0.1f, 0.5f, 1.0f, 1.0f); // XANH 
				else
					Color = FLinearColor(0.8f, 0.8f, 0.8f, 0.8f); // XÁM
				// Gán màu vào parameter "BaseColor" của material
				DynMat->SetVectorParameterValue(TEXT("Color"), Color);
				// Gán material vào mesh
				SphereActor->GetStaticMeshComponent()->SetMaterial(0, DynMat);
			}

		}
	}
	UE_LOG(LogTemp, Warning,
		TEXT("=== Đã spawn %d quả cầu vào scene ==="), Vertices.Num());
}

//hàm đọc CẠNH
bool AGraphDataLoader::LoadEdgesFromCSV(const FString& FileName) {
	Edges.Empty();
	FString FullPath = FPaths::ProjectContentDir() + TEXT("Data/") + FileName;
	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *FullPath)) {
		UE_LOG(LogTemp, Error, TEXT("Không tìm thấy file: %s"), *FullPath);
		return false;
	}
	//duyet tung dong(tru dong header)
	for (int32 i = 1; i < Lines.Num(); i++) {

		const FString& Line = Lines[i];
		if (Line.IsEmpty()) continue;

		TArray<FString> Cols;

		Line.ParseIntoArray(Cols, TEXT(","), false);

		if (Cols.Num() < 5) continue;
		//tao dinh moi
		FEdgeData E;

		E.StartId = FCString::Atoi(*Cols[2]); 
		E.EndId = FCString::Atoi(*Cols[3]);   
		E.Weight = FCString::Atof(*Cols[4]);  

		Edges.Add(E);
	}
	//tra ve true neu doc duoc it nhat 1 canh
	return Edges.Num() > 0;
}

//Hàm vẽ cạnh
void AGraphDataLoader::DrawEdges() {
	UWorld* World = GetWorld();
	if (!World) return;

	TMap<int32, FVector> VertexLocationMap;
	for (const FVertexData& V : Vertices) {
		//them dl vao map
		VertexLocationMap.Add(V.Id, V.Location);
		int32 DrawnCount = 0;
		for (const FEdgeData& E : Edges) {
			FVector* StartLoc = VertexLocationMap.Find(E.StartId);
			FVector* EndLoc = VertexLocationMap.Find(E.EndId);
			if (!StartLoc || !EndLoc) continue;
			DrawDebugLine(
				World,
				*StartLoc,
				*EndLoc,
				EdgeColor.ToFColor(true),
				true,
				-1.0f,//lifetime
				0,//DepthPriority
				80.0f//Thickness
			);
			DrawnCount++;
		}
		UE_LOG(LogTemp, Warning, TEXT("=== Đã vẽ %d cạnh ==="), DrawnCount);
	}
}

//Hàm đọc ĐƠN VỊ
bool AGraphDataLoader::LoadUnitsFromCSV(const FString& FileName) {
	Units.Empty();
	FString FullPath = FPaths::ProjectContentDir() + TEXT("Data/") + FileName;

	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *FullPath)) {
		UE_LOG(LogTemp, Error, TEXT("Không tìm thấy file: %s"), *FullPath);
		return false;
	}
	//duyet tung dong(tru dong header)
	for (int32 i = 1; i < Lines.Num(); i++) {

		const FString& Line = Lines[i];
		if (Line.IsEmpty()) continue;

		TArray<FString> Cols;

		Line.ParseIntoArray(Cols, TEXT(","), false);

		if (Cols.Num() < 6) continue;
		//tao unit moi
		FUnitData U;

		float X = FCString::Atof(*Cols[3]);
		float Y = FCString::Atof(*Cols[4]);
		float Z = FCString::Atof(*Cols[5]);
		//ghep X,Y,Z thanh location
		U.Location = FVector(X, Y, Z);
		U.UnitId = Cols[1].TrimStartAndEnd();
		U.UnitName = Cols[2].TrimStartAndEnd();
		Units.Add(U);

		UE_LOG(LogTemp, Warning, TEXT("Unit %s tại (%.1f, %.1f, %.1f)"),
			*U.UnitId, U.Location.X, U.Location.Y, U.Location.Z);
	}
	//tra ve true neu doc duoc it nhat 1 unit
	return Units.Num() > 0;
}

//spawn đơn vị
void AGraphDataLoader::SpawnUnitActors() {
	UWorld* World = GetWorld();
	if (!World || !VertexMesh) return;
	for (const FUnitData& U : Units) {
		FTransform SpawnTransform;
		//Lay vi tri Unit dat vao Transform = dat vi tri spawn
		SpawnTransform.SetLocation(U.Location);
		//Dat kich thuoc U
		SpawnTransform.SetScale3D(FVector(UnitScale / 100.0f));
		//spawn, tao actor trong world
		AStaticMeshActor* UnitActor = World->SpawnActor<AStaticMeshActor>(
			AStaticMeshActor::StaticClass(), SpawnTransform);
		if (UnitActor) {

			UnitActor->GetStaticMeshComponent()->SetStaticMesh(VertexMesh);

			UnitActor->SetActorLabel(
				FString::Printf(TEXT("Unit_%s"), *U.UnitId));

			UnitActor->GetStaticMeshComponent()
				->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			//color
			UMaterial* BaseMat = LoadObject<UMaterial>(
				nullptr,
				TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")
			);

			if (BaseMat)
			{
				UMaterialInstanceDynamic* DynMat =
					UMaterialInstanceDynamic::Create(BaseMat, UnitActor);
				DynMat->SetVectorParameterValue(
					TEXT("Color"),
					FLinearColor(1.0f, 0.0f, 0.0f, 1.0f) //Đỏ
				);
				UnitActor->GetStaticMeshComponent()->SetMaterial(0, DynMat);
			}
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("=== Đã spawn %d Unit Actor ==="), Units.Num());
}

int32 AGraphDataLoader::FindNearestVertexIndex(const FVector& Position) const {
	int32 NearestIdx = -1;
	float MinDist = FLT_MAX;
	for (int32 i = 0; i < Vertices.Num(); i++) {
		float Dist = FVector::Dist(Position, Vertices[i].Location);
		if (Dist < MinDist) {
			MinDist = Dist;
			NearestIdx=i;
		}
	}
	return NearestIdx;
}

void AGraphDataLoader::ConnectUnitsToGraph() {
	UWorld* World = GetWorld();
	if (!World || Vertices.IsEmpty() || Units.IsEmpty()) return;
	FColor UnitEdgeColor = FColor::White;
	int32 ConnectedCount = 0;
	for (const FUnitData& U : Units) {
		//tìm đỉnh gần nhất 
		int32 NearestIdx = FindNearestVertexIndex(U.Location);
		if (NearestIdx < 0) continue;
		const FVertexData& NearestV = Vertices[NearestIdx];

		// Tính khoảng cách thực tế
		float Dist = FVector::Dist(U.Location, NearestV.Location);

		DrawDebugLine(
			World,
			U.Location,          
			NearestV.Location,   
			UnitEdgeColor,       
			true,                
			-1.0f,
			0,
			80.0f                
		);
		ConnectedCount++;

		UE_LOG(LogTemp, Warning,
			TEXT("Unit %s → Đỉnh %d (%.0f m)"),
			*U.UnitId, NearestV.Id, Dist / 100.0f);
	}
	UE_LOG(LogTemp, Warning,
		TEXT("=== Đã nối %d Unit vào graph ==="), ConnectedCount);
} 

TArray<int32> AGraphDataLoader::FindShortestPath(
	int32 StartVertexId, int32 EndVertexId) {
	TArray<int32> EmptyPath;
	if (Vertices.IsEmpty() || Edges.IsEmpty()) return EmptyPath;

	//Bước 1: Xây dựng danh sách kề
	TMap<int32, TArray<TPair<int32, float>>> AdjList;//adjacency list
	for (const FVertexData& V : Vertices)
		//khoi tao danh sach rong cho tung vertex
		AdjList.Add(V.Id, TArray<TPair<int32, float>>());
	//them cạnh 2chieu vao danh sach ke
	for (const FEdgeData& E : Edges) {
		AdjList[E.StartId].Add(TPair<int32, float>(E.EndId, E.Weight));
		AdjList[E.EndId].Add(TPair<int32, float>(E.StartId, E.Weight));
	}

	//Bước 2: Khởi tạo Dijkstra
	TMap <int32, float>Dist;
	TMap<int32, int32> Prev;
	for (const FVertexData& V : Vertices) {
		Dist.Add(V.Id, FLT_MAX);//ban dau all deu bằng vo cung
		Prev.Add(V.Id, -1);//ban dau chua cho dinh truoc do
	}
	Dist[StartVertexId] = 0.0f;//Khoảng cách từ đỉnh bắt đầu đến chính nó là 0
	
	//Priority Queue
	TArray<TPair<float, int32>> PQ;
	PQ.Add(TPair<float, int32>(0.0f, StartVertexId));

	//Bước 3: Vòng lặp Dijkstra
	while (!PQ.IsEmpty()) {
		int32 MinIdx = 0;
		for (int32 k = 1; k < PQ.Num(); k++)
			if (PQ[k].Key < PQ[MinIdx].Key) MinIdx = k;
		TPair<float, int32> Top = PQ[MinIdx];
		PQ.RemoveAt(MinIdx);

		float  D = Top.Key;//Dist
		int32  U = Top.Value;//IDVertex
		// Bỏ qua nếu đã xử lý (tìm thấy đường ngắn hơn trước đó)
		if (D > Dist[U]) continue;

		// Đã đến đích → dừng vòng lặp
		if (U == EndVertexId) break;

		if (!AdjList.Contains(U)) continue;
		// Duyệt các đỉnh kề
		for (const TPair<int32, float>& Neighbor : AdjList[U])
		{
			int32 V = Neighbor.Key;
			float W = Neighbor.Value;
			float NewD = Dist[U] + W;

			if (NewD < Dist[V])
			{
				Dist[V] = NewD;
				Prev[V] = U;
				PQ.Add(TPair<float, int32>(NewD, V));
			}
		}
	}

	//Bước 4: Truy vết đường đi từ End về Start
	TArray<int32> Path;
	// Kiểm tra có đường không
	if (Dist[EndVertexId] == FLT_MAX)
	{
		UE_LOG(LogTemp, Error,
			TEXT("Dijkstra: Không tìm thấy đường %d → %d"),
			StartVertexId, EndVertexId);
		return EmptyPath;
	}
	// Truy vết ngược từ End về Start
	int32 Current = EndVertexId;
	while (Current != -1)
	{
		Path.Insert(Current, 0); // chèn vào đầu để đảo chiều
		Current = Prev[Current];
	}

	UE_LOG(LogTemp, Warning,
		TEXT("Dijkstra: %d → %d | %d đỉnh | %.0f m"),
		StartVertexId, EndVertexId,
		Path.Num(), Dist[EndVertexId] / 100.0f);

	return Path;
}

//Hàm vẽ đường tìm được
void AGraphDataLoader::DrawPath(
	const TArray<int32>& PathIds,
	FLinearColor Color,
	float Thickness)
{
	UWorld* World = GetWorld();
	if (!World || PathIds.Num() < 2) return;

	// Tạo map Id → Location để tra cứu nhanh
	TMap<int32, FVector> LocMap;
	for (const FVertexData& V : Vertices)
		LocMap.Add(V.Id, V.Location);

	// Vẽ từng đoạn của đường đi
	for (int32 i = 0; i < PathIds.Num() - 1; i++)
	{
		FVector* StartLoc = LocMap.Find(PathIds[i]);
		FVector* EndLoc = LocMap.Find(PathIds[i + 1]);
		if (!StartLoc || !EndLoc) continue;

		DrawDebugLine(
			World,
			*StartLoc, *EndLoc,
			Color.ToFColor(true),
			true,    // persistent
			-1.0f,
			0,
			Thickness
		);
	}
}

//GA Data
bool AGraphDataLoader::LoadTargetsFromCSV(const FString& FileName) {
	Targets.Empty();
	FString FullPath = FPaths::ProjectContentDir() + TEXT("Data/") + FileName;
	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *FullPath)) return false;

	for (int32 i = 1; i < Lines.Num(); i++)
	{
		if (Lines[i].IsEmpty()) continue;
		TArray<FString> C;
		Lines[i].ParseIntoArray(C, TEXT(","), false);
		if (C.Num() < 11) continue;

		FTargetData T;
		T.TargetId = FCString::Atoi(*C[4]);
		T.Code = C[5].TrimStartAndEnd();
		T.Name = C[6].TrimStartAndEnd();
		T.Priority = FCString::Atoi(*C[7]);
		T.Explosive = FCString::Atof(*C[8]);
		T.MilitaryValue = FCString::Atof(*C[9]);
		T.VertexId = FCString::Atoi(*C[10]);
		Targets.Add(T);

		UE_LOG(LogTemp, Warning, TEXT("Target %s: E=%.0f mv=%.0f pri=%d vertex=%d"),
			*T.Name, T.Explosive, T.MilitaryValue, T.Priority, T.VertexId);
	}
	return Targets.Num() > 0;
}

bool AGraphDataLoader::LoadUAVsFromCSV(const FString& FileName)
{
	UAVs.Empty();
	FString FullPath = FPaths::ProjectContentDir() + TEXT("Data/") + FileName;
	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *FullPath)) return false;

	for (int32 i = 1; i < Lines.Num(); i++)
	{
		if (Lines[i].IsEmpty()) continue;
		TArray<FString> C;
		Lines[i].ParseIntoArray(C, TEXT(","), false);
		if (C.Num() < 12) continue;

		FUAVData U;
		U.UavId = FCString::Atoi(*C[1]);
		U.UavCode = C[2].TrimStartAndEnd();
		U.UavType = C[3].TrimStartAndEnd();
		U.Quantity = FCString::Atoi(*C[4]);
		U.Range = FCString::Atof(*C[5]);
		U.Speed = FCString::Atof(*C[6]);
		U.Explosive = FCString::Atof(*C[8]);
		U.MilitaryValue = FCString::Atof(*C[10]);
		U.UnitId = C[11].TrimStartAndEnd();
		UAVs.Add(U);

		UE_LOG(LogTemp, Warning,
			TEXT("UAV %s (%s) qty=%d e=%.0f mv=%.0f unit=%s"),
			*U.UavCode, *U.UavType, U.Quantity,
			U.Explosive, U.MilitaryValue, *U.UnitId);
	}
	return UAVs.Num() > 0;
}

bool AGraphDataLoader::LoadProbabilityFromCSV(const FString& FileName)
{
	ProbabilityMap.Empty();
	FString FullPath = FPaths::ProjectContentDir() + TEXT("Data/") + FileName;
	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *FullPath)) return false;

	for (int32 i = 1; i < Lines.Num(); i++)
	{
		if (Lines[i].IsEmpty()) continue;
		TArray<FString> C;
		Lines[i].ParseIntoArray(C, TEXT(","), false);
		if (C.Num() < 4) continue;

		FString UavCode = C[1].TrimStartAndEnd();
		int32   TargetId = FCString::Atoi(*C[2]);
		float   Prob = FCString::Atof(*C[3]);

		// Key: "C11|2" → tra cứu nhanh O(1)
		FString Key = FString::Printf(TEXT("%s|%d"), *UavCode, TargetId);
		ProbabilityMap.Add(Key, Prob);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("Đã load %d xác suất"), ProbabilityMap.Num());
	return ProbabilityMap.Num() > 0;
}

// Hàm tra xác suất p_ij
float AGraphDataLoader::GetProbability(
	const FString& UavCode, int32 TargetId) const
{
	FString Key = FString::Printf(TEXT("%s|%d"), *UavCode, TargetId);
	const float* Found = ProbabilityMap.Find(Key);
	return Found ? *Found : 0.0f; // 0 nếu không có
}

//Đánh giá độ tối ưu của 1 phương án phân công 
float AGraphDataLoader::EvaluateFitness(const TArray<TArray<int32>>& X) {
	int32 n = UAVs.Num();
	int32 m = Targets.Num();
	float F = 0.0f;//giá trị kỳ vọng tiêu diệt mtieu
	float Penalty = 0.0f; 
	const float k = 2.0f; //hệ số pen

	for (int32 j = 0; j < m; j++) {
		float Sj = 1.0f;//xác suất sống sót của mục tiêu
		float  TotalExplosive = 0.0f;//tổng lượng nổ được gán cho j
		bool AnyAssigned = false;// Target j có ít nhất 1 UAV được phân công chưa ?
		for (int32 i = 0; i < n; i++) {
			if (X[i][j] == 1) // tức là UAV i được phân công cho Target j trong ma trận phương án X
			{
				float Pij = GetProbability(UAVs[i].UavCode, Targets[j].TargetId);
				Sj *= (1.0f - Pij);
				TotalExplosive += UAVs[i].Explosive;
				AnyAssigned = true;//đánh dấu target j đã được phân công uav
			}

		}
		// F += v_j × (1 - S_j): giá trị kỳ vọng tiêu diệt mục tiêu j
		// (1 - S_j) = xác suất bị tiêu diệt
		F += Targets[j].MilitaryValue * (1.0f - Sj);//hàm mục tiêu.->MAX

		//pen nếu lượng nổ k đủ
		if (AnyAssigned && TotalExplosive < Targets[j].Explosive) {// tức là lượng nổ của UAV i được gán cho mục tiêu j nhỏ hơn lượng nổ đnag có của mục tiêu j
			float ShortageRatio = (Targets[j].Explosive - TotalExplosive) / Targets[j].Explosive;//tỷ lệ thiếu hụt	
			Penalty += ShortageRatio * Targets[j].MilitaryValue * k;
		}
	}
	return F - Penalty;
}

//Sửa ràng buộc(2, 3)
void AGraphDataLoader::RepairSolution(TArray<TArray<int32>>& X) {
	int32 n = UAVs.Num();
	int32 m = Targets.Num();
	//R2
	for (int32 i = 0; i < n; i++) {
		// Đếm số mục tiêu loại UAV i đang được gán
		int32 AssignedCount = 0;//đêm UAV đang được giao bao nhiêu mục tiêu?
		for (int32 j = 0; j < m; j++) 
			AssignedCount += X[i][j];

		// Nếu số phân công vượt quá số chiếc UAV có → vi phạm R2
		if (AssignedCount > UAVs[i].Quantity)
		{
			// Score càng cao= phân công càng tối ưu=> giữ lại tối ưu nhất
			TArray<TPair<float, int32>> Scores;

			for (int32 j = 0; j < m; j++)
			{
				if (X[i][j] != 1) continue;
				float Pij = GetProbability(UAVs[i].UavCode, Targets[j].TargetId);
				float Score = Pij * Targets[j].MilitaryValue / (float)Targets[j].Priority;
				Scores.Add(TPair<float, int32>(Score, j));
			}

			Scores.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B)
			{
				return A.Key > B.Key; // A.Key > B.Key → giảm dần
			});

			// Xóa TOÀN BỘ phân công của UAV i trước
			for (int32 j = 0; j < m; j++) X[i][j] = 0;

			int32 KeepCount = FMath::Min(UAVs[i].Quantity, Scores.Num());//tức là FMath::Min đảm bảo không vượt quá số phần tử có trong Scores

			//gán lại phân công tối ưu nhất.
			for (int32 k = 0; k < KeepCount; k++)
				X[i][Scores[k].Value] = 1;//value ở đây là TargetId có kiểu dữ liệu int32 ở khai báo của Scores
		}
	}
	
	//R3: Σ_i x_ij ≤ Σ a_ij (khả dụng)
	for (int32 i = 0; i < n; i++)
		for (int32 j = 0; j < m; j++)
			if (Targets[j].VertexId <= 0)// =-1( k tồn tại)
				X[i][j] = 0;
}

//Khởi tạo một phương án phân công ngẫu nhiên_tức là điền ngẫu nhiên vào ma trận X[n][m]
TArray<TArray<int32>> AGraphDataLoader::RandomIndividual() {

	int32 n = UAVs.Num();
	int32 m = Targets.Num();
	TArray<TArray<int32>> X;

	for (int32 i = 0; i < n; i++) {
		TArray<int32> Row;
		Row.Init(0, m);//khởi tạo m phần từ, tất cả có giá trị bằng 0
		X.Add(Row);
	}

	if (FMath::FRand() < 0.5f) {//nếu rand()<0.5=> khởi tạo thông minh

		for (int32 j = 0; j < m; j++) {

			float Accumulated = 0.0f;             // tổng nổ đã tích lũy của UAV được gán cho Target
			float Required = Targets[j].Explosive; // E_j cần thiết
			//xáo trộn thứ tự UAV
			TArray<int32> UAVOrder;
			for (int32 i = 0; i < n; i++) UAVOrder.Add(i);
			for (int32 s = UAVOrder.Num() - 1; s > 0; s--)//duyệt từ index cuối cùng.
			{
				int32 r = FMath::RandRange(0, s);//chọn 1 vị trí ngẫu nhiên trong mảng để swap vs s.
				UAVOrder.Swap(s, r);
			}

			for (int32 i : UAVOrder)
			{
				// Đủ rồi → dừng, không gán thêm UAV 
				if (Accumulated >= Required) break;

				// Kiểm tra UAV i chưa vượt quá Quantity
				int32 AlreadyUsed = 0;//số target mà UAV i đã được phân công hiện tại.
				for (int32 jj = 0; jj < m; jj++)
					AlreadyUsed += X[i][jj];
				if (AlreadyUsed >= UAVs[i].Quantity) continue;
				// UAVs[i].Quantity=1 → mỗi UAV chỉ gán 1 mục tiêu

				// Gán UAV i cho mục tiêu j
				X[i][j] = 1;
				Accumulated += UAVs[i].Explosive;
			}
		}
	}
	else {
		//Khở tạo ngẫu nhiên. 
		for (int32 i = 0; i < n; i++) //Duyệt từng UAV
		{
			for (int32 q = 0; q < UAVs[i].Quantity; q++) // duyệt từng chiếc UAV trong loại i 
			{
				if (FMath::FRand() < 0.9f) // 90% xác suất được gán
				{
					int32 j = FMath::RandRange(0, m - 1);// chọn mục tiêu ngẫu nhiên
					X[i][j] = 1;
				}
				// 10% còn lại: UAV này nghỉ, không đánh gì
			}
		}
	}
	return X;
}

void AGraphDataLoader::RunGA() {
	if (UAVs.IsEmpty() || Targets.IsEmpty()) return;
	int32 n = UAVs.Num();
	int32 m = Targets.Num();

	const int32 PopSize = 100;   // 100 cá thể trong quần thể
	const int32 MaxGen = 300;   // 300 thế hệ tiến hóa
	const float CrRate = 0.85f; // 85% xác suất lai ghép
	const float MutRate = 0.1f;  // 10% xác suất đột biến mỗi gen

	// ── Khởi tạo quần thể ──
	TArray<TArray<TArray<int32>>> Pop;    // quần thể: mảng các cá thể
	TArray<float>                 Fitness; // điểm fitness tương ứng

	for (int32 k = 0; k < PopSize; k++)
	{
		// Tạo cá thể ngẫu nhiên
		TArray<TArray<int32>> Ind = RandomIndividual();

		// Sửa ràng buộc trước khi đánh giá
		RepairSolution(Ind);

		// Tính fitness
		float F = EvaluateFitness(Ind);

		Pop.Add(Ind);
		Fitness.Add(F);
	}
	
	//tìm cá thể tốt nhất
	int32 BestIdx = 0;
	for (int32 k = 1; k < PopSize; k++)
		if (Fitness[k] > Fitness[BestIdx])
			BestIdx = k;
	BestAssignment = Pop[BestIdx];
	BestFitness = Fitness[BestIdx];

	for (int32 Gen = 0; Gen < MaxGen; Gen++)
	{
		TArray<TArray<TArray<int32>>> NewPop;
		TArray<float>                 NewFit;

		//thế hệ mới luôn chứa: cá thể tốt nhất, điểm cao nhất.
		NewPop.Add(BestAssignment);
		NewFit.Add(BestFitness);

		//tìm fitness nhỏ nhất
		float MinFit = Fitness[0];
		for (float F : Fitness)
			if (F < MinFit) 
				MinFit = F;

		// Tính tổng fitness đã shift
		float ShiftSum = 0.0f;
		for (float F : Fitness)
			ShiftSum += (F - MinFit + 1e-9f);

		//Chọn cha mẹ bằng Roulette
		auto Select = [&]()->int32
		{
			float r = FMath::FRand() * ShiftSum; // sinh số ngẫu nhiên [0, ShiftSum]
			float acc = 0.0f;//accumulator: tích lũy 
			for (int32 k = 0; k < PopSize; k++)
			{
				acc += (Fitness[k] - MinFit + 1e-9f);//Cộng dồn Fitness đã shift sang dương của cá thể k
				if (acc >= r) return k; // chọn cá thể k
			}
			return PopSize - 1;  //tưc là nếu vòng lặp k tìm được cá thể nào thỏa acc>=r thì chọn cá thể cuối( Pop-1 là index của cá thể cuối)
		};

		// Tạo PopSize-1 cá thể mới (1 slot đã dùng cho elitism)
		while (NewPop.Num() < PopSize) {
			int32 P1 = Select(); //bố
			int32 P2 = Select();//mẹ

			//lai ghép (Crossover)
			TArray<TArray<int32>> Child = Pop[P1]; // con bắt đầu = bố
			if (FMath::FRand() < CrRate) {
				int32 Cut = FMath::RandRange(1, n - 1);// lấy ngẫu nhiên 1 số nguyên từ 1 đến n-1 làm cut
				//Hàng từ 0 đến Cut - 1 → con lấy gene từ bố
				//Hàng từ Cut đến n - 1 → con lấy gene từ mẹ
				for (int32 i = Cut; i < n; i++)
					Child[i] = Pop[P2][i];
			}
			// ── Đột biến Bit-flip Mutation ──
			for (int32 i = 0; i < n; i++)
				for (int32 j = 0; j < m; j++)
					if (FMath::FRand() < MutRate)//tức là mỗi gen có 10% khả năng bị thay đổi.
						Child[i][j] = 1 - Child[i][j];//( tức là nếu đang là 0 -> 1; nếu đnag là 1 -> 0.
			// ── Repair + Evaluate ── 
			RepairSolution(Child);             // sửa ràng buộc
			float F = EvaluateFitness(Child);  // tính fitness

			NewPop.Add(Child);
			NewFit.Add(F);
			// Cập nhật best nếu con tốt hơn
			if (F > BestFitness)
			{
				BestFitness = F;
				BestAssignment = Child;
			}
		}
		// Chuyển sang thế hệ mới
		Pop = NewPop;
		Fitness = NewFit;
	}

	RepairSolution(BestAssignment);
	BestFitness = EvaluateFitness(BestAssignment);

	// ── In kết quả ──
	UE_LOG(LogTemp, Warning,
		TEXT("=== GA HOÀN THÀNH | Fitness = %.2f ==="), BestFitness);

	/*for (int32 i = 0; i < n; i++)
		for (int32 j = 0; j < m; j++)
			if (BestAssignment[i][j] == 1)
				UE_LOG(LogTemp, Warning,
					TEXT("  %s (qty=%d) → %s"),
					*UAVs[i].UavCode,
					UAVs[i].Quantity,
					*Targets[j].Name);*/
}

//Vẽ đường sau phân công
void AGraphDataLoader::DrawAssignmentPaths() {

	if (BestAssignment.IsEmpty()) return;
	int32 n = UAVs.Num();
	int32 m = Targets.Num();

	TMap<FString, int32> UnitStartVertex;
	for (const FUnitData& U : Units)
	{
		int32 NearestIdx = FindNearestVertexIndex(U.Location);
		if (NearestIdx >= 0)
			UnitStartVertex.Add(U.UnitId, Vertices[NearestIdx].Id);
	}
	// Màu theo loại UAV
   // Chiến đấu → xanh lam, Cảm tử → đỏ cam
	int32 PathCount = 0;

	for (int32 i = 0; i < n; i++)
	{
		for (int32 j = 0; j < m; j++)
		{
			if (BestAssignment[i][j] != 1) continue;

			// Tìm đỉnh xuất phát của đơn vị UAV i
			int32* StartId = UnitStartVertex.Find(UAVs[i].UnitId);
			if (!StartId) continue;

			// Đỉnh đích = vertex của mục tiêu j
			int32 EndId = Targets[j].VertexId;

			// Chạy Dijkstra
			TArray<int32> Path = FindShortestPath(*StartId, EndId);
			if (Path.IsEmpty()) continue;

			// Chọn màu theo loại UAV
			FLinearColor Color;
			FString Type = UAVs[i].UavType.ToLower();
			if (Type.Contains(TEXT("chiến đấu")))
				Color = FLinearColor(0.0f, 0.5f, 1.0f, 1.0f); // XANH LAM
			else
				Color = FLinearColor(1.0f, 0.3f, 0.0f, 1.0f); // CAM ĐỎ

			// Vẽ đường bay
			DrawPath(Path, Color, 150.0f);

			PathCount++;

			UE_LOG(LogTemp, Warning,
				TEXT("Đường bay: %s (%s) → %s | %d đỉnh"),
				*UAVs[i].UavCode, *UAVs[i].UavType,
				*Targets[j].Name, Path.Num());
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("=== Đã vẽ %d đường bay ==="), PathCount);

}

//hậu xử lý GA( tránh lãng phí UAV )
void AGraphDataLoader::OptimizeSubsets()
{
	int32 n = UAVs.Num();
	int32 m = Targets.Num();

	for (int32 j = 0; j < m; j++)
	{
		// Lấy lượng nổ cần thiết để tiêu diệt mục tiêu j
		float Ej = Targets[j].Explosive;
		if (Ej <= 0.0f) continue;

		// Thu thập tất cả UAV đang được gán cho mục tiêu j
		// assignedUAVs: danh sách index i của UAV đang gán j
		TArray<int32> AssignedUAVs;
		for (int32 i = 0; i < n; i++)
			if (BestAssignment[i][j] == 1 && UAVs[i].Explosive > 0.0f)
				AssignedUAVs.Add(i);

		// Không có UAV nào gán mục tiêu j → bỏ qua
		int32 k = AssignedUAVs.Num();
		if (k == 0) continue;

		// ── Brute-force(thuật toán vét cạn) tìm tập con nhỏ nhất đủ lượng nổ ──
		// Duyệt tất cả 2^k tập con (k ≤ 8 → tối đa 256 tập con, rất nhanh)
		// mask: số nhị phân biểu diễn tập con
		//   mask=001 → chỉ UAV đầu tiên
		//   mask=011 → UAV thứ 1 và 2
		//   mask=111 → cả 3 UAV

		float      MinSumE = FLT_MAX; // tổng nổ nhỏ nhất tìm được mà vẫn đủ
		TArray<int32> BestSubset;       // tập con tốt nhất

		for (int32 mask = 1; mask < (1 << k); mask++)
		{
			float      SumE = 0.0f;// tổng nổ của UAV đang xét
			TArray<int32> Subset;

			// Duyệt từng bit trong mask
			for (int32 b = 0; b < k; b++)
			{
				// Bit b của mask = 1 → UAV thứ b được chọn vào tập con
				if (mask & (1 << b))
				{
					SumE += UAVs[AssignedUAVs[b]].Explosive;
					Subset.Add(AssignedUAVs[b]); // lưu index UAV
				}
			}

			// Tập con hợp lệ: đủ nổ VÀ tổng nổ nhỏ nhất từ trước đến nay
			// → Mục tiêu: tìm tập con ÍT UAV NHẤT mà vẫn đủ nổ
			if (SumE >= Ej && SumE < MinSumE)
			{
				MinSumE = SumE;
				BestSubset = Subset;
			}
		}

		// ── Cập nhật BestAssignment theo tập con tối ưu ──
		for (int32 iUAV : AssignedUAVs)
		{
			// Kiểm tra UAV này có trong tập con tối ưu không
			bool Keep = BestSubset.Contains(iUAV);

			// Giữ lại nếu trong tập con, xóa nếu thừa
			BestAssignment[iUAV][j] = Keep ? 1 : 0;

			//if (!Keep)//Keep = false
			//	UE_LOG(LogTemp, Warning,
			//		TEXT("[SUBSET] Giải phóng %s khỏi %s (thừa)"),
			//		*UAVs[iUAV].UavCode, *Targets[j].Name);
		}

		// ── Xử lý trường hợp không tìm được tập con đủ nổ ──
		if (BestSubset.IsEmpty())
		{
			// Hủy toàn bộ phân công cho mục tiêu j
			/*UE_LOG(LogTemp, Error,
				TEXT("[SUBSET] %s không đủ nổ E=%.0f → hủy phân công"),
				*Targets[j].Name, Ej);*/
			for (int32 i = 0; i < n; i++)
				BestAssignment[i][j] = 0;
		}
		/*else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[SUBSET] %s | E=%.0f | Nổ=%0.f | %d UAV"),
				*Targets[j].Name, Ej, MinSumE, BestSubset.Num());
		}*/
	}
	// ── In phương án cuối cùng ──
	UE_LOG(LogTemp, Warning, TEXT("=== PHƯƠNG ÁN PHÂN CÔNG CUỐI CÙNG ==="));
	for (int32 i = 0; i < n; i++)
		for (int32 j = 0; j < m; j++)
			if (BestAssignment[i][j] == 1)
				UE_LOG(LogTemp, Warning,
					TEXT("  %s (%s) → %s | e=%.0f E=%.0f p=%.2f"),
					*UAVs[i].UavCode,
					*UAVs[i].UavType,
					*Targets[j].Name,
					UAVs[i].Explosive,
					Targets[j].Explosive,
					GetProbability(UAVs[i].UavCode, Targets[j].TargetId));

	UE_LOG(LogTemp, Warning, TEXT("=== UAV KHÔNG ĐƯỢC PHÂN CÔNG ==="));
	for (int32 i = 0; i < n; i++)
	{
		int32 Total = 0;
		for (int32 j = 0; j < m; j++) 
			Total += BestAssignment[i][j];
		if (Total == 0)
			UE_LOG(LogTemp, Warning,
				TEXT("  %s (%s) — không có mục tiêu"),
				*UAVs[i].UavCode, *UAVs[i].UavType);
	}
}













// Called every frame
//void AGraphDataLoader::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//}

