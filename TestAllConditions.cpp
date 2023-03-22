// Fill out your copyright notice in the Description page of Project Settings.


#include "Tests/TestAllConditions.h"

#include "ArticyEditorSystem.h"
#include "HistoryData.h"
#include "ArticyGenerated/BBArticyTypes.h"

void UTestAllConditions::StartTest(UBBDialogue* Dialogue, UArticyPlayerManager* FlowPlayerActor)
{
	CurrentDialogue = Dialogue;
	CurrentBranchesTree = GetBranchesTree(Dialogue);
	CurrentFlowPlayerActor = FlowPlayerActor;
	FProgressState L_Progress = FlowPlayerActor->GetProgressState();
	CurrentFlowPlayerActor->OnAddedToHistory.AddDynamic(this, &UTestAllConditions::AddedToHistoryHandler);

	CurrentFlowPlayerActor->FindDialFragmentsDataTable(CurrentFlowPlayerActor->GetParentFlowFragment(Dialogue));
	
	UArticyEditorSystem* ArticyEditorSystem = GEditor->GetEditorSubsystem<UArticyEditorSystem>();
	StartDialogObject = ArticyEditorSystem->GetStartObject(CurrentDialogue);

	UObjectNode* L_CursorNode = CurrentBranchesTree;
	if (L_CursorNode == nullptr)
	{
		FlowPlayerActor->StartDialogueById(Dialogue->GetId());
		while (FlowPlayerActor->ContinueDialogue()){}

		if (!CurrentHistoryDataArray.IsEmpty())
		{
			UTestVariant* L_Variant = NewObject<UTestVariant>();
			L_Variant->HistoryDataArray = CurrentHistoryDataArray;
			L_Variant->VariantText = TEXT("Main variant");
			Variants.Add(L_Variant);
			CurrentHistoryDataArray.Empty();
		}
	}
	else
	{
		bool L_StartDialLoop = true;
		while (L_StartDialLoop)
		{
			L_CursorNode->Update();

			bool L_FindCursorLoop = true;
			while (L_FindCursorLoop)
			{				
				if (const auto NodePtr = L_CursorNode->GetCurrentChildNode())
				{
					NodePtr->Update();
					L_CursorNode = NodePtr;
				}
				else
				{
					L_FindCursorLoop = false;
				}
			}

			if (RunDialogueTest() && L_CursorNode != nullptr && !CurrentHistoryDataArray.IsEmpty())
			{
				UTestVariant* L_Variant = NewObject<UTestVariant>();
				L_Variant->HistoryDataArray = CurrentHistoryDataArray;				
				L_Variant->VariantText = FString::Printf(TEXT("Variant %d"), Variants.Num() + 1);
				L_Variant->SubVariants = CurrentSubVariants;
				Variants.Add(L_Variant);
			}

			CurrentHistoryDataArray.Empty();
			CurrentSubVariants.Empty();
			
			while (L_CursorNode != nullptr && L_CursorNode->IsCompleted())
			{
				L_CursorNode = L_CursorNode->ParentNode;
			}

			if (L_CursorNode == nullptr)
			{
				L_StartDialLoop = false;
			}
		}
	}
	
	FlowPlayerActor->SetProgressState(L_Progress);
	CurrentFlowPlayerActor->OnAddedToHistory.RemoveDynamic(this, &UTestAllConditions::AddedToHistoryHandler);
}

FString UTestAllConditions::GetTestDescription()
{
	return CurrentDialogue->GetDisplayName().ToString();
}

FString UTestAllConditions::GetTestName()
{
	return TEXT("Test ALL Conditions:");
}

