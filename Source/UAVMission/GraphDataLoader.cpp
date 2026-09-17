// Fill out your copyright notice in the Description page of Project Settings.


#include "GraphDataLoader.h"
#include "Misc/FileHelper.h"      
#include "Misc/Paths.h"  
#include "Blueprint/UserWidget.h"
#include "Engine/StaticMeshActor.h"   
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Components/StaticMeshComponent.h" 


// Sets default values
AGraphDataLoader::AGraphDataLoader()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
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
		//SpawnUnitActors();
		//ConnectUnitsToGraph();
	}

	LoadTargetsFromCSV(TEXT("Data_target1.csv"));
	LoadUAVsFromCSV(TEXT("Data_uav1.csv"));
	LoadProbabilityFromCSV(TEXT("Probability1.csv"));

	UE_LOG(LogTemp, Warning, TEXT("=== GA Data: %d targets, %d UAVs, %d probs ==="),
		Targets.Num(), UAVs.Num(), ProbabilityMap.Num());

	BuildReachability();//tính ma trận tầm bay trước khi chạy GA

	RunGA();

	OptimizeSubsets();

	DrawAssignmentPaths();

	SpawnUAVs();

	// Tao minimap
	if (MinimapWidgetClass)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			UUserWidget* W = CreateWidget<UUserWidget>(PC, MinimapWidgetClass);
			if (W)
			{
				W->AddToViewport();
				UE_LOG(LogTemp, Warning, TEXT("=== Minimap DA TAO ==="));
			}
			else UE_LOG(LogTemp, Error, TEXT("=== CreateWidget THAT BAI ==="));
		}
		else UE_LOG(LogTemp, Error, TEXT("=== KHONG CO PlayerController ==="));
	}
	else UE_LOG(LogTemp, Error, TEXT("=== CHUA GAN MinimapWidgetClass ==="));
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

		if (Cols.Num() < 5) continue;
		//tao dinh moi
		FVertexData V;

		double Northing = FCString::Atod(*Cols[0]);
		double Easting = FCString::Atod(*Cols[1]);
		double Alt = FCString::Atod(*Cols[2]);
		//ghep X,Y,Z thanh location
		V.Location = FVector((Northing - OriginNorthing) * MetersToUE,
			(Easting - OriginEasting) * MetersToUE,
			Alt * MetersToUE);
		V.Id = FCString::Atoi(*Cols[3]);
		V.TypeVertex = Cols[4].TrimStartAndEnd();

		// >>> In toạ độ đã chuẩn hoá, gắn với code mục tiêu <
		UE_LOG(LogTemp, Warning,
			TEXT("[%s] CSV(X=%.2f  Y=%.2f  Z=%.2f) -> UE(%.1f, %.1f, %.1f)"),
			*V.TypeVertex, Northing, Easting, Alt,
			V.Location.X, V.Location.Y, V.Location.Z);

		Vertices.Add(V);
	}
	//tra ve true neu doc duoc it nhat 1 dinh
	return Vertices.Num() > 0;

}

//Hàm Spawn quả cầu=>duyệt từng vertex, sinnh ra AStaticMeshActor
void AGraphDataLoader::SpawnVertexActors() {
	UWorld* World = GetWorld();
	if (!World || !VertexMesh) return;

	VertexActors.Empty();  // Xóa list cũ

	for (const FVertexData& V : Vertices) {
		FTransform SpawnTransform;

		FVector SphereLocation = V.Location + FVector(0, 0, GraphHeightOffset);
		SpawnTransform.SetLocation(SphereLocation);
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
				if (Type == TEXT("target"))
					Color = FLinearColor(0.1f, 0.5f, 1.0f, 1.0f);   // Xanh
				else if (Type == TEXT("unit"))
					Color = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);   // Do
				else
					Color = FLinearColor(0.8f, 0.8f, 0.8f, 0.8f);   //Xam
				// Gán màu vào parameter "BaseColor" của material
				DynMat->SetVectorParameterValue(TEXT("Color"), Color);
				// Gán material vào mesh
				SphereActor->GetStaticMeshComponent()->SetMaterial(0, DynMat);
			}
			// === Thêm vào list ===
			VertexActors.Add(SphereActor);

			//// ===== Nhãn trên đỉnh =====
			//FString TargetName = TEXT("V");  // mặc định
			//for (const FTargetData& T : Targets)
			//{
			//	if (T.VertexId == V.Id)
			//	{
			//		TargetName = T.Name;
			//		break;
			//	}
			//}

			//// Vẽ tên ở trên Sphere
			//DrawDebugString(
			//	World,
			//	V.Location + FVector(0, 0, VertexScale * 2),  // Vị trí trên Sphere
			//	TargetName,
			//	nullptr,
			//	FColor::White,
			//	-1.0f,  // vô thời hạn
			//	true,   // bDrawShadow
			//	1.2f    // TextScale
			//);
		}
	}
	//UE_LOG(LogTemp, Warning,
		//TEXT("=== Đã spawn %d quả cầu vào scene (offset Z=%.0f) ==="), Vertices.Num());
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

		if (Cols.Num() < 4) continue;
		//tao dinh moi
		FEdgeData E;

		E.StartId = FCString::Atoi(*Cols[1]); 
		E.EndId = FCString::Atoi(*Cols[2]);   
		E.Weight = FCString::Atof(*Cols[3]);  

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

	// ===== TÍNH TỔNG CẠNH =====
	int32 TotalDrawnCount = 0;

	for (const FVertexData& V : Vertices) {
		VertexLocationMap.Add(V.Id, V.Location);
	}

	// ===== VẼ CÁC CẠNH =====
	for (const FEdgeData& E : Edges) {
		FVector* StartLoc = VertexLocationMap.Find(E.StartId);
		FVector* EndLoc = VertexLocationMap.Find(E.EndId);
		if (!StartLoc || !EndLoc) continue;

		FVector Start = *StartLoc + FVector(0, 0, GraphHeightOffset);
		FVector End = *EndLoc + FVector(0, 0, GraphHeightOffset);

		DrawDebugLine(
			World,
			Start,
			End,
			EdgeColor.ToFColor(true),
			true,
			-1.0f,
			0,
			1500.0f
		);
		TotalDrawnCount++;
	}
	UE_LOG(LogTemp, Warning, TEXT("=== Đã vẽ %d cạnh (offset Z=%f) ==="),
		TotalDrawnCount, GraphHeightOffset);
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

		double Northing = FCString::Atod(*Cols[0]);
		double Easting = FCString::Atod(*Cols[1]);
		double Alt = FCString::Atod(*Cols[2]);
		//ghep X,Y,Z thanh location
		U.Location = FVector(
			(Northing - OriginNorthing) * MetersToUE,
			(Easting - OriginEasting) * MetersToUE,
			Alt * MetersToUE
		);
		U.UnitId = Cols[3].TrimStartAndEnd();
		U.UnitName = Cols[4].TrimStartAndEnd();
		U.VertexId = FCString::Atoi(*Cols[5]);
		Units.Add(U);

		UE_LOG(LogTemp, Warning, TEXT("Unit %s tại (%.1f, %.1f, %.1f)"),
			*U.UnitId, U.Location.X, U.Location.Y, U.Location.Z);
	}
	//tra ve true neu doc duoc it nhat 1 unit
	return Units.Num() > 0;
}

