// Fill out your copyright notice in the Description page of Project Settings.


#include "ArticyPlayerManager.h"

#include "HistoryData.h"

UArticyPlayerManager::UArticyPlayerManager()
{
	
	ArticyFlowPlayer = NewObject<UArticyFlowPlayer>();
	ArticyFlowPlayer->SetOwner(this);
	
	bIsShowAnswers = false;
}

UArticyPlayerManager::~UArticyPlayerManager()
{
	if (IsValid(ArticyFlowPlayer))
	{		
		ArticyFlowPlayer->OnPlayerPaused.Unbind();
		ArticyFlowPlayer->OnBranchesUpdated.Unbind();
		ArticyFlowPlayer->OnInstructionExecuted.Unbind();
		ArticyFlowPlayer->OnCustomConditionSelected.Unbind();
		ArticyFlowPlayer->OnConditionExecuted.Unbind();
		ArticyFlowPlayer->OnBranchesDone.Unbind();
	}
}

void UArticyPlayerManager::Init()
{
	if (auto WorldContext = GetWorldContext())
	{
		if (auto ArticyDatabase = UArticyDatabase::Get(WorldContext))
		{
			ArticyDatabase->SetDefaultUserMethodsProvider(this);
		}
	}

	
	if (IsValid(ArticyFlowPlayer))
	{
		ArticyFlowPlayer->OnPlayerPaused.BindUObject(this, &UArticyPlayerManager::PlayerPausedHandler);
		ArticyFlowPlayer->OnBranchesUpdated.BindUObject(this, &UArticyPlayerManager::BranchesUpdatedHandler);
		ArticyFlowPlayer->OnInstructionExecuted.BindUObject(this, &UArticyPlayerManager::InstructionExecutedHandler);
		ArticyFlowPlayer->OnCustomConditionSelected.BindUObject(this, &UArticyPlayerManager::CheckCondition);
		ArticyFlowPlayer->OnConditionExecuted.BindUObject(this, &UArticyPlayerManager::ConditionExecutedHandler);
		ArticyFlowPlayer->OnBranchesDone.BindUObject(this, &UArticyPlayerManager::BranchesDoneHandler);
	}

	SetDataTables();
}

void UArticyPlayerManager::Restore(FProgressHistory* InProgressHistory)
{
	FindDialFragmentsDataTable(GetParentFlowFragment(InProgressHistory->ObjectId.GetObject<UArticyObject>(this)));
	if (InProgressHistory->bIsStarted)
	{
		Reset();
	}
	else
	{
		TScriptInterface<IArticyFlowObject> ptr;
		ptr.SetObject(InProgressHistory->ObjectId.GetObject(this));
		ptr.SetInterface(Cast<IArticyFlowObject>(InProgressHistory->ObjectId.GetObject(this)));
		ArticyFlowPlayer->SetCursorTo(ptr);

		bIsDialogueActive = true;
	}	
}

UWorld* UArticyPlayerManager::GetWorldContext()
{
	UWorld* PIE = nullptr;
	UWorld* GamePreview = nullptr;
	UWorld* Editor = nullptr;
	if (GEngine == nullptr)
	{
		return nullptr;
	}
	for (FWorldContext const& Context : GEngine->GetWorldContexts())
	{
		switch (Context.WorldType)
		{
		case EWorldType::PIE:
			PIE = Context.World();
			break;
		case EWorldType::GamePreview:
			GamePreview = Context.World();
			break;
		case EWorldType::Editor:
		case EWorldType::EditorPreview:
			Editor = Context.World();
			break;
		}
	}

	if (PIE)
	{
		return PIE;
	}

	if (GamePreview)
	{
		return GamePreview;
	}

	if (Editor)
	{
		return Editor;
	}

	return nullptr;
}

void UArticyPlayerManager::PlayerPausedHandler(TScriptInterface<IArticyFlowObject> PausedOn)
{
	UArticyNode* ArticyNode = Cast<UArticyNode>(PausedOn.GetObjectRef());
	
	switch (ArticyNode->GetType())
	{
	case EArticyPausableType::Dialogue:
		ArticyFlowPlayer->Play(0);
		break;

	case EArticyPausableType::DialogueFragment:
		{
			if (UArticyDialogueFragment* DialogueFragment = Cast<UArticyDialogueFragment>(ArticyNode))
			{
				DialogueFragmentHandler(DialogueFragment);
			}
			break;
		}
	default:
		break;
	}
}

