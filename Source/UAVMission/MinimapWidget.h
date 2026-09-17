// MinimapWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GraphTypes.h"
#include "EngineUtils.h"
#include "MinimapWidget.generated.h"

UCLASS()
class UAVMISSION_API UMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ===== LIFECYCLE =====
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ===== CANVAS PANELS (phải match tên trong Designer) =====
	UPROPERTY(meta = (BindWidget))
	class UCanvasPanel* Canvas_Graph;

	UPROPERTY(meta = (BindWidget))
	class UCanvasPanel* Canvas_Objects;

	// ===== KÍCH THƯỚC MINIMAP =====
	UPROPERTY(EditAnywhere, Category = "Minimap")
	float MinimapWidth = 512.0f;

	UPROPERTY(EditAnywhere, Category = "Minimap")
	float MinimapHeight = 384.0f;

	// ===== REFERENCE TỚI GRAPHDATALOADERS =====
	UPROPERTY()
	class AGraphDataLoader* GraphDataLoader;

private:
	// ===== HÀM VẼ =====
	void DrawGraphOnCanvas(const FMinimapData& Data);
	void DrawObjectsOnCanvas(const FMinimapData& Data);
};