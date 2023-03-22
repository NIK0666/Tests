// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BaseDialogTest.generated.h"

class UHistoryData;
class UObjectNode;
class AArticyFlowPlayerActor;
class UBBDialogue;



UCLASS(BlueprintType)
class USubVariant : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FText Text;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FLinearColor Color;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 HistoryIndex; 

};

UCLASS(BlueprintType)
class UTestVariant : public UObject
{
	GENERATED_BODY()
public:

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectedSubVariant, USubVariant*, SubVariant);
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnSelectedSubVariant OnSelectedSubVariant;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString VariantText;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<USubVariant*> SubVariants;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<UHistoryData*> HistoryDataArray; 

};


/**
 * 
 */
UCLASS(BlueprintType, Abstract)
class ARTICYEDITOR_API UBaseDialogTest : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	virtual void StartTest(UBBDialogue* Dialogue, UArticyPlayerManager* FlowPlayerActor){}

	UFUNCTION(BlueprintPure)
	virtual FString GetTestName() {return "";}

	UFUNCTION(BlueprintPure)
	virtual FString GetTestDescription() {return "";}

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TArray<UTestVariant*> Variants;
};