TArray<int32> AGraphDataLoader::FindShortestPath(
	int32 StartVertexId, int32 EndVertexId, float* OutDistance) {
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
		if (OutDistance) *OutDistance = FLT_MAX;//khong toi duoc
		return EmptyPath;
	}
	// Truy vết ngược từ End về Start
	int32 Current = EndVertexId;
	while (Current != -1)
	{
		Path.Insert(Current, 0); // chèn vào đầu để đảo chiều
		Current = Prev[Current];
	}

	/*UE_LOG(LogTemp, Warning,
		TEXT("Dijkstra: %d → %d | %d đỉnh | %.0f m"),
		StartVertexId, EndVertexId,
		Path.Num(), Dist[EndVertexId]);*/
		if (OutDistance) *OutDistance = Dist[EndVertexId];// trả về khoảng cách
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

void AGraphDataLoader::DrawSavedAssignmentPaths()
{
	UWorld* World = GetWorld();
	if (!World) return;

	for (const FFlightPath& P : FlightPaths)
	{
		for (int32 i = 0; i < P.Points.Num() - 1; i++)
		{
			DrawDebugLine(World,
				P.Points[i], P.Points[i + 1],
				P.Color.ToFColor(true),
				true,      // persistent
				-1.0f, 0, 2000.0f);
		}
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
		if (C.Num() < 10) continue;

		FTargetData T;
		T.GroundAltitude = FCString::Atof(*C[2]);
		T.TargetId = FCString::Atoi(*C[3]);
		T.Code = C[4].TrimStartAndEnd();
		T.Name = C[5].TrimStartAndEnd();
		T.Priority = FCString::Atoi(*C[6]);
		T.ExplosiveReq = FCString::Atof(*C[7]);
		T.ValueUSD = FCString::Atof(*C[8]);
		T.VertexId = FCString::Atoi(*C[9]);
		Targets.Add(T);

		UE_LOG(LogTemp, Warning, TEXT("Target %s: E=%.0f mv=%.0f pri=%d vertex=%d"),
			*T.Name, T.ExplosiveReq, T.ValueUSD, T.Priority, T.VertexId);
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
		if (C.Num() < 11) continue;

		FUAVData U;
		U.UavId = FCString::Atoi(*C[0]);
		U.UavCode = C[1].TrimStartAndEnd();
		U.UavType = C[2].TrimStartAndEnd();
		U.Quantity = FCString::Atoi(*C[3]);
		U.Range = FCString::Atof(*C[4]);
		U.Speed = FCString::Atof(*C[5]);
		U.Explosive = FCString::Atof(*C[7]);
		U.CostUSD = FCString::Atof(*C[9]);
		U.Budget = FCString::Atof(*C[10]);
		U.UnitId = C[11].TrimStartAndEnd();
		UAVs.Add(U);

		UE_LOG(LogTemp, Warning,
			TEXT("UAV %s (%s) qty=%d W=%.0f cost=%.0f C=%.0f unit=%s"),
			*U.UavCode, *U.UavType, U.Quantity,
			U.Explosive, U.CostUSD, U.Budget, *U.UnitId);
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

float AGraphDataLoader::PathLength(const TArray<int32>& Path)
{
	float Total = 0.f;
	for (int32 k = 0; k < Path.Num() - 1; k++)
	{
		int32 A = Path[k], B = Path[k + 1];
		// Tim canh noi A-B trong danh sach canh
		for (const FEdgeData& E : Edges)
		{
			if ((E.StartId == A && E.EndId == B) || (E.StartId == B && E.EndId == A))
			{
				Total += E.Weight;   // Weight da la met
				break;
			}
		}
	}
	return Total;
}

// c_ij = cost_i + k * L_ij/1000   (L_ij = cu ly Dijkstra met)
float AGraphDataLoader::ComputeCostIJ(int32 i, int32 j)
{
	// Tim dinh xuat phat cua UAV i
	int32 StartV = -1;

	for (const FUnitData& U : Units)
	{
		if (U.UnitId == UAVs[i].UnitId)
		{
			StartV = U.VertexId;
			break;
		}
	}

	// Khong xac dinh duoc diem xuat phat / dich
	if (StartV < 0 || Targets[j].VertexId <= 0)
		return FLT_MAX;

	// Tim duong ngan nhat
	TArray<int32> Path =
		FindShortestPath(StartV, Targets[j].VertexId);

	// Khong co duong di
	if (Path.IsEmpty())
		return FLT_MAX;

	// Tong chieu dai duong di, don vi met
	float L = PathLength(Path);

	// c_ij = Cost_i + k * L_ij(km)
	return UAVs[i].CostUSD
		+ CostCoefK * (L / 1000.0f);
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
		F += Targets[j].ValueUSD * (1.0f - Sj);//hàm mục tiêu.->MAX

		//pen nếu lượng nổ k đủ
		if (AnyAssigned && TotalExplosive < Targets[j].ExplosiveReq) {// tức là lượng nổ của UAV i được gán cho mục tiêu j nhỏ hơn lượng nổ đnag có của mục tiêu j
			float ShortageRatio = (Targets[j].ExplosiveReq - TotalExplosive) / Targets[j].ExplosiveReq;//tỷ lệ thiếu hụt	
			Penalty += ShortageRatio * Targets[j].ValueUSD * k;
		}
	}
	return F - Penalty;
}

//Sửa ràng buộc(2, 3)
void AGraphDataLoader::RepairSolution(TArray<TArray<int32>>& X) {
	int32 n = UAVs.Num();
	int32 m = Targets.Num();

	// ===== R4: Tam bay =====
	// ===== R4: TAM BAY =====
	for (int32 i = 0; i < n; i++)
	{
		for (int32 j = 0; j < m; j++)
		{
			if (X[i][j] != 1)
				continue;

			if (Targets[j].VertexId <= 0)
			{
				X[i][j] = 0;
				continue;
			}

			if (!Reachable.IsValidIndex(i) ||
				!Reachable[i].IsValidIndex(j) ||
				!Reachable[i][j])
			{
				X[i][j] = 0;
			}
		}
	}

	// ===== R2 + R1: So luong va ngan sach =====
	for (int32 i = 0; i < n; i++)
	{
		// Dem so phan cong va tong chi phi
		auto Recount = [&](int32& Cnt, float& Cost)
			{
				Cnt = 0; Cost = 0.f;
				for (int32 j = 0; j < m; j++)
					if (X[i][j] == 1) { Cnt++; Cost += ComputeCostIJ(i, j); }
			};

		int32 Cnt; float Cost;
		Recount(Cnt, Cost);

		// Loai phan cong hieu suat thap nhat cho den khi thoa R2 va R1
		while (Cnt > UAVs[i].Quantity ||
			(UAVs[i].Budget > 0.f && Cost > UAVs[i].Budget))
		{
			int32 WorstJ = -1;
			float WorstE = FLT_MAX;

			for (int32 j = 0; j < m; j++)
			{
				if (X[i][j] != 1) continue;
				float Pij = GetProbability(UAVs[i].UavCode, Targets[j].TargetId);
				float Cij = FMath::Max(ComputeCostIJ(i, j), 1.0f);
				float Eij = Targets[j].ValueUSD * Pij / Cij;  // e_ij = v_j*p_ij/c_ij
				if (Eij < WorstE) { WorstE = Eij; WorstJ = j; }
			}
			if (WorstJ < 0) break;
			X[i][WorstJ] = 0;
			Recount(Cnt, Cost);
		}
	}

	// ===== R3: Hoa luc =====
	// Xu ly theo thu tu uu tien: muc tieu quan trong (priority nho) truoc
	TArray<int32> TargetOrder;
	for (int32 j = 0; j < m; j++) TargetOrder.Add(j);
	TargetOrder.Sort([&](int32 A, int32 B) {
		return Targets[A].Priority < Targets[B].Priority;
		});

	for (int32 Idx = 0; Idx < m; Idx++)
	{
		int32 j = TargetOrder[Idx];
		float Wjreq = Targets[j].ExplosiveReq;
		if (Wjreq <= 0.f) continue;

		// Tinh tong W hien tai
		float TotalW = 0.f;
		bool AnyAssigned = false;
		TArray<int32> AssignedList;
		for (int32 i = 0; i < n; i++)
		{
			if (X[i][j] == 1)
			{
				TotalW += UAVs[i].Explosive;
				AssignedList.Add(i);
				AnyAssigned = true;
			}
		}

		// Truong hop DU: loai UAV W nho nhat neu van du
		if (AnyAssigned && TotalW > Wjreq)
		{
			AssignedList.Sort([&](int32 A, int32 B) {
				return UAVs[A].Explosive < UAVs[B].Explosive;
				});
			for (int32 iRemove : AssignedList)
			{
				float Wi = UAVs[iRemove].Explosive;
				if (TotalW - Wi >= Wjreq)
				{
					X[iRemove][j] = 0;
					TotalW -= Wi;
				}
			}
			continue;
		}

		// Truong hop THIEU: tim UAV bo sung
		if (AnyAssigned && TotalW < Wjreq)
		{
			float Deficit = Wjreq - TotalW;

			// Tim ung vien
			struct FCandidate { int32 i; float W; float Gap; };
			TArray<FCandidate> Cands;
			for (int32 i = 0; i < n; i++)
			{
				if (X[i][j] == 1) continue;
				if (UAVs[i].Explosive <= 0.f) continue;

				// Kiem tra con slot khong(R2)
				int32 Used = 0;
				for (int32 jj = 0; jj < m; jj++) Used += X[i][jj];
				if (Used >= UAVs[i].Quantity) continue;

				// Bao toan R4: UAV phai toi duoc target j =====
				if (!Reachable.IsValidIndex(i) ||
					!Reachable[i].IsValidIndex(j) ||
					!Reachable[i][j])
				{
					continue;
				}

				// R1: kiểm tra ngân sách


				float CurrentCost = 0.0f;

				// Tong chi phi cua UAV loai i dang su dung
				for (int32 jj = 0; jj < m; jj++)
				{
					if (X[i][jj] == 1)
					{
						CurrentCost += ComputeCostIJ(i, jj);
					}
				}

				// Chi phi neu UAV i danh them target j
				float NewAssignmentCost = ComputeCostIJ(i, j);

				// Khong tinh duoc chi phi
				if (NewAssignmentCost == FLT_MAX)
					continue;

				// Neu them phan cong nay lam vuot Budget thi bo UAV nay
				if (UAVs[i].Budget > 0.0f &&
					CurrentCost + NewAssignmentCost > UAVs[i].Budget)
				{
					continue;
				}

				//R3: mức độ phù hợp về lượng nỏ
				float Gap = FMath::Abs(UAVs[i].Explosive - Deficit);
				Cands.Add({ i, UAVs[i].Explosive, Gap });
			}
			Cands.Sort([](const FCandidate& A, const FCandidate& B) {
				return A.Gap < B.Gap;
				});

			for (const FCandidate& Cand : Cands)
			{
				if (TotalW >= Wjreq) break;

				// Kiểm tra số lượng lại(R2)
				int32 Used = 0;

				for (int32 jj = 0; jj < m; jj++)
					Used += X[Cand.i][jj];

				if (Used >= UAVs[Cand.i].Quantity)
					continue;

				// Kiểm tra budget lại(R1)
				float CurrentCost = 0.0f;

				for (int32 jj = 0; jj < m; jj++)
				{
					if (X[Cand.i][jj] == 1)
						CurrentCost += ComputeCostIJ(Cand.i, jj);
				}

				float NewCost =
					ComputeCostIJ(Cand.i, j);

				if (UAVs[Cand.i].Budget > 0.0f &&
					CurrentCost + NewCost >
					UAVs[Cand.i].Budget)
				{
					continue;
				}
				
				// ===== Hợp lệ -> bổ sung UAV cho R3 =====
				X[Cand.i][j] = 1;
				TotalW += Cand.W;
			}

			// Van thieu -> huy toan bo (phan cong nua voi la lang phi)
			if (TotalW < Wjreq)
			{
				for (int32 i = 0; i < n; i++) X[i][j] = 0;
			}
		}
	}

	//  VertexId <= 0 → x_ij = 0 
	for (int32 i = 0; i < n; i++)
	{
		for (int32 j = 0; j < m; j++)
		{
			// Target không có vertex
			if (Targets[j].VertexId <= 0)
			{
				X[i][j] = 0;
				continue;
			}

			// UAV ngoài tầm
			if (Reachable.IsValidIndex(i) &&
				Reachable[i].IsValidIndex(j) &&
				!Reachable[i][j])
			{
				X[i][j] = 0;
			}
		}
	}


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

	if (FMath::FRand() < 0.25f) {//nếu rand()<0.25=> khởi tạo thông minh
		
		// Sắp mục tiêu theo Priority tăng dần (Priority=1 quan trọng nhất) — khớp 2D
		TArray<int32> TargetByPriority;
		for (int32 t = 0; t < m; t++) TargetByPriority.Add(t);
		TargetByPriority.Sort([&](int32 a, int32 b) {
			return Targets[a].Priority < Targets[b].Priority;});

		for (int32 jIdx = 0; jIdx < m; jIdx++) {
			int32 j = TargetByPriority[jIdx];
			float Accumulated = 0.0f;             // tổng nổ đã tích lũy của UAV được gán cho Target
			float Required = Targets[j].ExplosiveReq; // E_j cần thiết
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
				
				// Bỏ qua nếu UAV i ngoài tầm mục tiêu j
				if (Reachable.IsValidIndex(i) && !Reachable[i][j]) continue;

				// Gán UAV i cho mục tiêu j
				X[i][j] = 1;
				Accumulated += UAVs[i].Explosive;
			}
		}
	}
	else {
		//Khởi tạo ngẫu nhiên. 
		for (int32 i = 0; i < n; i++) //Duyệt từng UAV
		{
			int32 NumPieces = UAVs[i].Quantity;

			for (int32 piece = 0; piece < NumPieces; piece++) // duyệt từng chiếc UAV trong loại i 
			{
				if (FMath::FRand() < 0.9f) // 90% xác suất được gán
				{
					int32 j = FMath::RandRange(0, m - 1);// chọn mục tiêu ngẫu nhiên
					//Kiểm tra loại i có còn slot không =====
					int32 AlreadyAssigned = 0;
					for (int32 jj = 0; jj < m; jj++)
						AlreadyAssigned += X[i][jj];

					if (AlreadyAssigned >= NumPieces)
						continue;  // Slot đủ rồi, bỏ chiếc này
					if (Reachable.IsValidIndex(i) && Reachable[i][j])   // chỉ gán nếu trong tầm
					X[i][j] = 1;
				}
				// 10% còn lại: UAV này nghỉ, không đánh gì
			}
		}
	}
	return X;
}

