// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"
#include "testinginBP/Ball/CPPBall.h"
#include "Components/PrimitiveComponent.h"
#include "CPPTestCharacter.generated.h"

UCLASS()
class TESTINGINBP_API ACPPTestCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACPPTestCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override; //Replication
	virtual void PostInitializeComponents() override;


	void PlayThrowMontage();
	UFUNCTION(BlueprintCallable, Category = "BallThrow")
		void OnBallReleased();
	UPROPERTY(visibleAnywhere, Category = Camera)
		class UCameraComponent* followCamera;

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly,  meta = (AllowPrivateAccess = "true"))
		class ULockOnTargetComponent* lockOnTargets;

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly,  meta = (AllowPrivateAccess = "true"))
		class UTargetingHelperComponent* targetComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		AActor* FoundTarget;

	UFUNCTION(BlueprintCallable)
	void ThrowButtonPressed();

protected:
	virtual void BeginPlay() override;

	void MoveForward(float value);
	void MoveRight(float value);
	void Turn(float value);
	void LookUp(float value);
	void EquipButtonPressed();
	void Catch();
	void Dash();
	void CanDash();
	//void CanLock();
	void CanCatch();
	void ThrowButtonReleased();
	void LockTarget();
	USkeletalMeshComponent* CharacterMesh;

private:
	UPROPERTY(visibleAnywhere, Category = Camera)
		class USpringArmComponent* cameraBoom;
	

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UWidgetComponent* overHeadWidget;
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = Animations, meta = (AllowPrivateAccess = "true"))
		class UAnimMontage* throwAnim;
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = Animations, meta = (AllowPrivateAccess = "true"))
		class UAnimMontage* DashAnim;
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = Animations, meta = (AllowPrivateAccess = "true"))
		class UAnimMontage* CatchAnim;
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingBall) //Replication
		class ACPPBall* overlappingBall;
;
	

	UFUNCTION()
	void OnRep_OverlappingBall(ACPPBall* lastBall); //Replication

	UPROPERTY(visibleAnywhere)
	class UCombatComponent* combat;

	UPROPERTY(visibleAnywhere)
	class UGameplayStatics* gameStatic;

	UPROPERTY(visibleAnywhere)
	class UWorld* world;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	UFUNCTION(Server, Reliable)
		void ServerCatch();

	UFUNCTION(Server, Reliable)
		void ServerThrowButtonPressed();

	UFUNCTION(Server, Reliable)
		void ServerThrowButtonReleased();

	UFUNCTION(Server, Reliable)
	void DashButtonPressed();

	UFUNCTION()
		void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
			bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(Server, Reliable, WithValidation, Category = Animation)
		void ServerPlayAnimMontage(class UAnimMontage* AnimMontage, float InPlayRate = 1.f, FName StartSectionName = NAME_None);

	UFUNCTION(NetMulticast, Reliable, WithValidation, Category = Animation)
		void MulticastPlayAnimMontage(class UAnimMontage* AnimMontage, float InPlayRate = 1.f, FName StartSectionName = NAME_None);

	

public:	
	 void SetOverlappingBall(ACPPBall* cppBall);

	 bool IsBallEquipped();

	 bool bCanDash;

	 bool bThrown;

	 bool bCanThrow;

	UPROPERTY(Replicated)
	 bool bCatching;

	 UPROPERTY(Replicated)
	 bool bEquipped;

	 UPROPERTY(Replicated)
		 bool bSlerps;

	 UPROPERTY(Replicated)
		 bool bisLocked;

	 UFUNCTION()
		 void MyLockedThrow();

	 UFUNCTION()
		 void MyThrow();

	 //UFUNCTION(NetMulticast, Reliable)
		// void MyServerLockedThrow();

	 //UFUNCTION(NetMulticast, Reliable)
		// void MyServerThrow();

	 //UFUNCTION(NetMulticast, Reliable)
		// void ServerLockTarget();

	 void StopThrow();

	UPROPERTY(EditAnywhere, Replicated, Category = "Movement")
	 float DashDistance = 6000.f;

	UPROPERTY(EditAnywhere, Replicated, Category = "Throw power")
		float throwPower = 500.0f;
	UPROPERTY(EditAnywhere, Category = "Catching")
		float CatchCooldown = 10.f;

	FVector testVect;

	

	

	

};
