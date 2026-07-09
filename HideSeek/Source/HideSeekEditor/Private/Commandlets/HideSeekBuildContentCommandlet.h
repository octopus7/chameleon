#pragma once

#include "Commandlets/Commandlet.h"
#include "HideSeekBuildContentCommandlet.generated.h"

UCLASS()
class HIDESEEKEDITOR_API UHideSeekBuildContentCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UHideSeekBuildContentCommandlet();

	virtual int32 Main(const FString& Params) override;
};