void UArticyPlayerManager::BranchesUpdatedHandler(const TArray<FArticyBranch>& AvailableBranches)
{
	if (AvailableBranches.Num() == 1)
	{
		OnAutoSkip.Broadcast();
		return;
	}
	
	bIsShowAnswers = true;
	OnHideBranches.Broadcast();
	
	BranchIds.Empty();
	
	// for (auto L_Branch : AvailableBranches)
	for (int i = 0; i < AvailableBranches.Num(); i++)
	{
		const FArticyBranch& L_Branch = AvailableBranches[i];
		if (UArticyDialogueFragment* ArticyDialogueFragment = Cast<UArticyDialogueFragment>(L_Branch.GetTarget().GetObjectRef()))
		{
			if (auto L_Row = GetDialFragmentRowPtr(ArticyDialogueFragment))
			{
				OnAddBranch.Broadcast(L_Row->MenuText.IsEmpty() ? L_Row->Text : L_Row->MenuText, L_Branch.Index);
				BranchIds.Add(i, L_Branch.Index);
			}			
		}
	}
	
	if (UBBHub* L_Hub = Cast<UBBHub>(ArticyFlowPlayer->CurrentHubNode))
	{
		LastHubId = L_Hub->GetId();
		OnHubShowed.ExecuteIfBound(L_Hub);
	}
}

void UArticyPlayerManager::StartDialogue(FArticyRef StartNodeId)
{
	StartDialogueById(StartNodeId.GetId());
}