//Range
void AGraphDataLoader::BuildReachability() {
	int32 n = UAVs.Num();
	int32 m = Targets.Num();

	// Bản đồ: UnitId → đỉnh căn cứ của đơn vị đó
	TMap<FString, int32> UnitStartVertex;
	for (const FUnitData& U : Units)
		if (U.VertexId >= 0)
			UnitStartVertex.Add(U.UnitId, U.VertexId);
	// Khởi tạo ma trận n×m, mặc định false (chưa tới được)
	Reachable.SetNum(n);
	for (int32 i = 0; i < n; i++)
	{
		Reachable[i].Init(false, m);

		// Tìm đỉnh xuất phát của UAV i (theo đơn vị của nó)
		int32* StartId = UnitStartVertex.Find(UAVs[i].UnitId);
		if (!StartId) continue;   // không rõ căn cứ → để false hết

		for (int32 j = 0; j < m; j++)
		{
			int32 EndId = Targets[j].VertexId;

			float DistM = FLT_MAX;
			FindShortestPath(*StartId, EndId, &DistM);

			// Trong tầm → tới được
			Reachable[i][j] = (DistM <= UAVs[i].Range);

			UE_LOG(LogTemp, Warning,
				TEXT("[REACH] UAV %s → %s | dist=%.0fm range=%.0fm → %s"),
				*UAVs[i].UavCode, *Targets[j].Name,
				DistM, UAVs[i].Range,                       
				Reachable[i][j] ? TEXT("OK") : TEXT("NGOAI TAM"));
		}
	}
}


