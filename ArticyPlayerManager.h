// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ArticyDraftData.h"
#include "ArticyGenerated/BBExpressoScripts.h"
#include "ArticyPlayerManager.generated.h"

class UHistoryData;

/**
 * 
 */
UCLASS()
class ARTICYRUNTIME_API UArticyPlayerManager : public UObject, public IBBMethodsProvider
{
	GENERATED_BODY()

public:
	UArticyPlayerManager();

	virtual ~UArticyPlayerManager() override;

	void Init();

	void Restore(FProgressHistory* InProgressHistory);
	
	static UWorld* GetWorldContext();
	
	//Delegates 
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShowDialogueUI);
	UPROPERTY(BlueprintAssignable)
	FOnShowDialogueUI OnShowDialogueUI;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHideDialogueUI);
	UPROPERTY(BlueprintAssignable)
	FOnHideDialogueUI OnHideDialogueUI;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShowDialogueFragment, FText, SpeakerName, FText, Phrase);
	UPROPERTY(BlueprintAssignable)
	FOnShowDialogueFragment OnShowDialogueFragment;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAddedToHistory, UHistoryData*, HistoryData);
	UPROPERTY(BlueprintAssignable)
	FOnAddedToHistory OnAddedToHistory;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHideBranches);
	UPROPERTY(BlueprintAssignable)
	FOnHideBranches OnHideBranches;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAutoSkip);
	UPROPERTY(BlueprintAssignable)
	FOnAutoSkip OnAutoSkip;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAddBranch, const FText&, Text, int32, Index);
	UPROPERTY(BlueprintAssignable)
	FOnAddBranch OnAddBranch;	
	
	DECLARE_DELEGATE_OneParam(FOnHubShowed, UBBHub* Hub);
	FOnHubShowed OnHubShowed;
	
	UFUNCTION()
	void PlayerPausedHandler(TScriptInterface<IArticyFlowObject> PausedOn);
	
	UFUNCTION()
	void BranchesUpdatedHandler(const TArray<FArticyBranch>& AvailableBranches);
	
	UFUNCTION(BlueprintCallable)
	void StartDialogue(FArticyRef StartNodeId);

	UDataTable* FindDialFragmentsDataTable(UArticyFlowFragment* FlowFragment);
	UArticyFlowFragment* GetParentFlowFragment(UArticyObject* ArticyObject);	
	void SetDialFragmentsDataTable(UDataTable* DataTable);
	
	UFUNCTION(BlueprintCallable)
	void StartDialogueById(FArticyId NewId);

	UFUNCTION(BlueprintCallable)
	void CloseDialogue();

	UFUNCTION(BlueprintCallable)
	bool ContinueDialogue();

	UFUNCTION(BlueprintCallable)
	void BranchSelectedHandler(int32 BranchIndex);

	UFUNCTION(BlueprintCallable)
	bool SelectBranchByName(const FName& BranchName);

	UFUNCTION(BlueprintCallable)
	void ResetProgress();

	void SetIsAddToHistory(bool bValue);

	UFUNCTION(BlueprintPure)
	FArticyDialFragment GetDialFragmentRow(UArticyDialogueFragment* DialogueFragment);

	UFUNCTION(BlueprintPure)
	FArticyQuest GetQuestRow(UBBEntityQuest* EntityQuest);

	UFUNCTION(BlueprintPure)
	FArticyItem GetItemRow(UBBEntityItem* EntityItem);

	UFUNCTION(BlueprintPure)
	FArticyCharacter GetCharacterRow(UArticyObject* EntityCharacter);

	UFUNCTION(BlueprintPure)
	FProgressState GetProgressState();

	UFUNCTION(BlueprintCallable)
	void SetProgressState(const FProgressState& InProgressState);

	UFUNCTION()
	void InstructionExecutedHandler(UArticyInstruction* ArticyInstruction);

	UFUNCTION()
	UHistoryData* StartDialogueToHistory(UArticyDialogue* Dialogue);

	UFUNCTION()
	void ConditionExecutedHandler(UArticyCondition* ArticyCondition, bool bIsSelected);

	UFUNCTION()
	void DialogueFragmentHandler(UArticyDialogueFragment* ArticyDialogueFragment);

	UFUNCTION(BlueprintPure)
	bool GetIsDialogueActive();

	UFUNCTION(BlueprintPure)
	bool GetIsShowAnswers();

	UFUNCTION(BlueprintPure)
	TMap<int32, int32> GetBranchIds();

	FArticyDialFragment* GetDialFragmentRowPtr(UArticyDialogueFragment* DialogueFragment);
	FArticyQuest* GetQuestRowPtr(UBBEntityQuest* EntityQuest);	
	FArticyItem* GetItemRowPtr(UBBEntityItem* EntityItem);	
	FArticyCharacter* GetCharacterRowPtr(UArticyObject* EntityCharacter);

	FNPCInfo* GetNPCInfo(const UBBNPC* NPC);
	
	UPROPERTY(BlueprintReadOnly, Transient)
	UArticyFlowPlayer* ArticyFlowPlayer;
	
	UPROPERTY(BlueprintReadWrite)
	FProgressState ProgressState;

	UPROPERTY(BlueprintReadWrite)
	TMap<UHistoryData*, FProgressHistory> ProgressHistory;
	
	UFUNCTION(BlueprintCallable)
	void CheckCondition(UArticyCondition* ArticyCondition, int& SelectedPin);

protected:
	void ExecQuestInstruction(UBBQuestInstruction* QuestInstruction);
	void ExecItemInstruction(UBBItemInstruction* ItemInstruction);

	UFUNCTION(BlueprintCallable)
	void BranchesDoneHandler();
	
	UFUNCTION(BlueprintCallable)
	void Reset();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UDataTable* CharactersDataTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UDataTable* QuestsDataTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UDataTable* ItemsDataTable;

	UPROPERTY()
	UDataTable* DialFragmentsDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsAddToHistory = false;


	const FName PlayerCharacterTechnicalName = FName("Ntt_Everett");

	TSet<FNPCInfo*> AlreadyTalkedSet;	

	FArticyId LastHubId;

private:
	UHistoryData* AddToHistory(UArticyObject* ArticyObject, EHistoryType Type, FDataTableRowHandle RowHandle = FDataTableRowHandle());

	void SetDataTables();
	
	bool bIsDialogueActive = false;

	bool bIsShowAnswers = false;
	
	TMap<int32, int32> BranchIds;
};