UDataTable* UArticyPlayerManager::FindDialFragmentsDataTable(UArticyFlowFragment* FlowFragment)
{
	if (FlowFragment == nullptr)
	{		
		UE_LOG(LogTemp, Error, TEXT("FlowFragment is NULL!"));

		return nullptr;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	const FString TableName = "DT_" + FlowFragment->GetTechnicalName().ToString();
	const FAssetData& AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(*(GetDefault<UArticyPluginSettings>()->DataTablesDir.Path + "/" + 
		TableName + "." + TableName));

	SetDialFragmentsDataTable(Cast<UDataTable>(AssetData.GetAsset()));
	return DialFragmentsDataTable;
}

UArticyFlowFragment* UArticyPlayerManager::GetParentFlowFragment(UArticyObject* ArticyObject)
{
	while (ArticyObject->GetParent() != nullptr)
	{
		ArticyObject = ArticyObject->GetParent();
		if (UArticyFlowFragment* FlowFragment = Cast<UArticyFlowFragment>(ArticyObject))
		{
			return FlowFragment;
		}
	}
	return nullptr;
}

void UArticyPlayerManager::SetDialFragmentsDataTable(UDataTable* DataTable)
{
	DialFragmentsDataTable = DataTable;
}

void UArticyPlayerManager::StartDialogueById(FArticyId NewId)
{
	if (!IsValid(ArticyFlowPlayer))
	{
		UE_LOG(LogTemp, Error, TEXT("AArticyFlowPlayerActor::StartDialogueById - ArticyFlowPlayer Component is NULL!"));
		return;
	}
	UArticyDialogue* CurrentArticyDialogue = NewId.GetObject<UArticyDialogue>(GetWorldContext());

			
	if (bIsAddToHistory)
	{
		UHistoryData* HistoryData = StartDialogueToHistory(CurrentArticyDialogue);
		
		FProgressHistory L_ProgressHistory;
		L_ProgressHistory.ProgressState = GetProgressState();
		L_ProgressHistory.bIsStarted = true;
		L_ProgressHistory.ObjectId = NewId;
		ProgressHistory.Add(HistoryData, L_ProgressHistory);
	}
	
	FindDialFragmentsDataTable(GetParentFlowFragment(CurrentArticyDialogue));

	OnShowDialogueUI.Broadcast();
	
	bIsDialogueActive = true;
	ArticyFlowPlayer->SetStartNodeById(NewId);
}

void UArticyPlayerManager::CloseDialogue()
{
	if (!AlreadyTalkedSet.IsEmpty())
	{
		for (FNPCInfo* NPCInfoPtr : AlreadyTalkedSet)
		{
			NPCInfoPtr->bIsAlreadyTalked = true;
		}
	}
	Reset();
	if (bIsAddToHistory)
	{
		AddToHistory(nullptr, EHistoryType::EndDialogue);
	}
	
}

bool UArticyPlayerManager::ContinueDialogue()
{
	if (GetIsShowAnswers() || !bIsDialogueActive)
	{
		return false;
	}
	
	return ArticyFlowPlayer->Play(0);
}

void UArticyPlayerManager::BranchSelectedHandler(int32 BranchIndex)
{
	if (!GetIsShowAnswers())
	{
		return;
	}
	
	OnHideBranches.Broadcast();
	ArticyFlowPlayer->Play(BranchIndex);
	bIsShowAnswers = false;
	
	OnAutoSkip.Broadcast();
}

bool UArticyPlayerManager::SelectBranchByName(const FName& BranchName)
{
	if (!GetIsShowAnswers())
	{
		return false;
	}
	
	for (auto AvailableBranch : ArticyFlowPlayer->GetAvailableBranches())
	{
		if (UArticyObject* ArticyObject = Cast<UArticyObject>(AvailableBranch.GetTarget().GetObjectRef()))
		{
			UE_LOG(LogTemp, Log, TEXT("%s"), *ArticyObject->GetTechnicalName().ToString());
			if (ArticyObject->GetTechnicalName() == BranchName)
			{
				OnHideBranches.Broadcast();
				ArticyFlowPlayer->PlayBranch(AvailableBranch);
				bIsShowAnswers = false;
				return true;
			}
		}				
	}

	return false;
}

void UArticyPlayerManager::ResetProgress()
{
	ProgressState.AllNPCs.Empty();
	ProgressState.AllInventoryItems.Empty();
	ProgressState.AllQuestStates.Empty();
	ProgressState.DoOnceConditions.Empty();
}

void UArticyPlayerManager::SetIsAddToHistory(bool bValue)
{
	bIsAddToHistory = bValue;
}

FArticyDialFragment UArticyPlayerManager::GetDialFragmentRow(UArticyDialogueFragment* DialogueFragment)
{
	FArticyDialFragment DialFragmentRow;
	if (FArticyDialFragment* DialFragmentRowPtr = GetDialFragmentRowPtr(DialogueFragment))
	{
		DialFragmentRow = *DialFragmentRowPtr;
	}
	return DialFragmentRow;
}

FArticyQuest UArticyPlayerManager::GetQuestRow(UBBEntityQuest* EntityQuest)
{
	FArticyQuest ArticyQuestRow;
	if (FArticyQuest* ArticyQuestPtr = GetQuestRowPtr(EntityQuest))
	{
		ArticyQuestRow = *ArticyQuestPtr;
	}
	return ArticyQuestRow;
}

FArticyItem UArticyPlayerManager::GetItemRow(UBBEntityItem* EntityItem)
{
	FArticyItem ArticyItemRow;
	if (FArticyItem* ArticyItemPtr = GetItemRowPtr(EntityItem))
	{
		ArticyItemRow = *ArticyItemPtr;
	}
	return ArticyItemRow;
}

FArticyCharacter UArticyPlayerManager::GetCharacterRow(UArticyObject* EntityCharacter)
{
	FArticyCharacter ArticyCharacterRow;
	if (FArticyCharacter* ArticyCharacterPtr = GetCharacterRowPtr(EntityCharacter))
	{
		ArticyCharacterRow = *ArticyCharacterPtr;
	}
	return ArticyCharacterRow;
}

FProgressState UArticyPlayerManager::GetProgressState()
{
	return ProgressState;
}

void UArticyPlayerManager::SetProgressState(const FProgressState& InProgressState)
{
	ProgressState = InProgressState;
}

void UArticyPlayerManager::InstructionExecutedHandler(UArticyInstruction* ArticyInstruction)
{
	if (UBBQuestInstruction* QuestInstruction = Cast<UBBQuestInstruction>(ArticyInstruction))
	{
		if (!QuestInstruction->QuestStateFeature->QuestEntity.IsNull())
		{
			ExecQuestInstruction(QuestInstruction);
		}
	}
	else if (UBBItemInstruction* ItemInstruction = Cast<UBBItemInstruction>(ArticyInstruction))
	{
		if (!ItemInstruction->ItemChangeProperties->Item.IsNull())
		{
			ExecItemInstruction(ItemInstruction);
		}
	}
	else if (UBBNPC_LoaltyAddInstruction* LoyaltyInstruction = Cast<UBBNPC_LoaltyAddInstruction>(ArticyInstruction))
	{
		if (!LoyaltyInstruction->NPC_SetLoyaltyFeature->ReferenceSlot.IsNull())
		{
			const UBBNPC* NPC = Cast<UBBNPC>(LoyaltyInstruction->NPC_SetLoyaltyFeature->ReferenceSlot.GetObject(this));

			FNPCInfo* NPCInfoPtr = GetNPCInfo(NPC);
			NPCInfoPtr->Loyalty = FMath::Clamp(NPCInfoPtr->Loyalty + LoyaltyInstruction->NPC_SetLoyaltyFeature->Count, -100, 100);


			if (bIsAddToHistory)
			{
				AddToHistory(LoyaltyInstruction, EHistoryType::Loyalty, FDataTableRowHandle());
			}
		}
	}
}

UHistoryData* UArticyPlayerManager::StartDialogueToHistory(UArticyDialogue* Dialogue)
{
	return AddToHistory(Dialogue, EHistoryType::StartDialogue);
}

void UArticyPlayerManager::ConditionExecutedHandler(UArticyCondition* ArticyCondition, bool bIsSelected)
{
	if (const UBBDo_Once* DoOnce = Cast<UBBDo_Once>(ArticyCondition))
	{
		if (bIsSelected)
		{			
			ProgressState.DoOnceConditions.Add(DoOnce->GetTechnicalName());
		}
	}
}

void UArticyPlayerManager::DialogueFragmentHandler(UArticyDialogueFragment* DialogueFragment)
{
	const FArticyDialFragment* DialFragmentRow = GetDialFragmentRowPtr(DialogueFragment);
	if (DialFragmentRow == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Dialogue Fragment \"%s\" is not found! "), *DialogueFragment->GetTechnicalName().ToString());
		return;
	}
	const FText& PhraseText = DialFragmentRow->Text;
	FText SpeakerName;

	if (IArticyObjectWithSpeaker* ArticyObjectWithSpeaker = Cast<IArticyObjectWithSpeaker>(DialogueFragment))
	{
		if (UArticyObject* Speaker = ArticyObjectWithSpeaker->GetSpeaker())
		{
			const FArticyCharacter* CharacterRow = GetCharacterRowPtr(Speaker);
			if (CharacterRow == nullptr)
			{
				UE_LOG(LogTemp, Error, TEXT("Character \"%s\" is not found! "), *Speaker->GetTechnicalName().ToString());
				return;
			}
			SpeakerName = CharacterRow->CharacterName;

			//Здесь проверяется нужно ли добавить персонажа в список "уже говоривших", чтобы при закрытии диалога обновить информацию об этом поле
			if (Speaker->GetTechnicalName() != PlayerCharacterTechnicalName)
			{
				if (const UBBNPC* NPCSpeaker = Cast<UBBNPC>(Speaker))
				{
					FNPCInfo* NPCInfoPtr = GetNPCInfo(NPCSpeaker);
					if (!AlreadyTalkedSet.Find(NPCInfoPtr) && !NPCInfoPtr->bIsAlreadyTalked)
					{
						AlreadyTalkedSet.Add(NPCInfoPtr);
					}
				}
			}
		}		
	}
		
	OnShowDialogueFragment.Broadcast(SpeakerName, PhraseText);
		
	if (bIsAddToHistory)
	{
		FDataTableRowHandle QuestsRowHandle;
		QuestsRowHandle.DataTable = DialFragmentsDataTable;
		QuestsRowHandle.RowName = DialogueFragment->GetTechnicalName();
		if (GetIsShowAnswers())
		{
			UHistoryData* HistoryData = AddToHistory(DialogueFragment, EHistoryType::SelectedDialFragment, QuestsRowHandle);

			FProgressHistory L_ProgressHistory;
			L_ProgressHistory.ProgressState = GetProgressState();
			L_ProgressHistory.ObjectId = LastHubId;
			ProgressHistory.Add(HistoryData, L_ProgressHistory);
		}
		else
		{
			AddToHistory(DialogueFragment, EHistoryType::DialFragment, QuestsRowHandle);
		}
	}
}

