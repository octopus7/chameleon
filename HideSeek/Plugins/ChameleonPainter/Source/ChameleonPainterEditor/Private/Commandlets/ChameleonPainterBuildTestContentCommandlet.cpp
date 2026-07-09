#include "Commandlets/ChameleonPainterBuildTestContentCommandlet.h"

#include "Actors/ChameleonHiderBodyActor.h"
#include "Actors/ChameleonPaintSprayActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Camera/CameraActor.h"
#include "Character/ChameleonHiderCharacter.h"
#include "Components/ChameleonMetaballBodyComponent.h"
#include "Components/ChameleonPaintComponent.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Data/ChameleonPainterInputConfig.h"
#include "Editor.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Texture2D.h"
#include "EnhancedActionKeyMapping.h"
#include "FileHelpers.h"
#include "Factories/TextureFactory.h"
#include "Game/ChameleonPainterGameInstance.h"
#include "Game/ChameleonPainterGameMode.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "GameMapsSettings.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionMax.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "MaterialEditingLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"
#include "UI/ChameleonColorPickerWidget.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

DEFINE_LOG_CATEGORY_STATIC(LogChameleonPainterContent, Log, All);

namespace
{
const FString RootPath = TEXT("/Game/ChameleonPainterTest");
const FString TexturePath = RootPath / TEXT("Textures");
const FString MaterialPath = RootPath / TEXT("Materials");
const FString InputPath = RootPath / TEXT("Input");
const FString BlueprintPath = RootPath / TEXT("Blueprints");
const FString UIPath = RootPath / TEXT("UI");
const FString MapPath = RootPath / TEXT("Maps");
constexpr float ColorPickerWidgetWidth = 400.0f;
constexpr float ColorPickerWidgetHeight = 340.0f;

FString ToObjectPath(const FString& PackageName)
{
	return PackageName + TEXT(".") + FPackageName::GetLongPackageAssetName(PackageName);
}

template <typename T>
T* LoadAssetByPackageName(const FString& PackageName)
{
	return Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *ToObjectPath(PackageName)));
}

template <typename T>
T* FindOrCreateAsset(const FString& PackageName)
{
	if (T* ExistingAsset = LoadAssetByPackageName<T>(PackageName))
	{
		return ExistingAsset;
	}

	UPackage* Package = CreatePackage(*PackageName);
	Package->FullyLoad();

	const FName AssetName(*FPackageName::GetLongPackageAssetName(PackageName));
	T* Asset = NewObject<T>(Package, T::StaticClass(), AssetName, RF_Public | RF_Standalone | RF_Transactional);
	FAssetRegistryModule::AssetCreated(Asset);
	Asset->MarkPackageDirty();
	return Asset;
}

bool SavePackageForObject(UObject* Asset)
{
	if (!Asset)
	{
		return false;
	}

	UPackage* Package = Asset->GetPackage();
	const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	SaveArgs.Error = GWarn;

	return UPackage::SavePackage(Package, nullptr, *Filename, SaveArgs);
}

void SaveAssets(const TArray<UObject*>& Assets)
{
	for (UObject* Asset : Assets)
	{
		if (Asset)
		{
			SavePackageForObject(Asset);
		}
	}
}

TMap<FName, UTexture2D*> ImportTextures()
{
	const FString SourceDirectory = FPaths::Combine(FPaths::ProjectDir(), TEXT("SourceAssets/Textures/ChameleonPainterTest"));
	const TArray<FString> SourceFiles = {
		TEXT("T_CPT_Concrete.png"),
		TEXT("T_CPT_Wood.png"),
		TEXT("T_CPT_GreenPanel.png"),
		TEXT("T_CPT_FloorTile.png")
	};

	TMap<FName, UTexture2D*> Textures;
	for (const FString& SourceFile : SourceFiles)
	{
		const FString AssetName = FPaths::GetBaseFilename(SourceFile);
		const FString PackageName = TexturePath / AssetName;
		const FString FullPath = FPaths::Combine(SourceDirectory, SourceFile);

		UTexture2D* Texture = LoadAssetByPackageName<UTexture2D>(PackageName);
		if (!Texture && FPaths::FileExists(FullPath))
		{
			UPackage* Package = CreatePackage(*PackageName);
			Package->FullyLoad();

			UTextureFactory* TextureFactory = NewObject<UTextureFactory>();
			TextureFactory->AddToRoot();
			TextureFactory->bCreateMaterial = false;
			TextureFactory->ColorSpaceMode = ETextureSourceColorSpace::SRGB;
			UTextureFactory::SuppressImportOverwriteDialog(true);

			Texture = Cast<UTexture2D>(UFactory::StaticImportObject(
				UTexture2D::StaticClass(),
				Package,
				*AssetName,
				RF_Public | RF_Standalone | RF_Transactional,
				*FullPath,
				nullptr,
				TextureFactory,
				nullptr,
				GWarn));

			TextureFactory->RemoveFromRoot();
			if (Texture)
			{
				FAssetRegistryModule::AssetCreated(Texture);
			}
		}
		else if (!Texture)
		{
			UE_LOG(LogChameleonPainterContent, Warning, TEXT("Texture source file is missing: %s"), *FullPath);
		}

		if (Texture)
		{
			Texture->SRGB = true;
			Texture->MarkPackageDirty();
			Textures.Add(*AssetName, Texture);
			SavePackageForObject(Texture);
		}
	}

	return Textures;
}

UTexture2D* ImportBrushCursorTexture()
{
	const FString SourceFile = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("SourceAssets/UI/ChameleonPainter/T_ChameleonBrushCursor.png"));
	const FString PackageName = UIPath / TEXT("T_ChameleonBrushCursor");

	UTexture2D* Texture = LoadAssetByPackageName<UTexture2D>(PackageName);
	if (FPaths::FileExists(SourceFile))
	{
		UPackage* Package = CreatePackage(*PackageName);
		Package->FullyLoad();
		const bool bHadExistingTexture = Texture != nullptr;

		UTextureFactory* TextureFactory = NewObject<UTextureFactory>();
		TextureFactory->AddToRoot();
		TextureFactory->NoAlpha = false;
		TextureFactory->CompressionSettings = TC_EditorIcon;
		TextureFactory->LODGroup = TEXTUREGROUP_UI;
		TextureFactory->MipGenSettings = TMGS_NoMipmaps;
		TextureFactory->bCreateMaterial = false;
		TextureFactory->ColorSpaceMode = ETextureSourceColorSpace::SRGB;
		UTextureFactory::SuppressImportOverwriteDialog(true);

		UTexture2D* ImportedTexture = Cast<UTexture2D>(UFactory::StaticImportObject(
			UTexture2D::StaticClass(),
			Package,
			TEXT("T_ChameleonBrushCursor"),
			RF_Public | RF_Standalone | RF_Transactional,
			*SourceFile,
			nullptr,
			TextureFactory,
			nullptr,
			GWarn));

		TextureFactory->RemoveFromRoot();
		if (ImportedTexture)
		{
			Texture = ImportedTexture;
			if (!bHadExistingTexture)
			{
				FAssetRegistryModule::AssetCreated(Texture);
			}
		}
	}
	else if (!Texture)
	{
		UE_LOG(LogChameleonPainterContent, Warning, TEXT("Brush cursor source file is missing: %s"), *SourceFile);
	}

	if (Texture)
	{
		Texture->SRGB = true;
		Texture->LODGroup = TEXTUREGROUP_UI;
		Texture->CompressionSettings = TC_EditorIcon;
		Texture->MipGenSettings = TMGS_NoMipmaps;
		Texture->MarkPackageDirty();
		SavePackageForObject(Texture);
	}

	return Texture;
}