void AGraphDataLoader::RunGA() {
	if (UAVs.IsEmpty() || Targets.IsEmpty()) return;
	int32 n = UAVs.Num();
	int32 m = Targets.Num();

	const int32 PopSize = 100;   // 100 cá thể trong quần thể
	const int32 MaxGen = 500;   // 500 thế hệ tiến hóa
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

}
float AGraphDataLoader::GetGroundZAt(const FVector& Location) const
{
	FVector Start = FVector(Location.X, Location.Y, 1000000.0f);    // 10km trên trời
	FVector End = FVector(Location.X, Location.Y, -1000000.0f);   // 10km dưới đất

	FCollisionQueryParams Params;

	for (int32 i = 0; i < 50; i++)   // xuyên tối đa 50 lớp vật thể
	{
		FHitResult Hit;
		if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
			break;   // không còn gì bên dưới

		AActor* HitActor = Hit.GetActor();
		if (!HitActor) break;

		// Chỉ Landscape mới được tính là mặt đất
		if (HitActor->GetClass()->GetName().Contains(TEXT("Landscape")))
			return Hit.ImpactPoint.Z;

		// Không phải Landscape (xe, nhà, hàng rào...) → bỏ qua, dò xuyên tiếp xuống
		Params.AddIgnoredActor(HitActor);
	}

	UE_LOG(LogTemp, Error, TEXT("[GROUND] KHONG tim thay Landscape tai X=%.0f Y=%.0f"),
		Location.X, Location.Y);
	return 0.0f;
}

