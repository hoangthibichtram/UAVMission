
#include "MinimapWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "GraphDataLoader.h"

// ===== ĐƯỢC GỌI KHI WIDGET ĐƯỢC TẠO (lần đầu) =====
void UMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// ===== TÌM GRAPHDATALOADERS TRONG WORLD =====
	for (TActorIterator<AGraphDataLoader> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		GraphDataLoader = *ActorItr;
		UE_LOG(LogTemp, Warning, TEXT("[Minimap] Found GraphDataLoader"));
		break;
	}

	// ===== KIỂM TRA =====
	if (!GraphDataLoader)
	{
		UE_LOG(LogTemp, Error, TEXT("[Minimap] GraphDataLoader NOT found!"));
	}

	if (!Canvas_Graph)
	{
		UE_LOG(LogTemp, Error, TEXT("[Minimap] Canvas_Graph NOT bound! Check Designer."));
	}

	if (!Canvas_Objects)
	{
		UE_LOG(LogTemp, Error, TEXT("[Minimap] Canvas_Objects NOT bound! Check Designer."));
	}
}

// ===== ĐƯỢC GỌI MỖI FRAME ĐỂ UPDATE =====
void UMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// ===== KIỂM TRA REFERENCE =====
	if (!GraphDataLoader || !Canvas_Graph || !Canvas_Objects)
		return;

	// ===== XÓA CANVAS CŨ =====
	Canvas_Graph->ClearChildren();
	Canvas_Objects->ClearChildren();

	// ===== LẤY DỮ LIỆU =====
	FMinimapData Data = GraphDataLoader->GetMinimapData();

	// ===== VẼ =====
	DrawGraphOnCanvas(Data);
	DrawObjectsOnCanvas(Data);
}

// ===== VẼ CẠNH GRAPH =====
void UMinimapWidget::DrawGraphOnCanvas(const FMinimapData& Data)
{
	if (!Canvas_Graph) return;
	UE_LOG(LogTemp, Warning, TEXT("[DrawGraph] Edges count: %d"), Data.GraphEdges.Num());
	for (const FMinimapEdge& Edge : Data.GraphEdges)
	{
		// ===== TÍNH TOẠ ĐỘ SCREEN =====
		FVector2D Start = Edge.Start * FVector2D(MinimapWidth, MinimapHeight);
		FVector2D End = Edge.End * FVector2D(MinimapWidth, MinimapHeight);
		FVector2D Dir = (End - Start).GetSafeNormal();
		float Dist = FVector2D::Distance(Start, End);

		if (Dist < 1.0f) continue;

		// ===== TẠO IMAGE =====
		UImage* LineImage = NewObject<UImage>(Canvas_Graph);
		LineImage->SetColorAndOpacity(Edge.Color);

		// ===== THÊM VÀO CANVAS =====
		UCanvasPanelSlot* EdgeSlot = Canvas_Graph->AddChildToCanvas(LineImage);  
		EdgeSlot->SetPosition(Start);
		EdgeSlot->SetSize(FVector2D(Dist, 1.0f));
		EdgeSlot->SetAnchors(FAnchors(0, 0));
	}
}