void UTestAllConditions::AddedToHistoryHandler(UHistoryData* HistoryData)
{
	if (NeedMarkDialogFragment && HistoryData->Type == EHistoryType::DialFragment)
	{
		HistoryData->Type = EHistoryType::SelectedDialFragment;
		if (const FArticyDialFragment* L_RowPtr = CurrentFlowPlayerActor->GetDialFragmentRowPtr(Cast<UBBDialogueFragment>(HistoryData->ArticyObject)))
		{
			USubVariant* L_SubVariant = NewObject<USubVariant>();
			L_SubVariant->Text = L_RowPtr->MenuText.IsEmpty() ? L_RowPtr->Text : L_RowPtr->MenuText;
			L_SubVariant->Color = FLinearColor(0.25f, 0.75f, 1.f);
			L_SubVariant->HistoryIndex = CurrentHistoryDataArray.Num();
			CurrentSubVariants.Add(L_SubVariant);
		}
		NeedMarkDialogFragment = false;
	}
	CurrentHistoryDataArray.Add(HistoryData);
}

bool UTestAllConditions::RunDialogueTest()
{
	CurrentFlowPlayerActor->StartDialogueToHistory(CurrentDialogue);
	
	if (StartDialogObject != CurrentBranchesTree->GetArticyObject())
	{
		AddToHistoryInstructionOrDialFragment(StartDialogObject);
		AddToHistory(FindChildDialFragmentsAndInstructions(StartDialogObject, 0));		
	}

	UObjectNode* L_CursorNode = CurrentBranchesTree;
	UArticyEditorSystem* ArticyEditorSystem = GEditor->GetEditorSubsystem<UArticyEditorSystem>();

	bool L_FoundHub = false;
	bool L_EmptyHistoryAfterHub = false;
	
	while (L_CursorNode != nullptr)
	{
		if (L_CursorNode->Condition != nullptr)
		{
			USubVariant* L_SubVariant = NewObject<USubVariant>();
			L_SubVariant->Text = FText::FromString(ArticyEditorSystem->GetConditionText(L_CursorNode->Condition));
			L_SubVariant->Color = L_CursorNode->CurrentIndex == 1 ? FLinearColor(0.5f, 1.f, 0.5f) : FLinearColor(1.f, 0.5f, 0.5f);
			L_SubVariant->HistoryIndex = CurrentHistoryDataArray.Num() - 1;
			CurrentSubVariants.Add(L_SubVariant);
		}
		else if (L_CursorNode->Hub != nullptr)
		{
			L_FoundHub = true;
			L_EmptyHistoryAfterHub = true;
		}
		const TArray<UArticyObject*> L_History = FindChildDialFragmentsAndInstructions(L_CursorNode->GetArticyObject(), L_CursorNode->CurrentIndex - 1);
		if (L_FoundHub && L_EmptyHistoryAfterHub)
		{
			L_EmptyHistoryAfterHub = L_History.IsEmpty();
			NeedMarkDialogFragment = true;
		}
		
		AddToHistory(L_History);
		NeedMarkDialogFragment = false;
		L_CursorNode = L_CursorNode->GetCurrentChildNode();
	}

	CurrentFlowPlayerActor->CloseDialogue();
	
	if (L_FoundHub && L_EmptyHistoryAfterHub)
	{
		return false;
	}
	return true;
}

void UTestAllConditions::AddToHistoryInstructionOrDialFragment(UArticyObject* ArticyObject)
{
	if (UBBInstruction* Instruction = Cast<UBBInstruction>(ArticyObject))
	{
		CurrentFlowPlayerActor->InstructionExecutedHandler(Instruction);
	}
	else if (UBBDialogueFragment* DialFragment = Cast<UBBDialogueFragment>(ArticyObject))
	{
		CurrentFlowPlayerActor->DialogueFragmentHandler(DialFragment);
	}
}

void UTestAllConditions::AddToHistory(TArray<UArticyObject*> ArticyObjects)
{
	for (auto ArticyObject : ArticyObjects)
	{
		AddToHistoryInstructionOrDialFragment(ArticyObject);
	}
}



UObjectNode* UTestAllConditions::GetBranchesTree(UBBDialogue* Dialogue)
{
	UArticyEditorSystem* ArticyEditorSystem = GEditor->GetEditorSubsystem<UArticyEditorSystem>();
	//Получаем первый ArticyObject в диалоге
	UArticyObject* CursorObject = ArticyEditorSystem->GetStartObject(Dialogue);
	//Ищем condition или hub, создаем ноду
	UObjectNode* Result = FindConditionOrHubAndCreateNode(CursorObject);
	//Ищем ему Child Nodes
	if (Result != nullptr)
	{
		CreateChildNodes(Result);
		RecursiveCreateChildNodes(Result);
	}
	
	return Result;
}