//tìm vị trí đỉnh
FVector AGraphDataLoader::FindVertexLocation(int32 VertexId) const
{
	for (const FVertexData& V : Vertices)
		if (V.Id == VertexId)
			return V.Location;
	return FVector::ZeroVector;
}
// ===== HÀM TÍNH TỔNG KHOẢNG CÁCH ĐƯỜNG ĐI =====
float AGraphDataLoader::CalculatePathDistance(const TArray<int32>& Path) const
{
	float TotalDistanceMeters = 0.0f; 

	for (int32 i = 0; i < Path.Num() - 1; i++)
	{
		int32 StartId = Path[i];       
		int32 EndId = Path[i + 1];      

		for (const FEdgeData& E : Edges)
		{
			if ((E.StartId == StartId && E.EndId == EndId) ||
				(E.StartId == EndId && E.EndId == StartId))
			{
				TotalDistanceMeters += E.Weight;
				break; 
			}
		}
	}
	return TotalDistanceMeters; 
}

//Sinh các Phases
void AGraphDataLoader::BuildFlightPhases(FFlightPath& Path, const TArray<int32>& Waypoints)
{
	Path.Phases.Empty();

	// Quãng đường (mét) và tốc độ (m/s)
	float DistanceMeters = CalculatePathDistance(Waypoints);
	float SpeedMeterPerSec = Path.Speed / 3.6f;
	// Độ vồng = 8% quãng đường, kẹp trong khoảng 300m..3000m
	float ArcHeightCm = FMath::Clamp(DistanceMeters * 0.20f, 300.0f, 6000.0f) * 100.0f;
	if (SpeedMeterPerSec <= 0.0f) SpeedMeterPerSec = 16.7f;   // phòng chia cho 0

	// ===== PHASE 1: BAY CONG — từ mặt đất (đơn vị) tới mục tiêu =====
	{
		FFlightPhase P;
		P.PhaseType = EFlightPhaseType::Cruise;
		P.TargetPos = Path.TargetPos;
		P.AltitudeAboveGround = Path.AttackAltitude;  
		P.ArcHeight = ArcHeightCm;           
		P.Duration = DistanceMeters / SpeedMeterPerSec;
		Path.Phases.Add(P);
	}

	// ===== PHASE 2: TẤN CÔNG =====
	{
		FFlightPhase P;
		P.PhaseType = EFlightPhaseType::Attack;
		P.TargetPos = Path.TargetPos;
		P.AltitudeAboveGround = Path.AttackAltitude;
		P.ArcHeight = 0.0f;
		P.Duration = 2.0f;
		Path.Phases.Add(P);
	}

	// ===== PHASE 3: QUAY VỀ (chỉ UAV chiến đấu) — cũng bay cong =====
	if (!Path.bIsKamikaze)
	{
		FFlightPhase P;
		P.PhaseType = EFlightPhaseType::Return;
		P.TargetPos = Path.StartUnitPos;
		P.AltitudeAboveGround = 0.0f;               // hạ về mặt đất tại đơn vị
		P.ArcHeight = ArcHeightCm;
		P.Duration = DistanceMeters / SpeedMeterPerSec;
		Path.Phases.Add(P);
	}
}

