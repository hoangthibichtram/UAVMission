// Fill out your copyright notice in the Description page of Project Settings.


#include "GraphDataLoader.h"
#include "Misc/FileHelper.h"      
#include "Misc/Paths.h"  
#include "Engine/StaticMeshActor.h"    //để spawn Static Mesh
#include "Components/StaticMeshComponent.h" //để set mesh

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
	//đọc dl đỉnh
	bool bSuccess = LoadVerticesFromCSV(TEXT("Vertex1.csv"));
	if (bSuccess) {
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
		}
	}
	UE_LOG(LogTemp, Warning,
		TEXT("=== Đã spawn %d quả cầu vào scene ==="), Vertices.Num());
}
//Hàm đọc file đỉnh
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


// Called every frame
//void AGraphDataLoader::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//}

