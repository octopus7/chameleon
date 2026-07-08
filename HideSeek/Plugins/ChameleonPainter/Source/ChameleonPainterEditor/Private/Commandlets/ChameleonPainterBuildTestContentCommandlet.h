#pragma once

#include "Commandlets/Commandlet.h"
#include "ChameleonPainterBuildTestContentCommandlet.generated.h"

UCLASS()
class CHAMELEONPAINTEREDITOR_API UChameleonPainterBuildTestContentCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UChameleonPainterBuildTestContentCommandlet();

	virtual int32 Main(const FString& Params) override;
};
