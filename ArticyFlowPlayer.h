//  
// Copyright (c) articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyRuntimeModule.h"
#include "ArticyDatabase.h"
#include "ArticyGlobalVariables.h"
#include "ArticyRef.h"
#include "ArticyData.h"
#include "ArticyFlowPlayer.generated.h"

class UArticyPlayerManager;
class IArticyNode;
class IArticyFlowObject;
class UArticyInstruction;
class UArticyCondition;
class UArticyHub;


/**
 * This component handles traversal of the flow, starting and halting at specific nodes.
 * The GlobalVariables instance and the UserMethodProvider used for this flow player
 * instance can be specified.
 */
UCLASS(BlueprintType)
class ARTICYRUNTIME_API UArticyFlowPlayer : public UObject
{
	GENERATED_BODY()

public:

	void BeginPlay();
	
	void SetOwner(UArticyPlayerManager* InOwner);
	
	UArticyPlayerManager* GetOwner();

	//---------------------------------------------------------------------------//

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup", meta=(Bitmask, BitmaskEnum = "EArticyPausableType"))
	uint8 PauseOn = 1 << uint8(EArticyPausableType::DialogueFragment)
					| 1 << uint8(EArticyPausableType::Dialogue)
					| 1 << uint8(EArticyPausableType::FlowFragment);


	/**
	 * Pushes a shadow state, executes the operation, then pops the shadow state back.
	 * The method is 'const' to eliminate side-effects (except for ShadowLevel).
	 */
	template<typename Lambda>
	void ShadowedOperation(Lambda Operation) const;

	uint32 GetShadowLevel() const { return ShadowLevel; }

	//---------------------------------------------------------------------------//

	/** Set the StartOn node to a certain node. */
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Start Node (ArticyRef)"), Category = "Setup")
	void SetStartNode(FArticyRef NewId);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Start Node (ArticyID)"), Category = "Setup")
	void SetStartNodeById(FArticyId NewId);

	/** Set the StartOn node to a certain node. */
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Start Node (FlowObject)"), Category = "Setup")
	void SetStartNodeWithFlowObject(TScriptInterface<IArticyFlowObject> Node);

	/** Gets the last set StartOn node */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Start Node"), Category = "Setup")
	FArticyRef GetStartNode() { return StartOn; }

	/** Gets the last set StartOn node */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Ignore Invalid Branches"), Category = "Setup")
	void SetIgnoreInvalidBranches(bool bNewIgnoreInvalidBranches) { bIgnoreInvalidBranches = bNewIgnoreInvalidBranches; }
		
	/** Set the Cursor (current node) to this Node and updates the available branches. */
	UFUNCTION(BlueprintCallable, Category = "Flow")
	void SetCursorTo(TScriptInterface<IArticyFlowObject> Node);

	/** Get the Cursor (current node). */
	UFUNCTION(BlueprintCallable, Category = "Flow")
	const TScriptInterface<IArticyFlowObject> GetCursor() const { return Cursor; }

	/** Play a branch, identified by its index in the AvailableBranches array. */
	UFUNCTION(BlueprintCallable, Category="Flow")
	bool Play(int Index = 0);

	/** Calls the script on an output pin of the current object */
	UFUNCTION(BlueprintCallable, Category = "Flow")
	void FinishCurrentPausedObject(int PinIndex = 0);

	/**
	 * Traverse this branch to the end, executing all scripts and updating the Cursor
	 * to the branche's target.
	 */
	UFUNCTION(BlueprintCallable, Category = "Flow")
	void PlayBranch(const FArticyBranch& Branch);

	//---------------------------------------------------------------------------//

	/**
	 * Gather all branches that start from this node.
	 * The explore can be performed as shadowed operation.
	 * If the node is submergeable, a submerge is performed.
	 */
	TArray<FArticyBranch> Explore(IArticyFlowObject* Node, bool bShadowed, int32 Depth, bool IncludeCurrent = true);

	void SetPauseOn(EArticyPausableType Types);
	/** Returns true if Node is one of the PauseOn types. */
	bool ShouldPauseOn(IArticyFlowObject* Node) const;