//Vẽ đường sau phân công
void AGraphDataLoader::DrawAssignmentPaths() {

	if (BestAssignment.IsEmpty()) return;
	int32 n = UAVs.Num();
	int32 m = Targets.Num();

	TMap<FString, int32> UnitStartVertex;
	TMap<FString, FVector> UnitStartPos;
	for (const FUnitData& U : Units)
	{
		if (U.VertexId >= 0)
			UnitStartVertex.Add(U.UnitId, U.VertexId);
		UnitStartPos.Add(U.UnitId, U.Location);
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

			// ===== LUU duong bay lai de UAV bay theo =====
			FFlightPath NewPath;
			NewPath.UAVCode = UAVs[i].UavCode;
			NewPath.TargetName = Targets[j].Name;
			NewPath.Color = Color;
			NewPath.Speed = (UAVs[i].Speed > 0.0f) ? UAVs[i].Speed : 60.0f;
			NewPath.bIsKamikaze = Type.Contains(TEXT("cảm tử")) || Type.Contains(TEXT("cam tu"));

			// Vị trí xuất phát và đích
			FVector* StartPos = UnitStartPos.Find(UAVs[i].UnitId);
			NewPath.StartUnitPos = StartPos ? *StartPos : FVector::ZeroVector;
			NewPath.TargetPos = Targets[j].VertexId > 0 ?
			FindVertexLocation(Targets[j].VertexId) : FVector::ZeroVector;

			NewPath.StartUnitPos.Z = GetGroundZAt(NewPath.StartUnitPos);
			NewPath.TargetPos.Z = GetGroundZAt(NewPath.TargetPos);
			//
			UE_LOG(LogTemp, Warning,
				TEXT("[MUCTIEU] %s | X=%.0f Y=%.0f | Dat_Z=%.0f (%.0f m)"),
				*Targets[j].Name, NewPath.TargetPos.X, NewPath.TargetPos.Y,
				NewPath.TargetPos.Z, NewPath.TargetPos.Z / 100.0f);

			// Độ cao cruise = 300 m 
			NewPath.CruiseAltitude = 30000.0f;

			// Cảm tử: tấn công sát mục tiêu; Chiến đấu: cách 10m
			//NewPath.AttackDistance = NewPath.bIsKamikaze ? 500.0f : 10000.0f;
			NewPath.AttackAltitude = NewPath.bIsKamikaze ? 0.0f : 4000.0f;  

			// ===== SINH CÁC PHASE =====
			BuildFlightPhases(NewPath, Path);

			// Chuyen chuoi vertex ID thanh chuoi toa do
			TMap<int32, FVector> PathLocMap;
			for (const FVertexData& V : Vertices)
				PathLocMap.Add(V.Id, V.Location);

			for (int32 Id : Path)
			{
				if (FVector* Loc = PathLocMap.Find(Id))
				{
					NewPath.Points.Add(*Loc);
				}
			}

			FlightPaths.Add(NewPath);
			// ===== het phan luu =====
			
			// Vẽ đường bay
			DrawPath(Path, Color, 2000.0f);

			PathCount++;

			UE_LOG(LogTemp, Warning,
				TEXT("Đường bay: %s → %s | Phases=%d | %s"),
				*UAVs[i].UavCode, *Targets[j].Name,
				NewPath.Phases.Num(),
				NewPath.bIsKamikaze ? TEXT("KAMIKAZE") : TEXT("COMBAT"));
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
		float Ej = Targets[j].ExplosiveReq;
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
			for (int32 i = 0; i < n; i++)
				BestAssignment[i][j] = 0;
		}
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
					Targets[j].ExplosiveReq,
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

 //Called every frame
void AGraphDataLoader::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		// Tab → toggle Dashboard
		if (PC->WasInputKeyJustPressed(EKeys::Tab))
		{
			ToggleDashboard();
		}

		// T → toggle Graph
		if (PC->WasInputKeyJustPressed(EKeys::T))
		{
			ToggleGraph();
		}
	}

}

void AGraphDataLoader::ToggleDashboard()
{
	// Tạo và đưa vào viewport ĐÚNG MỘT LẦN
	if (!DashboardWidget && DashboardWidgetClass)
	{
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		DashboardWidget = CreateWidget<UUserWidget>(PC, DashboardWidgetClass);
		if (DashboardWidget)
			DashboardWidget->AddToViewport();
	}
	if (!DashboardWidget) return;

	bDashboardVisible = !bDashboardVisible;
	DashboardWidget->SetVisibility(bDashboardVisible
		? ESlateVisibility::Visible
		: ESlateVisibility::Collapsed);
}

void AGraphDataLoader::ToggleGraph()
{
	bGraphVisible = !bGraphVisible;

	if (bGraphVisible)
	{
		// Bật Graph → vẽ cạnh
		DrawEdges();
		DrawSavedAssignmentPaths();
		for (AStaticMeshActor* Actor : VertexActors)
		{
			if (IsValid(Actor))
				Actor->SetActorHiddenInGame(false);
		}
		UE_LOG(LogTemp, Warning, TEXT("=== Graph HIỂN THỊ ==="));
	}
	else
	{
		// Tắt Graph → xóa tất cả persistent debug lines
		FlushPersistentDebugLines(GetWorld());
		for (AStaticMeshActor* Actor : VertexActors)
		{
			if (IsValid(Actor))
				Actor->SetActorHiddenInGame(true);
		}
		UE_LOG(LogTemp, Warning, TEXT("=== Graph ẨN ==="));
	}
}