bool UArticyPlayerManager::GetIsDialogueActive()
{
	return bIsDialogueActive;
}

bool UArticyPlayerManager::GetIsShowAnswers()
{
	return bIsShowAnswers;
}

TMap<int32, int32> UArticyPlayerManager::GetBranchIds()
{
	return BranchIds;
}

FArticyDialFragment* UArticyPlayerManager::GetDialFragmentRowPtr(UArticyDialogueFragment* DialogueFragment)
{
	return DialFragmentsDataTable->FindRow<FArticyDialFragment>(
		DialogueFragment->GetTechnicalName(), TEXT("DialFragmentRow"));
}

FArticyQuest* UArticyPlayerManager::GetQuestRowPtr(UBBEntityQuest* EntityQuest)
{
	return QuestsDataTable->FindRow<FArticyQuest>(EntityQuest->GetTechnicalName(), TEXT("ArticyQuestRow"));
}

FArticyItem* UArticyPlayerManager::GetItemRowPtr(UBBEntityItem* EntityItem)
{
	return ItemsDataTable->FindRow<FArticyItem>(EntityItem->GetTechnicalName(), TEXT("ArticyItemRow"));
}

FArticyCharacter* UArticyPlayerManager::GetCharacterRowPtr(UArticyObject* EntityCharacter)
{
	return CharactersDataTable->FindRow<FArticyCharacter>(EntityCharacter->GetTechnicalName(), TEXT("ArticyCharacterRow"));
}

