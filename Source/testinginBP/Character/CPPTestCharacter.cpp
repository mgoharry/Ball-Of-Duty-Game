

#include "CPPTestCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h" //Replication
#include "testinginBP\Ball\CPPBall.h"
#include "testinginBP\GameComponents\CombatComponent.h"
#include "Animation/AnimMontage.h"
#include "testinginBP\Character\CPPAnimInstance.h"
#include "Kismet//KismetMathLibrary.h"
#include "LockOnTargetComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "LockOnSubobjects/RotationModes/RotationModeBase.h"
#include "LockOnSubobjects/TargetHandlers/TargetHandlerBase.h"
#include "TargetingHelperComponent.h"
#include "../../LockOnTarget/Source/LockOnTarget/Public/LockOnTargetComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"


ACPPTestCharacter::ACPPTestCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	cameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	cameraBoom->SetupAttachment(GetMesh());
	cameraBoom->TargetArmLength = 300.0f;
	cameraBoom->bUsePawnControlRotation = true;

	followCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	followCamera->SetupAttachment(cameraBoom, USpringArmComponent::SocketName);
	followCamera->bUsePawnControlRotation = false;
	

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	overHeadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	overHeadWidget->SetupAttachment(RootComponent);

	lockOnTargets = CreateDefaultSubobject<ULockOnTargetComponent>(TEXT("LockOnTarget"));
	lockOnTargets->SetIsReplicated(true);

	targetComponent = CreateDefaultSubobject<UTargetingHelperComponent>(TEXT("targetComponent"));
	targetComponent->SetIsReplicated(true);

	combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	combat->SetIsReplicated(true);

	CharacterMesh = GetMesh();

	bCanDash = true;

	bThrown = false;

	bCanThrow = false;

	bEquipped = false;

	bisLocked = false;
	//overlappingBall->SetBallState(EBallState::EBS_Dropped);
	//testVect = combat->character->GetActorForwardVector() -/*lockly->GetTarget()->GetActorForwardVector()*/;
	UE_LOG(LogTemp, Warning, TEXT("The vector value is: %s"), *testVect.ToString());

}
void ACPPTestCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//important to include #include "Net/UnrealNetwork.h" whenever we use the replication macro
	DOREPLIFETIME_CONDITION(ACPPTestCharacter, overlappingBall, COND_OwnerOnly);//Replication

	DOREPLIFETIME(ACPPTestCharacter, bEquipped);

	DOREPLIFETIME(ACPPTestCharacter, bCatching);

	DOREPLIFETIME(ACPPTestCharacter, bisLocked);

	DOREPLIFETIME(ACPPTestCharacter, throwPower);
}


void ACPPTestCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (combat)
	{
		combat->character = this;
	}
}


void ACPPTestCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority()) {
		CharacterMesh->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnOverlapBegin);
	} 
}
void ACPPTestCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
void ACPPTestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ACPPTestCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("LockOn", IE_Pressed, this, &ACPPTestCharacter::LockTarget);


	PlayerInputComponent->BindAction("Catch", IE_Pressed, this, &ACPPTestCharacter::Catch);
	PlayerInputComponent->BindAction("Dash", IE_Pressed, this, &ACPPTestCharacter::Dash);

	PlayerInputComponent->BindAction("Throw", IE_Pressed, this, &ACPPTestCharacter::ThrowButtonPressed);
	PlayerInputComponent->BindAction("Throw", IE_Released, this, &ACPPTestCharacter::ThrowButtonReleased);



	PlayerInputComponent->BindAxis("MoveForward", this, &ACPPTestCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ACPPTestCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ACPPTestCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ACPPTestCharacter::LookUp);


}

