// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ArticyPlayerManager.h"
#include "BaseDialogTest.h"
#include "TestAllConditions.generated.h"

class UBBHub;
class UObjectNode;
class AArticyFlowPlayerActor;
class UBBDialogue;
class UHistoryData;

/**
 *
 */

UCLASS()
class ARTICYEDITOR_API UTestAllConditions : public UBaseDialogTest
{
	GENERATED_BODY()
public:

	virtual void StartTest(UBBDialogue* Dialogue, UArticyPlayerManager* FlowPlayerActor) override;

	virtual FString GetTestDescription() override;

	virtual FString GetTestName() override;
	
protected:
	UFUNCTION()
	void AddedToHistoryHandler(UHistoryData* HistoryData);

	UFUNCTION()
	void RecursiveCreateChildNodes(UObjectNode* Result);
	
private:

	UPROPERTY()
	UBBDialogue* CurrentDialogue;
	
	UPROPERTY()
	UObjectNode* CurrentBranchesTree;

	UPROPERTY()
	UArticyPlayerManager* CurrentFlowPlayerActor;
	
	UPROPERTY()
	UArticyObject* StartDialogObject;

	UPROPERTY()	
	TArray<UHistoryData*> CurrentHistoryDataArray;
	
	UPROPERTY()	
	TArray<USubVariant*> CurrentSubVariants;
	
	bool NeedMarkDialogFragment = false;

	bool RunDialogueTest();

	void AddToHistoryInstructionOrDialFragment(UArticyObject* ArticyObject);
	
	void AddToHistory(TArray<UArticyObject*> ArticyObjects);
	
	UObjectNode* GetBranchesTree(UBBDialogue* Dialogue);
	
	bool CreateChildNodes(UObjectNode* ParentNode);

	UObjectNode* FindConditionOrHubAndCreateNode(UArticyObject* CursorObject);

	TArray<UArticyObject*> FindChildDialFragmentsAndInstructions(UArticyObject* CursorObject, int32 BranchIndex);
};



//*****************************************************



UCLASS(BlueprintType)
class UObjectNode : public UObject
{
	GENERATED_BODY()
public:
	DECLARE_DELEGATE_TwoParams(FOnSetCondition, UBBCondition* Condition, bool IsTrue);
	FOnSetCondition OnSetCondition;
	
	UFUNCTION(BlueprintPure)
	bool IsCompleted() const;
	
	UFUNCTION(BlueprintPure)
	bool IsNotUpdated() const;

	void SetCondition(UBBCondition* InCondition, bool bCond) const;

	UObjectNode* GetCurrentChildNode();
	
	UFUNCTION(BlueprintCallable)
	void Update();

	UFUNCTION(BlueprintPure)
	UArticyObject* GetArticyObject();

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	UBBCondition* Condition = nullptr;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	UBBHub* Hub = nullptr;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	UObjectNode* ParentNode = nullptr;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TMap<int32, UObjectNode*> ChildNodes;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TArray<UArticyObject*> DialFragmentsAndInstructions;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	int32 MaxIndex = 0;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	int32 CurrentIndex = 0;

private:	
	bool bLogUpdate = false;
};