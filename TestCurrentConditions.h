// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseDialogTest.h"
#include "TestCurrentConditions.generated.h"

class UHistoryData;
class UArticyPlayerManager;
/**
 * 
 */
UCLASS()
class ARTICYEDITOR_API UTestCurrentConditions : public UBaseDialogTest
{
	GENERATED_BODY()
public:
	// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFindBranchesCount, const int32&, Count);
	// UPROPERTY(BlueprintAssignable)
	// FOnFindBranchesCount OnFindBranchesCount;

	virtual void StartTest(UBBDialogue* Dialogue, UArticyPlayerManager* FlowPlayerActor) override;

	virtual FString GetTestName() override;

	virtual FString GetTestDescription() override;

	UFUNCTION()
	void AddedToHistoryHandler(UHistoryData* HistoryData);

protected:
	UPROPERTY()	
	TArray<UHistoryData*> CurrentHistoryDataArray;
	
	UPROPERTY()	
	TArray<USubVariant*> CurrentSubVariants;

	UPROPERTY()
	UBBDialogue* CurrentDialogue;

	UPROPERTY()
	UArticyPlayerManager* CurrentFlowPlayerActor;
};