UMaterial* CreateHiderPaintMaterial()
{
	UMaterial* Material = FindOrCreateAsset<UMaterial>(MaterialPath / TEXT("M_CPT_HiderPaint"));
	Material->PreEditChange(nullptr);
	UMaterialEditingLibrary::DeleteAllMaterialExpressions(Material);

	UMaterialExpressionTextureSampleParameter2D* BaseColorTexture = Cast<UMaterialExpressionTextureSampleParameter2D>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionTextureSampleParameter2D::StaticClass(), -520, -100));
	UMaterialExpressionTextureSampleParameter2D* RoughnessTexture = Cast<UMaterialExpressionTextureSampleParameter2D>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionTextureSampleParameter2D::StaticClass(), -520, 120));
	UMaterialExpressionTextureSampleParameter2D* MetallicTexture = Cast<UMaterialExpressionTextureSampleParameter2D>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionTextureSampleParameter2D::StaticClass(), -520, 300));

	UTexture2D* WhiteTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	UTexture2D* BlackTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/Black.Black"));

	if (BaseColorTexture)
	{
		BaseColorTexture->ParameterName = TEXT("BaseColorPaintTexture");
		BaseColorTexture->Texture = WhiteTexture;
		BaseColorTexture->AutoSetSampleType();
	}
	if (RoughnessTexture)
	{
		RoughnessTexture->ParameterName = TEXT("RoughnessPaintTexture");
		RoughnessTexture->Texture = WhiteTexture;
		RoughnessTexture->SamplerType = SAMPLERTYPE_Color;
	}
	if (MetallicTexture)
	{
		MetallicTexture->ParameterName = TEXT("MetallicPaintTexture");
		MetallicTexture->Texture = BlackTexture ? BlackTexture : WhiteTexture;
		MetallicTexture->SamplerType = SAMPLERTYPE_Color;
	}

	UMaterialEditingLibrary::ConnectMaterialProperty(BaseColorTexture, TEXT(""), MP_BaseColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(RoughnessTexture, TEXT("R"), MP_Roughness);
	UMaterialEditingLibrary::ConnectMaterialProperty(MetallicTexture, TEXT("R"), MP_Metallic);
	UMaterialEditingLibrary::RecompileMaterial(Material);
	Material->PostEditChange();
	Material->MarkPackageDirty();
	SavePackageForObject(Material);
	return Material;
}

UMaterial* CreatePaintSprayMaterial()
{
	UMaterial* Material = FindOrCreateAsset<UMaterial>(MaterialPath / TEXT("M_CPT_PaintSpray"));
	Material->PreEditChange(nullptr);
	UMaterialEditingLibrary::DeleteAllMaterialExpressions(Material);
	Material->BlendMode = BLEND_Translucent;
	Material->TwoSided = true;
	Material->SetShadingModel(MSM_Unlit);

	UMaterialExpressionVertexColor* VertexColor = Cast<UMaterialExpressionVertexColor>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionVertexColor::StaticClass(), -640, -40));
	UMaterialExpressionScalarParameter* OpacityParameter = Cast<UMaterialExpressionScalarParameter>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionScalarParameter::StaticClass(), -640, 180));
	UMaterialExpressionMultiply* OpacityMultiply = Cast<UMaterialExpressionMultiply>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionMultiply::StaticClass(), -320, 150));

	if (OpacityParameter)
	{
		OpacityParameter->ParameterName = TEXT("PaintSprayOpacity");
		OpacityParameter->DefaultValue = 1.0f;
	}

	UMaterialEditingLibrary::ConnectMaterialProperty(VertexColor, TEXT(""), MP_EmissiveColor);
	UMaterialEditingLibrary::ConnectMaterialExpressions(VertexColor, TEXT("A"), OpacityMultiply, TEXT("A"));
	UMaterialEditingLibrary::ConnectMaterialExpressions(OpacityParameter, TEXT(""), OpacityMultiply, TEXT("B"));
	UMaterialEditingLibrary::ConnectMaterialProperty(OpacityMultiply, TEXT(""), MP_Opacity);

	UMaterialEditingLibrary::RecompileMaterial(Material);
	Material->PostEditChange();
	Material->MarkPackageDirty();
	SavePackageForObject(Material);
	return Material;
}

UMaterial* CreateSurfaceMaterial(const FString& AssetName, UTexture2D* Texture, FLinearColor SampleColor)
{
	UMaterial* Material = FindOrCreateAsset<UMaterial>(MaterialPath / AssetName);
	Material->PreEditChange(nullptr);
	UMaterialEditingLibrary::DeleteAllMaterialExpressions(Material);

	UMaterialExpressionVectorParameter* SampleColorParameter = Cast<UMaterialExpressionVectorParameter>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionVectorParameter::StaticClass(), -640, -40));
	if (SampleColorParameter)
	{
		SampleColorParameter->ParameterName = TEXT("ChameleonSampleColor");
		SampleColorParameter->DefaultValue = SampleColor;
	}

	UMaterialExpressionConstant* Roughness = Cast<UMaterialExpressionConstant>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionConstant::StaticClass(), -640, 140));
	if (Roughness)
	{
		Roughness->R = 0.78f;
	}

	if (Texture)
	{
		UMaterialExpressionTextureSample* TextureSample = Cast<UMaterialExpressionTextureSample>(
			UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionTextureSample::StaticClass(), -640, -260));
		UMaterialExpressionMultiply* Multiply = Cast<UMaterialExpressionMultiply>(
			UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionMultiply::StaticClass(), -300, -150));

		if (TextureSample)
		{
			TextureSample->Texture = Texture;
			TextureSample->AutoSetSampleType();
		}

		UMaterialEditingLibrary::ConnectMaterialExpressions(TextureSample, TEXT(""), Multiply, TEXT("A"));
		UMaterialEditingLibrary::ConnectMaterialExpressions(SampleColorParameter, TEXT(""), Multiply, TEXT("B"));
		UMaterialEditingLibrary::ConnectMaterialProperty(Multiply, TEXT(""), MP_BaseColor);
	}
	else
	{
		UMaterialEditingLibrary::ConnectMaterialProperty(SampleColorParameter, TEXT(""), MP_BaseColor);
	}

	UMaterialEditingLibrary::ConnectMaterialProperty(Roughness, TEXT(""), MP_Roughness);
	UMaterialEditingLibrary::RecompileMaterial(Material);
	Material->PostEditChange();
	Material->MarkPackageDirty();
	SavePackageForObject(Material);
	return Material;
}