void UMinimapWidget::DrawObjectsOnCanvas(const FMinimapData& Data)
{
	if (!Canvas_Objects) return;

	// ===== PHẦN 1: VẼ TARGETS, UNITS, VERTICES =====
	for (const FMinimapObject& Obj : Data.Objects)
	{
		FVector2D ScreenPos = Obj.Position * FVector2D(MinimapWidth, MinimapHeight);
		float Size = Obj.Size;

		UImage* IconImage = NewObject<UImage>(Canvas_Objects);

		// ===== KIỂM TRA LOẠI =====
		if (Obj.Type == TEXT("Unit"))
		{
			// Unit: Dùng chung 1 icon
			UTexture2D* UnitTexture = LoadObject<UTexture2D>(nullptr,
				TEXT("/Game/UI/MiniMap/Icon/Unit.Unit"));

			if (UnitTexture)
			{
				IconImage->SetBrushFromTexture(UnitTexture);
				IconImage->SetColorAndOpacity(FLinearColor::White);
			}
			else
			{
				IconImage->SetColorAndOpacity(Obj.Color);
			}
			Size = 12.0f;
		}
		else if (Obj.Type == TEXT("Target"))
		{
			// Target: Mỗi target khác icon (dựa vào tên)
			UTexture2D* TargetTexture = nullptr;

			// Gán icon khác nhau cho từng target
			if (Obj.Name == TEXT("T01") || Obj.Name.Contains(TEXT("Radar")))
			{
				TargetTexture = LoadObject<UTexture2D>(nullptr,
					TEXT("/Game/UI/MiniMap/Icon/rada.rada"));
			}
			else if (Obj.Name == TEXT("T02") || Obj.Name.Contains(TEXT("Silo")))
			{
				TargetTexture = LoadObject<UTexture2D>(nullptr,
					TEXT("/Game/UI/MiniMap/Icon/SCH.SCH"));
			}
			else if (Obj.Name == TEXT("T03") || Obj.Name.Contains(TEXT("Command")))
			{
				TargetTexture = LoadObject<UTexture2D>(nullptr,
					TEXT("/Game/UI/MiniMap/Icon/EW.EW"));
			}
			else
			{
				// Target khác → icon mặc định
				TargetTexture = LoadObject<UTexture2D>(nullptr,
					TEXT("/Game/UI/MiniMap/Icon/target_icon.target_icon"));
			}

			if (TargetTexture)
			{
				IconImage->SetBrushFromTexture(TargetTexture);
				IconImage->SetColorAndOpacity(FLinearColor::White);
			}
			else
			{
				IconImage->SetColorAndOpacity(Obj.Color);
			}
			Size = 10.0f;
		}
		else 
		{
			continue;
		}

		// ===== THÊM VÀO CANVAS (centered) =====
		UCanvasPanelSlot* ObjSlot = Canvas_Objects->AddChildToCanvas(IconImage);  
		ObjSlot->SetPosition(ScreenPos - FVector2D(Size / 2.0f, Size / 2.0f));
		ObjSlot->SetSize(FVector2D(Size, Size));
		ObjSlot->SetAnchors(FAnchors(0, 0));
	}
	// ===== PHẦN 2: VẼ UAV + FIRE + DESTROYED =====
	TArray<FMinimapMarker> Markers = GraphDataLoader->GetMinimapMarkers();
	for (const FMinimapMarker& Marker : Markers)
	{
		FVector2D ScreenPos = Marker.MapPos * FVector2D(MinimapWidth, MinimapHeight);
		float Size = 12.0f;

		// ===== KỀU BÁO TEXTURE (QUAN TRỌNG!) =====
		UTexture2D* Texture = nullptr;

		// ===== SET TEXTURE VÀ KÍCH THƯỚC =====
		switch (Marker.Type)
		{
		case EMarkerType::Combat:
			Texture = LoadObject<UTexture2D>(nullptr,
				TEXT("/Game/UI/MiniMap/Icon/Arrow_blue.Arrow_blue"));
			Size = 12.0f;
			break;
		case EMarkerType::Kamikaze:
			Texture = LoadObject<UTexture2D>(nullptr,
				TEXT("/Game/UI/MiniMap/Icon/Arrow_red.Arrow_red"));
			Size = 12.0f;
			break;
		case EMarkerType::Fire:
			Texture = LoadObject<UTexture2D>(nullptr,
				TEXT("/Game/UI/MiniMap/Icon/fire.fire"));
			Size = 10.0f;
			break;
		case EMarkerType::Destroyed:
			Texture = LoadObject<UTexture2D>(nullptr,
				TEXT("/Game/UI/MiniMap/Icon/X_red.X_red"));
			Size = 8.0f;
			break;
		default:
			Texture = nullptr;
			Size = 10.0f;
		}

		// ===== TẠO IMAGE VÀ GÁN TEXTURE =====
		if (Texture)
		{
			UImage* IconImage = NewObject<UImage>(Canvas_Objects);
			IconImage->SetBrushFromTexture(Texture);
			IconImage->SetColorAndOpacity(FLinearColor::White);

			// ===== THÊM VÀO CANVAS (centered) =====
			UCanvasPanelSlot* UAVSlot = Canvas_Objects->AddChildToCanvas(IconImage);
			UAVSlot->SetPosition(ScreenPos - FVector2D(Size / 2.0f, Size / 2.0f));
			UAVSlot->SetSize(FVector2D(Size, Size));
			UAVSlot->SetAnchors(FAnchors(0, 0));
		}
	}
}