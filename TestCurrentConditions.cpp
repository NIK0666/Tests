// Fill out your copyright notice in the Description page of Project Settings.


#include "Tests/TestCurrentConditions.h"

#include "ArticyEditorSystem.h"
#include "HistoryData.h"
#include "ArticyPlayerManager.h"

void UTestCurrentConditions::StartTest(UBBDialogue* Dialogue, UArticyPlayerManager* FlowPlayerActor)
{
	CurrentDialogue = Dialogue;
	CurrentFlowPlayerActor = FlowPlayerActor;
	
	FProgressState L_Progress = FlowPlayerActor->GetProgressState();
	
	FlowPlayerActor->OnAddedToHistory.AddDynamic(this, &UTestCurrentConditions::AddedToHistoryHandler);

	TArray<int32> L_DebugMax;
	TArray<int32> L_DebugCurrent;
	
	bool L_IsStartNewDialLoop = true;
	while (L_IsStartNewDialLoop)
	{
		FlowPlayerActor->SetProgressState(L_Progress);
		FlowPlayerActor->StartDialogueById(Dialogue->GetId());

		bool L_IsFoundBranches = true;
		int32 L_Pos = -1;
		while (L_IsFoundBranches)
		{
			//Debug find branches
			while (FlowPlayerActor->GetIsDialogueActive() && !FlowPlayerActor->GetIsShowAnswers())
			{
				FlowPlayerActor->ContinueDialogue();
			}
			const int32 L_BranchesCount = FlowPlayerActor->GetBranchIds().Num();

			
			L_IsFoundBranches = FlowPlayerActor->GetIsDialogueActive() && FlowPlayerActor->GetIsShowAnswers();
			if (L_IsFoundBranches)
			{
				L_Pos++;				

				if (L_DebugMax.Num() > L_Pos)
				{
					L_DebugMax[L_Pos] = L_BranchesCount;
				}
				else
				{
					L_DebugMax.Add(L_BranchesCount);
				}
				

				int32 L_TempNextBranch = 0;
				if (L_DebugCurrent.Num() > L_Pos)
				{
					L_TempNextBranch = L_DebugCurrent[L_Pos];
				}

				if (L_Pos >= L_DebugMax.Num() - 1)
				{
					L_TempNextBranch++;
				}

				if (L_DebugCurrent.Num() > L_Pos)
				{
					L_DebugCurrent[L_Pos] = L_TempNextBranch;
				}
				else
				{
					L_DebugCurrent.Add(L_TempNextBranch);
				}

				FlowPlayerActor->BranchSelectedHandler(FlowPlayerActor->GetBranchIds()[L_TempNextBranch - 1]);			
			}
			else
			{
				bool L_IsLoop = !L_DebugMax.IsEmpty();
				int32 L_LoopIndex = L_DebugMax.Num() - 1;
				while (L_IsLoop)
				{
					if (L_DebugMax[L_LoopIndex] == L_DebugCurrent[L_LoopIndex])
					{
						L_DebugMax.RemoveAt(L_LoopIndex);
						L_DebugCurrent.RemoveAt(L_LoopIndex);
						L_LoopIndex--;
						if (L_LoopIndex < 0)
						{
							L_IsLoop = false;
						}
					}
					else
					{
						L_IsLoop = false;
					}
				}

				if (!CurrentHistoryDataArray.IsEmpty())
				{
					UTestVariant* L_Variant = NewObject<UTestVariant>();
					L_Variant->HistoryDataArray = CurrentHistoryDataArray;
					L_Variant->VariantText = FString::Printf(TEXT("Variant %d"), Variants.Num() + 1);
					L_Variant->SubVariants = CurrentSubVariants;
					Variants.Add(L_Variant);
					CurrentHistoryDataArray.Empty();
					CurrentSubVariants.Empty();
				}

				if (L_DebugCurrent.IsEmpty())
				{
					L_IsStartNewDialLoop = false;
				}
			}
		}
	}

	FlowPlayerActor->OnAddedToHistory.RemoveDynamic(this, &UTestCurrentConditions::AddedToHistoryHandler);
	FlowPlayerActor->SetProgressState(L_Progress);
}

FString UTestCurrentConditions::GetTestName()
{
	return TEXT("Test conditions:");
}

FString UTestCurrentConditions::GetTestDescription()
{
	UArticyEditorSystem* ArticyEditorSystem = GEditor->GetEditorSubsystem<UArticyEditorSystem>();
	auto L_ConditionsStates = ArticyEditorSystem->GetConditionsStates(CurrentDialogue);
	FString L_Description;
	for (auto L_Tuple : L_ConditionsStates)
	{
		if (!L_Tuple.Value)
		{
			continue;
		}

		if (!L_Description.IsEmpty())
		{
			L_Description.Appendf(TEXT("\n"));
		}

		L_Description.Appendf(TEXT("%s"), *ArticyEditorSystem->GetConditionText(L_Tuple.Key));
	}
	
	if (L_Description.IsEmpty())
	{
		return TEXT("No conditions");
	}
	return L_Description;
}

void UTestCurrentConditions::AddedToHistoryHandler(UHistoryData* HistoryData)
{
	if (HistoryData->Type == EHistoryType::SelectedDialFragment)
	{
		if (FArticyDialFragment* L_RowPtr = CurrentFlowPlayerActor->GetDialFragmentRowPtr(Cast<UBBDialogueFragment>(HistoryData->ArticyObject)))
		{
			USubVariant* L_SubVariant = NewObject<USubVariant>();
			L_SubVariant->Text = L_RowPtr->MenuText.IsEmpty() ? L_RowPtr->Text : L_RowPtr->MenuText;
			L_SubVariant->Color = FLinearColor(0.25f, 0.75f, 1.f);
			L_SubVariant->HistoryIndex = CurrentHistoryDataArray.Num();
			CurrentSubVariants.Add(L_SubVariant);
		}		
	}
	CurrentHistoryDataArray.Add(HistoryData);
}