UMaterial* CreateBrushCursorMaterial(UTexture2D* CursorTexture)
{
	UMaterial* Material = FindOrCreateAsset<UMaterial>(UIPath / TEXT("M_CPT_BrushCursor_UI"));
	Material->PreEditChange(nullptr);
	UMaterialEditingLibrary::DeleteAllMaterialExpressions(Material);
	Material->MaterialDomain = MD_UI;
	Material->BlendMode = BLEND_Translucent;

	UMaterialExpressionTextureSample* TextureSample = Cast<UMaterialExpressionTextureSample>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionTextureSample::StaticClass(), -640, -80));
	if (TextureSample)
	{
		TextureSample->Texture = CursorTexture;
		TextureSample->AutoSetSampleType();
	}

	UMaterialExpressionMax* MaxRG = Cast<UMaterialExpressionMax>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionMax::StaticClass(), -120, 140));
	UMaterialExpressionMax* MaxRGB = Cast<UMaterialExpressionMax>(
		UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionMax::StaticClass(), 120, 220));

	UMaterialEditingLibrary::ConnectMaterialExpressions(TextureSample, TEXT("R"), MaxRG, TEXT("A"));
	UMaterialEditingLibrary::ConnectMaterialExpressions(TextureSample, TEXT("G"), MaxRG, TEXT("B"));
	UMaterialEditingLibrary::ConnectMaterialExpressions(MaxRG, TEXT(""), MaxRGB, TEXT("A"));
	UMaterialEditingLibrary::ConnectMaterialExpressions(TextureSample, TEXT("B"), MaxRGB, TEXT("B"));
	UMaterialEditingLibrary::ConnectMaterialProperty(TextureSample, TEXT(""), MP_EmissiveColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(MaxRGB, TEXT(""), MP_Opacity);

	UMaterialEditingLibrary::RecompileMaterial(Material);
	Material->PostEditChange();
	Material->MarkPackageDirty();
	SavePackageForObject(Material);
	return Material;
}

UInputAction* CreateInputActionAsset(const FString& AssetName, EInputActionValueType ValueType)
{
	UInputAction* Action = FindOrCreateAsset<UInputAction>(InputPath / AssetName);
	Action->ValueType = ValueType;
	Action->MarkPackageDirty();
	SavePackageForObject(Action);
	return Action;
}

void AddSwizzle(UInputMappingContext* Context, FEnhancedActionKeyMapping& Mapping)
{
	UInputModifierSwizzleAxis* Modifier = NewObject<UInputModifierSwizzleAxis>(Context, NAME_None, RF_Transactional);
	Modifier->Order = EInputAxisSwizzle::YXZ;
	Mapping.Modifiers.Add(Modifier);
}

void AddNegate(UInputMappingContext* Context, FEnhancedActionKeyMapping& Mapping, bool bX, bool bY, bool bZ)
{
	UInputModifierNegate* Modifier = NewObject<UInputModifierNegate>(Context, NAME_None, RF_Transactional);
	Modifier->bX = bX;
	Modifier->bY = bY;
	Modifier->bZ = bZ;
	Mapping.Modifiers.Add(Modifier);
}

FEnhancedActionKeyMapping& MapKey(UInputMappingContext* Context, UInputAction* Action, FKey Key)
{
	return Context->MapKey(Action, Key);
}

UChameleonPainterInputConfig* CreateInputAssets()
{
	UInputAction* MoveAction = CreateInputActionAsset(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	MoveAction->AccumulationBehavior = EInputActionAccumulationBehavior::Cumulative;
	MoveAction->MarkPackageDirty();
	SavePackageForObject(MoveAction);
	UInputAction* LookAction = CreateInputActionAsset(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	LookAction->AccumulationBehavior = EInputActionAccumulationBehavior::Cumulative;
	LookAction->MarkPackageDirty();
	SavePackageForObject(LookAction);
	UInputAction* JumpAction = CreateInputActionAsset(TEXT("IA_Jump"), EInputActionValueType::Boolean);
	UInputAction* PaintAction = CreateInputActionAsset(TEXT("IA_Paint"), EInputActionValueType::Boolean);
	UInputAction* SampleColorAction = CreateInputActionAsset(TEXT("IA_SampleColor"), EInputActionValueType::Boolean);
	UInputAction* ToggleColorPickerAction = CreateInputActionAsset(TEXT("IA_ToggleColorPicker"), EInputActionValueType::Boolean);

	UInputMappingContext* MappingContext = FindOrCreateAsset<UInputMappingContext>(InputPath / TEXT("IMC_ChameleonPlayer"));
	MappingContext->UnmapAll();

	FEnhancedActionKeyMapping& MoveW = MapKey(MappingContext, MoveAction, EKeys::W);
	AddSwizzle(MappingContext, MoveW);
	FEnhancedActionKeyMapping& MoveS = MapKey(MappingContext, MoveAction, EKeys::S);
	AddSwizzle(MappingContext, MoveS);
	AddNegate(MappingContext, MoveS, false, true, false);
	FEnhancedActionKeyMapping& MoveD = MapKey(MappingContext, MoveAction, EKeys::D);
	(void)MoveD;
	FEnhancedActionKeyMapping& MoveA = MapKey(MappingContext, MoveAction, EKeys::A);
	AddNegate(MappingContext, MoveA, true, false, false);

	MapKey(MappingContext, LookAction, EKeys::MouseX);
	FEnhancedActionKeyMapping& LookY = MapKey(MappingContext, LookAction, EKeys::MouseY);
	AddSwizzle(MappingContext, LookY);

	MapKey(MappingContext, JumpAction, EKeys::SpaceBar);
	MapKey(MappingContext, PaintAction, EKeys::LeftMouseButton);
	MapKey(MappingContext, SampleColorAction, EKeys::E);
	MapKey(MappingContext, ToggleColorPickerAction, EKeys::F);

	MappingContext->MarkPackageDirty();
	SavePackageForObject(MappingContext);

	UChameleonPainterInputConfig* InputConfig = FindOrCreateAsset<UChameleonPainterInputConfig>(InputPath / TEXT("DA_ChameleonPainterInputConfig"));
	InputConfig->DefaultMappingContext = MappingContext;
	InputConfig->MoveAction = MoveAction;
	InputConfig->LookAction = LookAction;
	InputConfig->JumpAction = JumpAction;
	InputConfig->PaintAction = PaintAction;
	InputConfig->SampleColorAction = SampleColorAction;
	InputConfig->ToggleColorPickerAction = ToggleColorPickerAction;
	InputConfig->MarkPackageDirty();
	SavePackageForObject(InputConfig);

	return InputConfig;
}

UBlueprint* FindOrCreateBlueprint(const FString& PackageName, UClass* ParentClass)
{
	if (UBlueprint* ExistingBlueprint = LoadAssetByPackageName<UBlueprint>(PackageName))
	{
		if (ExistingBlueprint->ParentClass != ParentClass)
		{
			ExistingBlueprint->ParentClass = ParentClass;
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(ExistingBlueprint);
		}
		FKismetEditorUtilities::CompileBlueprint(ExistingBlueprint);
		return ExistingBlueprint;
	}

	UPackage* Package = CreatePackage(*PackageName);
	Package->FullyLoad();
	const FName AssetName(*FPackageName::GetLongPackageAssetName(PackageName));
	UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
		ParentClass,
		Package,
		AssetName,
		BPTYPE_Normal,
		UBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass());
	FAssetRegistryModule::AssetCreated(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	Blueprint->MarkPackageDirty();
	return Blueprint;
}

UTextBlock* CreateWidgetText(UWidgetTree* WidgetTree, const FName& Name, const FText& Text)
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	TextBlock->SetText(Text);
	TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	return TextBlock;
}

FLinearColor GetDefaultBrushColor()
{
	return FLinearColor(1.0f, 0.02f, 0.02f, 1.0f);
}

FLinearColor GetDefaultBodyColor()
{
	return FLinearColor::White;
}

float GetDefaultBrushRoughness()
{
	return 0.84f;
}

float GetDefaultBrushMetallic()
{
	return 0.0f;
}

TArray<FLinearColor> GetHighVisibilitySwatchColors()
{
	return {
		FLinearColor(1.0f, 0.02f, 0.02f, 1.0f),
		FLinearColor(1.0f, 0.42f, 0.0f, 1.0f),
		FLinearColor(1.0f, 0.92f, 0.0f, 1.0f),
		FLinearColor(0.05f, 1.0f, 0.08f, 1.0f),
		FLinearColor(0.0f, 0.85f, 1.0f, 1.0f),
		FLinearColor(0.02f, 0.18f, 1.0f, 1.0f),
		FLinearColor(0.58f, 0.0f, 1.0f, 1.0f),
		FLinearColor(1.0f, 0.0f, 0.65f, 1.0f),
		FLinearColor::White,
		FLinearColor::Black,
		FLinearColor(0.5f, 0.5f, 0.5f, 1.0f)
	};
}

void EnsureWidgetVariableGuid(UWidgetBlueprint* WidgetBlueprint, UWidget* Widget)
{
	if (WidgetBlueprint && Widget && !WidgetBlueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
	{
		WidgetBlueprint->OnVariableAdded(Widget->GetFName());
	}
}

void EnsureWidgetTreeVariableGuids(UWidgetBlueprint* WidgetBlueprint, UWidgetTree* WidgetTree)
{
	if (!WidgetBlueprint || !WidgetTree)
	{
		return;
	}

	TArray<UWidget*> Widgets;
	WidgetTree->GetAllWidgets(Widgets);
	for (UWidget* Widget : Widgets)
	{
		EnsureWidgetVariableGuid(WidgetBlueprint, Widget);
	}
}

USlider* EnsureColorPickerSliderRow(UWidgetBlueprint* WidgetBlueprint, UWidgetTree* WidgetTree, UVerticalBox* RootBox, const FText& LabelText, const FName& SliderName, float InitialValue)
{
	if (!WidgetTree || !RootBox)
	{
		return nullptr;
	}

	if (USlider* ExistingSlider = Cast<USlider>(WidgetTree->FindWidget(SliderName)))
	{
		ExistingSlider->SetValue(InitialValue);
		EnsureWidgetVariableGuid(WidgetBlueprint, ExistingSlider);
		return ExistingSlider;
	}

	const FName RowName = MakeUniqueObjectName(WidgetTree, UHorizontalBox::StaticClass(), FName(*FString::Printf(TEXT("%sRow"), *SliderName.ToString())));
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), RowName);
	EnsureWidgetVariableGuid(WidgetBlueprint, Row);

	USizeBox* LabelSize = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(),
		MakeUniqueObjectName(WidgetTree, USizeBox::StaticClass(), FName(*FString::Printf(TEXT("%sLabelSize"), *SliderName.ToString()))));
	EnsureWidgetVariableGuid(WidgetBlueprint, LabelSize);
	LabelSize->SetWidthOverride(34.0f);

	UTextBlock* Label = CreateWidgetText(
		WidgetTree,
		MakeUniqueObjectName(WidgetTree, UTextBlock::StaticClass(), FName(*FString::Printf(TEXT("%sLabel"), *SliderName.ToString()))),
		LabelText);
	EnsureWidgetVariableGuid(WidgetBlueprint, Label);
	LabelSize->SetContent(Label);
	Row->AddChildToHorizontalBox(LabelSize);

	USizeBox* SliderSize = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(),
		MakeUniqueObjectName(WidgetTree, USizeBox::StaticClass(), FName(*FString::Printf(TEXT("%sSize"), *SliderName.ToString()))));
	EnsureWidgetVariableGuid(WidgetBlueprint, SliderSize);
	SliderSize->SetWidthOverride(240.0f);

	USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), SliderName);
	EnsureWidgetVariableGuid(WidgetBlueprint, Slider);
	Slider->SetValue(InitialValue);
	SliderSize->SetContent(Slider);
	Row->AddChildToHorizontalBox(SliderSize);

	if (UVerticalBoxSlot* RowSlot = RootBox->AddChildToVerticalBox(Row))
	{
		RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	return Slider;
}