FDashboardData AGraphDataLoader::GetDashboardData() const
{
	FDashboardData Data;

	const int32 NumUAV = UAVs.Num();
	const int32 NumTarget = Targets.Num();

	Data.TargetsTotal = NumTarget;

	// Tong gia tri quan su cua TAT CA muc tieu (mau so de tinh %)
	float TotalValue = 0.f;
	for (const FTargetData& T : Targets)
	{
		TotalValue += T.ValueUSD;
	}

	float DestroyedValue = 0.f;   // tong gia tri ky vong tieu diet
	int32 Covered = 0;     // so muc tieu du luong no

	// ===== Duyet tung MUC TIEU j =====
	for (int32 j = 0; j < NumTarget; ++j)
	{
		const FTargetData& Tgt = Targets[j];

		float Survival = 1.f;   // Sj = tich (1 - p_ij)
		float ExplosiveSum = 0.f;   // tong luong no cac UAV danh j

		// Duyet tung UAV i xem co danh muc tieu j khong
		for (int32 i = 0; i < NumUAV; ++i)
		{
			if (!BestAssignment.IsValidIndex(i))       continue;
			if (!BestAssignment[i].IsValidIndex(j))    continue;
			if (BestAssignment[i][j] != 1)             continue;   // UAV i KHONG danh j -> bo qua

			const FUAVData& U = UAVs[i];

			// p_ij: xac suat UAV i tieu diet muc tieu j
			float p = GetProbability(U.UavCode, Tgt.TargetId);
			Survival *= (1.f - p);          // nhan don xac suat song sot

			ExplosiveSum += U.Explosive;

			// --- Panel 2: them 1 dong phan cong ---
			FAssignmentRow Row;
			Row.UAVCode = U.UavCode;
			Row.TargetName = Tgt.Name;
			Row.KillProb = p;
			Data.Assignments.Add(Row);

			// --- Panel 4: cong don chi phi UAV ta dung ---
			Data.OurCost += U.CostUSD;   // MilitaryValue cua UAV = chi phi
		}

		float KillProb = 1.f - Survival;   // 1 - Sj = xac suat muc tieu bi diet

		// --- Panel 3: mot cot bieu do ---
		FTargetResult TR;
		TR.TargetName = Tgt.Name;
		TR.DestroyPercent = KillProb * 100.f;   // phan MAU DO
		TR.Priority = Tgt.Priority;
		Data.TargetResults.Add(TR);

		// Gia tri ky vong tieu diet cua muc tieu nay
		DestroyedValue += Tgt.ValueUSD * KillProb;

		// Du luong no chua? (tong no cac UAV >= no can thiet cua muc tieu)
		if (ExplosiveSum >= Tgt.ExplosiveReq)
		{
			Covered++;
		}
	}

	Data.TargetsCovered = Covered;
	Data.DestroyedValue = DestroyedValue;
	Data.NetBenefit = Data.DestroyedValue - Data.OurCost;

	// FitnessPercent = % gia tri dich bi tieu diet tren tong gia tri
	if (TotalValue > 0.f)
	{
		Data.FitnessPercent = (DestroyedValue / TotalValue) * 100.f;
	}

	return Data;
}

//Thực hiện hóa kết quả tối ưu từ GA và Dijkstra
void AGraphDataLoader::SpawnUAVs() {
	int32 SpawnCount = 0;
	for (const FFlightPath& FPath : FlightPaths)
	{
		if (FPath.Points.Num() < 1) continue;

		// Tra loai UAV tu UAVCode
		FString Type;
		for (const FUAVData& U : UAVs)
		{
			if (U.UavCode == FPath.UAVCode)
			{
				Type = U.UavType.ToLower();
				break;
			}
		}
		// Chon Blueprint theo loai
		bool bKamikaze = Type.Contains(TEXT("cảm tử")) || Type.Contains(TEXT("cam tu"));

		// Ưu tiên Blueprint riêng theo mã UAV
		TSubclassOf<AUAVPawn> ChosenClass = nullptr;
		if (TSubclassOf<AUAVPawn>* Found = UAVClassByCode.Find(FPath.UAVCode))
			ChosenClass = *Found;

		// Chưa khai báo riêng → dùng class chung theo loại như cũ
		if (ChosenClass == nullptr)
			ChosenClass = bKamikaze ? KamikazeClass : CombatClass;

		if (ChosenClass == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("UAV %s: chua gan Blueprint nao"), *FPath.UAVCode);
			continue;
		}

		// Spawn UAV tai diem xuat phat(unit)
		FVector SpawnPos = FPath.StartUnitPos;

		//Điều chỉnh Z để UAV nằm trên mặt đất
		FHitResult HitResult;
		FVector TraceStart = FVector(SpawnPos.X, SpawnPos.Y, 10000000.0f);
		FVector TraceEnd = FVector(SpawnPos.X, SpawnPos.Y, -10000000.0f);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(nullptr);

		if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
		{
			SpawnPos.Z = HitResult.ImpactPoint.Z + 500.0f;  // +500cm = 5m trên mặt đất
		}
		else
		{
			SpawnPos.Z += 5000.0f;  // Nếu không hit, thêm 50m
		}

		AUAVPawn* UAV = GetWorld()->SpawnActor<AUAVPawn>(
			ChosenClass, FPath.Points[0], FRotator::ZeroRotator);

		if (UAV)// nếu spawn thành công UAV
		{
			UAV->bIsKamikaze = bKamikaze;// thông báo loại UAV này
			UAV->UAVCode = FPath.UAVCode;
			ActiveUAVs.Add(UAV);

			UAV->StartFlight(FPath);// ra lệnh bay
			SpawnCount++;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("=== Đã spawn %d UAV ==="), SpawnCount);
}

FVector2D AGraphDataLoader::WorldToMinimap(const FVector& WorldPos) const
{
	// UE: X = Bac, Y = Dong.  Minimap: Bac o TREN, Dong o PHAI
	float u = (WorldPos.Y + MapExtentEast) / (2.f * MapExtentEast);   // ngang
	float v = (MapExtentNorth - WorldPos.X) / (2.f * MapExtentNorth);  // doc
	return FVector2D(FMath::Clamp(u, 0.f, 1.f), FMath::Clamp(v, 0.f, 1.f));
}