	UFUNCTION(BlueprintCallable, Category="Flow")
	bool ShouldPauseOn(TScriptInterface<IArticyFlowObject> Node) const;
	
	/**
	 * Get the GV instance used for expresso script execution.
	 */
	UFUNCTION(BlueprintCallable, Category="Setup")
	UArticyGlobalVariables* GetGVs() const;

	/**
	 * Get the UserMethodsProvider used for resolving user defined methods.
	 */
	UFUNCTION(BlueprintPure, Category = "Setup")
	UObject* GetMethodsProvider() const;

	//---------------------------------------------------------------------------//

	/** Explore branches starting from the current StartOn node. */
	UFUNCTION(BlueprintCallable, Category="Flow")
	void UpdateAvailableBranches();

	/** Get the currently available branches. This does not re-explore the branches. */
	UFUNCTION(BlueprintCallable, Category="Flow")
	const TArray<FArticyBranch>& GetAvailableBranches() const { return AvailableBranches; }

	//---------------------------------------------------------------------------//

	/** Wether bIgnoreInvalidBranches is set. */
	UFUNCTION(BlueprintCallable, Category="Setup")
	bool IgnoresInvalidBranches() const { return bIgnoreInvalidBranches; }

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPushState);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPopState);
	DECLARE_DELEGATE_OneParam(FOnPlayerPaused, TScriptInterface<IArticyFlowObject> PausedOn);
	DECLARE_DELEGATE_OneParam(FOnBranchesUpdated, const TArray<FArticyBranch>& AvailableBranches);
	DECLARE_DELEGATE(FOnBranchesDone);
	DECLARE_DELEGATE_OneParam(FOnInstructionExecuted, UArticyInstruction* ArticyInstruction);
	DECLARE_DELEGATE_TwoParams(FOnConditionExecuted, UArticyCondition* ArticyCondition, bool IsEnter);
	DECLARE_DELEGATE_TwoParams(FOnCustomConditionSelected, UArticyCondition* ArticyCondition, int& SelectedPin);


	/** This event is broadcast whenever a new ShadowedOperation starts. */
	UPROPERTY(BlueprintAssignable, Category = "Flow")
	FOnPushState OnShadowOpStart;

	/** This event is broadcast once the current ShadowedOperation ends. */
	UPROPERTY(BlueprintAssignable, Category = "Flow")
	FOnPopState OnShadowOpEnd;

	/**
	 * This delegate is called when the flow player pauses on one of the object types set up in
	 * "PauseOn", or if the branch is done.
	 */
	// UPROPERTY(BlueprintAssignable, Category = "Flow")
	FOnPlayerPaused OnPlayerPaused;

	/**
	 * This delegate is called when the AvailableBranches change.
	 */
	// UPROPERTY(BlueprintAssignable, Category = "Flow")
	FOnBranchesUpdated OnBranchesUpdated;

	/** NIKO
	 * This delegate is called when the instruction is execute.
	 */
	FOnInstructionExecuted OnInstructionExecuted;

	/** NIKO
	 * This delegate is called when the instruction is execute.
	 */
	FOnConditionExecuted OnConditionExecuted;

	/** NIKO
	 */
	FOnCustomConditionSelected OnCustomConditionSelected;


	/** NIKO
	 * This delegate is called when the branch is done.
	 */
	// UPROPERTY(BlueprintAssignable, Category = "Flow")
	FOnBranchesDone OnBranchesDone;

	UPROPERTY(Transient)
	UArticyHub* CurrentHubNode;