void ApplyColorPickerSwatchColors(UWidgetBlueprint* WidgetBlueprint, UWidgetTree* WidgetTree)
{
	if (!WidgetTree)
	{
		return;
	}

	const TArray<FLinearColor> SwatchColors = GetHighVisibilitySwatchColors();
	if (UBorder* ColorPreview = Cast<UBorder>(WidgetTree->FindWidget(FName(TEXT("ColorPreview")))))
	{
		ColorPreview->SetBrushColor(GetDefaultBrushColor());
	}

	USizeBox* ColorPickerSize = Cast<USizeBox>(WidgetTree->FindWidget(FName(TEXT("ColorPickerSize"))));
	if (!ColorPickerSize)
	{
		ColorPickerSize = Cast<USizeBox>(WidgetTree->RootWidget);
	}
	if (ColorPickerSize)
	{
		ColorPickerSize->SetWidthOverride(ColorPickerWidgetWidth);
		ColorPickerSize->SetHeightOverride(ColorPickerWidgetHeight);
	}

	UHorizontalBox* SwatchRow = Cast<UHorizontalBox>(WidgetTree->FindWidget(FName(TEXT("Swatches"))));
	for (int32 Index = 0; Index < SwatchColors.Num(); ++Index)
	{
		USizeBox* SwatchSize = Cast<USizeBox>(WidgetTree->FindWidget(FName(*FString::Printf(TEXT("SwatchSize%d"), Index))));
		if (!SwatchSize && SwatchRow)
		{
			SwatchSize = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				FName(*FString::Printf(TEXT("SwatchSize%d"), Index)));
			EnsureWidgetVariableGuid(WidgetBlueprint, SwatchSize);
			if (UHorizontalBoxSlot* SwatchSlot = SwatchRow->AddChildToHorizontalBox(SwatchSize))
			{
				SwatchSlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));
			}
		}
		if (SwatchSize)
		{
			SwatchSize->SetWidthOverride(26.0f);
			SwatchSize->SetHeightOverride(26.0f);
			if (UHorizontalBoxSlot* SwatchSlot = Cast<UHorizontalBoxSlot>(SwatchSize->Slot))
			{
				SwatchSlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));
			}
		}

		UButton* SwatchButton = Cast<UButton>(WidgetTree->FindWidget(FName(*FString::Printf(TEXT("Swatch%d"), Index))));
		if (!SwatchButton && SwatchSize)
		{
			SwatchButton = WidgetTree->ConstructWidget<UButton>(
				UButton::StaticClass(),
				FName(*FString::Printf(TEXT("Swatch%d"), Index)));
			EnsureWidgetVariableGuid(WidgetBlueprint, SwatchButton);
			SwatchSize->SetContent(SwatchButton);
		}
		if (SwatchButton)
		{
			SwatchButton->SetBackgroundColor(SwatchColors[Index]);
		}
	}

	if (UVerticalBox* RootBox = Cast<UVerticalBox>(WidgetTree->FindWidget(FName(TEXT("ColorPickerRoot")))))
	{
		UButton* CommitButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("CommitButton"))));
		if (CommitButton)
		{
			RootBox->RemoveChild(CommitButton);
		}

		EnsureColorPickerSliderRow(WidgetBlueprint, WidgetTree, RootBox, FText::FromString(TEXT("R")), TEXT("RedSlider"), GetDefaultBrushColor().R);
		EnsureColorPickerSliderRow(WidgetBlueprint, WidgetTree, RootBox, FText::FromString(TEXT("G")), TEXT("GreenSlider"), GetDefaultBrushColor().G);
		EnsureColorPickerSliderRow(WidgetBlueprint, WidgetTree, RootBox, FText::FromString(TEXT("B")), TEXT("BlueSlider"), GetDefaultBrushColor().B);
		EnsureColorPickerSliderRow(WidgetBlueprint, WidgetTree, RootBox, FText::FromString(TEXT("Rgh")), TEXT("RoughnessSlider"), GetDefaultBrushRoughness());
		EnsureColorPickerSliderRow(WidgetBlueprint, WidgetTree, RootBox, FText::FromString(TEXT("Met")), TEXT("MetallicSlider"), GetDefaultBrushMetallic());

		if (!CommitButton)
		{
			CommitButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CommitButton"));
			EnsureWidgetVariableGuid(WidgetBlueprint, CommitButton);
			UTextBlock* CommitText = CreateWidgetText(WidgetTree, TEXT("CommitText"), FText::FromString(TEXT("Apply")));
			EnsureWidgetVariableGuid(WidgetBlueprint, CommitText);
			CommitButton->SetContent(CommitText);
		}
		if (CommitButton)
		{
			RootBox->AddChildToVerticalBox(CommitButton);
		}
	}

	EnsureWidgetTreeVariableGuids(WidgetBlueprint, WidgetTree);
}