TArray<FMinimapMarker> AGraphDataLoader::GetMinimapMarkers()
{
	TArray<FMinimapMarker> Out;

	// --- duyet danh sach UAV, phat hien chiec vua bien mat ---
	for (int32 i = ActiveUAVs.Num() - 1; i >= 0; --i)
	{
		AUAVPawn* U = ActiveUAVs[i];

		if (!IsValid(U))
		{
			// UAV nổ -> neu la cam tu thi ghi lai vi tri de ve dau X
			if (FVector* Pos = LastKnownPos.Find(U))
			{
				DestroyedPositions.Add(*Pos);
				LastKnownPos.Remove(U);
			}
			ActiveUAVs.RemoveAt(i);
			continue;
		}

		LastKnownPos.Add(U, U->GetActorLocation());

		FMinimapMarker M;
		M.MapPos = WorldToMinimap(U->GetActorLocation());
		M.Heading = U->GetActorRotation().Yaw;
		M.Type = U->bIsKamikaze ? EMarkerType::Kamikaze : EMarkerType::Combat;
		M.UAVRef = U;
		Out.Add(M);
	}

	// --- Icon lua tai cac muc tieu da bi danh ---
	for (const FVector& P : BurningTargets)
	{
		FMinimapMarker M;
		M.MapPos = WorldToMinimap(P)+FVector2D(0.f, -0.015f);
		M.Type = EMarkerType::Fire;
		Out.Add(M);
	}

	// --- Dau X tai cac diem da no ---
	for (const FVector& P : DestroyedPositions)
	{
		FMinimapMarker M;
		M.MapPos = WorldToMinimap(P);
		M.Type = EMarkerType::Destroyed;
		Out.Add(M);
	}

	return Out;
}

void AGraphDataLoader::MarkTargetHit(const FVector& TargetPos)
{
	// Tranh trung lap khi 2 UAV cung danh mot muc tieu
	// Nguong 1 km (100.000 don vi UE)
	const float ThresholdSq = FMath::Square(100000.f);
	for (const FVector& P : BurningTargets)
		if (FVector::DistSquared(P, TargetPos) < ThresholdSq)
			return;

	BurningTargets.Add(TargetPos);
}

FMinimapData AGraphDataLoader::GetMinimapData()
{
	FMinimapData Data;

	// ===== PHẦN 1: Vẽ Graph (Cạnh) =====
	TMap<int32, FVector2D> VertexMapPos;  // Cache vị trí vertices trên minimap

	// Tính vị trí 2D của tất cả đỉnh
	for (const FVertexData& V : Vertices)
	{
		FVector2D MinimapPos = WorldToMinimap(V.Location);
		VertexMapPos.Add(V.Id, MinimapPos);
	}

	// Vẽ từng cạnh
	for (const FEdgeData& E : Edges)
	{
		FVector2D* StartPos = VertexMapPos.Find(E.StartId);
		FVector2D* EndPos = VertexMapPos.Find(E.EndId);
		if (StartPos && EndPos)
		{
			FMinimapEdge Edge;
			Edge.Start = *StartPos;
			Edge.End = *EndPos;
			Edge.Color = FLinearColor(0.3f, 0.3f, 0.3f, 0.8f);  // Xám
			Edge.Thickness = 1.0f;
			Data.GraphEdges.Add(Edge);
		}
	}

	// ===== PHẦN 1.5: VẼ VERTICES (SPHERE) — THÊM PHẦN NÀY =====
	for (const FVertexData& V : Vertices)
	{
		FVector2D MinimapPos = VertexMapPos.FindRef(V.Id);

		// Xác định màu theo loại vertex
		FString Type = V.TypeVertex.TrimStartAndEnd().ToLower();
		FLinearColor Color;
		float Size = 6.0f;

		if (Type == TEXT("target"))
		{
			Color = FLinearColor(0.1f, 0.5f, 1.0f, 1.0f);  // Xanh (khớp SpawnVertexActors)
			Size = 8.0f;
		}
		else if (Type == TEXT("unit"))
		{
			Color = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);  // Đỏ
			Size = 10.0f;
		}
		else
		{
			Color = FLinearColor(0.8f, 0.8f, 0.8f, 0.8f);  // Xám
			Size = 6.0f;
		}

		FMinimapObject Obj;
		Obj.Position = MinimapPos;
		Obj.Name = FString::Printf(TEXT("V%d"), V.Id);
		Obj.Type = TEXT("Vertex");
		Obj.Color = Color;
		Obj.Size = Size;
		Data.Objects.Add(Obj);
	}

	// ===== PHẦN 2: Vẽ Targets =====
	for (const FTargetData& T : Targets)
	{
		if (T.VertexId <= 0) continue;
		FVector2D MinimapPos = VertexMapPos.FindRef(T.VertexId);

		FMinimapObject Obj;
		Obj.Position = MinimapPos;
		Obj.Name = T.Name;
		Obj.Type = TEXT("Target");
		Obj.Color = FLinearColor(0.1f, 0.5f, 1.0f, 1.0f);  // Xanh
		Obj.Size = 10.0f;
		Data.Objects.Add(Obj);
	}

	// ===== PHẦN 3: Vẽ Units =====
	for (const FUnitData& U : Units)
	{
		FVector2D MinimapPos = WorldToMinimap(U.Location);
		FMinimapObject Obj;
		Obj.Position = MinimapPos;
		Obj.Name = U.UnitName;
		Obj.Type = TEXT("Unit");
		Obj.Color = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);  // Đỏ
		Obj.Size = 12.0f;
		Data.Objects.Add(Obj);
	}

	// ===== PHẦN 4: Vẽ UAVs (đang bay) =====
	for (const AUAVPawn* UAV : ActiveUAVs)
	{
		if (!IsValid(UAV)) continue;
		FVector2D MinimapPos = WorldToMinimap(UAV->GetActorLocation());
		FMinimapObject Obj;
		Obj.Position = MinimapPos;
		Obj.Name = UAV->UAVCode;
		Obj.Type = UAV->bIsKamikaze ? TEXT("Kamikaze") : TEXT("Combat");
		Obj.Color = UAV->bIsKamikaze ?
			FLinearColor(1.0f, 0.3f, 0.0f, 1.0f) :  // Cam
			FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);   // Xanh lá
		Obj.Size = 12.0f;
		Data.Objects.Add(Obj);
	}

	return Data;
}




