FNPCInfo* UArticyPlayerManager::GetNPCInfo(const UBBNPC* NPC)
{
	FNPCInfo* NPCInfoPtr = ProgressState.AllNPCs.Find(NPC->GetTechnicalName());
	if (NPCInfoPtr == nullptr)
	{
		FNPCInfo NPCInfo;
		NPCInfo.Loyalty = NPC->NPC_Info->Loyalty;
		NPCInfo.bIsAlreadyTalked = false;
		
		return &ProgressState.AllNPCs.Add(NPC->GetTechnicalName(), NPCInfo);
	}
	return NPCInfoPtr;
}

void UArticyPlayerManager::ExecQuestInstruction(UBBQuestInstruction* QuestInstruction)
{
	UBBEntityQuest* EntityQuest = Cast<UBBEntityQuest>(QuestInstruction->QuestStateFeature->QuestEntity.GetObject(this));
	ProgressState.AllQuestStates.Add(EntityQuest->GetTechnicalName(), QuestInstruction->QuestStateFeature->State);

	if (FArticyQuest* QuestRowPtr = GetQuestRowPtr(EntityQuest))
	{
		if (bIsAddToHistory)
		{
			FDataTableRowHandle QuestsRowHandle;
			QuestsRowHandle.DataTable = QuestsDataTable;
			QuestsRowHandle.RowName = EntityQuest->GetTechnicalName();			
			AddToHistory(QuestInstruction, EHistoryType::Quest, QuestsRowHandle);
		}
		
		FString Msg = "QUEST " + QuestRowPtr->Description.ToString() + " " + 
			StaticEnum<EBBQuestStateList>()->GetDisplayNameTextByValue((int64)QuestInstruction->QuestStateFeature->State).ToString() + "!";
		GEngine->AddOnScreenDebugMessage(-1, 7.0f, FColor::Yellow, *Msg);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Quest %s is not found in data table!"), *EntityQuest->GetTechnicalName().ToString());
	}
}