void RebuildColorPickerWidgetTree(UWidgetBlueprint* WidgetBlueprint)
{
	if (!WidgetBlueprint)
	{
		return;
	}

	if (!WidgetBlueprint->WidgetTree)
	{
		WidgetBlueprint->WidgetTree = NewObject<UWidgetTree>(WidgetBlueprint, TEXT("WidgetTree"), RF_Transactional);
	}

	UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
	if (!WidgetTree)
	{
		return;
	}
	if (WidgetTree->RootWidget)
	{
		WidgetBlueprint->Modify();
		WidgetTree->Modify();
		ApplyColorPickerSwatchColors(WidgetBlueprint, WidgetTree);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		return;
	}

	WidgetBlueprint->Modify();
	WidgetTree->Modify();

	USizeBox* ColorPickerSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ColorPickerSize"));
	EnsureWidgetVariableGuid(WidgetBlueprint, ColorPickerSize);
	ColorPickerSize->SetWidthOverride(ColorPickerWidgetWidth);
	ColorPickerSize->SetHeightOverride(ColorPickerWidgetHeight);
	WidgetTree->RootWidget = ColorPickerSize;

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BrushColorPanel"));
	EnsureWidgetVariableGuid(WidgetBlueprint, PanelBorder);
	PanelBorder->SetBrushColor(FLinearColor(0.02f, 0.025f, 0.028f, 0.92f));
	PanelBorder->SetPadding(FMargin(12.0f));
	ColorPickerSize->SetContent(PanelBorder);

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ColorPickerRoot"));
	EnsureWidgetVariableGuid(WidgetBlueprint, RootBox);
	PanelBorder->SetContent(RootBox);

	UTextBlock* TitleText = CreateWidgetText(WidgetTree, TEXT("TitleText"), FText::FromString(TEXT("Brush Color")));
	EnsureWidgetVariableGuid(WidgetBlueprint, TitleText);
	if (UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	USizeBox* PreviewSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PreviewSize"));
	EnsureWidgetVariableGuid(WidgetBlueprint, PreviewSize);
	PreviewSize->SetHeightOverride(30.0f);
	UBorder* ColorPreview = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ColorPreview"));
	EnsureWidgetVariableGuid(WidgetBlueprint, ColorPreview);
	ColorPreview->SetBrushColor(GetDefaultBrushColor());
	PreviewSize->SetContent(ColorPreview);
	if (UVerticalBoxSlot* PreviewSlot = RootBox->AddChildToVerticalBox(PreviewSize))
	{
		PreviewSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	UHorizontalBox* SwatchRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Swatches"));
	EnsureWidgetVariableGuid(WidgetBlueprint, SwatchRow);
	const TArray<FLinearColor> SwatchColors = GetHighVisibilitySwatchColors();
	for (int32 Index = 0; Index < SwatchColors.Num(); ++Index)
	{
		USizeBox* SwatchSize = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			FName(*FString::Printf(TEXT("SwatchSize%d"), Index)));
		EnsureWidgetVariableGuid(WidgetBlueprint, SwatchSize);
		SwatchSize->SetWidthOverride(26.0f);
		SwatchSize->SetHeightOverride(26.0f);

		UButton* SwatchButton = WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(),
			FName(*FString::Printf(TEXT("Swatch%d"), Index)));
		EnsureWidgetVariableGuid(WidgetBlueprint, SwatchButton);
		SwatchButton->SetBackgroundColor(SwatchColors[Index]);
		SwatchSize->SetContent(SwatchButton);

		if (UHorizontalBoxSlot* SwatchSlot = SwatchRow->AddChildToHorizontalBox(SwatchSize))
		{
			SwatchSlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));
		}
	}
	if (UVerticalBoxSlot* SwatchRowSlot = RootBox->AddChildToVerticalBox(SwatchRow))
	{
		SwatchRowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	auto AddSliderRow = [WidgetTree, RootBox](const FText& LabelText, const FName& SliderName, float InitialValue)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(),
			FName(*FString::Printf(TEXT("%sRow"), *SliderName.ToString())));

		USizeBox* LabelSize = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			FName(*FString::Printf(TEXT("%sLabelSize"), *SliderName.ToString())));
		LabelSize->SetWidthOverride(34.0f);
		LabelSize->SetContent(CreateWidgetText(WidgetTree, FName(*FString::Printf(TEXT("%sLabel"), *SliderName.ToString())), LabelText));
		Row->AddChildToHorizontalBox(LabelSize);

		USizeBox* SliderSize = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			FName(*FString::Printf(TEXT("%sSize"), *SliderName.ToString())));
		SliderSize->SetWidthOverride(240.0f);
		USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), SliderName);
		Slider->SetValue(InitialValue);
		SliderSize->SetContent(Slider);
		Row->AddChildToHorizontalBox(SliderSize);

		if (UVerticalBoxSlot* RowSlot = RootBox->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
		}
	};

	AddSliderRow(FText::FromString(TEXT("R")), TEXT("RedSlider"), GetDefaultBrushColor().R);
	AddSliderRow(FText::FromString(TEXT("G")), TEXT("GreenSlider"), GetDefaultBrushColor().G);
	AddSliderRow(FText::FromString(TEXT("B")), TEXT("BlueSlider"), GetDefaultBrushColor().B);
	AddSliderRow(FText::FromString(TEXT("Rgh")), TEXT("RoughnessSlider"), GetDefaultBrushRoughness());
	AddSliderRow(FText::FromString(TEXT("Met")), TEXT("MetallicSlider"), GetDefaultBrushMetallic());

	UButton* CommitButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CommitButton"));
	EnsureWidgetVariableGuid(WidgetBlueprint, CommitButton);
	UTextBlock* CommitText = CreateWidgetText(WidgetTree, TEXT("CommitText"), FText::FromString(TEXT("Apply")));
	EnsureWidgetVariableGuid(WidgetBlueprint, CommitText);
	CommitButton->SetContent(CommitText);
	RootBox->AddChildToVerticalBox(CommitButton);
	EnsureWidgetTreeVariableGuids(WidgetBlueprint, WidgetTree);

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
	WidgetBlueprint->MarkPackageDirty();
}