UObjectNode* UTestAllConditions::FindConditionOrHubAndCreateNode(UArticyObject* CursorObject)
{
	UObjectNode* Result = nullptr;
	while (Result == nullptr)
	{
		if (UBBHub* L_FoundHub = Cast<UBBHub>(CursorObject))
		{
			Result = NewObject<UObjectNode>();
			Result->Hub = L_FoundHub;

			for (auto L_OutputPin : Result->Hub->OutputPins)
			{		
				Result->MaxIndex = L_OutputPin->Connections.Num();
				break;
			}
		}
		else if (UBBCondition* L_FoundCondition = Cast<UBBCondition>(CursorObject))
		{
			Result = NewObject<UObjectNode>();
			Result->Condition = L_FoundCondition;
			Result->MaxIndex = 2;
		}
		else if (UBBJump* L_FoundJump = Cast<UBBJump>(CursorObject))
		{
			CursorObject = L_FoundJump->GetTargetID().GetObject<UArticyObject>(CursorObject->GetWorld());
		}
		else
		{
			UArticyObject* L_FoundChildObject = nullptr;
			for (auto L_Subobject : CursorObject->GetSubobjects())
			{
				if (auto L_ArticyOutputPin = Cast<UArticyOutputPin>(L_Subobject.Value))
				{
					for (auto L_Connection : L_ArticyOutputPin->Connections)
					{
						L_FoundChildObject = L_Connection->GetTargetID().GetObject<UArticyObject>(CursorObject->GetWorld());
						if (L_FoundChildObject != nullptr)
						{
							break;
						}
					}
				}
			}
			if (L_FoundChildObject == nullptr)
			{
				break;
			}
			CursorObject = L_FoundChildObject;
		}
	}
	return Result;
}

bool UTestAllConditions::CreateChildNodes(UObjectNode* ParentNode)
{	
	TArray<UArticyOutputPin*> L_OutputPins = ParentNode->Condition != nullptr ? ParentNode->Condition->OutputPins : ParentNode->Hub->OutputPins;
	int32 Index = -1;
	for (int32 i = 0; i < L_OutputPins.Num(); i++)
	{
		if (auto L_ArticyOutputPin = L_OutputPins[i])
		{
			for (auto L_Connection : L_ArticyOutputPin->Connections)
			{
				UArticyObject* L_FoundChildObject = L_Connection->GetTargetID().GetObject<UArticyObject>(ParentNode->GetArticyObject()->GetWorld());
				if (L_FoundChildObject != nullptr)
				{
					Index++;
					UObjectNode* ChildNode = FindConditionOrHubAndCreateNode(L_FoundChildObject);
					if (ChildNode)
					{
						ChildNode->ParentNode = ParentNode;
						ParentNode->ChildNodes.Add(Index, ChildNode);
					}
				}
			}
		}
	}
	
	return !ParentNode->ChildNodes.IsEmpty();
}

void UTestAllConditions::RecursiveCreateChildNodes(UObjectNode* Result)
{	
	for (auto L_ChildNode : Result->ChildNodes)
	{
		bool FoundNode = false;
		UObjectNode* L_Parent = Result;
		while (L_Parent)
		{
			if (L_Parent->GetArticyObject()->GetTechnicalName() == L_ChildNode.Value->GetArticyObject()->GetTechnicalName())
			{
				FoundNode = true;
				break;
			}			
			L_Parent = L_Parent->ParentNode;
		}
		if (FoundNode)
		{
			continue;
		}
		
		if (CreateChildNodes(L_ChildNode.Value))
		{			
			RecursiveCreateChildNodes(L_ChildNode.Value);
		}
	}
}