#pragma region Movement and Dashing
void ACPPTestCharacter::MoveForward(float value)
{
	if (Controller != nullptr && value != 0.0f)
	{
		const FRotator yawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		const FVector direction(FRotationMatrix(yawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(direction, value);
	}
}

void ACPPTestCharacter::MoveRight(float value)
{
	if (Controller != nullptr && value != 0.0f)
	{
		const FRotator yawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		const FVector direction(FRotationMatrix(yawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(direction, value);
	}
}

void ACPPTestCharacter::Turn(float value)
{
	AddControllerYawInput(value);
}

void ACPPTestCharacter::LookUp(float value)
{
	AddControllerPitchInput(value);
}

// Lock on Target 
void ACPPTestCharacter::LockTarget()
{
	lockOnTargets->EnableTargeting();

	//bisLocked = true;
	/*if (lockOnTargets->GetTarget())
	{
		bisLocked = true;
	}
	else
	{
		bisLocked = false;
	}*/
}

void ACPPTestCharacter::Dash()
{

	if (HasAuthority())
	{
		if (bCanDash && CanJump()) {



			if (DashAnim)
			{
				MulticastPlayAnimMontage(DashAnim, 1, NAME_None);

			}

			const FVector ForwardVector = this->GetActorRotation().Vector();
			LaunchCharacter(ForwardVector * DashDistance, true, true);

			bCanDash = false;

			FTimerHandle handle;
			GetWorld()->GetTimerManager().SetTimer(handle, this, &ThisClass::CanDash, 1.f);
		}
	}
	else
	{
		DashButtonPressed();
	}

}


void ACPPTestCharacter::CanDash()
{
	bCanDash = true;
}

void ACPPTestCharacter::CanCatch()
{
	bCatching = false;
}

void ACPPTestCharacter::DashButtonPressed_Implementation()
{
	if (bCanDash && CanJump()) {

		if (DashAnim)
		{
			PlayAnimMontage(DashAnim, 1, NAME_None);
			ServerPlayAnimMontage(DashAnim, 1, NAME_None);

		}

		const FVector ForwardVector = this->GetActorRotation().Vector();
		LaunchCharacter(ForwardVector * DashDistance, true, true);

		bCanDash = false;

		FTimerHandle handle;
		GetWorld()->GetTimerManager().SetTimer(handle, this, &ThisClass::CanDash, 1.f);
	}
}
#pragma endregion

#pragma region BallEquipping
void ACPPTestCharacter::EquipButtonPressed()
{
	if (combat && overlappingBall)
	{
		if (HasAuthority())
		{
			bEquipped = true;
			combat->EquipBall(overlappingBall);
			if (combat->equippedBall->GetBallState() == EBallState::EBS_Dropped)
				bThrown = false;
			if (combat->equippedBall->GetBallState() == EBallState::EBS_Equipped)
				bCanThrow = true;
		}
		else
		{
			ServerEquipButtonPressed();
		}
	}
}
void ACPPTestCharacter::ServerEquipButtonPressed_Implementation() //RPC
{
	if (combat)
	{
		bEquipped = true;

		combat->EquipBall(overlappingBall);
		if (combat->equippedBall->GetBallState() == EBallState::EBS_Dropped)
			bThrown = false;
		if (combat->equippedBall->GetBallState() == EBallState::EBS_Equipped)
			bCanThrow = true;

	}
}
#pragma endregion

//	Throw Mechanics TODO
//		-> Throw ball on target

#pragma region ThrowMechanics
void ACPPTestCharacter::ThrowButtonPressed()
{
	if (combat)
	{
		if (HasAuthority())
		{
			combat->ThrowButtonPressed(true);
			if (combat && bEquipped)
			{
				MyThrow();
				/*if (bisLocked == true)
				{*/
					//MyLockedThrow();
				//}
				/*else 
				{
					MyThrow();
				}*/
			}
			
		}
		else
		{
			ServerThrowButtonPressed();
		}
	}
}
void ACPPTestCharacter::ServerThrowButtonPressed_Implementation()
	{
		if (combat && IsBallEquipped())
		{
			combat->ThrowButtonPressed(true);
			
			if (combat && bEquipped)
			{
				MyThrow();
				/*if (bisLocked == true)
				{
					*/
				//MyLockedThrow();
				//}
				/*else
				{
					MyThrow();
				}*/
			}
		}
	}


void ACPPTestCharacter::ThrowButtonReleased()
{
	//if (combat && bEquipped)
	//{
	//	if (HasAuthority())
	//	{
	//		UKismetMathLibrary::GetForwardVector(GetControlRotation()) *= throwPower;

	//		const FVector forwardVec = this->GetMesh()->GetForwardVector();

	//		combat->ThrowButtonPressed(false); //gded
	//		combat->equippedBall->GetBallMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	//		combat->equippedBall->SetBallState(EBallState::EBS_Dropped);

	//		//combat->eqippedBall->DetachRootComponentFromParent(true);

	//		//combat->eqippedBall->GetBallMesh()->SetPhysicsLinearVelocity(FVector::ForwardVector);

	//		//SetOverlappingBall(combat->equippedBall); Can be useful for bungee gum ability 
	//		//OnRep_OverlappingBall(combat->equippedBall);
	//		bThrown = true;
	//		//if (combat->equippedBall->GetBallState() == EBallState::EBS_Equipped)
	//		//	bCanThrow = true;

	//		combat->equippedBall->GetBallMesh()->SetSimulatePhysics(true);

	//		//combat->equippedBall->GetBallMesh()->AddForce(GetActorLocation() + (/*GetActorUpVector() +*/ GetActorForwardVector()) * throwPower * 75/*combat->equippedBall->GetBallMesh()->GetMass()*/);
	//	//	combat->equippedBall->GetBallMesh()->AddForce(GetActorLocation() + (followCamera->GetForwardVector()  +followCamera->GetUpVector()) * throwPower * 75);
	//		overlappingBall->GetBallMesh()->AddForce(GetActorForwardVector() + (followCamera->GetForwardVector() /*+ followCamera->GetUpVector()*/)  *throwPower * 1000);

	//		bEquipped = false;
	//		//overlappingBall->EventThrowing();
	//		//overlappingBall->FindHomingTarget();

	//		combat->equippedBall = nullptr;


	//		//combat->equippedBall->FindHomingTarget();
	//		//combat->eqippedBall->GetBallMesh()->AddImpulse(UKismetMathLibrary::GetForwardVector(GetControlRotation()));

	//		//combat->equippedBall->GetBallMesh()->AddForce(forwardVec * throwPower * combat->equippedBall->GetBallMesh()->GetMass());
	//	}
	//	else
	//	{
	//		ServerThrowButtonReleased();
	//	}

	//}

}





void ACPPTestCharacter::ServerThrowButtonReleased_Implementation()
{
	//if (combat && IsBallEquipped())
	//{
	//	UKismetMathLibrary::GetForwardVector(GetControlRotation()) *= throwPower;

	//	const FVector forwardVec = this->GetMesh()->GetForwardVector();


	//	combat->ThrowButtonPressed(false); //gded
	//	combat->equippedBall->GetBallMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	//	combat->equippedBall->SetBallState(EBallState::EBS_Dropped);

	//	//combat->eqippedBall->DetachRootComponentFromParent(true);

	//	//combat->eqippedBall->GetBallMesh()->SetPhysicsLinearVelocity(FVector::ForwardVector);

	//	//SetOverlappingBall(combat->equippedBall);  Can be useful for bungee gum ability 
	//	//OnRep_OverlappingBall(combat->equippedBall);
	//	bThrown = true;
	//	//if (combat->equippedBall->GetBallState() == EBallState::EBS_Equipped)
	//	//	bCanThrow = true;

	//	combat->equippedBall->GetBallMesh()->SetSimulatePhysics(true);

	//	//combat->equippedBall->GetBallMesh()->AddForce(GetActorLocation() + (/*GetActorUpVector() +*/ GetActorForwardVector()) * throwPower * 75/*combat->equippedBall->GetBallMesh()->GetMass()*/);
	//	bEquipped = false;
	//	//combat->equippedBall->GetBallMesh()->AddForce(GetActorLocation() + (followCamera->GetForwardVector()  +followCamera->GetUpVector()) * throwPower * 75);
	//	overlappingBall->GetBallMesh()->AddForce(GetActorForwardVector() + (followCamera->GetForwardVector()  /*+followCamera->GetUpVector()*/)  *throwPower * 1000);
	//	//overlappingBall->FindHomingTarget();

	//	//TODO: set equippedball to null

	//	combat->equippedBall = nullptr;

	//	//	combat->equippedBall->GetBallMesh()->AddImpulse(UKismetMathLibrary::GetForwardVector(GetControlRotation()));

	//		//combat->equippedBall->GetBallMesh()->AddForce(forwardVec * throwPower * combat->equippedBall->GetBallMesh()->GetMass());
	//}
}
#pragma endregion


void ACPPTestCharacter::Catch()
{
	//TODO
	// -> implement Catch Mechanics

	if (!bCatching) {

		if (HasAuthority())
		{

			if (CatchAnim)
			{
				MulticastPlayAnimMontage(CatchAnim, 1, NAME_None);
				//bEquipped = true;


			}
			bCatching = true;
			FTimerHandle TimerHandle;
			GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ACPPTestCharacter::CanCatch, CatchCooldown);

		}
		else
		{
			ServerCatch();
		}
	}
}

void ACPPTestCharacter::ServerCatch_Implementation()
{
	//TODO
	// -> Catch Replication (must be fixed)
	
		if (CatchAnim)
		{
			PlayAnimMontage(CatchAnim, 1, NAME_None);
			ServerPlayAnimMontage(CatchAnim, 1, NAME_None);
			//bEquipped = true;

		}

		bCatching = true;
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ACPPTestCharacter::CanCatch, CatchCooldown);
	
}

void ACPPTestCharacter::OnBallReleased()
{
	UE_LOG(LogTemp, Warning, TEXT("md5lsh"));
	if (combat)
	{
		UE_LOG(LogTemp, Warning, TEXT("d5l ?"));
		//overlappingBall->OnReleased();
		//TODO
		// 1- Stop Velocity
		// 2- Add impulse
	}
}


void ACPPTestCharacter::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// BallHit;
	//TODO: overlapping ball to be removed and/or change name

	if (ACPPBall* BallHit = Cast<ACPPBall>(OtherActor))
	{

		if (bCatching && combat && !IsBallEquipped())
		{
			combat->EquipBall(BallHit);
			bCanThrow = true;
			
		}

	}
}

void ACPPTestCharacter::MyThrow()
{
	/*if (HasAuthority())*/
	{
		UKismetMathLibrary::GetForwardVector(GetControlRotation()) *= throwPower;
		const FVector forwardVec = this->GetMesh()->GetForwardVector();
		combat->ThrowButtonPressed(false); //gded
		combat->equippedBall->GetBallMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		combat->equippedBall->SetBallState(EBallState::EBS_Dropped);
		bThrown = true;
		combat->equippedBall->GetBallMesh()->SetSimulatePhysics(true);

		if (lockOnTargets->GetTarget())
		{
		combat->equippedBall->GetBallMesh()->AddForce((((lockOnTargets->GetTarget()->GetTargetLocation()) - (GetActorLocation())) + GetActorUpVector()) * throwPower * 25);
		}
		else
		{
		combat->equippedBall->GetBallMesh()->AddForce((followCamera->GetForwardVector() + GetActorUpVector()) * throwPower * 2500);
		}
		//combat->equippedBall->GetBallMesh()->AddForce(  throwPower * 25);
		//combat->equippedBall->ProjectileMovementComponent->bIsHomingProjectile = true;
		//combat->equippedBall->ProjectileMovementComponent->HomingTargetComponent = Cast<USceneComponent>(lockOnTargets->GetTarget());
		//combat->equippedBall->GetBallMesh()->AddForce((((GetActorLocation())) + followCamera->GetForwardVector() + GetActorUpVector()) * throwPower * 25);
		bEquipped = false;
		combat->equippedBall = nullptr;
		OnBallReleased();
	}
	/*else
	{
		MyServerThrow();
	}*/
	
}

void ACPPTestCharacter::MyLockedThrow()
{
	/*if (HasAuthority())*/
	{
		UKismetMathLibrary::GetForwardVector(GetControlRotation()) *= throwPower;
		const FVector forwardVec = this->GetMesh()->GetForwardVector();
		combat->ThrowButtonPressed(false); //gded
		combat->equippedBall->GetBallMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		combat->equippedBall->SetBallState(EBallState::EBS_Dropped);
		bThrown = true;
		combat->equippedBall->GetBallMesh()->SetSimulatePhysics(true);
		//	combat->equippedBall->GetBallMesh()->AddForce(/*GetActorLocation() +*/ (/*followCamera->GetForwardVector() +*/ GetActorForwardVector() + 0.1 * (GetActorUpVector())) * throwPower * 25);
			//combat->equippedBall->GetBallMesh()->AddForce(  throwPower * 25);
			//combat->equippedBall->ProjectileMovementComponent->bIsHomingProjectile = true;
			//combat->equippedBall->ProjectileMovementComponent->HomingTargetComponent = Cast<USceneComponent>(lockOnTargets->GetTarget());
		combat->equippedBall->GetBallMesh()->AddForce((((lockOnTargets->GetTarget()->GetTargetLocation()) - (GetActorLocation())) + GetActorUpVector()) * throwPower * 25);


		bEquipped = false;
		combat->equippedBall = nullptr;
		OnBallReleased();
	}
	/*else
	{
		MyServerLockedThrow();
	}*/
}

//void ACPPTestCharacter::MyServerLockedThrow_Implementation()
//{
//	UKismetMathLibrary::GetForwardVector(GetControlRotation()) *= throwPower;
//	const FVector forwardVec = this->GetMesh()->GetForwardVector();
//	combat->ThrowButtonPressed(false); //gded
//	combat->equippedBall->GetBallMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
//	combat->equippedBall->SetBallState(EBallState::EBS_Dropped);
//	bThrown = true;
//	combat->equippedBall->GetBallMesh()->SetSimulatePhysics(true);
//	//	combat->equippedBall->GetBallMesh()->AddForce(/*GetActorLocation() +*/ (/*followCamera->GetForwardVector() +*/ GetActorForwardVector() + 0.1 * (GetActorUpVector())) * throwPower * 25);
//		//combat->equippedBall->GetBallMesh()->AddForce(  throwPower * 25);
//		//combat->equippedBall->ProjectileMovementComponent->bIsHomingProjectile = true;
//		//combat->equippedBall->ProjectileMovementComponent->HomingTargetComponent = Cast<USceneComponent>(lockOnTargets->GetTarget());
//	combat->equippedBall->GetBallMesh()->AddForce((((lockOnTargets->GetTarget()->GetTargetLocation()) - (GetActorLocation())) + GetActorUpVector()) * throwPower * 25);
//
//
//	bEquipped = false;
//	combat->equippedBall = nullptr;
//	OnBallReleased();
//}

//void ACPPTestCharacter::MyServerThrow_Implementation()
//{
//	UKismetMathLibrary::GetForwardVector(GetControlRotation()) *= throwPower;
//	const FVector forwardVec = this->GetMesh()->GetForwardVector();
//	combat->ThrowButtonPressed(false); //gded
//	combat->equippedBall->GetBallMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
//	combat->equippedBall->SetBallState(EBallState::EBS_Dropped);
//	bThrown = true;
//	combat->equippedBall->GetBallMesh()->SetSimulatePhysics(true);
//	combat->equippedBall->GetBallMesh()->AddForce(/*GetActorLocation() +*/ (/*followCamera->GetForwardVector() +*/ GetActorForwardVector() + 0.1 * (GetActorUpVector())) * throwPower * 25);
//	//combat->equippedBall->GetBallMesh()->AddForce(  throwPower * 25);
//	//combat->equippedBall->ProjectileMovementComponent->bIsHomingProjectile = true;
//	//combat->equippedBall->ProjectileMovementComponent->HomingTargetComponent = Cast<USceneComponent>(lockOnTargets->GetTarget());
////combat->equippedBall->GetBallMesh()->AddForce((((GetActorLocation())) + followCamera->GetForwardVector() + GetActorUpVector()) * throwPower * 25);
//	bEquipped = false;
//	combat->equippedBall = nullptr;
//	OnBallReleased();
//}

void ACPPTestCharacter::SetOverlappingBall(ACPPBall* cppBall)
{
	if (overlappingBall)
	{
		overlappingBall->ShowPickupWidget(false);
	}
	overlappingBall = cppBall;
	if (IsLocallyControlled())
	{
		if (overlappingBall)
		{
			overlappingBall->ShowPickupWidget(true);
			//overlappingBall->DisableComponentsSimulatePhysics();
		}
	}
}

void ACPPTestCharacter::OnRep_OverlappingBall(ACPPBall* lastBall)
{
	if (overlappingBall)
	{
		overlappingBall->ShowPickupWidget(true);
		//overlappingBall->DisableComponentsSimulatePhysics();
	}
	if (lastBall)
	{
		lastBall->ShowPickupWidget(false);
	}
}

#pragma region Animations
bool ACPPTestCharacter::IsBallEquipped()
{
	
	return (combat && combat->equippedBall);
	//return(bEquipped && );

	/*if(overlappingBall->ballState == EBallState::EBS_Equipped)
		return(true);
	else
	{
		return false;
	}*/
	//return(combat->equippedBall->GetBallState() == EBallState::EBS_Equipped);

}

void ACPPTestCharacter::StopThrow()
{
	bThrown = false;
	bCanThrow = false;
}


void ACPPTestCharacter::PlayThrowMontage()
{
	if (combat == nullptr || combat->equippedBall == nullptr) return;

	UAnimInstance* animInstance = GetMesh()->GetAnimInstance();
	if (animInstance && throwAnim)
	{
		animInstance->Montage_Play(throwAnim);
		ServerPlayAnimMontage(throwAnim);
	}
}

void ACPPTestCharacter::ServerPlayAnimMontage_Implementation(UAnimMontage* AnimMontage, float InPlayRate,
	FName StartSectionName)
{
	MulticastPlayAnimMontage(AnimMontage, InPlayRate, StartSectionName);
}

bool ACPPTestCharacter::ServerPlayAnimMontage_Validate(UAnimMontage* AnimMontage, float InPlayRate,
	FName StartSectionName)
{
	return true;
}

void ACPPTestCharacter::MulticastPlayAnimMontage_Implementation(UAnimMontage* AnimMontage, float InPlayRate, FName StartSectionName)
{
	PlayAnimMontage(AnimMontage, InPlayRate, StartSectionName);
}

bool ACPPTestCharacter::MulticastPlayAnimMontage_Validate(UAnimMontage* AnimMontage, float InPlayRate,
	FName StartSectionName)
{
	return true;
}



#pragma endregion