void RebuildBrushCursorWidgetTree(UWidgetBlueprint* WidgetBlueprint, UTexture2D* BrushCursorTexture, UMaterialInterface* BrushCursorMaterial)
{
	if (!WidgetBlueprint)
	{
		return;
	}

	if (!WidgetBlueprint->WidgetTree)
	{
		WidgetBlueprint->WidgetTree = NewObject<UWidgetTree>(WidgetBlueprint, TEXT("WidgetTree"), RF_Transactional);
	}

	UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
	if (!WidgetTree)
	{
		return;
	}

	UImage* ExistingImage = Cast<UImage>(WidgetTree->FindWidget(FName(TEXT("BrushCursorImage"))));
	if (ExistingImage && (BrushCursorMaterial || BrushCursorTexture))
	{
		if (BrushCursorMaterial)
		{
			ExistingImage->SetBrushFromMaterial(BrushCursorMaterial);
		}
		else
		{
			ExistingImage->SetBrushFromTexture(BrushCursorTexture, true);
		}
		WidgetBlueprint->MarkPackageDirty();
		return;
	}

	if (WidgetTree->RootWidget)
	{
		return;
	}

	WidgetBlueprint->Modify();
	WidgetTree->Modify();

	USizeBox* CursorSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BrushCursorSize"));
	CursorSize->SetWidthOverride(64.0f);
	CursorSize->SetHeightOverride(64.0f);
	WidgetTree->RootWidget = CursorSize;

	UImage* CursorImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BrushCursorImage"));
	if (BrushCursorMaterial)
	{
		CursorImage->SetBrushFromMaterial(BrushCursorMaterial);
	}
	else if (BrushCursorTexture)
	{
		CursorImage->SetBrushFromTexture(BrushCursorTexture, true);
	}
	CursorImage->SetDesiredSizeOverride(FVector2D(64.0f, 64.0f));
	CursorSize->SetContent(CursorImage);

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
	WidgetBlueprint->MarkPackageDirty();
}

UWidgetBlueprint* FindOrCreateColorPickerWidgetBlueprint()
{
	const FString PackageName = UIPath / TEXT("WBP_ChameleonColorPicker");
	if (UWidgetBlueprint* ExistingWidgetBlueprint = LoadAssetByPackageName<UWidgetBlueprint>(PackageName))
	{
		if (ExistingWidgetBlueprint->ParentClass != UChameleonColorPickerWidget::StaticClass())
		{
			ExistingWidgetBlueprint->ParentClass = UChameleonColorPickerWidget::StaticClass();
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(ExistingWidgetBlueprint);
		}
		RebuildColorPickerWidgetTree(ExistingWidgetBlueprint);
		FKismetEditorUtilities::CompileBlueprint(ExistingWidgetBlueprint);
		SavePackageForObject(ExistingWidgetBlueprint);
		return ExistingWidgetBlueprint;
	}

	UPackage* Package = CreatePackage(*PackageName);
	Package->FullyLoad();
	const FName AssetName(*FPackageName::GetLongPackageAssetName(PackageName));
	UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
		UChameleonColorPickerWidget::StaticClass(),
		Package,
		AssetName,
		BPTYPE_Normal,
		UWidgetBlueprint::StaticClass(),
		UWidgetBlueprintGeneratedClass::StaticClass()));
	if (WidgetBlueprint)
	{
		FAssetRegistryModule::AssetCreated(WidgetBlueprint);
		RebuildColorPickerWidgetTree(WidgetBlueprint);
		FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		SavePackageForObject(WidgetBlueprint);
	}

	return WidgetBlueprint;
}

UWidgetBlueprint* FindOrCreateBrushCursorWidgetBlueprint(UTexture2D* BrushCursorTexture, UMaterialInterface* BrushCursorMaterial)
{
	const FString PackageName = UIPath / TEXT("WBP_ChameleonBrushCursor");
	if (UWidgetBlueprint* ExistingWidgetBlueprint = LoadAssetByPackageName<UWidgetBlueprint>(PackageName))
	{
		if (ExistingWidgetBlueprint->ParentClass != UUserWidget::StaticClass())
		{
			ExistingWidgetBlueprint->ParentClass = UUserWidget::StaticClass();
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(ExistingWidgetBlueprint);
		}
		RebuildBrushCursorWidgetTree(ExistingWidgetBlueprint, BrushCursorTexture, BrushCursorMaterial);
		FKismetEditorUtilities::CompileBlueprint(ExistingWidgetBlueprint);
		SavePackageForObject(ExistingWidgetBlueprint);
		return ExistingWidgetBlueprint;
	}

	UPackage* Package = CreatePackage(*PackageName);
	Package->FullyLoad();
	const FName AssetName(*FPackageName::GetLongPackageAssetName(PackageName));
	UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
		UUserWidget::StaticClass(),
		Package,
		AssetName,
		BPTYPE_Normal,
		UWidgetBlueprint::StaticClass(),
		UWidgetBlueprintGeneratedClass::StaticClass()));
	if (WidgetBlueprint)
	{
		FAssetRegistryModule::AssetCreated(WidgetBlueprint);
		RebuildBrushCursorWidgetTree(WidgetBlueprint, BrushCursorTexture, BrushCursorMaterial);
		FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		SavePackageForObject(WidgetBlueprint);
	}

	return WidgetBlueprint;
}

