#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ChameleonPainterGameInstance.generated.h"

class UChameleonPainterInputConfig;

UCLASS(BlueprintType, Blueprintable)
class CHAMELEONPAINTER_API UChameleonPainterGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chameleon|Input")
	TObjectPtr<UChameleonPainterInputConfig> ChameleonInputConfig;
};