TArray<UArticyObject*> UTestAllConditions::FindChildDialFragmentsAndInstructions(UArticyObject* CursorObject, int32 BranchIndex)
{
	TArray<UArticyObject*> Result;
	
	bool L_Loop = true;
	int32 L_NextIndex = BranchIndex;
	while (L_Loop)
	{
		if (UBBHub* L_FoundHub = Cast<UBBHub>(CursorObject))
		{
			for (auto L_Subobject : L_FoundHub->GetSubobjects())
			{		
				if (const UArticyOutputPin* OutputPin = Cast<UArticyOutputPin>(L_Subobject.Value))
				{
					UArticyOutgoingConnection* L_Connection = OutputPin->Connections[L_NextIndex];
					CursorObject = L_Connection->GetTargetID().GetObject<UArticyObject>(CursorObject->GetWorld());
					L_NextIndex = 0;
					break;
				}
			}
		}
		else if (UBBCondition* L_FoundCondition = Cast<UBBCondition>(CursorObject))
		{
			if (const UArticyOutputPin* OutputPin = L_FoundCondition->OutputPins[L_NextIndex])
			{
				if (OutputPin->Connections.Num() > 0)
				{
					UArticyOutgoingConnection* L_Connection = OutputPin->Connections[0];
					CursorObject = L_Connection->GetTargetID().GetObject<UArticyObject>(CursorObject->GetWorld());
				}
				else
				{
					CursorObject = nullptr;
				}
				L_NextIndex = 0;
			}
		}
		else if (UBBJump* L_FoundJump = Cast<UBBJump>(CursorObject))
		{
			CursorObject = L_FoundJump->GetTargetID().GetObject<UArticyObject>(CursorObject->GetWorld());
		}
		else
		{
			for (auto L_Subobject : CursorObject->GetSubobjects())
			{
				if (const UArticyOutputPin* OutputPin = Cast<UArticyOutputPin>(L_Subobject.Value))
				{
					if (OutputPin->Connections.Num() > L_NextIndex)
					{
						UArticyOutgoingConnection* L_Connection = OutputPin->Connections[L_NextIndex];
						CursorObject = L_Connection->GetTargetID().GetObject<UArticyObject>(CursorObject->GetWorld());
					}
					else
					{
						CursorObject = nullptr;
					}
					break;
				}
			}
		}
		
		//Получаем потомка
		if (CursorObject == nullptr || Cast<UBBHub>(CursorObject) || Cast<UBBCondition>(CursorObject))
		{
			L_Loop = false;
		}
		else if (Cast<UBBDialogueFragment>(CursorObject) || Cast<UBBInstruction>(CursorObject))
		{
			Result.Add(CursorObject);
		}
	}

	return Result;
}


// *********************************************


bool UObjectNode::IsCompleted() const
{
	return CurrentIndex >= MaxIndex;
}

bool UObjectNode::IsNotUpdated() const
{
	return CurrentIndex == 0;
}

void UObjectNode::SetCondition(UBBCondition* InCondition, bool bCond) const
{
	if (ParentNode != nullptr)
	{
		ParentNode->SetCondition(InCondition, bCond);
	}
	else
	{
		OnSetCondition.ExecuteIfBound(InCondition, bCond);
	}
}

UObjectNode* UObjectNode::GetCurrentChildNode()
{
	if (auto Child = ChildNodes.Find(CurrentIndex - 1))
	{
		return *Child;
	}
	return nullptr;
}

void UObjectNode::Update()
{
	if (bLogUpdate)
	{		
		FString Tabs = TEXT("");
		const UObjectNode* L_TestNode = this;
		while (L_TestNode->ParentNode)
		{
			Tabs = Tabs + TEXT("    ");
			L_TestNode = L_TestNode->ParentNode;
		}
		UE_LOG(LogTemp, Log, TEXT("%s%s"), *Tabs, *GetArticyObject()->GetTechnicalName().ToString());
	}
	
	CurrentIndex++;
	
	if (Condition != nullptr)
	{
		SetCondition(Condition, CurrentIndex == 1 ? true : false);
	}
}

UArticyObject* UObjectNode::GetArticyObject()
{
	if (Condition != nullptr)
	{
		return Condition;
	}
	return Hub;
}