protected:

	//========================================//

	/**
	 * If this number of branches is reached, no more branches will be added.
	 */
	// [Alewinn] : no valid reference in code, limit most probably set at Articy side.
	//UPROPERTY(EditAnywhere, Category = "Setup", meta=(ClampMin=0))
	//int32 BranchLimit = 32;

	/**
	 * If a branch reaches this length, exploration on it is aborted.
	 */
	UPROPERTY(EditAnywhere, Category = "Setup", meta=(ClampMin=0))
	int32 ExploreDepthLimit = 128;

	/**
	 * If more than this amount of ShadowLevels are needed at the same time,
	 * branch exploration will abort.
	 */
	UPROPERTY(EditAnywhere, Category = "Setup")
	uint8 ShadowLevelLimit = 10;

	/**
	 * Invalid branches will not be part of the AvailableBranches.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	bool bIgnoreInvalidBranches = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (ArticyClassRestriction = "ArticyNode"))
	FArticyRef StartOn;

	/** All the branches available at the current flow position. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flow")
	TArray<FArticyBranch> AvailableBranches;

	//---------------------------------------------------------------------------//

	/**
	 * The instance of the global variables to be used with this flow player instance.
	 * This property is private so it can only be directly changed through the editor,
	 * so we can ensure that the needed setup is triggered every time this changes.
	 * 
	 * Keep as nullptr if you want to use the default shared global variables.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Setup")
	UArticyAlternativeGlobalVariables* OverrideGV = nullptr;

	/**
	 * This object will be used to resolve user-defined ScriptMethods. It must implement
	 * the project-specific I<ProjectName>MethodsProvider interface (through blueprint or
	 * through inheritance).
	 * This property is private so it can only be directly changed through the editor,
	 * so we can ensure that the needed setup is triggered every time this changes.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Setup")
	mutable UObject* UserMethodsProvider;

	UPROPERTY()
	UArticyPlayerManager* Owner;
private:
	/** The current shadow level (0 == live state). */
	UPROPERTY(Transient, VisibleAnywhere, Category="Debug")
	mutable uint32 ShadowLevel = 0;

private:
	/**
	 * Updates the list of available branches.
	 * 
	 * Use Startup if this is the first time we're creating branches for a starting node.
	 * This will make sure FastForward gets called and we end up on a valid PauseOn node.
	 */
	void UpdateAvailableBranchesInternal(bool Startup);

	/** The current position in the flow. */
	UPROPERTY(Transient)
	TScriptInterface<IArticyFlowObject> Cursor = nullptr;

	/** Set the Cursor to the object referenced by StartOn. */
	void SetCursorToStartNode();

	/**
	 * Fast-forwards the Cursor to the next PauseOn node, or to the next node where
	 * the path branches out.
	 */
	bool FastForwardToPause();

	/** Returns a ptr to the unshadowed object of this node */
	IArticyFlowObject* GetUnshadowedNode(IArticyFlowObject* Node);

	UArticyDatabase* GetDB() const;
	UArticyExpressoScripts* GetExpresso() const;
};

//---------------------------------------------------------------------------//
// TEMPLATED METHODS
//---------------------------------------------------------------------------//

template<typename Lambda>
void UArticyFlowPlayer::ShadowedOperation(Lambda Operation) const
{
	if(std::is_null_pointer<Lambda>::value)
		return;
	if(!ensure(GetGVs()))
	{
		UE_LOG(LogArticyRuntime, Warning, TEXT("FlowPlayer cannot get GVs!"));
		return;
	}
	if(!ensure(ShadowLevel < ShadowLevelLimit))
	{
		UE_LOG(LogArticyRuntime, Warning, TEXT("More than 100 nested ShadowedOperations, probably there's an infinite loop in the Flow!"));
		return;
	}

	//push shadow state
	++ShadowLevel;

	//notify on push
	GetGVs()->PushState(ShadowLevel);
	UArticyDatabase::Get(this)->PushState(ShadowLevel);
	OnShadowOpStart.Broadcast();

	//execute the operation
	Operation();

	//notify on pop
	OnShadowOpEnd.Broadcast();
	UArticyDatabase::Get(this)->PopState(ShadowLevel);
	GetGVs()->PopState(ShadowLevel);

	//pop shadow state
	if(ensure(ShadowLevel > 0))
		--ShadowLevel;
}

UCLASS(BlueprintType, HideCategories=(Replication, Physics, Rendering, Input, Collision, Actor, LOD, Cooking))
class AArticyFlowDebugger : public AActor
{
	GENERATED_BODY()
	
public:
	AArticyFlowDebugger();
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Articy")
	UArticyFlowPlayer* FlowPlayer = nullptr;
};