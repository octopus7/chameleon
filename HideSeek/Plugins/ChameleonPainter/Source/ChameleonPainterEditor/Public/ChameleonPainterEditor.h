#pragma once

#include "Modules/ModuleInterface.h"

class FChameleonPainterEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