void UArticyPlayerManager::ExecItemInstruction(UBBItemInstruction* ItemInstruction)
{
	UBBEntityItem* ItemObject = Cast<UBBEntityItem>(ItemInstruction->ItemChangeProperties->Item.GetObject(this));

	const FArticyItem* ItemRowPtr = GetItemRowPtr(ItemObject);

	if (ItemRowPtr == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Item %s is not found in data table!"), *ItemObject->GetTechnicalName().ToString());
	}
	else
	{
		FString Msg = "ITEM " + ItemRowPtr->ItemName.ToString() + (ItemInstruction->ItemChangeProperties->bIsAdded ? " ADDED: " : " REMOVED: ") +
			FString::FromInt(ItemInstruction->ItemChangeProperties->Count);
		GEngine->AddOnScreenDebugMessage(-1, 7.0f, FColor::Green, *Msg);
	}
	
	if (auto Count = ProgressState.AllInventoryItems.Find(ItemObject->GetTechnicalName()))
	{
		if (ItemInstruction->ItemChangeProperties->bIsAdded)
		{
			ProgressState.AllInventoryItems[ItemObject->GetTechnicalName()] = *Count + ItemInstruction->ItemChangeProperties->Count;
		}
		else
		{
			ProgressState.AllInventoryItems[ItemObject->GetTechnicalName()] = FMath::Max(*Count - ItemInstruction->ItemChangeProperties->Count, 0);
		}
	}
	else 
	{
		if (ItemInstruction->ItemChangeProperties->bIsAdded)
		{
			ProgressState.AllInventoryItems.Add(ItemObject->GetTechnicalName(), ItemInstruction->ItemChangeProperties->Count);
		}
		else if (!bIsAddToHistory) // Если не тест
			{
			UE_LOG(LogTemp, Warning, TEXT("В инвентаре нет предметов \"%s\", а инструкция \"%s\" хочет забрать из инвентаря предмет!"),
				*ItemInstruction->GetTechnicalName().ToString(), *ItemObject->GetTechnicalName().ToString());
			}
	}
	
	if (bIsAddToHistory)
	{
		FDataTableRowHandle QuestsRowHandle;
		QuestsRowHandle.DataTable = ItemsDataTable;
		QuestsRowHandle.RowName = ItemObject->GetTechnicalName();
		AddToHistory(ItemInstruction, EHistoryType::Item, QuestsRowHandle);
	}
}

void UArticyPlayerManager::CheckCondition(UArticyCondition* ArticyCondition, int& SelectedPin)
{
	if (const UBBGetQuestState* CheckQuestState = Cast<UBBGetQuestState>(ArticyCondition))
	{
		UArticyObject* ArticyObject = CheckQuestState->QuestStateFeature->QuestEntity.GetObject<UArticyObject>(this);
		const UBBEntityQuest* QuestEntity = Cast<UBBEntityQuest>(ArticyObject);
		if (QuestEntity == nullptr)
		{
			return;
		}
		
		if (const auto QuestStatePtr = ProgressState.AllQuestStates.Find(QuestEntity->GetTechnicalName()))
		{
			SelectedPin = (CheckQuestState->QuestStateFeature->State == *QuestStatePtr) ? 0 : 1;
		}
		else
		{
			SelectedPin = (CheckQuestState->QuestStateFeature->State == EBBQuestStateList::None) ? 0 : 1;
		}
	}
	else if (const UBBGetItemsCount* CheckItem = Cast<UBBGetItemsCount>(ArticyCondition))
	{
		const UBBEntityItem* ItemEntity = Cast<UBBEntityItem>(CheckItem->ItemsCountFeature->Item.GetObject<UArticyObject>(this));
		if (ItemEntity == nullptr)
		{
			return;
		}
		
		const int32* CountPtr = ProgressState.AllInventoryItems.Find(ItemEntity->GetTechnicalName());
		if (CountPtr != nullptr)
		{
			SelectedPin = (*CountPtr >= CheckItem->ItemsCountFeature->Count) ? 0 : 1;
		}
		else
		{
			SelectedPin = (CheckItem->ItemsCountFeature->Count == 0) ? 0 : 1;
		}
	}
	else if (const UBBNPC_GetIsLoyaltyGreaterValue* NPCGreaterLoyalty = Cast<UBBNPC_GetIsLoyaltyGreaterValue>(ArticyCondition))
	{
		const UBBNPC* NPC = Cast<UBBNPC>(NPCGreaterLoyalty->NPC_GetLoyaltyFeature->NPC.GetObject<UArticyObject>(this));
		if (NPC == nullptr)
		{
			return;
		}

		const FNPCInfo* NPCInfo = GetNPCInfo(NPC);
		if (NPCGreaterLoyalty->NPC_GetLoyaltyFeature->IncludeValue)
		{
			SelectedPin = NPCInfo->Loyalty >= NPCGreaterLoyalty->NPC_GetLoyaltyFeature->Value ? 0 : 1;
		}
		else
		{
			SelectedPin = NPCInfo->Loyalty > NPCGreaterLoyalty->NPC_GetLoyaltyFeature->Value ? 0 : 1;
		}
		
	}
	else if (const UBBNPC_GetIsLoyaltyLessValue* NPCLessLoyalty = Cast<UBBNPC_GetIsLoyaltyLessValue>(ArticyCondition))
	{
		const UBBNPC* NPC = Cast<UBBNPC>(NPCLessLoyalty->NPC_GetLoyaltyFeature->NPC.GetObject<UArticyObject>(this));
		if (NPC == nullptr)
		{
			return;
		}

		const FNPCInfo* NPCInfo = GetNPCInfo(NPC);
		if (NPCLessLoyalty->NPC_GetLoyaltyFeature->IncludeValue)
		{
			SelectedPin = NPCInfo->Loyalty <= NPCLessLoyalty->NPC_GetLoyaltyFeature->Value ? 0 : 1;
		}
		else
		{
			SelectedPin = NPCInfo->Loyalty < NPCLessLoyalty->NPC_GetLoyaltyFeature->Value ? 0 : 1;
		}		
	}
	else if (const UBBNPC_GetIsAlreadyTalked* NPCIsAlreadyTalked = Cast<UBBNPC_GetIsAlreadyTalked>(ArticyCondition))
	{
		const UBBNPC* NPC = Cast<UBBNPC>(NPCIsAlreadyTalked->NPC_IsAlreadyTalkedFeature->ReferenceSlot.GetObject<UArticyObject>(this));
		if (NPC == nullptr)
		{
			return;
		}

		const FNPCInfo* NPCInfo = GetNPCInfo(NPC);
		if (NPCIsAlreadyTalked->NPC_IsAlreadyTalkedFeature->IsAlreadyTalked)
		{
			SelectedPin = NPCInfo->bIsAlreadyTalked ? 0 : 1;
		}
		else
		{
			SelectedPin = NPCInfo->bIsAlreadyTalked ? 1 : 0;
		}
	}
	else if (const UBBDo_Once* Do_Once = Cast<UBBDo_Once>(ArticyCondition))
	{		
		if (ProgressState.DoOnceConditions.Find(Do_Once->GetTechnicalName()))
		{
			SelectedPin = 1;
		}
		else
		{
			SelectedPin = 0;
		}
	}
}