void ConfigureBlueprints(UChameleonPainterInputConfig* InputConfig, UMaterialInterface* HiderMaterial, UMaterialInterface* PaintSprayMaterial, UTexture2D* BrushCursorTexture, UMaterialInterface* BrushCursorMaterial, UClass*& OutGameModeClass)
{
	UWidgetBlueprint* ColorPickerWidgetBlueprint = FindOrCreateColorPickerWidgetBlueprint();
	UWidgetBlueprint* BrushCursorWidgetBlueprint = FindOrCreateBrushCursorWidgetBlueprint(BrushCursorTexture, BrushCursorMaterial);

	UBlueprint* CharacterBlueprint = FindOrCreateBlueprint(BlueprintPath / TEXT("BP_ChameleonHiderCharacter"), AChameleonHiderCharacter::StaticClass());
	if (AChameleonHiderCharacter* CharacterCDO = CharacterBlueprint && CharacterBlueprint->GeneratedClass
		? Cast<AChameleonHiderCharacter>(CharacterBlueprint->GeneratedClass->GetDefaultObject())
		: nullptr)
	{
		if (CharacterCDO->BodyComponent)
		{
			CharacterCDO->BodyComponent->BodyMaterial = HiderMaterial;
			CharacterCDO->BodyComponent->CamouflageBaseColor = GetDefaultBodyColor();
			CharacterCDO->BodyComponent->CamouflageBaseRoughness = GetDefaultBrushRoughness();
			CharacterCDO->BodyComponent->CamouflageBaseMetallic = GetDefaultBrushMetallic();
		}
		if (CharacterCDO->PaintComponent)
		{
			CharacterCDO->PaintComponent->PaintColor = GetDefaultBrushColor();
			CharacterCDO->PaintComponent->bApplyOnRegister = false;
			CharacterCDO->PaintComponent->bApplyOnBeginPlay = false;
			CharacterCDO->PaintComponent->bApplyOnTargetComponentChange = false;
		}
		UClass* ColorPickerWidgetClass = ColorPickerWidgetBlueprint && ColorPickerWidgetBlueprint->GeneratedClass
			? static_cast<UClass*>(ColorPickerWidgetBlueprint->GeneratedClass.Get())
			: UChameleonColorPickerWidget::StaticClass();
		CharacterCDO->ColorPickerWidgetClass = TSubclassOf<UChameleonColorPickerWidget>(ColorPickerWidgetClass);
		CharacterCDO->BrushCursorWidgetClass = BrushCursorWidgetBlueprint && BrushCursorWidgetBlueprint->GeneratedClass
			? TSubclassOf<UUserWidget>(static_cast<UClass*>(BrushCursorWidgetBlueprint->GeneratedClass.Get()))
			: TSubclassOf<UUserWidget>();
		CharacterCDO->PaintSprayEffectClass = AChameleonPaintSprayActor::StaticClass();
		CharacterCDO->PaintSprayMaterial = PaintSprayMaterial;
		CharacterCDO->CurrentBrushColor = GetDefaultBrushColor();
		CharacterCDO->CurrentBrushRoughness = GetDefaultBrushRoughness();
		CharacterCDO->CurrentBrushMetallic = GetDefaultBrushMetallic();
		CharacterCDO->MarkPackageDirty();
		FBlueprintEditorUtils::MarkBlueprintAsModified(CharacterBlueprint);
	}
	FKismetEditorUtilities::CompileBlueprint(CharacterBlueprint);

	UBlueprint* GameModeBlueprint = FindOrCreateBlueprint(BlueprintPath / TEXT("BP_ChameleonPainterGameMode"), AChameleonPainterGameMode::StaticClass());
	if (AGameModeBase* GameModeCDO = GameModeBlueprint && GameModeBlueprint->GeneratedClass
		? Cast<AGameModeBase>(GameModeBlueprint->GeneratedClass->GetDefaultObject())
		: nullptr)
	{
		GameModeCDO->DefaultPawnClass = CharacterBlueprint->GeneratedClass;
		GameModeCDO->MarkPackageDirty();
		FBlueprintEditorUtils::MarkBlueprintAsModified(GameModeBlueprint);
	}
	FKismetEditorUtilities::CompileBlueprint(GameModeBlueprint);
	OutGameModeClass = GameModeBlueprint->GeneratedClass ? static_cast<UClass*>(GameModeBlueprint->GeneratedClass.Get()) : AChameleonPainterGameMode::StaticClass();

	UBlueprint* GameInstanceBlueprint = FindOrCreateBlueprint(BlueprintPath / TEXT("BP_ChameleonPainterGameInstance"), UChameleonPainterGameInstance::StaticClass());
	if (UChameleonPainterGameInstance* GameInstanceCDO = GameInstanceBlueprint && GameInstanceBlueprint->GeneratedClass
		? Cast<UChameleonPainterGameInstance>(GameInstanceBlueprint->GeneratedClass->GetDefaultObject())
		: nullptr)
	{
		GameInstanceCDO->ChameleonInputConfig = InputConfig;
		GameInstanceCDO->MarkPackageDirty();
		FBlueprintEditorUtils::MarkBlueprintAsModified(GameInstanceBlueprint);
	}
	FKismetEditorUtilities::CompileBlueprint(GameInstanceBlueprint);

	UGameMapsSettings* MapsSettings = UGameMapsSettings::GetGameMapsSettings();
	if (GameInstanceBlueprint && GameInstanceBlueprint->GeneratedClass)
	{
		MapsSettings->GameInstanceClass = FSoftClassPath(GameInstanceBlueprint->GeneratedClass->GetPathName());
	}
	if (OutGameModeClass)
	{
		UGameMapsSettings::SetGlobalDefaultGameMode(OutGameModeClass->GetPathName());
		GConfig->SetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("GlobalDefaultGameMode"),
			*OutGameModeClass->GetPathName(),
			GEngineIni);
	}
	if (GameInstanceBlueprint && GameInstanceBlueprint->GeneratedClass)
	{
		GConfig->SetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("GameInstanceClass"),
			*GameInstanceBlueprint->GeneratedClass->GetPathName(),
			GEngineIni);
	}
	GConfig->Flush(false, GEngineIni);
	MapsSettings->SaveConfig();

	SaveAssets({ ColorPickerWidgetBlueprint, BrushCursorWidgetBlueprint, CharacterBlueprint, GameModeBlueprint, GameInstanceBlueprint });
}

AStaticMeshActor* SpawnStaticMeshActor(UWorld* World, UStaticMesh* Mesh, UMaterialInterface* Material, const FVector& Location, const FRotator& Rotation, const FVector& Scale, const FString& Label)
{
	AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Location, Rotation);
	if (!Actor)
	{
		return nullptr;
	}

	Actor->SetActorLabel(Label);
	Actor->SetActorScale3D(Scale);

	if (UStaticMeshComponent* MeshComponent = Actor->GetStaticMeshComponent())
	{
		MeshComponent->SetStaticMesh(Mesh);
		MeshComponent->SetMaterial(0, Material);
		MeshComponent->SetMobility(EComponentMobility::Static);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}

	return Actor;
}