void UArticyPlayerManager::BranchesDoneHandler()
{
	CloseDialogue();
}

void UArticyPlayerManager::Reset()
{
	bIsDialogueActive = false;
	bIsShowAnswers = false;
	OnHideBranches.Broadcast();
	OnHideDialogueUI.Broadcast();
}

void UArticyPlayerManager::SetDataTables()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	FString DTAssetPath;
	GConfig->GetString(TEXT("/Script/ArticyRuntime.ArticyPluginSettings"), TEXT("CharactersTableName"), DTAssetPath, GEngineIni);

	const FAssetData& CharactersAssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(
		*(GetDefault<UArticyPluginSettings>()->DataTablesDir.Path + "/" + DTAssetPath + "." + DTAssetPath)));
	CharactersDataTable = Cast<UDataTable>(CharactersAssetData.GetAsset());

	GConfig->GetString(TEXT("/Script/ArticyRuntime.ArticyPluginSettings"), TEXT("QuestsTableName"), DTAssetPath, GEngineIni);
	const FAssetData& QuestsAssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(
		*(GetDefault<UArticyPluginSettings>()->DataTablesDir.Path + "/" + DTAssetPath + "." + DTAssetPath)));
	QuestsDataTable = Cast<UDataTable>(QuestsAssetData.GetAsset());

	GConfig->GetString(TEXT("/Script/ArticyRuntime.ArticyPluginSettings"), TEXT("ItemsTableName"), DTAssetPath, GEngineIni);
	const FAssetData& ItemsAssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(
		*(GetDefault<UArticyPluginSettings>()->DataTablesDir.Path + "/" + DTAssetPath + "." + DTAssetPath)));
	ItemsDataTable = Cast<UDataTable>(ItemsAssetData.GetAsset());
}

UHistoryData* UArticyPlayerManager::AddToHistory(UArticyObject* ArticyObject, EHistoryType Type,
                                             FDataTableRowHandle RowHandle)
{
	UHistoryData* HistoryData = NewObject<UHistoryData>();
	HistoryData->ArticyObject = ArticyObject;
	HistoryData->Type = Type;
	HistoryData->DataTableRowHandle = RowHandle;
	OnAddedToHistory.Broadcast(HistoryData);

	return HistoryData;
}