bool CreateTestLevel(
	UMaterialInterface* ConcreteMaterial,
	UMaterialInterface* WoodMaterial,
	UMaterialInterface* GreenMaterial,
	UMaterialInterface* FloorMaterial,
	UClass* GameModeClass)
{
	UWorld* World = UEditorLoadingAndSavingUtils::NewBlankMap(false);
	if (!World)
	{
		return false;
	}

	AWorldSettings* WorldSettings = World->GetWorldSettings();
	if (WorldSettings)
	{
		WorldSettings->DefaultGameMode = GameModeClass ? GameModeClass : AChameleonPainterGameMode::StaticClass();
	}

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));

	SpawnStaticMeshActor(World, Plane, FloorMaterial, FVector::ZeroVector, FRotator::ZeroRotator, FVector(18.0f, 18.0f, 1.0f), TEXT("CPT_FloorTile_SamplingPlane"));
	SpawnStaticMeshActor(World, Cube, ConcreteMaterial, FVector(850.0f, 0.0f, 160.0f), FRotator::ZeroRotator, FVector(0.35f, 17.0f, 3.2f), TEXT("CPT_Concrete_BackWall"));
	SpawnStaticMeshActor(World, Cube, ConcreteMaterial, FVector(-850.0f, 0.0f, 160.0f), FRotator::ZeroRotator, FVector(0.35f, 17.0f, 3.2f), TEXT("CPT_Concrete_FrontWall"));
	SpawnStaticMeshActor(World, Cube, GreenMaterial, FVector(100.0f, -620.0f, 120.0f), FRotator(0.0f, 14.0f, 0.0f), FVector(5.0f, 0.35f, 2.4f), TEXT("CPT_GreenPanel_Left"));
	SpawnStaticMeshActor(World, Cube, GreenMaterial, FVector(160.0f, 620.0f, 120.0f), FRotator(0.0f, -10.0f, 0.0f), FVector(5.5f, 0.35f, 2.4f), TEXT("CPT_GreenPanel_Right"));
	SpawnStaticMeshActor(World, Cube, WoodMaterial, FVector(-290.0f, -210.0f, 75.0f), FRotator(0.0f, 22.0f, 0.0f), FVector(1.6f, 1.6f, 1.5f), TEXT("CPT_WoodCrate_A"));
	SpawnStaticMeshActor(World, Cube, WoodMaterial, FVector(360.0f, 180.0f, 55.0f), FRotator(0.0f, -18.0f, 0.0f), FVector(1.2f, 1.2f, 1.1f), TEXT("CPT_WoodCrate_B"));
	SpawnStaticMeshActor(World, Cube, ConcreteMaterial, FVector(0.0f, 0.0f, 45.0f), FRotator(0.0f, 45.0f, 0.0f), FVector(2.4f, 0.35f, 0.9f), TEXT("CPT_LowConcreteDivider"));

	APlayerStart* PlayerStart = World->SpawnActor<APlayerStart>(FVector(-520.0f, 0.0f, 100.0f), FRotator(0.0f, 0.0f, 0.0f));
	if (PlayerStart)
	{
		PlayerStart->SetActorLabel(TEXT("CPT_PlayerStart"));
	}

	ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(FVector(0.0f, 0.0f, 700.0f), FRotator(-42.0f, -36.0f, 0.0f));
	if (Sun && Sun->GetLightComponent())
	{
		Sun->SetActorLabel(TEXT("CPT_DirectionalLight"));
		Sun->GetLightComponent()->SetIntensity(4.0f);
		Sun->GetComponent()->SetAtmosphereSunLight(true);
		Sun->GetComponent()->SetAtmosphereSunLightIndex(0);
	}

	ASkyLight* SkyLight = World->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (SkyLight && SkyLight->GetLightComponent())
	{
		SkyLight->SetActorLabel(TEXT("CPT_SkyLight"));
		SkyLight->GetLightComponent()->SetIntensity(0.75f);
	}

	ASkyAtmosphere* SkyAtmosphere = World->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (SkyAtmosphere && SkyAtmosphere->GetComponent())
	{
		SkyAtmosphere->SetActorLabel(TEXT("CPT_SkyAtmosphere"));
	}

	const FTransform PreviewBodyTransform(FRotator(0.0f, -40.0f, 0.0f), FVector(480.0f, -360.0f, 10.0f));
	AChameleonHiderBodyActor* PreviewBody = World->SpawnActorDeferred<AChameleonHiderBodyActor>(AChameleonHiderBodyActor::StaticClass(), PreviewBodyTransform);
	if (PreviewBody)
	{
		PreviewBody->SetActorLabel(TEXT("CPT_PreviewPaintableHiderBody"));
		if (PreviewBody->BodyComponent)
		{
			PreviewBody->BodyComponent->BodyMaterial = LoadAssetByPackageName<UMaterial>(MaterialPath / TEXT("M_CPT_HiderPaint"));
			PreviewBody->BodyComponent->CamouflageBaseColor = GetDefaultBodyColor();
			PreviewBody->BodyComponent->CamouflageBaseRoughness = GetDefaultBrushRoughness();
			PreviewBody->BodyComponent->CamouflageBaseMetallic = GetDefaultBrushMetallic();
			PreviewBody->BodyComponent->bBuildQueryCollision = true;
		}
		if (PreviewBody->PaintComponent)
		{
			PreviewBody->PaintComponent->PaintColor = GetDefaultBrushColor();
			PreviewBody->PaintComponent->bApplyOnRegister = false;
			PreviewBody->PaintComponent->bApplyOnBeginPlay = false;
			PreviewBody->PaintComponent->bApplyOnTargetComponentChange = false;
		}
		PreviewBody->FinishSpawning(PreviewBodyTransform);
	}

	const FString LevelPackage = MapPath / TEXT("L_ChameleonPainter_Test");
	if (!UEditorLoadingAndSavingUtils::SaveMap(World, LevelPackage))
	{
		return false;
	}

	UGameMapsSettings* MapsSettings = UGameMapsSettings::GetGameMapsSettings();
	const FString LevelObjectPath = ToObjectPath(LevelPackage);
	MapsSettings->EditorStartupMap = FSoftObjectPath(LevelObjectPath);
	UGameMapsSettings::SetGameDefaultMap(LevelPackage);
	GConfig->SetString(
		TEXT("/Script/EngineSettings.GameMapsSettings"),
		TEXT("EditorStartupMap"),
		*LevelObjectPath,
		GEngineIni);
	GConfig->SetString(
		TEXT("/Script/EngineSettings.GameMapsSettings"),
		TEXT("GameDefaultMap"),
		*LevelObjectPath,
		GEngineIni);
	GConfig->Flush(false, GEngineIni);
	MapsSettings->SaveConfig();

	return true;
}
}

UChameleonPainterBuildTestContentCommandlet::UChameleonPainterBuildTestContentCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UChameleonPainterBuildTestContentCommandlet::Main(const FString& Params)
{
	UE_LOG(LogChameleonPainterContent, Display, TEXT("Building Chameleon Painter test content. Params: %s"), *Params);

	const TMap<FName, UTexture2D*> Textures = ImportTextures();
	UMaterial* HiderMaterial = CreateHiderPaintMaterial();
	UMaterial* PaintSprayMaterial = CreatePaintSprayMaterial();
	UMaterial* ConcreteMaterial = CreateSurfaceMaterial(
		TEXT("M_CPT_Concrete"),
		Textures.FindRef(TEXT("T_CPT_Concrete")),
		FLinearColor(0.60f, 0.61f, 0.57f, 1.0f));
	UMaterial* WoodMaterial = CreateSurfaceMaterial(
		TEXT("M_CPT_Wood"),
		Textures.FindRef(TEXT("T_CPT_Wood")),
		FLinearColor(0.53f, 0.33f, 0.18f, 1.0f));
	UMaterial* GreenMaterial = CreateSurfaceMaterial(
		TEXT("M_CPT_GreenPanel"),
		Textures.FindRef(TEXT("T_CPT_GreenPanel")),
		FLinearColor(0.18f, 0.39f, 0.20f, 1.0f));
	UMaterial* FloorMaterial = CreateSurfaceMaterial(
		TEXT("M_CPT_FloorTile"),
		Textures.FindRef(TEXT("T_CPT_FloorTile")),
		FLinearColor(0.30f, 0.33f, 0.34f, 1.0f));

	UChameleonPainterInputConfig* InputConfig = CreateInputAssets();
	UTexture2D* BrushCursorTexture = ImportBrushCursorTexture();
	UMaterial* BrushCursorMaterial = CreateBrushCursorMaterial(BrushCursorTexture);
	UClass* GameModeClass = nullptr;
	ConfigureBlueprints(InputConfig, HiderMaterial, PaintSprayMaterial, BrushCursorTexture, BrushCursorMaterial, GameModeClass);

	const bool bCreatedLevel = CreateTestLevel(ConcreteMaterial, WoodMaterial, GreenMaterial, FloorMaterial, GameModeClass);
	UEditorLoadingAndSavingUtils::SaveDirtyPackages(false, true);

	if (!bCreatedLevel)
	{
		UE_LOG(LogChameleonPainterContent, Error, TEXT("Failed to create Chameleon Painter test level."));
		return 1;
	}

	UE_LOG(LogChameleonPainterContent, Display, TEXT("Chameleon Painter test content is ready at %s"), *RootPath);
	return 0;
}
